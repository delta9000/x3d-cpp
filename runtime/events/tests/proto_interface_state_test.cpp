#include "doctest/doctest.h"
// proto_interface_state_test.cpp
// PROTO-INTERFACE-STATE: a prototype interface field with no IS target must
// still exist on the instance as a real field with its initial value and a
// routable event endpoint (ISO/IEC 19775-1 §4.4.2.2; §4.4.4.2).
//
// Relay declares inputOutput SFVec3f value, a Group body and NO IS connection.
// Value-forwarding/redirect capture never touches it, so before this change the
// bridge rejected both external routes ("has no IS route target"). Expansion now
// registers every unconnected scalar interface field as independent storage on
// the primary (the dynamic-field store), and the bridge resolves such a field to
// its own endpoint. A route IN sets the field; a route OUT relays it.
//
// Also asserts the fieldValue/interface default survives, and that an
// IS-connected field is unchanged (the redirect path still wins).

#include "DynamicField.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DParse.hpp"
#include "X3DProtoExpand.hpp"
#include "X3DSceneBridge.hpp"

#include "x3d/nodes/Group.hpp"
#include "x3d/nodes/Transform.hpp"

#include <any>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace x3d;
using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;

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

// Relay: inputOutput SFVec3f value (no IS) over a Group body. Source drives it;
// Dest receives the relayed value_changed.
const char *kRelayXml = R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='Relay'><ProtoInterface>
<field name='value' type='SFVec3f' accessType='inputOutput' value='0 0 0'/>
</ProtoInterface><ProtoBody><Group/></ProtoBody></ProtoDeclare>
<Transform DEF='Source'/>
<ProtoInstance name='Relay' DEF='RelayNode'/>
<Transform DEF='Dest'/>
<ROUTE fromNode='Source' fromField='translation_changed' toNode='RelayNode' toField='set_value'/>
<ROUTE fromNode='RelayNode' fromField='value_changed' toNode='Dest' toField='set_translation'/>
</Scene></X3D>)";

void test_unmapped_inputoutput_relays() {
  dynamicFieldStore().clear();
  auto doc = x3d::codec::parseDocument(kRelayXml, x3d::codec::Encoding::XML);
  Scene &scene = doc.scene;

  auto source = std::dynamic_pointer_cast<Transform>(scene.resolve("Source"));
  auto relay = scene.resolve("RelayNode");
  auto dest = std::dynamic_pointer_cast<Transform>(scene.resolve("Dest"));
  check(source && relay && dest, "Relay scene expanded its three nodes");
  if (!(source && relay && dest)) return;

  X3DExecutionContext ctx;
  BridgeResult res = buildRoutes(scene, ctx);
  check(res.rejected.empty(), "no route rejected (both interfaces resolve)");
  check(res.routesAdded == 2, "two routes wired through the interface field");

  ctx.buildSceneGraph(scene);
  ctx.postEvent(source.get(), "translation", std::any(SFVec3f{1, 2, 3}));
  ctx.tick(0.0);

  auto stored = dynamicFieldStore().getValue(*relay, "value");
  const SFVec3f v = stored.has_value() ? std::any_cast<SFVec3f>(stored)
                                       : SFVec3f{-1, -1, -1};
  check(v.x == 1.f && v.y == 2.f && v.z == 3.f,
        "route in set the unconnected interface field");
  const SFVec3f out = dest->getTranslation();
  check(out.x == 1.f && out.y == 2.f && out.z == 3.f,
        "route out relayed the changed event to Dest");
}

void test_unconnected_keeps_default() {
  dynamicFieldStore().clear();
  auto doc = x3d::codec::parseDocument(kRelayXml, x3d::codec::Encoding::XML);
  Scene &scene = doc.scene;
  auto relay = scene.resolve("RelayNode");
  check(relay != nullptr, "Relay primary expanded");
  if (!relay) return;

  // A fieldValue override replaces the interface default.
  auto stored = dynamicFieldStore().getValue(*relay, "value");
  const SFVec3f v = stored.has_value() ? std::any_cast<SFVec3f>(stored)
                                       : SFVec3f{-1, -1, -1};
  check(v.x == 0.f && v.y == 0.f && v.z == 0.f,
        "unconnected field keeps its interface default 0 0 0");

  // A matched fieldValue override is the field's initial value.
  const std::string xml = R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='Relay'><ProtoInterface>
<field name='value' type='SFVec3f' accessType='inputOutput' value='0 0 0'/>
</ProtoInterface><ProtoBody><Group/></ProtoBody></ProtoDeclare>
<ProtoInstance name='Relay' DEF='R'><fieldValue name='value' value='4 5 6'/></ProtoInstance>
</Scene></X3D>)";
  auto doc2 = x3d::codec::parseDocument(xml, x3d::codec::Encoding::XML);
  auto r2 = doc2.scene.resolve("R");
  check(r2 != nullptr, "override Relay primary expanded");
  if (!r2) return;
  auto s2 = dynamicFieldStore().getValue(*r2, "value");
  const SFVec3f v2 = s2.has_value() ? std::any_cast<SFVec3f>(s2)
                                    : SFVec3f{-1, -1, -1};
  check(v2.x == 4.f && v2.y == 5.f && v2.z == 6.f,
        "fieldValue override seeds the unconnected field");
}

// An IS-connected field must still route through its body clone, not the new
// independent-storage fallback.
const char *kMappedXml = R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='Mapped'><ProtoInterface>
<field name='value' type='SFVec3f' accessType='inputOutput' value='0 0 0'/>
</ProtoInterface><ProtoBody><Transform><IS>
<connect nodeField='translation' protoField='value'/>
</IS></Transform></ProtoBody></ProtoDeclare>
<Transform DEF='Source'/>
<ProtoInstance name='Mapped' DEF='M'/>
<Transform DEF='Dest'/>
<ROUTE fromNode='Source' fromField='translation_changed' toNode='M' toField='set_value'/>
<ROUTE fromNode='M' fromField='value_changed' toNode='Dest' toField='set_translation'/>
</Scene></X3D>)";

void test_is_connected_unchanged() {
  dynamicFieldStore().clear();
  auto doc = x3d::codec::parseDocument(kMappedXml, x3d::codec::Encoding::XML);
  Scene &scene = doc.scene;
  auto source = std::dynamic_pointer_cast<Transform>(scene.resolve("Source"));
  auto mapped = std::dynamic_pointer_cast<Transform>(scene.resolve("M"));
  auto dest = std::dynamic_pointer_cast<Transform>(scene.resolve("Dest"));
  check(source && mapped && dest, "Mapped scene expanded");
  if (!(source && mapped && dest)) return;

  X3DExecutionContext ctx;
  BridgeResult res = buildRoutes(scene, ctx);
  check(res.rejected.empty(), "mapped: routes resolve through the IS redirect");
  check(res.routesAdded == 2, "mapped: two routes wired");

  ctx.postEvent(source.get(), "translation", std::any(SFVec3f{7, 8, 9}));
  ctx.tick(0.0);
  const SFVec3f out = dest->getTranslation();
  check(out.x == 7.f && out.y == 8.f && out.z == 9.f,
        "IS-connected field still relays through its body clone");
  // The IS-mapped field forwards onto the primary Transform.translation, not an
  // independent store entry.
  const SFVec3f body = mapped->getTranslation();
  check(body.x == 7.f && body.y == 8.f && body.z == 9.f,
        "IS-connected value lands on the body field");
}

// -------------------------------------------------------------------------
// Risk: independent-storage registration must not overwrite or duplicate an
// unrelated instance's entry. Two sibling nested instances of the same
// prototype (distinct primaries), plus the enclosing Outer's own unconnected
// field, must each keep independent storage; a second Outer must be isolated.
// -------------------------------------------------------------------------
const char *kNestedXml = R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='Inner'><ProtoInterface>
<field name='shift' type='SFVec3f' accessType='inputOutput' value='0 0 0'/>
</ProtoInterface><ProtoBody><Group/></ProtoBody></ProtoDeclare>
<ProtoDeclare name='Outer'><ProtoInterface>
<field name='top' type='SFVec3f' accessType='inputOutput' value='0 0 0'/>
</ProtoInterface><ProtoBody><Group>
<ProtoInstance name='Inner' DEF='A'/><Transform DEF='Ord'/>
<ProtoInstance name='Inner' DEF='B'/>
<ROUTE fromNode='A' fromField='shift_changed' toNode='Ord' toField='set_translation'/>
<ROUTE fromNode='Ord' fromField='translation_changed' toNode='B' toField='set_shift'/>
</Group></ProtoBody></ProtoDeclare>
<Transform DEF='Driver'/><Transform DEF='External'/>
<ProtoInstance name='Outer' DEF='O1'/><ProtoInstance name='Outer' DEF='O2'/>
<ROUTE fromNode='Driver' fromField='translation_changed' toNode='O1' toField='set_top'/>
<ROUTE fromNode='O1' fromField='top_changed' toNode='External' toField='set_translation'/>
</Scene></X3D>)";

SFVec3f storedVec3(const X3DNode &n, const std::string &name) {
  auto v = dynamicFieldStore().getValue(n, name);
  return v.has_value() ? std::any_cast<SFVec3f>(v) : SFVec3f{-1, -1, -1};
}

void test_nested_siblings_do_not_overwrite() {
  dynamicFieldStore().clear();
  auto doc = x3d::codec::parseDocument(kNestedXml, x3d::codec::Encoding::XML);
  Scene &scene = doc.scene;
  auto o1 = std::dynamic_pointer_cast<Group>(scene.resolve("O1"));
  auto o2 = std::dynamic_pointer_cast<Group>(scene.resolve("O2"));
  auto driver = std::dynamic_pointer_cast<Transform>(scene.resolve("Driver"));
  auto external = std::dynamic_pointer_cast<Transform>(scene.resolve("External"));
  check(o1 && o2 && driver && external, "nested: nodes expanded");
  if (!(o1 && o2 && driver && external) || o1->getChildren().size() != 3 ||
      o2->getChildren().size() != 3) {
    check(false, "nested: each Outer has three ordered children");
    return;
  }
  auto a1 = std::dynamic_pointer_cast<Group>(o1->getChildren()[0]);
  auto b1 = std::dynamic_pointer_cast<Group>(o1->getChildren()[2]);
  auto a2 = std::dynamic_pointer_cast<Group>(o2->getChildren()[0]);
  auto b2 = std::dynamic_pointer_cast<Group>(o2->getChildren()[2]);
  check(a1 && b1 && a2 && b2 && a1 != b1 && a1 != a2 && a2 != b2,
        "nested: four distinct Inner primaries");
  if (!(a1 && b1 && a2 && b2)) return;

  // Every unconnected interface field exists on its own instance.
  check(storedVec3(*a1, "shift").x == 0.f && storedVec3(*b1, "shift").x == 0.f,
        "nested: sibling Inner primaries each own an independent 'shift'");
  check(storedVec3(*a2, "shift").x == 0.f && storedVec3(*b2, "shift").x == 0.f,
        "nested: second Outer's Inner primaries own independent 'shift'");
  check(storedVec3(*o1, "top").x == 0.f && storedVec3(*o2, "top").x == 0.f,
        "nested: each Outer primary owns its own 'top' (no clobber)");

  X3DExecutionContext ctx;
  BridgeResult res = buildRoutes(scene, ctx);
  check(res.rejected.empty(), "nested: all routes resolve");
  // buildRoutes counts the SCENE-level routes only (Driver->O1.top and
  // O1.top->External); the A->Ord->B routes inside each Outer body are not part
  // of this count. Observed on the host build; the behavior checks below carry
  // the real assertions.
  check(res.routesAdded == 2, "nested: the two scene-level routes are added");

  ctx.buildSceneGraph(scene);
  ctx.postEvent(driver.get(), "translation", std::any(SFVec3f{9, 0, 0}));
  ctx.tick(0.0);
  check(storedVec3(*o1, "top").x == 9.f, "nested: O1.top received the route");
  check(storedVec3(*o2, "top").x == 0.f, "nested: O2.top untouched (isolated)");
  check(external->getTranslation().x == 9.f,
        "nested: O1.top_changed relayed the changed event out");
}

// -------------------------------------------------------------------------
// Risk: an Outer whose primary IS a nested instance's primary (shared pointer)
// must have BOTH interfaces' independent fields present on the single node —
// the outer registration must not drop the inner's entry.
// -------------------------------------------------------------------------
const char *kSharedPrimaryXml = R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='Inner'><ProtoInterface>
<field name='inner' type='SFVec3f' accessType='inputOutput' value='1 1 1'/>
</ProtoInterface><ProtoBody><Group/></ProtoBody></ProtoDeclare>
<ProtoDeclare name='Outer'><ProtoInterface>
<field name='outer' type='SFVec3f' accessType='inputOutput' value='2 2 2'/>
</ProtoInterface><ProtoBody><ProtoInstance name='Inner' DEF='A'/></ProtoBody></ProtoDeclare>
<ProtoInstance name='Outer' DEF='O'/>
</Scene></X3D>)";

void test_shared_primary_keeps_both() {
  dynamicFieldStore().clear();
  auto doc = x3d::codec::parseDocument(kSharedPrimaryXml,
                                       x3d::codec::Encoding::XML);
  auto primary = doc.scene.resolve("O");
  check(primary != nullptr, "shared: Outer primary expanded");
  if (!primary) return;
  check(storedVec3(*primary, "inner").x == 1.f && storedVec3(*primary, "inner").y == 1.f,
        "shared: nested Inner's independent field survives on the shared node");
  check(storedVec3(*primary, "outer").x == 2.f && storedVec3(*primary, "outer").y == 2.f,
        "shared: Outer's own independent field present on the same node");
}

// -------------------------------------------------------------------------
// EXTERNPROTO: an instance whose declaration comes from the resolver still
// registers its unconnected interface fields and routes through them.
// -------------------------------------------------------------------------
void test_externproto_unconnected_field() {
  dynamicFieldStore().clear();
  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "ExtRelay";
  ProtoField pf;
  pf.name = "value";
  pf.type = X3DFieldType::SFVec3f;
  pf.access = AccessType::InputOutput;
  pf.value = std::any(SFVec3f{0, 0, 0});
  decl->interface.push_back(pf);
  auto body = createX3DNode("Group");
  decl->body.nodes.push_back(body);

  const char *xml = R"(<X3D version='4.0'><Scene>
<ExternProtoDeclare name='ExtRelay' url='relay.x3d'><field name='value' type='SFVec3f' accessType='inputOutput'/></ExternProtoDeclare>
<Transform DEF='Source'/><ProtoInstance name='ExtRelay' DEF='E'/><Transform DEF='Dest'/>
<ROUTE fromNode='Source' fromField='translation_changed' toNode='E' toField='set_value'/>
<ROUTE fromNode='E' fromField='value_changed' toNode='Dest' toField='set_translation'/>
</Scene></X3D>)";
  auto doc = x3d::codec::parseDocument(xml, x3d::codec::Encoding::XML);
  Scene &scene = doc.scene;
  auto resolver = [&](const std::vector<std::string> &, const std::string &) {
    return decl;
  };
  std::vector<ProtoWarning> w;
  expandScene(scene, resolver, "", w);
  auto source = std::dynamic_pointer_cast<Transform>(scene.resolve("Source"));
  auto e = scene.resolve("E");
  auto dest = std::dynamic_pointer_cast<Transform>(scene.resolve("Dest"));
  check(source && e && dest, "extern: nodes expanded");
  if (!(source && e && dest)) return;
  check(storedVec3(*e, "value").x == 0.f,
        "extern: unconnected interface field registered with its default");

  X3DExecutionContext ctx;
  BridgeResult res = buildRoutes(scene, ctx);
  check(res.rejected.empty(), "extern: routes resolve through the EXTERN interface");
  check(res.routesAdded == 2, "extern: two routes wired");
  ctx.postEvent(source.get(), "translation", std::any(SFVec3f{3, 6, 9}));
  ctx.tick(0.0);
  check(storedVec3(*e, "value").x == 3.f, "extern: route in set the field");
  check(dest->getTranslation().x == 3.f && dest->getTranslation().z == 9.f,
        "extern: changed event relayed out");
}

} // namespace

TEST_CASE("proto_interface_state_test") {
  test_unmapped_inputoutput_relays();
  test_unconnected_keeps_default();
  test_is_connected_unchanged();
  test_nested_siblings_do_not_overwrite();
  test_shared_primary_keeps_both();
  test_externproto_unconnected_field();
  dynamicFieldStore().clear();

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    CHECK(false);
    return;
  }
  std::cout << "all proto interface-state tests passed\n";
}

TEST_CASE("unconnected PROTO MFNode interface relays node identity and empty values") {
  dynamicFieldStore().clear();
  auto doc = x3d::codec::parseDocument(R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='Relay'><ProtoInterface>
<field name='value' type='MFNode' accessType='inputOutput'/>
</ProtoInterface><ProtoBody><Group/></ProtoBody></ProtoDeclare>
<Group DEF='Source'/><ProtoInstance name='Relay' DEF='RelayNode'/><Group DEF='Dest'/>
<ROUTE fromNode='Source' fromField='children_changed' toNode='RelayNode' toField='set_value'/>
<ROUTE fromNode='RelayNode' fromField='value_changed' toNode='Dest' toField='set_children'/>
</Scene></X3D>)", x3d::codec::Encoding::XML);
  auto source = std::dynamic_pointer_cast<Group>(doc.scene.resolve("Source"));
  auto relay = doc.scene.resolve("RelayNode");
  auto dest = std::dynamic_pointer_cast<Group>(doc.scene.resolve("Dest"));
  REQUIRE(source); REQUIRE(relay); REQUIRE(dest);
  X3DExecutionContext ctx;
  auto routes = buildRoutes(doc.scene, ctx);
  REQUIRE(routes.routesAdded == 2);
  REQUIRE(routes.rejected.empty());
  ctx.buildSceneGraph(doc.scene);
  const std::vector<std::shared_ptr<X3DNode>> nodes{std::make_shared<Transform>()};
  ctx.postEvent(source.get(), "children", std::any(nodes));
  ctx.tick(0.0);
  CHECK(dest->getChildren() == nodes);
  CHECK(std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
      dynamicFieldStore().getValue(*relay, "value")) == nodes);
  ctx.postEvent(source.get(), "children", std::any(std::vector<std::shared_ptr<X3DNode>>{}));
  ctx.tick(1.0);
  CHECK(dest->getChildren().empty());
  dynamicFieldStore().clear();
}

TEST_CASE("unconnected PROTO SFNode interface relays node identity and NULL") {
  dynamicFieldStore().clear();
  auto doc = x3d::codec::parseDocument(R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='Relay'><ProtoInterface>
<field name='value' type='SFNode' accessType='inputOutput'/>
</ProtoInterface><ProtoBody><Group/></ProtoBody></ProtoDeclare>
<Shape DEF='Source'/><ProtoInstance name='Relay' DEF='RelayNode'/><Shape DEF='Dest'/>
<ROUTE fromNode='Source' fromField='geometry_changed' toNode='RelayNode' toField='set_value'/>
<ROUTE fromNode='RelayNode' fromField='value_changed' toNode='Dest' toField='set_geometry'/>
</Scene></X3D>)", x3d::codec::Encoding::XML);
  auto source = doc.scene.resolve("Source");
  auto relay = doc.scene.resolve("RelayNode");
  auto dest = doc.scene.resolve("Dest");
  REQUIRE(source); REQUIRE(relay); REQUIRE(dest);
  X3DExecutionContext ctx;
  auto routes = buildRoutes(doc.scene, ctx);
  REQUIRE(routes.routesAdded == 2);
  REQUIRE(routes.rejected.empty());
  const std::shared_ptr<X3DNode> geometry = x3d::nodes::X3DNodeFactory::create("Box");
  REQUIRE(geometry);
  ctx.postEvent(source.get(), "geometry", std::any(geometry));
  ctx.tick(0.0);
  CHECK(std::any_cast<std::shared_ptr<X3DNode>>(
      dynamicFieldStore().getValue(*relay, "value")) == geometry);
  CHECK(std::any_cast<std::shared_ptr<X3DNode>>(
      proto_detail::findField(*dest, "geometry")->get(*dest)) == geometry);
  ctx.postEvent(source.get(), "geometry", std::any(std::shared_ptr<X3DNode>{}));
  ctx.tick(1.0);
  CHECK_FALSE(std::any_cast<std::shared_ptr<X3DNode>>(
      proto_detail::findField(*dest, "geometry")->get(*dest)));
  dynamicFieldStore().clear();
}

TEST_CASE("unconnected PROTO node defaults are private but caller overrides retain sharing") {
  dynamicFieldStore().clear();
  auto doc = x3d::codec::parseDocument(R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='Holder'><ProtoInterface>
<field name='single' type='SFNode' accessType='initializeOnly'><Transform DEF='N' translation='1 2 3'/></field>
<field name='many' type='MFNode' accessType='inputOutput'><Transform USE='N'/></field>
</ProtoInterface><ProtoBody><Group/></ProtoBody></ProtoDeclare>
<Transform DEF='Caller'/>
<ProtoInstance name='Holder' DEF='A'/><ProtoInstance name='Holder' DEF='B'/>
<ProtoInstance name='Holder' DEF='C'><fieldValue name='single'><Transform USE='Caller'/></fieldValue><fieldValue name='many'/></ProtoInstance>
<ProtoInstance name='Holder' DEF='D'><fieldValue name='single'/><fieldValue name='many'><Transform USE='Caller'/></fieldValue></ProtoInstance>
</Scene></X3D>)", x3d::codec::Encoding::XML);
  auto sf = [&](const char *def) {
    auto node = doc.scene.resolve(def);
    REQUIRE(node);
    auto value = dynamicFieldStore().getValue(*node, "single");
    REQUIRE(value.type() == typeid(std::shared_ptr<X3DNode>));
    return std::any_cast<std::shared_ptr<X3DNode>>(value);
  };
  auto mf = [&](const char *def) {
    auto node = doc.scene.resolve(def);
    REQUIRE(node);
    auto value = dynamicFieldStore().getValue(*node, "many");
    REQUIRE(value.type() == typeid(std::vector<std::shared_ptr<X3DNode>>));
    return std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(value);
  };
  auto a = sf("A"), b = sf("B");
  REQUIRE(a); REQUIRE(b);
  CHECK(a != b);
  CHECK(a != doc.scene.findProto("Holder")->interface[0].nodeDefault[0]);
  REQUIRE(mf("A").size() == 1);
  REQUIRE(mf("B").size() == 1);
  CHECK(mf("A")[0] == a);
  CHECK(mf("B")[0] == b);
  CHECK(sf("C") == doc.scene.resolve("Caller"));
  CHECK(mf("C").empty());
  CHECK_FALSE(sf("D"));
  REQUIRE(mf("D").size() == 1);
  CHECK(mf("D")[0] == doc.scene.resolve("Caller"));
  dynamicFieldStore().clear();
}
