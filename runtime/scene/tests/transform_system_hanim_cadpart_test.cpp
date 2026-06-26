// transform_system_hanim_cadpart_test.cpp — M2C-4: TransformSystem covers
// HAnimHumanoid, HAnimJoint, and CADPart as transform-bearing nodes (each
// carries the full TRS field set: translation/rotation/scale/center/
// scaleOrientation). Asserts that a child Shape's world matrix is composed
// correctly through each new type, just as it is for Transform.
//
// Billboard is view-dependent (active Viewpoint) — deferred to M2c/M2d.
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

static void setTranslation(const std::shared_ptr<X3DNode> &n, SFVec3f v) {
  for (auto &f : n->fields())
    if (f.x3dName == "translation" && f.set) { f.set(*n, std::any(v)); return; }
}

static void setChildren(const std::shared_ptr<X3DNode> &parent,
                        const std::shared_ptr<X3DNode> &child) {
  for (auto &f : parent->fields())
    if (f.x3dName == "children" && f.set) {
      auto kids =
          std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*parent));
      kids.push_back(child);
      f.set(*parent, std::any(std::move(kids)));
      return;
    }
}

// Some HAnim/CADPart types use "skeleton" or specific slots for children.
// We add a Transform child via the "children" slot where available; if the
// node doesn't carry "children" we use any MFNode field it does have. For
// world-transform purposes it doesn't matter which slot — the walk descends
// ALL node-typed fields.
static bool addToFirstMFNodeField(const std::shared_ptr<X3DNode> &parent,
                                  const std::shared_ptr<X3DNode> &child) {
  for (auto &f : parent->fields()) {
    if (f.type != X3DFieldType::MFNode) continue;
    if (!f.get || !f.set) continue;
    auto kids =
        std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*parent));
    kids.push_back(child);
    f.set(*parent, std::any(std::move(kids)));
    return true;
  }
  return false;
}

static bool addToNamedMFNodeField(const std::shared_ptr<X3DNode> &parent,
                                  const char *name,
                                  const std::shared_ptr<X3DNode> &child) {
  for (auto &f : parent->fields()) {
    if (f.x3dName != name || f.type != X3DFieldType::MFNode || !f.get || !f.set)
      continue;
    auto kids =
        std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*parent));
    kids.push_back(child);
    f.set(*parent, std::any(std::move(kids)));
    return true;
  }
  return false;
}

static void addChild(const std::shared_ptr<X3DNode> &parent,
                     const std::shared_ptr<X3DNode> &child) {
  // Prefer the real geometry-hierarchy container: "children" (Transform/Group),
  // then "skeleton" (HAnimHumanoid's child-hierarchy slot — NOT "joints", which
  // is a list of USE references). Fall back to any MFNode for other types.
  for (auto &f : parent->fields())
    if (f.x3dName == "children" && f.set) { setChildren(parent, child); return; }
  if (addToNamedMFNodeField(parent, "skeleton", child)) return;
  addToFirstMFNodeField(parent, child);
}

// Run one test scenario: parentType node with translation (tx,ty,tz), child
// Transform with translation (cx,cy,cz). Assert the child's world translation
// equals the sum.
static void runScenario(const char *parentType, float tx, float ty, float tz,
                        float cx, float cy, float cz) {
  auto parent = createX3DNode(parentType);
  CHECK((parent && "createX3DNode must return non-null for known type"));
  setTranslation(parent, {tx, ty, tz});

  auto child = createX3DNode("Transform");
  setTranslation(child, {cx, cy, cz});
  addChild(parent, child);

  Scene scene;
  scene.addRootNode(parent);

  TransformSystem ts;
  ts.buildIndex(scene);

  // Parent world = its own translation.
  auto pw = ts.worldTransform(parent.get()).transformPoint({0, 0, 0});
  CHECK((feq(pw.x, tx) && feq(pw.y, ty) && feq(pw.z, tz)));

  // Child world = parent world * child local = sum of translations.
  auto cw = ts.worldTransform(child.get()).transformPoint({0, 0, 0});
  CHECK((feq(cw.x, tx + cx) && feq(cw.y, ty + cy) && feq(cw.z, tz + cz)));
}

// Test incremental propagation: dirty parent re-propagates to child.
static void runPropagateScenario(const char *parentType) {
  auto parent = createX3DNode(parentType);
  setTranslation(parent, {1, 0, 0});
  auto child = createX3DNode("Transform");
  setTranslation(child, {0, 2, 0});
  addChild(parent, child);

  Scene scene;
  scene.addRootNode(parent);

  TransformSystem ts;
  ts.buildIndex(scene);

  // Change parent translation, dirty-propagate.
  setTranslation(parent, {3, 0, 0});
  DirtyTracker dirty;
  dirty.markDirty(parent.get(), DirtyLocalTransform);
  ts.propagate(dirty);

  auto cw = ts.worldTransform(child.get()).transformPoint({0, 0, 0});
  CHECK((feq(cw.x, 3) && feq(cw.y, 2) && feq(cw.z, 0)));
  CHECK((dirty.flags(parent.get()) & DirtyWorldTransform));
  CHECK((dirty.flags(child.get()) & DirtyWorldTransform));
}

TEST_CASE("transform_system_hanim_cadpart_test") {
  // --- HAnimHumanoid ---
  runScenario("HAnimHumanoid", 1, 0, 0, 0, 2, 0);
  runPropagateScenario("HAnimHumanoid");

  // --- HAnimJoint ---
  runScenario("HAnimJoint", 0, 1, 0, 0, 0, 3);
  runPropagateScenario("HAnimJoint");

  // --- CADPart ---
  runScenario("CADPart", 0, 0, 2, 4, 0, 0);
  runPropagateScenario("CADPart");

  // --- Nested: HAnimHumanoid > HAnimJoint > Transform ---
  {
    auto humanoid = createX3DNode("HAnimHumanoid");
    setTranslation(humanoid, {1, 0, 0});
    auto joint = createX3DNode("HAnimJoint");
    setTranslation(joint, {0, 1, 0});
    auto leaf = createX3DNode("Transform");
    setTranslation(leaf, {0, 0, 1});
    addChild(humanoid, joint);
    addChild(joint, leaf);
    Scene scene;
    scene.addRootNode(humanoid);
    TransformSystem ts;
    ts.buildIndex(scene);
    auto lw = ts.worldTransform(leaf.get()).transformPoint({0, 0, 0});
    CHECK((feq(lw.x, 1) && feq(lw.y, 1) && feq(lw.z, 1)));
  }

  return;
}
