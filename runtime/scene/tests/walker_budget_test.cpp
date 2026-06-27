// walker_budget_test.cpp
// Regression for #21: an acyclic "doubling DAG" (G0->[G1,G1], G1->[G2,G2], ...)
// has no containment back-edge and stays shallow, so the MEM-1 cycle+depth guards
// leave it alone — but its leaf is reachable via 2^depth distinct paths, so the
// per-path graph walks fan out exponentially (a CPU/memory DoS). worldOf is a
// first-found *search*, fixed by visited-set memoization (this file's first
// cases); the per-path *emitters* (extractor/pick/light) are bounded by a
// WalkBudget with a graceful budgetExceeded signal (the later cases).
#include "PickSystem.hpp"
#include "SceneExtractor.hpp"
#include "TransformSystem.hpp"
#include "BoundsSystem.hpp"
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp"

#include <any>
#include "doctest/doctest.h"
#include <memory>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
static void addChild(const std::shared_ptr<X3DNode> &p, const std::shared_ptr<X3DNode> &c) {
  for (auto &f : p->fields())
    if (f.x3dName == "children" && f.set) {
      auto k = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*p));
      k.push_back(c);
      f.set(*p, std::any(std::move(k)));
      return;
    }
}

// A doubling DAG `depth` levels tall over `leaf`: each Group lists `node` TWICE,
// so the leaf is reachable by 2^depth distinct paths from the returned root,
// using only depth+1 distinct nodes (no cycle, all forward edges).
static std::shared_ptr<X3DNode> makeDoublingDag(int depth,
                                                const std::shared_ptr<X3DNode> &leaf) {
  std::shared_ptr<X3DNode> node = leaf;
  for (int i = 0; i < depth; ++i) {
    auto g = createX3DNode("Group");
    addChild(g, node);
    addChild(g, node); // same child twice -> path count doubles per level
    node = g;
  }
  return node;
}

TEST_CASE("worldof_absent_target_terminates_on_wide_doubling_dag") {
  // 2^45 paths: pre-memoization worldOfRec re-explores every path searching for a
  // target that is not present -> does not terminate in any reasonable time.
  // With visited-set memoization it is O(nodes) and returns identity at once.
  auto leaf = createX3DNode("Group");
  Scene scene;
  scene.addRootNode(makeDoublingDag(45, leaf));
  PickSystem ps; ps.build(scene);

  auto stranger = createX3DNode("Group");
  Mat4 w = ps.worldOf(stranger.get());
  CHECK(w.transformPoint({1, 2, 3}).x == doctest::Approx(1.0f)); // identity
}

TEST_CASE("worldof_finds_present_target_in_wide_dag") {
  // Memoization must not change the result: the bound target's world transform is
  // still its first-found placement (the DAG's transform-bearing leaf).
  auto leaf = createX3DNode("Transform");
  setF(leaf, "translation", std::any(SFVec3f{7, 0, 0}));
  Scene scene;
  scene.addRootNode(makeDoublingDag(45, leaf));
  PickSystem ps; ps.build(scene);

  Mat4 w = ps.worldOf(leaf.get());
  CHECK(w.transformPoint({0, 0, 0}).x == doctest::Approx(7.0f));
}

TEST_CASE("worldof_depth_truncated_subtree_is_not_falsely_memoized") {
  // The memoization must not collude with the MEM-1 depth cap. A node `P` is
  // reachable BOTH near the cap (where its subtree truncates unexplored) AND
  // shallowly (where the target IS findable). Marking `P` visited on the deep,
  // truncated path would wrongly prune the shallow path and lose the target.
  auto target = createX3DNode("Transform");
  setF(target, "translation", std::any(SFVec3f{5, 0, 0}));
  auto p = createX3DNode("Group");
  addChild(p, target); // target is one level under P

  // A 998-Group chain places P at depth 999 (under root), so P is explored but its
  // child `target` sits at depth 1000 == kMaxNestingDepth and is truncated.
  std::shared_ptr<X3DNode> chain = p;
  for (int i = 0; i < 998; ++i) {
    auto g = createX3DNode("Group");
    addChild(g, chain);
    chain = g;
  }
  auto root = createX3DNode("Group");
  addChild(root, chain); // deep path FIRST — truncates P's subtree
  addChild(root, p);     // shallow path: target findable at depth 2
  Scene scene;
  scene.addRootNode(root);
  PickSystem ps; ps.build(scene);

  Mat4 w = ps.worldOf(target.get());
  CHECK(w.transformPoint({0, 0, 0}).x == doctest::Approx(5.0f)); // found via shallow path
}

// A Shape with a unit Box, for the per-path emitters below.
static std::shared_ptr<X3DNode> makeBoxShape() {
  auto box = createX3DNode("Box");
  setF(box, "size", std::any(SFVec3f{1, 1, 1}));
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(box)));
  return shape;
}

TEST_CASE("scene_extractor_budget_bounds_doubling_dag") {
  // 2^22 paths to one Shape: the extractor's exponential cost here is the WALK
  // (2^22 node-visits), not emission — every path reuses the same node pointers,
  // so all paths share one PathKey and emit() interns them to a single
  // RenderItem. The small budget stops the walk early and latches budgetExceeded.
  Scene scene;
  scene.addRootNode(makeDoublingDag(22, makeBoxShape()));
  X3DExecutionContext ctx;
  extract::MeshBuildOptions opts;
  opts.maxWalkVisits = 10'000;
  extract::SceneExtractor ex(ctx, scene, opts);
  extract::RenderDelta snap = ex.fullSnapshot();

  CHECK(ex.budgetExceeded());        // the walk was bounded, not run to 2^22
  // Light collection runs first in fullSnapshot and shares the budget, so on this
  // (light-less but exponentially wide) scene it can consume the budget before the
  // geometry walk starts — both outcomes (0, or 1 interned item) are bounded.
  CHECK(ex.itemCount() <= 1);
}

TEST_CASE("scene_extractor_budget_untouched_by_a_small_scene") {
  // A legitimate small scene never trips the budget (regression guard).
  Scene scene;
  scene.addRootNode(makeBoxShape());
  X3DExecutionContext ctx;
  extract::SceneExtractor ex(ctx, scene); // default cap
  extract::RenderDelta snap = ex.fullSnapshot();

  CHECK_FALSE(ex.budgetExceeded());
  CHECK(snap.added.size() == 1);
}

TEST_CASE("light_collection_budget_bounds_doubling_dag") {
  // A PointLight reachable by 2^22 paths would collect ~4M LightDescs unbounded.
  // The light walk shares the budget ceiling; the extractor surfaces the trip.
  auto light = createX3DNode("PointLight");
  Scene scene;
  scene.addRootNode(makeDoublingDag(22, light));
  X3DExecutionContext ctx;
  extract::MeshBuildOptions opts;
  opts.maxWalkVisits = 10'000;
  extract::SceneExtractor ex(ctx, scene, opts);
  ex.fullSnapshot();

  CHECK(ex.budgetExceeded()); // tripped during light collection
}

TEST_CASE("pick_budget_bounds_doubling_dag") {
  // pickNode tests geometry per path (each placement has its own world frame), so
  // it cannot memoize — the budget bounds the fan-out and flags the trip.
  Scene scene;
  scene.addRootNode(makeDoublingDag(22, makeBoxShape()));
  TransformSystem ts; ts.buildIndex(scene);
  BoundsSystem bs; bs.buildBounds(scene, ts);
  PickSystem ps; ps.build(scene);

  PickResult r =
      ps.pickClosest(Ray{{0, 0, 10}, {0, 0, -1}}, bs, {0, 0, 0}, {0, 1, 0}, 10'000);
  CHECK(r.budgetExceeded);
}
