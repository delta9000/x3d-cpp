#include "JsonWriter.hpp"
#include "VrmlWriter.hpp"
#include "XmlWriter.hpp"
#include "X3DParse.hpp"

#include "doctest/doctest.h"

using namespace x3d;

TEST_CASE("proto_sfnode_default_roundtrip_test") {
  const char *xml =
      "<X3D version='4.0'><Scene><ProtoDeclare name='P'>"
      "<ProtoInterface><field name='appearance' type='SFNode' accessType='initializeOnly'>"
      "<Appearance><Material/></Appearance></field></ProtoInterface>"
      "<ProtoBody><Shape/></ProtoBody></ProtoDeclare></Scene></X3D>";
  auto source = codec::parseDocument(xml);
  const auto checkDefault = [](const runtime::X3DDocument &doc) {
    REQUIRE((!doc.scene.protoDeclarations.empty()));
    const auto &fields = doc.scene.protoDeclarations.front()->interface;
    REQUIRE((!fields.empty()));
    REQUIRE((fields.front().nodeDefault.size() == 1));
    CHECK((fields.front().nodeDefault.front()->nodeTypeName() == "Appearance"));
  };
  checkDefault(source);

  const auto xmlOut = codec::XmlWriter().writeDocument(source);
  CHECK((xmlOut.find("<Appearance") != std::string::npos));
  checkDefault(codec::parseDocument(xmlOut));

  const auto jsonOut = codec::JsonWriter().writeDocument(source);
  CHECK((jsonOut.find("\"-children\"") != std::string::npos));
  checkDefault(codec::parseDocument(jsonOut, codec::Encoding::JSON));

  const auto vrmlOut = codec::VrmlWriter().writeDocument(source);
  CHECK((vrmlOut.find("SFNode appearance Appearance") != std::string::npos));
  CHECK((vrmlOut.find("SFNode appearance [") == std::string::npos));
  checkDefault(codec::parseDocument(vrmlOut, codec::Encoding::ClassicVRML));
}
