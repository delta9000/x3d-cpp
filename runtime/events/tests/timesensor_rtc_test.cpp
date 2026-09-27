#include "doctest/doctest.h"
// timesensor_rtc_test.cpp
// Regression tests for four verified TimeSensor / X3DTimeDependentNode
// spec-conformance bugs (RTC-1..RTC-4), driven through the clock via
// X3DExecutionContext::tick(now) over a TimeSensorSystem. Outputs are read back
// off the node getters (the cascade applies each outputOnly emit thunk during
// the tick).
//
//   RTC-1: cycleInterval <= 0 with loop=TRUE must not crash (divide-by-zero /
//          UB in the cycle-index/activation math) and must behave sanely.
//   RTC-2: a looping sensor must emit fraction_changed == 1.0 at each cycle
//          boundary tick (spec: if (f==0 && now>startTime) fraction_changed=1).
//   RTC-3: cycleTime value == "time at beginning of current cycle" ==
//          startTime + cycleIndex*cycleInterval, NOT the tick's `now`.
//   RTC-4: set_enabled FALSE while active must emit the final
//          time/fraction_changed/elapsedTime outputs BEFORE isActive=FALSE.
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "TimeSensorSystem.hpp"
#include "MediaTimeSystem.hpp"
#include "SoundTimeSystem.hpp"
#include "X3DExecutionContext.hpp"

#include "x3d/nodes/TimeSensor.hpp"
#include "x3d/nodes/AudioClip.hpp"
#include "x3d/nodes/MovieTexture.hpp"
#include "x3d/nodes/Gain.hpp"
#include "x3d/nodes/BiquadFilter.hpp"
#include "x3d/nodes/OscillatorSource.hpp"

#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace x3d;
using namespace x3d::runtime;
using namespace x3d::nodes;

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

constexpr double kEps = 1e-6;
bool deq(double a, double b) { return std::fabs(a - b) <= kEps; }

bool active(const std::shared_ptr<TimeSensor> &ts) {
  return ts->X3DTimeDependentNode::getIsActive();
}

struct Rig {
  std::shared_ptr<TimeSensor> ts = std::make_shared<TimeSensor>();
  X3DExecutionContext ctx;
  std::shared_ptr<TimeSensorSystem> sys = std::make_shared<TimeSensorSystem>();

  Rig() {
    ctx.addSystem(sys);
    sys->attach(ts.get(), ctx);
  }
};

// --- RTC-1: cycleInterval=0 + loop=TRUE must not crash -----------------------
void test_rtc1_zero_cycle_loop_no_crash() {
  Rig r;
  r.ts->setCycleInterval(0.0); // degenerate
  r.ts->setStartTime(0.0);
  r.ts->setLoop(true);

  // These ticks previously divided by zero / produced UB in the looping
  // cycle-index math. They must complete without crashing.
  r.ctx.tick(0.0);
  r.ctx.tick(1.0);
  r.ctx.tick(2.0);

  // Sanity: fraction stays a finite value in [0,1].
  const double f = r.ts->getFraction_changed();
  check(std::isfinite(f), "rtc1: fraction is finite with cycleInterval=0+loop");
  check(f >= 0.0 && f <= 1.0,
        "rtc1: fraction in [0,1] with cycleInterval=0+loop");
}

// --- RTC-2: looping emits fraction_changed == 1.0 at each cycle boundary -----
void test_rtc2_loop_fraction_one_at_boundary() {
  Rig r;
  r.ts->setCycleInterval(1.0);
  r.ts->setStartTime(0.0);
  r.ts->setLoop(true);

  r.ctx.tick(0.0); // activation: fraction 0 at startTime
  check(deq(r.ts->getFraction_changed(), 0.0), "rtc2: f=0 at startTime");

  // Tick exactly on each cycle boundary (now > startTime, f==0) -> 1.0.
  r.ctx.tick(1.0);
  check(deq(r.ts->getFraction_changed(), 1.0),
        "rtc2: f=1.0 at cycle-1 boundary (t=1)");

  r.ctx.tick(2.0);
  check(deq(r.ts->getFraction_changed(), 1.0),
        "rtc2: f=1.0 at cycle-2 boundary (t=2)");

  r.ctx.tick(3.0);
  check(deq(r.ts->getFraction_changed(), 1.0),
        "rtc2: f=1.0 at cycle-3 boundary (t=3)");

  // Mid-cycle still wraps normally.
  r.ctx.tick(3.5);
  check(deq(r.ts->getFraction_changed(), 0.5),
        "rtc2: f=0.5 mid-cycle (t=3.5)");
}

// --- RTC-3: cycleTime == startTime + cycleIndex*cycleInterval (not now) ------
void test_rtc3_cycletime_is_cycle_start() {
  Rig r;
  r.ts->setCycleInterval(2.0);
  r.ts->setStartTime(10.0);
  r.ts->setLoop(true);

  r.ctx.tick(10.0); // activation
  check(deq(r.ts->getCycleTime(), 10.0),
        "rtc3: cycleTime == startTime at activation");

  // Cross into cycle 1 at a tick t=13.3 (now != cycle start 12.0).
  r.ctx.tick(13.3);
  check(deq(r.ts->getCycleTime(), 12.0),
        "rtc3: cycleTime == startTime+1*cycleInterval (12), not now (13.3)");

  // Cross into cycle 2 at t=14.7 (cycle start 14.0).
  r.ctx.tick(14.7);
  check(deq(r.ts->getCycleTime(), 14.0),
        "rtc3: cycleTime == startTime+2*cycleInterval (14), not now (14.7)");
}

// --- RTC-4: disable mid-cycle emits final outputs THEN isActive=false --------
void test_rtc4_disable_emits_final_outputs() {
  Rig r;
  r.ts->setCycleInterval(10.0);
  r.ts->setStartTime(0.0);
  r.ts->setLoop(true);

  r.ctx.tick(0.0);
  check(active(r.ts) == true, "rtc4: active at start");

  r.ctx.tick(2.0);
  check(deq(r.ts->getFraction_changed(), 0.2), "rtc4: f=0.2 at t=2");
  check(deq(r.ts->getElapsedTime(), 2.0), "rtc4: elapsed=2 at t=2");

  // Disable at t=5: spec requires final time/fraction_changed/elapsedTime
  // (evaluated for THIS tick) before isActive=FALSE.
  r.ts->setEnabled(false);
  r.ctx.tick(5.0);
  check(active(r.ts) == false, "rtc4: inactive after disable");
  check(deq(r.ts->getFraction_changed(), 0.5),
        "rtc4: final fraction emitted (0.5) on disable");
  check(deq(r.ts->getTime(), 5.0), "rtc4: final time emitted (now=5) on disable");
  check(deq(r.ts->getElapsedTime(), 5.0),
        "rtc4: final elapsedTime emitted (5) on disable");
}

} // namespace

TEST_CASE("timesensor_rtc_test") {
  test_rtc1_zero_cycle_loop_no_crash();
  test_rtc2_loop_fraction_one_at_boundary();
  test_rtc3_cycletime_is_cycle_start();
  test_rtc4_disable_emits_final_outputs();

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    CHECK(false); return;
  }
  std::cout << "all TimeSensor RTC regression tests passed\n";
  return;
}

TEST_CASE("timesensor_late_resume_keeps_fraction_frozen") {
  Rig r;
  r.ts->setLoop(true);
  r.ts->setCycleInterval(10.0);
  r.ctx.tick(0.0);
  r.ts->setPauseTime(2.0);
  r.ctx.tick(3.0);
  r.ts->setResumeTime(5.0);
  r.ctx.tick(7.0);
  INFO("fraction after late resume = " << r.ts->getFraction_changed());
  CHECK(deq(r.ts->getFraction_changed(), 0.3));
}

TEST_CASE("timesensor_stop_uses_stop_time_for_final_output") {
  Rig r;
  r.ts->setLoop(true);
  r.ts->setCycleInterval(10.0);
  r.ctx.tick(0.0);
  r.ts->setStopTime(4.0);
  r.ctx.tick(6.0);
  INFO("final time = " << r.ts->getTime() << ", fraction = " << r.ts->getFraction_changed());
  CHECK(deq(r.ts->getTime(), 4.0));
  CHECK(deq(r.ts->getFraction_changed(), 0.4));
}

TEST_CASE("timesensor_past_stop_event_uses_receiving_time") {
  Rig r;
  r.ts->setLoop(true);
  r.ts->setCycleInterval(10.0);
  r.ctx.tick(0.0);
  r.ctx.tick(5.0);
  r.ctx.postEvent(r.ts.get(), "set_stopTime", std::any(SFTime{2.0}));
  r.ctx.process();
  r.ctx.tick(7.0);
  CHECK(deq(r.ts->getTime(), 5.0));
  CHECK(deq(r.ts->getFraction_changed(), 0.5));
}

TEST_CASE("timesensor_ignores_start_and_invalid_stop_while_active") {
  Rig r;
  r.ts->setStartTime(1.0);
  r.ts->setStopTime(-1.0);
  r.ts->setLoop(true);
  r.ctx.tick(1.0);
  r.ctx.postEvent(r.ts.get(), "set_startTime", std::any(SFTime{8.0}));
  r.ctx.postEvent(r.ts.get(), "set_stopTime", std::any(SFTime{0.0}));
  r.ctx.process();
  INFO("startTime = " << r.ts->getStartTime() << ", stopTime = " << r.ts->getStopTime());
  CHECK(deq(r.ts->getStartTime(), 1.0));
  CHECK(deq(r.ts->getStopTime(), -1.0));
  CHECK(r.ctx.writeField(r.ts.get(), "startTime", std::any(SFTime{9.0})) ==
        FieldWriteResult::Ok);
  CHECK(r.ctx.writeField(r.ts.get(), "stopTime", std::any(SFTime{0.0})) ==
        FieldWriteResult::Ok);
  CHECK(deq(r.ts->getStartTime(), 1.0));
  CHECK(deq(r.ts->getStopTime(), -1.0));
}

TEST_CASE("timesensor_accepts_negative_absolute_pause_time") {
  Rig r;
  r.ts->setStartTime(-5.0);
  r.ts->setLoop(true);
  r.ctx.tick(-5.0);
  r.ts->setResumeTime(-4.0);
  r.ts->setPauseTime(-3.0);
  r.ctx.tick(-2.0);
  INFO("isPaused = " << r.ts->X3DTimeDependentNode::getIsPaused());
  CHECK(r.ts->X3DTimeDependentNode::getIsPaused());
  r.ts->setResumeTime(-1.0);
  r.ctx.tick(-0.5);
  CHECK_FALSE(r.ts->X3DTimeDependentNode::getIsPaused());
}

TEST_CASE("timesensor_cycle_interval_change_keeps_fraction_continuous") {
  Rig r;
  r.ts->setLoop(true);
  r.ts->setCycleInterval(10.0);
  r.ctx.tick(0.0);
  r.ctx.tick(3.0);
  CHECK(deq(r.ts->getFraction_changed(), 0.3));
  r.ts->setCycleInterval(20.0);
  r.ctx.tick(4.0);
  INFO("fraction after slowing cycle at t=3 = " << r.ts->getFraction_changed());
  CHECK(deq(r.ts->getFraction_changed(), 0.35));
}

TEST_CASE("timesensor_shorter_interval_completes_current_cycle") {
  Rig r;
  r.ts->setLoop(true);
  r.ts->setCycleInterval(10.0);
  r.ctx.tick(0.0);
  r.ctx.tick(7.0);
  r.ts->setCycleInterval(5.0);
  r.ctx.tick(8.0);
  CHECK(deq(r.ts->getCycleTime(), 7.0));
  CHECK(deq(r.ts->getFraction_changed(), 0.2));
}

TEST_CASE("time_dependent_nodes_ignore_active_timing_inputs") {
  std::vector<std::shared_ptr<X3DTimeDependentNode>> nodes = {
      std::make_shared<AudioClip>(), std::make_shared<MovieTexture>(),
      std::make_shared<Gain>(), std::make_shared<BiquadFilter>(),
      std::make_shared<OscillatorSource>()};
  X3DExecutionContext ctx;
  auto media = std::make_shared<MediaTimeSystem>();
  auto sound = std::make_shared<SoundTimeSystem>();
  ctx.addSystem(media);
  ctx.addSystem(sound);
  for (const auto &node : nodes) {
    node->setStopTime(-1.0);
    media->attach(node.get(), ctx);
    sound->attach(node.get(), ctx);
  }
  ctx.tick(0.0);
  for (const auto &node : nodes) {
    REQUIRE(node->getIsActive());
    ctx.postEvent(node.get(), "set_startTime", std::any(SFTime{8.0}));
    ctx.postEvent(node.get(), "set_stopTime", std::any(SFTime{-2.0}));
  }
  ctx.process();
  for (const auto &node : nodes) {
    CHECK(deq(node->getStartTime(), 0.0));
    CHECK(deq(node->getStopTime(), -1.0));
  }
}
