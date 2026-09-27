// scene_extractor_fog_test.cpp — ENV-10: the bound Fog is surfaced on the
// extraction seam as a FogDesc (§24.4.2), mirroring background().
//
//   1) No bound Fog -> FogDesc defaults (white, LINEAR, range 0 = disabled).
//   2) A bound Fog's color/fogType/visibilityRange are surfaced verbatim.
//   3) visibilityRange is in the Fog node's LOCAL frame: a world Transform
//      scale multiplies it (uniform scale exact).
//   4) delta()/fullSnapshot() flag fogChanged for a caching consumer.
#include "SceneExtractor.hpp"

#include "X3DDocument.hpp" // Scene::addRootNode definition.
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DScene.hpp"

#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
static void addChild(const std::shared_ptr<X3DNode> &p,
                     const std::shared_ptr<X3DNode> &c) {
  for (auto &f : p->fields())
    if (f.x3dName == "children" && f.set) {
      auto k = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*p));
      k.push_back(c);
      f.set(*p, std::any(std::move(k)));
      return;
    }
}

TEST_CASE("scene_extractor_fog_test") {
  // === 1) No bound Fog -> defaults, range 0 disables fog ====================
  {
    auto tri = createX3DNode("TriangleSet");
    Scene scene;
    scene.addRootNode(tri);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);
    auto fog = ex.fog();
    CHECK((fog.visibilityRange == 0.0f)); // disabled.
    CHECK((fog.fogType == extract::FogDesc::Type::Linear));
    CHECK((feq(fog.color.r, 1.0f) && feq(fog.color.g, 1.0f) && feq(fog.color.b, 1.0f)));
    CHECK((fog.fogChanged));
  }

  // === 2) Bound Fog surfaced verbatim (color/type/range) ====================
  {
    auto fogNode = createX3DNode("Fog");
    setF(fogNode, "color", std::any(SFColor{0.25f, 0.5f, 0.75f}));
    setF(fogNode, "fogType", std::any(FogTypeChoices::EXPONENTIAL));
    setF(fogNode, "visibilityRange", std::any(20.0f));
    Scene scene;
    scene.addRootNode(fogNode);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);
    auto fog = ex.fog();
    CHECK((feq(fog.color.r, 0.25f) && feq(fog.color.g, 0.5f) && feq(fog.color.b, 0.75f)));
    CHECK((fog.fogType == extract::FogDesc::Type::Exponential));
    CHECK((feq(fog.visibilityRange, 20.0f))); // no transform: local == world.
  }

  // === 3) visibilityRange is LOCAL: a Transform scale multiplies it =========
  {
    auto xform = createX3DNode("Transform");
    setF(xform, "scale", std::any(SFVec3f{2.0f, 2.0f, 2.0f}));
    auto fogNode = createX3DNode("Fog");
    setF(fogNode, "visibilityRange", std::any(10.0f));
    addChild(xform, fogNode);
    Scene scene;
    scene.addRootNode(xform);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);
    auto fog = ex.fog();
    CHECK((feq(fog.visibilityRange, 20.0f))); // 10 * world scale 2.
  }

  // === 4) fogChanged surfaced by both delta channels =======================
  {
    auto fogNode = createX3DNode("Fog");
    Scene scene;
    scene.addRootNode(fogNode);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);
    auto full = ex.fullSnapshot();
    CHECK((full.fogChanged));
    ctx.tick(0.0);
    auto delta = ex.delta();
    CHECK((delta.fogChanged));
  }
}
