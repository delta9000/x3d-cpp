// EXTERNPROTO URL candidates survive JSON round-trips in their original order.
#include "JsonWriter.hpp"
#include "X3DParse.hpp"

#include "doctest/doctest.h"
#include <string>

using namespace x3d;

TEST_CASE("proto_extern_url_json_roundtrip_test") {
  // Ground-truth source: an XML ExternProtoDeclare with a multi-element url.
  const char *xml =
      "<X3D version='4.0'><Scene>"
      "<ExternProtoDeclare name='ExtWidget'"
      "   url='\"widget.x3d#Widget\" \"http://example.com/widget.x3d#Widget\"'>"
      "  <field name='size' type='SFVec3f' accessType='inputOutput'/>"
      "</ExternProtoDeclare>"
      "</Scene></X3D>";

  auto doc0 = codec::parseDocument(xml);
  CHECK((!doc0.scene.externProtoDeclarations.empty()));
  // Ground truth: XML reader captured both url elements.
  const auto &url0 = doc0.scene.externProtoDeclarations.front()->url;
  CHECK((url0.size() == 2));
  CHECK((url0[0] == "widget.x3d#Widget"));
  CHECK((url0[1] == "http://example.com/widget.x3d#Widget"));
  std::string js = codec::JsonWriter().writeDocument(doc0);
  // The writer emits @url as a JSON array.
  CHECK((js.find("\"@url\": [") != std::string::npos));

  auto docJ = codec::parseDocument(js, codec::Encoding::JSON);
  CHECK((!docJ.scene.externProtoDeclarations.empty()));
  // The JSON reader must capture both url elements on re-read (array form).
  const auto &urlJ = docJ.scene.externProtoDeclarations.front()->url;
  CHECK((urlJ.size() == 2));
  CHECK((urlJ[0] == "widget.x3d#Widget"));
  CHECK((urlJ[1] == "http://example.com/widget.x3d#Widget"));

  return;
}
