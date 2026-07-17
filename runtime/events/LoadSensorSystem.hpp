// LoadSensorSystem.hpp
// §9.4.3 Networking — LoadSensor as a live X3DNetworkSensorNode. A time-driven
// System observes each LoadSensor's watched X3DUrlObject children per tick,
// resolving their load state through the injected extract::AssetResolver seam
// (default: a SEC-3-confined local-file resolver), and emits isActive / isLoaded
// / loadTime / progress per the spec. Per-sensor state lives here in a map, never
// on the node (the node model stays pure data). Closes findings NSN-1..7, NSN-9.
//
// Design: docs/superpowers/specs/2026-07-17-loadsensor-design.md.
#ifndef X3D_RUNTIME_LOAD_SENSOR_SYSTEM_HPP
#define X3D_RUNTIME_LOAD_SENSOR_SYSTEM_HPP

#include "X3DSystem.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DScene.hpp"
#include "AssetResolver.hpp"
#include "io/file/FileResolver.hpp"
#include "x3d/nodes/LoadSensor.hpp"
#include "x3d/nodes/X3DUrlObject.hpp"
#include "x3d/nodes/X3DInterfaceRegistry.hpp"

#include <any>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace x3d::runtime {

using namespace x3d::core;

namespace detail_loadsensor {
// Embedded schemes carry their bytes in the URL string itself, so they resolve
// Ready without any resolver call.
inline bool isEmbeddedScheme(const std::string &u) {
  auto starts = [&](const char *p) { return u.rfind(p, 0) == 0; };
  return starts("ecmascript:") || starts("javascript:") || starts("data:");
}
// AssetKind for a watched child (advisory to current backends; picks the
// intent branch per the two AssetResolver invocation contracts).
inline extract::AssetKind kindForNode(X3DNode *c) {
  const std::string t = c->nodeTypeName();
  if (t == "MovieTexture" || t == "AudioClip")
    return extract::AssetKind::Movie; // streamed-media first-frame rule
  if (t == "Inline")
    return extract::AssetKind::Inline;
  if (t.find("Texture") != std::string::npos)
    return extract::AssetKind::Texture;
  return extract::AssetKind::Inline; // generic bytes
}
// Anchor default policy: an intra-scene "#Name" fragment is "loaded" iff a
// Viewpoint DEF'd `name` exists in the scene (the viewpoint-bind Anchor case).
inline bool isViewpointDef(const Scene &scene, const std::string &name) {
  auto n = scene.resolve(name);
  return n && x3d::nodes::X3DInterfaceRegistry::nodeImplements(
                  n.get(), x3d::nodes::InterfaceId::X3DViewpointNode);
}
} // namespace detail_loadsensor

// Per-child resolution status the system tracks across ticks.
enum class ChildStatus { NotStarted, Loading, Ready, Failed };

// A headed embedder's per-child ruling. `watch=false` drops the child from the
// watch set entirely; `vacuousReady=true` treats it as instantly loaded without
// any resolver call (e.g. an Anchor whose target is an in-scene DEF). `kind`
// selects the AssetResolver intent branch.
struct ChildLoadPlan {
  bool watch = true;
  bool vacuousReady = false;
  extract::AssetKind kind = extract::AssetKind::Inline;
};
using ChildLoadPolicy = std::function<ChildLoadPlan(X3DNode *child, const Scene &)>;

class LoadSensorSystem : public System {
public:
  explicit LoadSensorSystem(extract::AssetResolver resolver = nullptr,
                            std::string baseUrl = "")
      : resolver_(resolver ? std::move(resolver)
                           : io::file::makeFileResolver(baseUrl)),
        baseUrl_(std::move(baseUrl)) {}

  void setScene(const Scene *s) { scene_ = s; }
  void setChildLoadPolicy(ChildLoadPolicy p) { policy_ = std::move(p); }
  void setSensorHook(std::function<void(X3DNode *, bool, double)> h) {
    sensorHook_ = std::move(h);
  }
  void setChildStateHook(std::function<void(X3DNode *, X3DNode *, ChildStatus)> h) {
    childHook_ = std::move(h);
  }

  void attach(X3DNode *node, X3DExecutionContext &) override {
    if (auto *ls = dynamic_cast<x3d::nodes::LoadSensor *>(node))
      state_.emplace(ls, SensorState{});
  }

  void update(double now, X3DExecutionContext &ctx) override; // Tasks 3-6 fill in.

private:
  struct ChildState {
    ChildStatus status = ChildStatus::NotStarted;
    std::size_t candidate = 0;       // next MFString index to try
    MFString lastUrl;                // NSN-7 url snapshot
    bool lastLoad = true;            // NSN-7 load snapshot
    bool preseeded = false;          // parse-time expanded Inline (already Ready)
  };
  struct SensorState {
    std::unordered_map<X3DNode *, ChildState> children;
    bool active = false;
    bool terminal = false;           // burst emitted; idle until an NSN-7 reset
    bool everEvaluated = false;      // R7 first-evaluation
    bool enabled = true;
    double activatedAt = 0.0;
    float lastProgress = -1.0f;
    std::uint64_t lastGen = 0;       // tick this sensor was last processed
  };

  template <typename T>
  void emit(X3DExecutionContext &ctx, X3DNode *n, const char *field, T v) {
    ctx.postEvent(n, field, std::any(v));
  }

  // Transition a child's status, firing the childHook on change only.
  void setChildStatus(ChildState &cs, X3DNode *child,
                      x3d::nodes::LoadSensor *sensor, ChildStatus s) {
    if (cs.status == s)
      return;
    cs.status = s;
    if (childHook_)
      childHook_(sensor, child, s);
  }

  extract::AssetResolver resolver_;
  std::string baseUrl_;
  const Scene *scene_ = nullptr;
  ChildLoadPolicy policy_;
  std::function<void(X3DNode *, bool, double)> sensorHook_;
  std::function<void(X3DNode *, X3DNode *, ChildStatus)> childHook_;
  std::unordered_map<x3d::nodes::LoadSensor *, SensorState> state_;
  std::unordered_set<std::string> readyMemo_; // ADR-0045: memo Ready only.
};

// Per-tick algorithm (design §2/§3): refresh the watch set, poll each still-
// loading child through the resolver (≤1 call per child per tick), aggregate,
// and emit the isActive/isLoaded/loadTime/progress edges. Per-sensor state lives
// in state_; the node stays pure data.
inline void LoadSensorSystem::update(double now, X3DExecutionContext &ctx) {
  // tick() re-invokes update() in a settle loop until the cascade quiesces, so
  // the per-tick work (resolver polls, state transitions, bursts) must run at
  // most once per tick. Gate on tickGeneration(); re-invocations within the same
  // tick emit nothing new, quiescing the loop (TimeSensorSystem idempotency
  // pattern — it recomputes from `now`; we cannot, since the resolver has side
  // effects and Failed advances a candidate).
  const std::uint64_t gen = ctx.tickGeneration();
  for (auto &entry : state_) {
    x3d::nodes::LoadSensor *ls = entry.first;
    SensorState &st = entry.second;
    if (st.lastGen == gen)
      continue;
    st.lastGen = gen;

    // 1. enabled gate: a disabled sensor deactivates and resets so a later
    //    re-enable starts a fresh monitoring cycle (design §3 last row).
    if (!ls->getEnabled()) {
      if (st.active) {
        emit(ctx, ls, "isActive", SFBool{false});
        if (sensorHook_)
          sensorHook_(ls, false, now);
      }
      st = SensorState{};
      st.lastGen = gen; // keep the gate closed for this tick's re-invocations
      continue;
    }

    bool restartTimeout = false;

    // 2. Refresh the watch set from children (unique by pointer — R5 USE dedup),
    //    seeding new entries and diffing url/load for NSN-7 resets.
    std::vector<X3DNode *> watched;
    std::unordered_set<X3DNode *> present;
    for (const auto &childAny : ls->getChildren()) {
      X3DNode *c = childAny.get();
      if (!c || present.count(c))
        continue;

      const bool pre = scene_ && scene_->expandedInlines.count(c) > 0;
      ChildLoadPlan plan;
      if (!pre) {
        if (!x3d::nodes::X3DInterfaceRegistry::nodeImplements(
                c, x3d::nodes::InterfaceId::X3DUrlObject))
          continue; // non-X3DUrlObject children are ignored per spec.
        if (policy_ && scene_)
          plan = policy_(c, *scene_);
        if (!plan.watch)
          continue;
      }

      present.insert(c);
      watched.push_back(c);

      auto [it, inserted] = st.children.try_emplace(c, ChildState{});
      ChildState &cs = it->second;
      auto *uo = dynamic_cast<x3d::nodes::X3DUrlObject *>(c);
      if (inserted) {
        cs.preseeded = pre;
        if (pre || plan.vacuousReady)
          setChildStatus(cs, c, ls, ChildStatus::Ready);
        if (!pre && uo) {
          cs.lastUrl = uo->getUrl();
          cs.lastLoad = uo->getLoad();
        }
        // A newly watched child that still needs loading reopens a terminal
        // sensor (membership growth is an NSN-7 reset) and restarts the window.
        if (cs.status != ChildStatus::Ready) {
          st.terminal = false;
          restartTimeout = true;
        }
      } else if (!pre && uo) {
        // NSN-7 poll-and-diff on the inputOutput url/load fields.
        MFString curUrl = uo->getUrl();
        const bool curLoad = uo->getLoad();
        if (curUrl != cs.lastUrl) {
          setChildStatus(cs, c, ls, ChildStatus::Loading);
          cs.candidate = 0;
          cs.lastUrl = curUrl;
          restartTimeout = true;
          st.terminal = false;
        }
        if (curLoad != cs.lastLoad) { // either direction restarts the child
          setChildStatus(cs, c, ls, ChildStatus::NotStarted);
          cs.candidate = 0;
          cs.lastLoad = curLoad;
          st.terminal = false;
        }
        if (plan.vacuousReady)
          setChildStatus(cs, c, ls, ChildStatus::Ready);
      }
    }

    // Membership diff: drop state for children that left the watch set.
    for (auto it = st.children.begin(); it != st.children.end();) {
      if (!present.count(it->first))
        it = st.children.erase(it);
      else
        ++it;
    }

    // 3. Terminal sensors idle until an NSN-7 reset above clears the flag.
    if (st.terminal) {
      st.everEvaluated = true;
      continue;
    }

    // 4. Poll each still-loading child: at most one resolver call per child.
    for (X3DNode *c : watched) {
      ChildState &cs = st.children[c];
      if (cs.preseeded || cs.status == ChildStatus::Ready ||
          cs.status == ChildStatus::Failed)
        continue;
      auto *uo = dynamic_cast<x3d::nodes::X3DUrlObject *>(c);
      const bool load = uo ? static_cast<bool>(uo->getLoad()) : true;
      if (!load) {
        cs.status = ChildStatus::NotStarted; // load=FALSE holds the child idle
        continue;
      }
      MFString urls = uo ? uo->getUrl() : MFString{};
      if (urls.empty()) {
        setChildStatus(cs, c, ls, ChildStatus::Ready); // R3: empty url is vacuous
        continue;
      }
      cs.status = ChildStatus::Loading;
      const extract::AssetKind kind = detail_loadsensor::kindForNode(c);
      bool called = false;
      while (cs.candidate < urls.size()) {
        const std::string &u = urls[cs.candidate];
        // Anchor default policy: "#Name" is loaded iff a Viewpoint DEF exists;
        // otherwise it falls through to the resolver (which fails for a bare
        // fragment), yielding a Failed child.
        if (!u.empty() && u[0] == '#' && scene_ &&
            detail_loadsensor::isViewpointDef(*scene_, u.substr(1))) {
          setChildStatus(cs, c, ls, ChildStatus::Ready);
          break;
        }
        if (detail_loadsensor::isEmbeddedScheme(u) || readyMemo_.count(u)) {
          setChildStatus(cs, c, ls, ChildStatus::Ready);
          break;
        }
        if (called)
          break; // ≤1 resolver call/tick: remaining candidates wait for next tick
        const extract::AssetResult r = resolver_(u, kind);
        called = true;
        if (r.ready()) {
          readyMemo_.insert(u);
          setChildStatus(cs, c, ls, ChildStatus::Ready);
          break;
        }
        if (r.pending()) {
          cs.status = ChildStatus::Loading;
          break;
        }
        ++cs.candidate; // Failed: advance to the next candidate.
      }
      if (cs.candidate >= urls.size() && cs.status != ChildStatus::Ready)
        setChildStatus(cs, c, ls, ChildStatus::Failed); // all candidates failed
    }

    // 5. A child (re)entering Loading via NSN-7 restarts the timeout window.
    if (restartTimeout)
      st.activatedAt = now;

    // 6. Aggregate over the unique watch set and emit the transition edges.
    const std::size_t total = watched.size();
    std::size_t readyCount = 0, failedCount = 0, loadingCount = 0;
    for (X3DNode *c : watched) {
      switch (st.children[c].status) {
      case ChildStatus::Ready:
        ++readyCount;
        break;
      case ChildStatus::Failed:
        ++failedCount;
        break;
      case ChildStatus::Loading:
        ++loadingCount;
        break;
      case ChildStatus::NotStarted:
        break;
      }
    }

    auto successBurst = [&]() {
      emit(ctx, ls, "isLoaded", SFBool{true});
      emit(ctx, ls, "loadTime", SFTime{now});
      emit(ctx, ls, "progress", SFFloat{1.0f});
      if (st.active) {
        emit(ctx, ls, "isActive", SFBool{false});
        if (sensorHook_)
          sensorHook_(ls, false, now);
      }
      st.active = false;
      st.terminal = true;
      st.lastProgress = 1.0f;
    };
    auto failureBurst = [&]() {
      emit(ctx, ls, "isLoaded", SFBool{false}); // loadTime is NOT sent on failure
      if (st.active) {
        emit(ctx, ls, "isActive", SFBool{false});
        if (sensorHook_)
          sensorHook_(ls, false, now);
      }
      st.active = false;
      st.terminal = true;
    };

    if (total == 0) {
      if (!st.everEvaluated)
        successBurst(); // R6: empty watch set is a vacuous first-eval success
    } else if (failedCount > 0) {
      failureBurst();
    } else if (readyCount == total) {
      successBurst();
    } else if (loadingCount > 0) {
      const float progress =
          static_cast<float>(readyCount) / static_cast<float>(total);
      if (!st.active) {
        st.active = true;
        if (!restartTimeout)
          st.activatedAt = now;
        emit(ctx, ls, "isActive", SFBool{true});
        emit(ctx, ls, "progress", SFFloat{progress});
        st.lastProgress = progress;
        if (sensorHook_)
          sensorHook_(ls, true, now);
      } else if (progress != st.lastProgress) {
        emit(ctx, ls, "progress", SFFloat{progress});
        st.lastProgress = progress;
      }
    }
    // else: nothing loading, none failed, not all ready (e.g. load=FALSE
    // children) — the sensor idles until a child begins loading.

    // 7. timeOut deadline: measured from activation, re-read live, 0 = forever.
    if (st.active && ls->getTimeOut() > 0 &&
        now - st.activatedAt > ls->getTimeOut())
      failureBurst();

    // 8.
    st.everEvaluated = true;
  }
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_LOAD_SENSOR_SYSTEM_HPP
