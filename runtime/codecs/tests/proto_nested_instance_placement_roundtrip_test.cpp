// proto_nested_instance_placement_roundtrip_test.cpp
// PRF-3: a Case-A nested ProtoInstance — one nested UNDER a parent body node
// (not a direct ProtoBody child) — must be re-emitted by the JSON and
// ClassicVRML writers INSIDE that parent body node's child slot, mirroring the
// XmlWriter (which pushes the instance onto the parent element's children).
//
// Previously both writers APPENDED the Case-A instance as a sibling of the body
// node (a direct ProtoBody child), so on re-read it lost its parent linkage and
// degraded to Case B (parent == null). The XmlReader/XmlWriter pair is the
// ground truth; this test asserts the JSON and VRML codecs match it.
//
// Oracle: read -> write(JSON|VRML) -> re-read; the nested instance must survive
// with parent == the body node (the Group) and the same parentField slot.
#include "JsonWriter.hpp"
#include "VrmlWriter.hpp"
#include "X3DParse.hpp"

#include "doctest/doctest.h"
#include <string>

using namespace x3d;

// Locate the first ProtoDeclaration named `declName`, and within its body find
// the single nested instance of `instName`. Assert it is Case A: its parent
// resolves to a body node, and that body node is the body's first top-level
// node (the Group). Returns the recovered parentField.
static std::string checkCaseA(const runtime::Scene &scene,
                              const std::string &declName,
                              const std::string &instName) {
  const runtime::ProtoDeclaration *decl = nullptr;
  for (const auto &d : scene.protoDeclarations)
    if (d && d->name == declName)
      decl = d.get();
  CHECK((decl && "declaration must round-trip"));
  const auto &body = decl->body;
  // The body must still have exactly one top-level node (the Group); the nested
  // instance must NOT have leaked into body.nodes nor become a Case-B child.
  CHECK((body.nodes.size() == 1 && "Group body node must survive"));
  const auto &group = body.nodes.front();
  CHECK((group && "body node non-null"));

  const runtime::ProtoInstance *found = nullptr;
  for (const auto &ni : body.nestedInstances)
    if (ni.name == instName)
      found = &ni;
  CHECK((found && "nested instance must round-trip into the body"));
  auto parent = found->parent.lock();
  CHECK((parent && "Case A: nested instance must carry a parent node"));
  CHECK((parent.get() == group.get() &&
         "Case A: parent must be the Group body node, not null (Case B)"));
  return found->parentField;
}

TEST_CASE("proto_nested_instance_placement_roundtrip_test") {
  // Ground-truth source: PROTO "Outer" whose body is a single Group, and a
  // ProtoInstance of "Inner" nested UNDER that Group (containerField=children).
  // "Inner" is declared first so the instance resolves its declaration.
  const char *xml =
      "<X3D version='4.0'><Scene>"
      "<ProtoDeclare name='Inner'>"
      "  <ProtoInterface>"
      "    <field name='c' type='SFColor' accessType='inputOutput' value='1 0 0'/>"
      "  </ProtoInterface>"
      "  <ProtoBody><Shape/></ProtoBody>"
      "</ProtoDeclare>"
      "<ProtoDeclare name='Outer'>"
      "  <ProtoBody>"
      "    <Group>"
      "      <ProtoInstance name='Inner' containerField='children'>"
      "        <fieldValue name='c' value='0 1 0'/>"
      "      </ProtoInstance>"
      "    </Group>"
      "  </ProtoBody>"
      "</ProtoDeclare>"
      "</Scene></X3D>";

  auto doc0 = codec::parseDocument(xml);
  // Sanity: the XML path itself produces Case A.
  CHECK((checkCaseA(doc0.scene, "Outer", "Inner") == "children"));

  // ---- JSON round-trip ----
  {
    std::string js = codec::JsonWriter().writeDocument(doc0);
    auto docJ = codec::parseDocument(js, codec::Encoding::JSON);
    CHECK((checkCaseA(docJ.scene, "Outer", "Inner") == "children"));
  }

  // ---- ClassicVRML round-trip ----
  {
    std::string vr = codec::VrmlWriter().writeDocument(doc0);
    auto docV = codec::parseDocument(vr, codec::Encoding::ClassicVRML);
    CHECK((checkCaseA(docV.scene, "Outer", "Inner") == "children"));
  }

  return;
}
