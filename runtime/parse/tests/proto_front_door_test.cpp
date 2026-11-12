// runtime/parse/tests/proto_front_door_test.cpp
//
// front-door PROTO expansion. Asserts that parseDocument/parseFile
// drive expandScene with the default local-file resolver, so:
//   - a local PROTO instance expands end-to-end through the in-memory front door
//   - an EXTERNPROTO instance resolves a sibling file relative to the parsed
//     file's directory (baseUrl derivation in parseFile)
//   - a mutually-referential EXTERN cycle terminates (thread_local file guard)
//     rather than recursing without bound.
//
// The fixtures dir (runtime/parse/tests/data/proto) is passed as X3D_TEST_DATA_DIR so the
// test runs from any working directory.
#include "doctest/doctest.h"
#include "X3DParse.hpp"
#include "X3DProtoExpand.hpp"
#include "X3DCodecs.hpp"
#include "ClassicVrmlReader.hpp"
#include "JsonReader.hpp"
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/Group.hpp"
#include "x3d/nodes/Collision.hpp"
#include "x3d/nodes/Shape.hpp"

#include <algorithm>
#include <any>
#include <stdexcept>
#include <string>
#include <vector>

static std::string g_dataDir = "runtime/parse/tests/data/proto";

static bool hasRootNamed(const x3d::runtime::X3DDocument &doc,
                         const std::string &typeName) {
  for (const auto &n : doc.getScene().rootNodes)
    if (n && n->nodeTypeName() == typeName)
      return true;
  return false;
}

// Local PROTO expands end-to-end through the in-memory front door.
static void frontDoorLocalExpandTest() {
  const char *xml =
      "<X3D version='4.0'><Scene>"
      "<ProtoDeclare name='P'><ProtoBody><Box/></ProtoBody></ProtoDeclare>"
      "<ProtoInstance name='P'/></Scene></X3D>";
  auto doc = x3d::codec::parseDocument(xml);
  REQUIRE((hasRootNamed(doc, "Box")));
  REQUIRE((doc.protoWarnings.empty()));
}

// A PROTO has one DEF table for its interface and body, isolated from the
// enclosing scene and from a nested PROTO. Expansion preserves aliases within
// each instance but gives separate instances independent copies.
static void declarationDefScopeTest() {
  const struct {
    x3d::codec::Encoding encoding;
    const char *text;
  } cases[] = {
      {x3d::codec::Encoding::XML,
       "<X3D version='4.0'><Scene><Group DEF='Outside'/>"
       "<ProtoDeclare name='Leaf'><ProtoBody><Group/></ProtoBody></ProtoDeclare>"
       "<ProtoDeclare name='P'><ProtoInterface>"
       "<field name='first' type='MFNode' accessType='initializeOnly'>"
       "<Group DEF='Shared'/><Group USE='Shared'/></field>"
       "<field name='second' type='SFNode' accessType='initializeOnly'>"
       "<Group USE='Shared'/></field>"
       "<field name='outside' type='SFNode' accessType='initializeOnly'>"
       "<Group USE='Outside'/></field></ProtoInterface>"
       "<ProtoBody><Group><Group USE='Shared'/><Group USE='Shared'/>"
       "</Group><ProtoDeclare name='Inner'><ProtoBody>"
       "<Group USE='Shared'/></ProtoBody></ProtoDeclare>"
       "<ProtoInstance name='Leaf'/><ProtoInstance name='Inner'/>"
       "</ProtoBody></ProtoDeclare>"
       "<Group USE='Shared'/><ProtoInstance name='P'/>"
       "<ProtoInstance name='P'/></Scene></X3D>"},
      {x3d::codec::Encoding::JSON,
       R"({"X3D":{"@version":"4.0","Scene":{"-children":[{"Group":{"@DEF":"Outside"}},{"ProtoDeclare":{"@name":"Leaf","ProtoBody":{"-children":[{"Group":{}}]}}},{"ProtoDeclare":{"@name":"P","ProtoInterface":{"field":[{"@name":"first","@type":"MFNode","@accessType":"initializeOnly","-children":[{"Group":{"@DEF":"Shared"}},{"Group":{"@USE":"Shared"}}]},{"@name":"second","@type":"SFNode","@accessType":"initializeOnly","-children":[{"Group":{"@USE":"Shared"}}]},{"@name":"outside","@type":"SFNode","@accessType":"initializeOnly","-children":[{"Group":{"@USE":"Outside"}}]}]},"ProtoBody":{"-children":[{"Group":{"-children":[{"Group":{"@USE":"Shared"}},{"Group":{"@USE":"Shared"}}]}},{"ProtoDeclare":{"@name":"Inner","ProtoBody":{"-children":[{"Group":{"@USE":"Shared"}}]}}},{"ProtoInstance":{"@name":"Leaf"}},{"ProtoInstance":{"@name":"Inner"}}]}}},{"Group":{"@USE":"Shared"}},{"ProtoInstance":{"@name":"P"}},{"ProtoInstance":{"@name":"P"}}]}}})"},
      {x3d::codec::Encoding::ClassicVRML,
       "#X3D V4.0 utf8\nDEF Outside Group { }\n"
       "PROTO Leaf [ ] { Group { } }\n"
       "PROTO P [ field MFNode first [ DEF Shared Group { } USE Shared ] "
       "field SFNode second USE Shared field SFNode outside USE Outside ] "
       "{ Group { children [ USE Shared USE Shared ] } "
       "PROTO Inner [ ] { USE Shared } Leaf { } Inner { } }\n"
       "USE Shared\nP { }\nP { }\n"},
  };
  for (const auto &c : cases) {
    auto doc = x3d::codec::parseDocument(c.text, c.encoding);
    REQUIRE((doc.scene.protoDeclarations.size() == 2));
    auto p = doc.scene.protoDeclarations[1];
    REQUIRE((p->interface.size() == 3));
    REQUIRE((p->interface[0].nodeDefault.size() == 2));
    auto shared = p->interface[0].nodeDefault[0];
    REQUIRE((shared == p->interface[0].nodeDefault[1]));
    REQUIRE((p->interface[1].nodeDefault.size() == 1));
    REQUIRE((shared == p->interface[1].nodeDefault[0]));
    REQUIRE((p->interface[2].nodeDefault.empty()));
    REQUIRE((p->body.nodes.size() == 1));
    auto body = std::dynamic_pointer_cast<x3d::nodes::Group>(p->body.nodes[0]);
    REQUIRE((body && body->getChildren().size() == 2));
    REQUIRE((body->getChildren()[0] == shared));
    REQUIRE((body->getChildren()[1] == shared));
    REQUIRE((p->body.nestedInstances.size() == 2));
    REQUIRE((p->body.nestedInstances[0].declaration == doc.scene.protoDeclarations[0]));
    REQUIRE((p->body.nestedInstances[1].declaration));
    REQUIRE((p->body.nestedInstances[1].declaration->body.nodes.empty()));
    REQUIRE((doc.scene.defs.count("Shared") == 0));
    REQUIRE((doc.scene.rootNodes.size() == 3)); // Outside and two instances
    auto a = std::dynamic_pointer_cast<x3d::nodes::Group>(doc.scene.rootNodes[1]);
    auto b = std::dynamic_pointer_cast<x3d::nodes::Group>(doc.scene.rootNodes[2]);
    REQUIRE((a && b && a != b));
    REQUIRE((a->getChildren().size() >= 2 && b->getChildren().size() >= 2));
    REQUIRE((a->getChildren()[0] == a->getChildren()[1]));
    REQUIRE((b->getChildren()[0] == b->getChildren()[1]));
    REQUIRE((a->getChildren()[0] != b->getChildren()[0]));
  }
}

static void nestedDeclarationShadowingTest() {
  const struct {
    x3d::codec::Encoding encoding;
    const char *text;
  } cases[] = {
      {x3d::codec::Encoding::XML,
       "<X3D version='4.0'><Scene>"
       "<ProtoDeclare name='Choice'><ProtoBody><Box/></ProtoBody></ProtoDeclare>"
       "<ExternProtoDeclare name='External' url='&quot;missing.x3d&quot;'/>"
       "<ProtoDeclare name='WrapProto'><ProtoBody><Group/>"
       "<ProtoDeclare name='Choice'><ProtoBody><Sphere/></ProtoBody></ProtoDeclare>"
       "<ProtoInstance name='Choice'/></ProtoBody></ProtoDeclare>"
       "<ProtoDeclare name='WrapExtern'><ProtoBody><Group/>"
       "<ExternProtoDeclare name='Choice' url='&quot;missing.x3d&quot;'/>"
       "<ProtoInstance name='Choice'/></ProtoBody></ProtoDeclare>"
       "<ProtoDeclare name='WrapCross'><ProtoBody><Group/>"
       "<ProtoDeclare name='External'><ProtoBody><Cone/></ProtoBody></ProtoDeclare>"
       "<ProtoInstance name='External'/></ProtoBody></ProtoDeclare>"
       "</Scene></X3D>"},
      {x3d::codec::Encoding::JSON,
       R"({"X3D":{"@version":"4.0","Scene":{"-children":[{"ProtoDeclare":{"@name":"Choice","ProtoBody":{"-children":[{"Box":{}}]}}},{"ExternProtoDeclare":{"@name":"External","@url":["missing.x3d"]}},{"ProtoDeclare":{"@name":"WrapProto","ProtoBody":{"-children":[{"Group":{}},{"ProtoDeclare":{"@name":"Choice","ProtoBody":{"-children":[{"Sphere":{}}]}}},{"ProtoInstance":{"@name":"Choice"}}]}}},{"ProtoDeclare":{"@name":"WrapExtern","ProtoBody":{"-children":[{"Group":{}},{"ExternProtoDeclare":{"@name":"Choice","@url":["missing.x3d"]}},{"ProtoInstance":{"@name":"Choice"}}]}}},{"ProtoDeclare":{"@name":"WrapCross","ProtoBody":{"-children":[{"Group":{}},{"ProtoDeclare":{"@name":"External","ProtoBody":{"-children":[{"Cone":{}}]}}},{"ProtoInstance":{"@name":"External"}}]}}}]}}})"},
      {x3d::codec::Encoding::ClassicVRML,
       "#X3D V4.0 utf8\nPROTO Choice [ ] { Box { } }\n"
       "EXTERNPROTO External [ ] [ \"missing.x3d\" ]\n"
       "PROTO WrapProto [ ] { Group { } "
       "PROTO Choice [ ] { Sphere { } } Choice { } }\n"
       "PROTO WrapExtern [ ] { Group { } "
       "EXTERNPROTO Choice [ ] [ \"missing.x3d\" ] Choice { } }\n"
       "PROTO WrapCross [ ] { Group { } "
       "PROTO External [ ] { Cone { } } External { } }\n"},
  };
  for (const auto &c : cases) {
    auto doc = x3d::codec::parseDocument(c.text, c.encoding);
    REQUIRE((doc.scene.protoDeclarations.size() == 4));
    auto outer = doc.scene.protoDeclarations[0];
    auto local = doc.scene.protoDeclarations[1]->body.nestedInstances;
    REQUIRE((local.size() == 1 && local[0].declaration));
    REQUIRE((local[0].declaration != outer));
    REQUIRE((local[0].declaration->body.nodes.size() == 1));
    REQUIRE((local[0].declaration->body.nodes[0]->nodeTypeName() == "Sphere"));
    auto external = doc.scene.protoDeclarations[2]->body.nestedInstances;
    REQUIRE((external.size() == 1 && external[0].externDeclaration));
    REQUIRE((!external[0].declaration));
    REQUIRE((external[0].externDeclaration->name == "Choice"));
    auto cross = doc.scene.protoDeclarations[3]->body.nestedInstances;
    REQUIRE((cross.size() == 1 && cross[0].declaration));
    REQUIRE((cross[0].declaration->body.nodes.size() == 1));
    REQUIRE((cross[0].declaration->body.nodes[0]->nodeTypeName() == "Cone"));
    REQUIRE((doc.scene.findProto("Choice") == outer));
  }
}

// EXTERN resolves a sibling file via the default local-file resolver, with the
// base directory derived from the parsed file's path.
static void frontDoorExternResolveTest() {
  auto doc = x3d::codec::parseFile(g_dataDir + "/main.x3d");
  REQUIRE((hasRootNamed(doc, "Box")));
}

// A mutually-referential EXTERN cycle must terminate (bounded, no hang).
static void frontDoorExternCycleTest() {
  auto doc = x3d::codec::parseFile(g_dataDir + "/cycleA.x3d");
  // We only require that parsing returns at all. The cycle resolves to nothing
  // concrete, so no Box is spliced; the key property is termination.
  (void)doc;
}

static void sourceUnitProvenanceTest() {
  const std::string parentHead =
      "<head><unit category='length' name='kilometre' "
      "conversionFactor='1000'/></head>";
  const std::string parentPrefix =
      "<X3D profile='Interchange' version='4.0'>" + parentHead + "<Scene>";

  auto localDoc = x3d::codec::parseDocument(
      parentPrefix +
      "<ProtoDeclare name='Leaf'><ProtoBody><Box/></ProtoBody></ProtoDeclare>"
      "<ProtoDeclare name='Wrap'><ProtoBody><Transform>"
      "<ProtoInstance name='Leaf'/></Transform></ProtoBody></ProtoDeclare>"
      "<ProtoInstance name='Wrap'/></Scene></X3D>");
  REQUIRE((localDoc.scene.protoDeclarations.size() == 2));
  REQUIRE((localDoc.scene.protoDeclarations[0]->sourceUnits.size() == 1));
  REQUIRE((localDoc.scene.protoDeclarations[1]->sourceUnits.size() == 1));
  const auto &nested =
      localDoc.scene.protoDeclarations[1]->body.nestedInstances;
  REQUIRE((nested.size() == 1 && nested[0].declaration));
  REQUIRE((nested[0].declaration->sourceUnits[0].name == "kilometre"));

  auto inlineDoc = x3d::codec::parseDocument(
      parentPrefix + "<Inline DEF='Inl' url='\"unit-child.x3d\"'/>"
                     "<Inline DEF='Plain' url='\"no-unit-child.x3d\"'/>"
                     "</Scene></X3D>",
      x3d::codec::Encoding::XML, g_dataDir);
  REQUIRE((inlineDoc.head.units.size() == 1));
  REQUIRE((inlineDoc.scene.sourceUnits.size() == 1));
  REQUIRE((inlineDoc.scene.sourceUnits[0].name == "kilometre"));
  REQUIRE((inlineDoc.scene.sourceUnits[0].category == "length"));
  REQUIRE((inlineDoc.scene.sourceUnits[0].conversionFactor == 1000.0));
  REQUIRE((inlineDoc.scene.expandedInlineScenes.size() == 2));
  for (const auto &[node, child] : inlineDoc.scene.expandedInlineScenes) {
    REQUIRE((node && child));
    if (node->getDEF() == "Inl") {
      REQUIRE((child->sourceUnits.size() == 1));
      REQUIRE((child->sourceUnits[0].name == "centimetre"));
      REQUIRE((child->sourceUnits[0].category == "length"));
      REQUIRE((child->sourceUnits[0].conversionFactor == 0.01));
    } else {
      REQUIRE((node->getDEF() == "Plain"));
      REQUIRE((child->sourceUnits.empty()));
    }
  }

  auto customChild = x3d::codec::parseDocument(
      "<X3D version='4.0'><head><unit category='length' name='millimetre' "
      "conversionFactor='0.001'/></head><Scene><Group/></Scene></X3D>");
  auto returnedScene = std::make_shared<x3d::runtime::Scene>(
      std::move(customChild.scene));
  auto customDoc = x3d::codec::parseDocument(
      parentPrefix + "<Inline DEF='Custom' url='\"virtual\"'/>"
                     "</Scene></X3D>",
      x3d::codec::Encoding::XML, "", x3d::codec::localFileProtoResolver,
      [returnedScene](const std::vector<std::string> &,
                      const std::string &) { return returnedScene; });
  REQUIRE((customDoc.scene.expandedInlineScenes.size() == 1));
  REQUIRE((customDoc.scene.expandedInlineScenes.begin()->second->sourceUnits.size() == 1));
  REQUIRE((customDoc.scene.expandedInlineScenes.begin()->second->sourceUnits[0].name ==
         "millimetre"));

  auto externDoc = x3d::codec::parseDocument(
      parentPrefix +
      "<ExternProtoDeclare name='ChildBox' url='\"unit-child.x3d#ChildBox\"'/>"
      "<ExternProtoDeclare name='PlainBox' url='\"no-unit-child.x3d#PlainBox\"'/>"
      "<ProtoInstance name='ChildBox'/><ProtoInstance name='PlainBox'/>"
      "</Scene></X3D>",
      x3d::codec::Encoding::XML, g_dataDir);
  REQUIRE((externDoc.scene.protoInstances.size() == 2));
  const auto &child = externDoc.scene.protoInstances[0].declaration;
  const auto &plain = externDoc.scene.protoInstances[1].declaration;
  REQUIRE((child && child->sourceUnits.size() == 1));
  REQUIRE((child->sourceUnits[0].name == "centimetre"));
  REQUIRE((child->sourceUnits[0].category == "length"));
  REQUIRE((child->sourceUnits[0].conversionFactor == 0.01));
  REQUIRE((plain && plain->sourceUnits.empty()));
  REQUIRE((externDoc.scene.expandedSources.size() == 2));
  for (const auto &[node, expanded] : externDoc.scene.expandedSources) {
    REQUIRE((node && expanded.declaration));
    if (expanded.name == "ChildBox") {
      REQUIRE((expanded.declaration->sourceUnits.size() == 1));
      REQUIRE((expanded.declaration->sourceUnits[0].conversionFactor == 0.01));
    } else {
      REQUIRE((expanded.name == "PlainBox"));
      REQUIRE((expanded.declaration->sourceUnits.empty()));
    }
  }

  // Runtime snapshots do not become extra serialized head statements.
  x3d::codec::XmlWriter writer;
  auto roundtrip = x3d::codec::parseDocument(writer.writeDocument(inlineDoc));
  REQUIRE((roundtrip.head.units.size() == 1));
  REQUIRE((roundtrip.head.units[0].name == "kilometre"));
}

static void authoredScalarPresenceTest() {
  const struct {
    x3d::codec::Encoding encoding;
    const char *text;
  } cases[] = {
      {x3d::codec::Encoding::XML,
       "<X3D version='4.0'><Scene><Box DEF='Explicit' size='2 2 2' "
       "bogus='1'/><Box USE='Explicit'/><Box DEF='Absent'/></Scene></X3D>"},
      {x3d::codec::Encoding::JSON,
       R"({"X3D":{"@version":"4.0","Scene":{"-children":[{"Box":{"@DEF":"Explicit","@size":[2,2,2],"@bogus":1}},{"Box":{"@USE":"Explicit"}},{"Box":{"@DEF":"Absent"}}]}}})"},
      {x3d::codec::Encoding::ClassicVRML,
       "#X3D V4.0 utf8\nDEF Explicit Box { size 2 2 2 bogus 1 }\n"
       "USE Explicit\nDEF Absent Box { }\n"},
  };
  for (const auto &c : cases) {
    auto doc = x3d::codec::parseDocument(c.text, c.encoding);
    REQUIRE((doc.scene.rootNodes.size() == 3));
    auto explicitNode = doc.scene.rootNodes[0];
    REQUIRE((explicitNode == doc.scene.rootNodes[1]));
    REQUIRE((doc.scene.authoredScalarFields.contains(explicitNode, "size")));
    REQUIRE((!doc.scene.authoredScalarFields.contains(explicitNode, "bogus")));
    REQUIRE((!doc.scene.authoredScalarFields.contains(doc.scene.rootNodes[2],
                                                    "size")));
  }

  auto rejectedEnum = x3d::codec::parseDocument(
      "<X3D version='4.0'><Scene>"
      "<EspduTransform networkMode='notARealMode'/>"
      "</Scene></X3D>");
  REQUIRE((rejectedEnum.scene.rootNodes.size() == 1));
  REQUIRE((!rejectedEnum.scene.authoredScalarFields.contains(
      rejectedEnum.scene.rootNodes.front(), "networkMode")));

  auto proto = x3d::codec::parseDocument(
      "<X3D version='4.0'><Scene><ProtoDeclare name='P'><ProtoBody>"
      "<Box size='2 2 2'/></ProtoBody></ProtoDeclare></Scene></X3D>");
  auto decl = proto.scene.protoDeclarations.front();
  REQUIRE((decl->body.nodes.size() == 1));
  REQUIRE((decl->authoredScalarFields.contains(decl->body.nodes.front(), "size")));

  auto nested = x3d::codec::parseDocument(
      "<X3D version='4.0'><Scene><ProtoDeclare name='Nested'><ProtoBody>"
      "<Transform><Shape><Box size='2 2 2'/></Shape></Transform>"
      "</ProtoBody></ProtoDeclare><ProtoInstance name='Nested'/>"
      "</Scene></X3D>");
  // Find the deeply cloned Box through reflected child fields.
  auto findBox = [&](const auto &self,
                     const std::shared_ptr<x3d::nodes::X3DNode> &node)
      -> std::shared_ptr<x3d::nodes::X3DNode> {
    if (!node) return nullptr;
    if (node->nodeTypeName() == "Box") return node;
    std::shared_ptr<x3d::nodes::X3DNode> found;
    x3d::runtime::forEachChildNode(*node, [&](const auto &, const auto &child) {
      if (!found) found = self(self, child);
    });
    return found;
  };
  auto clonedBox = findBox(findBox, nested.scene.rootNodes.front());
  REQUIRE((clonedBox));
  REQUIRE((nested.scene.authoredScalarFields.contains(clonedBox, "size")));

  auto interfaceDefault = x3d::codec::parseDocument(
      "<X3D version='4.0'><Scene><ProtoDeclare name='DefaultNode'>"
      "<ProtoInterface><field name='geo' type='SFNode' "
      "accessType='initializeOnly'><Box size='2 2 2'/></field>"
      "</ProtoInterface><ProtoBody><Shape><IS>"
      "<connect nodeField='geometry' protoField='geo'/>"
      "</IS></Shape></ProtoBody></ProtoDeclare>"
      "<ProtoInstance name='DefaultNode'/></Scene></X3D>");
  auto defaultBox = findBox(findBox, interfaceDefault.scene.rootNodes.front());
  REQUIRE((defaultBox));
  REQUIRE((interfaceDefault.scene.authoredScalarFields.contains(defaultBox,
                                                              "size")));

  const struct {
    x3d::codec::Encoding encoding;
    const char *text;
  } otherDefaults[] = {
      {x3d::codec::Encoding::ClassicVRML,
       "#X3D V4.0 utf8\nPROTO P [ field SFNode geo Box { size 2 2 2 } ] "
       "{ Shape { geometry IS geo } } P { }"},
      {x3d::codec::Encoding::JSON,
       R"({"X3D":{"@version":"4.0","Scene":{"-children":[{"ProtoDeclare":{"@name":"P","ProtoInterface":{"field":[{"@name":"geo","@type":"SFNode","@accessType":"initializeOnly","-children":[{"Box":{"@size":[2,2,2]}}]}]},"ProtoBody":{"-children":[{"Shape":{"IS":{"connect":[{"@nodeField":"geometry","@protoField":"geo"}]}}}]}}},{"ProtoInstance":{"@name":"P"}}]}}})"},
  };
  for (const auto &c : otherDefaults) {
    auto doc = x3d::codec::parseDocument(c.text, c.encoding);
    REQUIRE((doc.scene.protoDeclarations.size() == 1));
    auto declaration = doc.scene.protoDeclarations.front();
    REQUIRE((declaration->interface.size() == 1));
    REQUIRE((declaration->interface.front().nodeDefault.size() == 1));
    auto defaultNode = declaration->interface.front().nodeDefault.front();
    REQUIRE((declaration->authoredScalarFields.contains(defaultNode, "size")));
    auto expandedBox = findBox(findBox, doc.scene.rootNodes.front());
    REQUIRE((expandedBox));
    REQUIRE((doc.scene.authoredScalarFields.contains(expandedBox, "size")));
  }

  auto nestedLiteral = x3d::codec::parseDocument(
      "<X3D version='4.0'><Scene>"
      "<ProtoDeclare name='Inner'><ProtoInterface>"
      "<field name='geo' type='SFNode' accessType='initializeOnly'/>"
      "</ProtoInterface><ProtoBody><Shape><IS>"
      "<connect nodeField='geometry' protoField='geo'/>"
      "</IS></Shape></ProtoBody></ProtoDeclare>"
      "<ProtoDeclare name='Outer'><ProtoBody><Transform>"
      "<ProtoInstance name='Inner'><fieldValue name='geo'>"
      "<Box size='2 2 2'/></fieldValue></ProtoInstance>"
      "</Transform></ProtoBody></ProtoDeclare>"
      "<ProtoInstance name='Outer'/></Scene></X3D>");
  auto literalBox = findBox(findBox, nestedLiteral.scene.rootNodes.front());
  REQUIRE((literalBox));
  REQUIRE((nestedLiteral.scene.authoredScalarFields.contains(literalBox, "size")));
}

static void directNestedDeclarationWriteTest() {
  const char *source =
      "<X3D version='4.0'><head><unit category='length' name='metre' "
      "conversionFactor='2'/></head><Scene>"
      "<ProtoDeclare name='Choice'><ProtoBody><Box/></ProtoBody></ProtoDeclare>"
      "<ProtoDeclare name='Wrap'><ProtoBody>"
      "<Group DEF='Shared'/><ProtoInstance name='Choice'/>"
      "<ProtoDeclare name='Choice'><ProtoBody><Sphere/></ProtoBody></ProtoDeclare>"
      "<ProtoInstance name='Choice'/>"
      "<ExternProtoDeclare name='UnusedExtern' url='&quot;missing.x3d&quot;'/>"
      "<ProtoDeclare name='Unused'><ProtoBody><Group/>"
      "<ProtoDeclare name='Deep'><ProtoBody><Cone/></ProtoBody></ProtoDeclare>"
      "</ProtoBody></ProtoDeclare>"
      "</ProtoBody></ProtoDeclare></Scene></X3D>";

  auto check = [](const x3d::runtime::X3DDocument &doc) {
    REQUIRE((doc.scene.protoDeclarations.size() == 2));
    const auto &body = doc.scene.protoDeclarations[1]->body;
    REQUIRE((body.nodes.size() == 1));
    REQUIRE((body.nestedInstances.size() == 2));
    REQUIRE((body.nestedInstances[0].declaration == doc.scene.protoDeclarations[0]));
    REQUIRE((body.nestedInstances[0].declaration->body.nodes[0]->nodeTypeName() == "Box"));
    REQUIRE((body.nestedInstances[1].declaration));
    REQUIRE((body.nestedInstances[1].declaration->body.nodes[0]->nodeTypeName() == "Sphere"));
    REQUIRE((body.statements.size() == 6));
    REQUIRE((body.statements[0].kind == x3d::runtime::ProtoBodyStatement::Kind::Node));
    REQUIRE((body.statements[1].kind == x3d::runtime::ProtoBodyStatement::Kind::Instance));
    REQUIRE((body.statements[2].kind == x3d::runtime::ProtoBodyStatement::Kind::Proto));
    REQUIRE((body.statements[3].kind == x3d::runtime::ProtoBodyStatement::Kind::Instance));
    REQUIRE((body.statements[4].kind == x3d::runtime::ProtoBodyStatement::Kind::ExternProto));
    REQUIRE((body.statements[5].kind == x3d::runtime::ProtoBodyStatement::Kind::Proto));
    const auto &unused = body.statements[5].proto;
    REQUIRE((unused && unused->body.statements.size() == 2));
    REQUIRE((unused->sourceUnits.size() == 1));
    REQUIRE((unused->body.statements[1].proto->sourceUnits.size() == 1));
    REQUIRE((unused->body.statements[1].proto->name == "Deep"));
  };

  auto xml = x3d::codec::parseDocument(source);
  check(xml);
  const struct {
    x3d::codec::Encoding encoding;
    std::string text;
  } inputs[] = {
      {x3d::codec::Encoding::XML, source},
      {x3d::codec::Encoding::JSON, x3d::codec::JsonWriter().writeDocument(xml)},
      {x3d::codec::Encoding::ClassicVRML, x3d::codec::VrmlWriter().writeDocument(xml)},
  };
  for (const auto &input : inputs) {
    auto doc = x3d::codec::parseDocument(input.text, input.encoding);
    check(doc);
    const struct {
      x3d::codec::Encoding encoding;
      std::string text;
    } outputs[] = {
        {x3d::codec::Encoding::XML, x3d::codec::XmlWriter().writeDocument(doc)},
        {x3d::codec::Encoding::XML, x3d::codec::CanonicalXmlWriter().writeDocument(doc)},
        {x3d::codec::Encoding::JSON, x3d::codec::JsonWriter().writeDocument(doc)},
        {x3d::codec::Encoding::ClassicVRML, x3d::codec::VrmlWriter().writeDocument(doc)},
    };
    for (const auto &output : outputs)
      check(x3d::codec::parseDocument(output.text, output.encoding));
  }

  // Existing public vectors remain authoritative when callers edit a body.
  auto &body = xml.scene.protoDeclarations[1]->body;
  auto oldNode = body.nodes.front();
  body.nodes.clear();
  auto removed = x3d::codec::parseDocument(x3d::codec::XmlWriter().writeDocument(xml));
  REQUIRE((removed.scene.protoDeclarations[1]->body.nodes.empty()));
  body.nodes.push_back(oldNode);
  body.nodes.push_back(oldNode); // a second direct USE occurrence
  auto added = x3d::codec::parseDocument(x3d::codec::XmlWriter().writeDocument(xml));
  const auto &nodes = added.scene.protoDeclarations[1]->body.nodes;
  REQUIRE((nodes.size() == 2 && nodes[0] == nodes[1]));
  body.recordProto({});
  auto withNull = x3d::codec::parseDocument(
      x3d::codec::JsonWriter().writeDocument(xml), x3d::codec::Encoding::JSON);
  REQUIRE((withNull.scene.protoDeclarations[1]->body.nodes.size() == 2));
}

static void declarationAliasWriteTest() {
  auto doc = x3d::codec::parseDocument(
      "<X3D version='4.0'><Scene><ProtoDeclare name='P'><ProtoInterface>"
      "<field name='a' type='SFNode' accessType='initializeOnly'>"
      "<Group DEF='Shared'/></field>"
      "<field name='b' type='SFNode' accessType='initializeOnly'>"
      "<Group USE='Shared'/></field></ProtoInterface>"
      "<ProtoBody><Group><Group USE='Shared'/></Group>"
      "<ProtoDeclare name='Inner'><ProtoBody>"
      "<Group DEF='Shared'/></ProtoBody></ProtoDeclare>"
      "</ProtoBody></ProtoDeclare></Scene></X3D>");
  const struct {
    x3d::codec::Encoding encoding;
    std::string text;
  } outputs[] = {
      {x3d::codec::Encoding::XML, x3d::codec::XmlWriter().writeDocument(doc)},
      {x3d::codec::Encoding::XML, x3d::codec::CanonicalXmlWriter().writeDocument(doc)},
      {x3d::codec::Encoding::JSON, x3d::codec::JsonWriter().writeDocument(doc)},
      {x3d::codec::Encoding::ClassicVRML, x3d::codec::VrmlWriter().writeDocument(doc)},
  };
  for (const auto &output : outputs) {
    auto parsed = x3d::codec::parseDocument(output.text, output.encoding);
    REQUIRE((parsed.scene.protoDeclarations.size() == 1));
    const auto &p = *parsed.scene.protoDeclarations.front();
    REQUIRE((p.interface.size() == 2));
    REQUIRE((p.interface[0].nodeDefault.size() == 1));
    auto shared = p.interface[0].nodeDefault.front();
    REQUIRE((p.interface[1].nodeDefault.size() == 1));
    REQUIRE((p.interface[1].nodeDefault.front() == shared));
    REQUIRE((p.body.nodes.size() == 1));
    auto root = std::dynamic_pointer_cast<x3d::nodes::Group>(p.body.nodes.front());
    REQUIRE((root && root->getChildren().size() == 1));
    REQUIRE((root->getChildren().front() == shared));
    REQUIRE((p.body.statements.size() == 2));
    auto inner = p.body.statements[1].proto;
    REQUIRE((inner && inner->body.nodes.size() == 1));
    REQUIRE((inner->body.nodes.front() != shared));
  }
}

static void nodeContainedDeclarationPlacementTest() {
  const std::string classic =
      "#X3D V4.0 utf8\n"
      "PROTO Choice [ ] { Group { } }\n"
      "PROTO Wrap [ ] { Collision { "
      "proxy Choice { } "
      "PROTO Choice [ ] { Transform { } } "
      "children [ Choice { } ] "
      "} }\n";
  auto original = x3d::codec::parseDocument(
      classic, x3d::codec::Encoding::ClassicVRML);
  REQUIRE((original.scene.protoDeclarations.size() == 2));
  const auto &body = original.scene.protoDeclarations[1]->body;
  REQUIRE((body.nodes.size() == 1 && body.nestedInstances.size() == 2));
  REQUIRE((body.nestedInstances[0].declaration == original.scene.protoDeclarations[0]));
  REQUIRE((body.nestedInstances[0].parentField == "proxy"));
  REQUIRE((body.nestedInstances[1].parentField == "children"));
  auto sequence = body.nodeStatements.find(std::weak_ptr<x3d::nodes::X3DNode>(
      body.nodes.front()));
  REQUIRE((sequence != body.nodeStatements.end()));
  REQUIRE((sequence->second.size() == 3));
  REQUIRE((sequence->second[0].kind == x3d::runtime::ProtoBodyStatement::Kind::Instance));
  REQUIRE((sequence->second[1].kind == x3d::runtime::ProtoBodyStatement::Kind::Proto));
  REQUIRE((sequence->second[2].kind == x3d::runtime::ProtoBodyStatement::Kind::Instance));
  REQUIRE((body.nestedInstances[1].declaration == sequence->second[1].proto));
  REQUIRE((body.nestedInstances[1].declaration->body.nodes.front()->nodeTypeName() ==
         "Transform"));
  const struct {
    x3d::codec::Encoding encoding;
    std::string text;
  } outputs[] = {
      {x3d::codec::Encoding::XML, x3d::codec::XmlWriter().writeDocument(original)},
      {x3d::codec::Encoding::XML,
       x3d::codec::CanonicalXmlWriter().writeDocument(original)},
      {x3d::codec::Encoding::ClassicVRML,
       x3d::codec::VrmlWriter().writeDocument(original)},
      {x3d::codec::Encoding::JSON,
       x3d::codec::JsonWriter().writeDocument(original)},
  };
  for (const auto &output : outputs) {
    auto roundtrip = x3d::codec::parseDocument(output.text, output.encoding);
    const auto &again = roundtrip.scene.protoDeclarations[1]->body;
    REQUIRE((again.nestedInstances.size() == 2));
    REQUIRE((again.nestedInstances[0].declaration ==
           roundtrip.scene.protoDeclarations[0]));
    REQUIRE((again.nestedInstances[0].parentField == "proxy"));
    REQUIRE((again.nestedInstances[1].parentField == "children"));
    REQUIRE((again.nestedInstances[1].declaration));
    REQUIRE((again.nestedInstances[1].declaration !=
           roundtrip.scene.protoDeclarations[0]));
    REQUIRE((again.nestedInstances[1].declaration->body.nodes.front()->nodeTypeName() ==
           "Transform"));
  }
}

static void nodeContainedInterleavedFieldTest() {
  const char *xml =
      "<X3D version='4.0'><Scene>"
      "<ProtoDeclare name='Choice'><ProtoBody><Group/></ProtoBody></ProtoDeclare>"
      "<ProtoDeclare name='Wrap'><ProtoBody><Group>"
      "<ProtoInstance name='Choice'/>"
      "<ProtoDeclare name='Choice'><ProtoBody><Transform/></ProtoBody></ProtoDeclare>"
      "<ProtoInstance name='Choice'/>"
      "<ProtoDeclare name='Unused'><ProtoBody><Shape/></ProtoBody></ProtoDeclare>"
      "</Group></ProtoBody></ProtoDeclare>"
      "</Scene></X3D>";
  auto original = x3d::codec::parseDocument(xml);
  const auto &body = original.scene.protoDeclarations[1]->body;
  REQUIRE((body.nestedInstances.size() == 2));
  REQUIRE((body.nestedInstances[0].declaration == original.scene.protoDeclarations[0]));
  REQUIRE((body.nestedInstances[1].declaration != original.scene.protoDeclarations[0]));
  for (const std::string &written : {
           x3d::codec::XmlWriter().writeDocument(original),
           x3d::codec::CanonicalXmlWriter().writeDocument(original)}) {
    auto parsed = x3d::codec::parseDocument(written);
    const auto &again = parsed.scene.protoDeclarations[1]->body;
    REQUIRE((again.nestedInstances.size() == 2));
    REQUIRE((again.nestedInstances[0].declaration == parsed.scene.protoDeclarations[0]));
    REQUIRE((again.nestedInstances[1].declaration != parsed.scene.protoDeclarations[0]));
    auto node = again.nodes.front();
    auto sequence = again.nodeStatements.find(
        std::weak_ptr<x3d::nodes::X3DNode>(node));
    REQUIRE((sequence != again.nodeStatements.end()));
    REQUIRE((sequence->second.size() == 4)); // includes unused declaration
  }
  auto json = x3d::codec::JsonWriter().writeDocument(original);
  auto fromJson = x3d::codec::parseDocument(json, x3d::codec::Encoding::JSON);
  const auto &jsonBody = fromJson.scene.protoDeclarations[1]->body;
  REQUIRE((jsonBody.nestedInstances.size() == 2));
  REQUIRE((jsonBody.nestedInstances[0].declaration ==
         fromJson.scene.protoDeclarations[0]));
  REQUIRE((jsonBody.nestedInstances[1].declaration));
  REQUIRE((jsonBody.nestedInstances[1].declaration !=
         fromJson.scene.protoDeclarations[0]));
  REQUIRE((jsonBody.nestedInstances[1].declaration->body.nodes.front()->nodeTypeName() ==
         "Transform"));
  auto jsonSequence = jsonBody.nodeStatements.find(
      std::weak_ptr<x3d::nodes::X3DNode>(jsonBody.nodes.front()));
  REQUIRE((jsonSequence != jsonBody.nodeStatements.end()));
  REQUIRE((jsonSequence->second.size() == 4));
  bool rejected = false;
  try {
    (void)x3d::codec::VrmlWriter().writeDocument(original);
  } catch (const std::runtime_error &e) {
    rejected = std::string(e.what()).find("cannot currently preserve") !=
               std::string::npos;
  }
  REQUIRE((rejected));
}

// Classic declarations cannot occur inside an MFNode list. A literal prefix
// can move after them without changing prototype lookup or child order.
static void literalPrefixDeclarationRoundTripTest() {
  auto original = x3d::codec::parseDocument(R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='Wrap'><ProtoBody><Group>
<Group DEF='Before'><Shape/></Group>
<ProtoDeclare name='Leaf'><ProtoBody><Transform/></ProtoBody></ProtoDeclare>
<ExternProtoDeclare name='Unused' url='"missing.x3d#Unused"'/>
<ProtoInstance name='Leaf'/><Group USE='Before'/>
</Group></ProtoBody></ProtoDeclare><ProtoInstance name='Wrap'/>
</Scene></X3D>)");
  const auto classic = x3d::codec::VrmlWriter().writeDocument(original);
  auto parsed = x3d::codec::parseDocument(classic, x3d::codec::Encoding::ClassicVRML);
  for (const auto &text : {x3d::codec::XmlWriter().writeDocument(parsed),
                          x3d::codec::CanonicalXmlWriter().writeDocument(parsed)}) {
    auto again = x3d::codec::parseDocument(text);
    auto json = x3d::codec::JsonWriter().writeDocument(again);
    again = x3d::codec::parseDocument(json, x3d::codec::Encoding::JSON);
    REQUIRE((again.protoWarnings.empty()));
    auto group = std::dynamic_pointer_cast<x3d::nodes::Group>(again.scene.rootNodes.at(0));
    REQUIRE((group && group->getChildren().size() == 3));
    REQUIRE((group->getChildren()[0]->nodeTypeName() == "Group"));
    REQUIRE((group->getChildren()[1]->nodeTypeName() == "Transform"));
    REQUIRE((group->getChildren()[0] == group->getChildren()[2]));
    const auto &body = again.scene.protoDeclarations.at(0)->body;
    const auto &sequence = body.nodeStatements.at(body.nodes.at(0));
    REQUIRE((sequence.size() == 5));
    REQUIRE((sequence[0].proto && sequence[0].proto->name == "Leaf"));
    REQUIRE((sequence[1].externProto && sequence[1].externProto->name == "Unused"));
  }
}

static void descendantPrototypeHoistGuardTest() {
  auto doc = x3d::codec::parseDocument(R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='Leaf'><ProtoBody><Shape/></ProtoBody></ProtoDeclare>
<ProtoDeclare name='Wrap'><ProtoBody><Group>
<Group><ProtoInstance name='Leaf'/></Group>
<ProtoDeclare name='Leaf'><ProtoBody><Transform/></ProtoBody></ProtoDeclare>
<ProtoInstance name='Leaf'/>
</Group></ProtoBody></ProtoDeclare></Scene></X3D>)");
  bool rejected = false;
  try {
    (void)x3d::codec::VrmlWriter().writeDocument(doc);
  } catch (const std::runtime_error &e) {
    rejected = std::string(e.what()).find("cannot currently preserve") != std::string::npos;
  }
  REQUIRE((rejected)); // Hoisting would rebind the descendant to Transform.
}

static void nodeContainedMutationTest() {
  const char *xml =
      "<X3D version='4.0'><Scene><ProtoDeclare name='P'><ProtoBody><Group>"
      "<Group DEF='Shared'/><Group USE='Shared'/>"
      "<ProtoDeclare name='Unused'><ProtoBody><Shape/></ProtoBody></ProtoDeclare>"
      "<Group USE='Shared'/>"
      "</Group></ProtoBody></ProtoDeclare></Scene></X3D>";
  auto doc = x3d::codec::parseDocument(xml);
  auto body = doc.scene.protoDeclarations[0]->body.nodes.front();
  auto group = std::dynamic_pointer_cast<x3d::nodes::Group>(body);
  REQUIRE((group && group->getChildren().size() == 3));
  auto shared = group->getChildren().front();
  REQUIRE((group->getChildren()[1] == shared && group->getChildren()[2] == shared));
  group->setChildren({shared, shared, std::make_shared<x3d::nodes::Shape>()});
  auto roundtrip = x3d::codec::parseDocument(
      x3d::codec::XmlWriter().writeDocument(doc));
  const auto &again = roundtrip.scene.protoDeclarations[0]->body;
  auto replay = std::dynamic_pointer_cast<x3d::nodes::Group>(again.nodes.front());
  REQUIRE((replay && replay->getChildren().size() == 3));
  REQUIRE((replay->getChildren()[0] == replay->getChildren()[1]));
  REQUIRE((replay->getChildren()[2]->nodeTypeName() == "Shape"));
  auto sequence = again.nodeStatements.find(
      std::weak_ptr<x3d::nodes::X3DNode>(again.nodes.front()));
  REQUIRE((sequence != again.nodeStatements.end()));
  REQUIRE((sequence->second.size() == 4)); // unused declaration retained
  REQUIRE((sequence->second[2].proto && sequence->second[2].proto->name == "Unused"));
}

static void nodeContainedReorderTest() {
  const char *xml =
      "<X3D version='4.0'><Scene><ProtoDeclare name='P'><ProtoBody><Group>"
      "<Shape DEF='A'/><Shape DEF='B'/>"
      "</Group></ProtoBody></ProtoDeclare></Scene></X3D>";
  auto doc = x3d::codec::parseDocument(xml);
  auto group = std::dynamic_pointer_cast<x3d::nodes::Group>(
      doc.scene.protoDeclarations[0]->body.nodes.front());
  REQUIRE((group && group->getChildren().size() == 2));
  auto a = group->getChildren()[0];
  auto b = group->getChildren()[1];
  auto c = std::make_shared<x3d::nodes::Shape>();
  c->setDEF("C");
  group->setChildren({b, c, a});

  const struct {
    x3d::codec::Encoding encoding;
    std::string text;
  } outputs[] = {
      {x3d::codec::Encoding::XML, x3d::codec::XmlWriter().writeDocument(doc)},
      {x3d::codec::Encoding::XML,
       x3d::codec::CanonicalXmlWriter().writeDocument(doc)},
      {x3d::codec::Encoding::ClassicVRML,
       x3d::codec::VrmlWriter().writeDocument(doc)},
      {x3d::codec::Encoding::JSON, x3d::codec::JsonWriter().writeDocument(doc)},
  };
  for (const auto &output : outputs) {
    auto roundtrip = x3d::codec::parseDocument(output.text, output.encoding);
    auto replay = std::dynamic_pointer_cast<x3d::nodes::Group>(
        roundtrip.scene.protoDeclarations[0]->body.nodes.front());
    REQUIRE((replay && replay->getChildren().size() == 3));
    REQUIRE((replay->getChildren()[0]->getDEF() == "B"));
    REQUIRE((replay->getChildren()[1]->getDEF() == "C"));
    REQUIRE((replay->getChildren()[2]->getDEF() == "A"));
  }
}

static void repeatedNodeFieldJsonGuardTest() {
  const char *xml =
      "<X3D version='4.0'><Scene><ProtoDeclare name='P'><ProtoBody><Group>"
      "<Group/><MetadataString containerField='metadata'/>"
      "<Group/></Group></ProtoBody></ProtoDeclare></Scene></X3D>";
  auto doc = x3d::codec::parseDocument(xml);
  bool rejected = false;
  try {
    (void)x3d::codec::JsonWriter().writeDocument(doc);
  } catch (const std::runtime_error &e) {
    rejected = std::string(e.what()).find("cannot currently preserve") !=
               std::string::npos;
  }
  REQUIRE((rejected));
}

static void interfaceDefaultContainedProtoTest() {
  const char *xml = R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='Outer'><ProtoInterface><field name='parts' type='MFNode' accessType='inputOutput'>
<Group DEF='DefaultGroup'><ProtoDeclare name='Leaf'><ProtoBody><Shape/></ProtoBody></ProtoDeclare>
<ProtoInstance name='Leaf'/><ProtoDeclare name='Unused'><ProtoBody><Box/></ProtoBody></ProtoDeclare>
</Group></field></ProtoInterface><ProtoBody><Group><IS><connect nodeField='children' protoField='parts'/></IS></Group></ProtoBody></ProtoDeclare>
<ProtoInstance name='Outer'/><ProtoInstance name='Outer'/></Scene></X3D>)";
  const char *classic = R"(#X3D V4.0 utf8
PROFILE Immersive
PROTO Outer [ inputOutput MFNode parts [
  DEF DefaultGroup Group {
    PROTO Leaf [ ] { Shape { } }
    children [ Leaf { } ]
    PROTO Unused [ ] { Box { } }
  }
] ] { Group { children IS parts } }
Outer { }
Outer { }
)";
  auto check = [](const x3d::runtime::X3DDocument &doc) {
    REQUIRE((doc.scene.protoDeclarations.size() == 1));
    const auto &decl = *doc.scene.protoDeclarations[0];
    REQUIRE((decl.interface.size() == 1 && decl.interface[0].nodeDefault.size() == 1));
    auto owner = decl.interface[0].nodeDefault.front();
    auto entries = decl.body.nodeStatements.find(std::weak_ptr<x3d::nodes::X3DNode>(owner));
    REQUIRE((entries != decl.body.nodeStatements.end() && entries->second.size() == 3));
    REQUIRE((entries->second[0].kind == x3d::runtime::ProtoBodyStatement::Kind::Proto));
    REQUIRE((entries->second[1].kind == x3d::runtime::ProtoBodyStatement::Kind::Instance));
    REQUIRE((entries->second[2].kind == x3d::runtime::ProtoBodyStatement::Kind::Proto));
    REQUIRE((decl.body.nestedInstances.size() == 1));
    REQUIRE((decl.body.nestedInstances[0].parent.lock() == owner));
    REQUIRE((decl.body.nestedInstances[0].declaration == entries->second[0].proto));
    REQUIRE((doc.scene.protoInstances.size() == 2));
    REQUIRE((doc.scene.rootNodes.size() == 2));
    auto outerA = std::dynamic_pointer_cast<x3d::nodes::Group>(doc.scene.rootNodes[0]);
    auto outerB = std::dynamic_pointer_cast<x3d::nodes::Group>(doc.scene.rootNodes[1]);
    REQUIRE((outerA && outerB && outerA != outerB));
    REQUIRE((outerA->getChildren().size() == 1 && outerB->getChildren().size() == 1));
    auto defaultA = std::dynamic_pointer_cast<x3d::nodes::Group>(outerA->getChildren()[0]);
    auto defaultB = std::dynamic_pointer_cast<x3d::nodes::Group>(outerB->getChildren()[0]);
    REQUIRE((defaultA && defaultB && defaultA != defaultB));
    REQUIRE((defaultA->getChildren().size() == 1 && defaultB->getChildren().size() == 1));
    REQUIRE((defaultA->getChildren()[0]->nodeTypeName() == "Shape"));
    REQUIRE((defaultB->getChildren()[0]->nodeTypeName() == "Shape"));
    REQUIRE((defaultA->getChildren()[0] != defaultB->getChildren()[0]));
  };
  auto original = x3d::codec::parseDocument(xml);
  check(original);
  check(x3d::codec::parseDocument(classic, x3d::codec::Encoding::ClassicVRML));
  struct Output { x3d::codec::Encoding encoding; std::string text; } outputs[] = {
      {x3d::codec::Encoding::XML, x3d::codec::XmlWriter().writeDocument(original)},
      {x3d::codec::Encoding::XML, x3d::codec::CanonicalXmlWriter().writeDocument(original)},
      {x3d::codec::Encoding::ClassicVRML, x3d::codec::VrmlWriter().writeDocument(original)},
      {x3d::codec::Encoding::JSON, x3d::codec::JsonWriter().writeDocument(original)},
  };
  for (const auto &output : outputs)
    check(x3d::codec::parseDocument(output.text, output.encoding));
}

static void sfNodeDefaultContainedProtoTest() {
  const char *xml = R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='Outer'><ProtoInterface><field name='proxy' type='SFNode' accessType='inputOutput'>
<Group><ProtoDeclare name='Leaf'><ProtoBody><Shape/></ProtoBody></ProtoDeclare>
<ProtoInstance name='Leaf'/></Group></field></ProtoInterface>
<ProtoBody><Collision><IS><connect nodeField='proxy' protoField='proxy'/></IS></Collision></ProtoBody>
</ProtoDeclare></Scene></X3D>)";
  const char *classic = R"(#X3D V4.0 utf8
PROFILE Immersive
PROTO Outer [ inputOutput SFNode proxy Group {
  PROTO Leaf [ ] { Shape { } }
  children [ Leaf { } ]
} ] { Collision { proxy IS proxy } }
)";
  auto check = [](const x3d::runtime::X3DDocument &doc) {
    REQUIRE((doc.scene.protoDeclarations.size() == 1));
    const auto &decl = *doc.scene.protoDeclarations[0];
    REQUIRE((decl.interface.size() == 1 && decl.interface[0].nodeDefault.size() == 1));
    auto owner = decl.interface[0].nodeDefault.front();
    auto entries = decl.body.nodeStatements.find(std::weak_ptr<x3d::nodes::X3DNode>(owner));
    REQUIRE((entries != decl.body.nodeStatements.end() && entries->second.size() == 2));
    REQUIRE((entries->second[0].kind == x3d::runtime::ProtoBodyStatement::Kind::Proto));
    REQUIRE((entries->second[1].kind == x3d::runtime::ProtoBodyStatement::Kind::Instance));
    REQUIRE((decl.body.nestedInstances.size() == 1));
    REQUIRE((decl.body.nestedInstances[0].parent.lock() == owner));
    REQUIRE((decl.body.nestedInstances[0].declaration == entries->second[0].proto));
  };
  auto original = x3d::codec::parseDocument(xml);
  check(original);
  check(x3d::codec::parseDocument(classic, x3d::codec::Encoding::ClassicVRML));
  check(x3d::codec::parseDocument(x3d::codec::XmlWriter().writeDocument(original)));
  check(x3d::codec::parseDocument(x3d::codec::CanonicalXmlWriter().writeDocument(original)));
  check(x3d::codec::parseDocument(x3d::codec::VrmlWriter().writeDocument(original),
                                   x3d::codec::Encoding::ClassicVRML));
  check(x3d::codec::parseDocument(x3d::codec::JsonWriter().writeDocument(original),
                                   x3d::codec::Encoding::JSON));
}

static void directDefaultInstanceTest() {
  const char *xml = R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='Leaf'><ProtoBody><Group><Shape/></Group></ProtoBody></ProtoDeclare>
<ProtoDeclare name='Outer'><ProtoInterface>
<field name='parts' type='MFNode' accessType='inputOutput'>
<ProtoInstance name='Leaf' DEF='Shared'/><Group USE='Shared'/><Shape/></field>
<field name='single' type='SFNode' accessType='initializeOnly'><Group USE='Shared'/></field>
</ProtoInterface><ProtoBody>
<Group><IS><connect nodeField='children' protoField='parts'/></IS></Group>
<Group USE='Shared'/></ProtoBody></ProtoDeclare>
<ProtoInstance name='Outer'/><ProtoInstance name='Outer'/>
</Scene></X3D>)";
  const char *classic = R"(#X3D V4.0 utf8
PROFILE Immersive
PROTO Leaf [ ] { Group { children [ Shape { } ] } }
PROTO Outer [
  inputOutput MFNode parts [ DEF Shared Leaf { } USE Shared Shape { } ]
  initializeOnly SFNode single USE Shared
] { Group { children IS parts } USE Shared }
Outer { }
Outer { }
)";
  auto check = [](const x3d::runtime::X3DDocument &doc) {
    REQUIRE((doc.scene.protoDeclarations.size() == 2));
    const auto &outer = *doc.scene.protoDeclarations[1];
    REQUIRE((outer.interface.size() == 2));
    REQUIRE((outer.interface[0].nodeDefault.size() == 3));
    auto source = outer.interface[0].nodeDefault[0];
    auto wrapper = std::dynamic_pointer_cast<x3d::runtime::ProtoInstanceTemplate>(source);
    REQUIRE((wrapper && wrapper->instance.name == "Leaf"));
    REQUIRE((source->getDEF() == "Shared"));
    REQUIRE((outer.interface[0].nodeDefault[1] == source));
    REQUIRE((outer.interface[0].nodeDefault[2]->nodeTypeName() == "Shape"));
    REQUIRE((outer.interface[1].nodeDefault.size() == 1));
    REQUIRE((outer.interface[1].nodeDefault[0] == source));
    REQUIRE((outer.body.nestedInstances.empty()));
    REQUIRE((outer.body.nodes.size() == 2 && outer.body.nodes[1] == source));
    REQUIRE((outer.body.orderedStatements().size() == 2));
    REQUIRE((doc.scene.rootNodes.size() == 2));
    auto a = std::dynamic_pointer_cast<x3d::nodes::Group>(doc.scene.rootNodes[0]);
    auto b = std::dynamic_pointer_cast<x3d::nodes::Group>(doc.scene.rootNodes[1]);
    REQUIRE((a && b && a != b));
    REQUIRE((a->getChildren().size() == 3 && b->getChildren().size() == 3));
    REQUIRE((a->getChildren()[0] && b->getChildren()[0]));
    REQUIRE((a->getChildren()[0]->nodeTypeName() == "Group"));
    REQUIRE((a->getChildren()[0] == a->getChildren()[1]));
    REQUIRE((b->getChildren()[0] == b->getChildren()[1]));
    REQUIRE((a->getChildren()[0] != b->getChildren()[0]));
    REQUIRE((a->getChildren()[2]->nodeTypeName() == "Shape"));
    REQUIRE((doc.scene.protoPeerNodes.size() == 2));
    REQUIRE((doc.scene.protoPeerNodes[0] == a->getChildren()[0]));
    REQUIRE((doc.scene.protoPeerNodes[1] == b->getChildren()[0]));
    REQUIRE((doc.protoWarnings.empty()));
  };
  auto original = x3d::codec::parseDocument(xml);
  check(original);
  check(x3d::codec::parseDocument(classic, x3d::codec::Encoding::ClassicVRML));
  check(x3d::codec::parseDocument(x3d::codec::XmlWriter().writeDocument(original)));
  check(x3d::codec::parseDocument(x3d::codec::CanonicalXmlWriter().writeDocument(original)));
  check(x3d::codec::parseDocument(x3d::codec::VrmlWriter().writeDocument(original),
                                  x3d::codec::Encoding::ClassicVRML));
  check(x3d::codec::parseDocument(x3d::codec::JsonWriter().writeDocument(original),
                                  x3d::codec::Encoding::JSON));

  const char *sfXml = R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='Leaf'><ProtoBody><Group/></ProtoBody></ProtoDeclare>
<ProtoDeclare name='Outer'><ProtoInterface>
<field name='proxy' type='SFNode' accessType='initializeOnly'><ProtoInstance name='Leaf'/></field>
</ProtoInterface><ProtoBody><Collision><IS><connect nodeField='proxy' protoField='proxy'/></IS>
</Collision></ProtoBody></ProtoDeclare><ProtoInstance name='Outer'/><ProtoInstance name='Outer'/>
</Scene></X3D>)";
  auto sf = x3d::codec::parseDocument(sfXml);
  REQUIRE((sf.scene.rootNodes.size() == 2));
  auto first = std::dynamic_pointer_cast<x3d::nodes::Collision>(sf.scene.rootNodes[0]);
  auto second = std::dynamic_pointer_cast<x3d::nodes::Collision>(sf.scene.rootNodes[1]);
  REQUIRE((first && second && first->getProxy() && second->getProxy()));
  REQUIRE((first->getProxy()->nodeTypeName() == "Group"));
  REQUIRE((first->getProxy() != second->getProxy()));
  REQUIRE((sf.scene.protoPeerNodes.empty()));
  REQUIRE((sf.protoWarnings.empty()));
}

static void unresolvedDirectDefaultRoundTripTest() {
  const char *xml = R"(<X3D version='4.0'><Scene>
<ExternProtoDeclare name='Remote' url='"missing.x3d#Remote"'/>
<ProtoDeclare name='Outer'><ProtoInterface>
<field name='items' type='MFNode' accessType='inputOutput'>
<Shape/><ProtoInstance name='Remote'/></field></ProtoInterface>
<ProtoBody><Group><IS><connect nodeField='children' protoField='items'/></IS>
</Group></ProtoBody></ProtoDeclare><ProtoInstance name='Outer'/>
</Scene></X3D>)";
  int resolves = 0;
  auto resolver = [&resolves](const std::vector<std::string> &,
                             const std::string &) -> std::shared_ptr<x3d::runtime::ProtoDeclaration> {
    ++resolves;
    return nullptr;
  };
  auto doc = x3d::codec::parseDocument(xml, x3d::codec::Encoding::XML, "", resolver);
  REQUIRE((resolves == 1));
  REQUIRE((doc.protoWarnings.size() == 1));
  REQUIRE((doc.protoWarnings[0].kind == x3d::runtime::ProtoWarning::Kind::UnresolvedExtern));
  REQUIRE((doc.scene.rootNodes.size() == 1));
  auto body = std::dynamic_pointer_cast<x3d::nodes::Group>(doc.scene.rootNodes[0]);
  REQUIRE((body && body->getChildren().size() == 2));
  REQUIRE((body->getChildren()[0]->nodeTypeName() == "Shape"));
  REQUIRE((!body->getChildren()[1]));
  auto source = std::dynamic_pointer_cast<x3d::runtime::ProtoInstanceTemplate>(
      doc.scene.protoDeclarations[0]->interface[0].nodeDefault[1]);
  REQUIRE((source && source->instance.externDeclaration));

  auto output = x3d::codec::XmlWriter().writeDocument(doc);
  auto again = x3d::codec::parseDocument(output, x3d::codec::Encoding::XML, "", resolver);
  REQUIRE((again.scene.protoDeclarations.size() == 1));
  const auto &defaultNodes = again.scene.protoDeclarations[0]->interface[0].nodeDefault;
  REQUIRE((defaultNodes.size() == 2));
  auto retained = std::dynamic_pointer_cast<x3d::runtime::ProtoInstanceTemplate>(defaultNodes[1]);
  REQUIRE((retained && retained->instance.name == "Remote"));
  REQUIRE((retained->instance.externDeclaration));
}

static void directDefaultInstanceIsForwardTest() {
  const char *xml = R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='Leaf'><ProtoInterface>
<field name='items' type='MFNode' accessType='inputOutput'/></ProtoInterface>
<ProtoBody><Group><IS><connect nodeField='children' protoField='items'/></IS></Group>
</ProtoBody></ProtoDeclare>
<ProtoDeclare name='Outer'><ProtoInterface>
<field name='supplied' type='MFNode' accessType='inputOutput'><Shape/></field>
<field name='proxy' type='SFNode' accessType='initializeOnly'>
<ProtoInstance name='Leaf'><IS><connect nodeField='items' protoField='supplied'/></IS>
</ProtoInstance></field></ProtoInterface>
<ProtoBody><Collision><IS><connect nodeField='proxy' protoField='proxy'/></IS>
</Collision></ProtoBody></ProtoDeclare>
<ProtoInstance name='Outer'><fieldValue name='supplied'><Shape DEF='Override'/></fieldValue>
</ProtoInstance></Scene></X3D>)";
  auto doc = x3d::codec::parseDocument(xml);
  REQUIRE((doc.scene.rootNodes.size() == 1));
  REQUIRE((doc.scene.protoInstances.size() == 1));
  const auto &caller = doc.scene.protoInstances[0].fieldValues[0].nodeValue;
  REQUIRE((caller.size() == 1));
  auto outer = std::dynamic_pointer_cast<x3d::nodes::Collision>(doc.scene.rootNodes[0]);
  REQUIRE((outer && outer->getProxy()));
  auto inner = std::dynamic_pointer_cast<x3d::nodes::Group>(outer->getProxy());
  REQUIRE((inner && inner->getChildren().size() == 1));
  REQUIRE((inner->getChildren()[0] == caller[0]));
  REQUIRE((doc.protoWarnings.empty()));
}

static void instanceNodeValueRootTest() {
  const char *xml = R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='Leaf'><ProtoBody><Shape/></ProtoBody></ProtoDeclare>
<ProtoDeclare name='Holder'><ProtoInterface>
<field name='payload' type='SFNode' accessType='initializeOnly'/></ProtoInterface>
<ProtoBody><Collision><IS><connect nodeField='proxy' protoField='payload'/></IS>
</Collision></ProtoBody></ProtoDeclare>
<ProtoDeclare name='Outer'><ProtoInterface>
<field name='value' type='SFNode' accessType='initializeOnly'/></ProtoInterface>
<ProtoBody><Collision><IS><connect nodeField='proxy' protoField='value'/></IS>
</Collision></ProtoBody></ProtoDeclare>
<ProtoInstance name='Outer'><fieldValue name='value'><ProtoInstance name='Holder'>
<fieldValue name='payload'><ProtoInstance name='Leaf'/></fieldValue>
</ProtoInstance></fieldValue></ProtoInstance></Scene></X3D>)";
  const char *classic = R"(#X3D V4.0 utf8
PROFILE Immersive
PROTO Leaf [ ] { Shape { } }
PROTO Holder [ initializeOnly SFNode payload NULL ] { Collision { proxy IS payload } }
PROTO Outer [ initializeOnly SFNode value NULL ] { Collision { proxy IS value } }
Outer { value Holder { payload Leaf { } } }
)";
  auto check = [](const x3d::runtime::X3DDocument &doc) {
    REQUIRE((doc.scene.protoDeclarations.size() == 3));
    REQUIRE((doc.scene.protoInstances.size() == 1));
    REQUIRE((doc.scene.rootNodes.size() == 1));
    const auto &outerValue = doc.scene.protoInstances[0].fieldValues;
    REQUIRE((outerValue.size() == 1 && outerValue[0].nodeValue.size() == 1));
    auto first = std::dynamic_pointer_cast<x3d::nodes::Collision>(doc.scene.rootNodes[0]);
    REQUIRE((first && first->getProxy()));
    auto second = std::dynamic_pointer_cast<x3d::nodes::Collision>(first->getProxy());
    REQUIRE((second && second->getProxy()));
    REQUIRE((outerValue[0].nodeValue[0] == second));
    auto holderSource = doc.scene.expandedSources.find(second.get());
    REQUIRE((holderSource != doc.scene.expandedSources.end()));
    REQUIRE((holderSource->second.name == "Holder"));
    REQUIRE((holderSource->second.fieldValues.size() == 1));
    REQUIRE((holderSource->second.fieldValues[0].nodeValue.at(0) == second->getProxy()));
    auto leafSource = doc.scene.expandedSources.find(second->getProxy().get());
    REQUIRE((leafSource != doc.scene.expandedSources.end()));
    REQUIRE((leafSource->second.name == "Leaf"));
    REQUIRE((second->getProxy()->nodeTypeName() == "Shape"));
    REQUIRE((doc.scene.protoPeerNodes.empty()));
    REQUIRE((doc.protoWarnings.empty()));
  };
  auto original = x3d::codec::parseDocument(xml);
  check(original);
  check(x3d::codec::parseDocument(classic, x3d::codec::Encoding::ClassicVRML));
  check(x3d::codec::parseDocument(x3d::codec::XmlWriter().writeDocument(original)));
  check(x3d::codec::parseDocument(x3d::codec::CanonicalXmlWriter().writeDocument(original)));
  check(x3d::codec::parseDocument(x3d::codec::VrmlWriter().writeDocument(original),
                                  x3d::codec::Encoding::ClassicVRML));
  check(x3d::codec::parseDocument(x3d::codec::JsonWriter().writeDocument(original),
                                  x3d::codec::Encoding::JSON));
}

static void instanceNodeValueMixedTest() {
  const char *xml = R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='Leaf'><ProtoBody><Shape/></ProtoBody></ProtoDeclare>
<ProtoDeclare name='Outer'><ProtoInterface>
<field name='items' type='MFNode' accessType='initializeOnly'/></ProtoInterface>
<ProtoBody><Group><IS><connect nodeField='children' protoField='items'/></IS>
</Group></ProtoBody></ProtoDeclare>
<ProtoInstance name='Outer'><fieldValue name='items'><Shape DEF='A'/>
<ProtoInstance name='Leaf'/><Group><ProtoInstance name='Leaf'/></Group>
<Shape DEF='B'/></fieldValue></ProtoInstance></Scene></X3D>)";
  auto check = [](const x3d::runtime::X3DDocument &doc) {
    REQUIRE((doc.scene.protoInstances.size() == 1));
    const auto &value = doc.scene.protoInstances[0].fieldValues[0].nodeValue;
    REQUIRE((value.size() == 4));
    REQUIRE((value[0]->nodeTypeName() == "Shape"));
    REQUIRE((value[1]->nodeTypeName() == "Shape"));
    auto directSource = doc.scene.expandedSources.find(value[1].get());
    REQUIRE((directSource != doc.scene.expandedSources.end()));
    REQUIRE((directSource->second.name == "Leaf"));
    auto sourceGroup = std::dynamic_pointer_cast<x3d::nodes::Group>(value[2]);
    REQUIRE((sourceGroup && sourceGroup->getChildren().size() == 1));
    REQUIRE((sourceGroup->getChildren()[0]->nodeTypeName() == "Shape"));
    auto containedSource = doc.scene.expandedSources.find(sourceGroup->getChildren()[0].get());
    REQUIRE((containedSource != doc.scene.expandedSources.end()));
    REQUIRE((containedSource->second.name == "Leaf"));
    REQUIRE((value[3]->nodeTypeName() == "Shape"));
    REQUIRE((doc.scene.rootNodes.size() == 1));
    auto root = std::dynamic_pointer_cast<x3d::nodes::Group>(doc.scene.rootNodes[0]);
    REQUIRE((root && root->getChildren().size() == 4));
    REQUIRE((root->getChildren()[0] == value[0]));
    REQUIRE((root->getChildren()[1] == value[1]));
    REQUIRE((root->getChildren()[2] == value[2]));
    REQUIRE((root->getChildren()[3] == value[3]));
    REQUIRE((sourceGroup->getChildren()[0]->nodeTypeName() == "Shape"));
    REQUIRE((doc.scene.protoPeerNodes.empty()));
    REQUIRE((doc.protoWarnings.empty()));
  };
  auto original = x3d::codec::parseDocument(xml);
  check(original);
  check(x3d::codec::parseDocument(x3d::codec::XmlWriter().writeDocument(original)));
  check(x3d::codec::parseDocument(x3d::codec::CanonicalXmlWriter().writeDocument(original)));
  check(x3d::codec::parseDocument(x3d::codec::VrmlWriter().writeDocument(original),
                                  x3d::codec::Encoding::ClassicVRML));
  check(x3d::codec::parseDocument(x3d::codec::JsonWriter().writeDocument(original),
                                  x3d::codec::Encoding::JSON));
}

static void instanceNodeValueSharedCallerTest() {
  const char *xml = R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='Leaf'><ProtoBody><Shape/></ProtoBody></ProtoDeclare>
<ProtoDeclare name='Outer'><ProtoInterface>
<field name='items' type='MFNode' accessType='initializeOnly'>
<Group><ProtoInstance name='Leaf'/></Group></field></ProtoInterface>
<ProtoBody><Group><IS><connect nodeField='children' protoField='items'/></IS>
</Group></ProtoBody></ProtoDeclare>
<ProtoInstance name='Outer' DEF='O1'><fieldValue name='items'>
<Group DEF='SharedParent'><ProtoInstance name='Leaf'/></Group>
<ProtoInstance name='Leaf' DEF='Direct'/></fieldValue></ProtoInstance>
<ProtoInstance name='Outer' DEF='O2'><fieldValue name='items'>
<Group USE='SharedParent'/><ProtoInstance USE='Direct'/></fieldValue></ProtoInstance>
<Group USE='SharedParent'/><ProtoInstance USE='Direct'/>
<ProtoInstance name='Outer' DEF='O3'/>
<ProtoInstance name='Outer' DEF='O4'/>
</Scene></X3D>)";
  const char *classic = R"(#X3D V4.0 utf8
PROFILE Immersive
PROTO Leaf [ ] { Shape { } }
PROTO Outer [ initializeOnly MFNode items [ Group { children [ Leaf { } ] } ] ]
{ Group { children IS items } }
DEF O1 Outer { items [ DEF SharedParent Group { children [ Leaf { } ] }
                DEF Direct Leaf { } ] }
DEF O2 Outer { items [ USE SharedParent USE Direct ] }
USE SharedParent
USE Direct
DEF O3 Outer { }
DEF O4 Outer { }
)";
  auto check = [](const x3d::runtime::X3DDocument &doc) {
    std::vector<const x3d::runtime::ProtoInstance *> outerInstances;
    for (const auto &instance : doc.scene.protoInstances) {
      if (instance.name == "Outer") outerInstances.push_back(&instance);
      else if (!instance.parent.lock())
        REQUIRE((instance.name == "Leaf" && instance.DEF == "Direct"));
    }
    REQUIRE((outerInstances.size() == 4));
    REQUIRE((doc.scene.rootNodes.size() == 6));
    auto first = std::dynamic_pointer_cast<x3d::nodes::Group>(doc.scene.resolve("O1"));
    auto second = std::dynamic_pointer_cast<x3d::nodes::Group>(doc.scene.resolve("O2"));
    auto third = std::dynamic_pointer_cast<x3d::nodes::Group>(doc.scene.resolve("O3"));
    auto fourth = std::dynamic_pointer_cast<x3d::nodes::Group>(doc.scene.resolve("O4"));
    REQUIRE((first && second && third && fourth));
    REQUIRE((first->getChildren().size() == 2 && second->getChildren().size() == 2));
    auto callerParent = std::dynamic_pointer_cast<x3d::nodes::Group>(first->getChildren()[0]);
    REQUIRE((callerParent && callerParent->getChildren().size() == 1));
    REQUIRE((callerParent->getChildren()[0]->nodeTypeName() == "Shape"));
    REQUIRE((second->getChildren()[0] == callerParent));
    REQUIRE((std::find(doc.scene.rootNodes.begin(), doc.scene.rootNodes.end(),
                     callerParent) != doc.scene.rootNodes.end()));
    REQUIRE((first->getChildren()[1]->nodeTypeName() == "Shape"));
    REQUIRE((second->getChildren()[1] == first->getChildren()[1]));
    REQUIRE((std::find(doc.scene.rootNodes.begin(), doc.scene.rootNodes.end(),
                     first->getChildren()[1]) != doc.scene.rootNodes.end()));
    const auto &firstValues = outerInstances[0]->fieldValues[0].nodeValue;
    const auto &secondValues = outerInstances[1]->fieldValues[0].nodeValue;
    REQUIRE((firstValues.size() == 2 && secondValues.size() == 2));
    REQUIRE((firstValues[0] == callerParent && secondValues[0] == callerParent));
    REQUIRE((firstValues[1] == secondValues[1]));
    REQUIRE((third->getChildren().size() == 1 && fourth->getChildren().size() == 1));
    auto defaultA = std::dynamic_pointer_cast<x3d::nodes::Group>(third->getChildren()[0]);
    auto defaultB = std::dynamic_pointer_cast<x3d::nodes::Group>(fourth->getChildren()[0]);
    REQUIRE((defaultA && defaultB && defaultA != defaultB));
    REQUIRE((defaultA->getChildren().size() == 1 && defaultB->getChildren().size() == 1));
    REQUIRE((defaultA->getChildren()[0]->nodeTypeName() == "Shape"));
    REQUIRE((defaultB->getChildren()[0]->nodeTypeName() == "Shape"));
    REQUIRE((defaultA->getChildren()[0] != defaultB->getChildren()[0]));
    REQUIRE((doc.scene.protoPeerNodes.empty()));
    REQUIRE((doc.protoWarnings.empty()));
  };
  auto original = x3d::codec::parseDocument(xml);
  check(original);
  check(x3d::codec::parseDocument(classic, x3d::codec::Encoding::ClassicVRML));
  auto xmlOutput = x3d::codec::XmlWriter().writeDocument(original);
  check(x3d::codec::parseDocument(xmlOutput));
  check(x3d::codec::parseDocument(x3d::codec::CanonicalXmlWriter().writeDocument(original)));
  check(x3d::codec::parseDocument(x3d::codec::VrmlWriter().writeDocument(original),
                                  x3d::codec::Encoding::ClassicVRML));
  check(x3d::codec::parseDocument(x3d::codec::JsonWriter().writeDocument(original),
                                  x3d::codec::Encoding::JSON));
}

static void unresolvedInstanceNodeValueTest() {
  const char *xml = R"(<X3D version='4.0'><Scene>
<ExternProtoDeclare name='Remote' url='"missing.x3d#Remote"'/>
<ProtoDeclare name='Outer'><ProtoInterface>
<field name='items' type='MFNode' accessType='initializeOnly'/></ProtoInterface>
<ProtoBody><Group><IS><connect nodeField='children' protoField='items'/></IS>
</Group></ProtoBody></ProtoDeclare>
<ProtoInstance name='Outer'><fieldValue name='items'><Shape/>
<ProtoInstance name='Remote'/></fieldValue></ProtoInstance></Scene></X3D>)";
  int resolves = 0;
  auto resolver = [&resolves](const std::vector<std::string> &,
                             const std::string &) -> std::shared_ptr<x3d::runtime::ProtoDeclaration> {
    ++resolves;
    return nullptr;
  };
  auto parse = [&](const std::string &text, x3d::codec::Encoding encoding) {
    auto doc = x3d::codec::parseDocument(text, encoding, "", resolver);
    REQUIRE((resolves == 1));
    resolves = 0;
    REQUIRE((doc.scene.protoInstances.size() == 1));
    REQUIRE((doc.scene.rootNodes.size() == 1));
    const auto &sources = doc.scene.protoInstances[0].fieldValues[0].nodeValue;
    REQUIRE((sources.size() == 2));
    auto source = std::dynamic_pointer_cast<x3d::runtime::ProtoInstanceTemplate>(sources[1]);
    REQUIRE((source && source->instance.name == "Remote"));
    REQUIRE((source->instance.externDeclaration));
    auto root = std::dynamic_pointer_cast<x3d::nodes::Group>(doc.scene.rootNodes[0]);
    REQUIRE((root && root->getChildren().size() == 2));
    REQUIRE((root->getChildren()[0] == sources[0]));
    REQUIRE((root->getChildren()[1] == source));
    REQUIRE((doc.protoWarnings.size() == 1));
    REQUIRE((doc.protoWarnings[0].kind == x3d::runtime::ProtoWarning::Kind::UnresolvedExtern));
    return doc;
  };
  auto original = parse(xml, x3d::codec::Encoding::XML);
  parse(x3d::codec::XmlWriter().writeDocument(original), x3d::codec::Encoding::XML);
  parse(x3d::codec::CanonicalXmlWriter().writeDocument(original), x3d::codec::Encoding::XML);
  parse(x3d::codec::VrmlWriter().writeDocument(original), x3d::codec::Encoding::ClassicVRML);
  parse(x3d::codec::JsonWriter().writeDocument(original), x3d::codec::Encoding::JSON);

}

static void sceneRootProtoOrderTest() {
  const char *xml = R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='Leaf'><ProtoBody><Group/></ProtoBody></ProtoDeclare>
<ProtoInstance name='Leaf' DEF='First'/><Shape DEF='Middle'/>
<ProtoInstance name='Leaf' DEF='Last'/><ProtoInstance USE='First'/>
<ProtoInstance name='Leaf'/></Scene></X3D>)";
  const char *classic = R"(#X3D V4.0 utf8
PROFILE Immersive
PROTO Leaf [ ] { Group { } }
DEF First Leaf { }
DEF Middle Shape { }
DEF Last Leaf { }
USE First
Leaf { }
)";
  auto check = [](const x3d::runtime::X3DDocument &doc) {
    const auto &roots = doc.scene.rootNodes;
    REQUIRE((roots.size() == 5));
    REQUIRE((roots[0]->nodeTypeName() == "Group" && roots[0]->getDEF() == "First"));
    REQUIRE((roots[1]->nodeTypeName() == "Shape" && roots[1]->getDEF() == "Middle"));
    REQUIRE((roots[2]->nodeTypeName() == "Group" && roots[2]->getDEF() == "Last"));
    REQUIRE((roots[3] == roots[0]));
    REQUIRE((roots[4]->nodeTypeName() == "Group" && roots[4]->getDEF().empty()));
    REQUIRE((roots[4] != roots[0] && roots[4] != roots[2]));
    REQUIRE((doc.protoWarnings.empty()));
  };
  auto original = x3d::codec::parseDocument(xml);
  check(original);
  check(x3d::codec::parseDocument(classic, x3d::codec::Encoding::ClassicVRML));
  check(x3d::codec::parseDocument(x3d::codec::XmlWriter().writeDocument(original)));
  check(x3d::codec::parseDocument(x3d::codec::CanonicalXmlWriter().writeDocument(original)));
  check(x3d::codec::parseDocument(x3d::codec::VrmlWriter().writeDocument(original),
                                  x3d::codec::Encoding::ClassicVRML));
  check(x3d::codec::parseDocument(x3d::codec::JsonWriter().writeDocument(original),
                                  x3d::codec::Encoding::JSON));

  auto raw = x3d::codec::XmlReader().readDocument(xml);
  REQUIRE((raw.scene.rootNodes.size() == 5));
  REQUIRE((std::dynamic_pointer_cast<x3d::runtime::ProtoInstanceTemplate>(
      raw.scene.rootNodes[0])));
  REQUIRE((raw.scene.rootNodes[3] == raw.scene.rootNodes[0]));
  REQUIRE((raw.scene.protoInstances[0].hasPlacementTemplate()));
  check(x3d::codec::parseDocument(x3d::codec::XmlWriter().writeDocument(raw)));
  auto rawClassic = x3d::codec::ClassicVrmlReader().readDocument(classic);
  REQUIRE((rawClassic.scene.rootNodes.size() == 5));
  REQUIRE((rawClassic.scene.rootNodes[3] == rawClassic.scene.rootNodes[0]));
  check(x3d::codec::parseDocument(x3d::codec::VrmlWriter().writeDocument(rawClassic),
                                  x3d::codec::Encoding::ClassicVRML));
  auto rawJson = x3d::codec::JsonReader().readDocument(
      x3d::codec::JsonWriter().writeDocument(original));
  REQUIRE((rawJson.scene.rootNodes.size() == 5));
  REQUIRE((rawJson.scene.rootNodes[3] == rawJson.scene.rootNodes[0]));
  check(x3d::codec::parseDocument(x3d::codec::JsonWriter().writeDocument(rawJson),
                                  x3d::codec::Encoding::JSON));
}

static void unresolvedRootProtoOrderTest() {
  const char *xml = R"(<X3D version='4.0'><Scene>
<ExternProtoDeclare name='Remote' url='"missing.x3d#Remote"'/>
<Shape DEF='Before'/><ProtoInstance name='Remote' DEF='Missing'/>
<Group DEF='After'/></Scene></X3D>)";
  auto check = [](const x3d::runtime::X3DDocument &doc) {
    REQUIRE((doc.scene.rootNodes.size() == 3));
    REQUIRE((doc.scene.rootNodes[0]->getDEF() == "Before"));
    auto inert = std::dynamic_pointer_cast<x3d::runtime::ProtoInstanceTemplate>(
        doc.scene.rootNodes[1]);
    REQUIRE((inert && inert->instance.name == "Remote"));
    REQUIRE((inert->getDEF() == "Missing"));
    REQUIRE((doc.scene.rootNodes[2]->getDEF() == "After"));
    REQUIRE((doc.protoWarnings.size() == 1));
    REQUIRE((doc.protoWarnings[0].kind == x3d::runtime::ProtoWarning::Kind::UnresolvedExtern));
  };
  int resolves = 0;
  auto resolver = [&resolves](const std::vector<std::string> &,
                             const std::string &) -> std::shared_ptr<x3d::runtime::ProtoDeclaration> {
    ++resolves;
    return nullptr;
  };
  auto parse = [&](const std::string &text, x3d::codec::Encoding encoding) {
    resolves = 0;
    auto doc = x3d::codec::parseDocument(text, encoding, "", resolver);
    REQUIRE((resolves == 1));
    check(doc);
    return doc;
  };
  auto original = parse(xml, x3d::codec::Encoding::XML);
  parse(x3d::codec::XmlWriter().writeDocument(original), x3d::codec::Encoding::XML);
  parse(x3d::codec::CanonicalXmlWriter().writeDocument(original), x3d::codec::Encoding::XML);
  parse(x3d::codec::VrmlWriter().writeDocument(original), x3d::codec::Encoding::ClassicVRML);
  parse(x3d::codec::JsonWriter().writeDocument(original), x3d::codec::Encoding::JSON);
  const char *childXml = R"(<X3D version='4.0'><Scene>
<ExternProtoDeclare name='Remote' url='"missing.x3d#Remote"'/>
<Group DEF='Parent'><Shape DEF='Before'/>
<ProtoInstance name='Remote' DEF='Missing'/><Group DEF='After'/>
</Group></Scene></X3D>)";
  auto parseChild = [&](const std::string &text, x3d::codec::Encoding encoding) {
    resolves = 0;
    auto doc = x3d::codec::parseDocument(text, encoding, "", resolver);
    REQUIRE((resolves == 1));
    REQUIRE((doc.scene.rootNodes.size() == 1));
    auto parent = std::dynamic_pointer_cast<x3d::nodes::Group>(doc.scene.rootNodes[0]);
    REQUIRE((parent && parent->getChildren().size() == 3));
    REQUIRE((parent->getChildren()[0]->getDEF() == "Before"));
    auto inert = std::dynamic_pointer_cast<x3d::runtime::ProtoInstanceTemplate>(
        parent->getChildren()[1]);
    REQUIRE((inert && inert->instance.name == "Remote" && inert->getDEF() == "Missing"));
    REQUIRE((parent->getChildren()[2]->getDEF() == "After"));
    REQUIRE((doc.protoWarnings.size() == 1));
    REQUIRE((doc.protoWarnings[0].kind == x3d::runtime::ProtoWarning::Kind::UnresolvedExtern));
    return doc;
  };
  auto child = parseChild(childXml, x3d::codec::Encoding::XML);
  parseChild(x3d::codec::XmlWriter().writeDocument(child), x3d::codec::Encoding::XML);
  parseChild(x3d::codec::CanonicalXmlWriter().writeDocument(child), x3d::codec::Encoding::XML);
  parseChild(x3d::codec::VrmlWriter().writeDocument(child), x3d::codec::Encoding::ClassicVRML);
  parseChild(x3d::codec::JsonWriter().writeDocument(child), x3d::codec::Encoding::JSON);
}

static void rootProtoMutationTest() {
  const char *xml = R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='Leaf'><ProtoBody><Group/></ProtoBody></ProtoDeclare>
<ProtoDeclare name='Alt'><ProtoBody><Shape/></ProtoBody></ProtoDeclare>
<ProtoInstance name='Leaf'/><Shape DEF='Middle'/>
<ProtoInstance name='Leaf' DEF='Named'/></Scene></X3D>)";
  auto readRaw = [&] { return x3d::codec::XmlReader().readDocument(xml); };
  auto checkOrder = [](const x3d::runtime::X3DDocument &doc,
                       const std::vector<std::string> &types) {
    REQUIRE((doc.scene.rootNodes.size() == types.size()));
    for (std::size_t i = 0; i < types.size(); ++i)
      REQUIRE((doc.scene.rootNodes[i]->nodeTypeName() == types[i]));
  };

  auto reordered = readRaw();
  std::swap(reordered.scene.rootNodes[0], reordered.scene.rootNodes[2]);
  auto reorderedWrite = x3d::codec::XmlWriter().writeDocument(reordered);
  auto reorderedParsed = x3d::codec::parseDocument(reorderedWrite);
  checkOrder(reorderedParsed, {"Group", "Shape", "Group"});
  REQUIRE((reorderedParsed.scene.rootNodes[0]->getDEF() == "Named"));
  REQUIRE((reorderedParsed.scene.rootNodes[2]->getDEF().empty()));
  std::vector<x3d::runtime::ProtoWarning> warnings;
  x3d::runtime::expandScene(reordered.scene, x3d::codec::noopProtoResolver, "", warnings);
  checkOrder(reordered, {"Group", "Shape", "Group"});
  REQUIRE((reordered.scene.rootNodes[0]->getDEF() == "Named"));
  REQUIRE((warnings.empty()));

  auto unnamedRemoved = readRaw();
  auto unnamedSlot = unnamedRemoved.scene.protoInstances[0].placementTemplate;
  unnamedRemoved.scene.rootNodes.erase(unnamedRemoved.scene.rootNodes.begin());
  REQUIRE((unnamedSlot.expired()));
  auto unnamedWrite = x3d::codec::XmlWriter().writeDocument(unnamedRemoved);
  auto unnamedParsed = x3d::codec::parseDocument(unnamedWrite);
  checkOrder(unnamedParsed, {"Shape", "Group"});
  warnings.clear();
  x3d::runtime::expandScene(unnamedRemoved.scene, x3d::codec::noopProtoResolver,
                            "", warnings);
  checkOrder(unnamedRemoved, {"Shape", "Group"});
  REQUIRE((warnings.empty()));

  auto namedRemoved = readRaw();
  namedRemoved.scene.rootNodes.pop_back();
  auto namedWrite = x3d::codec::XmlWriter().writeDocument(namedRemoved);
  auto namedParsed = x3d::codec::parseDocument(namedWrite);
  checkOrder(namedParsed, {"Group", "Shape"});
  warnings.clear();
  x3d::runtime::expandScene(namedRemoved.scene, x3d::codec::noopProtoResolver,
                            "", warnings);
  checkOrder(namedRemoved, {"Group", "Shape"});
  REQUIRE((warnings.empty()));

  auto changed = readRaw();
  changed.scene.protoInstances[0].name = "Alt";
  changed.scene.protoInstances[0].declaration = changed.scene.findProto("Alt");
  auto namedRecord = std::find_if(changed.scene.protoInstances.begin(),
                                  changed.scene.protoInstances.end(),
                                  [](const x3d::runtime::ProtoInstance &instance) {
                                    return instance.DEF == "Named";
                                  });
  REQUIRE((namedRecord != changed.scene.protoInstances.end()));
  namedRecord->DEF = "Renamed";
  auto changedWrite = x3d::codec::XmlWriter().writeDocument(changed);
  auto changedParsed = x3d::codec::parseDocument(changedWrite);
  checkOrder(changedParsed, {"Shape", "Shape", "Group"});
  REQUIRE((changedParsed.scene.rootNodes[2]->getDEF() == "Renamed"));
  warnings.clear();
  x3d::runtime::expandScene(changed.scene, x3d::codec::noopProtoResolver, "", warnings);
  checkOrder(changed, {"Shape", "Shape", "Group"});
  REQUIRE((changed.scene.rootNodes[2]->getDEF() == "Renamed"));
  REQUIRE((changed.scene.resolve("Renamed") == changed.scene.rootNodes[2]));
  REQUIRE((!changed.scene.resolve("Named")));
  REQUIRE((warnings.empty()));
}

static void sceneChildProtoOrderTest() {
  const char *xml = R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='Leaf'><ProtoBody><Group/></ProtoBody></ProtoDeclare>
<Group DEF='Parent'><ProtoInstance name='Leaf' DEF='First'/><Shape/>
<ProtoInstance name='Leaf' DEF='Last'/><ProtoInstance USE='First'/>
<ProtoInstance name='Leaf'/></Group></Scene></X3D>)";
  const char *classic = R"(#X3D V4.0 utf8
PROFILE Immersive
PROTO Leaf [ ] { Group { } }
DEF Parent Group { children [ DEF First Leaf { } Shape { }
  DEF Last Leaf { } USE First Leaf { } ] }
)";
  auto check = [](const x3d::runtime::X3DDocument &doc, bool hasUnnamed) {
    REQUIRE((doc.scene.rootNodes.size() == 1));
    auto parent = std::dynamic_pointer_cast<x3d::nodes::Group>(doc.scene.rootNodes[0]);
    REQUIRE((parent && parent->getDEF() == "Parent"));
    const auto &children = parent->getChildren();
    REQUIRE((children.size() == (hasUnnamed ? 5u : 4u)));
    REQUIRE((children[0]->nodeTypeName() == "Group" && children[0]->getDEF() == "First"));
    REQUIRE((children[1]->nodeTypeName() == "Shape"));
    REQUIRE((children[2]->nodeTypeName() == "Group" && children[2]->getDEF() == "Last"));
    REQUIRE((children[3] == children[0]));
    if (hasUnnamed) {
      REQUIRE((children[4]->nodeTypeName() == "Group"));
      REQUIRE((children[4]->getDEF().empty()));
    }
    REQUIRE((doc.protoWarnings.empty()));
  };
  auto original = x3d::codec::parseDocument(xml);
  check(original, true);
  check(x3d::codec::parseDocument(classic, x3d::codec::Encoding::ClassicVRML), true);
  check(x3d::codec::parseDocument(x3d::codec::XmlWriter().writeDocument(original)), true);
  check(x3d::codec::parseDocument(x3d::codec::CanonicalXmlWriter().writeDocument(original)), true);
  check(x3d::codec::parseDocument(x3d::codec::VrmlWriter().writeDocument(original),
                                  x3d::codec::Encoding::ClassicVRML), true);
  check(x3d::codec::parseDocument(x3d::codec::JsonWriter().writeDocument(original),
                                  x3d::codec::Encoding::JSON), true);

  auto raw = x3d::codec::XmlReader().readDocument(xml);
  auto rawParent = std::dynamic_pointer_cast<x3d::nodes::Group>(raw.scene.rootNodes[0]);
  REQUIRE((rawParent && rawParent->getChildren().size() == 5));
  REQUIRE((std::dynamic_pointer_cast<x3d::runtime::ProtoInstanceTemplate>(
      rawParent->getChildren()[0])));
  REQUIRE((rawParent->getChildren()[3] == rawParent->getChildren()[0]));
  check(x3d::codec::parseDocument(x3d::codec::XmlWriter().writeDocument(raw)), true);
  auto rawClassic = x3d::codec::ClassicVrmlReader().readDocument(classic);
  check(x3d::codec::parseDocument(x3d::codec::VrmlWriter().writeDocument(rawClassic),
                                  x3d::codec::Encoding::ClassicVRML), true);

  auto removed = x3d::codec::XmlReader().readDocument(xml);
  auto removedParent = std::dynamic_pointer_cast<x3d::nodes::Group>(removed.scene.rootNodes[0]);
  auto unnamedSlot = removed.scene.protoInstances.back().placementTemplate;
  auto children = removedParent->getChildren();
  children.pop_back();
  removedParent->setChildren(std::move(children));
  REQUIRE((unnamedSlot.expired()));
  check(x3d::codec::parseDocument(x3d::codec::XmlWriter().writeDocument(removed)), false);
  std::vector<x3d::runtime::ProtoWarning> warnings;
  x3d::runtime::expandScene(removed.scene, x3d::codec::noopProtoResolver, "", warnings);
  check(removed, false);
  REQUIRE((warnings.empty()));
}

static void firstProtoViewpointBindsTest() {
  const char *xmlProtoFirst = R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='VP'><ProtoBody><Viewpoint/></ProtoBody></ProtoDeclare>
<ProtoInstance name='VP' DEF='First'/><Viewpoint DEF='Second'/></Scene></X3D>)";
  const char *xmlOrdinaryFirst = R"(<X3D version='4.0'><Scene>
<ProtoDeclare name='VP'><ProtoBody><Viewpoint/></ProtoBody></ProtoDeclare>
<Viewpoint DEF='Second'/><ProtoInstance name='VP' DEF='First'/></Scene></X3D>)";
  const char *classicProtoFirst = R"(#X3D V4.0 utf8
PROFILE Immersive
PROTO VP [ ] { Viewpoint { } }
DEF First VP { }
DEF Second Viewpoint { }
)";
  const char *classicOrdinaryFirst = R"(#X3D V4.0 utf8
PROFILE Immersive
PROTO VP [ ] { Viewpoint { } }
DEF Second Viewpoint { }
DEF First VP { }
)";
  // Core §7.2.2: the first encountered bindable, including a PROTO primary,
  // initially occupies the Viewpoint binding stack.
  auto check = [](x3d::runtime::X3DDocument doc, const std::string &expected) {
    REQUIRE((doc.scene.rootNodes.size() == 2));
    auto first = doc.scene.resolve("First");
    auto second = doc.scene.resolve("Second");
    REQUIRE((first && second));
    x3d::runtime::X3DExecutionContext context;
    context.buildSceneGraph(doc.scene);
    REQUIRE((context.boundViewpoint() == doc.scene.resolve(expected).get()));
    REQUIRE((doc.protoWarnings.empty()));
  };
  auto protoFirst = x3d::codec::parseDocument(xmlProtoFirst);
  check(protoFirst, "First");
  check(x3d::codec::parseDocument(xmlOrdinaryFirst), "Second");
  check(x3d::codec::parseDocument(classicProtoFirst, x3d::codec::Encoding::ClassicVRML),
        "First");
  check(x3d::codec::parseDocument(classicOrdinaryFirst, x3d::codec::Encoding::ClassicVRML),
        "Second");
  check(x3d::codec::parseDocument(x3d::codec::XmlWriter().writeDocument(protoFirst)), "First");
  check(x3d::codec::parseDocument(x3d::codec::CanonicalXmlWriter().writeDocument(protoFirst)),
        "First");
  check(x3d::codec::parseDocument(x3d::codec::VrmlWriter().writeDocument(protoFirst),
                                  x3d::codec::Encoding::ClassicVRML), "First");
  check(x3d::codec::parseDocument(x3d::codec::JsonWriter().writeDocument(protoFirst),
                                  x3d::codec::Encoding::JSON), "First");
}

TEST_CASE("proto_front_door_test") {
  g_dataDir = X3D_TEST_DATA_DIR;
  frontDoorLocalExpandTest();
  declarationDefScopeTest();
  nestedDeclarationShadowingTest();
  frontDoorExternResolveTest();
  frontDoorExternCycleTest();
  sourceUnitProvenanceTest();
  authoredScalarPresenceTest();
  directNestedDeclarationWriteTest();
  declarationAliasWriteTest();
  nodeContainedDeclarationPlacementTest();
  nodeContainedInterleavedFieldTest();
  literalPrefixDeclarationRoundTripTest();
  descendantPrototypeHoistGuardTest();
  nodeContainedMutationTest();
  nodeContainedReorderTest();
  repeatedNodeFieldJsonGuardTest();
  interfaceDefaultContainedProtoTest();
  sfNodeDefaultContainedProtoTest();
  directDefaultInstanceTest();
  unresolvedDirectDefaultRoundTripTest();
  directDefaultInstanceIsForwardTest();
  instanceNodeValueRootTest();
  instanceNodeValueMixedTest();
  instanceNodeValueSharedCallerTest();
  unresolvedInstanceNodeValueTest();
  sceneRootProtoOrderTest();
  unresolvedRootProtoOrderTest();
  rootProtoMutationTest();
  sceneChildProtoOrderTest();
  firstProtoViewpointBindsTest();
}
