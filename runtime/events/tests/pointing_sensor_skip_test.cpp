// pointing_sensor_skip_test.cpp — PERF-POINTING regression.
//
// PointingSensorSystem resolves the pointer against the scene with a full-scene
// pick (ctx.pick) on every pointer motion. A consumer that re-feeds the pointer
// ray each frame (the OpenGL PoC does) therefore drove a brute-force scene pick
// EVERY tick — ~8ms/frame on the kelp exhibit — even though that scene contains
// no pointing-device sensors at all, so the pick can never resolve to one.
//
// Contract: when a full scene inventory (the per-node attach pass the bridge
// runs) finds ZERO pointing-device sensors, the system performs no pick. When
// the inventory was never taken, or a sensor exists, it still picks (so sensors
// keep working). Observed via X3DExecutionContext::pickCallCount().

#include "doctest/doctest.h"

#include "PointingSensorSystem.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSceneBridge.hpp" // detail::forEachNode (the bridge's inventory pass)
#include "X3DScene.hpp"

#include "x3d/nodes/Box.hpp"
#include "x3d/nodes/Group.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/TouchSensor.hpp"

#include <any>
#include <memory>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;

namespace {

void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
void addChild(const std::shared_ptr<X3DNode> &p,
              const std::shared_ptr<X3DNode> &c) {
  for (auto &f : p->fields())
    if (f.x3dName == "children" && f.set) {
      auto k = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*p));
      k.push_back(c);
      f.set(*p, std::any(std::move(k)));
      return;
    }
}
std::shared_ptr<X3DNode> boxShape() {
  auto shape = std::make_shared<Shape>();
  auto box = std::make_shared<Box>();
  setF(box, "size", std::any(SFVec3f{2, 2, 2}));
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(box)));
  return shape;
}
Ray downZ() { return Ray{{0, 0, 10}, {0, 0, -1}}; }

// Mirror the bridge wiring: build, run the per-node inventory pass, register.
void wireInventoried(X3DExecutionContext &ctx, Scene &scene,
                     const std::shared_ptr<PointingSensorSystem> &sys) {
  ctx.buildSceneGraph(scene);
  detail::forEachNode(scene, [&](X3DNode *n) { sys->attach(n, ctx); });
  ctx.addSystem(sys);
}

} // namespace

TEST_CASE("pointing system performs no per-frame pick when the scene has no "
          "pointing sensors") {
  auto root = std::make_shared<Group>();
  addChild(root, boxShape()); // a plain Shape — NO TouchSensor / drag sensor
  Scene scene;
  scene.addRootNode(root);

  X3DExecutionContext ctx;
  auto sys = std::make_shared<PointingSensorSystem>();
  wireInventoried(ctx, scene, sys);

  ctx.setPointerPresent(true);
  const std::uint64_t before = X3DExecutionContext::pickCallCount();
  // Two ticks with a MOVING pointer (revision bumps each tick) — the exact PoC
  // pattern that, unguarded, picked the whole scene every frame.
  ctx.setPointer(downZ());
  ctx.tick(0.0);
  ctx.setPointer(Ray{{0.1f, 0, 10}, {0, 0, -1}});
  ctx.tick(0.016);
  CHECK(X3DExecutionContext::pickCallCount() - before == 0);
}

TEST_CASE("pointing system still picks when a TouchSensor is present "
          "(skip is not over-eager)") {
  auto group = std::make_shared<Group>();
  addChild(group, std::make_shared<TouchSensor>());
  addChild(group, boxShape());
  Scene scene;
  scene.addRootNode(group);

  X3DExecutionContext ctx;
  auto sys = std::make_shared<PointingSensorSystem>();
  wireInventoried(ctx, scene, sys);

  ctx.setPointerPresent(true);
  const std::uint64_t before = X3DExecutionContext::pickCallCount();
  ctx.setPointer(downZ());
  ctx.tick(0.0);
  CHECK(X3DExecutionContext::pickCallCount() - before >= 1);
}
