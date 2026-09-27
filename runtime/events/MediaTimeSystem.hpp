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
#include <unordered_map>

namespace x3d::runtime {

using namespace x3d::core;

// §18.4.2: the consumer reports duration after loading media; the SDK does not open it.
inline void reportMovieDuration(X3DExecutionContext &ctx, x3d::nodes::MovieTexture &movie,
                                SFTime seconds) {
  ctx.postEvent(&movie, "duration_changed", std::any(seconds));
}

class MediaTimeSystem : public X3DTimeDependentSystem {
public:
  void attach(x3d::nodes::X3DNode *node, X3DExecutionContext &ctx) override {
    auto *media = dynamic_cast<x3d::nodes::X3DTimeDependentNode *>(node);
    if (!media || (!dynamic_cast<x3d::nodes::AudioClip *>(node) &&
                   !dynamic_cast<x3d::nodes::MovieTexture *>(node))) return;
    if (filterContext_ != &ctx) {
      // §16.4.2 / §18.4.2: active pitch and speed inputs are ignored, including ROUTEs.
      ctx.addInputFilter([](const FieldAddress &a, const std::any &) {
        if (a.field == "pitch")
          if (auto *clip = dynamic_cast<x3d::nodes::AudioClip *>(a.node))
            return !clip->X3DTimeDependentNode::getIsActive();
        if (a.field == "speed")
          if (auto *movie = dynamic_cast<x3d::nodes::MovieTexture *>(a.node))
            return !movie->X3DTimeDependentNode::getIsActive();
        return true;
      });
      filterContext_ = &ctx;
    }
    rates_[media] = authoredRate(media);
    X3DTimeDependentSystem::attach(node, ctx);
  }

  void update(double now, X3DExecutionContext &ctx) override {
    for (auto &[node, rate] : rates_)
      if (!node->getIsActive()) rate = authoredRate(node);
    X3DTimeDependentSystem::update(now, ctx);
  }

  double playbackRate(const x3d::nodes::X3DTimeDependentNode *node) const {
    auto it = rates_.find(node);
    return it == rates_.end() ? authoredRate(node) : it->second;
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
    double duration = -1.0;
    if (auto *a = dynamic_cast<x3d::nodes::AudioClip *>(node)) {
      duration = a->getDuration_changed();
    } else if (auto *m = dynamic_cast<x3d::nodes::MovieTexture *>(node)) {
      duration = m->getDuration_changed();
    }
    const double rate = playbackRate(node);
    // Unknown duration (or a stopped / reversed rate): play until stopTime.
    if (!(duration > 0.0) || !(std::fabs(rate) > 0.0))
      return std::numeric_limits<double>::infinity();
    return duration / std::fabs(rate);
  }

private:
  static double authoredRate(const x3d::nodes::X3DTimeDependentNode *node) {
    if (auto *a = dynamic_cast<const x3d::nodes::AudioClip *>(node)) return a->getPitch();
    if (auto *m = dynamic_cast<const x3d::nodes::MovieTexture *>(node)) return m->getSpeed();
    return 1.0;
  }

  X3DExecutionContext *filterContext_ = nullptr;
  std::unordered_map<const x3d::nodes::X3DTimeDependentNode *, double> rates_;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_MEDIA_TIME_SYSTEM_HPP
