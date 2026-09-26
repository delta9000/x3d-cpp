// pick_index_perf_test.cpp
// Performance-invariant lock for the lazily-indexed PickSystem. On an UNCHANGED
// scene the reflective per-pick node walk must amortize to zero: the index is
// rebuilt once (O(nodes)), and every later pick touches only the flat candidate
// list (O(candidate leaves)). The counters are exposed as lastIndexVisits() /
// lastCandidateTests().
#include "PickSystem.hpp"
#include "TransformSystem.hpp"
#include "BoundsSystem.hpp"
#include "DirtyTracker.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp"

#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;

static bool feq(float a, float b, float eps = 1e-3f) { return std::fabs(a - b) < eps; }

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
static std::shared_ptr<X3DNode> shapeWith(const std::shared_ptr<X3DNode> &geom) {
  auto s = createX3DNode("Shape");
  setF(s, "geometry", std::any(std::shared_ptr<X3DNode>(geom)));
  return s;
}

// A root Transform with N deep branches (4 nested Groups each) ending in one
// Box Shape: ~6 graph nodes per geometry leaf, so a reflective walk visits far
// more nodes than there are candidate leaves.
static std::shared_ptr<X3DNode> makeDeepScene(int n) {
  auto root = createX3DNode("Transform");
  for (int i = 0; i < n; ++i) {
    std::shared_ptr<X3DNode> leaf = createX3DNode("Group");
    for (int d = 0; d < 4; ++d) {
      auto inner = createX3DNode("Group");
      addChild(inner, leaf);
      leaf = inner;
    }
    auto box = createX3DNode("Box"); setF(box, "size", std::any(SFVec3f{1, 1, 1}));
    addChild(leaf, shapeWith(box));
    addChild(root, leaf);
  }
  return root;
}

TEST_CASE("pick_index_walk_amortizes_to_candidate_leaves") {
  const int N = 200;
  Scene sc; sc.addRootNode(makeDeepScene(N));
  TransformSystem ts; ts.buildIndex(sc);
  BoundsSystem bs; bs.buildBounds(sc, ts);
  PickSystem ps; ps.build(sc);

  const Ray ray{{0, 0, 10}, {0, 0, -1}};

  // First pick builds the index: a full O(nodes) reflective walk.
  ps.pickClosest(ray, bs, {0, 0, 0}, {0, 1, 0}, x3d::kMaxGraphWalkVisits, &ts);
  const std::uint64_t firstVisits = ps.lastIndexVisits();
  CHECK(firstVisits >= static_cast<std::uint64_t>(N)); // visited at least every leaf
  CHECK(firstVisits > static_cast<std::uint64_t>(3 * N)); // far more than leaves

  // Second pick, scene unchanged: NO graph walk — only the flat candidate list.
  ps.pickClosest(ray, bs, {0, 0, 0}, {0, 1, 0}, x3d::kMaxGraphWalkVisits, &ts);
  CHECK(ps.lastIndexVisits() == 0);                              // O(1), not O(nodes)
  CHECK(ps.lastCandidateTests() == static_cast<std::uint64_t>(N)); // O(candidate leaves)
}

TEST_CASE("pick_index_refits_on_transform_revision") {
  auto root = createX3DNode("Group");
  auto T = createX3DNode("Transform"); setF(T, "translation", std::any(SFVec3f{5, 0, 0}));
  auto box = createX3DNode("Box"); setF(box, "size", std::any(SFVec3f{2, 2, 2}));
  auto shape = shapeWith(box);
  addChild(T, shape);
  addChild(root, T);

  Scene sc; sc.addRootNode(root);
  TransformSystem ts; ts.buildIndex(sc);
  BoundsSystem bs; bs.buildBounds(sc, ts);
  PickSystem ps; ps.build(sc);

  auto a = ps.pickClosest(Ray{{5, 0, 10}, {0, 0, -1}}, bs, {0, 0, 0}, {0, 1, 0},
                          x3d::kMaxGraphWalkVisits, &ts);
  CHECK((a.hit && feq(a.point.x, 5)));

  // A transform change routed through the dirty pipeline bumps the revision, so
  // the cached index must rebuild/refit and pick up the new world frame.
  setF(T, "translation", std::any(SFVec3f{10, 0, 0}));
  DirtyTracker dirty;
  dirty.markDirty(T.get(), DirtyLocalTransform);
  ts.propagate(dirty);

  auto b = ps.pickClosest(Ray{{10, 0, 10}, {0, 0, -1}}, bs, {0, 0, 0}, {0, 1, 0},
                          x3d::kMaxGraphWalkVisits, &ts);
  CHECK((b.hit && feq(b.point.x, 10)));
  CHECK(ps.lastIndexVisits() > 0); // the revision change forced a rebuild
}
