#include "doctest/doctest.h"
// core_diagnostics_test.cpp
// Regression tests for the Core parsing/diagnostics findings:
//   PROTO-SHADOW          — a PROTO naming a built-in is quarantined + diagnosed
//   DIAG-UNKNOWN-NODE     — every reader surfaces a misspelled node element
//   DIAG-PROFILE-COERCE   — an unknown profile token is diagnosed and preserved
//   IMPORT-EXPORT-WIRE    — a ROUTE to an IMPORTed AS name reaches the Inline's
//                           exported node (§9.2 cross-Inline escape hatch)
#include "X3DCodecs.hpp"
#include "X3DParse.hpp"
#include "X3DRuntime.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSceneBridge.hpp"
#include "SceneExtractor.hpp"
#include "x3d/nodes/TimeSensor.hpp"
#include "x3d/nodes/Transform.hpp"
#include "x3d/nodes/Viewpoint.hpp"

#include <any>
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

using namespace x3d;
using x3d::runtime::ReaderWarning;

namespace {

// Depth-first walk over every SFNode/MFNode child of `n`, returning true if any
// node's type name equals `typeName`.
bool subtreeHasType(const std::shared_ptr<nodes::X3DNode> &n,
                    const std::string &typeName,
                    std::unordered_set<const nodes::X3DNode *> &seen) {
  if (!n || !seen.insert(n.get()).second)
    return false;
  if (n->nodeTypeName() == typeName)
    return true;
  for (const auto &f : n->fields()) {
    if (!f.get)
      continue;
    if (f.type == core::X3DFieldType::SFNode) {
      std::any v = f.get(*n);
      auto *c = std::any_cast<std::shared_ptr<nodes::X3DNode>>(&v);
      if (c && subtreeHasType(*c, typeName, seen))
        return true;
    } else if (f.type == core::X3DFieldType::MFNode) {
      std::any v = f.get(*n);
      auto *cs = std::any_cast<std::vector<std::shared_ptr<nodes::X3DNode>>>(&v);
      if (cs)
        for (const auto &c : *cs)
          if (subtreeHasType(c, typeName, seen))
            return true;
    }
  }
  return false;
}

bool sceneHasType(const runtime::Scene &scene, const std::string &typeName) {
  std::unordered_set<const nodes::X3DNode *> seen;
  for (const auto &r : scene.rootNodes)
    if (subtreeHasType(r, typeName, seen))
      return true;
  return false;
}

bool hasUnknownNode(const runtime::X3DDocument &doc) {
  for (const auto &w : doc.readerWarnings)
    if (w.kind == ReaderWarning::Kind::UnknownNode)
      return true;
  return false;
}

bool hasProfileCoerced(const runtime::X3DDocument &doc) {
  for (const auto &w : doc.readerWarnings)
    if (w.kind == ReaderWarning::Kind::ProfileCoerced)
      return true;
  return false;
}

bool hasBuiltinShadow(const runtime::X3DDocument &doc,
                      const std::string &name) {
  for (const auto &w : doc.protoWarnings)
    if (w.kind == runtime::ProtoWarning::Kind::BuiltinShadow &&
        w.instanceName == name)
      return true;
  return false;
}

} // namespace

TEST_CASE("unit_header_validation_all_encodings") {
  auto xml = [](const std::string &version, const std::string &category,
                const std::string &name, const std::string &factor) {
    return "<X3D version='" + version + "'><head><unit category='" +
           category + "' name='" + name + "' conversionFactor='" +
           factor + "'/></head><Scene/></X3D>";
  };
  auto vrml = [](const std::string &version, const std::string &category,
                 const std::string &name, const std::string &factor) {
    return "#X3D V" + version + " utf8\nUNIT " + category + " " + name +
           " " + factor + "\n";
  };
  auto json = [](const std::string &version, const std::string &category,
                 const std::string &name, const std::string &factor) {
    return "{\"X3D\":{\"@version\":\"" + version +
           "\",\"head\":{\"unit\":[{\"@category\":\"" + category +
           "\",\"@name\":\"" + name + "\",\"@conversionFactor\":" +
           factor + "}]},\"Scene\":{}}}";
  };
  for (const auto enc : {codec::Encoding::XML, codec::Encoding::ClassicVRML,
                         codec::Encoding::JSON}) {
    auto source = [&](const std::string &version, const std::string &category,
                      const std::string &name, const std::string &factor) {
      if (enc == codec::Encoding::XML) return xml(version, category, name, factor);
      if (enc == codec::Encoding::ClassicVRML)
        return vrml(version, category, name, factor);
      return json(version, category, name, factor);
    };
    auto valid = codec::parseDocument(source("3.3", "length", "centimetre", "0.01"), enc);
    REQUIRE(valid.head.units.size() == 1);
    CHECK(valid.head.units[0].conversionFactor == doctest::Approx(0.01));
    CHECK_THROWS(codec::parseDocument(source("3.2", "length", "cm", "0.01"), enc));
    CHECK_THROWS(codec::parseDocument(source("4.0", "speed", "kph", "1"), enc));
    CHECK_THROWS(codec::parseDocument(source("4.0", "length", "cm", "0"), enc));
    CHECK_THROWS(codec::parseDocument(source("4.0", "length", "cm", "-1"), enc));
  }
  CHECK_NOTHROW(codec::parseDocument(xml("4.0", "force", "newton", " +1e0 ")));
  CHECK_THROWS(codec::parseDocument(xml("3.x", "length", "m", "1")));
  CHECK_THROWS(codec::parseDocument(xml("+4.0", "length", "m", "1")));
  CHECK_THROWS(codec::parseDocument(xml("4.0", "length", "m", "1junk")));
  CHECK_THROWS(codec::parseDocument(
      vrml("4.0", "length", "m", "1junk"), codec::Encoding::ClassicVRML));
  CHECK_THROWS(codec::parseDocument(xml("4.0", "length", "two words", "1")));
  CHECK_THROWS(codec::parseDocument(xml("4.0", "length", "", "1")));
  CHECK_THROWS(codec::parseDocument(xml("4.0", "length", "m", "nan")));
  CHECK_THROWS(codec::parseDocument(xml("4.0", "length", "m", "inf")));
  CHECK_THROWS(codec::parseDocument(
      "<X3D version='4.0'><head><unit category='length' name='m'/>"
      "</head><Scene/></X3D>"));
  CHECK_THROWS(codec::parseDocument(
      "{\"X3D\":{\"@version\":\"4.0\",\"head\":{\"unit\":[{"
      "\"@category\":\"length\",\"@name\":\"m\"}]},\"Scene\":{}}}",
      codec::Encoding::JSON));
  CHECK_THROWS(codec::parseDocument(
      "<X3D version='4.0'><head><unit category='length' name='m' "
      "conversionFactor='1'/><unit category='length' name='cm' "
      "conversionFactor='0.01'/></head><Scene/></X3D>"));
  CHECK_THROWS(codec::parseDocument(
      "#X3D V4.0 utf8\nUNIT length m 1\nUNIT length cm 0.01\n",
      codec::Encoding::ClassicVRML));
  CHECK_THROWS(codec::parseDocument(
      "{\"X3D\":{\"@version\":\"4.0\",\"head\":{\"unit\":[{"
      "\"@category\":\"length\",\"@name\":\"m\",\"@conversionFactor\":1},{"
      "\"@category\":\"length\",\"@name\":\"cm\",\"@conversionFactor\":0.01}]},"
      "\"Scene\":{}}}", codec::Encoding::JSON));
  const auto all = codec::parseDocument(
      "<X3D version='4.0'><head>"
      "<unit category='angle' name='degrees' conversionFactor='0.017453292519943295'/>"
      "<unit category='force' name='pound-force' conversionFactor='4.4482216152605'/>"
      "<unit category='length' name='centimetre' conversionFactor='0.01'/>"
      "<unit category='mass' name='gram' conversionFactor='0.001'/>"
      "</head><Scene/></X3D>");
  CHECK(all.head.units.size() == 4);
}

// ── PROTO-SHADOW: a PROTO reusing a built-in name is rejected + diagnosed ────
TEST_CASE("proto_shadow_xml") {
  const char *xml =
      "<X3D version='4.0'><Scene>"
      "<ProtoDeclare name='Box'><ProtoBody><Sphere/></ProtoBody></ProtoDeclare>"
      "<Shape><Box/></Shape></Scene></X3D>";
  auto doc = codec::parseDocument(xml);
  CHECK(hasBuiltinShadow(doc, "Box"));
  // The rogue declaration is quarantined; the built-in keeps the name.
  CHECK(doc.scene.protoDeclarations.empty());
  // The later <Box/> still resolves through the factory (built-in semantics).
  CHECK(sceneHasType(doc.scene, "Box"));
}

// An explicit <ProtoInstance> of the quarantined name gets the built-in too,
// with its fieldValues applied — not the rogue body (ADR-0033).
TEST_CASE("proto_shadow_instance_becomes_builtin") {
  const char *xml =
      "<X3D version='4.0'><Scene>"
      "<ProtoDeclare name='Box'><ProtoInterface>"
      "<field accessType='initializeOnly' name='size' type='SFVec3f' value='1 1 1'/>"
      "</ProtoInterface><ProtoBody><Sphere/></ProtoBody></ProtoDeclare>"
      "<ExternProtoDeclare name='Disk2D' url='\"missing.wrl#Disk2D\"'>"
      "<field accessType='initializeOnly' name='outerRadius' type='SFFloat'/>"
      "</ExternProtoDeclare>"
      "<Shape><ProtoInstance name='Box' containerField='geometry'>"
      "<fieldValue name='size' value='2 3 4'/></ProtoInstance></Shape>"
      "<Shape><ProtoInstance name='Disk2D' containerField='geometry'>"
      "<fieldValue name='outerRadius' value='5'/></ProtoInstance></Shape>"
      "</Scene></X3D>";
  auto doc = codec::parseDocument(xml);
  CHECK(hasBuiltinShadow(doc, "Box"));
  CHECK(hasBuiltinShadow(doc, "Disk2D"));
  CHECK_FALSE(sceneHasType(doc.scene, "Sphere"));
  for (const auto &w : doc.protoWarnings)
    CHECK(w.kind == runtime::ProtoWarning::Kind::BuiltinShadow); // no unresolved-extern noise
  REQUIRE(doc.scene.rootNodes.size() == 2);
  auto geomOf = [](const std::shared_ptr<nodes::X3DNode> &shape) {
    for (const auto &f : shape->fields())
      if (f.x3dName == "geometry")
        return std::any_cast<std::shared_ptr<nodes::X3DNode>>(f.get(*shape));
    return std::shared_ptr<nodes::X3DNode>{};
  };
  auto field = [](const std::shared_ptr<nodes::X3DNode> &n, const char *name) {
    for (const auto &f : n->fields())
      if (f.x3dName == name) return f.get(*n);
    return std::any{};
  };
  auto box = geomOf(doc.scene.rootNodes[0]);
  REQUIRE(box);
  CHECK(box->nodeTypeName() == "Box");
  CHECK(std::any_cast<core::SFVec3f>(field(box, "size")).y == doctest::Approx(3.0f));
  auto disk = geomOf(doc.scene.rootNodes[1]);
  REQUIRE(disk);
  CHECK(disk->nodeTypeName() == "Disk2D");
  CHECK(std::any_cast<float>(field(disk, "outerRadius")) == doctest::Approx(5.0f));
}

TEST_CASE("proto_shadow_classicvrml") {
  const char *vrml =
      "#X3D V4.0 utf8\n"
      "PROTO Box [ ] { Sphere { } }\n"
      "Shape { geometry Box { } }\n";
  auto doc = codec::parseDocument(vrml, codec::Encoding::ClassicVRML);
  CHECK(hasBuiltinShadow(doc, "Box"));
  CHECK(doc.scene.protoDeclarations.empty());
}

TEST_CASE("proto_shadow_json") {
  const char *json =
      "{ \"X3D\": { \"@version\": \"4.0\", \"Scene\": { \"-children\": ["
      "{ \"ProtoDeclare\": { \"@name\": \"Box\", \"ProtoBody\": { "
      "\"-children\": [ { \"Sphere\": {} } ] } } },"
      "{ \"Shape\": { \"-geometry\": [ { \"Box\": {} } ] } }"
      "] } } }";
  auto doc = codec::parseDocument(json, codec::Encoding::JSON);
  CHECK(hasBuiltinShadow(doc, "Box"));
  CHECK(doc.scene.protoDeclarations.empty());
}

// A non-colliding PROTO is untouched (no false positive).
TEST_CASE("proto_shadow_no_false_positive") {
  const char *xml =
      "<X3D version='4.0'><Scene>"
      "<ProtoDeclare name='MyWidget'><ProtoBody><Sphere/></ProtoBody>"
      "</ProtoDeclare></Scene></X3D>";
  auto doc = codec::parseDocument(xml);
  CHECK_FALSE(hasBuiltinShadow(doc, "MyWidget"));
  CHECK(doc.scene.protoDeclarations.size() == 1);
}

// ── DIAG-UNKNOWN-NODE: each reader surfaces a misspelled node element ────────
TEST_CASE("unknown_node_xml") {
  auto doc = codec::parseDocument(
      "<X3D version='4.0'><Scene><Shape><BogusNode/></Shape></Scene></X3D>");
  CHECK(hasUnknownNode(doc));
}

TEST_CASE("unknown_node_classicvrml") {
  auto doc = codec::parseDocument("#X3D V4.0 utf8\nBogusNode { }\n",
                                  codec::Encoding::ClassicVRML);
  CHECK(hasUnknownNode(doc));
}

TEST_CASE("unknown_node_json") {
  auto doc = codec::parseDocument(
      "{ \"X3D\": { \"@version\": \"4.0\", \"Scene\": { \"-children\": ["
      "{ \"BogusNode\": {} } ] } } }",
      codec::Encoding::JSON);
  CHECK(hasUnknownNode(doc));
}

// A clean document emits no reader-recovery warnings.
TEST_CASE("unknown_node_clean_no_warning") {
  auto doc = codec::parseDocument(
      "<X3D version='4.0'><Scene><Shape><Box/></Shape></Scene></X3D>");
  CHECK(doc.readerWarnings.empty());
}

// A ROUTE among a node's children is a scene (or PROTO body) statement, not an
// unknown node: it is kept and not diagnosed (it used to be dropped silently).
TEST_CASE("nested_route_xml_is_kept") {
  auto doc = codec::parseDocument(
      "<X3D version='4.0'><Scene><TimeSensor DEF='T'/><Transform>"
      "<TouchSensor DEF='S'/>"
      "<ROUTE fromNode='S' fromField='isOver' toNode='T' toField='enabled'/>"
      "</Transform>"
      "<ProtoDeclare name='P'><ProtoBody><Group><TimeSensor DEF='A'/><TimeSensor DEF='B'/>"
      "<ROUTE fromNode='A' fromField='isActive' toNode='B' toField='enabled'/>"
      "</Group></ProtoBody></ProtoDeclare></Scene></X3D>");
  CHECK_FALSE(hasUnknownNode(doc));
  REQUIRE(doc.scene.routes.size() == 1);
  CHECK(doc.scene.routes[0].fromNode == "S");
  CHECK(doc.scene.routes[0].toField == "enabled");
  REQUIRE(doc.scene.protoDeclarations.size() == 1);
  REQUIRE(doc.scene.protoDeclarations[0]->body.routes.size() == 1);
  CHECK(doc.scene.protoDeclarations[0]->body.routes[0].fromNode == "A");
}

// ── DIAG-PROFILE-COERCE: unknown token diagnosed + preserved on write ────────
TEST_CASE("profile_coerce_diagnosed_and_preserved_xml") {
  auto doc = codec::parseDocument(
      "<X3D profile='Interchang' version='4.0'><Scene><Shape><Box/></Shape>"
      "</Scene></X3D>");
  CHECK(hasProfileCoerced(doc));
  // Resolved to Interchange (for profile-fit), but the authored token survives.
  CHECK(doc.profileName() == "Interchange");
  CHECK(doc.profileRaw == "Interchang");
  CHECK(doc.profileToken() == "Interchang");
  codec::XmlWriter w;
  CHECK(w.writeDocument(doc).find("profile=\"Interchang\"") != std::string::npos);
}

TEST_CASE("profile_coerce_classicvrml_and_json") {
  auto cv = codec::parseDocument("#X3D V4.0 utf8\nPROFILE Immersiv\nGroup { }\n",
                                 codec::Encoding::ClassicVRML);
  CHECK(hasProfileCoerced(cv));
  CHECK(cv.profileRaw == "Immersiv");

  auto js = codec::parseDocument(
      "{ \"X3D\": { \"@profile\": \"Fullish\", \"@version\": \"4.0\", "
      "\"Scene\": { \"-children\": [] } } }",
      codec::Encoding::JSON);
  CHECK(hasProfileCoerced(js));
  CHECK(js.profileRaw == "Fullish");
  codec::JsonWriter jw;
  CHECK(jw.writeDocument(js).find("\"Fullish\"") != std::string::npos);
}

// A canonical token is neither diagnosed nor altered.
TEST_CASE("profile_canonical_no_warning") {
  auto doc = codec::parseDocument(
      "<X3D profile='Immersive' version='4.0'><Scene><Shape><Box/></Shape>"
      "</Scene></X3D>");
  CHECK_FALSE(hasProfileCoerced(doc));
  CHECK(doc.profileName() == "Immersive");
  CHECK(doc.profileToken() == "Immersive");
}

// ── IMPORT-EXPORT-WIRE: a ROUTE to an IMPORTed AS name reaches the Inline ────
TEST_CASE("import_export_wire_route_to_imported_as_name") {
  const char *parent =
      "<X3D version='4.0'><Scene>"
      "<Inline DEF='Inl' url='child.x3d'/>"
      "<IMPORT inlineDEF='Inl' importedDEF='MoverOut' AS='M'/>"
      "<TimeSensor DEF='T'/>"
      "<ROUTE fromNode='T' fromField='fraction_changed' "
      "toNode='M' toField='set_translation'/>"
      "</Scene></X3D>";
  const char *child =
      "<X3D version='4.0'><Scene>"
      "<Transform DEF='Mover'><Shape><Box/></Shape></Transform>"
      "<EXPORT localDEF='Mover' AS='MoverOut'/>"
      "</Scene></X3D>";

  auto resolver = [&](const std::vector<std::string> &,
                      const std::string &) -> std::shared_ptr<runtime::Scene> {
    auto sub = codec::parseDocument(child);
    return std::make_shared<runtime::Scene>(std::move(sub.scene));
  };

  auto doc = codec::parseDocument(parent, codec::Encoding::Unknown, "",
                                  codec::localFileProtoResolver, resolver);

  // The AS alias is registered and points at the child's exported DEF.
  auto aliasIt = doc.scene.defs.find("M");
  REQUIRE(aliasIt != doc.scene.defs.end());
  REQUIRE(aliasIt->second != nullptr);
  CHECK(aliasIt->second->nodeTypeName() == "Transform");
  CHECK(aliasIt->second->getDEF() == "Mover");

  // The ROUTE now resolves both endpoints instead of dropping the imported one.
  bool wired = false;
  for (const auto &r : doc.scene.routes)
    if (r.toNode == "M") {
      CHECK(r.fromNode == "T");
      CHECK_FALSE(r.to.expired());
      wired = !r.to.expired();
    }
  CHECK(wired);
}

TEST_CASE("import_requires_explicit_export") {
  const char *parent =
      "<X3D version='4.0'><Scene>"
      "<Inline DEF='Inl' url='child.x3d'/>"
      "<IMPORT inlineDEF='Inl' importedDEF='Private' AS='Leak'/>"
      "</Scene></X3D>";
  const char *child =
      "<X3D version='4.0'><Scene><Transform DEF='Private'/></Scene></X3D>";
  auto resolver = [&](const std::vector<std::string> &,
                      const std::string &) -> std::shared_ptr<runtime::Scene> {
    auto sub = codec::parseDocument(child);
    return std::make_shared<runtime::Scene>(std::move(sub.scene));
  };
  auto doc = codec::parseDocument(parent, codec::Encoding::Unknown, "",
                                  codec::localFileProtoResolver, resolver);
  CHECK(doc.scene.defs.count("Leak") == 0);
}

TEST_CASE("inline_load_true_event_expands_content") {
  const char *parent =
      "<X3D version='4.0'><Scene>"
      "<Inline DEF='Inl' load='false' url='child.x3d'/>"
      "</Scene></X3D>";
  const char *child = "<X3D version='4.0'><Scene><Shape><Box/></Shape></Scene></X3D>";
  int resolves = 0;
  auto resolver = [&](const std::vector<std::string> &,
                      const std::string &) -> std::shared_ptr<runtime::Scene> {
    ++resolves;
    auto sub = codec::parseDocument(child);
    return std::make_shared<runtime::Scene>(std::move(sub.scene));
  };
  auto doc = codec::parseDocument(parent, codec::Encoding::Unknown, "",
                                  codec::localFileProtoResolver, resolver);
  REQUIRE(resolves == 0);
  auto inl = doc.scene.resolve("Inl");
  REQUIRE(inl != nullptr);
  runtime::X3DExecutionContext ctx;
  ctx.buildSceneGraph(doc.scene);
  ctx.buildFrom(doc.scene);
  runtime::attachStandardRuntime(doc.scene, ctx, nullptr, resolver);
  runtime::extract::SceneExtractor extractor(ctx, doc.scene);
  CHECK(extractor.fullSnapshot().added.empty());
  ctx.postEvent(inl.get(), "load", std::any(core::SFBool{true}));
  ctx.tick(1.0);
  CHECK(resolves == 1);
  CHECK(sceneHasType(doc.scene, "Shape"));
  CHECK(extractor.delta().added.size() == 1);
  CHECK(extractor.fullSnapshot().added.size() == 1);
}

TEST_CASE("inline_late_load_wires_import_route") {
  const char *parent =
      "<X3D version='4.0'><Scene>"
      "<Inline DEF='Inl' load='false' url='child.x3d'/>"
      "<IMPORT inlineDEF='Inl' importedDEF='Mover' AS='Imported'/>"
      "<Transform DEF='Driver'/>"
      "<ROUTE fromNode='Driver' fromField='translation_changed' "
      "toNode='Imported' toField='set_translation'/>"
      "</Scene></X3D>";
  const char *child =
      "<X3D version='4.0'><Scene><Transform DEF='Mover' visible='false'/>"
      "<TimeSensor DEF='Clock' cycleInterval='10'/>"
      "<ROUTE fromNode='Clock' fromField='isActive' "
      "toNode='Mover' toField='visible'/>"
      "<EXPORT localDEF='Mover'/></Scene></X3D>";
  auto resolver = [&](const std::vector<std::string> &,
                      const std::string &) -> std::shared_ptr<runtime::Scene> {
    auto sub = codec::parseDocument(child);
    return std::make_shared<runtime::Scene>(std::move(sub.scene));
  };
  auto doc = codec::parseDocument(parent, codec::Encoding::Unknown, "",
                                  codec::localFileProtoResolver, resolver);
  REQUIRE(doc.scene.resolve("Imported") == nullptr);
  auto inl = doc.scene.resolve("Inl");
  auto driver = doc.scene.resolve("Driver");
  REQUIRE(inl != nullptr);
  REQUIRE(driver != nullptr);
  runtime::X3DExecutionContext ctx;
  ctx.buildSceneGraph(doc.scene);
  ctx.buildFrom(doc.scene);
  runtime::attachStandardRuntime(doc.scene, ctx, nullptr, resolver);
  ctx.postEvent(inl.get(), "load", std::any(core::SFBool{true}));
  ctx.tick(1.0);
  auto target = std::dynamic_pointer_cast<nodes::Transform>(
      doc.scene.resolve("Imported"));
  REQUIRE(target != nullptr);
  ctx.postEvent(driver.get(), "translation", std::any(core::SFVec3f{1, 2, 3}));
  ctx.tick(2.0);
  CHECK(target->getVisible()); // newly enrolled TimeSensor fired the child ROUTE
  CHECK(target->getTranslation() == core::SFVec3f{1, 2, 3});
}

TEST_CASE("inline_nested_late_load_appears_in_delta") {
  const char *parent =
      "<X3D version='4.0'><Scene><Transform translation='5 0 0'>"
      "<Inline DEF='Inl' load='false' url='child.x3d'/>"
      "</Transform></Scene></X3D>";
  const char *child =
      "<X3D version='4.0'><Scene><Shape><Box/></Shape></Scene></X3D>";
  auto resolver = [&](const std::vector<std::string> &,
                      const std::string &) -> std::shared_ptr<runtime::Scene> {
    auto sub = codec::parseDocument(child);
    return std::make_shared<runtime::Scene>(std::move(sub.scene));
  };
  auto doc = codec::parseDocument(parent, codec::Encoding::Unknown, "",
                                  codec::localFileProtoResolver, resolver);
  auto inl = doc.scene.resolve("Inl");
  REQUIRE(inl != nullptr);
  runtime::X3DExecutionContext ctx;
  ctx.buildSceneGraph(doc.scene);
  ctx.buildFrom(doc.scene);
  runtime::attachStandardRuntime(doc.scene, ctx, nullptr, resolver);
  runtime::extract::SceneExtractor extractor(ctx, doc.scene);
  CHECK(extractor.fullSnapshot().added.empty());
  ctx.postEvent(inl.get(), "load", std::any(core::SFBool{true}));
  ctx.tick(1.0);
  auto delta = extractor.delta();
  REQUIRE(delta.added.size() == 1);
  CHECK(extractor.item(delta.added[0]).worldTransform.m[12] == doctest::Approx(5.0));
}

TEST_CASE("inline_load_unload_reload_detaches_content_and_reports_removal") {
  const char *parent =
      "<X3D version='4.0'><Scene><Viewpoint DEF='Outside'/>"
      "<Inline DEF='Inl' load='false' url='one.x3d'/>"
      "<IMPORT inlineDEF='Inl' importedDEF='Mover' AS='Imported'/>"
      "<Transform DEF='Driver'/>"
      "<ROUTE fromNode='Driver' fromField='translation_changed' "
      "toNode='Imported' toField='set_translation'/>"
      "</Scene></X3D>";
  const char *child =
      "<X3D version='4.0'><Scene><Transform DEF='Mover'>"
      "<Shape><Box/></Shape></Transform><Viewpoint DEF='Inside'/>"
      "<TimeSensor DEF='Clock' cycleInterval='10'/>"
      "<EXPORT localDEF='Mover'/></Scene></X3D>";
  int resolves = 0;
  auto resolver = [&](const std::vector<std::string> &,
                      const std::string &) -> std::shared_ptr<runtime::Scene> {
    ++resolves;
    auto sub = codec::parseDocument(child);
    return std::make_shared<runtime::Scene>(std::move(sub.scene));
  };
  auto doc = codec::parseDocument(parent, codec::Encoding::Unknown, "",
                                  codec::localFileProtoResolver, resolver);
  auto inl = std::dynamic_pointer_cast<nodes::Inline>(doc.scene.resolve("Inl"));
  auto driver = doc.scene.resolve("Driver");
  REQUIRE(inl != nullptr);
  REQUIRE(driver != nullptr);
  runtime::X3DExecutionContext ctx;
  ctx.buildSceneGraph(doc.scene);
  ctx.buildFrom(doc.scene);
  runtime::attachStandardRuntime(doc.scene, ctx, nullptr, resolver);
  runtime::extract::SceneExtractor extractor(ctx, doc.scene);
  extractor.fullSnapshot();

  ctx.postEvent(inl.get(), "load", std::any(core::SFBool{true}));
  ctx.tick(1.0);
  REQUIRE(extractor.delta().added.size() == 1);
  auto loaded = doc.scene.expandedInlineScenes.at(inl.get());
  auto mover = std::dynamic_pointer_cast<nodes::Transform>(loaded->resolve("Mover"));
  auto inside = std::dynamic_pointer_cast<nodes::Viewpoint>(loaded->resolve("Inside"));
  auto clock = std::dynamic_pointer_cast<nodes::TimeSensor>(loaded->resolve("Clock"));
  REQUIRE(mover != nullptr);
  REQUIRE(inside != nullptr);
  REQUIRE(clock != nullptr);
  ctx.postEvent(inside.get(), "set_bind", std::any(core::SFBool{true}));
  ctx.tick(2.0);
  CHECK(ctx.boundViewpoint() == inside.get());
  extractor.delta();

  ctx.postEvent(inl.get(), "load", std::any(core::SFBool{false}));
  ctx.postEvent(driver.get(), "translation", std::any(core::SFVec3f{4, 5, 6}));
  ctx.tick(3.0);
  CHECK(doc.scene.resolve("Imported") == nullptr);
  CHECK(ctx.boundViewpoint() != inside.get());
  CHECK_FALSE(inside->getIsBound());
  CHECK(extractor.delta().removed.size() == 1);
  CHECK(extractor.fullSnapshot().added.empty());
  CHECK_FALSE(sceneHasType(doc.scene, "Shape"));
  const auto before = mover->getTranslation();
  ctx.postEvent(driver.get(), "translation", std::any(core::SFVec3f{7, 8, 9}));
  ctx.postEvent(clock.get(), "enabled", std::any(core::SFBool{false}));
  ctx.tick(4.0);
  CHECK(mover->getTranslation() == before);
  CHECK(clock->getEnabled());
  CHECK(ctx.writeField(mover.get(), "translation", std::any(core::SFVec3f{1, 1, 1})) ==
        runtime::FieldWriteResult::DetachedNode);

  ctx.postEvent(inl.get(), "load", std::any(core::SFBool{true}));
  ctx.tick(5.0);
  CHECK(resolves == 2);
  CHECK(extractor.delta().added.size() == 1);
  CHECK(doc.scene.resolve("Imported") != nullptr);
  auto replacement = std::dynamic_pointer_cast<nodes::Transform>(doc.scene.resolve("Imported"));
  REQUIRE(replacement != nullptr);
  ctx.postEvent(driver.get(), "translation", std::any(core::SFVec3f{2, 3, 4}));
  ctx.tick(6.0);
  CHECK(replacement->getTranslation() == core::SFVec3f{2, 3, 4});
}

TEST_CASE("inline_url_change_replaces_loaded_content") {
  const char *parent =
      "<X3D version='4.0'><Scene>"
      "<Inline DEF='Inl' url='one.x3d'/>"
      "<IMPORT inlineDEF='Inl' importedDEF='Mover' AS='Imported'/>"
      "</Scene></X3D>";
  auto resolver = [&](const std::vector<std::string> &urls,
                      const std::string &) -> std::shared_ptr<runtime::Scene> {
    const bool second = !urls.empty() && urls.front() == "two.x3d";
    const char *first =
        "<X3D version='4.0'><Scene><Transform DEF='Mover'>"
        "<Shape><Box/></Shape></Transform><EXPORT localDEF='Mover'/></Scene></X3D>";
    const char *next =
        "<X3D version='4.0'><Scene><Transform DEF='Mover'>"
        "<Shape><Sphere/></Shape></Transform><EXPORT localDEF='Mover'/></Scene></X3D>";
    auto sub = codec::parseDocument(second ? next : first);
    return std::make_shared<runtime::Scene>(std::move(sub.scene));
  };
  auto doc = codec::parseDocument(parent, codec::Encoding::Unknown, "",
                                  codec::localFileProtoResolver, resolver);
  auto inl = std::dynamic_pointer_cast<nodes::Inline>(doc.scene.resolve("Inl"));
  auto old = doc.scene.resolve("Imported");
  REQUIRE(inl != nullptr);
  REQUIRE(old != nullptr);
  runtime::X3DExecutionContext ctx;
  ctx.buildSceneGraph(doc.scene);
  ctx.buildFrom(doc.scene);
  runtime::attachStandardRuntime(doc.scene, ctx, nullptr, resolver);
  runtime::extract::SceneExtractor extractor(ctx, doc.scene);
  REQUIRE(extractor.fullSnapshot().added.size() == 1);
  ctx.postEvent(inl.get(), "url", std::any(core::MFString{"two.x3d"}));
  ctx.tick(1.0);
  auto delta = extractor.delta();
  CHECK(delta.removed.size() == 1);
  CHECK(delta.added.size() == 1);
  CHECK(doc.scene.resolve("Imported") != old);
  CHECK(sceneHasType(doc.scene, "Sphere"));
  CHECK_FALSE(sceneHasType(doc.scene, "Box"));
}

TEST_CASE("nested_inline_loaded_with_parent_can_unload") {
  const char *parent =
      "<X3D version='4.0'><Scene><Inline DEF='Outer' load='false' url='outer.x3d'/>"
      "</Scene></X3D>";
  std::function<std::shared_ptr<runtime::Scene>(
      const std::vector<std::string> &, const std::string &)> resolver;
  resolver = [&](const std::vector<std::string> &urls,
                 const std::string &) -> std::shared_ptr<runtime::Scene> {
    const bool outer = !urls.empty() && urls.front() == "outer.x3d";
    const char *xml = outer
        ? "<X3D version='4.0'><Scene><Inline DEF='Inner' url='inner.x3d'/></Scene></X3D>"
        : "<X3D version='4.0'><Scene><Shape><Box/></Shape></Scene></X3D>";
    auto sub = codec::parseDocument(xml, codec::Encoding::Unknown, "",
                                    codec::localFileProtoResolver, resolver);
    return std::make_shared<runtime::Scene>(std::move(sub.scene));
  };
  auto doc = codec::parseDocument(parent, codec::Encoding::Unknown, "",
                                  codec::localFileProtoResolver, resolver);
  auto outer = std::dynamic_pointer_cast<nodes::Inline>(doc.scene.resolve("Outer"));
  REQUIRE(outer != nullptr);
  runtime::X3DExecutionContext ctx;
  ctx.buildSceneGraph(doc.scene);
  ctx.buildFrom(doc.scene);
  runtime::attachStandardRuntime(doc.scene, ctx, nullptr, resolver);
  runtime::extract::SceneExtractor extractor(ctx, doc.scene);
  extractor.fullSnapshot();
  ctx.postEvent(outer.get(), "load", std::any(core::SFBool{true}));
  ctx.tick(1.0);
  REQUIRE(extractor.delta().added.size() == 1);
  std::shared_ptr<nodes::Inline> inner;
  for (const auto &[_, original] : doc.scene.expandedInlines)
    if (original->getDEF() == "Inner")
      inner = std::dynamic_pointer_cast<nodes::Inline>(original);
  REQUIRE(inner != nullptr);
  ctx.postEvent(inner.get(), "load", std::any(core::SFBool{false}));
  ctx.tick(2.0);
  CHECK(extractor.delta().removed.size() == 1);
  CHECK_FALSE(sceneHasType(doc.scene, "Shape"));
  ctx.postEvent(outer.get(), "load", std::any(core::SFBool{false}));
  ctx.tick(3.0);
  CHECK(doc.scene.expandedInlines.empty());
}

TEST_CASE("inline_repeated_replacement_drops_unowned_nodes") {
  const char *parent =
      "<X3D version='4.0'><Scene><Inline DEF='Inl' load='false' url='a.x3d'/>"
      "</Scene></X3D>";
  auto resolver = [&](const std::vector<std::string> &,
                      const std::string &) -> std::shared_ptr<runtime::Scene> {
    auto sub = codec::parseDocument(
        "<X3D version='4.0'><Scene><TimeSensor cycleInterval='10'/>"
        "<Shape><Box/></Shape></Scene></X3D>");
    return std::make_shared<runtime::Scene>(std::move(sub.scene));
  };
  auto doc = codec::parseDocument(parent, codec::Encoding::Unknown, "",
                                  codec::localFileProtoResolver, resolver);
  auto inl = std::dynamic_pointer_cast<nodes::Inline>(doc.scene.resolve("Inl"));
  REQUIRE(inl != nullptr);
  runtime::X3DExecutionContext ctx;
  ctx.buildSceneGraph(doc.scene);
  ctx.buildFrom(doc.scene);
  runtime::attachStandardRuntime(doc.scene, ctx, nullptr, resolver);
  runtime::extract::SceneExtractor extractor(ctx, doc.scene);
  extractor.fullSnapshot();
  ctx.postEvent(inl.get(), "load", std::any(core::SFBool{true}));
  ctx.tick(1.0);
  extractor.delta();
  for (int i = 0; i < 12; ++i) {
    ctx.postEvent(inl.get(), "url",
                  std::any(core::MFString{i % 2 ? "a.x3d" : "b.x3d"}));
    ctx.tick(2.0 + i);
    auto d = extractor.delta();
    CHECK(d.added.size() == 1);
    CHECK(d.removed.size() == 1);
  }
}

TEST_CASE("metadata_worldinfo_xml_roundtrip") {
  const char *xml =
      "<X3D version='4.0'><Scene><WorldInfo title='audit' info='\"one\"'>"
      "<MetadataSet containerField='metadata' name='set' reference='test'>"
      "<MetadataBoolean name='b' value='true false'/>"
      "<MetadataDouble name='d' value='1.25'/>"
      "<MetadataFloat name='f' value='2.5'/>"
      "<MetadataInteger name='i' value='3'/>"
      "<MetadataString name='s' value='\"text\"'/>"
      "</MetadataSet></WorldInfo></Scene></X3D>";
  auto doc = codec::parseDocument(xml);
  codec::XmlWriter writer;
  auto back = codec::parseDocument(writer.writeDocument(doc));
  for (const char *type : {"WorldInfo", "MetadataSet", "MetadataBoolean",
                           "MetadataDouble", "MetadataFloat", "MetadataInteger",
                           "MetadataString"})
    CHECK(sceneHasType(back.scene, type));
}

TEST_CASE("proto_peer_timesensor_remains_active") {
  const char *xml =
      "<X3D version='4.0'><Scene>"
      "<ProtoDeclare name='P'><ProtoBody>"
      "<Transform DEF='X'/><TimeSensor DEF='T' cycleInterval='10'/>"
      "<ROUTE fromNode='T' fromField='isActive' toNode='X' toField='visible'/>"
      "</ProtoBody></ProtoDeclare><ProtoInstance name='P'/>"
      "</Scene></X3D>";
  auto doc = codec::parseDocument(xml);
  nodes::TimeSensor *peer = nullptr;
  for (const auto &r : doc.scene.resolvedProtoRoutes)
    if (r.from && r.from->nodeTypeName() == "TimeSensor")
      peer = dynamic_cast<nodes::TimeSensor *>(r.from.get());
  REQUIRE(peer != nullptr);
  auto control = std::make_shared<nodes::TimeSensor>();
  control->setCycleInterval(10.0);
  doc.scene.addRootNode(control);
  runtime::X3DExecutionContext ctx;
  ctx.buildSceneGraph(doc.scene);
  runtime::attachStandardRuntime(doc.scene, ctx);
  ctx.tick(0.1);
  CHECK(control->X3DTimeDependentNode::getIsActive());
  CHECK(peer->X3DTimeDependentNode::getIsActive());
}
