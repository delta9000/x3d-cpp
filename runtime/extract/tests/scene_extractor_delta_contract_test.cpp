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
#include <set>
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

namespace {
struct GeometryMirror {
  std::map<RenderItemId, RenderItem> items;
  void apply(const RenderDelta &delta, const SceneExtractor &ex,
             const X3DExecutionContext &ctx, const Scene &scene) {
    for (auto id : delta.removed) REQUIRE(items.erase(id) == 1);
    for (auto id : delta.added) REQUIRE(items.emplace(id, ex.item(id)).second);
    for (const auto *updates : {&delta.updatedGeometry, &delta.updatedTransform,
                               &delta.updatedMaterial, &delta.updatedSkinPose}) {
      std::set<RenderItemId> seen;
      for (auto id : *updates) {
        REQUIRE(seen.insert(id).second);
        REQUIRE(items.count(id) == 1);
        items.at(id) = ex.item(id);
      }
    }
    SceneExtractor oracle(ctx, scene);
    auto snapshot = oracle.fullSnapshot();
    REQUIRE(items.size() == snapshot.added.size());
    for (auto id : snapshot.added) {
      const auto &expected = oracle.item(id);
      auto found = std::find_if(items.begin(), items.end(), [&](const auto &entry) {
        return entry.second.path == expected.path;
      });
      REQUIRE(found != items.end());
      const auto &actual = found->second;
      CHECK(actual.geometry.node == expected.geometry.node);
      CHECK(actual.mesh->topology == expected.mesh->topology);
      CHECK(actual.mesh->positions == expected.mesh->positions);
      CHECK(actual.mesh->indices == expected.mesh->indices);
      CHECK(actual.mesh->normals == expected.mesh->normals);
      CHECK(actual.mesh->texcoords == expected.mesh->texcoords);
      CHECK(actual.mesh->colors == expected.mesh->colors);
      for (int j = 0; j < 16; ++j)
        CHECK(actual.worldTransform.m[j] == doctest::Approx(expected.worldTransform.m[j]));
      CHECK(actual.material.phong.diffuse == expected.material.phong.diffuse);
    }
  }
};

struct SharedGeometryFixture {
  std::shared_ptr<X3DNode> coord = createX3DNode("Coordinate");
  std::shared_ptr<X3DNode> color = createX3DNode("Color");
  std::shared_ptr<X3DNode> triangles = createX3DNode("TriangleSet");
  std::shared_ptr<X3DNode> lines = createX3DNode("IndexedLineSet");
  std::shared_ptr<X3DNode> left = createX3DNode("Transform");
  std::shared_ptr<X3DNode> right = createX3DNode("Transform");
  std::shared_ptr<X3DNode> material = createX3DNode("Material");
  Scene scene;
  MFVec3f points{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {2, 0, 0}};
  SharedGeometryFixture(bool initiallyEmpty = false) {
    if (!initiallyEmpty) setF(coord, "point", points);
    setF(color, "color", MFColor{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 1, 1}});
    setF(triangles, "coord", coord);
    setF(triangles, "color", color);
    setF(lines, "coord", coord);
    setF(lines, "color", color);
    setF(lines, "coordIndex", MFInt32{0, 3, -1});
    auto triShape = createX3DNode("Shape");
    auto lineShape = createX3DNode("Shape");
    auto appearance = createX3DNode("Appearance");
    setF(appearance, "material", material);
    setF(triShape, "appearance", appearance);
    setF(triShape, "geometry", triangles);
    setF(lineShape, "geometry", lines);
    setF(left, "translation", SFVec3f{-3, 0, 0});
    setF(right, "translation", SFVec3f{3, 0, 0});
    addChild(left, triShape);
    addChild(right, triShape); // one Shape, two placement paths
    auto unchanged = createX3DNode("Shape");
    setF(unchanged, "geometry", createX3DNode("Box"));
    scene.rootNodes = {left, right, lineShape, unchanged};
  }
};
} // namespace

TEST_CASE("geometry delta: shared sources preserve every owner's topology and coalesce builds") {
  SharedGeometryFixture f;
  X3DExecutionContext ctx; ctx.buildSceneGraph(f.scene);
  SceneExtractor ex(ctx, f.scene);
  GeometryMirror mirror;
  auto snapshot = ex.fullSnapshot();
  mirror.apply(snapshot, ex, ctx, f.scene);
  REQUIRE(snapshot.added.size() == 4);
  const auto tri0 = snapshot.added[0], tri1 = snapshot.added[1];
  auto oldMesh = ex.item(tri0).mesh;
  auto unchangedMesh = ex.item(snapshot.added[3]).mesh;
  CHECK(oldMesh == ex.item(tri1).mesh);
  f.points[1].x = 5;
  ctx.postEvent(f.coord.get(), "point", f.points);
  ctx.postEvent(f.color.get(), "color", MFColor{{0, 1, 1}, {1, 1, 0}, {1, 0, 1}, {0, 0, 0}});
  ctx.tick(1);
  const auto builds = buildLocalMeshCallCount();
  const auto delta = ex.delta();
  CHECK(buildLocalMeshCallCount() - builds == 2);
  CHECK(delta.added.empty());
  CHECK(delta.removed.empty());
  REQUIRE(delta.updatedGeometry.size() == 3);
  CHECK(ex.item(tri0).geometry.contentVersion == 1);
  CHECK(ex.item(tri1).geometry == ex.item(tri0).geometry);
  CHECK(ex.item(tri0).mesh == ex.item(tri1).mesh);
  CHECK(ex.item(tri0).mesh != oldMesh);
  CHECK(ex.item(snapshot.added[3]).mesh == unchangedMesh);
  CHECK(oldMesh->positions[1].x == 1);
  mirror.apply(delta, ex, ctx, f.scene);
  CHECK(ex.delta().updatedGeometry.empty());
}

TEST_CASE("geometry delta: empty transitions remove and restore all shared placements") {
  for (bool initiallyEmpty : {false, true}) {
    CAPTURE(initiallyEmpty);
    SharedGeometryFixture f(initiallyEmpty);
    X3DExecutionContext ctx; ctx.buildSceneGraph(f.scene);
    SceneExtractor ex(ctx, f.scene);
    GeometryMirror mirror;
    const auto snapshot = ex.fullSnapshot();
    mirror.apply(snapshot, ex, ctx, f.scene);
    REQUIRE(snapshot.added.size() == (initiallyEmpty ? 1 : 4));
    auto unchangedMesh = ex.item(snapshot.added.back()).mesh;
    for (bool empty : {true, true, false, false, true, false}) {
      CAPTURE(empty);
      const auto previousSize = mirror.items.size();
      ctx.postEvent(f.coord.get(), "point", empty ? MFVec3f{} : f.points);
      ctx.postEvent(f.left.get(), "translation", SFVec3f{empty ? -7.f : -9.f, 0, 0});
      ctx.postEvent(f.material.get(), "diffuseColor", SFColor{0.2f, 0.4f, 0.8f});
      ctx.tick(1); // paused timestamps still advance independent deltas
      const auto builds = buildLocalMeshCallCount();
      const auto delta = ex.delta();
      CHECK(buildLocalMeshCallCount() - builds == 2);
      CHECK(delta.removed.size() == (empty && previousSize == 4 ? 3 : 0));
      CHECK(delta.added.size() == (!empty && previousSize == 1 ? 3 : 0));
      CHECK(delta.updatedGeometry.size() == (!empty && previousSize == 4 ? 3 : 0));
      mirror.apply(delta, ex, ctx, f.scene);
      CHECK(mirror.items.size() == (empty ? 1 : 4));
      CHECK(ex.item(snapshot.added.back()).mesh == unchangedMesh);
      if (!empty) {
        std::vector<const RenderItem *> triangles;
        for (const auto &[id, item] : mirror.items) {
          (void)id;
          if (item.geometry.node == f.triangles.get()) triangles.push_back(&item);
        }
        REQUIRE(triangles.size() == 2);
        CHECK(triangles[0]->mesh == triangles[1]->mesh);
        CHECK(triangles[0]->geometry == triangles[1]->geometry);
      }
      CHECK(ex.delta().added.empty());
      CHECK(ex.delta().removed.empty());
    }
  }
}

TEST_CASE("geometry delta: geometry-owned index emptiness is incremental") {
  SharedGeometryFixture f;
  X3DExecutionContext ctx; ctx.buildSceneGraph(f.scene);
  SceneExtractor ex(ctx, f.scene);
  GeometryMirror mirror;
  const auto snapshot = ex.fullSnapshot();
  mirror.apply(snapshot, ex, ctx, f.scene);
  const auto lineId = snapshot.added[2];
  const auto triangleMesh = ex.item(snapshot.added[0]).mesh;
  for (bool empty : {true, false, true, false}) {
    ctx.postEvent(f.lines.get(), "coordIndex", empty ? MFInt32{} : MFInt32{0, 3, -1});
    ctx.tick(1);
    const auto builds = buildLocalMeshCallCount();
    const auto delta = ex.delta();
    CHECK(buildLocalMeshCallCount() - builds == 1);
    CHECK(delta.updatedGeometry.empty());
    if (empty) CHECK(delta.removed == std::vector<RenderItemId>{lineId});
    else CHECK(delta.added == std::vector<RenderItemId>{lineId});
    CHECK(ex.item(snapshot.added[0]).mesh == triangleMesh);
    mirror.apply(delta, ex, ctx, f.scene);
  }
}
