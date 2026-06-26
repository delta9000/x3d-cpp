// proto_appinfo_json_roundtrip_test.cpp
// PRF-4: @appinfo / @documentation on ProtoDeclare and ExternProtoDeclare must
// survive a JSON read -> write -> re-read cycle. The XmlReader/XmlWriter pair is
// the ground truth (XmlReader populates decl->appinfo/documentation from the
// attributes; XmlWriter re-emits them). Previously the JSON reader dropped both
// on ProtoDeclare and ExternProtoDeclare, and the JSON writer never emitted them
// for ExternProtoDeclare, so the metadata was lost across a JSON round trip.
//
// The oracle is a full read(XML) -> write(JSON) -> re-read(JSON) cycle: both the
// appinfo and documentation strings must reappear on the re-read declarations.
#include "JsonWriter.hpp"
#include "X3DParse.hpp"

#include "doctest/doctest.h"
#include <string>

using namespace x3d;

TEST_CASE("proto_appinfo_json_roundtrip_test") {
  // Ground-truth source: an XML ProtoDeclare and ExternProtoDeclare each
  // carrying appinfo + documentation metadata.
  const char *xml =
      "<X3D version='4.0'><Scene>"
      "<ExternProtoDeclare name='ExtWidget' url='\"widget.x3d#Widget\"'"
      "   appinfo='extern widget' documentation='http://example.com/ext'>"
      "  <field name='size' type='SFVec3f' accessType='inputOutput'/>"
      "</ExternProtoDeclare>"
      "<ProtoDeclare name='Widget'"
      "   appinfo='a reusable widget' documentation='http://example.com/widget'>"
      "  <ProtoInterface>"
      "    <field name='size' type='SFVec3f' accessType='inputOutput' value='1 1 1'/>"
      "  </ProtoInterface>"
      "  <ProtoBody>"
      "    <Transform><Shape><Box/></Shape></Transform>"
      "  </ProtoBody>"
      "</ProtoDeclare>"
      "</Scene></X3D>";

  auto doc0 = codec::parseDocument(xml);
  CHECK((!doc0.scene.protoDeclarations.empty()));
  CHECK((!doc0.scene.externProtoDeclarations.empty()));
  // Ground truth: XML reader captured the metadata.
  CHECK((doc0.scene.protoDeclarations.front()->appinfo == "a reusable widget"));
  CHECK((doc0.scene.protoDeclarations.front()->documentation ==
         "http://example.com/widget"));
  CHECK((doc0.scene.externProtoDeclarations.front()->appinfo == "extern widget"));
  CHECK((doc0.scene.externProtoDeclarations.front()->documentation ==
         "http://example.com/ext"));

  // ---- JSON round-trip ----
  std::string js = codec::JsonWriter().writeDocument(doc0);
  // The metadata members must be emitted for BOTH declaration kinds.
  CHECK((js.find("\"@appinfo\": \"a reusable widget\"") != std::string::npos));
  CHECK((js.find("\"@documentation\": \"http://example.com/widget\"") !=
         std::string::npos));
  CHECK((js.find("\"@appinfo\": \"extern widget\"") != std::string::npos));
  CHECK((js.find("\"@documentation\": \"http://example.com/ext\"") !=
         std::string::npos));

  auto docJ = codec::parseDocument(js, codec::Encoding::JSON);
  CHECK((!docJ.scene.protoDeclarations.empty()));
  CHECK((!docJ.scene.externProtoDeclarations.empty()));
  // The JSON reader must capture the metadata on re-read.
  CHECK((docJ.scene.protoDeclarations.front()->appinfo == "a reusable widget"));
  CHECK((docJ.scene.protoDeclarations.front()->documentation ==
         "http://example.com/widget"));
  CHECK((docJ.scene.externProtoDeclarations.front()->appinfo == "extern widget"));
  CHECK((docJ.scene.externProtoDeclarations.front()->documentation ==
         "http://example.com/ext"));

  return;
}
