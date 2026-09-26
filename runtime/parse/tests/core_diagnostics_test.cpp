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

#include <any>
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
