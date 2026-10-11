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
#include "CanonicalXmlWriter.hpp"
#include "JsonWriter.hpp"
#include "XmlWriter.hpp"
#include "VrmlWriter.hpp"
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
      "    <field name='size' type='SFVec3f' accessType='inputOutput' value='1 1 1'"
      " appinfo='dimensions' documentation='size docs'/>"
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
  CHECK((doc0.scene.protoDeclarations.front()->interface.front().appinfo ==
         "dimensions"));
  CHECK((doc0.scene.protoDeclarations.front()->interface.front().documentation ==
         "size docs"));
  CHECK((doc0.scene.externProtoDeclarations.front()->appinfo == "extern widget"));
  CHECK((doc0.scene.externProtoDeclarations.front()->documentation ==
         "http://example.com/ext"));

  const std::string xmlOut = codec::XmlWriter().writeDocument(doc0);
  CHECK((xmlOut.find("appinfo=\"dimensions\"") != std::string::npos));
  auto docX = codec::parseDocument(xmlOut);
  CHECK((docX.scene.protoDeclarations.front()->interface.front().appinfo ==
         "dimensions"));
  CHECK((docX.scene.protoDeclarations.front()->interface.front().documentation ==
         "size docs"));

  // ---- JSON round-trip ----
  std::string js = codec::JsonWriter().writeDocument(doc0);
  // The metadata members must be emitted for BOTH declaration kinds.
  CHECK((js.find("\"@appinfo\": \"a reusable widget\"") != std::string::npos));
  CHECK((js.find("\"@documentation\": \"http://example.com/widget\"") !=
         std::string::npos));
  CHECK((js.find("\"@appinfo\": \"extern widget\"") != std::string::npos));
  CHECK((js.find("\"@documentation\": \"http://example.com/ext\"") !=
         std::string::npos));
  CHECK((js.find("\"@appinfo\": \"dimensions\"") != std::string::npos));

  auto docJ = codec::parseDocument(js, codec::Encoding::JSON);
  CHECK((!docJ.scene.protoDeclarations.empty()));
  CHECK((!docJ.scene.externProtoDeclarations.empty()));
  // The JSON reader must capture the metadata on re-read.
  CHECK((docJ.scene.protoDeclarations.front()->appinfo == "a reusable widget"));
  CHECK((docJ.scene.protoDeclarations.front()->documentation ==
         "http://example.com/widget"));
  CHECK((docJ.scene.protoDeclarations.front()->interface.front().appinfo ==
         "dimensions"));
  CHECK((docJ.scene.protoDeclarations.front()->interface.front().documentation ==
         "size docs"));
  CHECK((docJ.scene.externProtoDeclarations.front()->appinfo == "extern widget"));
  CHECK((docJ.scene.externProtoDeclarations.front()->documentation ==
         "http://example.com/ext"));

  return;
}

TEST_CASE("PROTO field documentation survives canonical XML and JSON hops") {
  const auto source = codec::parseDocument(R"(<X3D version='4.0'><Scene>
<ExternProtoDeclare name='External' url='"missing.x3d#External"'
 appinfo='external declaration' documentation='external docs'>
 <field name='signal' type='SFBool' accessType='inputOnly'
  appinfo='external signal' documentation='signal docs'/>
</ExternProtoDeclare>
<ProtoDeclare name='Local' appinfo='local declaration' documentation='local docs'>
 <ProtoInterface>
  <field name='label' type='SFString' accessType='inputOutput' value='label'
   appinfo='a &amp; b &quot;quoted&quot;' documentation='https://example.org/?a=1&amp;b=2'/>
  <field name='part' type='SFNode' accessType='initializeOnly'
   appinfo='node default' documentation='part docs'><Group/></field>
 </ProtoInterface>
 <ProtoBody><Group/></ProtoBody>
</ProtoDeclare></Scene></X3D>)");
  auto check = [](const runtime::X3DDocument &doc) {
    REQUIRE(doc.scene.protoDeclarations.size() == 1);
    REQUIRE(doc.scene.externProtoDeclarations.size() == 1);
    const auto &local = *doc.scene.protoDeclarations.front();
    const auto &external = *doc.scene.externProtoDeclarations.front();
    CHECK(local.appinfo == "local declaration");
    CHECK(local.documentation == "local docs");
    CHECK(external.appinfo == "external declaration");
    CHECK(external.documentation == "external docs");
    REQUIRE(local.interface.size() == 2);
    CHECK(local.interface[0].appinfo == "a & b \"quoted\"");
    CHECK(local.interface[0].documentation == "https://example.org/?a=1&b=2");
    CHECK(local.interface[1].appinfo == "node default");
    CHECK(local.interface[1].documentation == "part docs");
    REQUIRE(external.interface.size() == 1);
    CHECK(external.interface[0].appinfo == "external signal");
    CHECK(external.interface[0].documentation == "signal docs");
  };
  check(source);
  const auto fromJson = codec::parseDocument(codec::JsonWriter().writeDocument(source),
                                              codec::Encoding::JSON);
  for (const auto &doc : {source, fromJson}) {
    const auto canonical = codec::CanonicalXmlWriter().writeDocument(doc);
    auto again = codec::parseDocument(canonical);
    check(again);
    CHECK(codec::CanonicalXmlWriter().writeDocument(again) == canonical);
    again = codec::parseDocument(codec::JsonWriter().writeDocument(again),
                                codec::Encoding::JSON);
    again = codec::parseDocument(codec::XmlWriter().writeDocument(again));
    check(again);
  }
}

TEST_CASE("PROTO metadata survives ClassicVRML comments") {
  auto source = codec::parseDocument(R"(<X3D version='4.0'><Scene>
<ExternProtoDeclare name='External' url='"missing.x3d#External"'
 appinfo='external' documentation='external docs'>
 <field name='signal' type='SFBool' accessType='inputOnly'
  appinfo='signal' documentation='signal docs'/>
</ExternProtoDeclare>
<ProtoDeclare name='Local' appinfo='local' documentation='local docs'>
 <ProtoInterface>
  <field name='value' type='SFString' accessType='inputOutput' value='kept'
   appinfo='value' documentation='value docs'/>
  <field name='plain' type='SFBool' accessType='inputOnly'/>
 </ProtoInterface>
 <ProtoBody><Group/>
  <ProtoDeclare name='Nested' appinfo='nested' documentation='nested docs'>
   <ProtoInterface><field name='part' type='SFNode' accessType='initializeOnly'
    appinfo='part' documentation='part docs'><Group/></field></ProtoInterface>
   <ProtoBody><Group/></ProtoBody>
  </ProtoDeclare>
 </ProtoBody>
</ProtoDeclare></Scene></X3D>)");
  const std::string tricky = "Unicode café, \"quotes\", \\ slash\n# comment\r\t";
  source.scene.protoDeclarations.front()->appinfo = tricky;
  auto check = [&](const runtime::X3DDocument &doc) {
    REQUIRE(doc.scene.protoDeclarations.size() == 1);
    REQUIRE(doc.scene.externProtoDeclarations.size() == 1);
    const auto &local = *doc.scene.protoDeclarations.front();
    const auto &external = *doc.scene.externProtoDeclarations.front();
    CHECK(local.appinfo == tricky);
    CHECK(local.documentation == "local docs");
    REQUIRE(local.interface.size() == 2);
    CHECK(local.interface[0].appinfo == "value");
    CHECK(local.interface[0].documentation == "value docs");
    CHECK(std::any_cast<core::SFString>(local.interface[0].value) == "kept");
    CHECK(local.interface[1].appinfo.empty());
    CHECK(local.interface[1].documentation.empty());
    REQUIRE(local.body.statements.size() == 2);
    const auto nested = local.body.statements[1].proto;
    REQUIRE(nested);
    CHECK(nested->appinfo == "nested");
    CHECK(nested->documentation == "nested docs");
    REQUIRE(nested->interface.size() == 1);
    CHECK(nested->interface[0].appinfo == "part");
    CHECK(nested->interface[0].documentation == "part docs");
    CHECK(external.appinfo == "external");
    CHECK(external.documentation == "external docs");
    REQUIRE(external.interface.size() == 1);
    CHECK(external.interface[0].appinfo == "signal");
    CHECK(external.interface[0].documentation == "signal docs");
  };
  const auto classic = codec::VrmlWriter().writeDocument(source);
  auto again = codec::parseDocument(classic, codec::Encoding::ClassicVRML);
  check(again);
  CHECK(codec::VrmlWriter().writeDocument(again) == classic);
  again = codec::parseDocument(codec::JsonWriter().writeDocument(again), codec::Encoding::JSON);
  again = codec::parseDocument(codec::CanonicalXmlWriter().writeDocument(again));
  check(again);
}

TEST_CASE("Classic prototype metadata comments attach only to their next token") {
  auto doc = codec::parseDocument(R"(#X3D V4.0 utf8
# An ordinary comment mentioning appinfo is just a comment.
PROTO Plain [] { Group {} }
# x3d-cpp:proto-metadata-v1 ["first", "old"]
# Ordinary trivia may separate metadata and the declaration.
# x3d-cpp:proto-metadata-v1 ["last", ""]
PROTO Annotated [] { Group {} }
# x3d-cpp:proto-metadata-v1 [false, "wrong type"]
PROTO WrongType [] { Group {} }
# x3d-cpp:proto-metadata-v1 not JSON
PROTO Malformed [] { Group {} }
# x3d-cpp:proto-metadata-v2 ["unknown version", "ignored"]
PROTO Future [] { Group {} }
# x3d-cpp:proto-metadata-v1 ["orphan", "ignored"]
Group {}
PROTO AfterNode [] { Group {} }
# x3d-cpp:proto-metadata-v1 ["trailing", "ignored"] garbage
PROTO Trailing [] { Group {} }
# x3d-cpp:proto-metadata-v1 ["", "docs only"]
EXTERNPROTO External [
# x3d-cpp:proto-metadata-v1 ["", "event docs"]
inputOnly SFBool signal
inputOnly SFBool plain
] ["missing.x3d#External"]
)", codec::Encoding::ClassicVRML);
  REQUIRE(doc.scene.protoDeclarations.size() == 7);
  CHECK(doc.scene.protoDeclarations[1]->appinfo == "last");
  for (std::size_t i : {0u, 2u, 3u, 4u, 5u, 6u}) {
    CHECK(doc.scene.protoDeclarations[i]->appinfo.empty());
    CHECK(doc.scene.protoDeclarations[i]->documentation.empty());
  }
  REQUIRE(doc.scene.rootNodes.size() == 1);
  REQUIRE(doc.scene.externProtoDeclarations.size() == 1);
  const auto &external = *doc.scene.externProtoDeclarations.front();
  CHECK(external.appinfo.empty());
  CHECK(external.documentation == "docs only");
  REQUIRE(external.interface.size() == 2);
  CHECK(external.interface[0].appinfo.empty());
  CHECK(external.interface[0].documentation == "event docs");
  CHECK(external.interface[1].appinfo.empty());
  CHECK(external.interface[1].documentation.empty());
}
