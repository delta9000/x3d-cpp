// bind_time_conformance_test.cpp — ISO/IEC 19775-1 §23.3.1 conformance for bindTime:
// (a) load-time default bind emits bindTime; (b) unbinding an active node emits bindTime.
// RTC-9 regression: drop `if (bound)` guard + load-time clock call.
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/X3DBindableNode.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp" // out-of-line Scene::addRootNode
#include <any>
#include "doctest/doctest.h"
#include <memory>
using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;

static X3DBindableNode* bnd(const std::shared_ptr<X3DNode>& n) {
  return dynamic_cast<X3DBindableNode*>(n.get());
}

// ── Test (a): load-time default bind must emit bindTime ──────────────────────
// buildSceneGraph -> bindDefaults -> first Viewpoint default-bound;
// its getBindTime() must reflect the clock value used at build time (non-zero).
static void test_load_time_bind_emits_bindTime() {
  auto vp = createX3DNode("Viewpoint");
  Scene scene; scene.addRootNode(vp);

  X3DExecutionContext ctx;
  // Advance the context clock so the load-time bindTime has a known, exact value.
  constexpr double kLoadTime = 42.0;
  ctx.tick(kLoadTime);
  ctx.buildSceneGraph(scene);

  // After buildSceneGraph, vp must be default-bound with isBound==true.
  CHECK((bnd(vp)->getIsBound() == true));
  // §23.3.1 case 1: "during loading" — bindTime must equal the build-time clock.
  CHECK((bnd(vp)->getBindTime() == kLoadTime && "load-time bind must emit bindTime (§23.3.1 case 1)"));
}

// ── Test (b): explicit unbind of active node must emit bindTime ───────────────
// When set_bind FALSE fires on the currently-bound node, bindTime must be emitted
// (with the timestamp of the unbind) in addition to isBound FALSE.
static void test_unbind_emits_bindTime() {
  auto vp1 = createX3DNode("Viewpoint");
  auto vp2 = createX3DNode("Viewpoint");
  Scene scene; scene.addRootNode(vp1); scene.addRootNode(vp2);

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);

  // vp2 bind at t=1
  ctx.postEvent(vp2.get(), "set_bind", std::any(SFBool(true)));
  ctx.tick(1.0);
  CHECK((bnd(vp2)->getIsBound() == true));
  CHECK((bnd(vp2)->getBindTime() == 1.0 && "bind must emit bindTime"));

  // Now unbind vp2 at t=2: bindTime must fire again on vp2 (unbind event).
  ctx.postEvent(vp2.get(), "set_bind", std::any(SFBool(false)));
  ctx.tick(2.0);
  CHECK((bnd(vp2)->getIsBound() == false));
  // §23.3.1: "sends the time at which the node is bound or unbound"
  CHECK((bnd(vp2)->getBindTime() == 2.0 && "unbind must emit bindTime (§23.3.1)"));

  // vp1 is now back on top and also got a bindTime at t=2 (the restore-bind event).
  CHECK((bnd(vp1)->getIsBound() == true));
  CHECK((bnd(vp1)->getBindTime() == 2.0 && "restored top must emit bindTime on re-bind"));
}

TEST_CASE("bind_time_conformance_test") {
  test_load_time_bind_emits_bindTime();
  test_unbind_emits_bindTime();
  return;
}
