// bounds_shared_subgraph_test.cpp
// Regression for the buildSceneGraph HANG (browser-level B1): BoundsSystem's
// index()/compute() recursed every graph EDGE, so a USE-shared node reachable by
// K root-to-node paths was visited K times — multiplicative explosion with nesting
// depth (real corpus: DirtBike 61 USE / Church 884 shapes hung indefinitely at
// 100% CPU). Here a depth-50 "diamond" makes the leaf reachable by 2^50 paths:
// pre-fix this test never returns (a true hang); post-fix it is O(nodes+edges) and
// completes instantly with correct bounds.
#include "BoundsSystem.hpp"
#include "TransformSystem.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp"
#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
#include <vector>
using namespace x3d::runtime;
static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }
static void setF(const std::shared_ptr<X3DNode>& n, const char* name, std::any v) {
  for (auto& f : n->fields()) if (f.x3dName == name && f.set) { f.set(*n, std::move(v)); return; }
}
static void addChild(const std::shared_ptr<X3DNode>& p, const std::shared_ptr<X3DNode>& c) {
  for (auto& f : p->fields()) if (f.x3dName == "children" && f.set) {
    auto k = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*p));
    k.push_back(c); f.set(*p, std::any(std::move(k))); return;
  }
}

TEST_CASE("bounds_shared_subgraph_test") {
  // leaf Group > Shape > Box(size 2); local bounds [-1,1].
  auto leaf = createX3DNode("Group");
  auto shape = createX3DNode("Shape");
  auto box = createX3DNode("Box"); setF(box, "size", std::any(SFVec3f{2,2,2}));
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(box)));
  addChild(leaf, shape);

  // 50 stacked Groups, each holding the SAME next-level node TWICE (a shared edge):
  // paths(root->leaf) = 2^50. Only the per-path-explosion bug makes this diverge;
  // all nodes share one frame (Groups, no Transforms) so bounds stay the box bounds.
  std::shared_ptr<X3DNode> cur = leaf;
  for (int i = 0; i < 50; ++i) {
    auto g = createX3DNode("Group");
    addChild(g, cur);
    addChild(g, cur);   // same shared_ptr -> a second edge to the same subtree
    cur = g;
  }
  Scene scene; scene.addRootNode(cur);

  TransformSystem ts; ts.buildIndex(scene);
  BoundsSystem bs; bs.buildBounds(scene, ts);   // pre-fix: never returns

  // Reaching here at all proves the hang is fixed; bounds must still be correct.
  Aabb rb = bs.localBounds(cur.get());
  CHECK((feq(rb.min.x, -1) && feq(rb.max.x, 1)));
  CHECK((feq(rb.min.y, -1) && feq(rb.max.y, 1)));
  Aabb lb = bs.localBounds(leaf.get());
  CHECK((feq(lb.size().x, 2)));
  return;
}
