// proto_nested_body_test.cpp
// A <ProtoInstance> nested inside a <ProtoBody> must expand once per outer
// instantiation and land inside the outer primary's cloned subtree — verified
// end-to-end through the XML and ClassicVRML front doors.
#include "X3DParse.hpp"
#include "X3DReflection.hpp"

#include <any>
#include "doctest/doctest.h"
#include <memory>
#include <string>

using namespace x3d;

static const FieldInfo *fieldByName(const X3DNode &n, const std::string &name) {
  for (const auto &f : n.fields()) if (f.x3dName == name) return &f;
  return nullptr;
}
static std::shared_ptr<X3DNode> firstRoot(const runtime::X3DDocument &doc) {
  for (auto &n : doc.scene.rootNodes) if (n) return n;
  return nullptr;
}

static void xmlNestedTest() {
  const char *xml =
      "<X3D version='4.0'><Scene>"
      "<ProtoDeclare name='Leaf'><ProtoBody><Box/></ProtoBody></ProtoDeclare>"
      "<ProtoDeclare name='Wrap'><ProtoBody>"
      "<Transform><ProtoInstance name='Leaf' containerField='children'/></Transform>"
      "</ProtoBody></ProtoDeclare>"
      "<ProtoInstance name='Wrap'/></Scene></X3D>";
  auto doc = codec::parseDocument(xml);
  auto root = firstRoot(doc);
  CHECK((root && root->nodeTypeName() == "Transform"));
  auto kids = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
      fieldByName(*root, "children")->get(*root));
  CHECK((kids.size() == 1 && kids[0] && kids[0]->nodeTypeName() == "Box"));
}

static void classicVrmlNestedTest() {
  const char *vrml =
      "#X3D V4.0 utf8\n"
      "PROTO Leaf [] { Box {} }\n"
      "PROTO Wrap [] { Transform { children Leaf {} } }\n"
      "Wrap {}\n";
  auto doc = codec::parseDocument(vrml);
  auto root = firstRoot(doc);
  CHECK((root && root->nodeTypeName() == "Transform"));
  auto kids = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
      fieldByName(*root, "children")->get(*root));
  CHECK((kids.size() == 1 && kids[0] && kids[0]->nodeTypeName() == "Box"));
}

// Depth-2: a ProtoInstance nested inside a Group inside the body's Transform must
// still route into the body (regression guard for currentProtoBody forwarding
// through readNode's recursion).
static void xmlNestedDepth2Test() {
  const char *xml =
      "<X3D version='4.0'><Scene>"
      "<ProtoDeclare name='Leaf'><ProtoBody><Box/></ProtoBody></ProtoDeclare>"
      "<ProtoDeclare name='Wrap'><ProtoBody>"
      "<Transform><Group containerField='children'>"
      "<ProtoInstance name='Leaf' containerField='children'/>"
      "</Group></Transform>"
      "</ProtoBody></ProtoDeclare>"
      "<ProtoInstance name='Wrap'/></Scene></X3D>";
  auto doc = codec::parseDocument(xml);
  auto root = firstRoot(doc);
  CHECK((root && root->nodeTypeName() == "Transform"));
  auto kids = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
      fieldByName(*root, "children")->get(*root));
  CHECK((kids.size() == 1 && kids[0] && kids[0]->nodeTypeName() == "Group"));
  auto gkids = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(
      fieldByName(*kids[0], "children")->get(*kids[0]));
  CHECK((gkids.size() == 1 && gkids[0] && gkids[0]->nodeTypeName() == "Box"));
  // The nested Leaf instance must NOT have leaked to scene.protoInstances;
  // only the top-level Wrap instance should be there (size==1).
  CHECK((doc.scene.protoInstances.size() == 1 &&
         doc.scene.protoInstances[0].name == "Wrap"));
}

TEST_CASE("proto_nested_body_test") {
  xmlNestedTest();
  classicVrmlNestedTest();
  xmlNestedDepth2Test();
  return;
}
