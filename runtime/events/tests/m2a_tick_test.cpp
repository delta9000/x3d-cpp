// m2a_tick_test.cpp — end-to-end: after buildSceneGraph, a ROUTE delivering into
// a Transform's translation during tick() marks it dirty and updates its world
// transform; the consumer pulls the changed-set + world matrix after tick.
#include "X3DExecutionContext.hpp"
#include "DirtyTracker.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp" // out-of-line Scene::addRootNode definition
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }
static void addChild(const std::shared_ptr<X3DNode>& parent,
                     const std::shared_ptr<X3DNode>& child) {
  for (auto& f : parent->fields())
    if (f.x3dName == "children" && f.set) {
      auto kids = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*parent));
      kids.push_back(child); f.set(*parent, std::any(std::move(kids))); return;
    }
}

TEST_CASE("m2a_tick_test") {
  auto root = createX3DNode("Transform");
  auto T = createX3DNode("Transform");
  addChild(root, T);
  Scene scene; scene.addRootNode(root);

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);

  // Seed a translation event onto T and tick. (Directly posting the event models
  // what a ROUTE into T.translation delivers during the cascade.)
  ctx.postEvent(T.get(), "translation", std::any(SFVec3f{4, 0, 0}));
  ctx.tick(0.0);

  // Pull: T appears in the changed-set, flagged local + world dirty, and its
  // world transform reflects the new translation.
  bool found = false;
  for (const X3DNode* n : ctx.dirtyTracker().changedNodes())
    if (n == T.get()) found = true;
  CHECK((found));
  CHECK((ctx.dirtyTracker().flags(T.get()) & DirtyLocalTransform));
  CHECK((ctx.dirtyTracker().flags(T.get()) & DirtyWorldTransform));
  auto w = ctx.worldTransform(T.get()).transformPoint({0,0,0});
  CHECK((feq(w.x,4) && feq(w.y,0) && feq(w.z,0)));

  // A second tick with no events clears the prior changed-set.
  ctx.tick(1.0);
  CHECK((ctx.dirtyTracker().changedNodes().empty()));
  return;
}
