#include "doctest/doctest.h"
// animation_test.cpp
// End-to-end proof that the event foundation drives a real X3D animation:
// a clock tick drives a TimeSensor, whose fraction_changed ROUTEs to a
// PositionInterpolator, whose value_changed ROUTEs to a Transform's translation.
// This exercises every layer: clock -> active-node update -> outputOnly emit ->
// cascade -> inputOnly handler -> outputOnly emit -> cascade -> inputOutput sink.
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "InterpolatorSystem.hpp"
#include "Interpolation.hpp"
#include "TimeSensorBehavior.hpp"
#include "X3DExecutionContext.hpp"

#include "PositionInterpolator.hpp"
#include "TimeSensor.hpp"
#include "Transform.hpp"

#include <iostream>
#include <memory>
#include <string>

using namespace x3d;
using namespace x3d::runtime;

namespace {

int failures = 0;

void check(bool cond, const std::string &what) {
  if (!cond) {
    std::cerr << "FAIL: " << what << "\n";
    ++failures;
  } else {
    std::cout << "ok: " << what << "\n";
  }
}

bool veq(const SFVec3f &a, float x, float y, float z) {
  return a.x == x && a.y == y && a.z == z;
}

SFVec3f tr(const std::shared_ptr<Transform> &t) { return t->getTranslation(); }

// TimeSensor (cycle 1s) -> PositionInterpolator (key 0..1, value (0,0,0)..(10,0,0))
// -> Transform.translation. A tick at fraction f should place the Transform at
// (10*f, 0, 0).
void test_animation_chain() {
  auto ts = std::make_shared<TimeSensor>();
  ts->setCycleInterval(1.0);
  ts->setStartTime(0.0);
  ts->setLoop(false);

  auto interp = std::make_shared<PositionInterpolator>();
  interp->setKey(MFFloat{0.0f, 1.0f});
  interp->setKeyValue(MFVec3f{SFVec3f{0, 0, 0}, SFVec3f{10, 0, 0}});

  auto target = std::make_shared<Transform>();

  X3DExecutionContext ctx;
  ctx.addRoute({ts.get(), "fraction_changed"}, {interp.get(), "set_fraction"});
  ctx.addRoute({interp.get(), "value_changed"}, {target.get(), "translation"});
  // The deprecation shim wraps an ActiveNode in a one-node System; new code
  // should implement System directly. Tracked for migration.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  ctx.addActiveNode(std::make_shared<TimeSensorBehavior>(ts.get()));
#pragma GCC diagnostic pop
  InterpolatorSystem<PositionInterpolator, SFVec3f> interpSys(
      [](const SFVec3f &a, const SFVec3f &b, float t) { return lerpVec3(a, b, t); });
  interpSys.attach(interp.get(), ctx);

  ctx.tick(0.5);
  check(veq(tr(target), 5, 0, 0), "tick(0.5) animates translation to (5,0,0)");

  ctx.tick(0.25);
  check(veq(tr(target), 2.5f, 0, 0),
        "tick(0.25) animates translation to (2.5,0,0)");

  ctx.tick(1.0);
  check(veq(tr(target), 10, 0, 0),
        "tick(1.0) animates translation to (10,0,0) (clamped end)");
}

} // namespace

TEST_CASE("animation_test") {
  test_animation_chain();

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    CHECK(false); return;
  }
  std::cout << "all animation tests passed\n";
  return;
}
