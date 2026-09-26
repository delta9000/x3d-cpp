#include "doctest/doctest.h"
// media_time_test.cpp — TDN-5: AudioClip and MovieTexture run the
// X3DTimeDependentNode lifecycle (ISO/IEC 19775-1 §8.2.4, §16.4.2, §18.4.6).

#include "MediaTimeSystem.hpp"
#include "X3DExecutionContext.hpp"

#include "x3d/nodes/AudioClip.hpp"
#include "x3d/nodes/MovieTexture.hpp"

#include <cmath>
#include <iostream>
#include <memory>
#include <string>

using namespace x3d;
using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;

namespace {
int failures = 0;
void check(bool cond, const std::string &what) {
  if (!cond) { std::cerr << "FAIL: " << what << "\n"; ++failures; }
  else        { std::cout << "ok: " << what << "\n"; }
}
bool deq(double a, double b) { return std::fabs(a - b) < 1e-6; }

template <class N> struct Rig {
  std::shared_ptr<N> node = std::make_shared<N>();
  X3DExecutionContext ctx;
  std::shared_ptr<MediaTimeSystem> sys = std::make_shared<MediaTimeSystem>();
  Rig() { ctx.addSystem(sys); sys->attach(node.get(), ctx); }
  bool active() const { return node->X3DTimeDependentNode::getIsActive(); }
  bool paused() const { return node->X3DTimeDependentNode::getIsPaused(); }
  double elapsed() const { return node->X3DTimeDependentNode::getElapsedTime(); }
};
} // namespace

TEST_CASE("media_time_test") {
  { // AudioClip, known duration 4 s at pitch 2: one pass lasts 2 s.
    Rig<AudioClip> r;
    r.node->emitDuration_changed(4.0);
    r.node->setPitch(2.0f);
    r.node->setStartTime(1.0);
    r.ctx.tick(0.5);
    check(!r.active(), "audio: idle before startTime");
    r.ctx.tick(1.0);
    check(r.active(), "audio: isActive TRUE at startTime");
    r.ctx.tick(2.0);
    check(r.active() && deq(r.elapsed(), 1.0), "audio: elapsedTime counts playback");
    r.node->setPauseTime(2.5);  // 1.5 s of the 2 s pass played by the pause
    r.ctx.tick(2.5);
    check(r.paused(), "audio: pauseTime pauses (isPaused TRUE)");
    r.node->setResumeTime(3.0);
    r.ctx.tick(3.0);
    check(!r.paused(), "audio: resumeTime resumes");
    r.ctx.tick(3.4);  // 0.5 s left after resuming at 3.0; without the pause it ended at 3.0
    check(r.active(), "audio: the paused span does not count toward the pass");
    r.ctx.tick(3.6);
    check(!r.active(), "audio: a non-looping clip stops after duration / pitch");
  }
  { // Looping clip keeps playing past one pass.
    Rig<AudioClip> r;
    r.node->emitDuration_changed(1.0);
    r.node->setLoop(true);
    r.ctx.tick(0.0);
    r.ctx.tick(3.5);
    check(r.active(), "audio: loop TRUE keeps the clip active");
  }
  { // Unknown duration (-1 before load): plays until stopTime.
    Rig<AudioClip> r;
    r.ctx.tick(0.0);
    r.ctx.tick(100.0);
    check(r.active(), "audio: unknown duration plays until stopTime");
    r.node->setStopTime(150.0);
    r.ctx.tick(151.0);
    check(!r.active(), "audio: stopTime stops it");
  }
  { // MovieTexture: duration / speed, disabled nodes never start.
    Rig<MovieTexture> r;
    r.node->emitDuration_changed(6.0);
    r.node->setSpeed(3.0f);
    r.ctx.tick(0.0);
    check(r.active(), "movie: isActive TRUE at startTime");
    r.ctx.tick(2.1);
    check(!r.active(), "movie: a pass lasts duration / speed");
    Rig<MovieTexture> off;
    off.node->setEnabled(false);
    off.ctx.tick(0.0);
    check(!off.active(), "movie: enabled FALSE never activates");
  }
  CHECK(failures == 0);
}
