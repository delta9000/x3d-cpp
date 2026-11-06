// Bounds traversal visits each node and edge once. A depth-50 shared diamond
// has 2^50 root-to-leaf paths without increasing the number of nodes exponentially.
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
using namespace x3d::core;
using namespace x3d::nodes;
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

  // Each Group holds the next node twice, producing 2^50 root-to-leaf paths.
  // Groups share one frame, so bounds stay equal to the box bounds.
  std::shared_ptr<X3DNode> cur = leaf;
  for (int i = 0; i < 50; ++i) {
    auto g = createX3DNode("Group");
    addChild(g, cur);
    addChild(g, cur);   // same shared_ptr -> a second edge to the same subtree
    cur = g;
  }
  Scene scene; scene.addRootNode(cur);

  TransformSystem ts; ts.buildIndex(scene);
  BoundsSystem bs; bs.buildBounds(scene, ts);

  Aabb rb = bs.localBounds(cur.get());
  CHECK((feq(rb.min.x, -1) && feq(rb.max.x, 1)));
  CHECK((feq(rb.min.y, -1) && feq(rb.max.y, 1)));
  Aabb lb = bs.localBounds(leaf.get());
  CHECK((feq(lb.size().x, 2)));
  return;
}
