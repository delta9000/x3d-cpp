// proto_roundtrip_test.cpp
// round-trip a <ProtoInstance>. parseDocument expands the instance to
// its primary node (a Box) and records the source instance in
// scene.expandedSources; the XML writer must re-emit the original
// <ProtoInstance ...> for that primary and suppress the expanded subtree,
// rather than serializing the spliced-in Box.
#include "X3DParse.hpp"
#include "XmlWriter.hpp"
#include "CanonicalXmlWriter.hpp"
#include "JsonWriter.hpp"
#include "VrmlWriter.hpp"

#include "doctest/doctest.h"
#include <string>

using namespace x3d;

TEST_CASE("proto_roundtrip_test") {
  const char *xml =
      "<X3D version='4.0'><Scene>"
      "<ProtoDeclare name='P'><ProtoBody><Box/></ProtoBody></ProtoDeclare>"
      "<ProtoInstance name='P' DEF='A'/></Scene></X3D>";
  auto doc = codec::parseDocument(xml); // expands to a Box root + expandedSources
  std::string out = codec::XmlWriter().writeDocument(doc);

  // The declaration is re-emitted (body included) and the instance is re-emitted;
  // the expansion's spliced Box is still suppressed via expandedSources.
  CHECK((out.find("<ProtoDeclare") != std::string::npos));
  CHECK((out.find("<ProtoBody") != std::string::npos));
  CHECK((out.find("<Box") != std::string::npos));     // the declaration body's Box
  CHECK((out.find("<ProtoInstance") != std::string::npos));
  CHECK((out.find("name=\"P\"") != std::string::npos));
  CHECK((out.find("DEF=\"A\"") != std::string::npos));
  // Exactly one Box element: the declaration body's, not a second from expansion.
  {
    std::size_t n = 0, p = 0;
    while ((p = out.find("<Box", p)) != std::string::npos) { ++n; p += 4; }
    CHECK((n == 1));
  }

  return;
}

TEST_CASE("ProtoBody IMPORT aliases survive every encoding without leaking scope") {
  const std::string xml = R"(<X3D version="4.0"><Scene>
    <ProtoDeclare name="P"><ProtoBody>
      <Group/><Inline DEF="Source"/>
      <IMPORT inlineDEF="Source" importedDEF="Clock" AS="LocalClock"/>
      <IMPORT inlineDEF="Source" importedDEF="Other"/>
      <ROUTE fromNode="LocalClock" fromField="time" toNode="Target" toField="set_time"/>
    </ProtoBody></ProtoDeclare>
    </Scene></X3D>)";
  const auto original = codec::parseDocument(xml);
  const std::vector<std::string> encoded = {
      codec::XmlWriter().writeDocument(original),
      codec::CanonicalXmlWriter().writeDocument(original),
      codec::JsonWriter().writeDocument(original),
      codec::VrmlWriter().writeDocument(original)};
  for (const auto &text : encoded) {
    CAPTURE(text);
    const auto doc = codec::parseDocument(text);
    CHECK(doc.scene.imports.empty());
    const auto proto = doc.scene.findProto("P");
    REQUIRE(proto);
    REQUIRE(proto->body.imports.size() == 2);
    CHECK(proto->body.imports[0].inlineDEF == "Source");
    CHECK(proto->body.imports[0].importedDEF == "Clock");
    CHECK(proto->body.imports[0].as == "LocalClock");
    CHECK(proto->body.imports[1].importedDEF == "Other");
    CHECK(proto->body.imports[1].as.empty());
    REQUIRE(proto->body.routes.size() == 1);
    CHECK(proto->body.routes[0].fromNode == "LocalClock");
  }
}

TEST_CASE("ProtoBody IMPORT runtime limitation is diagnosed on instantiation") {
  const auto doc = codec::parseDocument(R"(<X3D><Scene>
    <ProtoDeclare name="P"><ProtoBody><Group/><Inline DEF="Source"/>
    <IMPORT inlineDEF="Source" importedDEF="Clock"/>
    </ProtoBody></ProtoDeclare><ProtoInstance name="P"/>
    </Scene></X3D>)");
  bool diagnosed = false;
  for (const auto &warning : doc.protoWarnings)
    diagnosed |= warning.detail.find("per-instance import resolution") != std::string::npos;
  CHECK(diagnosed);
  CHECK(doc.scene.imports.empty());
}

TEST_CASE("JSON child-wrapped IMPORT stays in its prototype scope") {
  const auto doc = codec::parseDocument(R"({"X3D":{"@version":"4.0","Scene":{"-children":[
    {"ProtoDeclare":{"@name":"P","ProtoBody":{"-children":[
      {"Group":{}},
      {"IMPORT":{"@inlineDEF":"Source","@importedDEF":"Clock","@AS":"Local"}},
      {"ProtoDeclare":{"@name":"Nested","ProtoBody":{"-children":[
        {"Group":{}},{"IMPORT":{"@inlineDEF":"Inner","@importedDEF":"Other"}}
      ]}}}
    ]}}}
  ]}}})");
  CHECK(doc.scene.imports.empty());
  const auto p = doc.scene.findProto("P");
  REQUIRE(p);
  REQUIRE(p->body.imports.size() == 1);
  CHECK(p->body.imports[0].as == "Local");
  bool foundNested = false;
  for (const auto &entry : p->body.orderedStatements()) {
    if (entry.proto && entry.proto->name == "Nested") {
      foundNested = true;
      REQUIRE(entry.proto->body.imports.size() == 1);
      CHECK(entry.proto->body.imports[0].inlineDEF == "Inner");
    }
  }
  CHECK(foundNested);
}
