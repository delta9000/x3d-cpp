// scene_extractor_delta_contract_test.cpp — DELTA-CONTRACT regression.
//
// delta()'s one-delta-per-tick guard used to key on ctx_.now() and enforce itself
// with assert(). That was wrong twice over:
//
//   * `now` is embedder-supplied and may legitimately REPEAT — a paused scene
//     that still ticks, a fixed-timestep driver, deterministic replay. Those
//     consumers tripped `assert(now != lastDeltaNow_)` for doing nothing wrong.
//   * assert() compiles out under NDEBUG, and this repo's `ci` preset is
//     RelWithDebInfo. So the contract a consumer tested against in a debug build
//     was not the contract it shipped with.
//
// The guard now keys on X3DExecutionContext::tickGeneration() — a monotonic
// advance count that cannot repeat — and every previously-asserted misuse has a
// defined answer. These cases pin that the contract is TOTAL: no call sequence
// is undefined, and none of them depend on NDEBUG.

#include "SceneExtractor.hpp"

#include "X3DExecutionContext.hpp"
#include "X3DSceneBridge.hpp" // BridgeResult (buildFrom's return type)
#include "X3DScene.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"

#include <any>
#include "doctest/doctest.h"
#include <cstdint>
#include <memory>
#include <map>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::runtime::extract;
using namespace x3d::core;
using namespace x3d::nodes;

namespace {

void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
void addChild(const std::shared_ptr<X3DNode> &p,
              const std::shared_ptr<X3DNode> &c) {
  for (auto &f : p->fields())
    if (f.x3dName == "children" && f.set) {
      auto k = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*p));
      k.push_back(c);
      f.set(*p, std::any(std::move(k)));
      return;
    }
}
// One Transform over one Box — the Transform is what we animate.
Scene makeScene(std::shared_ptr<X3DNode> &xfOut) {
  auto box = createX3DNode("Box");
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(box)));
  auto xf = createX3DNode("Transform");
  addChild(xf, shape);
  Scene scene;
  scene.rootNodes.push_back(xf);
  xfOut = xf;
  return scene;
}

} // namespace

TEST_CASE("delta contract: tickGeneration is monotonic and clock-independent") {
  X3DExecutionContext ctx;
  CHECK(ctx.tickGeneration() == 0); // no advance has happened yet.

  ctx.tick(1.0);
  CHECK(ctx.tickGeneration() == 1);

  // A PAUSED consumer: same clock value, real advances. now() cannot tell these
  // apart; the generation can.
  ctx.tick(1.0);
  ctx.tick(1.0);
  CHECK(ctx.tickGeneration() == 3);
  CHECK(ctx.now() == 1.0);

  // Even a clock that goes BACKWARDS (replay/scrub) still advances generation.
  ctx.tick(0.5);
  CHECK(ctx.tickGeneration() == 4);
}

TEST_CASE("delta contract: a PAUSED clock still yields deltas") {
  // The regression that motivated this: tick() at an unchanging timestamp is a
  // legitimate pattern (the cascade still runs), and every delta() after the
  // first used to trip the now()-keyed assert.
  std::shared_ptr<X3DNode> xf;
  Scene scene = makeScene(xf);

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  ctx.buildFrom(scene);
  SceneExtractor ex(ctx, scene);
  ex.fullSnapshot();

  for (int i = 1; i <= 3; ++i) {
    ctx.tick(2.0); // clock FROZEN across every iteration.
    REQUIRE(ctx.writeField(xf.get(), "translation",
                           std::any(SFVec3f{static_cast<float>(i), 0, 0})) ==
            FieldWriteResult::Ok);
    RenderDelta d = ex.delta();
    // Each frozen-clock tick still reports its own change.
    CHECK(d.updatedTransform.size() == 1);
  }
}

TEST_CASE("delta contract: second delta() with no tick returns EMPTY, not garbage") {
  std::shared_ptr<X3DNode> xf;
  Scene scene = makeScene(xf);

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  ctx.buildFrom(scene);
  SceneExtractor ex(ctx, scene);
  ex.fullSnapshot();

  ctx.tick(1.0);
  REQUIRE(ctx.writeField(xf.get(), "translation", std::any(SFVec3f{5, 0, 0})) ==
          FieldWriteResult::Ok);

  RenderDelta first = ex.delta();
  CHECK(first.updatedTransform.size() == 1);

  // No tick() in between: nothing CAN have changed, so the honest answer is an
  // empty delta. (Previously: assert in debug, bogus re-diff under NDEBUG.)
  RenderDelta second = ex.delta();
  CHECK(second.updatedTransform.empty());
  CHECK(second.added.empty());
  CHECK(second.removed.empty());
  CHECK(second.updatedGeometry.empty());
  CHECK(second.updatedMaterial.empty());

  // ...and a real advance resumes reporting.
  ctx.tick(2.0);
  REQUIRE(ctx.writeField(xf.get(), "translation", std::any(SFVec3f{6, 0, 0})) ==
          FieldWriteResult::Ok);
  RenderDelta third = ex.delta();
  CHECK(third.updatedTransform.size() == 1);
}

TEST_CASE("delta contract: delta() with no prior fullSnapshot yields the baseline") {
  // No baseline to diff against => a full snapshot IS the baseline. Every item
  // lands in `added`, which is the same upload path frame 0 uses, so a consumer
  // that skipped fullSnapshot() still gets a correct first frame rather than an
  // assert (debug) or a walk over zero-initialised state (NDEBUG).
  std::shared_ptr<X3DNode> xf;
  Scene scene = makeScene(xf);

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  ctx.buildFrom(scene);
  SceneExtractor ex(ctx, scene);

  RenderDelta d = ex.delta(); // no fullSnapshot() first.
  CHECK(d.added.size() == 1);
  CHECK(ex.itemCount() == 1);
  CHECK(d.cameraChanged);
  CHECK(d.backgroundChanged);
  CHECK(d.lightsChanged);

  // The promotion seeds the guard, so the NEXT delta() with no tick is empty
  // rather than a second full snapshot.
  RenderDelta again = ex.delta();
  CHECK(again.added.empty());
}

TEST_CASE("delta contract: geometry replacement removal and reattachment reach consumers") {
  auto shape = createX3DNode("Shape");
  auto box = createX3DNode("Box");
  std::weak_ptr<X3DNode> dead = box;
  setF(shape, "geometry", box);
  Scene scene; scene.rootNodes.push_back(shape);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  SceneExtractor ex(ctx, scene);
  const auto initial = ex.fullSnapshot();
  REQUIRE(initial.added.size() == 1);
  auto retained = ex.item(initial.added[0]).mesh;
  box.reset(); // the scene alone owns the old node

  // Simulate a real consumer: removals first, additions/updates second. Keeping
  // only non-owning geometry identity and an immutable mesh makes node lifetime
  // independent of the mirror (a copied RenderItem is not a scene owner).
  std::map<RenderItemId, RenderItem> mirror;
  auto apply = [&](const RenderDelta &d) {
    for (auto id : d.removed) mirror.erase(id);
    for (auto id : d.added) mirror.insert_or_assign(id, ex.item(id));
    for (const auto *updates : {&d.updatedTransform, &d.updatedGeometry, &d.updatedMaterial})
      for (auto id : *updates) {
        REQUIRE(mirror.count(id) == 1);
        mirror.insert_or_assign(id, ex.item(id));
      }
    SceneExtractor oracle(ctx, scene);
    auto snap = oracle.fullSnapshot();
    REQUIRE(mirror.size() == snap.added.size());
    for (auto id : snap.added) {
      const auto &expected = oracle.item(id);
      auto found = std::find_if(mirror.begin(), mirror.end(), [&](const auto &p) {
        return p.second.path == expected.path;
      });
      REQUIRE(found != mirror.end());
      CHECK(found->second.geometry.node == expected.geometry.node);
      CHECK(found->second.mesh->positions == expected.mesh->positions);
      CHECK(found->second.mesh->indices == expected.mesh->indices);
    }
  };
  apply(initial);
  auto sphere = createX3DNode("Sphere");
  for (const auto &geometry : {sphere, std::shared_ptr<X3DNode>{}, sphere}) {
    ctx.postEvent(shape.get(), "set_geometry", geometry);
    ctx.tick(1); // same timestamp, three separate changes
    const auto d = ex.delta();
    apply(d);
    CHECK(dead.expired());
    CHECK(mirror.size() == (geometry ? 1 : 0));
    const auto repeated = ex.delta();
    CHECK(repeated.added.empty());
    CHECK(repeated.removed.empty());
  }
  CHECK_FALSE(retained->positions.empty()); // immutable payload still owned
}

TEST_CASE("delta contract: shared group topology preserves every placement") {
  auto group = createX3DNode("Group");
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", createX3DNode("Box"));
  addChild(group, shape);
  auto left = createX3DNode("Transform"), right = createX3DNode("Transform");
  setF(left, "translation", SFVec3f{-10,0,0});
  setF(right, "translation", SFVec3f{10,0,0});
  addChild(left, group); addChild(right, group);
  Scene scene; scene.rootNodes = {left, right};
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  SceneExtractor ex(ctx, scene);
  REQUIRE(ex.fullSnapshot().added.size() == 2);
  auto extra = createX3DNode("Shape");
  setF(extra, "geometry", createX3DNode("Sphere"));
  ctx.postEvent(group.get(), "children", std::vector<std::shared_ptr<X3DNode>>{shape, extra});
  ctx.tick(0);
  const auto d = ex.delta();
  REQUIRE(d.removed.size() == 2);
  REQUIRE(d.added.size() == 4);
  std::vector<float> positions;
  for (auto id : d.added) positions.push_back(ex.item(id).worldTransform.m[12]);
  std::sort(positions.begin(), positions.end());
  CHECK((positions == std::vector<float>{-10,-10,10,10}));
  CHECK(ex.delta().added.empty()); // at most one rebuild for this tick
}

TEST_CASE("delta contract: structural replacement precedes stale dirty path traversal") {
  auto root = createX3DNode("Group"), child = createX3DNode("Transform");
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", createX3DNode("Box"));
  addChild(child, shape); addChild(root, child);
  Scene scene; scene.rootNodes = {root};
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  SceneExtractor ex(ctx, scene);
  REQUIRE(ex.fullSnapshot().added.size() == 1);
  std::weak_ptr<X3DNode> dead = child;
  ctx.postEvent(child.get(), "translation", SFVec3f{2,0,0});
  ctx.postEvent(root.get(), "children", std::vector<std::shared_ptr<X3DNode>>{});
  child.reset(); shape.reset();
  ctx.tick(0);
  CHECK(dead.expired());
  auto d = ex.delta(); // must not reaccumulate through the destroyed Transform
  CHECK(d.removed == std::vector<RenderItemId>{0});
  CHECK(d.added.empty());
  ctx.tick(0);
  d = ex.delta();
  CHECK(d.removed.empty());
  CHECK(d.updatedTransform.empty());
}
