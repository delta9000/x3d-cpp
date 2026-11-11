#include "doctest/doctest.h"
// Unknown-node passthrough: the XML reader keeps an element of unknown type as
// an opaque runtime::UnknownNode and the XML writers re-emit it unchanged.
#include "X3DCodecs.hpp"
#include "X3DParse.hpp"
#include "x3d/nodes/IndexedFaceSet.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/Transform.hpp"

#include <memory>
#include <string>

using x3d::runtime::UnknownNode;
using Kind = x3d::runtime::ReaderWarning::Kind;

namespace {

const char *const kScene =
    "<X3D profile='Full' version='4.1'><Scene>"
    "<Transform DEF='T'>"
    "<Shape><IndexedFaceSet coordIndex='0 1 2 -1'>"
    "<Coordinate point='0 0 0 1 0 0 0 1 0'/>"
    "<Tangent vector='1 0 0 1'/>"
    "</IndexedFaceSet></Shape>"
    "<EnvironmentLight DEF='E' intensity='0.5' zzz='a &lt; b'>"
    "<ImageCubeMapTexture DEF='I' containerField='diffuseTexture' url='\"a.png\"'/>"
    "</EnvironmentLight>"
    "</Transform>"
    "<EnvironmentLight USE='E'/>"
    "<InstancedShape translations='0 0 0 1 1 1'><Box DEF='B'/></InstancedShape>"
    "<Shape><Box USE='B'/></Shape>"
    "</Scene></X3D>";

x3d::runtime::X3DDocument read(const std::string &xml) {
  return x3d::codec::parseDocument(xml, x3d::codec::Encoding::XML);
}

std::size_t count(const std::string &hay, const std::string &needle) {
  std::size_t n = 0;
  for (auto p = hay.find(needle); p != std::string::npos;
       p = hay.find(needle, p + needle.size()))
    ++n;
  return n;
}

} // namespace

TEST_CASE("XML reader keeps unknown elements as opaque nodes") {
  auto doc = read(kScene);
  REQUIRE(doc.scene.rootNodes.size() == 4);

  auto t = std::dynamic_pointer_cast<x3d::nodes::Transform>(doc.scene.rootNodes[0]);
  REQUIRE(t);
  REQUIRE(t->getChildren().size() == 2);
  auto env = std::dynamic_pointer_cast<UnknownNode>(t->getChildren()[1]);
  REQUIRE(env);
  CHECK(env->nodeTypeName() == "EnvironmentLight");
  CHECK(env->getDEF() == "E");
  REQUIRE(env->attributes.size() == 2);
  CHECK(env->attributes[0].first == "intensity");
  CHECK(env->attributes[0].second == "0.5");
  CHECK(env->attributes[1].second == "a < b");
  CHECK(env->fields().empty());

  // A known child of an unknown node is read normally and its DEF resolves.
  REQUIRE(env->children.size() == 1);
  CHECK(env->children[0].containerField == "diffuseTexture");
  CHECK(env->children[0].node->nodeTypeName() == "ImageCubeMapTexture");
  CHECK(doc.scene.resolve("I") == env->children[0].node);

  // USE of an unknown node shares identity.
  CHECK(doc.scene.rootNodes[1] == t->getChildren()[1]);
  CHECK(doc.scene.resolve("E") == doc.scene.rootNodes[1]);

  // A DEF inside an unknown node is usable from known content.
  auto shape = std::dynamic_pointer_cast<x3d::nodes::Shape>(doc.scene.rootNodes[3]);
  REQUIRE(shape);
  CHECK(shape->getGeometry() == doc.scene.resolve("B"));

  std::size_t kept = 0;
  for (const auto &w : doc.readerWarnings)
    if (w.kind == Kind::UnknownNode &&
        w.detail.find("kept as opaque") != std::string::npos)
      ++kept;
  CHECK(kept == 3); // EnvironmentLight, Tangent, InstancedShape
}

TEST_CASE("an unknown child with no field on its parent does not displace a real child") {
  auto doc = read(kScene);
  auto t = std::dynamic_pointer_cast<x3d::nodes::Transform>(doc.scene.rootNodes[0]);
  auto shape = std::dynamic_pointer_cast<x3d::nodes::Shape>(t->getChildren()[0]);
  REQUIRE(shape);
  auto ifs = std::dynamic_pointer_cast<x3d::nodes::IndexedFaceSet>(shape->getGeometry());
  REQUIRE(ifs);
  REQUIRE(ifs->getCoord());
  CHECK(ifs->getCoord()->nodeTypeName() == "Coordinate");
  auto parked = doc.scene.opaqueChildren.find(ifs.get());
  REQUIRE(parked != doc.scene.opaqueChildren.end());
  REQUIRE(parked->second.size() == 1);
  CHECK(parked->second[0]->nodeTypeName() == "Tangent");
}

TEST_CASE("XML writers re-emit unknown nodes and the output is stable") {
  auto doc = read(kScene);
  const std::string once = x3d::codec::XmlWriter().writeDocument(doc);
  CHECK(count(once, "<EnvironmentLight DEF=\"E\" intensity=\"0.5\" zzz=\"a &lt; b\">") == 1);
  CHECK(count(once, "<EnvironmentLight USE=\"E\"/>") == 1);
  CHECK(count(once, "<Tangent vector=\"1 0 0 1\"/>") == 1);
  CHECK(count(once, "<InstancedShape translations=\"0 0 0 1 1 1\">") == 1);
  CHECK(count(once, "<Box DEF=\"B\"/>") == 1);
  CHECK(count(once, "containerField=\"diffuseTexture\"") == 1);

  auto again = read(once);
  CHECK(x3d::codec::XmlWriter().writeDocument(again) == once);

  const std::string canon = x3d::codec::CanonicalXmlWriter().writeDocument(doc);
  CHECK(count(canon, "<EnvironmentLight DEF='E'") == 1);
  CHECK(count(canon, "<Tangent vector='1 0 0 1'/>") == 1);
}

TEST_CASE("ClassicVRML and JSON writers keep only the type and DEF of an unknown node") {
  auto doc = read(kScene);
  const std::string vrml = x3d::codec::VrmlWriter().writeDocument(doc);
  CHECK(vrml.find("DEF E EnvironmentLight {") != std::string::npos);
  CHECK(vrml.find("intensity") == std::string::npos);
  const std::string json = x3d::codec::JsonWriter().writeDocument(doc);
  CHECK(json.find("\"EnvironmentLight\"") != std::string::npos);
  CHECK(json.find("@intensity") == std::string::npos);
}
