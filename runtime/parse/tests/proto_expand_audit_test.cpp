// proto_expand_audit_test.cpp
//
// AUD-PROTO-EXP audit regression: PROTO expansion + IS routing + depth guard.
//
// Covers all 8 risk areas from the audit spec:
//   1. initializeOnly IS does value substitution; ProtoField value visible on instance
//   2. inputOnly IS creates a redirect (route from external source into interface)
//   3. inputOutput IS creates a bidirectional redirect (both set_xxx and xxx_changed)
//   4. Self-recursive PROTO (body instantiates itself) hits depth guard, no hang
//   5. EXTERNPROTO with local file URL resolves and expands
//   6. Clone preserves event-only semantics (inputOnly stays writable; initializeOnly stays constant)
//   7. PROTO body USE resolves to body-local instance, not scene-global
//   8. PROTO route redirect: invalid-direction / type-invalid redirected edges are rejected
//      (the flagged concern in BACKLOG.md §CDC note — without the fix the redirect
//       path bypasses X3DRoute.hpp::validateRoute)
//
// The depth-guard, EXTERNPROTO, and redirect-validation tests are unique to this
// audit; the rest overlap (intentionally) with proto_expand_test.cpp /
// scene_bridge_test.cpp to lock down end-to-end behavior in one place.

#include "x3d/nodes/BooleanTrigger.hpp"
#include "Encoding.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DParse.hpp"
#include "X3DSceneBridge.hpp"

#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DProtoExpand.hpp"
#include "x3d/core/X3DReflection.hpp"

#include <any>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;
namespace xn = x3d::nodes;
using x3d::codec::noopProtoResolver;

static int failures = 0;
static void check(bool cond, const std::string &what) {
  if (!cond) {
    std::cerr << "FAIL: " << what << "\n";
    ++failures;
  } else {
    std::cout << "ok: " << what << "\n";
  }
}

static const FieldInfo *fieldByName(const X3DNode &n, const std::string &name) {
  for (const auto &f : n.fields()) if (f.x3dName == name) return &f;
  return nullptr;
}

// Find an expanded primary by its instance DEF, walking the protoRedirects
// keys (each expanded primary is registered as a redirect-map key when its
// instance had at least one event-interface IS).
static X3DNode *primaryOf(const Scene &scene, const std::string &def) {
  for (auto &kv : scene.protoRedirects) {
    X3DNode *n = kv.first;
    if (n && n->getDEF() == def) return n;
  }
  // Fallback: scan root nodes (works when no redirects were registered, e.g.
  // for initializeOnly-only interfaces).
  for (auto &n : scene.rootNodes) {
    if (n && n->getDEF() == def) return n.get();
  }
  return nullptr;
}

// =============================================================================
// (1) initializeOnly IS does VALUE substitution; ProtoField value visible on
//     instance. The clone + forward-set path lands the override on the cloned
//     body field.
// =============================================================================
static void initOnlyValueForwardTest() {
  std::cerr << "\n[1] initializeOnly IS value substitution\n";

  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Cube";
  ProtoField pf;
  pf.name = "size"; pf.type = X3DFieldType::SFVec3f;
  pf.access = AccessType::InitializeOnly;
  pf.value = std::any(SFVec3f{2.f, 2.f, 2.f});
  decl->interface.push_back(pf);

  auto box = createX3DNode("Box");
  decl->body.nodes.push_back(box);
  decl->body.isConnections.push_back({box, "size", "size"});

  x3d::runtime::ProtoInstance inst;
  inst.name = "Cube"; inst.declaration = decl;
  ProtoFieldValue fv; fv.name = "size";
  fv.value = std::any(SFVec3f{5.f, 5.f, 5.f});
  inst.fieldValues.push_back(fv);

  Scene scene; ExpandGuard guard; std::vector<ProtoWarning> w;
  auto primary = expandInstance(inst, scene, noopProtoResolver, "", guard, w);

  check(primary != nullptr, "initializeOnly: primary clone produced");
  check(primary && primary->nodeTypeName() == "Box",
        "initializeOnly: primary is Box");
  auto sz = std::any_cast<SFVec3f>(
      fieldByName(*primary, "size")->get(*primary));
  check(sz.x == 5.f && sz.y == 5.f && sz.z == 5.f,
        "initializeOnly: override 5,5,5 lands on cloned body field");
  check(w.empty(), "initializeOnly: no warnings on clean override");
}

static void initOnlyDefaultTest() {
  std::cerr << "\n[1b] initializeOnly default used when no override\n";

  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Mat";
  ProtoField pf;
  pf.name = "diffuseColor"; pf.type = X3DFieldType::SFColor;
  pf.access = AccessType::InitializeOnly;
  pf.value = std::any(SFColor{0.5f, 0.5f, 0.5f});
  decl->interface.push_back(pf);

  auto mat = createX3DNode("Material");
  decl->body.nodes.push_back(mat);
  decl->body.isConnections.push_back({mat, "diffuseColor", "diffuseColor"});

  x3d::runtime::ProtoInstance inst;
  inst.name = "Mat"; inst.declaration = decl;

  Scene scene; ExpandGuard guard; std::vector<ProtoWarning> w;
  auto primary = expandInstance(inst, scene, noopProtoResolver, "", guard, w);

  check(primary != nullptr, "initializeOnly-default: primary clone produced");
  auto col = std::any_cast<SFColor>(
      fieldByName(*primary, "diffuseColor")->get(*primary));
  check(col.r == 0.5f && col.g == 0.5f && col.b == 0.5f,
        "initializeOnly-default: proto default 0.5 visible on clone");
}

// =============================================================================
// (2) inputOnly IS creates a redirect. Expansion populates protoRedirects and
//     the bridge wires an external source through the interface to the body's
//     inputOnly event handler.
// =============================================================================
static void inputOnlyRedirectPopulatedTest() {
  std::cerr << "\n[2] inputOnly IS populates protoRedirects\n";

  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Btn";
  ProtoField trig;
  trig.name = "trig"; trig.type = X3DFieldType::SFTime;
  trig.access = AccessType::InputOnly;
  decl->interface.push_back(trig);

  auto bt = createX3DNode("BooleanTrigger");
  decl->body.nodes.push_back(bt);
  // IS: body set_triggerTime <-> interface trig
  decl->body.isConnections.push_back({bt, "set_triggerTime", "trig"});

  x3d::runtime::ProtoInstance inst;
  inst.name = "Btn"; inst.declaration = decl; inst.DEF = "B";
  Scene scene; ExpandGuard guard; std::vector<ProtoWarning> w;
  auto primary = expandInstance(inst, scene, noopProtoResolver, "", guard, w);

  check(primary && primary->nodeTypeName() == "BooleanTrigger",
        "inputOnly: primary is BooleanTrigger");
  check(scene.protoRedirects.count(primary.get()) == 1,
        "inputOnly: primary registered in protoRedirects");
  auto &byField = scene.protoRedirects[primary.get()];
  check(byField.count("trig") == 1,
        "inputOnly: 'trig' interface field has one redirect target");
  check(byField["trig"].size() == 1,
        "inputOnly: one body endpoint per IS");
  check(byField["trig"][0].targetField == "set_triggerTime",
        "inputOnly: body target is set_triggerTime");
  check(byField["trig"][0].targetNode &&
        byField["trig"][0].targetNode->nodeTypeName() == "BooleanTrigger",
        "inputOnly: body target node is the cloned BooleanTrigger");
}

static void inputOnlyRedirectBridgesTest() {
  std::cerr << "\n[2b] inputOnly IS redirects external ROUTE through bridge\n";

  // Same interface/body mapping as 2a (trig -> set_triggerTime, SFTime). External
  // source: a TouchSensor that emits SFTime on touchTime — types match (both
  // SFTime), direction is valid (outputOnly source -> inputOnly sink). The
  // redirect path must add this route, NOT reject the interface field as
  // "unknown".
  Scene scene;
  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Btn";
  ProtoField trig;
  trig.name = "trig"; trig.type = X3DFieldType::SFTime;
  trig.access = AccessType::InputOnly;
  decl->interface.push_back(trig);

  auto bt = createX3DNode("BooleanTrigger");
  decl->body.nodes.push_back(bt);
  decl->body.isConnections.push_back({bt, "set_triggerTime", "trig"});
  scene.protoDeclarations.push_back(decl);

  // TouchSensor.touchTime is SFTime OutputOnly — types match the interface.
  auto ts = createX3DNode("TouchSensor"); ts->setDEF("TS");
  scene.addRootNode(ts);

  x3d::runtime::ProtoInstance inst;
  inst.name = "Btn"; inst.declaration = decl; inst.DEF = "B";
  scene.protoInstances.push_back(inst);

  std::vector<ProtoWarning> w;
  expandScene(scene, noopProtoResolver, "", w);

  scene.routes.push_back(
      Route{"TS", "touchTime", "B", "trig"});

  X3DExecutionContext ctx;
  BridgeResult r = buildRoutes(scene, ctx);

  check(r.routesAdded >= 1,
        "inputOnly: external route through interface added via redirect");
  bool interfaceRejected = false;
  for (const auto &err : r.rejected) {
    if (err.reason.find("trig") != std::string::npos)
      interfaceRejected = true;
  }
  check(!interfaceRejected,
        "inputOnly: interface field NOT rejected as unknown");
}

// =============================================================================
// (3) inputOutput IS: expansion populates protoRedirects AND the body field
//     has both a write thunk (set_xxx) and read thunk (xxx_changed) — both
//     directions can be the target of an external ROUTE.
// =============================================================================
static void inputOutputRedirectTest() {
  std::cerr << "\n[3] inputOutput IS populates redirect + is bidirectional\n";

  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Srv";
  ProtoField color;
  color.name = "color"; color.type = X3DFieldType::SFColor;
  color.access = AccessType::InputOutput;
  decl->interface.push_back(color);

  auto mat = createX3DNode("Material");
  decl->body.nodes.push_back(mat);
  decl->body.isConnections.push_back({mat, "diffuseColor", "color"});

  x3d::runtime::ProtoInstance inst;
  inst.name = "Srv"; inst.declaration = decl; inst.DEF = "S";
  Scene scene; ExpandGuard guard; std::vector<ProtoWarning> w;
  auto primary = expandInstance(inst, scene, noopProtoResolver, "", guard, w);

  check(primary && primary->nodeTypeName() == "Material",
        "inputOutput: primary is Material");
  check(scene.protoRedirects[primary.get()].count("color") == 1,
        "inputOutput: 'color' interface field redirects to body diffuseColor");

  // The body field has the correct reflection access type on the clone.
  const FieldInfo *fi = fieldByName(*primary, "diffuseColor");
  check(fi && fi->access == AccessType::InputOutput,
        "inputOutput: cloned body field diffuseColor stays InputOutput");

  // The redirect target node IS the cloned Material primary.
  check(scene.protoRedirects[primary.get()]["color"][0].targetNode.get()
            == primary.get(),
        "inputOutput: redirect target node is the cloned primary");

  // Bridge an external source (TimeSensor's fraction SFFloat, here just for
  // direction check — values would need to be SFFloat-compatible; we only
  // verify that an unknown interface field name is redirected, not rejected).
  scene.routes.push_back(
      Route{"Ghost", "value_changed", "S", "color"}); // dangling source -> skip
  X3DExecutionContext ctx;
  BridgeResult r = buildRoutes(scene, ctx);
  // The dangling source is skipped silently (not counted, not rejected) — but
  // the interface field 'color' on S should still NOT appear in `rejected`.
  bool interfaceRejected = false;
  for (const auto &err : r.rejected) {
    if (err.reason.find("color") != std::string::npos)
      interfaceRejected = true;
  }
  check(!interfaceRejected,
        "inputOutput: interface 'color' not rejected (dangling source only)");
}

// =============================================================================
// (4) Self-recursive PROTO (body instantiates itself) hits the depth guard.
//     Spec §4.4.4.3 mandates that self-referential PROTOS not loop forever;
//     our defensive cap is ExpandGuard::maxDepth = 32 (per expansion stack).
// =============================================================================
static void selfRecursiveDepthGuardTest() {
  std::cerr << "\n[4] self-recursive PROTO hits depth guard\n";

  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Self";
  auto box = createX3DNode("Box");
  decl->body.nodes.push_back(box);

  // A nested instance INSIDE the body that instantiates the same PROTO.
  x3d::runtime::ProtoInstance nested;
  nested.name = "Self";
  // parent weak_ptr left null => body-root splice via primary.children (Box
  // doesn't have a children slot, but expansion tolerates attachToParent
  // failure on a wrong slot — the depth-guard test doesn't need to actually
  // splice the recursion result; we only verify that the recursion terminates).
  decl->body.nestedInstances.push_back(nested);

  Scene scene;
  scene.protoDeclarations.push_back(decl);

  x3d::runtime::ProtoInstance inst;
  inst.name = "Self"; inst.declaration = decl; inst.DEF = "S";
  scene.protoInstances.push_back(inst);

  std::vector<ProtoWarning> w;
  // Bounded: this MUST return, not infinite-loop.
  expandScene(scene, noopProtoResolver, "", w);

  bool sawRecursionLimit = false;
  for (const auto &warn : w) {
    if (warn.kind == ProtoWarning::Kind::RecursionLimit) sawRecursionLimit = true;
  }
  check(sawRecursionLimit,
        "self-recursive: RecursionLimit warning recorded (depth cap fired)");
  // The top-level instance should still produce a Box primary — the top
  // instance itself is not at the cap; only its nested recursion is.
  bool foundBox = false;
  for (auto &n : scene.rootNodes) {
    if (n && n->nodeTypeName() == "Box") foundBox = true;
  }
  check(foundBox, "self-recursive: top-level primary is a Box");
}

static void depthGuardPerStackTest() {
  std::cerr << "\n[4b] depth guard is per-expansion-stack (resets per top-level)\n";

  // Two unrelated, non-recursive PROTOs each expanded as their own top-level
  // instance — each gets its own ExpandGuard, neither fires the cap.
  auto declA = std::make_shared<ProtoDeclaration>();
  declA->name = "A"; declA->body.nodes.push_back(createX3DNode("Box"));
  auto declB = std::make_shared<ProtoDeclaration>();
  declB->name = "B"; declB->body.nodes.push_back(createX3DNode("Sphere"));

  Scene scene;
  scene.protoDeclarations.push_back(declA);
  scene.protoDeclarations.push_back(declB);

  x3d::runtime::ProtoInstance a; a.name = "A"; a.declaration = declA;
  a.DEF = "IA"; scene.protoInstances.push_back(a);
  x3d::runtime::ProtoInstance b; b.name = "B"; b.declaration = declB;
  b.DEF = "IB"; scene.protoInstances.push_back(b);

  std::vector<ProtoWarning> w;
  expandScene(scene, noopProtoResolver, "", w);
  check(w.empty(),
        "depth-guard: two non-recursive top-level instances -> no warnings");
  int rootCount = 0;
  for (auto &n : scene.rootNodes) if (n) ++rootCount;
  check(rootCount == 2,
        "depth-guard: both top-level instances produce a root node");
}

// =============================================================================
// (5) EXTERNPROTO with local file URL resolves and expands. We test via
//     parseFile(front door) so the resolver seam is the production one.
// =============================================================================
static void externLocalFileResolveTest() {
  std::cerr << "\n[5] EXTERNPROTO local-file URL resolves\n";

  std::string dataDir = "runtime/parse/tests/data/proto";
  if (const char *env = std::getenv("X3D_TEST_DATA_DIR")) dataDir = env;
  auto doc = x3d::codec::parseFile(dataDir + "/main.x3d");

  bool foundBox = false;
  for (auto &n : doc.scene.rootNodes) {
    if (n && n->nodeTypeName() == "Box") foundBox = true;
  }
  check(foundBox,
        "EXTERNPROTO: sibling-file EXTERNPROTO expands into Box primary");
  check(doc.protoWarnings.empty(),
        "EXTERNPROTO: no warnings on resolved sibling-file URL");
}

static void externLocalFileRelativeUrlTest() {
  std::cerr << "\n[5b] EXTERNPROTO relative URL resolves against baseUrl\n";

  // Direct programmatic: pass baseUrl explicitly so we exercise the relative
  // resolution path independent of parseFile's path-derivation logic.
  auto ext = std::make_shared<ExternProtoDeclaration>();
  ext->name = "ExtBox"; ext->url = {"shapes.x3d#ExtBox"};

  x3d::runtime::ProtoInstance inst;
  inst.name = "ExtBox"; inst.externDeclaration = ext;

  std::string dataDir = "runtime/parse/tests/data/proto";
  if (const char *env = std::getenv("X3D_TEST_DATA_DIR")) dataDir = env;

  Scene scene; ExpandGuard guard; std::vector<ProtoWarning> w;
  auto primary = expandInstance(inst, scene, noopProtoResolver, dataDir, guard, w);

  // noopProtoResolver returns null, so this should warn and yield no primary.
  check(!primary, "EXTERNPROTO: noop resolver yields no primary");
  check(w.size() == 1 && w[0].kind == ProtoWarning::Kind::UnresolvedExtern,
        "EXTERNPROTO: UnresolvedExtern warning recorded");

  // Now exercise the real local-file resolver via parseDocument.
  const char *x3dDoc =
      "<X3D version='4.0'><Scene>"
      "<ExternProtoDeclare name='ExtBox' url='shapes.x3d#ExtBox'/>"
      "<ProtoInstance name='ExtBox'/>"
      "</Scene></X3D>";
  auto doc = x3d::codec::parseDocument(x3dDoc, x3d::codec::Encoding::XML, dataDir);
  bool got = false;
  for (auto &n : doc.scene.rootNodes) {
    if (n && n->nodeTypeName() == "Box") got = true;
  }
  check(got,
        "EXTERNPROTO: parseDocument with explicit baseUrl resolves relative URL");

  // Also test the URL with a #fragment that doesn't match any PROTO in the
  // target file — should be UnresolvedExtern.
  const char *x3dBad =
      "<X3D version='4.0'><Scene>"
      "<ExternProtoDeclare name='NoSuch' url='shapes.x3d#NoSuch'/>"
      "<ProtoInstance name='NoSuch'/>"
      "</Scene></X3D>";
  auto docBad = x3d::codec::parseDocument(x3dBad, x3d::codec::Encoding::XML,
                                          dataDir);
  bool badWarned = false;
  for (const auto &warn : docBad.protoWarnings) {
    if (warn.kind == ProtoWarning::Kind::UnresolvedExtern) badWarned = true;
  }
  check(badWarned,
        "EXTERNPROTO: unresolved #fragment warns (no such PROTO in target)");
}

// =============================================================================
// (6) Clone preserves event-only semantics: an inputOnly event field on the
//     body remains inputOnly on the clone (with a write thunk), and an
//     initializeOnly field stays constant after clone.
// =============================================================================
static void clonePreservesInputOnlyTest() {
  std::cerr << "\n[6] clone preserves inputOnly event-field semantics\n";

  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Btn";
  auto bt = createX3DNode("BooleanTrigger");
  decl->body.nodes.push_back(bt);
  decl->body.isConnections.push_back({bt, "set_triggerTime", "trig"});

  x3d::runtime::ProtoInstance inst;
  inst.name = "Btn"; inst.declaration = decl; inst.DEF = "B";
  Scene scene; ExpandGuard guard; std::vector<ProtoWarning> w;
  auto primary = expandInstance(inst, scene, noopProtoResolver, "", guard, w);

  check(primary && primary->nodeTypeName() == "BooleanTrigger",
        "clone: primary is BooleanTrigger");

  const FieldInfo *setTime = fieldByName(*primary, "set_triggerTime");
  check(setTime != nullptr, "clone: set_triggerTime field present on clone");
  check(setTime && setTime->access == AccessType::InputOnly,
        "clone: set_triggerTime access stays InputOnly on clone");
  check(setTime && setTime->set != nullptr,
        "clone: set_triggerTime still has a write thunk (events deliverable)");

  // Register a handler on the clone and post an event via the bridge — the
  // handler must fire, proving the cloned body node is fully wired.
  auto raw = primary.get();
  // Cast to register handler.
  auto btPtr = std::dynamic_pointer_cast<class xn::BooleanTrigger>(primary);
  check(btPtr != nullptr, "clone: primary downcasts to BooleanTrigger");
  if (btPtr) {
    int fired = 0;
    btPtr->setOnSet_triggerTimeHandler(
        [&fired](const SFTime &) { ++fired; });
    X3DExecutionContext ctx;
    ctx.postEvent(raw, "set_triggerTime", std::any(SFTime{1.0}));
    ctx.process();
    check(fired == 1,
          "clone: posted event reaches cloned body's inputOnly handler");
  }
}

static void clonePreservesInitializeOnlyTest() {
  std::cerr << "\n[6b] clone keeps initializeOnly field constant\n";

  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Cube";
  ProtoField pf;
  pf.name = "size"; pf.type = X3DFieldType::SFVec3f;
  pf.access = AccessType::InitializeOnly;
  pf.value = std::any(SFVec3f{3.f, 3.f, 3.f});
  decl->interface.push_back(pf);

  auto box = createX3DNode("Box");
  decl->body.nodes.push_back(box);
  decl->body.isConnections.push_back({box, "size", "size"});

  x3d::runtime::ProtoInstance inst;
  inst.name = "Cube"; inst.declaration = decl;
  Scene scene; ExpandGuard guard; std::vector<ProtoWarning> w;
  auto primary = expandInstance(inst, scene, noopProtoResolver, "", guard, w);

  const FieldInfo *sz = fieldByName(*primary, "size");
  check(sz && sz->access == AccessType::InitializeOnly,
        "clone: size remains InitializeOnly on the clone");
  auto got = std::any_cast<SFVec3f>(sz->get(*primary));
  check(got.x == 3.f && got.y == 3.f && got.z == 3.f,
        "clone: proto-default value lands on the clone (constant)");
}

// =============================================================================
// (7) PROTO body USE resolves to body-local instance, not scene-global.
//     A node with DEF inside a ProtoBody and another node using it via USE
//     must share identity at the BODY level after expansion — and that DEF
//     name must NOT be promoted into scene.defs (proto-local DEF scope).
// =============================================================================
static void bodyUseIsLocalTest() {
  std::cerr << "\n[7] PROTO body USE is proto-local\n";

  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Body";

  // Body node 1: a Shape with DEF "S".
  auto shape = createX3DNode("Shape");
  shape->setDEF("S");
  decl->body.nodes.push_back(shape);

  // Body node 2: a Group whose children reference the Shape via the SAME
  // shared_ptr (USE semantics at the body level — deepClone preserves
  // pointer identity via the cloneMap).
  auto group = createX3DNode("Group");
  group->setDEF("G");
  const FieldInfo *kids = fieldByName(*group, "children");
  kids->set(*group, std::any(std::vector<std::shared_ptr<X3DNode>>{shape}));
  decl->body.nodes.push_back(group);

  // Body-internal ROUTE naming the DEFs — proves the localDefs map resolves
  // the cloned Shape/Group names AND that the Group.clone's addChildren field
  // (the route sink) is fed from the cloned Shape pointer.
  decl->body.routes.push_back(
      Route{"S", "geometry", "G", "addChildren"});

  x3d::runtime::ProtoInstance inst;
  inst.name = "Body"; inst.declaration = decl; inst.DEF = "OUTER";
  Scene scene; ExpandGuard guard; std::vector<ProtoWarning> w;
  auto primary = expandInstance(inst, scene, noopProtoResolver, "", guard, w);

  // Primary is the FIRST body node per X3D spec — a clone of Shape. The
  // expansion overwrites its DEF with the instance DEF (so the body-DEF
  // 'S' is no longer the primary's DEF); what matters here is that the
  // body-level USE identity (Group.children[0] == cloned Shape) is preserved
  // and that the body DEF name does not leak into scene.defs.
  check(primary && primary->nodeTypeName() == "Shape",
        "body-USE: primary is Shape (first body node)");

  // The proto-local DEF "S" must NOT be promoted to scene.defs.
  check(scene.defs.count("S") == 0,
        "body-USE: body DEF 'S' NOT promoted to scene.defs (proto-local)");
  check(scene.defs.count("G") == 0,
        "body-USE: body DEF 'G' NOT promoted to scene.defs (proto-local)");

  // The body route must be pre-resolved via localDefs.
  check(scene.resolvedProtoRoutes.size() == 1,
        "body-USE: body route pre-resolved via localDefs");
  const auto &r = scene.resolvedProtoRoutes[0];
  check(r.from && r.from.get() == primary.get(),
        "body-USE: route source resolves to the cloned Shape (USE identity)");
  check(r.to && r.to->nodeTypeName() == "Group",
        "body-USE: route sink resolves to the cloned Group");
}

// =============================================================================
// (8) PROTO route redirect VALIDATION GAP (the BACKLOG.md concern).
//     The redirect path in X3DSceneBridge.hpp registers edges WITHOUT the
//     access-direction/type validation the normal route path enforces. Two
//     scenarios must be rejected:
//       (a) interface field is inputOnly but used as a ROUTE source — the
//           body target is an inputOnly event handler, which cannot emit;
//           redirect must NOT silently wire it.
//       (b) interface field is outputOnly but used as a ROUTE sink — the
//           body target is an outputOnly event, which cannot receive.
//     And the type-mismatch case (c): interface field type != other-endpoint
//     type — must also be rejected so the cascade isn't wired with a type
//     mismatch that the cascade would later drop or worse.
// =============================================================================
static void redirectRejectsInputOnlyAsSourceTest() {
  std::cerr << "\n[8a] redirect: inputOnly interface as source -> REJECTED\n";

  Scene scene;
  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Btn";
  ProtoField trig;
  trig.name = "trig"; trig.type = X3DFieldType::SFTime;
  trig.access = AccessType::InputOnly;
  decl->interface.push_back(trig);

  auto bt = createX3DNode("BooleanTrigger");
  decl->body.nodes.push_back(bt);
  decl->body.isConnections.push_back({bt, "set_triggerTime", "trig"});
  scene.protoDeclarations.push_back(decl);

  x3d::runtime::ProtoInstance inst;
  inst.name = "Btn"; inst.declaration = decl; inst.DEF = "B";
  scene.protoInstances.push_back(inst);

  std::vector<ProtoWarning> w;
  expandScene(scene, noopProtoResolver, "", w);

  // Bad route: try to use the inputOnly interface 'trig' as a SOURCE
  // (B.trig -> SinkNode.something). The redirect target is body.set_triggerTime
  // which is inputOnly — the route is direction-invalid.
  auto sink = createX3DNode("BooleanToggle"); sink->setDEF("SNK");
  scene.addRootNode(sink);
  scene.routes.push_back(Route{"B", "trig", "SNK", "set_boolean"});

  X3DExecutionContext ctx;
  BridgeResult r = buildRoutes(scene, ctx);

  // The route must NOT be added. The defining property: no redirected edge
  // wires inputOnly-as-source.
  bool seenTrigRejected = false;
  for (const auto &err : r.rejected) {
    if (err.reason.find("trig") != std::string::npos) seenTrigRejected = true;
  }
  check(seenTrigRejected,
        "inputOnly-as-source: rejected with diagnostic naming 'trig'");
  // The diagnostic must call out the direction failure (outputOnly/inputOutput
  // required for source), so users can tell what to fix.
  bool mentionsDirection = false;
  for (const auto &err : r.rejected) {
    if (err.reason.find("source") != std::string::npos &&
        err.reason.find("outputOnly") != std::string::npos) {
      mentionsDirection = true;
    }
  }
  check(mentionsDirection,
        "inputOnly-as-source: diagnostic mentions 'source' / 'outputOnly'");

  // And concretely: ctx.graph() has no source edge keyed on set_triggerTime.
  // The cloned BooleanTrigger is reachable via scene.protoRedirects[B].
  X3DNode *bodyNode = scene.protoRedirects[primaryOf(scene, "B")]["trig"]
                          [0].targetNode.get();
  auto broken = ctx.graph().sinks({bodyNode, "set_triggerTime"});
  check(broken.empty(),
        "inputOnly-as-source: no edge wired from set_triggerTime");
}

static void redirectRejectsOutputOnlyAsSinkTest() {
  std::cerr << "\n[8b] redirect: outputOnly interface as sink -> REJECTED\n";

  Scene scene;
  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Src";
  ProtoField out;
  out.name = "out"; out.type = X3DFieldType::SFBool;
  out.access = AccessType::OutputOnly;
  decl->interface.push_back(out);

  auto bt = createX3DNode("BooleanTrigger");
  decl->body.nodes.push_back(bt);
  decl->body.isConnections.push_back({bt, "triggerTrue", "out"});
  scene.protoDeclarations.push_back(decl);

  x3d::runtime::ProtoInstance inst;
  inst.name = "Src"; inst.declaration = decl; inst.DEF = "S";
  scene.protoInstances.push_back(inst);

  std::vector<ProtoWarning> w;
  expandScene(scene, noopProtoResolver, "", w);

  // Bad route: try to write INTO the outputOnly interface 'out'
  // (Src.out <- SRC.set_something). Body target is triggerTrue (outputOnly).
  auto src = createX3DNode("BooleanToggle"); src->setDEF("SRC");
  scene.addRootNode(src);
  scene.routes.push_back(Route{"SRC", "set_boolean", "S", "out"});

  X3DExecutionContext ctx;
  BridgeResult r = buildRoutes(scene, ctx);

  bool seenOutRejected = false;
  for (const auto &err : r.rejected) {
    if (err.reason.find("'out'") != std::string::npos ||
        err.reason.find("out") != std::string::npos) seenOutRejected = true;
  }
  check(seenOutRejected,
        "outputOnly-as-sink: rejected with diagnostic naming 'out'");

  // The defect signature (before fix): ctx.graph() has an edge
  // {SRC.set_boolean} -> {body.triggerTrue}. Verify it is NOT present.
  X3DNode *bodyNode = scene.protoRedirects[primaryOf(scene, "S")]["out"]
                          [0].targetNode.get();
  X3DNode *srcRaw = src.get();
  auto wired = ctx.graph().sinks({srcRaw, "set_boolean"});
  bool wiredToTriggerTrue = false;
  for (const auto &sink : wired) {
    if (sink.node == bodyNode && sink.field == "triggerTrue") {
      wiredToTriggerTrue = true;
    }
  }
  check(!wiredToTriggerTrue,
        "outputOnly-as-sink: no edge wired to body.triggerTrue");
}

static void redirectRejectsTypeMismatchTest() {
  std::cerr << "\n[8c] redirect: type mismatch -> REJECTED\n";

  // Interface 'trig' is SFTime (inputOnly). External source is SFFloat
  // (TimeSensor.fraction). The body target is set_triggerTime (SFTime);
  // but the source is SFFloat — direction is fine (inputOnly as sink OK),
  // type is NOT. Redirect must reject on type mismatch, mirroring the
  // normal route path's rule (X3D 19775-1 §4.4.8.2: no implicit field-type
  // coercion across a ROUTE).
  Scene scene;
  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Btn";
  ProtoField trig;
  trig.name = "trig"; trig.type = X3DFieldType::SFTime;
  trig.access = AccessType::InputOnly;
  decl->interface.push_back(trig);

  auto bt = createX3DNode("BooleanTrigger");
  decl->body.nodes.push_back(bt);
  decl->body.isConnections.push_back({bt, "set_triggerTime", "trig"});
  scene.protoDeclarations.push_back(decl);

  x3d::runtime::ProtoInstance inst;
  inst.name = "Btn"; inst.declaration = decl; inst.DEF = "B";
  scene.protoInstances.push_back(inst);

  std::vector<ProtoWarning> w;
  expandScene(scene, noopProtoResolver, "", w);

  auto ts = createX3DNode("TimeSensor"); ts->setDEF("TS");
  scene.addRootNode(ts);
  // TS.fraction_changed is SFFloat -> B.trig (SFTime): direction OK, type NOT.
  scene.routes.push_back(Route{"TS", "fraction_changed", "B", "trig"});

  X3DExecutionContext ctx;
  BridgeResult r = buildRoutes(scene, ctx);

  // The type-mismatch must be rejected (preferred) or at least not wired.
  bool seenTypeRejection = false;
  for (const auto &err : r.rejected) {
    if (err.reason.find("type") != std::string::npos ||
        err.reason.find("mismatch") != std::string::npos) {
      seenTypeRejection = true;
    }
  }
  // The defect signature (before fix): r.routesAdded == 1 because the
  // redirect path silently wires the type-mismatched edge. Assert the FIX.
  check(seenTypeRejection || r.routesAdded == 0,
        "type-mismatch: rejected OR not wired (redirect validation active)");
  check(seenTypeRejection,
        "type-mismatch: diagnostic names the type mismatch");
}

// =============================================================================
// (8d) A VALID redirect (inputOnly interface as sink with matching types) still
//      works after the fix — sanity guard that the validation didn't regress
//      the legitimate redirect path.
// =============================================================================
static void validRedirectStillWorksTest() {
  std::cerr << "\n[8d] redirect: valid inputOnly sink -> redirect path still wires\n";

  Scene scene;
  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Btn";
  ProtoField trig;
  trig.name = "trig"; trig.type = X3DFieldType::SFTime;
  trig.access = AccessType::InputOnly;
  decl->interface.push_back(trig);

  auto bt = createX3DNode("BooleanTrigger");
  decl->body.nodes.push_back(bt);
  decl->body.isConnections.push_back({bt, "set_triggerTime", "trig"});
  scene.protoDeclarations.push_back(decl);

  // Source: TouchSensor.touchTime (SFTime OutputOnly) -> interface trig.
  auto ts = createX3DNode("TouchSensor"); ts->setDEF("TS");
  scene.addRootNode(ts);

  x3d::runtime::ProtoInstance inst;
  inst.name = "Btn"; inst.declaration = decl; inst.DEF = "B";
  scene.protoInstances.push_back(inst);

  std::vector<ProtoWarning> w;
  expandScene(scene, noopProtoResolver, "", w);

  scene.routes.push_back(Route{"TS", "touchTime", "B", "trig"});

  X3DExecutionContext ctx;
  BridgeResult r = buildRoutes(scene, ctx);

  check(r.routesAdded == 1,
        "valid-redirect: one route added via redirect path");
  check(r.rejected.empty(),
        "valid-redirect: no rejections");

  // Confirm the wired edge: TS.touchTime -> body.set_triggerTime.
  X3DNode *bodyNode = scene.protoRedirects[primaryOf(scene, "B")]["trig"]
                          [0].targetNode.get();
  auto sinks = ctx.graph().sinks({ts.get(), "touchTime"});
  bool wiredToBody = false;
  for (const auto &sink : sinks) {
    if (sink.node == bodyNode && sink.field == "set_triggerTime") {
      wiredToBody = true;
    }
  }
  check(wiredToBody,
        "valid-redirect: edge wired from TS.touchTime to body.set_triggerTime");
}

// =============================================================================
// (9) IS access-type validation per Table 4.4 — invalid mappings warn and
//     are skipped rather than silently creating bad redirects / value forwards.
// =============================================================================
static void invalidIsMappingInitOnlyBodyToInputOnlyIfaceTest() {
  std::cerr << "\n[9a] IS Table 4.4: initOnly body -> inputOnly iface -> warn+skip\n";

  // Body: Box.size is initializeOnly. Interface: "trig" is inputOnly.
  // Table 4.4 says NO — this IS mapping is invalid.
  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "BadBox";
  ProtoField trig;
  trig.name = "trig"; trig.type = X3DFieldType::SFVec3f;
  trig.access = AccessType::InputOnly;
  decl->interface.push_back(trig);

  auto box = createX3DNode("Box");
  decl->body.nodes.push_back(box);
  decl->body.isConnections.push_back({box, "size", "trig"});

  x3d::runtime::ProtoInstance inst;
  inst.name = "BadBox"; inst.declaration = decl;
  Scene scene; ExpandGuard guard; std::vector<ProtoWarning> w;
  auto primary = expandInstance(inst, scene, noopProtoResolver, "", guard, w);

  check(primary != nullptr, "9a: expansion still succeeds despite bad IS");
  bool saw = false;
  for (const auto &warn : w) {
    if (warn.kind == ProtoWarning::Kind::InterfaceMismatch &&
        warn.detail.find("size IS trig") != std::string::npos) {
      saw = true;
    }
  }
  check(saw, "9a: InterfaceMismatch warning for initOnly->inputOnly IS");

  // No redirect must be registered for the bad mapping.
  auto it = scene.protoRedirects.find(primary.get());
  if (it != scene.protoRedirects.end()) {
    check(it->second.count("trig") == 0,
          "9a: no redirect registered for invalid IS mapping");
  }
}

static void invalidIsMappingInputOnlyBodyToInitOnlyIfaceTest() {
  std::cerr << "\n[9b] IS Table 4.4: inputOnly body -> initOnly iface -> warn+skip\n";

  // Body: BooleanTrigger.set_triggerTime is inputOnly.
  // Interface: "size" is initializeOnly. Table 4.4 says NO.
  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "BadBtn";
  ProtoField sz;
  sz.name = "size"; sz.type = X3DFieldType::SFVec3f;
  sz.access = AccessType::InitializeOnly;
  sz.value = std::any(SFVec3f{2.f, 2.f, 2.f});
  decl->interface.push_back(sz);

  auto bt = createX3DNode("BooleanTrigger");
  decl->body.nodes.push_back(bt);
  decl->body.isConnections.push_back({bt, "set_triggerTime", "size"});

  x3d::runtime::ProtoInstance inst;
  inst.name = "BadBtn"; inst.declaration = decl;
  Scene scene; ExpandGuard guard; std::vector<ProtoWarning> w;
  auto primary = expandInstance(inst, scene, noopProtoResolver, "", guard, w);

  check(primary != nullptr, "9b: expansion still succeeds despite bad IS");
  bool saw = false;
  for (const auto &warn : w) {
    if (warn.kind == ProtoWarning::Kind::InterfaceMismatch &&
        warn.detail.find("set_triggerTime IS size") != std::string::npos) {
      saw = true;
    }
  }
  check(saw, "9b: InterfaceMismatch warning for inputOnly->initOnly IS");

  // The inputOnly body field must NOT receive the initOnly interface value.
  const FieldInfo *setTime = fieldByName(*primary, "set_triggerTime");
  check(setTime != nullptr, "9b: set_triggerTime field present on clone");
  if (setTime && setTime->get) {
    // inputOnly fields may not have a get; if they do, value must not be the
    // interface default.
    try {
      auto val = std::any_cast<SFVec3f>(setTime->get(*primary));
      check(val.x != 2.f, "9b: inputOnly body field did NOT receive initOnly default");
    } catch (...) {
      // get threw or returned non-SFVec3f — acceptable for inputOnly.
    }
  }
}

static void invalidIsMappingOutputOnlyBodyToInputOnlyIfaceTest() {
  std::cerr << "\n[9c] IS Table 4.4: outputOnly body -> inputOnly iface -> warn+skip\n";

  // Body: BooleanTrigger.triggerTrue is outputOnly.
  // Interface: "trig" is inputOnly. Table 4.4 says NO.
  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "BadSrc";
  ProtoField trig;
  trig.name = "trig"; trig.type = X3DFieldType::SFTime;
  trig.access = AccessType::InputOnly;
  decl->interface.push_back(trig);

  auto bt = createX3DNode("BooleanTrigger");
  decl->body.nodes.push_back(bt);
  decl->body.isConnections.push_back({bt, "triggerTrue", "trig"});

  x3d::runtime::ProtoInstance inst;
  inst.name = "BadSrc"; inst.declaration = decl;
  Scene scene; ExpandGuard guard; std::vector<ProtoWarning> w;
  auto primary = expandInstance(inst, scene, noopProtoResolver, "", guard, w);

  check(primary != nullptr, "9c: expansion still succeeds despite bad IS");
  bool saw = false;
  for (const auto &warn : w) {
    if (warn.kind == ProtoWarning::Kind::InterfaceMismatch &&
        warn.detail.find("triggerTrue IS trig") != std::string::npos) {
      saw = true;
    }
  }
  check(saw, "9c: InterfaceMismatch warning for outputOnly->inputOnly IS");

  auto it = scene.protoRedirects.find(primary.get());
  if (it != scene.protoRedirects.end()) {
    check(it->second.count("trig") == 0,
          "9c: no redirect registered for invalid IS mapping");
  }
}

// =============================================================================
// Driver
// =============================================================================
int main(int argc, char **argv) {
  if (argc > 1) {
    // accept argv[1] as data dir for the file-based tests
    std::string d = argv[1];
    setenv("X3D_TEST_DATA_DIR", d.c_str(), 1);
  }

  initOnlyValueForwardTest();
  initOnlyDefaultTest();

  inputOnlyRedirectPopulatedTest();
  inputOnlyRedirectBridgesTest();

  inputOutputRedirectTest();

  selfRecursiveDepthGuardTest();
  depthGuardPerStackTest();

  externLocalFileResolveTest();
  externLocalFileRelativeUrlTest();

  clonePreservesInputOnlyTest();
  clonePreservesInitializeOnlyTest();

  bodyUseIsLocalTest();

  redirectRejectsInputOnlyAsSourceTest();
  redirectRejectsOutputOnlyAsSinkTest();
  redirectRejectsTypeMismatchTest();
  validRedirectStillWorksTest();

  invalidIsMappingInitOnlyBodyToInputOnlyIfaceTest();
  invalidIsMappingInputOnlyBodyToInitOnlyIfaceTest();
  invalidIsMappingOutputOnlyBodyToInputOnlyIfaceTest();

  if (failures) {
    std::cerr << "\n" << failures << " check(s) failed\n";
    return 1;
  }
  std::cout << "\nall proto-expand audit tests passed\n";
  return 0;
}
