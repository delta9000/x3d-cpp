// transform_system_test.cpp — transform-hierarchy index + incremental world
// propagation. Builds A>B>C plus a sibling D under A; dirties B; asserts B and C
// world transforms update and the sibling D is NOT re-propagated.
#include "TransformSystem.hpp"
#include "DirtyTracker.hpp"
#include "X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp" // out-of-line Scene::addRootNode definition
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
using namespace x3d::runtime;

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }

// helper: set a Transform's translation via reflection (the data-layer setter).
static void setTranslation(const std::shared_ptr<X3DNode>& n, SFVec3f v) {
  for (auto& f : n->fields())
    if (f.x3dName == "translation" && f.set) { f.set(*n, std::any(v)); return; }
}
static void addChild(const std::shared_ptr<X3DNode>& parent,
                     const std::shared_ptr<X3DNode>& child) {
  for (auto& f : parent->fields())
    if (f.x3dName == "children" && f.set) {
      auto kids = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*parent));
      kids.push_back(child);
      f.set(*parent, std::any(std::move(kids)));
      return;
    }
}

TEST_CASE("transform_system_test") {
  auto A = createX3DNode("Transform");
  auto B = createX3DNode("Transform");
  auto C = createX3DNode("Transform");
  auto D = createX3DNode("Transform"); // sibling of B under A
  setTranslation(A, {1,0,0});
  setTranslation(B, {0,2,0});
  setTranslation(C, {0,0,3});
  setTranslation(D, {5,5,5});
  addChild(A, B); addChild(B, C); addChild(A, D);

  Scene scene; scene.addRootNode(A);

  TransformSystem ts;
  ts.buildIndex(scene);

  // Initial world transforms (column-translation = sum down the chain).
  CHECK((feq(ts.worldTransform(A.get()).transformPoint({0,0,0}).x, 1)));
  // C world translation = A + B + C = (1,2,3)
  auto cw = ts.worldTransform(C.get()).transformPoint({0,0,0});
  CHECK((feq(cw.x,1) && feq(cw.y,2) && feq(cw.z,3)));
  auto dw = ts.worldTransform(D.get()).transformPoint({0,0,0});
  CHECK((feq(dw.x,6) && feq(dw.y,5) && feq(dw.z,5))); // A(1,0,0)+D(5,5,5)

  // Change B's local translation, mark it dirty, propagate.
  setTranslation(B, {0,10,0});
  DirtyTracker dirty;
  dirty.markDirty(B.get(), DirtyLocalTransform);
  ts.propagate(dirty);

  // B and C world transforms updated; D untouched.
  auto bw2 = ts.worldTransform(B.get()).transformPoint({0,0,0});
  CHECK((feq(bw2.x,1) && feq(bw2.y,10) && feq(bw2.z,0)));
  auto cw2 = ts.worldTransform(C.get()).transformPoint({0,0,0});
  CHECK((feq(cw2.x,1) && feq(cw2.y,10) && feq(cw2.z,3)));
  // Incremental: B and C marked DirtyWorldTransform; D and A are NOT.
  CHECK((dirty.flags(B.get()) & DirtyWorldTransform));
  CHECK((dirty.flags(C.get()) & DirtyWorldTransform));
  CHECK((!(dirty.flags(D.get()) & DirtyWorldTransform)));
  CHECK((!(dirty.flags(A.get()) & DirtyWorldTransform)));
  return;
}
