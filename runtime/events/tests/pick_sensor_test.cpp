// pick_sensor_test.cpp — §38 LinePickSensor wiring through PickSensorSystem.
// Builds a LinePickSensor with an IndexedLineSet pickingGeometry and a target
// Shape, ticks, and reads the sensor outputs back off the generated getters.
// Covers: a segment crossing a target box picks it (isActive + pickedGeometry +
// pickedPoint); moving the geometry away drops it; enabled=false never picks;
// objectType filtering.

#include "doctest/doctest.h"

#include "PickSensorSystem.hpp"
#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSceneBridge.hpp" // detail::forEachNode (the bridge's inventory pass)

#include "x3d/nodes/X3DNodeFactory.hpp"
#include "x3d/nodes/LinePickSensor.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/Box.hpp"

#include <any>
#include <cmath>
#include <memory>
#include <vector>

using namespace x3d;
using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;

namespace {

void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}

std::shared_ptr<X3DNode> makeCoord(std::vector<SFVec3f> pts) {
  auto c = createX3DNode("Coordinate");
  setF(c, "point", std::any(std::move(pts)));
  return c;
}

// An IndexedLineSet pickingGeometry with one polyline run through `pts`.
std::shared_ptr<X3DNode> makeLine(std::vector<SFVec3f> pts) {
  auto ils = createX3DNode("IndexedLineSet");
  setF(ils, "coord", std::any(makeCoord(std::move(pts))));
  setF(ils, "coordIndex", std::any(MFInt32{0, 1, -1}));
  return ils;
}

std::shared_ptr<X3DNode> boxShape() {
  auto shape = std::make_shared<Shape>();
  auto box = std::make_shared<Box>();
  setF(box, "size", std::any(SFVec3f{2, 2, 2}));
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(box)));
  return shape;
}

struct Rig {
  X3DExecutionContext ctx;
  std::shared_ptr<PickSensorSystem> sys = std::make_shared<PickSensorSystem>();
  // Mirror the bridge wiring: build, run the per-node inventory pass, register.
  void build(Scene &scene) {
    ctx.buildSceneGraph(scene);
    detail::forEachNode(scene, [&](X3DNode *n) { sys->attach(n, ctx); });
    ctx.addSystem(sys);
  }
};

} // namespace

TEST_CASE("LinePickSensor picks a target crossed by its segment") {
  auto sensor = std::make_shared<LinePickSensor>();
  setF(sensor, "pickingGeometry", std::any(makeLine({{-5, 0, 0}, {5, 0, 0}})));
  setF(sensor, "pickTarget", std::any(MFNode{boxShape()}));

  auto group = createX3DNode("Group");
  setF(group, "children", std::any(MFNode{sensor}));
  Scene scene; scene.addRootNode(group);
  Rig r; r.build(scene);
  r.ctx.tick(1.0);

  CHECK(sensor->getIsActive() == true);
  REQUIRE(sensor->getPickedGeometry().size() == 1);
  CHECK(sensor->getPickedGeometry()[0] != nullptr);
  REQUIRE(sensor->getPickedPoint().size() == 1);
  // Entry face of the box at x=-1 (segment runs -x→+x through the origin).
  CHECK(std::fabs(sensor->getPickedPoint()[0].x - (-1.0f)) < 1e-3f);
  CHECK(std::fabs(sensor->getPickedPoint()[0].y) < 1e-3f);

  // Move the geometry off the box → dropped, isActive falls.
  setF(sensor, "pickingGeometry", std::any(makeLine({{-5, 5, 0}, {5, 5, 0}})));
  r.ctx.tick(2.0);
  CHECK(sensor->getIsActive() == false);
}

TEST_CASE("LinePickSensor enabled=false never picks") {
  auto sensor = std::make_shared<LinePickSensor>();
  setF(sensor, "pickingGeometry", std::any(makeLine({{-5, 0, 0}, {5, 0, 0}})));
  setF(sensor, "pickTarget", std::any(MFNode{boxShape()}));
  setF(sensor, "enabled", std::any(SFBool{false}));

  auto group = createX3DNode("Group");
  setF(group, "children", std::any(MFNode{sensor}));
  Scene scene; scene.addRootNode(group);
  Rig r; r.build(scene);
  r.ctx.tick(1.0);

  CHECK(sensor->getIsActive() == false);
  CHECK(sensor->getPickedGeometry().empty());
}

TEST_CASE("LinePickSensor honors objectType filtering") {
  auto sensor = std::make_shared<LinePickSensor>();
  setF(sensor, "pickingGeometry", std::any(makeLine({{-5, 0, 0}, {5, 0, 0}})));
  setF(sensor, "pickTarget", std::any(MFNode{boxShape()}));
  setF(sensor, "objectType", std::any(MFString{"NONE"}));

  auto group = createX3DNode("Group");
  setF(group, "children", std::any(MFNode{sensor}));
  Scene scene; scene.addRootNode(group);
  Rig r; r.build(scene);
  r.ctx.tick(1.0);
  CHECK(sensor->getIsActive() == false);

  // 'GEOMETRY' matches the geometry-bearing target.
  setF(sensor, "objectType", std::any(MFString{"GEOMETRY"}));
  r.ctx.tick(2.0);
  CHECK(sensor->getIsActive() == true);
}
