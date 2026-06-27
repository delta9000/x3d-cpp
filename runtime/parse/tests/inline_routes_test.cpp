// inline_routes_test.cpp
// Task 4: Internal route hoisting registered by the scene-bridge.
// Parses animated_parent.x3d (which Inlines animated_child.x3d), asserts
// resolvedInlineRoutes has 2 pre-resolved routes, then builds the bridge
// and asserts those routes are registered on the execution context.
// Finally, attaches the TimeSensor and interpolator systems, ticks to t=1
// within the 4-second cycle (fraction=0.25), and asserts the inlined
// Transform "Mover" actually moved (translation.x > 0).
// Pass the inline fixtures dir as argv[1].
#include "x3d/sdk.hpp"
#include "TimeSensorSystem.hpp"
#include "InterpolatorRegistration.hpp"
#include "x3d/nodes/Transform.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
  assert(argc >= 2 && "pass the inline fixtures dir as argv[1]");
  std::string dir = argv[1];
  auto doc = x3d::sdk::parseFile(dir + "/animated_parent.x3d");

  // The child's two internal ROUTEs were hoisted, pre-resolved.
  assert(doc.scene.resolvedInlineRoutes.size() == 2 &&
         "both child ROUTEs hoisted with concrete endpoints");
  for (auto& r : doc.scene.resolvedInlineRoutes) {
    assert(r.from && r.to && "endpoints resolved in child scope");
  }

  // Bridge + attach systems: the inlined Transform should actually move on tick.
  x3d::sdk::X3DExecutionContext ctx;
  ctx.buildSceneGraph(doc.scene);
  auto bridge = ctx.buildFrom(doc.scene);
  assert(bridge.routesAdded >= 2 && "inline routes registered on the context");

  // Wire the TimeSensor system. Pull Clock from the first hoisted route's 'from'
  // endpoint (Clock.fraction_changed -> Path.set_fraction).
  using namespace x3d::runtime;
  auto tss = std::make_shared<TimeSensorSystem>();
  ctx.addSystem(tss);
  tss->attach(doc.scene.resolvedInlineRoutes[0].from.get(), ctx);

  // Wire all interpolator systems so PositionInterpolator reacts to set_fraction.
  attachInterpolators(doc.scene, ctx);

  // The TimeSensor has startTime=0 (default) and loop=true (per fixture).
  // Tick 1: t=0 — activates the sensor, fraction_changed=0 cascades to Path,
  //   value_changed=0 cascades to Mover (translation stays at origin).
  ctx.tick(0.0);

  // Tick 2: t=1 — cycleInterval=4, so fraction = 1/4 = 0.25.
  //   PositionInterpolator: key=[0,1], keyValue=[(0,0,0),(10,0,0)].
  //   value_changed = lerp(0,10,0.25) = (2.5, 0, 0) → Mover.translation.x ≈ 2.5.
  ctx.tick(1.0);

  // Locate Mover from the second hoisted route's 'to' endpoint
  // (Path.value_changed -> Mover.set_translation).
  std::shared_ptr<X3DNode> moverNode = doc.scene.resolvedInlineRoutes[1].to;
  assert(moverNode && "Mover node must be alive via hoisted route endpoint");

  auto* mover = dynamic_cast<Transform*>(moverNode.get());
  assert(mover && "Mover route endpoint must be a Transform");

  SFVec3f translation = mover->getTranslation();
  assert(translation.x > 0.0f &&
         "inlined Transform moved: translation.x > 0 after tick at t=1 "
         "(expected ~2.5 for fraction=0.25 over keyValue 0..10)");

  std::cout << "inline_routes_test OK (Mover.translation.x = "
            << translation.x << ")\n";
  return 0;
}
