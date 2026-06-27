#include "doctest/doctest.h"
// script_cdata_audit_test.cpp
// AUD-SCR-CDATA regression: verifies that Script node CDATA content
//   1) parses WITHOUT the <![CDATA[ wrapper in stored sourceCode
//   2) XML round-trip re-wraps it in CDATA
//   3) VRML round-trip does NOT leak CDATA markup
//   4) multi-line JS code survives with newlines intact
//   5) the url field coexists with sourceCode and is not confused
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "ClassicVrmlReader.hpp"
#include "DynamicField.hpp"
#include "VrmlWriter.hpp"
#include "X3DCodecs.hpp"
#include "X3DRuntime.hpp"

#include "x3d/nodes/Script.hpp"

#include <iostream>
#include <memory>
#include <string>

using namespace x3d;
using namespace x3d::nodes;

namespace {

int failures = 0;

void check(bool cond, const std::string &what) {
  if (!cond) {
    std::cerr << "FAIL: " << what << "\n";
    ++failures;
  } else {
    std::cout << "ok: " << what << "\n";
  }
}

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
  runtime::dynamicFieldStore().clear();
  codec::XmlReader reader;
  auto doc = reader.readDocument(kXmlWithCdata);
  auto script = findScript(doc.scene);
  check(script != nullptr, "parse: Script node found");
  if (!script) return;

  const std::string src = script->getSourceCode();
  check(!src.empty(), "parse: sourceCode is non-empty");

  // The stored string must NOT contain CDATA brackets.
  check(src.find("<![CDATA[") == std::string::npos,
        "parse: sourceCode does not contain <![CDATA[");
  check(src.find("]]>") == std::string::npos,
        "parse: sourceCode does not contain ]]>");

  // Multi-line newlines must survive.
  check(src.find('\n') != std::string::npos,
        "parse: sourceCode preserves newlines");

  // url must remain intact (not overwritten by CDATA body).
  auto url = script->getUrl();
  check(!url.empty(), "parse: url field is not empty");
  if (!url.empty()) {
    check(url[0].find("ecmascript:") != std::string::npos,
          "parse: url still holds the inline ecmascript scheme");
  }
}

void testXmlRoundTripWrapsCdata() {
  runtime::dynamicFieldStore().clear();
  codec::XmlReader reader;
  auto doc = reader.readDocument(kXmlWithCdata);
  codec::XmlWriter writer;
  std::string xml = writer.writeDocument(doc);

  check(xml.find("<![CDATA[") != std::string::npos,
        "xml-write: output contains <![CDATA[");
  check(xml.find("]]>") != std::string::npos,
        "xml-write: output contains ]]>");

  // Re-parse the XML and verify sourceCode is still clean.
  runtime::dynamicFieldStore().clear();
  auto doc2 = reader.readDocument(xml);
  auto script = findScript(doc2.scene);
  check(script != nullptr, "xml-roundtrip: Script re-parsed");
  if (!script) return;
  const std::string src = script->getSourceCode();
  check(src.find("<![CDATA[") == std::string::npos,
        "xml-roundtrip: sourceCode has no <![CDATA[ after re-parse");
  check(src.find("]]>") == std::string::npos,
        "xml-roundtrip: sourceCode has no ]]> after re-parse");
  check(src.find("function initialize()") != std::string::npos,
        "xml-roundtrip: JS body survived");
  check(src.find("function tick()") != std::string::npos,
        "xml-roundtrip: multi-line JS survived");
}

void testVrmlRoundTripNoCdataLeak() {
  runtime::dynamicFieldStore().clear();
  codec::XmlReader reader;
  auto doc = reader.readDocument(kXmlWithCdata);
  codec::VrmlWriter writer;
  std::string vrml = writer.writeDocument(doc);

  // VRML must never contain CDATA markup.
  check(vrml.find("<![CDATA[") == std::string::npos,
        "vrml-write: output does not contain <![CDATA[");
  check(vrml.find("]]>") == std::string::npos,
        "vrml-write: output does not contain ]]>");

  // Re-parse the VRML and verify sourceCode is still clean.
  runtime::dynamicFieldStore().clear();
  codec::ClassicVrmlReader vrmlReader;
  auto doc2 = vrmlReader.readDocument(vrml);
  auto script = findScript(doc2.scene);
  check(script != nullptr, "vrml-roundtrip: Script re-parsed");
  if (!script) return;
  const std::string src = script->getSourceCode();
  check(src.find("<![CDATA[") == std::string::npos,
        "vrml-roundtrip: sourceCode has no <![CDATA[ after re-parse");
  check(src.find("]]>") == std::string::npos,
        "vrml-roundtrip: sourceCode has no ]]> after re-parse");
  check(src.find("function initialize()") != std::string::npos,
        "vrml-roundtrip: JS body survived");
  check(src.find("function tick()") != std::string::npos,
        "vrml-roundtrip: multi-line JS survived");
}

// ---------------------------------------------------------------------------
// (6) Script.sourceCode must survive vrml -> xml -> reparse BYTE-IDENTICALLY.
//
// Regression for BUG-2: XmlReader::readScript() used to trim() the CDATA body
// before setSourceCode(), so the XML reader was the lone asymmetric, lossy site
// in the codec set. The ClassicVRML decode keeps leading/trailing whitespace
// (it is genuine author bytes — e.g. the space after a scheme prefix, or the
// trailing-line indentation inside a `javascript:{ ... }` block), the XML and
// JSON writers emit sourceCode verbatim, and the JSON reader rejoins verbatim.
// Trimming on the XML read destroyed those bytes, surfacing in the cli-gate as
//   Script.sourceCode: SFString mismatch
// on real corpus files (latlong.wrl, WorldHigh.wrl, WorldTest.wrl, ...). The fix
// stores el.text verbatim; this asserts the round-trip is now lossless.
// ---------------------------------------------------------------------------
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
  runtime::dynamicFieldStore().clear();
  codec::ClassicVrmlReader vrmlReader;
  auto doc = vrmlReader.readDocument(kVrmlScriptLeadingWs);
  std::shared_ptr<Script> s0;
  for (const auto &n : doc.scene.rootNodes)
    if (auto sc = std::dynamic_pointer_cast<Script>(n)) s0 = sc;
  check(s0 != nullptr, "vrml-verbatim: Script parsed from VRML");
  if (!s0) return;
  const std::string srcVrml = s0->getSourceCode();
  check(!srcVrml.empty(), "vrml-verbatim: sourceCode non-empty");
  // The leading whitespace inside the body must be present (it is what the
  // XML reader used to destroy).
  check(srcVrml.find(" function set_value") != std::string::npos ||
            srcVrml.find("\n function set_value") != std::string::npos,
        "vrml-verbatim: leading whitespace preserved on VRML decode");

  // VRML -> XML, then reparse the XML.
  codec::XmlWriter xmlWriter;
  std::string xml = xmlWriter.writeDocument(doc);

  runtime::dynamicFieldStore().clear();
  codec::XmlReader xmlReader;
  auto doc2 = xmlReader.readDocument(xml);
  std::shared_ptr<Script> s1;
  for (const auto &n : doc2.scene.rootNodes)
    if (auto sc = std::dynamic_pointer_cast<Script>(n)) s1 = sc;
  check(s1 != nullptr, "vrml-verbatim: Script re-parsed from XML");
  if (!s1) return;

  // The decisive assertion: byte-identical sourceCode across the round-trip.
  check(s1->getSourceCode() == srcVrml,
        "vrml-verbatim: sourceCode byte-identical after vrml->xml->reparse");
}

} // namespace

TEST_CASE("script_cdata_audit_test") {
  testParseStoresNoCdataWrapper();
  testXmlRoundTripWrapsCdata();
  testVrmlRoundTripNoCdataLeak();
  testScriptSourceVerbatimVrmlToXml();
  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    CHECK(false);
    return;
  }
  std::cout << "All Script CDATA audit checks passed.\n";
  return;
}
