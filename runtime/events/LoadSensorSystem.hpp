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
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace x3d::runtime {

using namespace x3d::core;

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
  };

  template <typename T>
  void emit(X3DExecutionContext &ctx, X3DNode *n, const char *field, T v) {
    ctx.postEvent(n, field, std::any(v));
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

// Task 2 skeleton: activate on any loadable watched child. The real per-tick
// resolution algorithm (poll / fallback / pre-seed / bursts / progress) replaces
// this in Task 3.
inline void LoadSensorSystem::update(double now, X3DExecutionContext &ctx) {
  for (auto &[ls, st] : state_) {
    if (!ls->getEnabled())
      continue;
    for (auto &childAny : ls->getChildren()) {
      X3DNode *c = childAny.get();
      if (!c || !x3d::nodes::X3DInterfaceRegistry::nodeImplements(
                    c, x3d::nodes::InterfaceId::X3DUrlObject))
        continue;
      if (!st.active) {
        st.active = true;
        st.activatedAt = now;
        emit(ctx, ls, "isActive", SFBool{true});
        emit(ctx, ls, "progress", SFFloat{0.0f});
        if (sensorHook_)
          sensorHook_(ls, true, now);
      }
    }
  }
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_LOAD_SENSOR_SYSTEM_HPP
