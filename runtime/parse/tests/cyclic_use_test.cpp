#include "doctest/doctest.h"
// A USE inside the body of the node it names (an "enclosing" USE) is dropped by
// every reader with a CyclicUse warning. Attaching it made the node contain
// itself: a shared_ptr cycle that leaked whenever the document was not handed to
// an execution context (whose buildSceneGraph runs breakContainmentCycles).
// Found by the libFuzzer parseDocument gate.
#include "X3DCodecs.hpp"
#include "X3DParse.hpp"
#include "DynamicField.hpp"
#include "x3d/nodes/Group.hpp"
#include "x3d/nodes/Script.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/Transform.hpp"

#include <memory>
#include <string>

using x3d::codec::Encoding;
using Kind = x3d::runtime::ReaderWarning::Kind;

namespace {

std::size_t cyclicWarnings(const x3d::runtime::X3DDocument &doc) {
  std::size_t n = 0;
  for (const auto &w : doc.readerWarnings)
    if (w.kind == Kind::CyclicUse)
      ++n;
  return n;
}

// Parses `text`, checks the enclosing USE was dropped from the root Transform's
// children, and checks the whole graph is freed with the document.
void checkDropped(const std::string &text, Encoding enc,
                  std::size_t keptChildren) {
  std::weak_ptr<x3d::nodes::X3DNode> root;
  {
    auto doc = x3d::codec::parseDocument(text, enc);
    REQUIRE(doc.scene.rootNodes.size() == 1);
    auto t = std::dynamic_pointer_cast<x3d::nodes::Transform>(doc.scene.rootNodes[0]);
    REQUIRE(t);
    CHECK(t->getDEF() == "A");
    CHECK(t->getChildren().size() == keptChildren);
    CHECK(cyclicWarnings(doc) == 1);
    for (const auto &w : doc.readerWarnings)
      if (w.kind == Kind::CyclicUse)
        CHECK(w.detail.find("USE 'A'") != std::string::npos);
    root = t;
  }
  CHECK(root.expired()); // no containment cycle keeps the graph alive
}

} // namespace

TEST_CASE("XML reader drops a USE of an enclosing node") {
  SUBCASE("direct child") {
    checkDropped("<X3D version='4.0'><Scene><Transform DEF='A'>"
                 "<Transform USE='A'/></Transform></Scene></X3D>",
                 Encoding::XML, 0);
  }
  SUBCASE("deeper descendant, siblings kept") {
    checkDropped("<X3D version='4.0'><Scene><Transform DEF='A'>"
                 "<Group><Transform USE='A'/></Group><Shape/>"
                 "</Transform></Scene></X3D>",
                 Encoding::XML, 2);
  }
  SUBCASE("inside an unknown element") {
    // EnvironmentLight is not a 4.0 node, so it is read as an opaque node.
    std::weak_ptr<x3d::nodes::X3DNode> root;
    {
      auto doc = x3d::codec::parseDocument(
          "<X3D version='4.0'><Scene><Transform DEF='A'>"
          "<EnvironmentLight DEF='E'><Bogus><Light USE='E'/></Bogus></EnvironmentLight>"
          "</Transform></Scene></X3D>",
          Encoding::XML);
      REQUIRE(doc.scene.rootNodes.size() == 1);
      CHECK(cyclicWarnings(doc) == 1);
      root = doc.scene.rootNodes[0];
    }
    CHECK(root.expired());
  }
}

TEST_CASE("JSON reader drops a USE of an enclosing node") {
  SUBCASE("direct child") {
    checkDropped(R"({"X3D":{"@version":"4.0","Scene":{"-children":[
      {"Transform":{"@DEF":"A","-children":[{"Transform":{"@USE":"A"}}]}}]}}})",
                 Encoding::JSON, 0);
  }
  SUBCASE("deeper descendant, siblings kept") {
    checkDropped(R"({"X3D":{"@version":"4.0","Scene":{"-children":[
      {"Transform":{"@DEF":"A","-children":[
        {"Group":{"-children":[{"Transform":{"@USE":"A"}}]}},{"Shape":{}}]}}]}}})",
                 Encoding::JSON, 2);
  }
}

TEST_CASE("ClassicVRML reader drops a USE of an enclosing node") {
  SUBCASE("direct child") {
    checkDropped("#X3D V4.0 utf8\nDEF A Transform { children [ USE A ] }",
                 Encoding::ClassicVRML, 0);
  }
  SUBCASE("deeper descendant, siblings kept") {
    checkDropped("#X3D V4.0 utf8\nDEF A Transform { children [ "
                 "Group { children USE A } Shape {} ] }",
                 Encoding::ClassicVRML, 2);
  }
}

TEST_CASE("USE of a closed node is still shared") {
  // Sibling USE after the DEF'd body is closed: a DAG, not a cycle.
  const char *const xml = "<X3D version='4.0'><Scene>"
                          "<Transform DEF='A'><Shape/></Transform>"
                          "<Group><Transform USE='A'/></Group>"
                          "</Scene></X3D>";
  auto doc = x3d::codec::parseDocument(xml, Encoding::XML);
  REQUIRE(doc.scene.rootNodes.size() == 2);
  auto g = std::dynamic_pointer_cast<x3d::nodes::Group>(doc.scene.rootNodes[1]);
  REQUIRE(g);
  REQUIRE(g->getChildren().size() == 1);
  CHECK(g->getChildren()[0] == doc.scene.rootNodes[0]);
  CHECK(cyclicWarnings(doc) == 0);
}

TEST_CASE("Script author field may still name its own Script") {
  // The field value lives in the scene's DynamicFieldStore, not in the node,
  // so a self reference is no containment cycle and must keep resolving.
  const char *const xml =
      "<X3D version='4.0'><Scene><Script DEF='S'>"
      "<field name='self' type='SFNode' accessType='initializeOnly'>"
      "<Script USE='S'/></field></Script></Scene></X3D>";
  auto doc = x3d::codec::parseDocument(xml, Encoding::XML);
  REQUIRE(doc.scene.rootNodes.size() == 1);
  auto s = std::dynamic_pointer_cast<x3d::nodes::Script>(doc.scene.rootNodes[0]);
  REQUIRE(s);
  CHECK(cyclicWarnings(doc) == 0);
  auto fields = doc.scene.authorFields->authorFields(*s);
  REQUIRE(fields.size() == 1);
  CHECK(fields[0].x3dName == "self");
}

namespace {

// The enclosing node as a node value of a ProtoInstance nested in it.
void checkFieldValueDropped(const std::string &text, Encoding enc) {
  std::weak_ptr<x3d::nodes::X3DNode> root;
  {
    auto doc = x3d::codec::parseDocument(text, enc);
    CHECK(cyclicWarnings(doc) == 1);
    for (const auto &n : doc.scene.rootNodes)
      if (n && n->getDEF() == "A")
        root = n;
    REQUIRE(!root.expired());
  }
  CHECK(root.expired());
}

} // namespace

TEST_CASE("Readers drop an enclosing node as a ProtoInstance fieldValue") {
  SUBCASE("XML") {
    checkFieldValueDropped(
        "<X3D version='4.0'><Scene>"
        "<ProtoDeclare name='P'><ProtoInterface>"
        "<field name='kids' type='MFNode' accessType='inputOutput'/>"
        "</ProtoInterface><ProtoBody><Group><IS>"
        "<connect nodeField='children' protoField='kids'/>"
        "</IS></Group></ProtoBody></ProtoDeclare>"
        "<Transform DEF='A'><ProtoInstance name='P'><fieldValue name='kids'>"
        "<Transform USE='A'/></fieldValue></ProtoInstance></Transform>"
        "</Scene></X3D>",
        Encoding::XML);
  }
  SUBCASE("JSON") {
    checkFieldValueDropped(R"({"X3D":{"@version":"4.0","Scene":{"-children":[
      {"ProtoDeclare":{"@name":"P","ProtoInterface":{"field":[
        {"@name":"kids","@type":"MFNode","@accessType":"inputOutput"}]},
       "ProtoBody":{"-children":[{"Group":{"IS":{"connect":[
        {"@nodeField":"children","@protoField":"kids"}]}}}]}}},
      {"Transform":{"@DEF":"A","-children":[{"ProtoInstance":{"@name":"P",
        "fieldValue":[{"@name":"kids","-children":[{"Transform":{"@USE":"A"}}]}]}}]}}]}}})",
                           Encoding::JSON);
  }
  SUBCASE("ClassicVRML") {
    checkFieldValueDropped(
        "#X3D V4.0 utf8\n"
        "PROTO P [ inputOutput MFNode kids [] ] { Group { children IS kids } }\n"
        "DEF A Transform { children [ P { kids [ USE A ] } ] }",
        Encoding::ClassicVRML);
  }
}
