// render_smoke_test.cpp — end-to-end: parse the bundled scene, extract, render to
// a CPU framebuffer, and assert the image is non-empty (geometry actually drew),
// for BOTH the material-model path and the GLSL-interpreter author path.
#include "RenderItem.hpp"
#include "SceneExtractor.hpp"
#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DParse.hpp"
#include "X3DSceneBridge.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"

#include "cpuraster/Framebuffer.hpp"
#include "cpuraster/GlslInterpreter.hpp"
#include "cpuraster/SceneRender.hpp"

#include <cmath>
#include <any>
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

static void setF(const std::shared_ptr<x3d::nodes::X3DNode> &n,
                 const char *name, std::any value) {
  for (auto &f : n->fields())
    if (f.x3dName == name && f.set) { f.set(*n, std::move(value)); return; }
}

static cr::Framebuffer renderFillShape(const char *geometryType, int mode) {
  using namespace x3d::core;
  using namespace x3d::nodes;
  auto geometry = createX3DNode(geometryType);
  if (std::string(geometryType) != "Box") {
    auto coord = createX3DNode("Coordinate");
    setF(coord, "point", std::any(MFVec3f{{-1, 0, 0}, {1, 0, 0}, {0, 1, 0}}));
    setF(geometry, "coord", std::any(std::shared_ptr<X3DNode>(coord)));
    if (std::string(geometryType) == "IndexedLineSet")
      setF(geometry, "coordIndex", std::any(MFInt32{0, 1, -1}));
  }
  auto app = createX3DNode("Appearance");
  auto material = createX3DNode("UnlitMaterial");
  setF(material, "emissiveColor", std::any(SFColor{0, 1, 0}));
  setF(app, "material", std::any(std::shared_ptr<X3DNode>(material)));
  if (mode != 0) {
    auto fill = createX3DNode("FillProperties");
    setF(fill, "filled", std::any(SFBool{false}));
    setF(fill, "hatched", std::any(SFBool{mode == 2}));
    setF(fill, "hatchColor", std::any(SFColor{1, 0, 0}));
    setF(app, "fillProperties", std::any(std::shared_ptr<X3DNode>(fill)));
  }
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(geometry)));
  setF(shape, "appearance", std::any(std::shared_ptr<X3DNode>(app)));
  x3d::runtime::Scene scene; scene.addRootNode(shape);
  x3d::runtime::X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene); ctx.buildFrom(scene); ctx.tick(0.0);
  ex::SceneExtractor extractor(ctx, scene); extractor.fullSnapshot();
  cr::RenderOptions opt; opt.width = 96; opt.height = 96;
  opt.clearColor = {0, 0, 0};
  return cr::renderScene(ctx, extractor, opt);
}

// REQ-LOCALFOG consumer check: a green Box under a disabled global Fog (range 0)
// and, when withLocalFog, inside a Group whose LocalFog is red with a tiny
// visibilityRange so the surface saturates to the LocalFog colour. Without the
// LocalFog the Box stays its unfogged green.
static void addChild(const std::shared_ptr<x3d::nodes::X3DNode> &p,
                     const std::shared_ptr<x3d::nodes::X3DNode> &c) {
  for (auto &f : p->fields())
    if (f.x3dName == "children" && f.set) {
      auto k = std::any_cast<std::vector<std::shared_ptr<x3d::nodes::X3DNode>>>(f.get(*p));
      k.push_back(c);
      f.set(*p, std::any(std::move(k)));
      return;
    }
}

static cr::Framebuffer renderLocalFogShape(bool withLocalFog) {
  using namespace x3d::core;
  using namespace x3d::nodes;
  auto geometry = createX3DNode("Box");
  auto app = createX3DNode("Appearance");
  auto material = createX3DNode("UnlitMaterial");
  setF(material, "emissiveColor", std::any(SFColor{0, 1, 0}));
  setF(app, "material", std::any(std::shared_ptr<X3DNode>(material)));
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(geometry)));
  setF(shape, "appearance", std::any(std::shared_ptr<X3DNode>(app)));

  auto fog = createX3DNode("Fog"); // disabled global: range 0 => no fog.
  setF(fog, "color", std::any(SFColor{0, 0, 1}));
  setF(fog, "visibilityRange", std::any(0.0f));

  x3d::runtime::Scene scene;
  scene.addRootNode(fog);
  if (withLocalFog) {
    auto group = createX3DNode("Group");
    auto lf = createX3DNode("LocalFog");
    setF(lf, "color", std::any(SFColor{1, 0, 0}));
    setF(lf, "visibilityRange", std::any(0.001f));
    addChild(group, lf);
    addChild(group, shape);
    scene.addRootNode(group);
  } else {
    scene.addRootNode(shape);
  }
  x3d::runtime::X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene); ctx.buildFrom(scene); ctx.tick(0.0);
  ex::SceneExtractor extractor(ctx, scene); extractor.fullSnapshot();
  cr::RenderOptions opt; opt.width = 96; opt.height = 96;
  opt.clearColor = {0, 0, 0};
  return cr::renderScene(ctx, extractor, opt);
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

  // Composed scene path: extraction -> material shader -> polygon hatch mask.
  {
    auto normal = renderFillShape("Box", 0);
    auto empty = renderFillShape("Box", 1);
    auto hatch = renderFillShape("Box", 2);
    const g::vec3 black{0, 0, 0};
    CHECK(drawnPixels(normal, black) > 100);
    CHECK(drawnPixels(empty, black) == 0);
    CHECK(drawnPixels(hatch, black) > 0);
    CHECK(drawnPixels(hatch, black) < drawnPixels(normal, black));
    for (const char *type : {"IndexedLineSet", "PointSet"}) {
      auto absent = renderFillShape(type, 0);
      auto noFill = renderFillShape(type, 1);
      CHECK(drawnPixels(absent, black) > 0);
      CHECK(absent.pixels() == noFill.pixels());
    }
  }

  // §24.4.3 local fog vs global fog at the fragment.
  {
    auto noFog = renderLocalFogShape(false);
    auto localFog = renderLocalFogShape(true);
    const g::vec4 a = noFog.colorAt(48, 48);    // centre of the unfogged Box.
    const g::vec4 b = localFog.colorAt(48, 48); // centre of the LocalFog Box.
    CHECK(a.y > 0.5f && a.x < 0.2f);       // green material, no fog.
    CHECK(b.x > 0.5f && b.y < 0.2f);       // saturated to the red LocalFog.
  }

  if (failures) { std::fprintf(stderr, "render_smoke_test: %d failure(s)\n", failures); return 1; }
  std::printf("render_smoke_test: OK\n");
  return 0;
}
