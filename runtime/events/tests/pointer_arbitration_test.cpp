#include "doctest/doctest.h"
// pointer_arbitration_test.cpp — the per-tick "pointer consumed by a sensor"
// flag on X3DExecutionContext (nav-vs-sensor arbitration seam). Default false;
// settable within a tick; reset to false at the start of each tick().
#include "X3DExecutionContext.hpp"
#include "PointingSensorSystem.hpp"
#include "X3DDocument.hpp"
#include "x3d/nodes/Box.hpp"
#include "x3d/nodes/Group.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/TouchSensor.hpp"
#include "Ray.hpp"
#include <any>
#include <iostream>
#include <memory>
#include <vector>

using namespace x3d;
using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;

static int failures = 0;
static void check(bool c, const char *what) {
  if (!c) { std::cerr << "FAIL: " << what << "\n"; ++failures; }
  else    { std::cout << "ok: " << what << "\n"; }
}

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

static void test_pointing_sets_flag() {
  auto group = std::make_shared<Group>();
  auto sensor = std::make_shared<TouchSensor>();
  auto shape = std::make_shared<Shape>();
  auto box = std::make_shared<Box>();
  setF(box, "size", std::any(SFVec3f{2, 2, 2}));
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(box)));
  addChild(group, sensor);
  addChild(group, shape);
  Scene scene; scene.addRootNode(group);

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  ctx.addSystem(std::make_shared<PointingSensorSystem>());

  // Pointer straight down -Z at the box, present, button up: over but no grab.
  ctx.setPointerPresent(true);
  ctx.setPointer(Ray{{0, 0, 10}, {0, 0, -1}});
  ctx.tick(1.0);
  check(ctx.pointerConsumedBySensor() == false, "over-but-no-grab => flag false");

  // Button down while over => grab => flag true.
  ctx.setPointerButton(true);
  ctx.tick(2.0);
  check(ctx.pointerConsumedBySensor() == true, "active grab => flag true");

  // Button up => grab released => flag false again.
  ctx.setPointerButton(false);
  ctx.tick(3.0);
  check(ctx.pointerConsumedBySensor() == false, "grab released => flag false");
}

// A System that sets the flag during update — proves a system can claim the
// pointer within a tick.
struct Claimer : System {
  void attach(X3DNode *, X3DExecutionContext &) override {}
  void update(double, X3DExecutionContext &ctx) override {
    ctx.setPointerConsumedBySensor(true);
  }
};

TEST_CASE("pointer_arbitration_test") {
  X3DExecutionContext ctx;
  check(ctx.pointerConsumedBySensor() == false, "default is false");

  ctx.setPointerConsumedBySensor(true);
  check(ctx.pointerConsumedBySensor() == true, "setter works");

  // tick() must reset the flag at tick start; with no claiming system it ends false.
  ctx.tick(1.0);
  check(ctx.pointerConsumedBySensor() == false, "tick() resets flag to false");

  // A claiming system leaves it true at end of tick.
  ctx.addSystem(std::make_shared<Claimer>());
  ctx.tick(2.0);
  check(ctx.pointerConsumedBySensor() == true, "claiming system sets flag during tick");

  test_pointing_sets_flag();

  CHECK(failures == 0);
  return;
}
