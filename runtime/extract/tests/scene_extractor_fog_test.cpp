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

// ---------------------------------------------------------------------------
// REQ-LOCALFOG §24.4.3: LocalFog is bound-independent and scoped to its
// enclosing grouping node; enabled==false is skipped so global Fog applies.
// ---------------------------------------------------------------------------
static std::shared_ptr<X3DNode> makeBoxShape() {
  auto shape = createX3DNode("Shape");
  auto geom = createX3DNode("Box");
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(geom)));
  return shape;
}

TEST_CASE("scene_extractor_local_fog_test") {
  // === 1) A LocalFog in a Group fogs only that Group's shapes ===============
  {
    auto group = createX3DNode("Group");
    auto lf = createX3DNode("LocalFog");
    setF(lf, "color", std::any(SFColor{0.1f, 0.2f, 0.3f}));
    setF(lf, "fogType", std::any(FogTypeChoices::EXPONENTIAL));
    setF(lf, "visibilityRange", std::any(15.0f));
    addChild(group, lf);
    addChild(group, makeBoxShape());
    auto outside = makeBoxShape();
    Scene scene;
    scene.addRootNode(group);
    scene.addRootNode(outside);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);
    ex.fullSnapshot();

    CHECK((ex.snapshotLocalFogs().size() == 1));
    const extract::LocalFogDesc &d = ex.snapshotLocalFogs()[0];
    CHECK((feq(d.color.r, 0.1f) && feq(d.color.g, 0.2f) && feq(d.color.b, 0.3f)));
    CHECK((d.fogType == extract::FogDesc::Type::Exponential));
    CHECK((feq(d.visibilityRange, 15.0f)));
    CHECK((d.scopeRoot == group.get()));

    int inScope = 0, outScope = 0;
    for (extract::RenderItemId id = 0; id < ex.itemCount(); ++id) {
      if (ex.item(id).localFog >= 0) ++inScope; else ++outScope;
    }
    CHECK((inScope == 1));  // the grouped Shape.
    CHECK((outScope == 1)); // the sibling Shape: global Fog governs.
  }

  // === 2) enabled==false LocalFog is skipped (global Fog unchanged) =========
  {
    auto group = createX3DNode("Group");
    auto lf = createX3DNode("LocalFog");
    setF(lf, "enabled", std::any(false));
    addChild(group, lf);
    addChild(group, makeBoxShape());
    Scene scene;
    scene.addRootNode(group);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);
    ex.fullSnapshot();
    CHECK((ex.snapshotLocalFogs().empty()));
    CHECK((ex.item(0).localFog == -1));
  }

  // === 3) visibilityRange is LOCAL: a Transform scale multiplies it =========
  {
    auto xform = createX3DNode("Transform");
    setF(xform, "scale", std::any(SFVec3f{2.0f, 2.0f, 2.0f}));
    auto lf = createX3DNode("LocalFog");
    setF(lf, "visibilityRange", std::any(10.0f));
    addChild(xform, lf);
    addChild(xform, makeBoxShape());
    Scene scene;
    scene.addRootNode(xform);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);
    ex.fullSnapshot();
    CHECK((ex.snapshotLocalFogs().size() == 1));
    CHECK((feq(ex.snapshotLocalFogs()[0].visibilityRange, 20.0f))); // 10 * 2.
    CHECK((ex.item(0).localFog == 0));
  }

  // === 4) Nested LocalFogs: the innermost grouping scope wins ===============
  {
    auto outer = createX3DNode("Group");
    auto outerLf = createX3DNode("LocalFog");
    setF(outerLf, "color", std::any(SFColor{0.0f, 0.0f, 1.0f}));
    addChild(outer, outerLf);
    addChild(outer, makeBoxShape()); // sibling of inner group: outer fog applies.
    auto inner = createX3DNode("Group");
    auto innerLf = createX3DNode("LocalFog");
    setF(innerLf, "color", std::any(SFColor{1.0f, 0.0f, 0.0f}));
    addChild(inner, innerLf);
    addChild(inner, makeBoxShape());
    addChild(outer, inner);
    Scene scene;
    scene.addRootNode(outer);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);
    ex.fullSnapshot();
    CHECK((ex.snapshotLocalFogs().size() == 2));
    // Reflect the item's resolved colour: find an item whose LocalFog is the
    // blue outer one, and one whose is the red inner one.
    int blue = 0, red = 0;
    for (extract::RenderItemId id = 0; id < ex.itemCount(); ++id) {
      const int i = ex.item(id).localFog;
      if (i < 0) continue;
      const float r = ex.snapshotLocalFogs()[i].color.r;
      if (feq(r, 1.0f)) ++red; else ++blue;
    }
    CHECK((blue == 1)); // outer-group Shape: nearest is the outer LocalFog.
    CHECK((red == 1));  // inner-group Shape: nearest is the inner LocalFog.
  }
}
