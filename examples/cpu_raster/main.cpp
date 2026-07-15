// examples/cpu_raster/main.cpp
//
// Headless CPU rasterizer — an out-of-SDK CONSUMER of the x3d_cpp extraction
// seam. Unlike the PoC OpenGL renderer it needs NO GPU, NO GLFW, NO display and
// NO image library: it parses an X3D scene, extracts RenderItems, rasterizes them
// on the CPU through the GLSL-emulation pipeline, and writes a PPM. That makes it
// a dependency-free golden-image / visual-regression harness that runs on a bare
// CI runner.
//
// It supports ALL THREE material models (Phong / Physical / Unlit) and a GLSL
// emulation layer: the fixed-function shaders are CPU ports of the PoC GLSL, and
// `--frag <file>` runs an author GLSL fragment shader through the interpreter
// (GlslInterpreter.hpp) against the extracted geometry — the ComposedShader path.
//
// USAGE:
//   x3d_cpu_raster [options] [scene.x3d]
//     -o, --out <file.ppm>   output image (default: cpu_raster.ppm)
//     -w, --width <px>       image width  (default 800)
//     -H, --height <px>      image height (default 600)
//     --frag <file.glsl>     run an author fragment shader via the interpreter
//     --headless             extract + self-check only; print counts, no image
//   With no scene argument the bundled assets/raster_smoke.x3d is used.

#include "GeometryBounds.hpp"
#include "RenderItem.hpp"
#include "SceneExtractor.hpp"
#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DParse.hpp"
#include "X3DSceneBridge.hpp"

#include "cpuraster/BuiltinFont.hpp"
#include "cpuraster/Framebuffer.hpp"
#include "cpuraster/GlslInterpreter.hpp"
#include "cpuraster/MaterialShader.hpp"
#include "cpuraster/ProceduralTexture.hpp"
#include "cpuraster/SceneRender.hpp"
#include "cpuraster/glsl.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace ex = x3d::runtime::extract;
namespace cr = x3d::cpuraster;
using namespace x3d::core;   // MFString etc. (ADR-0039 namespaces)
using namespace x3d::nodes;  // X3DNode (ADR-0039 namespaces)

namespace {

std::string readTextFile(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return {};
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

// Directory portion of a path (so a relative texture url resolves next to the
// scene), mirroring the PoC's dirOf.
std::string dirOf(const std::string &path) {
  auto slash = path.find_last_of("/\\");
  return (slash == std::string::npos) ? std::string(".") : path.substr(0, slash);
}

// Dump the built-in procedural textures as PPM files into `dir` for inspection.
int genTextures(const std::string &dir) {
  const std::pair<const char *, ex::TexturePixels> set[] = {
      {"checker", cr::makeChecker()},   {"uvgrid", cr::makeUvGrid()},
      {"gradient", cr::makeGradient()}, {"brick", cr::makeBrick()},
      {"bars", cr::makeColorBars()}};
  for (const auto &[name, tex] : set) {
    const std::string path = dir + "/" + name + ".ppm";
    if (!cr::writePpm(path, tex)) {
      std::fprintf(stderr, "[cpu-raster] failed to write %s\n", path.c_str());
      return 1;
    }
    std::fprintf(stderr, "[cpu-raster] wrote %s (%ux%u)\n", path.c_str(),
                 tex.width, tex.height);
  }
  return 0;
}

} // namespace

int main(int argc, char **argv) {
  std::string scenePath;
  std::string outPath = "cpu_raster.png"; // PNG by default; .ppm still honored.
  std::string fragPath;
  std::string genTexDir; // --gentex <dir>: dump procedural textures and exit.
  int width = 800, height = 600;
  bool headless = false;
  bool animate = false;
  int fps = 30;
  double duration = 4.0;
  std::string framesDir;

  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if ((a == "-o" || a == "--out") && i + 1 < argc) outPath = argv[++i];
    else if ((a == "-w" || a == "--width") && i + 1 < argc) width = std::atoi(argv[++i]);
    else if ((a == "-H" || a == "--height") && i + 1 < argc) height = std::atoi(argv[++i]);
    else if (a == "--frag" && i + 1 < argc) fragPath = argv[++i];
    else if (a == "--gentex" && i + 1 < argc) genTexDir = argv[++i];
    else if (a == "--headless") headless = true;
    else if (a == "--animate") animate = true;
    else if (a == "--fps" && i + 1 < argc) fps = std::atoi(argv[++i]);
    else if (a == "--duration" && i + 1 < argc) duration = std::atof(argv[++i]);
    else if (a == "--frames-dir" && i + 1 < argc) framesDir = argv[++i];
    else if (!a.empty() && a[0] != '-') scenePath = a;
  }

  if (!genTexDir.empty()) return genTextures(genTexDir);
  if (scenePath.empty())
    scenePath = std::string(X3D_CPURASTER_ASSET_DIR) + "/raster_smoke.x3d";

  std::fprintf(stderr, "[cpu-raster] loading %s\n", scenePath.c_str());

  x3d::runtime::X3DDocument doc;
  try {
    doc = x3d::codec::parseFile(scenePath);
  } catch (const std::exception &e) {
    std::fprintf(stderr, "[cpu-raster] parse failed: %s\n", e.what());
    return 1;
  }

  x3d::runtime::Scene &scene = doc.getScene();
  x3d::runtime::X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  x3d::runtime::BridgeResult bridge = ctx.buildFrom(scene);
  if (!bridge.ok())
    std::fprintf(stderr, "[cpu-raster] scene bridge: %zu rejected route(s)\n",
                 bridge.rejected.size());
  if (animate) x3d::runtime::attachStandardRuntime(scene, ctx);
  ctx.tick(0.0); // resolve bindings + initial transforms/bounds.

  // Built-in glyph font so Text nodes render as readable letters: its metrics
  // (with atlas UVs) go to the extractor, its atlas is bound in the glyph draw.
  cr::BuiltinFont font = cr::makeBuiltinFont();
  ex::MeshBuildOptions meshOpts;
  meshOpts.fontMetrics = font.metrics;
  // Texture resolver: synthesizes "proc:<name>" textures and decodes ".ppm"
  // files next to the scene, so ImageTexture scenes render with no image library.
  ex::SceneExtractor extractor(ctx, scene, meshOpts,
                               cr::makeTextureResolver(dirOf(scenePath)));
  ex::RenderDelta snap = extractor.fullSnapshot();
  const std::size_t items = extractor.itemCount();

  if (headless) {
    std::size_t firstVerts =
        items ? extractor.item(snap.added.front()).mesh->positions.size() : 0;
    std::size_t phong = 0, physical = 0, unlit = 0, transparent = 0, textured = 0;
    for (ex::RenderItemId id = 0; id < items; ++id) {
      const ex::RenderItem &it = extractor.item(id);
      switch (it.material.model) {
        case ex::MaterialModel::Phong: ++phong; break;
        case ex::MaterialModel::Physical: ++physical; break;
        case ex::MaterialModel::Unlit: ++unlit; break;
      }
      if (it.material.alphaMode == ex::AlphaMode::Blend ||
          it.material.transparency > 0.0f)
        ++transparent;
      if (!it.material.textures.empty()) ++textured;
    }
    std::printf("render_items=%zu first_item_vertices=%zu phong=%zu physical=%zu "
                "unlit=%zu transparent=%zu textured=%zu\n",
                items, firstVerts, phong, physical, unlit, transparent, textured);
    if (items == 0) {
      std::fprintf(stderr, "[cpu-raster] NO render items extracted\n");
      return 1;
    }
    return 0;
  }

  cr::RenderOptions opt;
  opt.width = width;
  opt.height = height;
  opt.glyphAtlas = &font.atlas;

  // Skybox: resolve the bound Background's six panorama faces (if any) through
  // the same proc:/.ppm/.png resolver used for ImageTexture, then bind them.
  cr::render_detail::SkyboxTextures skybox;
  {
    ex::TextureResolver texResolve = cr::makeTextureResolver(dirOf(scenePath));
    if (const X3DNode *bg = ctx.boundBackground()) {
      auto faceTex = [&](const char *field) -> cr::Texture {
        auto urls = x3d::runtime::geombounds::getField<MFString>(*bg, field, {});
        for (const std::string &u : urls) { // ordered fallback
          ex::TexturePixelResult r = texResolve(u);
          if (r.ready() && !r.pixels.rgba.empty()) {
            const auto &p = r.pixels;
            // srgb=false: the skybox is written straight to the display
            // framebuffer (no lighting, no shader gamma re-encode), so sample
            // the faces as display-referred — not decoded to linear.
            return cr::Texture::fromRGBA8(p.rgba.data(), static_cast<int>(p.width),
                                          static_cast<int>(p.height), true, true,
                                          /*srgb=*/false);
          }
        }
        return {};
      };
      skybox.front = faceTex("frontUrl");
      skybox.back = faceTex("backUrl");
      skybox.right = faceTex("rightUrl");
      skybox.left = faceTex("leftUrl");
      skybox.top = faceTex("topUrl");
      skybox.bottom = faceTex("bottomUrl");
    }
  }
  if (skybox.any()) opt.skybox = &skybox;

  // --frag: compile an author GLSL fragment shader once and apply it to every
  // item via the interpreter (a standalone demo of the ComposedShader path; when
  // the SDK wires RenderItem::shaderProgram this same machinery binds it).
  cr::InterpretedProgram authorProg;
  bool haveAuthor = false;
  if (!fragPath.empty()) {
    std::string src = readTextFile(fragPath);
    if (src.empty()) {
      std::fprintf(stderr, "[cpu-raster] cannot read --frag %s\n", fragPath.c_str());
      return 1;
    }
    std::string err;
    if (!authorProg.compile(src, &err)) {
      std::fprintf(stderr, "[cpu-raster] GLSL compile error: %s\n", err.c_str());
      return 1;
    }
    haveAuthor = true;
    std::fprintf(stderr, "[cpu-raster] author fragment shader compiled (%s)\n",
                 fragPath.c_str());
  }

  if (haveAuthor) {
    opt.authorShaderFor =
        [&authorProg](const ex::RenderItem &it,
                      const std::vector<cr::EyeLight> &lights,
                      bool hasColors) -> cr::FragmentShader {
      return cr::makeInterpretedShader(authorProg, it.material, lights, hasColors);
    };
  }

  if (animate) {
    if (fps <= 0 || duration <= 0.0 || framesDir.empty()) {
      std::fprintf(stderr, "[cpu-raster] --animate needs --fps>0, --duration>0, "
                           "and --frames-dir <dir>\n");
      return 1;
    }
    if (framesDir.back() == '/') framesDir.pop_back(); // avoid "dir//frame_..."
    const int nFrames = static_cast<int>(std::lround(duration * fps));
    if (nFrames == 0) {
      std::fprintf(stderr, "[cpu-raster] --animate: duration*fps rounds to 0 "
                           "frames; nothing to render\n");
      return 1;
    }
    for (int f = 0; f < nFrames; ++f) {
      ctx.tick(static_cast<double>(f) / fps);
      extractor.fullSnapshot();
      cr::Framebuffer frame = cr::renderScene(ctx, extractor, opt);
      char suffix[32];
      std::snprintf(suffix, sizeof suffix, "/frame_%04d.ppm", f);
      const std::string p = framesDir + suffix;
      if (!frame.writePPM(p)) {
        std::fprintf(stderr, "[cpu-raster] failed to write %s\n", p.c_str());
        return 1;
      }
    }
    std::fprintf(stderr, "[cpu-raster] wrote %d frame(s) -> %s\n", nFrames,
                 framesDir.c_str());
    return 0;
  }

  cr::Framebuffer fb = cr::renderScene(ctx, extractor, opt);
  // Dispatch by extension: PNG (default) via the vendored encoder, else PPM.
  auto endsWith = [&](const char *e) {
    const std::string s(e);
    return outPath.size() >= s.size() &&
           outPath.compare(outPath.size() - s.size(), s.size(), s) == 0;
  };
  const bool wrote = endsWith(".ppm") ? fb.writePPM(outPath) : fb.writePNG(outPath);
  if (!wrote) {
    std::fprintf(stderr, "[cpu-raster] failed to write %s\n", outPath.c_str());
    return 1;
  }
  std::fprintf(stderr, "[cpu-raster] rendered %zu item(s) -> %s (%dx%d)\n", items,
               outPath.c_str(), width, height);
  return 0;
}
