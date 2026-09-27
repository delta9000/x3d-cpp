// HAnimMotion playback: ISO/IEC 19774-2 §5.2.3–5.2.4 and draft 2.1 §6.3–6.4.
#ifndef X3D_RUNTIME_HANIM_MOTION_SYSTEM_HPP
#define X3D_RUNTIME_HANIM_MOTION_SYSTEM_HPP

#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"
#include "FieldRead.hpp"
#include "x3d/nodes/HAnimHumanoid.hpp"
#include "x3d/nodes/HAnimJoint.hpp"
#include "x3d/nodes/HAnimMotion.hpp"

#include <algorithm>
#include <any>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace x3d::runtime::hanim {

using namespace x3d::core;
using namespace x3d::nodes;

class HAnimMotionSystem : public System {
  struct Quaternion {
    double w = 1, x = 0, y = 0, z = 0;
  };
  static Quaternion multiply(Quaternion a, Quaternion b) {
    return {a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z,
            a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
            a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
            a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w};
  }
  static Quaternion axis(char axis, double degrees) {
    // Published 19774-2 §5.2.3–5.2.4: source Euler degrees, X3D radians.
    const double half = degrees * (3.14159265358979323846 / 360.0);
    const double s = std::sin(half);
    return {std::cos(half), axis == 'X' ? s : 0,
            axis == 'Y' ? s : 0, axis == 'Z' ? s : 0};
  }
  static SFRotation rotation(Quaternion q) {
    if (q.w < 0) q = {-q.w, -q.x, -q.y, -q.z};
    const double v = std::sqrt(q.x*q.x + q.y*q.y + q.z*q.z);
    if (v < 1e-12) return {0, 0, 1, 0};
    return {static_cast<float>(q.x/v), static_cast<float>(q.y/v),
            static_cast<float>(q.z/v), static_cast<float>(2*std::atan2(v, q.w))};
  }
  static std::vector<std::string> tokens(std::string s) {
    std::replace(s.begin(), s.end(), ',', ' ');
    std::istringstream in(s);
    std::vector<std::string> result;
    for (std::string t; in >> t;) result.push_back(std::move(t));
    return result;
  }
  struct Group {
    HAnimJoint *joint = nullptr;
    std::vector<std::string> channels;
  };
  struct Playback {
    HAnimHumanoid *humanoid = nullptr;
    HAnimMotion *motion = nullptr;
    std::size_t motionSlot = 0;
    std::vector<Group> groups;
    std::string channelText, jointText;
    std::size_t width = 0;
    int index = 0;
    int count = 0;
    double last = 0, accumulated = 0, elapsed = 0;
    std::uint64_t tick = 0;
    bool active = false, stopped = false;
  };
  std::vector<Playback> playbacks_;
  std::unordered_map<HAnimHumanoid *, std::vector<std::shared_ptr<X3DNode>>> motionLists_;

  static void collectJoints(X3DNode *node, std::unordered_map<std::string, HAnimJoint *> &out,
                            std::unordered_set<X3DNode *> &seen) {
    if (!node || !seen.insert(node).second) return;
    if (auto *joint = dynamic_cast<HAnimJoint *>(node)) out.emplace(joint->getName(), joint);
    forEachChildNode(*node, [&](const FieldInfo &, const std::shared_ptr<X3DNode> &child) {
      collectJoints(child.get(), out, seen);
    });
  }
  static void parse(Playback &p) {
    if (p.channelText == p.motion->getChannels() && p.jointText == p.motion->getJoints()) return;
    p.channelText = p.motion->getChannels();
    p.jointText = p.motion->getJoints();
    p.groups.clear(); p.width = 0;
    auto names = tokens(p.jointText);
    auto words = tokens(p.channelText);
    std::unordered_map<std::string, HAnimJoint *> joints;
    std::unordered_set<X3DNode *> seen;
    collectJoints(p.humanoid, joints, seen);
    std::size_t at = 0;
    for (const auto &name : names) {
      if (at >= words.size()) break;
      int n = 0;
      try { n = std::stoi(words[at++]); } catch (...) { break; }
      if (n < 0 || static_cast<std::size_t>(n) > words.size() - at) break;
      Group group;
      if (name != "IGNORED") {
        auto it = joints.find(name);
        if (it != joints.end()) group.joint = it->second;
      }
      for (int i = 0; i < n; ++i) group.channels.push_back(words[at++]);
      p.width += group.channels.size();
      p.groups.push_back(std::move(group));
    }
    if (p.groups.size() != names.size() || at != words.size()) {
      p.groups.clear(); p.width = 0;
    }
  }
  static bool allowed(const Playback &p) {
    const auto &flags = p.humanoid->getMotionsEnabled();
    return p.motion->getEnabled() && (p.motionSlot >= flags.size() || flags[p.motionSlot]);
  }
  static std::pair<int, int> range(const Playback &p) {
    const int last = p.count - 1;
    const int start = std::clamp(static_cast<int>(p.motion->getStartFrame()), 0, last);
    const int end = p.motion->getEndFrame() == 0 ? last :
        std::clamp(static_cast<int>(p.motion->getEndFrame()), 0, last);
    return {std::min(start, end), std::max(start, end)};
  }
  static void apply(Playback &p, X3DExecutionContext &ctx) {
    if (!allowed(p) || p.count == 0) return;
    const auto &values = p.motion->getValues();
    const auto &enabled = p.motion->getChannelsEnabled();
    std::size_t at = static_cast<std::size_t>(p.index) * p.width, channel = 0;
    // Draft 2.1 §6.3: values are frame, group, channel; IGNORED still consumes values.
    for (const Group &g : p.groups) {
      SFVec3f pos = g.joint ? g.joint->getTranslation() : SFVec3f{0, 0, 0};
      Quaternion q;
      bool position = false, orient = false;
      for (const std::string &kind : g.channels) {
        const float value = values[at++];
        const bool use = channel >= enabled.size() || enabled[channel];
        ++channel;
        if (!g.joint || !use) continue;
        if (kind == "Xposition") { pos.x = value; position = true; }
        else if (kind == "Yposition") { pos.y = value; position = true; }
        else if (kind == "Zposition") { pos.z = value; position = true; }
        else if (kind == "Xrotation" || kind == "Yrotation" || kind == "Zrotation") {
          // Draft 2.1 §6.3: local-axis rotations compose in listed channel order.
          q = multiply(q, axis(kind[0], value)); orient = true;
        }
      }
      if (g.joint && position) ctx.postEvent(g.joint, "translation", std::any(pos));
      if (g.joint && orient) ctx.postEvent(g.joint, "rotation", std::any(rotation(q)));
    }
  }
  static void setIndex(Playback &p, int index, X3DExecutionContext &ctx) {
    p.index = index;
    ctx.postEvent(p.motion, "frameIndex", std::any(SFInt32{index}));
    apply(p, ctx);
  }
  void step(HAnimMotion *motion, int direction, X3DExecutionContext &ctx) {
    for (Playback &p : playbacks_) {
      if (p.motion != motion || !allowed(p)) continue;
      parse(p);
      const int count = p.width ? static_cast<int>(p.motion->getValues().size() / p.width) : 0;
      if (count != p.count) {
        p.count = count;
        ctx.postEvent(p.motion, "frameCount", std::any(SFInt32{count}));
      }
      if (!p.count) continue;
      const auto [lo, hi] = range(p);
      int next = p.index + direction;
      if (next > hi) next = lo;
      if (next < lo) next = hi;
      setIndex(p, next, ctx);
      p.stopped = false;
    }
  }

  // Bring the playbacks of `humanoid` in line with its current motions list
  // (an [in,out] field, §26.3.2): keep the state of motions still referenced,
  // add new ones, and drop the rest.
  void sync(HAnimHumanoid *humanoid, X3DExecutionContext &ctx) {
    const auto &motions = humanoid->getMotions();
    std::vector<Playback> kept;
    for (std::size_t i = 0; i < motions.size(); ++i) {
      auto *motion = dynamic_cast<HAnimMotion *>(motions[i].get());
      if (!motion) continue;
      auto it = std::find_if(playbacks_.begin(), playbacks_.end(), [&](const Playback &p) {
        return p.humanoid == humanoid && p.motion == motion;
      });
      if (it != playbacks_.end()) {
        Playback p = std::move(*it);
        p.motionSlot = i;
        playbacks_.erase(it);
        kept.push_back(std::move(p));
        continue;
      }
      Playback playback;
      playback.humanoid = humanoid;
      playback.motion = motion;
      playback.motionSlot = i;
      kept.push_back(std::move(playback));
      // Draft 2.1 §6.3: TRUE steps once and wraps; FALSE is inert.
      motion->setOnNextHandler([this, motion, &ctx](const SFBool &v) {
        if (v) step(motion, 1, ctx);
      });
      motion->setOnPreviousHandler([this, motion, &ctx](const SFBool &v) {
        if (v) step(motion, -1, ctx);
      });
    }
    for (const Playback &p : playbacks_) if (p.humanoid == humanoid) {
      p.motion->setOnNextHandler({}); p.motion->setOnPreviousHandler({});
    }
    std::erase_if(playbacks_, [humanoid](const Playback &p) { return p.humanoid == humanoid; });
    for (Playback &p : kept) playbacks_.push_back(std::move(p));
    motionLists_[humanoid] = motions;
  }

public:
  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    auto *humanoid = dynamic_cast<HAnimHumanoid *>(node);
    if (!humanoid) return;
    sync(humanoid, ctx);
  }
  void detach(X3DNode *node, X3DExecutionContext &) override {
    auto *humanoid = dynamic_cast<HAnimHumanoid *>(node);
    if (!humanoid) return;
    for (const Playback &p : playbacks_) if (p.humanoid == humanoid) {
      p.motion->setOnNextHandler({}); p.motion->setOnPreviousHandler({});
    }
    std::erase_if(playbacks_, [humanoid](const Playback &p) { return p.humanoid == humanoid; });
    motionLists_.erase(humanoid);
  }
  void update(double now, X3DExecutionContext &ctx) override {
    for (auto &[humanoid, list] : motionLists_)
      if (humanoid->getMotions() != list) sync(humanoid, ctx);
    for (Playback &p : playbacks_) {
      if (p.tick == ctx.tickGeneration()) continue; // tick's cascade may re-evaluate systems
      p.tick = ctx.tickGeneration();
      parse(p);
      const int count = p.width ? static_cast<int>(p.motion->getValues().size() / p.width) : 0;
      if (count != p.count) {
        p.count = count;
        ctx.postEvent(p.motion, "frameCount", std::any(SFInt32{count}));
      }
      if (!count) { p.active = false; continue; }
      // Draft 2.1 §6.3: frameIndex is bounded even when this motion is disabled.
      const int authored = std::clamp(static_cast<int>(p.motion->getFrameIndex()), 0, count - 1);
      if (p.motion->getFrameIndex() != authored)
        ctx.postEvent(p.motion, "frameIndex", std::any(SFInt32{authored}));
      if (!allowed(p)) { p.index = authored; p.active = false; continue; }
      const auto [lo, hi] = range(p);
      const bool selected = authored != p.index;
      if (selected) { p.index = authored; p.stopped = false; }
      p.index = std::clamp(p.index, lo, hi);
      if (!p.active) {
        p.active = true; p.stopped = false; p.last = now; p.accumulated = 0; p.elapsed = 0;
        ctx.postEvent(p.motion, "cycleTime", std::any(SFTime{now}));
        ctx.postEvent(p.motion, "elapsedTime", std::any(SFTime{0}));
        setIndex(p, p.index, ctx);
        continue;
      }
      if (selected || p.motion->getFrameIndex() != p.index) setIndex(p, p.index, ctx);
      const double dt = std::max(0.0, now - p.last);
      p.last = now;
      const int increment = p.motion->getFrameIncrement();
      const double duration = p.motion->getFrameDuration();
      if (increment == 0 || p.stopped || duration <= 0) continue;
      p.accumulated += dt;
      while (p.accumulated + 1e-9 >= duration) {
        p.accumulated = std::max(0.0, p.accumulated - duration);
        p.elapsed += duration;
        int next = p.index + increment;
        const bool wrapped = next > hi || next < lo;
        if (wrapped && !p.motion->getLoop()) {
          p.stopped = true;
          p.index = increment > 0 ? hi : lo;
          break;
        }
        if (wrapped) {
          const int span = hi - lo + 1;
          next = lo + ((next - lo) % span + span) % span;
          ctx.postEvent(p.motion, "cycleTime", std::any(SFTime{now - p.accumulated}));
        }
        setIndex(p, next, ctx);
      }
      ctx.postEvent(p.motion, "elapsedTime", std::any(SFTime{p.elapsed}));
    }
  }
};

} // namespace x3d::runtime::hanim

#endif
