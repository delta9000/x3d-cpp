#include "VrmlWriter.hpp"
#include "X3DParse.hpp"

#include "doctest/doctest.h"

using namespace x3d;

TEST_CASE("proto_author_field_is_roundtrip_test") {
  const char *xml =
      "<X3D version='4.0'><Scene><ProtoDeclare name='P'>"
      "<ProtoInterface><field name='canopyOpen' type='SFBool' "
      "accessType='initializeOnly'/></ProtoInterface>"
      "<ProtoBody><Script><field name='traceEnabled' type='SFBool' "
      "accessType='initializeOnly'/><IS><connect nodeField='traceEnabled' "
      "protoField='canopyOpen'/></IS></Script></ProtoBody>"
      "</ProtoDeclare></Scene></X3D>";

  auto source = codec::parseDocument(xml);
  REQUIRE((source.scene.protoDeclarations.size() == 1));
  REQUIRE((source.scene.protoDeclarations.front()->body.isConnections.size() == 1));

  const std::string vrml = codec::VrmlWriter().writeDocument(source);
  CHECK((vrml.find("initializeOnly SFBool traceEnabled FALSE") !=
         std::string::npos));
  CHECK((vrml.find("initializeOnly SFBool canopyOpen FALSE") !=
         std::string::npos));

  auto reparsed = codec::parseDocument(vrml, codec::Encoding::ClassicVRML);
  REQUIRE((reparsed.scene.protoDeclarations.size() == 1));
  const auto &connections =
      reparsed.scene.protoDeclarations.front()->body.isConnections;
  REQUIRE((connections.size() == 1));
  CHECK((connections.front().nodeField == "traceEnabled"));
  CHECK((connections.front().protoField == "canopyOpen"));
}
