// scene_extractor_fog_test.cpp — ENV-10: the bound Fog is surfaced on the
// extraction seam as a FogDesc (§24.4.2), mirroring background().
//
//   1) No bound Fog -> FogDesc defaults (white, LINEAR, range 0 = disabled).
//   2) A bound Fog's color/fogType/visibilityRange are surfaced verbatim.
//   3) visibilityRange is in the Fog node's LOCAL frame: a world Transform
//      scale multiplies it (uniform scale exact).
//   4) delta()/fullSnapshot() flag fogChanged for a caching consumer.
#include "GeoFrame.hpp"
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

TEST_CASE("local fog placement: a shared enclosing group preserves each world range") {
  auto fog = createX3DNode("LocalFog");
  setF(fog, "visibilityRange", 10.0f);
  auto shared = createX3DNode("Group");
  addChild(shared, fog);
  addChild(shared, makeBoxShape());
  auto left = createX3DNode("Transform"), right = createX3DNode("Transform");
  setF(left, "scale", SFVec3f{2, 2, 2});
  setF(right, "scale", SFVec3f{3, 3, 3});
  addChild(left, shared);
  addChild(right, shared);
  Scene scene;
  SUBCASE("left first") { scene.rootNodes = {left, right}; }
  SUBCASE("right first") { scene.rootNodes = {right, left}; }
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  extract::SceneExtractor ex(ctx, scene);
  const auto snapshot = ex.fullSnapshot();
  REQUIRE(snapshot.added.size() == 2);
  REQUIRE(ex.snapshotLocalFogs().size() == 2);
  for (auto id : snapshot.added) {
    const auto &item = ex.item(id);
    REQUIRE(item.localFog >= 0);
    const auto &desc = ex.snapshotLocalFogs().at(item.localFog);
    const bool isLeft = item.path.front() == left.get();
    CHECK(desc.visibilityRange == doctest::Approx(isLeft ? 20 : 30));
    CHECK(desc.scopeRoot == shared.get());
    CHECK(desc.scopePath == extract::PathKey{item.path.front(), shared.get()});
  }
  // Appending placement identity retains the previous four aggregate fields.
  const extract::LocalFogDesc legacy{SFColor{1, 1, 1}, extract::FogDesc::Type::Linear,
                                     10.0f, shared.get()};
  CHECK(legacy.scopeRoot == shared.get());
  CHECK(legacy.scopePath.empty());
}

TEST_CASE("local fog placement: shared nested scopes beat outer and root fogs") {
  auto outerFog = createX3DNode("LocalFog"), innerFog = createX3DNode("LocalFog");
  setF(outerFog, "visibilityRange", 10.0f);
  setF(innerFog, "visibilityRange", 4.0f);
  auto outerShape = makeBoxShape(), innerShape = makeBoxShape();
  auto inner = createX3DNode("Group"), outer = createX3DNode("Group");
  addChild(inner, innerFog);
  addChild(inner, innerShape);
  addChild(outer, outerFog);
  addChild(outer, outerShape);
  addChild(outer, inner);
  auto left = createX3DNode("Transform"), right = createX3DNode("Transform");
  setF(left, "scale", SFVec3f{2, 2, 2});
  setF(right, "scale", SFVec3f{3, 3, 3});
  addChild(left, outer);
  addChild(right, outer);
  auto extraRoot = createX3DNode("Group"); // unequal full path lengths.
  addChild(extraRoot, left);
  auto rootFog = createX3DNode("LocalFog");
  setF(rootFog, "visibilityRange", 99.0f);
  auto outside = makeBoxShape();
  bool innerEnabled = true;
  SUBCASE("nearest shared inner fog") {}
  SUBCASE("disabled inner falls back to its own outer placement") {
    innerEnabled = false;
    setF(innerFog, "enabled", false);
  }
  Scene scene;
  scene.rootNodes = {extraRoot, right, outside, rootFog}; // root fog collected last.
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  extract::SceneExtractor ex(ctx, scene);
  const auto snapshot = ex.fullSnapshot();
  REQUIRE(snapshot.added.size() == 5);
  REQUIRE(ex.snapshotLocalFogs().size() == (innerEnabled ? 5 : 3));
  for (auto id : snapshot.added) {
    const auto &item = ex.item(id);
    REQUIRE(item.localFog >= 0);
    const auto &desc = ex.snapshotLocalFogs().at(item.localFog);
    if (item.path.back() == outside.get()) {
      CHECK(desc.visibilityRange == doctest::Approx(99));
      CHECK(desc.scopeRoot == nullptr);
      CHECK(desc.scopePath.empty());
      continue;
    }
    const bool useInner = innerEnabled && item.path.back() == innerShape.get();
    const float scale = item.path.front() == extraRoot.get() ? 2.0f : 3.0f;
    CHECK(desc.visibilityRange == doctest::Approx((useInner ? 4 : 10) * scale));
    CHECK(desc.scopeRoot == (useInner ? inner.get() : outer.get()));
    const auto end = std::find(item.path.begin(), item.path.end(), desc.scopeRoot);
    REQUIRE(end != item.path.end());
    CHECK(desc.scopePath == extract::PathKey(item.path.begin(), end + 1));
  }
}

TEST_CASE("local fog placement: collection remains cycle depth and visit bounded") {
  auto fog = createX3DNode("LocalFog");
  auto shared = createX3DNode("Group");
  addChild(shared, fog);
  Scene scene;
  extract::LocalFogSystem system;
  SUBCASE("cycle stops at the back edge without suppressing a sibling placement") {
    addChild(shared, shared);
    auto left = createX3DNode("Group"), right = createX3DNode("Group");
    addChild(left, shared);
    addChild(right, shared);
    scene.rootNodes = {left, right};
    x3d::WalkBudget budget(20);
    const auto fogs = system.collect(scene, geo::builtinProjection(), budget, SFVec3f{0, 0, 0});
    setF(shared, "children", std::vector<std::shared_ptr<X3DNode>>{fog}); // break ownership cycle.
    CHECK_FALSE(budget.tripped);
    REQUIRE(fogs.size() == 2);
    CHECK(fogs[0].scopePath == extract::PathKey{left.get(), shared.get()});
    CHECK(fogs[1].scopePath == extract::PathKey{right.get(), shared.get()});
  }
  SUBCASE("visit budget truncates then a new collection starts cleanly") {
    scene.rootNodes = {shared, fog};
    x3d::WalkBudget budget(2);
    const auto partial = system.collect(scene, geo::builtinProjection(), budget, SFVec3f{0, 0, 0});
    CHECK(budget.tripped);
    REQUIRE(partial.size() == 1);
    CHECK(partial[0].scopePath == extract::PathKey{shared.get()});
    const auto complete = system.collect(scene, geo::builtinProjection());
    REQUIRE(complete.size() == 2);
    CHECK(complete[1].scopePath.empty());
  }
  SUBCASE("depth cap does not hide a later shallow USE placement") {
    auto chain = shared;
    for (std::size_t i = 1; i < x3d::kMaxNestingDepth; ++i) {
      auto parent = createX3DNode("Group");
      addChild(parent, chain);
      chain = parent;
    }
    scene.rootNodes = {chain, shared};
    const auto fogs = system.collect(scene, geo::builtinProjection());
    REQUIRE(fogs.size() == 1);
    CHECK(fogs[0].scopePath == extract::PathKey{shared.get()});
  }
}
