// transform_system_test.cpp — transform-hierarchy index + incremental world
// propagation. Builds A>B>C plus a sibling D under A; dirties B; asserts B and C
// world transforms update and the sibling D is NOT re-propagated.
#include "TransformSystem.hpp"
#include "DirtyTracker.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp" // out-of-line Scene::addRootNode definition
#include "doctest/doctest.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;

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
static void removeChild(const std::shared_ptr<X3DNode>& parent,
                        const std::shared_ptr<X3DNode>& child) {
  for (auto& f : parent->fields())
    if (f.x3dName == "children" && f.set) {
      auto kids = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*parent));
      kids.erase(std::remove(kids.begin(), kids.end(), child), kids.end());
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

// M2C-2 structural re-index — add a child Transform under a Group at runtime.
TEST_CASE("transform_system_structural_add") {
  auto R = createX3DNode("Transform"); // root, translation (10,0,0)
  auto G = createX3DNode("Group");     // grouping node under R
  setTranslation(R, {10,0,0});
  addChild(R, G);

  Scene scene; scene.addRootNode(R);
  TransformSystem ts; ts.buildIndex(scene);
  const std::uint64_t rev0 = ts.revision();

  // No transform child yet.
  auto C = createX3DNode("Transform");
  setTranslation(C, {0,5,0});
  addChild(G, C);
  DirtyTracker dirty;
  dirty.markDirty(G.get(), DirtyChildren);
  ts.propagate(dirty);

  // C's world = R * C = (10,5,0); the index grew and revision bumped.
  auto cw = ts.worldTransform(C.get()).transformPoint({0,0,0});
  CHECK((feq(cw.x,10) && feq(cw.y,5) && feq(cw.z,0)));
  CHECK((ts.revision() > rev0));
  CHECK((dirty.flags(C.get()) & DirtyWorldTransform));
  return;
}

// M2C-2 — remove a runtime-added child: the index drops its subtree.
TEST_CASE("transform_system_structural_remove") {
  auto R = createX3DNode("Transform");
  auto G = createX3DNode("Group");
  setTranslation(R, {10,0,0});
  addChild(R, G);
  Scene scene; scene.addRootNode(R);
  TransformSystem ts; ts.buildIndex(scene);

  auto C = createX3DNode("Transform");
  setTranslation(C, {0,5,0});
  addChild(G, C);
  DirtyTracker add;
  add.markDirty(G.get(), DirtyChildren);
  ts.propagate(add);
  CHECK((feq(ts.worldTransform(C.get()).transformPoint({0,0,0}).x, 10)));

  const std::uint64_t rev1 = ts.revision();
  removeChild(G, C);
  DirtyTracker rem;
  rem.markDirty(G.get(), DirtyChildren);
  ts.propagate(rem);

  // C is no longer indexed (identity), and the revision bumped.
  CHECK((feq(ts.worldTransform(C.get()).transformPoint({5,5,5}).x, 5))); // identity
  CHECK((ts.worldTransformUnder(R.get(), C.get()).transformPoint({5,5,5}).x == 5));
  CHECK((ts.revision() > rev1));
  return;
}

// M2C-2 — a DEF/USE Transform under two parents has a world per parent.
TEST_CASE("transform_system_defuse_two_parents") {
  auto P1 = createX3DNode("Transform");
  auto P2 = createX3DNode("Transform");
  auto T  = createX3DNode("Transform");
  setTranslation(P1, {1,0,0});
  setTranslation(P2, {10,0,0});
  setTranslation(T,  {0,2,0});
  addChild(P1, T);
  addChild(P2, T); // USE: the SAME shared node under a second parent

  Scene scene; scene.addRootNode(P1); scene.addRootNode(P2);
  TransformSystem ts; ts.buildIndex(scene);

  auto w1 = ts.worldTransformUnder(P1.get(), T.get()).transformPoint({0,0,0});
  CHECK((feq(w1.x,1) && feq(w1.y,2)));
  auto w2 = ts.worldTransformUnder(P2.get(), T.get()).transformPoint({0,0,0});
  CHECK((feq(w2.x,10) && feq(w2.y,2)));
  // Canonical (first-path) world is unchanged.
  CHECK((feq(ts.worldTransform(T.get()).transformPoint({0,0,0}).x, 1)));
  return;
}

// M2C-2 regression — a shared Transform moved from one parent Transform to
// another within a single tick must resolve to the SAME world regardless of
// which grouping node is marked DirtyChildren first. During processing both
// parents keep it (refCount_ stays > 1), so the canonical parent must be
// re-derived once every structural change is applied.
TEST_CASE("transform_system_move_between_parents_dirty_order") {
  auto run = [](bool g1First) {
    auto R = createX3DNode("Transform");
    auto G1 = createX3DNode("Transform");
    auto G2 = createX3DNode("Transform");
    auto T = createX3DNode("Transform");
    setTranslation(G1, {1,0,0});
    setTranslation(G2, {10,0,0});
    addChild(R, G1); addChild(R, G2); addChild(G1, T);
    Scene scene; scene.addRootNode(R);
    TransformSystem ts; ts.buildIndex(scene);
    // Canonical (first-path) parent is G1 (1,0,0).
    CHECK((feq(ts.worldTransform(T.get()).transformPoint({0,0,0}).x, 1)));

    removeChild(G1, T);
    addChild(G2, T);
    DirtyTracker dirty;
    if (g1First) {
      dirty.markDirty(G1.get(), DirtyChildren);
      dirty.markDirty(G2.get(), DirtyChildren);
    } else {
      dirty.markDirty(G2.get(), DirtyChildren);
      dirty.markDirty(G1.get(), DirtyChildren);
    }
    ts.propagate(dirty);
    CHECK((feq(ts.worldTransformUnder(G2.get(), T.get()).transformPoint({0,0,0}).x, 10)));
    return ts.worldTransform(T.get()).transformPoint({0,0,0}).x;
  };
  CHECK((feq(run(true), 10)));
  CHECK((feq(run(false), 10)));
  return;
}

// M2C-2 regression — removing a shared Transform from its canonical parent while
// another parent still reaches it re-points the canonical parent/world.
TEST_CASE("transform_system_remove_canonical_parent_repoints") {
  auto P1 = createX3DNode("Transform");
  auto P2 = createX3DNode("Transform");
  auto T  = createX3DNode("Transform");
  setTranslation(P1, {1,0,0});
  setTranslation(P2, {10,0,0});
  setTranslation(T,  {0,2,0});
  addChild(P1, T);
  addChild(P2, T); // USE: same node under a second parent
  Scene scene; scene.addRootNode(P1); scene.addRootNode(P2);
  TransformSystem ts; ts.buildIndex(scene);
  CHECK((feq(ts.worldTransform(T.get()).transformPoint({0,0,0}).x, 1))); // canonical P1
  const std::uint64_t rev = ts.revision();

  removeChild(P1, T);
  DirtyTracker dirty;
  dirty.markDirty(P1.get(), DirtyChildren);
  ts.propagate(dirty);

  // Canonical parent re-points to P2 and its world is recomputed in P2's frame.
  auto w = ts.worldTransform(T.get()).transformPoint({0,0,0});
  CHECK((feq(w.x, 10) && feq(w.y, 2)));
  CHECK((ts.revision() > rev));
  CHECK((feq(ts.worldTransformUnder(P2.get(), T.get()).transformPoint({0,0,0}).x, 10)));
  return;
}

// M2C-2 — revision bumps on transform and structural changes, not on a no-op.
TEST_CASE("transform_system_revision") {
  auto R = createX3DNode("Transform");
  auto C = createX3DNode("Transform");
  setTranslation(R, {1,0,0});
  addChild(R, C);
  Scene scene; scene.addRootNode(R);
  TransformSystem ts; ts.buildIndex(scene);
  const std::uint64_t rev0 = ts.revision();

  // A no-op propagate (empty dirty set) leaves revision untouched.
  DirtyTracker empty;
  ts.propagate(empty);
  CHECK((ts.revision() == rev0));

  // A non-transform dirty field is a no-op for the transform index too.
  DirtyTracker field;
  field.markDirty(C.get(), DirtyField);
  ts.propagate(field);
  CHECK((ts.revision() == rev0));

  // A local transform change bumps it.
  setTranslation(R, {2,0,0});
  DirtyTracker local;
  local.markDirty(R.get(), DirtyLocalTransform);
  ts.propagate(local);
  const std::uint64_t rev1 = ts.revision();
  CHECK((rev1 > rev0));

  // A no-op after the change keeps it stable.
  DirtyTracker empty2;
  ts.propagate(empty2);
  CHECK((ts.revision() == rev1));
  return;
}
