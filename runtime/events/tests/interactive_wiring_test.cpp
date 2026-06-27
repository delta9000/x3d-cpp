#include "doctest/doctest.h"
// interactive_wiring_test.cpp — headless interaction acceptance gate (no GL).
// Proves attachInteractive wires both systems so that (a) a click fires a
// TouchSensor through the cascade, (b) WASD drives FLY navigation, and (c) a
// grab suppresses nav drag (arbitration ordering: pointing before nav).
#include "X3DSceneBridge.hpp"
#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/Box.hpp"
#include "x3d/nodes/Group.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/TouchSensor.hpp"
#include "x3d/nodes/Viewpoint.hpp"
#include "Ray.hpp"
#include <any>
#include <cmath>
#include <iostream>
#include <memory>
#include <vector>

using namespace x3d;
using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;

static int failures = 0;
static void check(bool c, const char *what) {
  if (!c) { std::cerr << "FAIL: " << what << "\n"; ++failures; }
  else    { std::cout << "ok: " << what << "\n"; }
}
static bool sameMatrix(const Mat4 &a, const Mat4 &b) {
  for (int i = 0; i < 16; ++i) if (std::fabs(a.m[i] - b.m[i]) > 1e-6f) return false;
  return true;
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

TEST_CASE("interactive_wiring_test") {
  // Scene: a viewpoint + a Group{ TouchSensor, Shape{ Box } }.
  auto group = std::make_shared<Group>();
  auto sensor = std::make_shared<TouchSensor>();
  auto shape = std::make_shared<Shape>();
  auto box = std::make_shared<Box>();
  setF(box, "size", std::any(SFVec3f{2, 2, 2}));
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(box)));
  addChild(group, sensor);
  addChild(group, shape);
  auto vp = std::make_shared<Viewpoint>();
  Scene scene; scene.addRootNode(vp); scene.addRootNode(group);

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  auto nav = attachInteractive(scene, ctx);
  check(nav != nullptr, "attachInteractive returns the NavigationSystem");

  // (a) Click on the box fires the TouchSensor through the cascade.
  ctx.setPointerPresent(true);
  ctx.setPointer(Ray{{0, 0, 10}, {0, 0, -1}}); // at the box
  ctx.setPointerButton(true);
  ctx.tick(1.0);                                // grab begins
  check(sensor->getIsActive() == true, "(a) TouchSensor isActive on click");
  ctx.setPointerButton(false);
  ctx.tick(2.0);                                // release while over ⇒ touchTime
  check(std::fabs(sensor->getTouchTime() - 2.0) < 1e-9, "(a) touchTime == release time");

  // (b) FLY + forward key moves the camera.
  nav->setForcedMode(NavigationSystem::Mode::Fly);
  ctx.setPointerPresent(false);
  ctx.tick(3.0);
  Mat4 beforeNav = ctx.viewMatrix();
  ctx.setKey(NavigationSystem::kKeyForward, true);
  ctx.tick(3.5);
  check(!sameMatrix(beforeNav, ctx.viewMatrix()), "(b) WASD drives FLY navigation");

  // (c) Arbitration invariant: a drag that begins on the TouchSensor must NOT
  // rotate the camera — the sensor grab claims the pointer (sets
  // pointerConsumedBySensor) and NavigationSystem honors it. NOTE: this does
  // NOT discriminate addSystem order — tick() re-runs all systems until cascade
  // quiescence and only resets the flag at tick start, so PointingSensorSystem's
  // claim in one iteration is visible to nav in a later one regardless of order
  // (verified by reversing the order: still suppressed). attachInteractive still
  // adds pointing first as defense for any single-iteration tick. What this gate
  // protects is the invariant itself, not the registration order.
  nav->setForcedMode(NavigationSystem::Mode::Examine);
  ctx.setPointerPresent(true);
  ctx.setPointerButton(false);
  ctx.setPointer(Ray{{0, 0, 10}, {0, 0, -1}}); // over the box
  ctx.tick(4.0);
  Mat4 beforeDrag = ctx.viewMatrix();
  ctx.setPointerButton(true);                     // grab the TouchSensor
  ctx.tick(4.5);                                  // grab begins; drag origin set
  ctx.setPointer(Ray{{0.3f, 0, 10}, {0, 0, -1}}); // move while grabbed
  ctx.tick(5.0);
  check(sameMatrix(beforeDrag, ctx.viewMatrix()),
        "(c) drag on a sensor does not rotate the camera (grab consumes pointer)");

  CHECK(failures == 0);
  return;
}
