// proto_writer_parity_test.cpp
// JSON and VRML writers must re-emit captured PROTO declarations and instances,
// suppressing the expansion's spliced primary — parity with XmlWriter.
#include "X3DParse.hpp"
#include "JsonWriter.hpp"
#include "XmlWriter.hpp"
#include "CanonicalXmlWriter.hpp"
#include "VrmlWriter.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DRuntime.hpp"

#include "doctest/doctest.h"
#include <algorithm>
#include <cstddef>
#include <string>

using namespace x3d;
using namespace x3d::core;
using namespace x3d::nodes;
// Test: mixed scalar + node fieldValues must produce a single "fieldValue"
// JSON array, not two duplicate "fieldValue" keys (invalid JSON).
static void jsonMixedFieldValueValidJsonTest() {
  using namespace x3d::runtime;

  // Build a declaration with a scalar field (size:SFVec3f) and a node field
  // (geom:SFNode).
  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "Mix";
  {
    ProtoField f;
    f.name = "size";
    f.type = X3DFieldType::SFVec3f;
    f.access = AccessType::InputOutput;
    decl->interface.push_back(f);
  }
  {
    ProtoField f;
    f.name = "geom";
    f.type = X3DFieldType::SFNode;
    f.access = AccessType::InputOutput;
    decl->interface.push_back(f);
  }

  Scene scene;
  scene.protoDeclarations.push_back(decl);

  // A primary node stands in for the expanded instance.
  auto primary = createX3DNode("Group");
  scene.rootNodes.push_back(primary);

  x3d::runtime::ProtoInstance src;
  src.name = "Mix";
  src.declaration = decl;
  {
    ProtoFieldValue v;
    v.name = "size";
    v.value = std::any(SFVec3f{1.f, 2.f, 3.f});
    src.fieldValues.push_back(v);
  }
  {
    ProtoFieldValue v;
    v.name = "geom";
    v.nodeValue.push_back(createX3DNode("Box"));
    src.fieldValues.push_back(v);
  }
  scene.expandedSources[primary.get()] = src;

  x3d::runtime::X3DDocument doc;
  doc.scene = std::move(scene);

  std::string js = codec::JsonWriter().writeDocument(doc);

  // Count occurrences of "fieldValue" in the output — must be exactly 1.
  std::size_t n = 0, p = 0;
  while ((p = js.find("\"fieldValue\"", p)) != std::string::npos) {
    ++n;
    p += 5;
  }
  CHECK((n == 1 && "duplicate \"fieldValue\" keys in JSON output"));

  // Re-parse to confirm the output is structurally valid JSON.
  auto doc2 = codec::parseDocument(js); // must not throw
  (void)doc2;
}

TEST_CASE("proto_writer_parity_test") {
  const char *xml =
      "<X3D version='4.0'><Scene>"
      "<ProtoDeclare name='P'><ProtoBody><Box/></ProtoBody></ProtoDeclare>"
      "<ProtoInstance name='P' DEF='A'/></Scene></X3D>";
  auto doc = codec::parseDocument(xml);

  std::string js = codec::JsonWriter().writeDocument(doc);
  CHECK((js.find("ProtoDeclare") != std::string::npos));
  CHECK((js.find("ProtoInstance") != std::string::npos));
  // The instance's expansion (a Box) is re-emitted as a ProtoInstance, not as a
  // second standalone Box at scene root. The declaration body's Box may appear.
  CHECK((js.find("\"@name\": \"P\"") != std::string::npos ||
         js.find("\"@name\":\"P\"") != std::string::npos));

  std::string vr = codec::VrmlWriter().writeDocument(doc);
  CHECK((vr.find("PROTO P") != std::string::npos));
  // The scene-level instance is re-emitted as a proto instance `P { ... }`.
  CHECK((vr.find("P {") != std::string::npos));
  jsonMixedFieldValueValidJsonTest();

  return;
}

TEST_CASE("nested instance IS survives every writer") {
  auto before = codec::parseDocument(
      "<X3D version='4.0'><Scene><ProtoDeclare name='Leaf'><ProtoInterface>"
      "<field name='size' type='SFVec3f' accessType='initializeOnly' value='2 2 2'/>"
      "</ProtoInterface><ProtoBody><Box><IS><connect nodeField='size' protoField='size'/>"
      "</IS></Box></ProtoBody></ProtoDeclare><ProtoDeclare name='Outer'><ProtoInterface>"
      "<field name='size' type='SFVec3f' accessType='initializeOnly' value='5 5 5'/>"
      "</ProtoInterface><ProtoBody><ProtoInstance name='Leaf'><IS>"
      "<connect nodeField='size' protoField='size'/></IS></ProtoInstance></ProtoBody>"
      "</ProtoDeclare><ProtoInstance name='Outer'/></Scene></X3D>");
  auto size = [](const runtime::X3DDocument &doc) {
    auto root = doc.scene.rootNodes.at(0);
    REQUIRE(root->nodeTypeName() == "Box");
    const auto &fields = root->fields();
    auto f = std::find_if(fields.begin(), fields.end(),
        [](const auto &field) { return field.x3dName == "size"; });
    REQUIRE(f != fields.end());
    return std::any_cast<SFVec3f>(f->get(*root));
  };
  CHECK(size(before).x == 5.f);
  const std::pair<std::string, codec::Encoding> outputs[] = {
      {codec::XmlWriter().writeDocument(before), codec::Encoding::XML},
      {codec::CanonicalXmlWriter().writeDocument(before), codec::Encoding::XML},
      {codec::JsonWriter().writeDocument(before), codec::Encoding::JSON},
      {codec::VrmlWriter().writeDocument(before), codec::Encoding::ClassicVRML}};
  for (const auto &[output, encoding] : outputs) {
    CAPTURE(output);
    auto after = codec::parseDocument(output, encoding);
    const auto &connections = after.scene.protoDeclarations.at(1)->body.nestedInstances.at(0).isConnections;
    CHECK(connections.size() == 1);
    if (!connections.empty()) {
      CHECK(connections[0].nodeField == "size");
      CHECK(connections[0].protoField == "size");
    }
    auto actual = size(after);
    CHECK(actual.x == 5.f);
    CHECK(actual.y == 5.f);
    CHECK(actual.z == 5.f);
  }
}

TEST_CASE("writers retain explicit empty prototype node overrides") {
  std::string type, node, field, initial;
  SUBCASE("SFNode NULL replaces a nonempty default") {
    type = "SFNode"; node = "Shape"; field = "geometry"; initial = "<Box/>";
  }
  SUBCASE("MFNode empty list replaces nonempty defaults") {
    type = "MFNode"; node = "Group"; field = "children";
    initial = "<Group/><Shape/>";
  }
  auto before = codec::parseDocument(
      "<X3D version='4.0'><Scene><ProtoDeclare name='P'><ProtoInterface>"
      "<field name='slot' type='" + type + "' accessType='initializeOnly'>" + initial +
      "</field></ProtoInterface><ProtoBody><" + node + "><IS><connect nodeField='" +
      field + "' protoField='slot'/></IS></" + node + "></ProtoBody></ProtoDeclare>"
      "<ProtoInstance name='P'><fieldValue name='slot'/></ProtoInstance></Scene></X3D>");
  auto checkEmpty = [&](const runtime::X3DDocument &doc) {
    const auto &values = doc.scene.protoInstances.at(0).fieldValues;
    CHECK(values.size() == 1);
    if (!values.empty()) {
      CHECK(values[0].name == "slot");
      CHECK(values[0].nodeValue.empty());
    }
    auto root = doc.scene.rootNodes.at(0);
    for (const auto &f : root->fields()) {
      if (f.x3dName != field) continue;
      const auto value = f.get(*root);
      if (type == "SFNode") CHECK_FALSE(std::any_cast<SFNode>(value));
      else CHECK(std::any_cast<MFNode>(value).empty());
    }
  };
  checkEmpty(before);
  const std::pair<std::string, codec::Encoding> outputs[] = {
      {codec::JsonWriter().writeDocument(before), codec::Encoding::JSON},
      {codec::VrmlWriter().writeDocument(before), codec::Encoding::ClassicVRML}};
  for (const auto &[output, encoding] : outputs) {
    CAPTURE(output);
    checkEmpty(codec::parseDocument(output, encoding));
  }
}
