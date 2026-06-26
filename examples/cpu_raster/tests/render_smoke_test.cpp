// render_smoke_test.cpp — end-to-end: parse the bundled scene, extract, render to
// a CPU framebuffer, and assert the image is non-empty (geometry actually drew),
// for BOTH the material-model path and the GLSL-interpreter author path.
#include "RenderItem.hpp"
#include "SceneExtractor.hpp"
#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DParse.hpp"
#include "X3DSceneBridge.hpp"

#include "cpuraster/Framebuffer.hpp"
#include "cpuraster/GlslInterpreter.hpp"
#include "cpuraster/SceneRender.hpp"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

namespace ex = x3d::runtime::extract;
namespace cr = x3d::cpuraster;
namespace g = x3d::cpuraster::glsl;

static int failures = 0;
#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::fprintf(stderr, "FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);     \
      ++failures;                                                              \
    }                                                                          \
  } while (0)

// Count pixels that differ meaningfully from the clear color (= drawn geometry).
static int drawnPixels(const cr::Framebuffer &fb, g::vec3 clear) {
  int n = 0;
  for (int y = 0; y < fb.height(); ++y)
    for (int x = 0; x < fb.width(); ++x) {
      g::vec4 c = fb.colorAt(x, y);
      if (std::fabs(c.x - clear.x) + std::fabs(c.y - clear.y) +
              std::fabs(c.z - clear.z) > 0.05f)
        ++n;
    }
  return n;
}

int main() {
  const std::string scene =
      std::string(X3D_CPURASTER_ASSET_DIR) + "/raster_smoke.x3d";

  x3d::runtime::X3DDocument doc;
  try {
    doc = x3d::codec::parseFile(scene);
  } catch (const std::exception &e) {
    std::fprintf(stderr, "parse failed: %s\n", e.what());
    return 1;
  }
  x3d::runtime::Scene &sc = doc.getScene();
  x3d::runtime::X3DExecutionContext ctx;
  ctx.buildSceneGraph(sc);
  ctx.buildFrom(sc);
  ctx.tick(0.0);

  ex::SceneExtractor extractor(ctx, sc);
  extractor.fullSnapshot();
  CHECK(extractor.itemCount() >= 5); // five shapes in the smoke scene.

  // Material-model render.
  cr::RenderOptions opt;
  opt.width = 96; opt.height = 64;
  cr::Framebuffer fb = cr::renderScene(ctx, extractor, opt);
  g::vec3 clear{0.08f, 0.10f, 0.16f}; // matches the scene Background.
  int drawn = drawnPixels(fb, clear);
  std::fprintf(stderr, "material render: %d drawn pixels\n", drawn);
  CHECK(drawn > 200); // geometry visibly covers a chunk of the frame.

  // Author-shader (GLSL interpreter) render of the same scene.
  std::string fragPath = std::string(X3D_CPURASTER_SHADER_DIR) + "/author_lambert.frag";
  std::ifstream in(fragPath, std::ios::binary);
  std::ostringstream ss; ss << in.rdbuf();
  cr::InterpretedProgram prog;
  std::string err;
  CHECK(prog.compile(ss.str(), &err));
  if (!err.empty()) std::fprintf(stderr, "shader compile: %s\n", err.c_str());

  cr::RenderOptions opt2 = opt;
  opt2.authorShaderFor = [&prog](const ex::RenderItem &it,
                                 const std::vector<cr::EyeLight> &lights,
                                 bool hasColors) -> cr::FragmentShader {
    return cr::makeInterpretedShader(prog, it.material, lights, hasColors);
  };
  cr::Framebuffer fb2 = cr::renderScene(ctx, extractor, opt2);
  int drawn2 = drawnPixels(fb2, clear);
  std::fprintf(stderr, "author-shader render: %d drawn pixels\n", drawn2);
  CHECK(drawn2 > 200);

  if (failures) { std::fprintf(stderr, "render_smoke_test: %d failure(s)\n", failures); return 1; }
  std::printf("render_smoke_test: OK\n");
  return 0;
}
