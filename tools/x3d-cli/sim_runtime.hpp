// tools/x3d-cli/sim_runtime.hpp
// ─────────────────────────────────────────────────────────────────────────────
// "Wire full runtime" helper for `x3d sim` — the reusable unit that, given a
// parsed Scene + an X3DExecutionContext, attaches EVERY behavior system the
// runtime ships so the scene's event/behavior graph actually runs headlessly.
//
// This is the gap the design (2026-06-20-x3d-sim-design.md, Unit 1) fills: no
// single "attach all behaviors" helper existed — each system had its own
// per-node attach idiom (attachInterpolators / attachEventUtilities /
// attachViewDependent in X3DSceneBridge.hpp; TimeSensorSystem + ScriptSystem
// wired by hand in tests). attachFullRuntime() composes them in one call.
//
// Systems attached (all behind buildSceneGraph + buildFrom, which the caller
// runs first):
//   • TimeSensorSystem            (§8 Time) — drives loop=true clocks
//   • the full interpolator set   (§19, incl. the 5 spline/squad/ease systems)
//   • EventUtilities systems      (§30 triggers/sequencers/filters)
//   • ViewDependentSystem         (§22/§23 Proximity/Visibility/LOD/Billboard)
//   • ViewpointBindSystem         (§23.3.1 — set_bind jump/retainUserOffsets + bind transitions)
//   • KeyDeviceSensorSystem       (§21 — inert until key input is fed; harmless)
//   • ScriptSystem(EcmaScript)    (§29) — ONLY when a script backend is linked
//                                  (X3D_SIM_HAVE_SCRIPT). Else scripts are inert
//                                  and that is reported, not a failure.
//
// Systems DELIBERATELY OMITTED:
//   • NavigationSystem  — sim drives the viewer directly via ctx.setHeadPose()
//                         (the --move driver), so the interactive nav controller
//                         is intentionally bypassed; attaching it would cause
//                         competing viewer updates.
//   • PointingSensorSystem — deferred to the --key/--click input-injection
//                            fast-follow (v1 has no per-tick mouse/pick input).
//                            KeyDeviceSensorSystem IS attached (it is inert
//                            until key events are fed, which is harmless).
//
// PHYSICS-ATTACH HOOK: attachPhysics() wires the Jolt-backed PhysicsSystem when
// built with X3D_HAVE_PHYSICS — enrolling RigidBodyCollections and the
// CollisionSensors that report §37 contacts. A clean no-op when physics is off.
// It is called by attachFullRuntime so the integration point is always present.
// ─────────────────────────────────────────────────────────────────────────────
#ifndef X3D_SIM_RUNTIME_HPP
#define X3D_SIM_RUNTIME_HPP

#include "X3DExecutionContext.hpp"
#include "X3DSceneBridge.hpp"   // attachInterpolators / attachEventUtilities /
                                // attachViewDependent / attachKeyDeviceSensors /
                                // attachLoadSensors + detail::forEachNode
#include "TimeSensorSystem.hpp"
#include "ViewpointBindSystem.hpp"  // attachViewpointBind (post-cascade hook)
#include "X3DScene.hpp"

#ifdef X3D_SIM_HAVE_SCRIPT
#  include "EcmaScriptBackend.hpp"
#  include "ScriptSystem.hpp"
#endif

#ifdef X3D_HAVE_PHYSICS
#  include "JoltBackend.hpp"
#  include "PhysicsSystem.hpp"
#endif

#include <memory>
#include <string>
#include <vector>

namespace x3d::sim {

/// Result of wiring the runtime: which optional subsystems actually attached.
struct RuntimeWiring {
  bool scriptEngineLinked = false;  ///< true if a ScriptSystem was attached.
  std::size_t scriptsEnrolled = 0;  ///< Script nodes offered to the engine.
};

/// Physics-attach hook (physics seam). When built with X3D_HAVE_PHYSICS, creates
/// the Jolt-backed PhysicsSystem, walks the scene to enrol each CollisionSensor
/// (first, so the contact reporter is ready) and each RigidBodyCollection, and
/// ctx.addSystem(...)s it — so `sim` traces the simulated poses and §37 contact
/// events with no change to the tick loop or tracer. A clean no-op when physics
/// is not compiled in. Kept as a real, called function so the integration point
/// is load-bearing and discoverable.
inline void attachPhysics(x3d::runtime::Scene &scene,
                          x3d::runtime::X3DExecutionContext &ctx) {
#ifdef X3D_HAVE_PHYSICS
  using namespace x3d::runtime;
  // Only wire physics when the scene actually has a RigidBodyCollection, so a
  // flag-ON build is byte-for-byte identical to OFF on every non-physics scene
  // (no extra System enters the tick loop / cascade quiescence pass). Build the
  // Jolt-backed integrator, attach every collection (createWorld(gravity) +
  // addBody per RigidBody); the PhysicsSystem then steps each tick and posts
  // position/orientation events that fan over the author ROUTEs to the
  // Transforms — sim traces it with no tick-loop change.
  std::vector<X3DNode *> collections;
  std::vector<X3DNode *> sensors;
  detail::forEachNode(scene, [&](X3DNode *n) {
    if (!n) return;
    if (n->nodeTypeName() == "RigidBodyCollection") collections.push_back(n);
    else if (n->nodeTypeName() == "CollisionSensor") sensors.push_back(n);
  });
  if (!collections.empty()) {
    auto backend = std::make_shared<JoltBackend>();
    auto physics = std::make_shared<PhysicsSystem>(backend);
    // Enroll CollisionSensors FIRST so the reporter is ready before the first
    // update() drains contacts (attach order: sensors then collections).
    for (X3DNode *n : sensors) physics->attach(n, ctx);
    for (X3DNode *n : collections) physics->attach(n, ctx);
    ctx.addSystem(physics);
  }
#else
  // No-op when the physics backend is not built (default). The integration point
  // stays load-bearing and discoverable. See the physics-attach hook note above.
  (void)scene;
  (void)ctx;
#endif
}

/// Attach EVERY behavior system to `ctx` for `scene`. The caller MUST have
/// already run ctx.buildSceneGraph(scene) + ctx.buildFrom(scene) so the route
/// graph + scene-graph index exist (this helper only attaches the time-/event-
/// driven Systems on top of that). Returns which optional subsystems wired.
inline RuntimeWiring attachFullRuntime(x3d::runtime::Scene &scene,
                                       x3d::runtime::X3DExecutionContext &ctx) {
  using namespace x3d::runtime;
  RuntimeWiring wiring;

  // ── ScriptSystem FIRST (when linked) — §29.2.5 prepareEvents must precede the
  // other sensor systems + the cascade drain; addScriptSystem inserts it at the
  // front of the system list and wires its post-cascade eventsProcessed hook.
#ifdef X3D_SIM_HAVE_SCRIPT
  {
    auto backend = std::make_shared<EcmaScriptBackend>();
    auto scriptSys = std::make_shared<ScriptSystem>(backend, "x3d-sim", "4.0");
    detail::forEachNode(scene, [&](X3DNode *n) {
      if (n && n->nodeTypeName() == "Script") {
        scriptSys->attach(n, ctx);
        ++wiring.scriptsEnrolled;
      }
    });
    ctx.addScriptSystem(scriptSys);
    wiring.scriptEngineLinked = true;
  }
#endif

  // ── TimeSensorSystem (§8) — one system services every TimeSensor.
  {
    auto tss = std::make_shared<TimeSensorSystem>();
    detail::forEachNode(scene, [&](X3DNode *n) { tss->attach(n, ctx); });
    ctx.addSystem(tss);
  }

  // ── Interpolators (§19, incl. spline/squad/ease), EventUtilities (§30),
  //    ViewDependent (§22/§23), KeyDeviceSensors (§21). These free helpers each
  //    create + attach + register their systems (the production wiring used by
  //    the SDK), so sim lights up exactly what a browser would.
  attachInterpolators(scene, ctx);
  attachFollowers(scene, ctx);
  attachEventUtilities(scene, ctx);
  attachViewDependent(scene, ctx);
  attachKeyDeviceSensors(scene, ctx);

  // ── LoadSensor (§9) — one system observes every LoadSensor's watched
  //    children through the AssetResolver seam. Defaults install the SEC-3
  //    confined local-file resolver, so `x3d sim` reports load state for local
  //    files out of the box.
  attachLoadSensors(scene, ctx);

  // ── ViewpointBindSystem (§23.3.1) — post-cascade hook driving set_bind
  //    jump/retainUserOffsets + bind-stack transitions (scene-agnostic: it reads
  //    the bound viewpoint each post-cascade pass).
  attachViewpointBind(ctx);

  // ── Physics-attach hook (no-op today; the Jolt RigidBodySystem plugs in here).
  attachPhysics(scene, ctx);

  return wiring;
}

}  // namespace x3d::sim

#endif  // X3D_SIM_RUNTIME_HPP
