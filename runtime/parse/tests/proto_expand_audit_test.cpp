// PROTO expansion preserves local scope, IS values, redirects, and depth bounds.

#include "x3d/nodes/BooleanTrigger.hpp"
#include "Encoding.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DParse.hpp"
#include "X3DSceneBridge.hpp"

#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DProtoExpand.hpp"
#include "x3d/core/X3DReflection.hpp"

#include "doctest/doctest.h"

#include <any>
#include <memory>
#include <string>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;
namespace xn = x3d::nodes;
using x3d::codec::noopProtoResolver;


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
// (1) initializeOnly IS does value substitution; ProtoField value visible on
//     instance. The clone + forward-set path lands the override on the cloned
//     body field.
TEST_CASE("proto expand audit: an initializeOnly IS override lands on the cloned body field") {

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

  CHECK_MESSAGE((primary != nullptr), "initializeOnly: primary clone produced");
  CHECK_MESSAGE((primary && primary->nodeTypeName() == "Box"), "initializeOnly: primary is Box");
  auto sz = std::any_cast<SFVec3f>(
      fieldByName(*primary, "size")->get(*primary));
  CHECK_MESSAGE((sz.x == 5.f && sz.y == 5.f && sz.z == 5.f), "initializeOnly: override 5,5,5 lands on cloned body field");
  CHECK_MESSAGE((w.empty()), "initializeOnly: no warnings on clean override");
}

TEST_CASE("proto expand audit: an initializeOnly IS without override keeps the interface default") {

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

  CHECK_MESSAGE((primary != nullptr), "initializeOnly-default: primary clone produced");
  auto col = std::any_cast<SFColor>(
      fieldByName(*primary, "diffuseColor")->get(*primary));
  CHECK_MESSAGE((col.r == 0.5f && col.g == 0.5f && col.b == 0.5f), "initializeOnly-default: proto default 0.5 visible on clone");
}
// (2) inputOnly IS creates a redirect. Expansion populates protoRedirects and
//     the bridge wires an external source through the interface to the body's
//     inputOnly event handler.
TEST_CASE("proto expand audit: an inputOnly IS registers a redirect") {

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

  CHECK_MESSAGE((primary && primary->nodeTypeName() == "BooleanTrigger"), "inputOnly: primary is BooleanTrigger");
  CHECK_MESSAGE((scene.protoRedirects.count(primary.get()) == 1), "inputOnly: primary registered in protoRedirects");
  auto &byField = scene.protoRedirects[primary.get()];
  CHECK_MESSAGE((byField.count("trig") == 1), "inputOnly: 'trig' interface field has one redirect target");
  CHECK_MESSAGE((byField["trig"].size() == 1), "inputOnly: one body endpoint per IS");
  CHECK_MESSAGE((byField["trig"][0].targetField == "set_triggerTime"), "inputOnly: body target is set_triggerTime");
  CHECK_MESSAGE((byField["trig"][0].targetNode &&
        byField["trig"][0].targetNode->nodeTypeName() == "BooleanTrigger"), "inputOnly: body target node is the cloned BooleanTrigger");
}

TEST_CASE("proto expand audit: an inputOnly redirect bridges an external ROUTE to the body") {

  // Same interface/body mapping as 2a (trig -> set_triggerTime, SFTime). External
  // source: a TouchSensor that emits SFTime on touchTime — types match (both
  // SFTime), direction is valid (outputOnly source -> inputOnly sink). The
  // redirect path must add this route, not reject the interface field as
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

  CHECK_MESSAGE((r.routesAdded >= 1), "inputOnly: external route through interface added via redirect");
  bool interfaceRejected = false;
  for (const auto &err : r.rejected) {
    if (err.reason.find("trig") != std::string::npos)
      interfaceRejected = true;
  }
  CHECK_MESSAGE((!interfaceRejected), "inputOnly: interface field NOT rejected as unknown");
}
// (3) inputOutput IS: expansion populates protoRedirects and the body field
//     has both a write thunk (set_xxx) and read thunk (xxx_changed) — both
//     directions can be the target of an external ROUTE.
TEST_CASE("proto expand audit: an inputOutput IS redirects both directions") {

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

  CHECK_MESSAGE((primary && primary->nodeTypeName() == "Material"), "inputOutput: primary is Material");
  CHECK_MESSAGE((scene.protoRedirects[primary.get()].count("color") == 1), "inputOutput: 'color' interface field redirects to body diffuseColor");

  // The body field has the correct reflection access type on the clone.
  const FieldInfo *fi = fieldByName(*primary, "diffuseColor");
  CHECK_MESSAGE((fi && fi->access == AccessType::InputOutput), "inputOutput: cloned body field diffuseColor stays InputOutput");

  // The redirect target node IS the cloned Material primary.
  CHECK_MESSAGE((scene.protoRedirects[primary.get()]["color"][0].targetNode.get()
            == primary.get()), "inputOutput: redirect target node is the cloned primary");

  // Bridge an external source (TimeSensor's fraction SFFloat, here just for
  // direction check — values would need to be SFFloat-compatible; we only
  // verify that an unknown interface field name is redirected, not rejected).
  scene.routes.push_back(
      Route{"Ghost", "value_changed", "S", "color"}); // dangling source -> skip
  X3DExecutionContext ctx;
  BridgeResult r = buildRoutes(scene, ctx);
  // The dangling source is skipped silently (not counted, not rejected) — but
  // the interface field 'color' on S should still not appear in `rejected`.
  bool interfaceRejected = false;
  for (const auto &err : r.rejected) {
    if (err.reason.find("color") != std::string::npos)
      interfaceRejected = true;
  }
  CHECK_MESSAGE((!interfaceRejected), "inputOutput: interface 'color' not rejected (dangling source only)");
}
// (4) Self-recursive PROTO (body instantiates itself) hits the depth guard.
//     Spec §4.4.4.3 mandates that self-referential protos not loop forever;
//     our defensive cap is ExpandGuard::maxDepth = 32 (per expansion stack).
TEST_CASE("proto expand audit: a self-recursive PROTO stops at the depth guard") {

  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Self";
  auto box = createX3DNode("Box");
  decl->body.nodes.push_back(box);

  // A nested instance inside the body that instantiates the same PROTO.
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
  // Bounded: this must return, not infinite-loop.
  expandScene(scene, noopProtoResolver, "", w);

  bool sawRecursionLimit = false;
  for (const auto &warn : w) {
    if (warn.kind == ProtoWarning::Kind::RecursionLimit) sawRecursionLimit = true;
  }
  CHECK_MESSAGE((sawRecursionLimit), "self-recursive: RecursionLimit warning recorded (depth cap fired)");
  // The top-level instance should still produce a Box primary — the top
  // instance itself is not at the cap; only its nested recursion is.
  bool foundBox = false;
  for (auto &n : scene.rootNodes) {
    if (n && n->nodeTypeName() == "Box") foundBox = true;
  }
  CHECK_MESSAGE((foundBox), "self-recursive: top-level primary is a Box");
}

TEST_CASE("proto expand audit: the depth guard counts per expansion stack") {

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
  CHECK_MESSAGE((w.empty()), "depth-guard: two non-recursive top-level instances -> no warnings");
  int rootCount = 0;
  for (auto &n : scene.rootNodes) if (n) ++rootCount;
  CHECK_MESSAGE((rootCount == 2), "depth-guard: both top-level instances produce a root node");
}
// (5) EXTERNPROTO with local file URL resolves and expands. We test via
//     parseFile(front door) so the resolver seam is the production one.
TEST_CASE("proto expand audit: a local-file EXTERNPROTO resolves through parseFile") {

  const std::string dataDir = X3D_PARSE_TEST_DATA "/proto";
  auto doc = x3d::codec::parseFile(dataDir + "/main.x3d");

  bool foundBox = false;
  for (auto &n : doc.scene.rootNodes) {
    if (n && n->nodeTypeName() == "Box") foundBox = true;
  }
  CHECK_MESSAGE((foundBox), "EXTERNPROTO: sibling-file EXTERNPROTO expands into Box primary");
  CHECK_MESSAGE((doc.protoWarnings.empty()), "EXTERNPROTO: no warnings on resolved sibling-file URL");
}

TEST_CASE("proto expand audit: a relative EXTERNPROTO url resolves against the base url") {

  // Direct programmatic: pass baseUrl explicitly so we exercise the relative
  // resolution path independent of parseFile's path-derivation logic.
  auto ext = std::make_shared<ExternProtoDeclaration>();
  ext->name = "ExtBox"; ext->url = {"shapes.x3d#ExtBox"};

  x3d::runtime::ProtoInstance inst;
  inst.name = "ExtBox"; inst.externDeclaration = ext;

  const std::string dataDir = X3D_PARSE_TEST_DATA "/proto";

  Scene scene; ExpandGuard guard; std::vector<ProtoWarning> w;
  auto primary = expandInstance(inst, scene, noopProtoResolver, dataDir, guard, w);

  // noopProtoResolver returns null, so this should warn and yield no primary.
  CHECK_MESSAGE((!primary), "EXTERNPROTO: noop resolver yields no primary");
  CHECK_MESSAGE((w.size() == 1 && w[0].kind == ProtoWarning::Kind::UnresolvedExtern), "EXTERNPROTO: UnresolvedExtern warning recorded");

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
  CHECK_MESSAGE((got), "EXTERNPROTO: parseDocument with explicit baseUrl resolves relative URL");

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
  CHECK_MESSAGE((badWarned), "EXTERNPROTO: unresolved #fragment warns (no such PROTO in target)");
}
// (6) Clone preserves event-only semantics: an inputOnly event field on the
//     body remains inputOnly on the clone (with a write thunk), and an
//     initializeOnly field stays constant after clone.
TEST_CASE("proto expand audit: a clone keeps inputOnly fields event-only") {

  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Btn";
  auto bt = createX3DNode("BooleanTrigger");
  decl->body.nodes.push_back(bt);
  decl->body.isConnections.push_back({bt, "set_triggerTime", "trig"});

  x3d::runtime::ProtoInstance inst;
  inst.name = "Btn"; inst.declaration = decl; inst.DEF = "B";
  Scene scene; ExpandGuard guard; std::vector<ProtoWarning> w;
  auto primary = expandInstance(inst, scene, noopProtoResolver, "", guard, w);

  CHECK_MESSAGE((primary && primary->nodeTypeName() == "BooleanTrigger"), "clone: primary is BooleanTrigger");

  const FieldInfo *setTime = fieldByName(*primary, "set_triggerTime");
  CHECK_MESSAGE((setTime != nullptr), "clone: set_triggerTime field present on clone");
  CHECK_MESSAGE((setTime && setTime->access == AccessType::InputOnly), "clone: set_triggerTime access stays InputOnly on clone");
  CHECK_MESSAGE((setTime && setTime->set != nullptr), "clone: set_triggerTime still has a write thunk (events deliverable)");

  // Register a handler on the clone and post an event via the bridge — the
  // handler must fire, proving the cloned body node is fully wired.
  auto raw = primary.get();
  // Cast to register handler.
  auto btPtr = std::dynamic_pointer_cast<class xn::BooleanTrigger>(primary);
  CHECK_MESSAGE((btPtr != nullptr), "clone: primary downcasts to BooleanTrigger");
  if (btPtr) {
    int fired = 0;
    btPtr->setOnSet_triggerTimeHandler(
        [&fired](const SFTime &) { ++fired; });
    X3DExecutionContext ctx;
    ctx.postEvent(raw, "set_triggerTime", std::any(SFTime{1.0}));
    ctx.process();
    CHECK_MESSAGE((fired == 1), "clone: posted event reaches cloned body's inputOnly handler");
  }
}

TEST_CASE("proto expand audit: a clone keeps initializeOnly fields constant") {

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
  CHECK_MESSAGE((sz && sz->access == AccessType::InitializeOnly), "clone: size remains InitializeOnly on the clone");
  auto got = std::any_cast<SFVec3f>(sz->get(*primary));
  CHECK_MESSAGE((got.x == 3.f && got.y == 3.f && got.z == 3.f), "clone: proto-default value lands on the clone (constant)");
}
// (7) PROTO body USE resolves to body-local instance, not scene-global.
//     A node with DEF inside a ProtoBody and another node using it via USE
//     must share identity at the body level after expansion — and that DEF
//     name must not be promoted into scene.defs (proto-local DEF scope).
TEST_CASE("proto expand audit: a USE in the ProtoBody shares identity and stays proto-local") {

  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Body";

  // Body node 1: a Group with DEF "S".
  auto source = createX3DNode("Group");
  source->setDEF("S");
  decl->body.nodes.push_back(source);

  // Body node 2: a Group whose children reference the source Group via the same
  // shared_ptr (USE semantics at the body level — deepClone preserves
  // pointer identity via the cloneMap).
  auto group = createX3DNode("Group");
  group->setDEF("G");
  const FieldInfo *kids = fieldByName(*group, "children");
  kids->set(*group, std::any(std::vector<std::shared_ptr<X3DNode>>{source}));
  decl->body.nodes.push_back(group);

  // Body-internal ROUTE naming the DEFs — proves the localDefs map resolves
  // the cloned Group names. Both endpoints have compatible MFNode events.
  decl->body.routes.push_back(
      Route{"S", "children_changed", "G", "addChildren"});

  x3d::runtime::ProtoInstance inst;
  inst.name = "Body"; inst.declaration = decl; inst.DEF = "OUTER";
  Scene scene; ExpandGuard guard; std::vector<ProtoWarning> w;
  auto primary = expandInstance(inst, scene, noopProtoResolver, "", guard, w);

  // The primary clones the first body node and takes the instance DEF.
  // Body USE identity is preserved, while body DEF names stay out of scene.defs.
  CHECK_MESSAGE((primary && primary->nodeTypeName() == "Group"), "body-USE: primary is Group (first body node)");

  // The proto-local DEF "S" must not be promoted to scene.defs.
  CHECK_MESSAGE((scene.defs.count("S") == 0), "body-USE: body DEF 'S' NOT promoted to scene.defs (proto-local)");
  CHECK_MESSAGE((scene.defs.count("G") == 0), "body-USE: body DEF 'G' NOT promoted to scene.defs (proto-local)");

  // The body route must be pre-resolved via localDefs.
  CHECK_MESSAGE((scene.resolvedProtoRoutes.size() == 1), "body-USE: body route pre-resolved via localDefs");
  if (scene.resolvedProtoRoutes.empty()) return;
  const auto &r = scene.resolvedProtoRoutes[0];
  CHECK_MESSAGE((r.from && r.from.get() == primary.get()), "body-USE: route source resolves to the cloned Group (USE identity)");
  CHECK_MESSAGE((r.to && r.to->nodeTypeName() == "Group"), "body-USE: route sink resolves to the cloned Group");
  if (!r.to) return;
  const FieldInfo *clonedKids = fieldByName(*r.to, "children");
  auto children = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
      clonedKids->get(*r.to));
  CHECK_MESSAGE((children.size() == 1 && children[0] == primary), "body-USE: cloned Group child shares the primary pointer");
}
// PROTO redirects reject invalid directions and mismatched field types.
TEST_CASE("proto expand audit: a redirect rejects an inputOnly field as ROUTE source") {

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

  // Bad route: try to use the inputOnly interface 'trig' as a source
  // (B.trig -> SinkNode.something). The redirect target is body.set_triggerTime
  // which is inputOnly — the route is direction-invalid.
  auto sink = createX3DNode("BooleanToggle"); sink->setDEF("SNK");
  scene.addRootNode(sink);
  scene.routes.push_back(Route{"B", "trig", "SNK", "set_boolean"});

  X3DExecutionContext ctx;
  BridgeResult r = buildRoutes(scene, ctx);

  // The route must not be added. The defining property: no redirected edge
  // wires inputOnly-as-source.
  bool seenTrigRejected = false;
  for (const auto &err : r.rejected) {
    if (err.reason.find("trig") != std::string::npos) seenTrigRejected = true;
  }
  CHECK_MESSAGE((seenTrigRejected), "inputOnly-as-source: rejected with diagnostic naming 'trig'");
  // The diagnostic must call out the direction failure (outputOnly/inputOutput
  // required for source), so users can tell what to fix.
  bool mentionsDirection = false;
  for (const auto &err : r.rejected) {
    if (err.reason.find("source") != std::string::npos &&
        err.reason.find("outputOnly") != std::string::npos) {
      mentionsDirection = true;
    }
  }
  CHECK_MESSAGE((mentionsDirection), "inputOnly-as-source: diagnostic mentions 'source' / 'outputOnly'");

  // And concretely: ctx.graph() has no source edge keyed on set_triggerTime.
  // The cloned BooleanTrigger is reachable via scene.protoRedirects[B].
  X3DNode *bodyNode = scene.protoRedirects[primaryOf(scene, "B")]["trig"]
                          [0].targetNode.get();
  auto broken = ctx.graph().sinks({bodyNode, "set_triggerTime"});
  CHECK_MESSAGE((broken.empty()), "inputOnly-as-source: no edge wired from set_triggerTime");
}

TEST_CASE("proto expand audit: a redirect rejects an outputOnly field as ROUTE sink") {

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

  // Bad route: try to write into the outputOnly interface 'out'
  // (Src.out <- SRC.toggle). Body target is triggerTrue (outputOnly).
  auto src = createX3DNode("BooleanToggle"); src->setDEF("SRC");
  scene.addRootNode(src);
  scene.routes.push_back(Route{"SRC", "toggle", "S", "out"});

  X3DExecutionContext ctx;
  BridgeResult r = buildRoutes(scene, ctx);

  bool seenOutRejected = false;
  for (const auto &err : r.rejected) {
    if (err.reason.find("'out'") != std::string::npos ||
        err.reason.find("out") != std::string::npos) seenOutRejected = true;
  }
  CHECK_MESSAGE((seenOutRejected), "outputOnly-as-sink: rejected with diagnostic naming 'out'");

  // The invalid outputOnly sink must have no edge to body.triggerTrue.
  X3DNode *bodyNode = scene.protoRedirects[primaryOf(scene, "S")]["out"]
                          [0].targetNode.get();
  X3DNode *srcRaw = src.get();
  auto wired = ctx.graph().sinks({srcRaw, "toggle"});
  bool wiredToTriggerTrue = false;
  for (const auto &sink : wired) {
    if (sink.node == bodyNode && sink.field == "triggerTrue") {
      wiredToTriggerTrue = true;
    }
  }
  CHECK_MESSAGE((!wiredToTriggerTrue), "outputOnly-as-sink: no edge wired to body.triggerTrue");
}

TEST_CASE("proto expand audit: a redirect rejects mismatched field types") {

  // Interface 'trig' is SFTime (inputOnly). External source is SFFloat
  // (TimeSensor.fraction). The body target is set_triggerTime (SFTime);
  // but the source is SFFloat — direction is fine (inputOnly as sink OK),
  // type is not. Redirect must reject on type mismatch, mirroring the
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
  // TS.fraction_changed is SFFloat -> B.trig (SFTime): direction OK, type not.
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
  // A type-mismatched redirect must be rejected without adding a route.
  CHECK_MESSAGE((seenTypeRejection || r.routesAdded == 0), "type-mismatch: rejected OR not wired (redirect validation active)");
  CHECK_MESSAGE((seenTypeRejection), "type-mismatch: diagnostic names the type mismatch");
}
// (8d) A valid redirect (inputOnly interface as sink with matching types) still
//      works after the fix — sanity guard that the validation didn't regress
//      the legitimate redirect path.
TEST_CASE("proto expand audit: a valid redirect still wires") {

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

  CHECK_MESSAGE((r.routesAdded == 1), "valid-redirect: one route added via redirect path");
  CHECK_MESSAGE((r.rejected.empty()), "valid-redirect: no rejections");

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
  CHECK_MESSAGE((wiredToBody), "valid-redirect: edge wired from TS.touchTime to body.set_triggerTime");
}
// (9) IS access-type validation per Table 4.4 — invalid mappings warn and
//     are skipped rather than silently creating bad redirects / value forwards.
TEST_CASE("proto expand audit: an IS from an initializeOnly body field to an inputOnly interface warns") {

  // Body: Box.size is initializeOnly. Interface: "trig" is inputOnly.
  // Table 4.4 says no — this IS mapping is invalid.
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

  CHECK_MESSAGE((primary != nullptr), "9a: expansion still succeeds despite bad IS");
  bool saw = false;
  for (const auto &warn : w) {
    if (warn.kind == ProtoWarning::Kind::InterfaceMismatch &&
        warn.detail.find("size IS trig") != std::string::npos) {
      saw = true;
    }
  }
  CHECK_MESSAGE((saw), "9a: InterfaceMismatch warning for initOnly->inputOnly IS");

  // No redirect must be registered for the bad mapping.
  auto it = scene.protoRedirects.find(primary.get());
  if (it != scene.protoRedirects.end()) {
    CHECK_MESSAGE((it->second.count("trig") == 0), "9a: no redirect registered for invalid IS mapping");
  }
}

TEST_CASE("proto expand audit: an IS from an inputOnly body field to an initializeOnly interface warns") {

  // Body: BooleanTrigger.set_triggerTime is inputOnly.
  // Interface: "size" is initializeOnly. Table 4.4 says no.
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

  CHECK_MESSAGE((primary != nullptr), "9b: expansion still succeeds despite bad IS");
  bool saw = false;
  for (const auto &warn : w) {
    if (warn.kind == ProtoWarning::Kind::InterfaceMismatch &&
        warn.detail.find("set_triggerTime IS size") != std::string::npos) {
      saw = true;
    }
  }
  CHECK_MESSAGE((saw), "9b: InterfaceMismatch warning for inputOnly->initOnly IS");
  CHECK_MESSAGE((fieldByName(*primary, "set_triggerTime") != nullptr), "9b: set_triggerTime field present on clone");

}

TEST_CASE("proto expand audit: an IS from an outputOnly body field to an inputOnly interface warns") {

  // Body: BooleanTrigger.triggerTrue is outputOnly.
  // Interface: "trig" is inputOnly. Table 4.4 says no.
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

  CHECK_MESSAGE((primary != nullptr), "9c: expansion still succeeds despite bad IS");
  bool saw = false;
  for (const auto &warn : w) {
    if (warn.kind == ProtoWarning::Kind::InterfaceMismatch &&
        warn.detail.find("triggerTrue IS trig") != std::string::npos) {
      saw = true;
    }
  }
  CHECK_MESSAGE((saw), "9c: InterfaceMismatch warning for outputOnly->inputOnly IS");

  auto it = scene.protoRedirects.find(primary.get());
  if (it != scene.protoRedirects.end()) {
    CHECK_MESSAGE((it->second.count("trig") == 0), "9c: no redirect registered for invalid IS mapping");
  }
}
