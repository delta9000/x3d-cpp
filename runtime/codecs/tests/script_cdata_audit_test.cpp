#include "doctest/doctest.h"
// Script source is stored without CDATA wrappers and preserves whitespace.

#include "ClassicVrmlReader.hpp"
#include "DynamicField.hpp"
#include "VrmlWriter.hpp"
#include "X3DCodecs.hpp"
#include "X3DRuntime.hpp"

#include "x3d/nodes/Script.hpp"

#include <memory>
#include <string>

using namespace x3d;
using namespace x3d::nodes;

namespace {

std::shared_ptr<Script> findScript(const runtime::Scene &scene) {
  for (const auto &n : scene.rootNodes)
    if (auto s = std::dynamic_pointer_cast<Script>(n))
      return s;
  return nullptr;
}

const char *kXmlWithCdata = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0">
  <Scene>
    <Script DEF="S" url='"ecmascript: function fallback() {}"'>
      <![CDATA[
ecmascript:
function initialize() {
  Browser.print('hello');
}
function tick() {
  Browser.print('tick');
}
      ]]>
    </Script>
  </Scene>
</X3D>
)XML";

void testParseStoresNoCdataWrapper() {
  codec::XmlReader reader;
  auto doc = reader.readDocument(kXmlWithCdata);
  auto script = findScript(doc.scene);
  CHECK_MESSAGE((script != nullptr), "parse: Script node found");
  if (!script) return;

  const std::string src = script->getSourceCode();
  CHECK_MESSAGE((!src.empty()), "parse: sourceCode is non-empty");

  // The stored string must not contain CDATA brackets.
  CHECK_MESSAGE((src.find("<![CDATA[") == std::string::npos),
        "parse: sourceCode does not contain <![CDATA[");
  CHECK_MESSAGE((src.find("]]>") == std::string::npos),
        "parse: sourceCode does not contain ]]>");

  // Multi-line newlines must survive.
  CHECK_MESSAGE((src.find('\n') != std::string::npos),
        "parse: sourceCode preserves newlines");

  // url must remain intact (not overwritten by CDATA body).
  auto url = script->getUrl();
  CHECK_MESSAGE((!url.empty()), "parse: url field is not empty");
  if (!url.empty()) {
    CHECK_MESSAGE((url[0].find("ecmascript:") != std::string::npos),
          "parse: url still holds the inline ecmascript scheme");
  }
}

void testXmlRoundTripWrapsCdata() {
  codec::XmlReader reader;
  auto doc = reader.readDocument(kXmlWithCdata);
  codec::XmlWriter writer;
  std::string xml = writer.writeDocument(doc);

  CHECK_MESSAGE((xml.find("<![CDATA[") != std::string::npos),
        "xml-write: output contains <![CDATA[");
  CHECK_MESSAGE((xml.find("]]>") != std::string::npos),
        "xml-write: output contains ]]>");

  // Re-parse the XML and verify sourceCode is still clean.
  auto doc2 = reader.readDocument(xml);
  auto script = findScript(doc2.scene);
  CHECK_MESSAGE((script != nullptr), "xml-roundtrip: Script re-parsed");
  if (!script) return;
  const std::string src = script->getSourceCode();
  CHECK_MESSAGE((src.find("<![CDATA[") == std::string::npos),
        "xml-roundtrip: sourceCode has no <![CDATA[ after re-parse");
  CHECK_MESSAGE((src.find("]]>") == std::string::npos),
        "xml-roundtrip: sourceCode has no ]]> after re-parse");
  CHECK_MESSAGE((src.find("function initialize()") != std::string::npos),
        "xml-roundtrip: JS body survived");
  CHECK_MESSAGE((src.find("function tick()") != std::string::npos),
        "xml-roundtrip: multi-line JS survived");
}

void testVrmlRoundTripNoCdataLeak() {
  codec::XmlReader reader;
  auto doc = reader.readDocument(kXmlWithCdata);
  codec::VrmlWriter writer;
  std::string vrml = writer.writeDocument(doc);

  // VRML must never contain CDATA markup.
  CHECK_MESSAGE((vrml.find("<![CDATA[") == std::string::npos),
        "vrml-write: output does not contain <![CDATA[");
  CHECK_MESSAGE((vrml.find("]]>") == std::string::npos),
        "vrml-write: output does not contain ]]>");

  // Re-parse the VRML and verify sourceCode is still clean.
  codec::ClassicVrmlReader vrmlReader;
  auto doc2 = vrmlReader.readDocument(vrml);
  auto script = findScript(doc2.scene);
  CHECK_MESSAGE((script != nullptr), "vrml-roundtrip: Script re-parsed");
  if (!script) return;
  const std::string src = script->getSourceCode();
  CHECK_MESSAGE((src.find("<![CDATA[") == std::string::npos),
        "vrml-roundtrip: sourceCode has no <![CDATA[ after re-parse");
  CHECK_MESSAGE((src.find("]]>") == std::string::npos),
        "vrml-roundtrip: sourceCode has no ]]> after re-parse");
  CHECK_MESSAGE((src.find("function initialize()") != std::string::npos),
        "vrml-roundtrip: JS body survived");
  CHECK_MESSAGE((src.find("function tick()") != std::string::npos),
        "vrml-roundtrip: multi-line JS survived");
}
// Script source preserves leading and trailing whitespace across VRML -> XML.
const char *kVrmlScriptLeadingWs = R"VRML(#VRML V2.0 utf8
PROFILE Interchange

DEF S Script {
  eventIn SFFloat set_value
  eventOut SFFloat value_changed
  url "javascript:
 function set_value( value ) {
   value_changed = value * 2;
 }
"
}
)VRML";

void testScriptSourceVerbatimVrmlToXml() {
  // Decode the VRML; the inline javascript: body (with its significant leading
  // space and trailing-line indentation) is mirrored into sourceCode.
  codec::ClassicVrmlReader vrmlReader;
  auto doc = vrmlReader.readDocument(kVrmlScriptLeadingWs);
  std::shared_ptr<Script> s0;
  for (const auto &n : doc.scene.rootNodes)
    if (auto sc = std::dynamic_pointer_cast<Script>(n)) s0 = sc;
  CHECK_MESSAGE((s0 != nullptr), "vrml-verbatim: Script parsed from VRML");
  if (!s0) return;
  const std::string srcVrml = s0->getSourceCode();
  CHECK_MESSAGE((!srcVrml.empty()), "vrml-verbatim: sourceCode non-empty");
  // Inline Script whitespace is part of the source bytes.
  CHECK_MESSAGE((srcVrml.find(" function set_value") != std::string::npos ||
            srcVrml.find("\n function set_value") != std::string::npos),
        "vrml-verbatim: leading whitespace preserved on VRML decode");

  // VRML -> XML, then reparse the XML.
  codec::XmlWriter xmlWriter;
  std::string xml = xmlWriter.writeDocument(doc);

  codec::XmlReader xmlReader;
  auto doc2 = xmlReader.readDocument(xml);
  std::shared_ptr<Script> s1;
  for (const auto &n : doc2.scene.rootNodes)
    if (auto sc = std::dynamic_pointer_cast<Script>(n)) s1 = sc;
  CHECK_MESSAGE((s1 != nullptr), "vrml-verbatim: Script re-parsed from XML");
  if (!s1) return;

  // The decisive assertion: byte-identical sourceCode across the round-trip.
  CHECK_MESSAGE((s1->getSourceCode() == srcVrml),
        "vrml-verbatim: sourceCode byte-identical after vrml->xml->reparse");
}

} // namespace

TEST_CASE("script_cdata_audit_test") {
  testParseStoresNoCdataWrapper();
  testXmlRoundTripWrapsCdata();
  testVrmlRoundTripNoCdataLeak();
  testScriptSourceVerbatimVrmlToXml();

  return;
}
