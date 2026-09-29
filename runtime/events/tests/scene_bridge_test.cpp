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
#include "x3d/nodes/Group.hpp"
#include "x3d/nodes/TimeSensor.hpp"
#include "x3d/nodes/Transform.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DProtoExpand.hpp"

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

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

// Body ROUTEs name the nested instance's declared interface, even when its
// physical primary has a different field name. Each Outer gets private clones.
void testNestedProtoBodyRoutes() {
  const std::string xml = R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='Inner'><ProtoInterface>
<field name='shift' type='SFVec3f' accessType='inputOutput' value='0 0 0'/>
</ProtoInterface><ProtoBody><Transform><IS>
<connect nodeField='translation' protoField='shift'/>
</IS></Transform></ProtoBody></ProtoDeclare>
<ProtoDeclare name='Outer'><ProtoBody><Group>
<ProtoInstance name='Inner' DEF='A'/><Transform DEF='Ord'/>
<ProtoInstance name='Inner' DEF='B'/>
</Group><ROUTE fromNode='A' fromField='shift_changed' toNode='Ord' toField='set_translation'/>
<ROUTE fromNode='Ord' fromField='translation_changed' toNode='B' toField='set_shift'/>
<ROUTE fromNode='A' fromField='shift_changed' toNode='B' toField='set_shift'/>
</ProtoBody></ProtoDeclare><ProtoInstance name='Outer' DEF='O1'/>
<ProtoInstance name='Outer' DEF='O2'/></Scene></X3D>)";
  const std::string classic = R"(#X3D V4.0 utf8
PROTO Inner [ inputOutput SFVec3f shift 0 0 0 ] {
  Transform { translation IS shift }
}
PROTO Outer [ ] {
  Group { children [ DEF A Inner { } DEF Ord Transform { } DEF B Inner { } ] }
  ROUTE A.shift_changed TO Ord.set_translation
  ROUTE Ord.translation_changed TO B.set_shift
  ROUTE A.shift_changed TO B.set_shift
}
DEF O1 Outer { }
DEF O2 Outer { }
)";
  for (const auto &[name, source, encoding] :
       std::vector<std::tuple<std::string, std::string, x3d::codec::Encoding>>{
           {"XML", xml, x3d::codec::Encoding::XML},
           {"Classic", classic, x3d::codec::Encoding::ClassicVRML}}) {
    auto doc = x3d::codec::parseDocument(source, encoding);
    auto o1 = std::dynamic_pointer_cast<Group>(doc.scene.resolve("O1"));
    auto o2 = std::dynamic_pointer_cast<Group>(doc.scene.resolve("O2"));
    check(o1 && o2 && o1 != o2, name + ": two Outer primaries expanded");
    if (!(o1 && o2) || o1->getChildren().size() != 3 ||
        o2->getChildren().size() != 3) {
      check(false, name + ": ordered nested/ordinary children retained");
      continue;
    }
    auto a1 = std::dynamic_pointer_cast<Transform>(o1->getChildren()[0]);
    auto mid1 = std::dynamic_pointer_cast<Transform>(o1->getChildren()[1]);
    auto b1 = std::dynamic_pointer_cast<Transform>(o1->getChildren()[2]);
    auto a2 = std::dynamic_pointer_cast<Transform>(o2->getChildren()[0]);
    auto mid2 = std::dynamic_pointer_cast<Transform>(o2->getChildren()[1]);
    auto b2 = std::dynamic_pointer_cast<Transform>(o2->getChildren()[2]);
    check(a1 && mid1 && b1 && a2 && mid2 && b2,
          name + ": nested interface endpoints have concrete Transform primaries");
    if (!(a1 && mid1 && b1 && a2 && mid2 && b2)) continue;
    X3DExecutionContext ctx;
    auto bridge = buildRoutes(doc.scene, ctx);
    check(bridge.rejected.empty(), name + ": valid nested body ROUTEs accepted");
    check(bridge.routesAdded == 6,
          name + ": all ordinary and two-nested-endpoint routes bridged per instance");
    ctx.postEvent(a1.get(), "translation_changed", std::any(SFVec3f{3, 4, 5}));
    ctx.tick(0.0);
    check(veq(tr(mid1), 3, 4, 5) && veq(tr(b1), 3, 4, 5),
          name + ": nested source reaches ordinary sink and nested sink");
    check(veq(tr(mid2), 0, 0, 0) && veq(tr(b2), 0, 0, 0),
          name + ": first Outer cannot deliver into second Outer's private nodes");
    ctx.postEvent(mid2.get(), "translation_changed", std::any(SFVec3f{6, 7, 8}));
    ctx.tick(1.0);
    check(veq(tr(b2), 6, 7, 8) && veq(tr(b1), 3, 4, 5),
          name + ": ordinary source reaches only its own nested sink");
  }
}

// A direct first nested instance is also the Outer primary. Body-local A must
// retain its own interface route mapping while scene routes see only Outer's
// exposed names; the underlying Transform.translation is private.
void testNestedFirstPrimaryAndEventIsAliases() {
  const char *xml = R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='Inner'><ProtoInterface>
<field name='shift' type='SFVec3f' accessType='inputOutput' value='0 0 0'/>
<field name='inShift' type='SFVec3f' accessType='inputOnly'/>
<field name='outShift' type='SFVec3f' accessType='outputOnly'/>
</ProtoInterface><ProtoBody><Transform><IS>
<connect nodeField='translation' protoField='shift'/>
</IS></Transform>
<Transform DEF='PhysicalSink'><IS>
<connect nodeField='set_translation' protoField='inShift'/>
</IS></Transform>
<Transform DEF='PhysicalSource'><IS>
<connect nodeField='translation_changed' protoField='outShift'/>
</IS></Transform></ProtoBody></ProtoDeclare>
<ProtoDeclare name='Outer'><ProtoInterface>
<field name='topIn' type='SFVec3f' accessType='inputOnly'/>
<field name='topOut' type='SFVec3f' accessType='outputOnly'/>
</ProtoInterface><ProtoBody>
<ProtoInstance name='Inner' DEF='A'><IS>
<connect nodeField='set_shift' protoField='topIn'/>
<connect nodeField='shift_changed' protoField='topOut'/>
</IS></ProtoInstance>
<Transform DEF='Ord'/>
<ROUTE fromNode='A' fromField='shift_changed' toNode='Ord' toField='set_translation'/>
<ROUTE fromNode='Ord' fromField='translation_changed' toNode='A' toField='inShift'/>
<ROUTE fromNode='A' fromField='outShift' toNode='Ord' toField='set_translation'/>
</ProtoBody></ProtoDeclare>
<Transform DEF='Driver'/><ProtoInstance name='Outer' DEF='O'/>
<Transform DEF='External'/><Transform DEF='Hidden'/>
<ROUTE fromNode='Driver' fromField='translation_changed' toNode='O' toField='topIn'/>
<ROUTE fromNode='O' fromField='topOut' toNode='External' toField='set_translation'/>
<ROUTE fromNode='O' fromField='translation_changed' toNode='Hidden' toField='set_translation'/>
</Scene></X3D>)";
  auto doc = x3d::codec::parseDocument(xml);
  auto driver = std::dynamic_pointer_cast<Transform>(doc.scene.resolve("Driver"));
  auto primary = std::dynamic_pointer_cast<Transform>(doc.scene.resolve("O"));
  auto ext = std::dynamic_pointer_cast<Transform>(doc.scene.resolve("External"));
  auto hidden = std::dynamic_pointer_cast<Transform>(doc.scene.resolve("Hidden"));
  std::shared_ptr<Transform> ordinary, physicalSink, physicalSource;
  for (auto &peer : doc.scene.protoPeerNodes) {
    if (peer && peer->getDEF() == "Ord")
      ordinary = std::dynamic_pointer_cast<Transform>(peer);
    if (peer && peer->getDEF() == "PhysicalSink")
      physicalSink = std::dynamic_pointer_cast<Transform>(peer);
    if (peer && peer->getDEF() == "PhysicalSource")
      physicalSource = std::dynamic_pointer_cast<Transform>(peer);
  }
  check(driver && primary && ordinary && physicalSink && physicalSource &&
            ext && hidden,
        "nested-first: first inner primary and ordinary peer are expanded");
  if (!(driver && primary && ordinary && physicalSink && physicalSource &&
        ext && hidden)) return;
  X3DExecutionContext ctx;
  auto bridge = buildRoutes(doc.scene, ctx);
  check(bridge.routesAdded == 5,
        "nested-first: body routes and two exposed outer interface routes bridged");
  check(!bridge.rejected.empty(),
        "nested-first: underlying primary-native field is hidden by Outer interface");
  ctx.postEvent(driver.get(), "translation_changed", std::any(SFVec3f{4, 5, 6}));
  ctx.tick(0.0);
  check(veq(tr(primary), 4, 5, 6),
        "nested-first: inputOnly IS alias delivers through Outer and Inner");
  check(veq(tr(ordinary), 4, 5, 6) && veq(tr(ext), 4, 5, 6),
        "nested-first: body ROUTE and outputOnly IS alias deliver events");
  check(veq(tr(physicalSink), 4, 5, 6),
        "nested-first: separate physical set_translation IS target receives event");
  ctx.postEvent(physicalSource.get(), "translation_changed",
                std::any(SFVec3f{7, 8, 9}));
  ctx.tick(1.0);
  check(veq(tr(ordinary), 7, 8, 9) && veq(tr(physicalSink), 7, 8, 9),
        "nested-first: separate physical translation_changed IS source delivers");
  check(veq(tr(hidden), 0, 0, 0),
        "nested-first: native field route cannot bypass nominal Outer interface");
}

void testNestedProtoRouteValidation() {
  const char *xml = R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='Inner'><ProtoInterface>
<field name='shift' type='SFVec3f' accessType='inputOutput' value='0 0 0'/>
<field name='inShift' type='SFVec3f' accessType='inputOnly'/>
<field name='outShift' type='SFVec3f' accessType='outputOnly'/>
</ProtoInterface><ProtoBody><Transform><IS>
<connect nodeField='translation' protoField='shift'/>
</IS></Transform>
<Transform><IS><connect nodeField='set_translation' protoField='inShift'/>
</IS></Transform>
<Transform><IS><connect nodeField='translation_changed' protoField='outShift'/>
</IS></Transform></ProtoBody></ProtoDeclare>
<ProtoDeclare name='Outer'><ProtoBody><Group>
<ProtoInstance name='Inner' DEF='A'/><ProtoInstance name='Inner' DEF='B'/>
<TimeSensor DEF='Clock'/></Group>
<ROUTE fromNode='A' fromField='set_shift' toNode='B' toField='inShift'/>
<ROUTE fromNode='A' fromField='outShift' toNode='B' toField='shift_changed'/>
<ROUTE fromNode='A' fromField='outShift' toNode='Clock' toField='set_startTime'/>
</ProtoBody></ProtoDeclare><ProtoInstance name='Outer' DEF='O'/>
</Scene></X3D>)";
  auto doc = x3d::codec::parseDocument(xml);
  auto outer = std::dynamic_pointer_cast<Group>(doc.scene.resolve("O"));
  check(outer && outer->getChildren().size() == 3,
        "invalid nested routes: endpoints survive expansion");
  if (!outer || outer->getChildren().size() != 3) return;
  auto a = std::dynamic_pointer_cast<Transform>(outer->getChildren()[0]);
  auto b = std::dynamic_pointer_cast<Transform>(outer->getChildren()[1]);
  check(a && b, "invalid nested routes: concrete primaries exist");
  if (!(a && b)) return;
  X3DExecutionContext ctx;
  auto bridge = buildRoutes(doc.scene, ctx);
  check(bridge.routesAdded == 0,
        "invalid nested routes: wrong direction and type add no edges");
  auto warnedFor = [&](const std::string &route) {
    for (const auto &warning : doc.protoWarnings)
      if (warning.detail.find(route) != std::string::npos) return true;
    for (const auto &error : bridge.rejected)
      if (error.reason.find(route) != std::string::npos) return true;
    return false;
  };
  check(warnedFor("A.set_shift TO B.inShift"),
        "invalid nested routes: input-only set_shift source diagnosed");
  check(warnedFor("A.outShift TO B.shift_changed"),
        "invalid nested routes: output-only shift_changed sink diagnosed");
  check(warnedFor("A.outShift TO Clock.set_startTime"),
        "invalid nested routes: SFVec3f to SFTime mismatch diagnosed");
  ctx.postEvent(a.get(), "translation_changed", std::any(SFVec3f{9, 8, 7}));
  ctx.tick(0.0);
  check(veq(tr(b), 0, 0, 0),
        "invalid nested routes: no value reaches outputOnly sink");
}

void testInheritedProtoMetadataRoutes() {
  const char *xml = R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='Inner'><ProtoBody><Transform/></ProtoBody></ProtoDeclare>
<ProtoDeclare name='Outer'><ProtoBody>
<ProtoInstance name='Inner' DEF='A'/><Transform DEF='Ord'/>
<ProtoInstance name='Inner' DEF='B'/>
<ROUTE fromNode='A' fromField='metadata_changed' toNode='Ord' toField='set_metadata'/>
<ROUTE fromNode='Ord' fromField='metadata_changed' toNode='B' toField='set_metadata'/>
</ProtoBody></ProtoDeclare>
<Transform DEF='Driver'/><ProtoInstance name='Outer' DEF='O'/>
<Transform DEF='External'/>
<ROUTE fromNode='Driver' fromField='metadata_changed' toNode='O' toField='set_metadata'/>
<ROUTE fromNode='O' fromField='metadata_changed' toNode='External' toField='set_metadata'/>
</Scene></X3D>)";
  auto doc = x3d::codec::parseDocument(xml);
  auto driver = std::dynamic_pointer_cast<Transform>(doc.scene.resolve("Driver"));
  auto primary = std::dynamic_pointer_cast<Transform>(doc.scene.resolve("O"));
  auto external = std::dynamic_pointer_cast<Transform>(doc.scene.resolve("External"));
  std::shared_ptr<Transform> ordinary, nestedSink;
  for (auto &peer : doc.scene.protoPeerNodes) {
    if (peer && peer->getDEF() == "Ord")
      ordinary = std::dynamic_pointer_cast<Transform>(peer);
    if (peer && peer->getDEF() == "B")
      nestedSink = std::dynamic_pointer_cast<Transform>(peer);
  }
  check(driver && primary && ordinary && nestedSink && external,
        "metadata: scene and nested PROTO endpoints expanded");
  if (!(driver && primary && ordinary && nestedSink && external)) return;
  X3DExecutionContext ctx;
  auto bridge = buildRoutes(doc.scene, ctx);
  check(bridge.routesAdded == 4 && bridge.rejected.empty(),
        "metadata: inherited PROTO source/sink aliases bridge at both scopes");
  auto metadata = createX3DNode("MetadataString");
  check(!!metadata, "metadata: MetadataString value constructed");
  if (!metadata) return;
  ctx.postEvent(driver.get(), "metadata_changed", std::any(SFNode{metadata}));
  ctx.tick(0.0);
  check(primary->getMetadata() == metadata && ordinary->getMetadata() == metadata &&
            nestedSink->getMetadata() == metadata &&
            external->getMetadata() == metadata,
        "metadata: event reaches outer primary, body ROUTEs, and scene sink");
}

void testSceneProtoToProtoRoute() {
  const char *xml = R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='Mover'><ProtoInterface>
<field name='shift' type='SFVec3f' accessType='inputOutput' value='0 0 0'/>
</ProtoInterface><ProtoBody><Transform><IS>
<connect nodeField='translation' protoField='shift'/>
</IS></Transform></ProtoBody></ProtoDeclare>
<ProtoInstance name='Mover' DEF='P1'/><ProtoInstance name='Mover' DEF='P2'/>
<ROUTE fromNode='P1' fromField='shift_changed' toNode='P2' toField='set_shift'/>
</Scene></X3D>)";
  auto doc = x3d::codec::parseDocument(xml);
  auto first = std::dynamic_pointer_cast<Transform>(doc.scene.resolve("P1"));
  auto second = std::dynamic_pointer_cast<Transform>(doc.scene.resolve("P2"));
  check(first && second && first != second,
        "scene PROTO-to-PROTO: two private primaries expanded");
  if (!(first && second)) return;
  X3DExecutionContext ctx;
  auto bridge = buildRoutes(doc.scene, ctx);
  check(bridge.routesAdded == 1 && bridge.rejected.empty(),
        "scene PROTO-to-PROTO: both nominal interface aliases accepted");
  ctx.postEvent(first.get(), "translation_changed", std::any(SFVec3f{2, 3, 4}));
  ctx.tick(0.0);
  check(veq(tr(second), 2, 3, 4),
        "scene PROTO-to-PROTO: event reaches second declared interface");
}

} // namespace

// §4.4.2.2: an inputOutput field zzz is addressable as set_zzz (sink) and
// zzz_changed (source). Document ROUTEs using the aliases must wire and
// deliver; the aliases are side-specific.
void testInputOutputAliases() {
  auto clock = std::make_shared<TimeSensor>();
  auto interp = std::make_shared<PositionInterpolator>();
  interp->setKey({0, 1});
  interp->setKeyValue({{0, 0, 0}, {2, 0, 0}});
  auto mover = std::make_shared<Transform>();
  auto follower = std::make_shared<Transform>();
  Scene scene;
  scene.define("Clock", clock);
  scene.define("Path", interp);
  scene.define("Mover", mover);
  scene.define("Follower", follower);
  scene.routes.emplace_back("Path", "value_changed", "Mover", "set_translation");
  scene.routes.emplace_back("Mover", "translation_changed", "Follower", "set_translation");
  // index 2: set_ is input-only; not a source.
  scene.routes.emplace_back("Mover", "set_translation", "Follower", "translation");
  // index 3: _changed is output-only; not a sink.
  scene.routes.emplace_back("Path", "value_changed", "Follower", "translation_changed");
  X3DExecutionContext ctx;
  BridgeResult r = buildRoutes(scene, ctx);
  check(r.routesAdded == 2, "set_/_changed alias routes added");
  check(r.rejected.size() == 2, "wrong-side aliases rejected");
  ctx.postEvent(interp.get(), "value_changed", std::any(SFVec3f{1, 0, 0}));
  ctx.tick(0.0);
  check(veq(tr(mover), 1, 0, 0), "set_translation alias delivers");
  check(veq(tr(follower), 1, 0, 0), "translation_changed alias fans out");
}

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
  testNestedProtoBodyRoutes();
  testNestedFirstPrimaryAndEventIsAliases();
  testNestedProtoRouteValidation();
  testInheritedProtoMetadataRoutes();
  testSceneProtoToProtoRoute();
  testInputOutputAliases();

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    return 1;
  }
  std::cout << "all scene-bridge tests passed\n";
  return 0;
}
