// MediaTimeSystem.hpp — the X3DTimeDependentNode lifecycle for media nodes
// (ISO/IEC 19775-1 §8.2.4, §16.4.2 AudioClip, §18.4.6 MovieTexture).
//
// AudioClip and MovieTexture are time-dependent: startTime / stopTime /
// pauseTime / resumeTime / loop drive isActive, isPaused and elapsedTime exactly
// as for a TimeSensor. This System reuses the shared clock machine
// (X3DTimeDependentSystem) with the media nodes' own reads:
//   * enabled   — X3DSoundSourceNode.enabled;
//   * loop      — the node's loop field;
//   * cycle     — one pass of the media: duration_changed divided by the
//                 playback rate (AudioClip.pitch, MovieTexture.speed). Until the
//                 duration is known (duration_changed is -1 before the media is
//                 loaded) the node plays until its stopTime.
// Decoding and playback are separate (the AudioBackend / MovieDecoder seams);
// this System only owns the timing outputs (TDN-5).
#ifndef X3D_RUNTIME_MEDIA_TIME_SYSTEM_HPP
#define X3D_RUNTIME_MEDIA_TIME_SYSTEM_HPP

#include "X3DExecutionContext.hpp"
#include "X3DTimeDependentSystem.hpp"

#include "x3d/nodes/AudioClip.hpp"
#include "x3d/nodes/MovieTexture.hpp"

#include <cmath>
#include <limits>

namespace x3d::runtime {

using namespace x3d::core;

class MediaTimeSystem : public X3DTimeDependentSystem {
public:
  void attach(x3d::nodes::X3DNode *node, X3DExecutionContext &ctx) override {
    if (dynamic_cast<x3d::nodes::AudioClip *>(node) ||
        dynamic_cast<x3d::nodes::MovieTexture *>(node))
      X3DTimeDependentSystem::attach(node, ctx);
  }

protected:
  bool readEnabled(x3d::nodes::X3DTimeDependentNode *node) const override {
    if (auto *a = dynamic_cast<x3d::nodes::AudioClip *>(node)) return a->getEnabled();
    if (auto *m = dynamic_cast<x3d::nodes::MovieTexture *>(node)) return m->getEnabled();
    return true;
  }
  bool readLoop(x3d::nodes::X3DTimeDependentNode *node) const override {
    if (auto *a = dynamic_cast<x3d::nodes::AudioClip *>(node)) return a->getLoop();
    if (auto *m = dynamic_cast<x3d::nodes::MovieTexture *>(node)) return m->getLoop();
    return false;
  }
  double readCycleInterval(x3d::nodes::X3DTimeDependentNode *node) const override {
    double duration = -1.0, rate = 1.0;
    if (auto *a = dynamic_cast<x3d::nodes::AudioClip *>(node)) {
      duration = a->getDuration_changed();
      rate = a->getPitch();
    } else if (auto *m = dynamic_cast<x3d::nodes::MovieTexture *>(node)) {
      duration = m->getDuration_changed();
      rate = m->getSpeed();
    }
    // Unknown duration (or a stopped / reversed rate): play until stopTime.
    if (!(duration > 0.0) || !(std::fabs(rate) > 0.0))
      return std::numeric_limits<double>::infinity();
    return duration / std::fabs(rate);
  }
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_MEDIA_TIME_SYSTEM_HPP
