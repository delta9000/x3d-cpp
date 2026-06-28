// scene_extractor_delta_perf_test.cpp — PERF-DELTA regression.
//
// delta()'s per-tick world re-accumulation must MEMOIZE shared ancestor
// transforms. A wide scene where M render items share one deep Transform prefix
// must recompose each DISTINCT ancestor's local matrix once per delta(), not
// once per dependent item — the O(items * depth) trap that made the kelp PoC
// run at ~25fps (delta() alone was ~19ms/frame, all in repeated localMatrix()
// recomposition over shared ancestors).
//
// Guarded by TransformSystem::localMatrixCallCount(). A second case pins per-
// PATH correctness under DEF/USE instancing so the memoization is never
// "simplified" into ctx.worldTransform()/world_ (the first-path-only table that
// holds a single matrix per node and would collapse all instances onto the
// first path — the M2C-1 hazard called out in SceneExtractor.hpp).

#include "SceneExtractor.hpp"

#include "TransformSystem.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DScene.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"

#include <algorithm>
#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

using namespace x3d::runtime;
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
std::shared_ptr<X3DNode> makeTriShape() {
  auto coord = createX3DNode("Coordinate");
  setF(coord, "point",
       std::any(std::vector<SFVec3f>{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}));
  auto tri = createX3DNode("TriangleSet");
  setF(tri, "coord", std::any(std::shared_ptr<X3DNode>(coord)));
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(tri)));
  return shape;
}

} // namespace

TEST_CASE("delta memoizes shared ancestor transforms (no per-item recompute)") {
  const int D = 6;   // depth of the shared Transform chain (T0..T5)
  const int M = 16;  // render items all sharing the full T0..T5 prefix

  std::vector<std::shared_ptr<X3DNode>> chain;
  for (int i = 0; i < D; ++i) chain.push_back(createX3DNode("Transform"));
  for (int i = 0; i + 1 < D; ++i) addChild(chain[i], chain[i + 1]);
  for (int i = 0; i < M; ++i) addChild(chain[D - 1], makeTriShape());

  Scene scene;
  scene.addRootNode(chain[0]);
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  extract::SceneExtractor ex(ctx, scene);

  auto snap = ex.fullSnapshot();
  REQUIRE(snap.added.size() == static_cast<size_t>(M));

  // Dirty the ROOT transform -> every one of the M items is updatedTransform.
  ctx.postEvent(chain[0].get(), "translation", std::any(SFVec3f{10, 0, 0}));
  ctx.tick(1.0);

  const std::uint64_t before = TransformSystem::localMatrixCallCount();
  auto d = ex.delta();
  const std::uint64_t calls = TransformSystem::localMatrixCallCount() - before;

  CHECK(d.updatedTransform.size() == static_cast<size_t>(M));

  // The contract: each of the D distinct ancestor transforms is recomposed at
  // most a handful of times across the whole delta() — NOT once per dependent
  // item. O(items * depth) would be >= M * D = 96 here (in fact higher, since
  // propagate() flags every node in the subtree DirtyWorldTransform, so each
  // item is re-accumulated once per dirty ancestor). Memoized: ~D.
  CHECK(calls <= static_cast<std::uint64_t>(2 * D));

  // Correctness: every item's world origin reflects the +10 X root translation
  // (the inner transforms are identity), proving the memoized path still yields
  // the exact per-path world matrix.
  for (auto id : d.updatedTransform) {
    SFVec3f o = ex.item(id).worldTransform.transformPoint(SFVec3f{0, 0, 0});
    CHECK(std::fabs(o.x - 10.0f) < 1e-4f);
    CHECK(std::fabs(o.y) < 1e-4f);
    CHECK(std::fabs(o.z) < 1e-4f);
  }
}

TEST_CASE("delta keeps per-path world transforms under DEF/USE instancing") {
  // One shared Shape instanced under TWO sibling transforms at different X. The
  // per-path re-accumulation must give each instance its OWN world position; a
  // node-keyed first-path table (world_) could only hold one and would collapse
  // both onto the same point.
  auto shared = makeTriShape();
  auto left = createX3DNode("Transform");
  setF(left, "translation", std::any(SFVec3f{-5, 0, 0}));
  auto right = createX3DNode("Transform");
  setF(right, "translation", std::any(SFVec3f{5, 0, 0}));
  addChild(left, shared);
  addChild(right, shared); // USE: same node, second path
  auto root = createX3DNode("Transform");
  addChild(root, left);
  addChild(root, right);

  Scene scene;
  scene.addRootNode(root);
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  extract::SceneExtractor ex(ctx, scene);

  auto snap = ex.fullSnapshot();
  REQUIRE(snap.added.size() == 2);

  // Dirty the common root so BOTH instances re-accumulate through delta().
  ctx.postEvent(root.get(), "translation", std::any(SFVec3f{0, 1, 0}));
  ctx.tick(1.0);
  auto d = ex.delta();
  REQUIRE(d.updatedTransform.size() == 2);

  std::vector<float> xs;
  for (auto id : d.updatedTransform) {
    SFVec3f o = ex.item(id).worldTransform.transformPoint(SFVec3f{0, 0, 0});
    CHECK(std::fabs(o.y - 1.0f) < 1e-4f); // shared root lift applies to both
    xs.push_back(o.x);
  }
  // The two instances stay at DISTINCT, correct X (-5 and +5), not collapsed.
  std::sort(xs.begin(), xs.end());
  CHECK(std::fabs(xs[0] - (-5.0f)) < 1e-4f);
  CHECK(std::fabs(xs[1] - (5.0f)) < 1e-4f);
}
