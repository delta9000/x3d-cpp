// bounds_system_test.cpp
#include "BoundsSystem.hpp"
#include "TransformSystem.hpp"
#include "DirtyTracker.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp" // out-of-line Scene::addRootNode
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

TEST_CASE("bounds_system_test") {
  // Transform(translate +10x) > Shape > Box(size 2) : local bounds of the Transform
  // is the box [-1,1] (children share T's frame; Shape has no transform). World
  // bounds = local * worldTransform(translate +10x) => [9,11]x[-1,1]x[-1,1].
  auto T = createX3DNode("Transform"); setF(T, "translation", std::any(SFVec3f{10,0,0}));
  auto shape = createX3DNode("Shape");
  auto box = createX3DNode("Box"); setF(box, "size", std::any(SFVec3f{2,2,2}));
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(box)));
  addChild(T, shape);
  Scene scene; scene.addRootNode(T);

  TransformSystem ts; ts.buildIndex(scene);
  BoundsSystem bs; bs.buildBounds(scene, ts);

  Aabb lb = bs.localBounds(T.get());
  CHECK((feq(lb.min.x,-1) && feq(lb.max.x,1)));
  Aabb wb = bs.worldBounds(T.get(), ts);
  CHECK((feq(wb.min.x,9) && feq(wb.max.x,11)));

  // Author override: a Group with explicit bboxSize ignores its (bigger) child.
  auto G = createX3DNode("Group");
  setF(G, "bboxCenter", std::any(SFVec3f{0,0,0}));
  setF(G, "bboxSize", std::any(SFVec3f{2,2,2}));
  auto bigShape = createX3DNode("Shape");
  auto bigBox = createX3DNode("Box"); setF(bigBox, "size", std::any(SFVec3f{100,100,100}));
  setF(bigShape, "geometry", std::any(std::shared_ptr<X3DNode>(bigBox)));
  addChild(G, bigShape);
  Scene s2; s2.addRootNode(G);
  TransformSystem ts2; ts2.buildIndex(s2);
  BoundsSystem bs2; bs2.buildBounds(s2, ts2);
  CHECK((feq(bs2.localBounds(G.get()).size().x, 2))); // author bbox, not 100

  // Incremental: grow the box, mark dirty, propagate -> Transform bounds grow.
  setF(box, "size", std::any(SFVec3f{4,4,4})); // box now [-2,2]
  DirtyTracker dirty; dirty.markDirty(box.get(), DirtyBounds);
  bs.propagate(dirty, ts);
  CHECK((feq(bs.localBounds(T.get()).max.x, 2))); // grew from 1 to 2
  CHECK((dirty.flags(T.get()) & DirtyBounds));     // ancestor re-marked
  return;
}
