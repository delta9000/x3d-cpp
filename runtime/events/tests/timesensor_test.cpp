#include "doctest/doctest.h"
// timesensor_test.cpp
// Clock-edge-case tests for the full TimeSensor / X3DTimeDependentNode
// lifecycle (TimeSensorSystem over X3DTimeDependentSystem). Each test drives a
// clock via X3DExecutionContext::tick(now) and reads the emitted outputs back
// off the node getters (the cascade applies each outputOnly emit thunk during
// the tick, so
// getIsActive/getFraction_changed/getTime/getCycleTime/getElapsedTime/getIsPaused
// reflect the value posted that tick).
//
// Covered edges:
//   - enabled=false: no activation, no outputs.
//   - start gating: idle until startTime is crossed.
//   - single-shot (loop=false): completion emits fraction 1, then deactivates.
//   - looping wrap: fraction wraps; cycleTime pulses once per cycle boundary.
//   - stopTime: a stopTime in (startTime, now] deactivates early.
//   - pause/resume: elapsedTime/fraction exclude the paused span; isPaused
//   edges.
//   - cycleInterval <= 0 guard: non-looping sensor deactivates after one tick.
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "TimeSensorSystem.hpp"
#include "X3DExecutionContext.hpp"

#include "x3d/nodes/TimeSensor.hpp"

#include <cmath>
#include <iostream>
#include <memory>
#include <string>

using namespace x3d;
using namespace x3d::runtime;
using namespace x3d::core;
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

// isActive/isPaused are inherited from BOTH X3DSensorNode and
// X3DTimeDependentNode on TimeSensor; the runtime lifecycle (and the reflected
// emit thunk) drive the X3DTimeDependentNode copy, so read it explicitly.
bool active(const std::shared_ptr<TimeSensor> &ts) {
  return ts->X3DTimeDependentNode::getIsActive();
}
bool paused(const std::shared_ptr<TimeSensor> &ts) {
  return ts->X3DTimeDependentNode::getIsPaused();
}

// Build a context wiring a TimeSensorSystem over one sensor.
struct Rig {
  std::shared_ptr<TimeSensor> ts = std::make_shared<TimeSensor>();
  X3DExecutionContext ctx;
  std::shared_ptr<TimeSensorSystem> sys = std::make_shared<TimeSensorSystem>();

  Rig() {
    ctx.addSystem(sys);
    sys->attach(ts.get(), ctx);
  }
};

// --- enabled=false: nothing happens -----------------------------------------
void test_disabled_emits_nothing() {
  Rig r;
  r.ts->setEnabled(false);
  r.ts->setCycleInterval(1.0);
  r.ts->setStartTime(0.0);
  r.ts->setLoop(true);

  r.ctx.tick(0.5);
  check(active(r.ts) == false, "disabled: never becomes active");
  check(deq(r.ts->getFraction_changed(), 0.0),
        "disabled: fraction stays at default 0");

  r.ctx.tick(2.0);
  check(active(r.ts) == false, "disabled: still inactive after ticks");
}

// --- start gating: idle until startTime -------------------------------------
void test_start_gating() {
  Rig r;
  r.ts->setCycleInterval(2.0);
  r.ts->setStartTime(10.0);
  r.ts->setLoop(true);

  r.ctx.tick(5.0);
  check(active(r.ts) == false, "before startTime: inactive");

  r.ctx.tick(10.0);
  check(active(r.ts) == true, "at startTime: active");
  check(deq(r.ts->getFraction_changed(), 0.0), "at startTime: fraction 0");
  check(deq(r.ts->getCycleTime(), 10.0),
        "at startTime: cycleTime pulse == now");

  r.ctx.tick(11.0);
  check(deq(r.ts->getFraction_changed(), 0.5),
        "1s into 2s cycle: fraction 0.5");
  check(deq(r.ts->getTime(), 11.0), "time output == now");
}

// --- single-shot (loop=false): completes and deactivates --------------------
void test_single_shot_completion() {
  Rig r;
  r.ts->setCycleInterval(1.0);
  r.ts->setStartTime(0.0);
  r.ts->setLoop(false);

  r.ctx.tick(0.0);
  check(active(r.ts) == true, "single-shot: active at start");

  r.ctx.tick(0.25);
  check(deq(r.ts->getFraction_changed(), 0.25), "single-shot: f=0.25");

  r.ctx.tick(0.75);
  check(deq(r.ts->getFraction_changed(), 0.75), "single-shot: f=0.75");
  check(active(r.ts) == true, "single-shot: still active mid-cycle");

  // At/after cycleInterval the sensor emits a final fraction of 1 and stops.
  r.ctx.tick(1.0);
  check(deq(r.ts->getFraction_changed(), 1.0),
        "single-shot: final fraction == 1");
  check(active(r.ts) == false, "single-shot: deactivates at end");

  // Further ticks do nothing (it stays inactive; startTime not re-crossed).
  r.ctx.tick(2.0);
  check(active(r.ts) == false, "single-shot: stays inactive after end");
}

// --- looping wrap + cycleTime pulses ----------------------------------------
void test_looping_wrap_and_cycletime() {
  Rig r;
  r.ts->setCycleInterval(1.0);
  r.ts->setStartTime(0.0);
  r.ts->setLoop(true);

  r.ctx.tick(0.0);
  check(active(r.ts) == true, "loop: active at start");
  check(deq(r.ts->getCycleTime(), 0.0), "loop: cycleTime pulse at start (t=0)");

  r.ctx.tick(0.5);
  check(deq(r.ts->getFraction_changed(), 0.5), "loop: f=0.5 in cycle 0");
  check(deq(r.ts->getCycleTime(), 0.0),
        "loop: no new cycleTime pulse mid-cycle (still 0)");

  // Cross into cycle 1 at t=1.5 -> fraction wraps to 0.5, cycleTime pulses.
  // cycleTime == time at the beginning of the current cycle ==
  // startTime + 1*cycleInterval == 1.0 (NOT the tick's now=1.5). (RTC-3)
  r.ctx.tick(1.5);
  check(deq(r.ts->getFraction_changed(), 0.5),
        "loop: fraction wraps to 0.5 in cycle 1");
  check(deq(r.ts->getCycleTime(), 1.0),
        "loop: cycleTime == cycle-1 start (1.0), not the tick now");

  // Stay in cycle 1: cycleTime must NOT pulse again.
  r.ctx.tick(1.75);
  check(deq(r.ts->getFraction_changed(), 0.75), "loop: f=0.75 in cycle 1");
  check(deq(r.ts->getCycleTime(), 1.0),
        "loop: cycleTime unchanged within cycle 1");

  // Cross into cycle 2. cycleTime == startTime + 2*cycleInterval == 2.0.
  r.ctx.tick(2.25);
  check(deq(r.ts->getFraction_changed(), 0.25),
        "loop: fraction wraps to 0.25 in cycle 2");
  check(deq(r.ts->getCycleTime(), 2.0),
        "loop: cycleTime == cycle-2 start (2.0), not the tick now");
}

// --- stopTime early stop ----------------------------------------------------
void test_stoptime_deactivates() {
  Rig r;
  r.ts->setCycleInterval(10.0);
  r.ts->setStartTime(0.0);
  r.ts->setStopTime(3.0);
  r.ts->setLoop(true);

  r.ctx.tick(0.0);
  check(active(r.ts) == true, "stopTime: active at start");

  r.ctx.tick(2.0);
  check(active(r.ts) == true, "stopTime: active before stopTime");
  check(deq(r.ts->getFraction_changed(), 0.2), "stopTime: f=0.2 at t=2");

  // stopTime (3) <= now (3): deactivate.
  r.ctx.tick(3.0);
  check(active(r.ts) == false, "stopTime: deactivates at stopTime");
}

// --- pause / resume: elapsed/fraction exclude the paused span ---------------
void test_pause_resume() {
  Rig r;
  r.ts->setCycleInterval(100.0); // long, non-wrapping window
  r.ts->setStartTime(0.0);
  r.ts->setLoop(true);

  r.ctx.tick(0.0);
  check(active(r.ts) == true, "pause: active at start");

  r.ctx.tick(2.0);
  check(deq(r.ts->getElapsedTime(), 2.0), "pause: elapsed=2 before pause");
  check(paused(r.ts) == false, "pause: not paused yet");

  // Pause at t=3 (pauseTime in (resumeTime=0, now]).
  r.ts->setPauseTime(3.0);
  r.ctx.tick(3.0);
  check(paused(r.ts) == true, "pause: isPaused true at pauseTime");

  // While paused, elapsed must not advance (held at the pre-pause value).
  r.ctx.tick(5.0);
  check(paused(r.ts) == true, "pause: still paused at t=5");
  check(deq(r.ts->getElapsedTime(), 2.0),
        "pause: elapsed frozen at 2 during pause");

  // Resume at t=6: paused span was [3,6] = 3s, which is excluded.
  r.ts->setResumeTime(6.0);
  r.ctx.tick(6.0);
  check(paused(r.ts) == false, "resume: isPaused false at resumeTime");

  // At t=8: wall time since start = 8, minus 3s paused = 5s elapsed.
  r.ctx.tick(8.0);
  check(deq(r.ts->getElapsedTime(), 5.0),
        "resume: elapsed excludes paused span (5s)");
  check(deq(r.ts->getFraction_changed(), 5.0 / 100.0),
        "resume: fraction tracks unpaused elapsed");
}

// --- cycleInterval <= 0 guard (non-looping deactivates after one tick) ------
void test_zero_cycle_interval_guard() {
  Rig r;
  r.ts->setCycleInterval(0.0); // spec-min is 0; guard treats <=0 as immediate
  r.ts->setStartTime(0.0);
  r.ts->setLoop(false);

  // Activates, emits final fraction, then deactivates in the same tick.
  r.ctx.tick(0.0);
  check(active(r.ts) == false,
        "zero-cycle: non-looping deactivates after one tick");
  check(deq(r.ts->getFraction_changed(), 1.0),
        "zero-cycle: emits final fraction 1");
}

// --- TDN-4: set_startTime ignored while active ------------------------------
void test_set_starttime_ignored_while_active() {
  Rig r;
  r.ts->setCycleInterval(10.0);
  r.ts->setStartTime(0.0); // stopTime stays at its default 0
  r.ts->setLoop(true);

  r.ctx.tick(0.0);
  check(active(r.ts) == true, "tdn4: active at start");
  r.ctx.tick(5.0);
  check(active(r.ts) == true, "tdn4: active mid-cycle");

  // A ROUTE delivers set_startTime to an earlier time while the node is active.
  // Per 8.2.4.3 this MUST be ignored. With a live per-tick re-read it flips the
  // deactivation guard (stopTime 0 > startTime -5) and spuriously deactivates.
  r.ts->setStartTime(-5.0);
  r.ctx.tick(6.0);
  check(active(r.ts) == true,
        "tdn4: set_startTime while active is ignored (no spurious deactivate)");
}

// --- TDN-3/TDN-7: loop=FALSE finishes the current cycle; final time at boundary
void test_loop_false_finishes_current_cycle() {
  Rig r;
  r.ts->setCycleInterval(1.0);
  r.ts->setStartTime(0.0);
  r.ts->setLoop(true);

  r.ctx.tick(0.0);
  check(active(r.ts) == true, "tdn3: active at start");
  r.ctx.tick(3.5);
  check(active(r.ts) == true, "tdn3: active after 3.5 cycles");

  // loop -> FALSE mid cycle 3: must run to the END of the current cycle (4.0),
  // not deactivate on the next tick.
  r.ts->setLoop(false);
  r.ctx.tick(3.7);
  check(active(r.ts) == true,
        "tdn3: still active mid current cycle after loop=FALSE");

  // Tick past the boundary (4.0): deactivates with a final fraction of 1.
  // (TDN-7 — final time == the exact cycle boundary rather than the overshooting
  // tick now — is deferred: it is entangled with the post-completion
  // re-activation behavior + the per-tick do-while re-evaluation (SCR-001), which
  // re-fire completion within the same tick. Fix those first.)
  r.ctx.tick(4.3);
  check(active(r.ts) == false,
        "tdn3: deactivates at end of current cycle, not immediately");
  check(deq(r.ts->getFraction_changed(), 1.0), "tdn3: final fraction == 1");
}

// --- TDN-6: resume requires resumeTime STRICTLY > pauseTime (§8.2.4.4) -------
void test_resume_requires_strict_greater() {
  Rig r;
  r.ts->setCycleInterval(100.0);
  r.ts->setStartTime(0.0);
  r.ts->setLoop(true);

  r.ctx.tick(0.0);
  check(active(r.ts) == true, "tdn6: active at start");
  r.ts->setPauseTime(3.0);
  r.ctx.tick(3.0);
  check(paused(r.ts) == true, "tdn6: paused at pauseTime");

  // resumeTime == pauseTime must NOT resume (spec requires resumeTime >
  // pauseTime, strict — symmetric with the strict pause guard).
  r.ts->setResumeTime(3.0);
  r.ctx.tick(4.0);
  check(paused(r.ts) == true,
        "tdn6: resumeTime == pauseTime does not resume (strict >)");
}

// --- TDN-1/TDN-2: pauseTime_changed / resumeTime_changed fire at the pause and
//     resume transitions (§8.2.4.4). Routed to a sink node's SFTime fields.
void test_pause_resume_changed_events() {
  Rig r;
  r.ts->setCycleInterval(100.0);
  r.ts->setStartTime(0.0);
  r.ts->setLoop(true);

  auto sink = std::make_shared<TimeSensor>();
  r.ctx.addRoute(FieldAddress{r.ts.get(), "pauseTime_changed"},
                 FieldAddress{sink.get(), "startTime"});
  r.ctx.addRoute(FieldAddress{r.ts.get(), "resumeTime_changed"},
                 FieldAddress{sink.get(), "stopTime"});

  r.ctx.tick(0.0);
  check(active(r.ts) == true, "tdn1/2: active at start");

  r.ts->setPauseTime(3.0);
  r.ctx.tick(3.0);
  check(paused(r.ts) == true, "tdn1: paused at pauseTime");
  check(deq(sink->getStartTime(), 3.0),
        "tdn1: pauseTime_changed fired on pause (routed value == pauseTime)");

  r.ts->setResumeTime(6.0);
  r.ctx.tick(6.0);
  check(paused(r.ts) == false, "tdn2: resumed at resumeTime");
  check(deq(sink->getStopTime(), 6.0),
        "tdn2: resumeTime_changed fired on resume (routed value == resumeTime)");
}

// --- TDN-7 + re-activation guard: a completed single-shot stays inactive on
//     later ticks (no auto-restart), and its final time is the exact cycle
//     boundary, not the overshooting tick now.
void test_no_reactivation_after_completion() {
  Rig r;
  r.ts->setCycleInterval(1.0);
  r.ts->setStartTime(0.0);
  r.ts->setLoop(false);

  r.ctx.tick(0.0);
  check(active(r.ts) == true, "reactivation: active at start");

  // Tick past the boundary (1.0): completes. Final time == boundary 1.0.
  r.ctx.tick(1.3);
  check(active(r.ts) == false, "reactivation: inactive after completion");
  check(deq(r.ts->getTime(), 1.0),
        "tdn7: final time == cycle boundary 1.0, not tick now 1.3");

  // A later tick (startTime unchanged) must NOT re-activate / re-emit.
  r.ctx.tick(5.0);
  check(active(r.ts) == false, "reactivation: stays inactive on later ticks");
  check(deq(r.ts->getTime(), 1.0),
        "reactivation: time frozen at completion (no spurious re-activation)");

  // Setting a NEW startTime DOES restart it.
  r.ts->setStartTime(10.0);
  r.ctx.tick(10.0);
  check(active(r.ts) == true, "reactivation: a new startTime re-activates");
}

} // namespace

TEST_CASE("timesensor_test") {
  test_disabled_emits_nothing();
  test_start_gating();
  test_single_shot_completion();
  test_looping_wrap_and_cycletime();
  test_stoptime_deactivates();
  test_pause_resume();
  test_zero_cycle_interval_guard();
  test_set_starttime_ignored_while_active();
  test_loop_false_finishes_current_cycle();
  test_resume_requires_strict_greater();
  test_pause_resume_changed_events();
  test_no_reactivation_after_completion();

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    CHECK(false); return;
  }
  std::cout << "all TimeSensor lifecycle tests passed\n";
  return;
}
