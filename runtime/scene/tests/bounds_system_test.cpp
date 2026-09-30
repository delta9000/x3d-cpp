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

TEST_CASE("bounds_geometry_replacement_and_removal") {
  auto root = createX3DNode("Transform");
  auto shape = createX3DNode("Shape");
  auto box = createX3DNode("Box");
  std::weak_ptr<X3DNode> old = box;
  setF(shape, "geometry", box);
  addChild(root, shape);
  Scene scene; scene.addRootNode(root);
  TransformSystem ts; ts.buildIndex(scene);
  BoundsSystem bs; bs.buildBounds(scene, ts);
  box.reset(); // the scene is the only owner; indexes must not prolong lifetime

  auto replacement = createX3DNode("Box");
  setF(replacement, "size", SFVec3f{6, 4, 2});
  setF(shape, "geometry", replacement);
  CHECK(old.expired());
  old.reset();
  DirtyTracker dirty;
  dirty.markDirty(shape.get(), DirtyChildren | DirtyBounds);
  ts.propagate(dirty);
  bs.propagate(dirty, ts);
  CHECK(feq(bs.localBounds(root.get()).size().x, 6));

  // Newly attached leaves must participate in later incremental updates.
  dirty.clear();
  setF(replacement, "size", SFVec3f{8, 4, 2});
  dirty.markDirty(replacement.get(), DirtyBounds);
  bs.propagate(dirty, ts);
  CHECK(feq(bs.localBounds(root.get()).size().x, 8));

  std::weak_ptr<X3DNode> removed = replacement;
  replacement.reset();
  setF(shape, "geometry", std::shared_ptr<X3DNode>{});
  CHECK(removed.expired());
  removed.reset();
  dirty.clear();
  dirty.markDirty(shape.get(), DirtyChildren | DirtyBounds);
  ts.propagate(dirty);
  bs.propagate(dirty, ts);
  CHECK(bs.localBounds(shape.get()).empty);
  CHECK(bs.localBounds(root.get()).empty);
  const auto revision = bs.revision();
  dirty.clear();
  bs.propagate(dirty, ts);
  CHECK(bs.revision() == revision);
}

TEST_CASE("bounds_shared_geometry_updates_every_parent") {
  auto root = createX3DNode("Group");
  auto left = createX3DNode("Transform");
  auto right = createX3DNode("Transform");
  setF(left, "translation", SFVec3f{-10, 0, 0});
  setF(right, "translation", SFVec3f{10, 0, 0});
  auto a = createX3DNode("Shape");
  auto b = createX3DNode("Shape");
  auto box = createX3DNode("Box");
  setF(a, "geometry", box); setF(b, "geometry", box);
  addChild(left, a); addChild(right, b);
  addChild(root, left); addChild(root, right);
  Scene scene; scene.addRootNode(root);
  TransformSystem ts; ts.buildIndex(scene);
  BoundsSystem bs; bs.buildBounds(scene, ts);
  DirtyTracker dirty;

  setF(box, "size", SFVec3f{4, 4, 4});
  dirty.markDirty(box.get(), DirtyBounds);
  bs.propagate(dirty, ts);
  CHECK(feq(bs.localBounds(a.get()).size().x, 4));
  CHECK(feq(bs.localBounds(b.get()).size().x, 4));
  CHECK(feq(bs.localBounds(root.get()).min.x, -12));
  CHECK(feq(bs.localBounds(root.get()).max.x, 12));
  CHECK((dirty.flags(left.get()) & DirtyBounds));
  CHECK((dirty.flags(right.get()) & DirtyBounds));

  // A Transform's local bound does not move with its own TRS, but its parent's
  // union does. This must update even when the geometry itself is unchanged.
  dirty.clear();
  setF(left, "translation", SFVec3f{-20, 0, 0});
  dirty.markDirty(left.get(), DirtyLocalTransform);
  ts.propagate(dirty); bs.propagate(dirty, ts);
  CHECK(feq(bs.localBounds(left.get()).min.x, -2));
  CHECK(feq(bs.localBounds(root.get()).min.x, -22));

  // Removing one shared edge must remove its union, not the surviving instance.
  dirty.clear();
  setF(a, "geometry", std::shared_ptr<X3DNode>{});
  dirty.markDirty(a.get(), DirtyChildren);
  ts.propagate(dirty); bs.propagate(dirty, ts);
  CHECK(bs.localBounds(left.get()).empty);
  CHECK(feq(bs.localBounds(root.get()).min.x, 8));
  dirty.clear();
  setF(box, "size", SFVec3f{6, 6, 6});
  dirty.markDirty(box.get(), DirtyBounds);
  bs.propagate(dirty, ts);
  CHECK(feq(bs.localBounds(root.get()).min.x, 7));
  CHECK(feq(bs.localBounds(b.get()).size().x, 6));
}

TEST_CASE("bounds_dense_dag_incremental_and_noop") {
  auto shape = createX3DNode("Shape");
  auto box = createX3DNode("Box");
  setF(shape, "geometry", box);
  auto root = shape;
  // Repeated diamonds yield millions of paths but only 50 distinct nodes.
  for (int i = 0; i != 16; ++i) {
    auto a = createX3DNode("Group"), b = createX3DNode("Group");
    auto next = createX3DNode("Group");
    addChild(a, root); addChild(b, root);
    addChild(next, a); addChild(next, b);
    root = next;
  }
  Scene scene; scene.addRootNode(root);
  TransformSystem ts; ts.buildIndex(scene);
  BoundsSystem bs; bs.buildBounds(scene, ts);
  setF(box, "size", SFVec3f{4, 4, 4});
  DirtyTracker dirty; dirty.markDirty(box.get(), DirtyBounds);
  bs.propagate(dirty, ts);
  CHECK(feq(bs.localBounds(root.get()).size().x, 4));
  const auto revision = bs.revision();
  dirty.clear(); dirty.markDirty(box.get(), DirtyBounds);
  bs.propagate(dirty, ts);
  CHECK(bs.revision() == revision);
}

TEST_CASE("bounds_removed_dirty_subtree_is_not_dereferenced") {
  auto root = createX3DNode("Transform");
  auto child = createX3DNode("Transform");
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", createX3DNode("Box"));
  addChild(child, shape); addChild(root, child);
  Scene scene; scene.addRootNode(root);
  TransformSystem ts; ts.buildIndex(scene);
  BoundsSystem bs; bs.buildBounds(scene, ts);
  DirtyTracker dirty;
  // Dirty descendants may die before their parent change is processed.
  dirty.markDirty(child.get(), DirtyLocalTransform | DirtyChildren);
  dirty.markDirty(shape.get(), DirtyBounds);
  dirty.markDirty(root.get(), DirtyLocalTransform | DirtyChildren);
  std::weak_ptr<X3DNode> dead = child;
  child.reset(); shape.reset();
  setF(root, "children", std::vector<std::shared_ptr<X3DNode>>{});
  CHECK(dead.expired());
  ts.propagate(dirty); bs.propagate(dirty, ts);
  CHECK(bs.localBounds(root.get()).empty);
}
