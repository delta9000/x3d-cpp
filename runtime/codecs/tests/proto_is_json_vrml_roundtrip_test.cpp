// ProtoBody IS connections survive JSON and ClassicVRML serialization at every depth.

#include "JsonWriter.hpp"
#include "VrmlWriter.hpp"
#include "X3DParse.hpp"

#include "doctest/doctest.h"
#include <string>

using namespace x3d;

// Count IsConnections on the first ProtoDeclaration's body mapping nodeField ->
// protoField.
static int countIs(const runtime::Scene &scene, const std::string &nodeField,
                   const std::string &protoField) {
  CHECK((!scene.protoDeclarations.empty()));
  const auto &body = scene.protoDeclarations.front()->body;
  int n = 0;
  for (const auto &ic : body.isConnections)
    if (ic.nodeField == nodeField && ic.protoField == protoField)
      ++n;
  return n;
}

TEST_CASE("proto is json vrml roundtrip: depth-1 and depth-2 IS survive") {
  // Ground-truth source: an XML PROTO with a depth-1 IS (Transform.translation
  // IS pos) and a depth-2 IS (Box.size IS boxSize inside a Shape inside the
  // Transform).
  const char *xml =
      "<X3D version='4.0'><Scene>"
      "<ProtoDeclare name='Deep'>"
      "  <ProtoInterface>"
      "    <field name='pos' type='SFVec3f' accessType='inputOutput' value='0 0 0'/>"
      "    <field name='boxSize' type='SFVec3f' accessType='inputOutput' value='2 2 2'/>"
      "  </ProtoInterface>"
      "  <ProtoBody>"
      "    <Transform>"
      "      <IS><connect nodeField='translation' protoField='pos'/></IS>"
      "      <Shape><Box><IS><connect nodeField='size' protoField='boxSize'/></IS></Box></Shape>"
      "    </Transform>"
      "  </ProtoBody>"
      "</ProtoDeclare>"
      "</Scene></X3D>";

  auto doc0 = codec::parseDocument(xml);
  CHECK((countIs(doc0.scene, "translation", "pos") == 1));
  CHECK((countIs(doc0.scene, "size", "boxSize") == 1));
  SUBCASE("JSON round trip") {
    std::string js = codec::JsonWriter().writeDocument(doc0);
    // The IS block must be emitted (matching the JsonReader's connect shape).
    CHECK((js.find("\"IS\"") != std::string::npos));
    CHECK((js.find("\"@protoField\": \"pos\"") != std::string::npos));
    CHECK((js.find("\"@protoField\": \"boxSize\"") != std::string::npos));
    auto docJ = codec::parseDocument(js, codec::Encoding::JSON);
    CHECK((countIs(docJ.scene, "translation", "pos") == 1));
    CHECK((countIs(docJ.scene, "size", "boxSize") == 1)); // deep IS survives JSON
  }
  SUBCASE("ClassicVRML round trip") {
    std::string vr = codec::VrmlWriter().writeDocument(doc0);
    // `field IS protoField` lines must be emitted inside the node bodies.
    CHECK((vr.find("translation IS pos") != std::string::npos));
    CHECK((vr.find("size IS boxSize") != std::string::npos)); // deep
    auto docV = codec::parseDocument(vr, codec::Encoding::ClassicVRML);
    CHECK((countIs(docV.scene, "translation", "pos") == 1));
    CHECK((countIs(docV.scene, "size", "boxSize") == 1)); // deep IS survives VRML
  }
}
