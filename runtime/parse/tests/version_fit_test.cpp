#include "doctest/doctest.h"
// X3D 4.1 typed bindings (ADR-0077): 4.1 nodes and fields read as typed nodes in
// every encoding, round-trip, and are reported when the declared version is older.
#include "X3DCodecs.hpp"
#include "X3DParse.hpp"
#include "X3DVersionFit.hpp"
#include "x3d/nodes/Box.hpp"
#include "x3d/nodes/EnvironmentLight.hpp"
#include "x3d/nodes/IndexedFaceSet.hpp"
#include "x3d/nodes/PhysicalMaterial.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/Tangent.hpp"
#include "x3d/nodes/TextureProjectorParallel.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"

#include <memory>
#include <string>

using x3d::codec::Encoding;
using x3d::core::X3DVersion;
using Kind = x3d::runtime::ReaderWarning::Kind;

namespace {

std::string scene41(const std::string &version) {
  return "<X3D profile='Full' version='" + version + "'><Scene>"
         "<Shape><IndexedFaceSet coordIndex='0 1 2 -1'>"
         "<Coordinate point='0 0 0 1 0 0 0 1 0'/>"
         "<Tangent vector='1 0 0 1 1 0 0 1 1 0 0 1'/>"
         "</IndexedFaceSet></Shape>"
         "<EnvironmentLight DEF='E' intensity='0.5' diffuseCoefficients='1 2 3'>"
         "<ImageCubeMapTexture containerField='specularTexture' url='\"a.png\"'/>"
         "</EnvironmentLight>"
         "<TextureProjectorParallel upVector='0 0 1'/>"
         "</Scene></X3D>";
}

x3d::runtime::X3DDocument read(const std::string &text, Encoding enc) {
  return x3d::codec::parseDocument(text, enc);
}

std::vector<std::string> versionWarnings(const x3d::runtime::X3DDocument &doc) {
  std::vector<std::string> out;
  for (const auto &w : doc.readerWarnings)
    if (w.kind == Kind::Version) out.push_back(w.detail);
  return out;
}

bool hasUnknown(const x3d::runtime::X3DDocument &doc) {
  for (const auto &w : doc.readerWarnings)
    if (w.kind == Kind::UnknownNode) return true;
  return false;
}

} // namespace

TEST_CASE("node types and fields carry the version that introduced them") {
  CHECK(x3d::nodes::Box().introducedIn() == X3DVersion{3, 0});
  CHECK(x3d::nodes::PhysicalMaterial().introducedIn() == X3DVersion{4, 0});
  CHECK(x3d::nodes::EnvironmentLight().introducedIn() == X3DVersion{4, 1});
  for (const char *name : {"EnvironmentLight", "FontLibrary", "HAnimPose",
                           "InlineGeometry", "RenderedTexture", "Tangent"}) {
    auto node = x3d::nodes::createX3DNode(name);
    REQUIRE_MESSAGE(node, name);
    CHECK(node->introducedIn() == X3DVersion{4, 1});
  }
  x3d::nodes::IndexedFaceSet ifs;
  for (const auto &f : ifs.fields()) {
    if (f.x3dName == "tangent") CHECK(f.since == X3DVersion{4, 1});
    if (f.x3dName == "coord") CHECK(f.since == X3DVersion{3, 0});
  }
  CHECK(x3d::runtime::X3DDocument().version == "4.1");
}

TEST_CASE("parseX3DVersion reads MAJOR.MINOR and nothing else") {
  CHECK(x3d::runtime::parseX3DVersion("4.1") == X3DVersion{4, 1});
  CHECK(x3d::runtime::parseX3DVersion("3.3") == X3DVersion{3, 3});
  CHECK_FALSE(x3d::runtime::parseX3DVersion("4"));
  CHECK_FALSE(x3d::runtime::parseX3DVersion("4.x"));
  CHECK_FALSE(x3d::runtime::parseX3DVersion("V4.0"));
}

TEST_CASE("an X3D 4.1 document reads as typed nodes with no warnings") {
  auto doc = read(scene41("4.1"), Encoding::XML);
  CHECK_FALSE(hasUnknown(doc));
  CHECK(versionWarnings(doc).empty());
  CHECK(doc.scene.opaqueChildren.empty());
  REQUIRE(doc.scene.rootNodes.size() == 3);

  auto shape = std::dynamic_pointer_cast<x3d::nodes::Shape>(doc.scene.rootNodes[0]);
  REQUIRE(shape);
  auto ifs = std::dynamic_pointer_cast<x3d::nodes::IndexedFaceSet>(shape->getGeometry());
  REQUIRE(ifs);
  auto tangent = std::dynamic_pointer_cast<x3d::nodes::Tangent>(ifs->getTangent());
  REQUIRE(tangent);
  CHECK(tangent->getVector().size() == 3);

  auto env = std::dynamic_pointer_cast<x3d::nodes::EnvironmentLight>(doc.scene.rootNodes[1]);
  REQUIRE(env);
  CHECK(env->getIntensity() == doctest::Approx(0.5));
  CHECK(env->getDiffuseCoefficients().size() == 3);
  REQUIRE(env->getSpecularTexture());
  CHECK(env->getSpecularTexture()->nodeTypeName() == "ImageCubeMapTexture");

  CHECK(x3d::runtime::requiredVersion(doc.scene) == X3DVersion{4, 1});
}

TEST_CASE("4.1 content round-trips XML -> ClassicVRML -> JSON -> XML unchanged") {
  auto doc = read(scene41("4.1"), Encoding::XML);
  const std::string canon0 = x3d::codec::CanonicalXmlWriter().writeDocument(doc);
  CHECK(canon0.find("<Tangent") != std::string::npos);
  CHECK(canon0.find("diffuseCoefficients='1 2 3'") != std::string::npos);

  auto vrmlDoc = read(x3d::codec::VrmlWriter().writeDocument(doc), Encoding::ClassicVRML);
  CHECK_FALSE(hasUnknown(vrmlDoc));
  CHECK(versionWarnings(vrmlDoc).empty());
  auto jsonDoc = read(x3d::codec::JsonWriter().writeDocument(vrmlDoc), Encoding::JSON);
  CHECK_FALSE(hasUnknown(jsonDoc));
  CHECK(versionWarnings(jsonDoc).empty());
  auto xmlDoc = read(x3d::codec::XmlWriter().writeDocument(jsonDoc), Encoding::XML);
  CHECK(x3d::codec::CanonicalXmlWriter().writeDocument(xmlDoc) == canon0);
}

TEST_CASE("4.1 nodes and fields in a 4.0 document are reported, once each") {
  for (Encoding enc : {Encoding::XML, Encoding::ClassicVRML, Encoding::JSON}) {
    auto doc41 = read(scene41("4.0"), Encoding::XML);
    std::string text;
    if (enc == Encoding::XML) text = scene41("4.0");
    else if (enc == Encoding::ClassicVRML) text = x3d::codec::VrmlWriter().writeDocument(doc41);
    else text = x3d::codec::JsonWriter().writeDocument(doc41);
    auto doc = read(text, enc);
    CAPTURE(static_cast<int>(enc));
    CHECK(doc.version == "4.0");
    const auto warnings = versionWarnings(doc);
    REQUIRE(warnings.size() == 4);
    CHECK(warnings[0] == "EnvironmentLight needs X3D 4.1 but the document declares 4.0");
    CHECK(warnings[1] == "IndexedFaceSet.tangent needs X3D 4.1 but the document declares 4.0");
    CHECK(warnings[2] == "Tangent needs X3D 4.1 but the document declares 4.0");
    CHECK(warnings[3] == "TextureProjectorParallel.upVector needs X3D 4.1 but the document declares 4.0");
    // Still typed: the warning reports, it does not drop or downgrade.
    CHECK(std::dynamic_pointer_cast<x3d::nodes::EnvironmentLight>(doc.scene.rootNodes[1]));
  }
}

TEST_CASE("older declared versions report 4.0 content too") {
  auto doc = read("<X3D profile='Full' version='3.3'><Scene>"
                  "<Shape><Appearance><PhysicalMaterial/></Appearance></Shape>"
                  "</Scene></X3D>",
                  Encoding::XML);
  const auto warnings = versionWarnings(doc);
  REQUIRE(warnings.size() == 1);
  CHECK(warnings[0] == "PhysicalMaterial needs X3D 4.0 but the document declares 3.3");
  CHECK(x3d::runtime::requiredVersion(doc.scene) == X3DVersion{4, 0});
}

TEST_CASE("a generated default never counts as authored") {
  // TextureProjectorParallel.upVector is 4.1, but an unset one is not authored.
  auto doc = read("<X3D profile='Full' version='4.0'><Scene>"
                  "<TextureProjectorParallel/></Scene></X3D>",
                  Encoding::XML);
  CHECK(versionWarnings(doc).empty());
  CHECK(x3d::runtime::requiredVersion(doc.scene) == X3DVersion{4, 0});
}

TEST_CASE("an undeclared version is not compared, but the needed version is known") {
  auto doc = read("<Scene><EnvironmentLight/></Scene>", Encoding::XML);
  CHECK_FALSE(doc.versionDeclared);
  CHECK(versionWarnings(doc).empty());
  CHECK(x3d::runtime::requiredVersion(doc.scene) == X3DVersion{4, 1});
}

TEST_CASE("PROTO bodies count toward the needed version") {
  auto doc = read("<X3D profile='Full' version='4.0'><Scene>"
                  "<ProtoDeclare name='P'><ProtoBody><EnvironmentLight/></ProtoBody>"
                  "</ProtoDeclare></Scene></X3D>",
                  Encoding::XML);
  const auto warnings = versionWarnings(doc);
  REQUIRE(warnings.size() == 1);
  CHECK(warnings[0] == "EnvironmentLight needs X3D 4.1 but the document declares 4.0");
}
