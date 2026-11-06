// Attach headless simulation behaviors after building the scene and route graph.
// Navigation is omitted because --move drives the viewer directly.
#ifndef X3D_SIM_RUNTIME_HPP
#define X3D_SIM_RUNTIME_HPP

#include "X3DExecutionContext.hpp"
#include "X3DSceneBridge.hpp"
#include "TimeSensorSystem.hpp"
#include "io/file/FileResolver.hpp"
#include "ViewpointBindSystem.hpp"
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

/// §37: with physics enabled, enroll CollisionSensors before RigidBodyCollections so
/// contact reporting is ready for the first update. Otherwise this is a no-op.
inline void attachPhysics(x3d::runtime::Scene &scene,
                          x3d::runtime::X3DExecutionContext &ctx) {
#ifdef X3D_HAVE_PHYSICS
  using namespace x3d::runtime;
  // Avoid adding a physics tick system when no RigidBodyCollection is present.
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
    // Enroll CollisionSensors first so the reporter is ready before the first
    // update() drains contacts (attach order: sensors then collections).
    for (X3DNode *n : sensors) physics->attach(n, ctx);
    for (X3DNode *n : collections) physics->attach(n, ctx);
    ctx.addSystem(physics);
  }
#else
  (void)scene;
  (void)ctx;
#endif
}

/// Requires ctx.buildSceneGraph(scene) and ctx.buildFrom(scene) to establish
/// the route graph and scene index. Reports optional subsystem attachment.
inline RuntimeWiring attachFullRuntime(x3d::runtime::Scene &scene,
                                       x3d::runtime::X3DExecutionContext &ctx) {
  using namespace x3d::runtime;
  RuntimeWiring wiring;

  // §29.2.5: prepareEvents precedes sensors and cascade delivery.
  // addScriptSystem also wires the post-cascade eventsProcessed hook.
#ifdef X3D_SIM_HAVE_SCRIPT
  {
    auto backend = std::make_shared<EcmaScriptBackend>();
    // Resolve Script URLs and autoRefresh requests within activeConfineRoot.
    // See ADR-0038 for path confinement.
    auto scriptSys = std::make_shared<ScriptSystem>(
        backend, "x3d-sim", "4.0", io::file::makeFileResolver());
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

  // §8: one system services every TimeSensor.
  {
    auto tss = std::make_shared<TimeSensorSystem>();
    detail::forEachNode(scene, [&](X3DNode *n) { tss->attach(n, ctx); });
    ctx.addSystem(tss);
  }

  // §19 interpolators, §30 event utilities, §22/§23 view-dependent systems,
  // and §21 key sensors.
  attachInterpolators(scene, ctx);
  attachFollowers(scene, ctx);
  attachEventUtilities(scene, ctx);
  attachViewDependent(scene, ctx);
  attachKeyDeviceSensors(scene, ctx);

  // §9: LoadSensor watches local files confined to activeConfineRoot.
  // See ADR-0038 for path confinement.
  attachLoadSensors(scene, ctx, io::file::makeFileResolver());

  // §31: validate shader structure without compilation.
  attachShaders(scene, ctx);

  // §34.4.2: update NEXT_FRAME_ONLY to NONE.
  attachGeneratedCubeMaps(scene, ctx);
  attachTextureProjectors(scene, ctx);

  // §23.3.1: apply viewpoint jumps and bind transitions after the cascade.
  attachViewpointBind(ctx);

  attachPhysics(scene, ctx);

  return wiring;
}

}  // namespace x3d::sim

#endif  // X3D_SIM_RUNTIME_HPP
