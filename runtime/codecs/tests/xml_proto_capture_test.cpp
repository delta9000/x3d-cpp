// xml_proto_capture_test.cpp
// Verifies the XML reader CAPTURES the PROTO element family (ProtoDeclare,
// ProtoInterface/field, ProtoBody, IS/connect, ProtoInstance/fieldValue) into
// the Scene data model, rather than skipping them as unknown elements.
#include "XmlReader.hpp"

#include "doctest/doctest.h"
#include <cstdio>

using namespace x3d;
using namespace x3d::core;

TEST_CASE("xml_proto_capture_test") {
  const char *xml =
      "<X3D profile='Interchange' version='4.0'><Scene>"
      "<ProtoDeclare name='Param'>"
      "  <ProtoInterface>"
      "    <field name='size' type='SFVec3f' accessType='initializeOnly' value='2 2 2'/>"
      "  </ProtoInterface>"
      "  <ProtoBody>"
      "    <Box><IS><connect nodeField='size' protoField='size'/></IS></Box>"
      "  </ProtoBody>"
      "</ProtoDeclare>"
      "<Transform><ProtoInstance name='Param' containerField='children'>"
      "  <fieldValue name='size' value='5 5 5'/>"
      "</ProtoInstance></Transform>"
      "</Scene></X3D>";
  codec::XmlReader reader;
  auto doc = reader.readDocument(xml);
  auto &scene = doc.getScene();

  CHECK((scene.protoDeclarations.size() == 1));
  auto &decl = *scene.protoDeclarations[0];
  CHECK((decl.name == "Param"));
  CHECK((decl.interface.size() == 1 && decl.interface[0].name == "size"));
  CHECK((decl.interface[0].type == X3DFieldType::SFVec3f));
  CHECK((decl.interface[0].access == AccessType::InitializeOnly));
  CHECK((decl.interface[0].value.has_value()));
  CHECK((decl.body.nodes.size() == 1 &&
         decl.body.nodes[0]->nodeTypeName() == "Box"));
  CHECK((decl.body.isConnections.size() == 1 &&
         decl.body.isConnections[0].nodeField == "size" &&
         decl.body.isConnections[0].protoField == "size"));
  CHECK((decl.body.isConnections[0].node == decl.body.nodes[0]));

  CHECK((scene.protoInstances.size() == 1));
  auto &inst = scene.protoInstances[0];
  CHECK((inst.name == "Param" && inst.fieldValues.size() == 1 &&
         inst.fieldValues[0].name == "size"));
  CHECK((inst.fieldValues[0].value.has_value()));
  CHECK((!inst.parent.expired() &&
         inst.parent.lock()->nodeTypeName() == "Transform"));
  CHECK((inst.parentField == "children"));

  std::puts("xml_proto_capture_test OK");
  return;
}

TEST_CASE("xml_proto_capture_nested_instance_is_connect_test") {
  const char *xml =
      "<X3D profile='Interchange' version='4.0'><Scene>"
      "<ProtoDeclare name='Inner'>"
      "  <ProtoInterface>"
      "    <field name='myShape' type='MFNode' accessType='inputOutput'>"
      "      <Sphere/>"
      "    </field>"
      "  </ProtoInterface>"
      "  <ProtoBody><Transform><IS><connect nodeField='children' protoField='myShape'/></IS></Transform></ProtoBody>"
      "</ProtoDeclare>"
      "<ProtoDeclare name='Outer'>"
      "  <ProtoInterface>"
      "    <field name='myShape' type='MFNode' accessType='inputOutput'>"
      "      <Cylinder/>"
      "    </field>"
      "  </ProtoInterface>"
      "  <ProtoBody>"
      "    <Transform>"
      "      <ProtoInstance name='Inner'>"
      "        <IS><connect nodeField='myShape' protoField='myShape'/></IS>"
      "      </ProtoInstance>"
      "    </Transform>"
      "  </ProtoBody>"
      "</ProtoDeclare>"
      "</Scene></X3D>";
  codec::XmlReader reader;
  auto doc = reader.readDocument(xml);
  auto &scene = doc.getScene();

  CHECK((scene.protoDeclarations.size() == 2));
  auto &outer = *scene.protoDeclarations[1];
  CHECK((outer.name == "Outer"));
  CHECK((outer.body.nestedInstances.size() == 1));
  const auto &nested = outer.body.nestedInstances[0];
  CHECK((nested.name == "Inner"));
  CHECK((nested.isConnections.size() == 1));
  CHECK((nested.isConnections[0].nodeField == "myShape"));
  CHECK((nested.isConnections[0].protoField == "myShape"));
}
