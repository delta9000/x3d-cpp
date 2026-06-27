// scene_bridge_test.cpp
// Tests for the Scene -> EventGraph bridge (runtime/events/X3DSceneBridge.hpp):
//
//   1. ROUTE validation: valid routes are added; unknown-field, wrong-direction,
//      and type-mismatch routes are rejected with a diagnostic carrying the
//      offending route index; an unresolved (dangling DEF) endpoint is skipped
//      silently (neither added nor rejected).
//   2. End-to-end load + tick: parse a .x3dv document that wires
//      TimeSensor.fraction_changed -> PositionInterpolator.set_fraction ->
//      Transform.translation, bridge its ROUTEs onto an execution context via
//      X3DExecutionContext::buildFrom, attach the reference behaviors to the
//      DEF-resolved nodes, tick the clock, and assert the Transform's
//      translation animates along the interpolator's keyValue curve.
//
// The fixtures dir is passed as argv[1] (CMake sets it); fall back to a
// repo-relative path so the test still runs from the source tree root.
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "InterpolatorSystem.hpp"
#include "Interpolation.hpp"
#include "TimeSensorBehavior.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSceneBridge.hpp"

#include "X3DParse.hpp"
#include "X3DRuntime.hpp"

#include "x3d/nodes/PositionInterpolator.hpp"
#include "x3d/nodes/TimeSensor.hpp"
#include "x3d/nodes/Transform.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DProtoExpand.hpp"

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;

namespace {

int failures = 0;

void check(bool cond, const std::string &what) {
  if (!cond) {
    std::cerr << "FAIL: " << what << "\n";
    ++failures;
  } else {
    std::cout << "ok: " << what << "\n";
  }
}

bool veq(const SFVec3f &a, float x, float y, float z) {
  return a.x == x && a.y == y && a.z == z;
}

SFVec3f tr(const std::shared_ptr<Transform> &t) { return t->getTranslation(); }

std::string g_dataDir;

std::string readFile(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

// --- (1) Validation -------------------------------------------------------
// Build a Scene programmatically with one valid route and one of each invalid
// kind, plus a dangling-DEF route. Bridge it and assert the diagnostics.
void test_route_validation() {
  auto clock = std::make_shared<TimeSensor>();
  auto interp = std::make_shared<PositionInterpolator>();
  auto mover = std::make_shared<Transform>();

  Scene scene;
  scene.define("Clock", clock);
  scene.define("Path", interp);
  scene.define("Mover", mover);

  // index 0: VALID  outputOnly SFVec3f -> inputOutput SFVec3f.
  scene.routes.emplace_back("Path", "value_changed", "Mover", "translation");
  // index 1: UNKNOWN source field.
  scene.routes.emplace_back("Path", "no_such_field", "Mover", "translation");
  // index 2: UNKNOWN sink field.
  scene.routes.emplace_back("Clock", "fraction_changed", "Mover", "nope");
  // index 3: WRONG DIRECTION (routing TO an outputOnly sink).
  scene.routes.emplace_back("Clock", "fraction_changed", "Path",
                            "value_changed");
  // index 4: WRONG DIRECTION (routing FROM an inputOnly source).
  scene.routes.emplace_back("Path", "set_fraction", "Mover", "translation");
  // index 5: TYPE MISMATCH (SFFloat fraction -> SFVec3f translation).
  scene.routes.emplace_back("Clock", "fraction_changed", "Mover",
                            "translation");
  // index 6: DANGLING DEF (unknown source node) -> skipped silently.
  scene.routes.emplace_back("Ghost", "value_changed", "Mover", "translation");

  X3DExecutionContext ctx;
  BridgeResult r = buildRoutes(scene, ctx);

  check(r.routesAdded == 1, "exactly one valid route added");
  check(r.rejected.size() == 5, "five routes rejected (dangling not counted)");

  // The rejected indices are 1..5 (0 valid, 6 silently skipped).
  bool got1 = false, got2 = false, got3 = false, got4 = false, got5 = false;
  for (const RouteError &e : r.rejected) {
    if (e.index == 1)
      got1 = true;
    if (e.index == 2)
      got2 = true;
    if (e.index == 3)
      got3 = true;
    if (e.index == 4)
      got4 = true;
    if (e.index == 5)
      got5 = true;
    check(!e.reason.empty(), "rejected route carries a reason");
  }
  check(got1, "unknown source field rejected (index 1)");
  check(got2, "unknown sink field rejected (index 2)");
  check(got3, "route to outputOnly sink rejected (index 3)");
  check(got4, "route from inputOnly source rejected (index 4)");
  check(got5, "type-mismatch route rejected (index 5)");
  check(r.ok() == false, "BridgeResult::ok() false when routes rejected");
}

// A scene whose every route is valid yields ok() == true and no rejects.
void test_all_valid_ok() {
  auto clock = std::make_shared<TimeSensor>();
  auto interp = std::make_shared<PositionInterpolator>();
  auto mover = std::make_shared<Transform>();

  Scene scene;
  scene.define("Clock", clock);
  scene.define("Path", interp);
  scene.define("Mover", mover);
  scene.routes.emplace_back("Clock", "fraction_changed", "Path",
                            "set_fraction");
  scene.routes.emplace_back("Path", "value_changed", "Mover", "translation");

  X3DExecutionContext ctx;
  BridgeResult r = buildRoutes(scene, ctx);
  check(r.routesAdded == 2, "both valid routes added");
  check(r.ok(), "BridgeResult::ok() true when nothing rejected");
}

// --- (2) Load a document with ROUTEs and tick it --------------------------
void test_load_and_tick() {
  std::string text = readFile(g_dataDir + "/animated_transform.x3dv");
  check(!text.empty(), "fixture animated_transform.x3dv loaded");

  X3DDocument doc = x3d::codec::parseDocument(text);
  Scene &scene = doc.scene;

  check(scene.routes.size() == 2, "two ROUTEs parsed from the document");

  // Bridge the DEF-named ROUTEs onto a fresh execution context in one call.
  X3DExecutionContext ctx;
  BridgeResult r = ctx.buildFrom(scene);
  check(r.routesAdded == 2, "both document ROUTEs bridged");
  check(r.ok(), "no ROUTEs rejected from the loaded document");

  // Resolve the DEF'd nodes and attach the reference behaviors. The behaviors
  // act on the SAME node pointers the bridge addressed (the Scene owns them).
  auto clock = std::dynamic_pointer_cast<TimeSensor>(scene.resolve("Clock"));
  auto path =
      std::dynamic_pointer_cast<PositionInterpolator>(scene.resolve("Path"));
  auto mover = std::dynamic_pointer_cast<Transform>(scene.resolve("Mover"));
  check(clock && path && mover, "DEF table resolves Clock/Path/Mover by type");
  if (!(clock && path && mover)) {
    return;
  }

  // cycleInterval 4.0, loop TRUE; key 0/0.5/1 -> value (0,0,0)/(10,0,0)/(0,0,0).
  // The deprecation shim wraps an ActiveNode in a one-node System; new code
  // should implement System directly. Tracked for migration.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  ctx.addActiveNode(std::make_shared<TimeSensorBehavior>(clock.get()));
#pragma GCC diagnostic pop
  InterpolatorSystem<PositionInterpolator, SFVec3f> interpSys(
      [](const SFVec3f &a, const SFVec3f &b, float t) { return lerpVec3(a, b, t); });
  interpSys.attach(path.get(), ctx);

  // t=0 -> fraction 0 -> value (0,0,0).
  ctx.tick(0.0);
  check(veq(tr(mover), 0, 0, 0), "tick(0): translation at curve start (0,0,0)");

  // t=1 -> fraction 0.25 -> halfway on the first segment -> (5,0,0).
  ctx.tick(1.0);
  check(veq(tr(mover), 5, 0, 0), "tick(1): translation animated to (5,0,0)");

  // t=2 -> fraction 0.5 -> mid keyValue -> (10,0,0).
  ctx.tick(2.0);
  check(veq(tr(mover), 10, 0, 0), "tick(2): translation animated to (10,0,0)");

  // t=4 -> looped fraction 0 -> back to (0,0,0).
  ctx.tick(4.0);
  check(veq(tr(mover), 0, 0, 0), "tick(4): loop wraps back to (0,0,0)");
}

// A ROUTE whose sink is an initializeOnly field must be rejected: the field is
// now data-layer writable, but initializeOnly is not a routable event sink.
void testInitializeOnlyNotRoutableSink() {
  Scene scene;
  auto ts = createX3DNode("TimeSensor");
  auto sphere = createX3DNode("Sphere");
  scene.define("TS", ts);
  scene.define("SP", sphere);
  scene.addRootNode(ts);
  scene.addRootNode(sphere);
  scene.routes.push_back(Route{"TS", "fraction_changed", "SP", "radius"});

  X3DExecutionContext ctx;
  BridgeResult r = buildRoutes(scene, ctx);
  check(r.routesAdded == 0, "initializeOnly: route to radius not added");
  check(r.rejected.size() == 1, "initializeOnly: route to radius rejected");
}

// Proto interface event redirect + pre-resolved body-route registration.
// An expanded instance "A" of proto "Anim" exposes inputOnly 'fraction'
// (IS body PositionInterpolator.set_fraction) and outputOnly 'out' (IS body
// PositionInterpolator.value_changed). The interface names deliberately DIFFER
// from the body field names so the redirect path is the only thing that can
// satisfy the external routes — a primary-native field lookup would not find
// 'fraction'/'out'. Asserts:
//   - external routes naming the instance DEF + an interface field are
//     redirected onto the cloned body endpoints (sink AND source side), not
//     rejected as unknown fields;
//   - a body-internal ROUTE is registered directly from resolvedProtoRoutes.
void testProtoRouteRedirect() {
  Scene scene;

  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Anim";
  ProtoField in;
  in.name = "fraction";
  in.type = X3DFieldType::SFFloat;
  in.access = AccessType::InputOnly;
  decl->interface.push_back(in);
  ProtoField out;
  out.name = "out";
  out.type = X3DFieldType::SFVec3f;
  out.access = AccessType::OutputOnly;
  decl->interface.push_back(out);

  auto pi = createX3DNode("PositionInterpolator");
  pi->setDEF("PI");
  auto tr = createX3DNode("Transform");
  tr->setDEF("T");
  decl->body.nodes.push_back(pi); // primary = first body node
  decl->body.nodes.push_back(tr);
  decl->body.isConnections.push_back({pi, "set_fraction", "fraction"});
  decl->body.isConnections.push_back({pi, "value_changed", "out"});
  decl->body.routes.push_back(Route{"PI", "value_changed", "T", "translation"});
  scene.protoDeclarations.push_back(decl);

  auto ts = createX3DNode("TimeSensor");
  ts->setDEF("TS");
  scene.addRootNode(ts);
  auto dest = createX3DNode("Transform");
  dest->setDEF("DEST");
  scene.addRootNode(dest);

  x3d::runtime::ProtoInstance inst; // disambiguate from generated node type
  inst.name = "Anim";
  inst.declaration = decl;
  inst.DEF = "A";
  scene.protoInstances.push_back(inst);

  std::vector<ProtoWarning> w;
  expandScene(scene, x3d::codec::noopProtoResolver, "", w);

  // External routes that name the instance interface fields (not body fields).
  scene.routes.push_back(
      Route{"TS", "fraction_changed", "A", "fraction"}); // sink redirect
  scene.routes.push_back(
      Route{"A", "out", "DEST", "translation"}); // source redirect

  X3DExecutionContext ctx;
  BridgeResult res = buildRoutes(scene, ctx);

  bool rejectedInterfaceField = false;
  for (auto &r : res.rejected)
    if (r.reason.find("fraction") != std::string::npos ||
        r.reason.find("'out'") != std::string::npos)
      rejectedInterfaceField = true;
  check(!rejectedInterfaceField,
        "proto redirect: interface fields not rejected");
  // 1 pre-resolved body route + 2 redirected external routes.
  check(res.routesAdded == 3,
        "proto redirect: 3 routes added (body route + sink + source redirect)");
}

} // namespace

int main(int argc, char **argv) {
  if (argc > 1) {
    g_dataDir = argv[1];
  } else {
    g_dataDir = "runtime/parse/tests/data/x3dv";
  }

  test_route_validation();
  test_all_valid_ok();
  test_load_and_tick();
  testInitializeOnlyNotRoutableSink();
  testProtoRouteRedirect();

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    return 1;
  }
  std::cout << "all scene-bridge tests passed\n";
  return 0;
}
