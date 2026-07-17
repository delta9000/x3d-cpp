#include "doctest/doctest.h"
// codec_string_hardening_test.cpp
// Regressions for the ENC-* string/value-handling findings
// (docs/conformance/findings.yaml, encoding review + round-trip sweep):
//
//   ENC-VRML-STRING   — VrmlWriter must escape '"' and '\' in META (an embedded
//                       quote truncates the value and swallows following lines).
//   ENC-MFSTRING-READ — MFString/VRML-string readers must keep the backslash on
//                       escapes other than \" and \\ (c:\new\tex -> c:newex).
//   ENC-VRML-SFIMAGE  — the ClassicVRML reader must consume width*height pixel
//                       words after the `w h comp` header (a 2x2 texture parsed
//                       back as `2 0 0`).
//   ENC-CDATA-SCRIPT  — XmlWriter must split a literal ]]> in Script source
//                       into consecutive CDATA sections instead of truncating.
//   ENC-JSON-UNIT     — JsonWriter must emit the head `unit` array (JsonReader
//                       already reads it; the asymmetry silently dropped UNIT).
//   ENC-JSON-CTRL     — JsonWriter must escape control chars (U+0000..001F) as
//                       \u00XX per RFC 8259 instead of emitting raw bytes.
//   ENC-C14N-ATTR     — canonical attribute values must escape tab/newline/CR
//                       as &#x9;/&#xA;/&#xD; (raw whitespace is normalized to a
//                       space by any conformant reader: non-portable canon).
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "ClassicVrmlReader.hpp"
#include "DynamicField.hpp"
#include "JsonReader.hpp"
#include "VrmlWriter.hpp"
#include "X3DCodecs.hpp"
#include "X3DRuntime.hpp"

#include "x3d/nodes/PixelTexture.hpp"
#include "x3d/nodes/Script.hpp"
#include "x3d/nodes/WorldInfo.hpp"

#include <functional>
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

bool contains(const std::string &hay, const std::string &needle) {
  return hay.find(needle) != std::string::npos;
}

template <typename T>
std::shared_ptr<T> findNode(const runtime::Scene &scene) {
  for (const auto &n : scene.rootNodes)
    if (auto t = std::dynamic_pointer_cast<T>(n))
      return t;
  return nullptr;
}

// ---------------------------------------------------------------------------
// ENC-VRML-STRING: META + string fields with embedded quote/backslash survive
// the XML -> ClassicVRML -> reparse hop.
// ---------------------------------------------------------------------------
void testVrmlStringEscaping() {
  const char *kXml = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0">
  <head>
    <meta name='description' content='adding a &quot;red&quot; ball via c:\tex'/>
  </head>
  <Scene>
    <WorldInfo title='say &quot;hi&quot;' info='"first \&quot;quoted\&quot; item" "second item"'/>
  </Scene>
</X3D>
)XML";
  codec::XmlReader xml;
  auto doc = xml.readDocument(kXml);
  check(doc.head.meta.size() == 1 &&
            doc.head.meta[0].content == "adding a \"red\" ball via c:\\tex",
        "ENC-VRML-STRING: XML parse keeps quote+backslash in meta content");
  {
    auto wi0 = findNode<WorldInfo>(doc.scene);
    check(wi0 && wi0->getInfo().size() == 2,
          "ENC-VRML-STRING: fixture sanity — XML parse yields 2 MFString "
          "elements");
  }

  codec::VrmlWriter vw;
  std::string vrml = vw.writeDocument(doc);
  codec::ClassicVrmlReader cv;
  auto doc2 = cv.readDocument(vrml);

  check(doc2.head.meta.size() == 1,
        "ENC-VRML-STRING: META statement count survives the VRML hop");
  if (doc2.head.meta.size() == 1) {
    check(doc2.head.meta[0].name == "description",
          "ENC-VRML-STRING: META name survives");
    check(doc2.head.meta[0].content == doc.head.meta[0].content,
          "ENC-VRML-STRING: META content with embedded quote survives (got '" +
              doc2.head.meta[0].content + "')");
  }
  auto wi = findNode<WorldInfo>(doc2.scene);
  check(wi != nullptr, "ENC-VRML-STRING: WorldInfo survives the VRML hop");
  if (wi) {
    check(wi->getTitle() == "say \"hi\"",
          "ENC-VRML-STRING: SFString title with quote survives (got '" +
              wi->getTitle() + "')");
    const auto &info = wi->getInfo();
    check(info.size() == 2,
          "ENC-VRML-STRING: MFString element count survives (got " +
              std::to_string(info.size()) + ")");
    if (info.size() == 2) {
      check(info[0] == "first \"quoted\" item",
            "ENC-VRML-STRING: MFString element with quote survives (got '" +
                info[0] + "')");
      check(info[1] == "second item",
            "ENC-VRML-STRING: following MFString element intact");
    }
  }
}

// ---------------------------------------------------------------------------
// ENC-MFSTRING-READ: an unknown escape keeps its backslash; the two defined
// escapes (\" and \\) still unescape. Exercised through the XML attribute
// MFString path (FieldValueIO::parseMFString).
// ---------------------------------------------------------------------------
void testMfStringUnknownEscape() {
  const char *kXml = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0"><Scene>
  <WorldInfo info='"c:\new\tex" "a\"b" "a\\b"'/>
</Scene></X3D>
)XML";
  codec::XmlReader xml;
  auto doc = xml.readDocument(kXml);
  auto wi = findNode<WorldInfo>(doc.scene);
  check(wi != nullptr, "ENC-MFSTRING-READ: WorldInfo parsed");
  if (!wi)
    return;
  const auto &info = wi->getInfo();
  check(info.size() == 3, "ENC-MFSTRING-READ: 3 MFString elements");
  if (info.size() == 3) {
    check(info[0] == "c:\\new\\tex",
          "ENC-MFSTRING-READ: unknown escape keeps backslash (got '" +
              info[0] + "')");
    check(info[1] == "a\"b", "ENC-MFSTRING-READ: \\\" still unescapes");
    check(info[2] == "a\\b", "ENC-MFSTRING-READ: \\\\ still unescapes");
  }

  // Same rule on the ClassicVRML tokenizer path.
  const char *kVrml = "#X3D V4.0 utf8\n"
                      "PROFILE Interchange\n"
                      "WorldInfo { title \"c:\\new\\tex\" }\n";
  codec::ClassicVrmlReader cv;
  auto doc2 = cv.readDocument(kVrml);
  auto wi2 = findNode<WorldInfo>(doc2.scene);
  check(wi2 != nullptr, "ENC-MFSTRING-READ: VRML WorldInfo parsed");
  if (wi2)
    check(wi2->getTitle() == "c:\\new\\tex",
          "ENC-MFSTRING-READ: VRML tokenizer keeps backslash on unknown "
          "escape (got '" +
              wi2->getTitle() + "')");
}

// ---------------------------------------------------------------------------
// ENC-VRML-SFIMAGE: an inline PixelTexture survives the ClassicVRML hop.
// ---------------------------------------------------------------------------
void testVrmlSfImageRead() {
  const char *kXml = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0"><Scene>
  <Shape>
    <Appearance>
      <PixelTexture image='2 2 3 0 65280 255 16711680'/>
    </Appearance>
  </Shape>
</Scene></X3D>
)XML";
  codec::XmlReader xml;
  auto doc = xml.readDocument(kXml);
  // The texture is nested (Shape > Appearance > PixelTexture): dig for it
  // through the reflection tables.
  std::shared_ptr<PixelTexture> orig;
  std::function<std::shared_ptr<PixelTexture>(
      const std::shared_ptr<X3DNode> &)>
      dig = [&](const std::shared_ptr<X3DNode> &n)
      -> std::shared_ptr<PixelTexture> {
    if (!n)
      return nullptr;
    if (auto pt = std::dynamic_pointer_cast<PixelTexture>(n))
      return pt;
    for (const auto &f : n->fields()) {
      if (!f.isNode() || !f.isReadable() || !f.get)
        continue;
      std::any v = f.get(*n);
      if (auto *sf = std::any_cast<core::SFNode>(&v)) {
        if (auto r = dig(*sf))
          return r;
      } else if (auto *mf = std::any_cast<core::MFNode>(&v)) {
        for (const auto &c : *mf)
          if (auto r = dig(c))
            return r;
      }
    }
    return nullptr;
  };
  for (const auto &n : doc.scene.rootNodes)
    if ((orig = dig(n)))
      break;
  check(orig != nullptr, "ENC-VRML-SFIMAGE: PixelTexture parsed from XML");
  if (!orig)
    return;
  const core::SFImage &img = orig->getImage();
  check(img.width == 2 && img.height == 2 && img.numComponents == 3 &&
            img.data.size() == 12,
        "ENC-VRML-SFIMAGE: XML parse yields a full 2x2x3 image");

  codec::VrmlWriter vw;
  std::string vrml = vw.writeDocument(doc);
  codec::ClassicVrmlReader cv;
  auto doc2 = cv.readDocument(vrml);
  std::shared_ptr<PixelTexture> re;
  for (const auto &n : doc2.scene.rootNodes)
    if ((re = dig(n)))
      break;
  check(re != nullptr, "ENC-VRML-SFIMAGE: PixelTexture survives the VRML hop");
  if (re)
    check(re->getImage() == img,
          "ENC-VRML-SFIMAGE: SFImage identical after the VRML hop (got " +
              std::to_string(re->getImage().width) + " " +
              std::to_string(re->getImage().height) + " " +
              std::to_string(re->getImage().numComponents) + ", " +
              std::to_string(re->getImage().data.size()) + " bytes)");
}

// ---------------------------------------------------------------------------
// ENC-CDATA-SCRIPT: Script source containing a literal ]]> survives the XML
// writer (split CDATA) and reparses intact (XmlLite concatenates consecutive
// CDATA sections).
// ---------------------------------------------------------------------------
void testCdataSplit() {
  const char *kXml = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0"><Scene>
  <Script DEF="S"><![CDATA[ecmascript: function initialize() {}]]></Script>
</Scene></X3D>
)XML";
  runtime::dynamicFieldStore().clear();
  codec::XmlReader xml;
  auto doc = xml.readDocument(kXml);
  auto script = findNode<Script>(doc.scene);
  check(script != nullptr, "ENC-CDATA-SCRIPT: Script parsed");
  if (!script)
    return;
  const std::string source = "ecmascript: if (a]]>b) { mask(); }";
  script->setSourceCode(source);

  codec::XmlWriter xw;
  std::string out = xw.writeDocument(doc);
  check(!contains(out, "if (a]]>b)"),
        "ENC-CDATA-SCRIPT: writer does not emit a raw ]]> inside CDATA");

  auto doc2 = xml.readDocument(out);
  auto script2 = findNode<Script>(doc2.scene);
  check(script2 != nullptr, "ENC-CDATA-SCRIPT: Script survives round-trip");
  if (script2)
    check(contains(script2->getSourceCode(), "if (a]]>b) { mask(); }"),
          "ENC-CDATA-SCRIPT: ]]> in source survives round-trip (got '" +
              script2->getSourceCode() + "')");
}

// ---------------------------------------------------------------------------
// ENC-JSON-UNIT: <unit> statements survive the JSON hop.
// ---------------------------------------------------------------------------
void testJsonUnit() {
  const char *kXml = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0">
  <head>
    <unit category='angle' name='degrees' conversionFactor='0.0174533'/>
  </head>
  <Scene><WorldInfo title='u'/></Scene>
</X3D>
)XML";
  codec::XmlReader xml;
  auto doc = xml.readDocument(kXml);
  check(doc.head.units.size() == 1, "ENC-JSON-UNIT: XML parse reads the unit");

  codec::JsonWriter jw;
  std::string json = jw.writeDocument(doc);
  check(contains(json, "\"unit\""),
        "ENC-JSON-UNIT: JSON output has a unit array");

  codec::JsonReader jr;
  auto doc2 = jr.readDocument(json);
  check(doc2.head.units.size() == 1,
        "ENC-JSON-UNIT: unit survives the JSON round-trip");
  if (doc2.head.units.size() == 1) {
    check(doc2.head.units[0].category == "angle" &&
              doc2.head.units[0].name == "degrees",
          "ENC-JSON-UNIT: category/name survive");
    check(doc2.head.units[0].conversionFactor > 0.017 &&
              doc2.head.units[0].conversionFactor < 0.018,
          "ENC-JSON-UNIT: conversionFactor survives");
  }
}

// ---------------------------------------------------------------------------
// ENC-JSON-CTRL: control characters are escaped per RFC 8259 and round-trip.
// ---------------------------------------------------------------------------
void testJsonControlChars() {
  const char *kXml = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0"><Scene><WorldInfo title='t'/></Scene></X3D>
)XML";
  codec::XmlReader xml;
  auto doc = xml.readDocument(kXml);
  const std::string ctrl = std::string("a") + '\b' + "b" + '\f' + "c" +
                           '\x01' + "d"; // U+0008, U+000C, U+0001
  doc.head.meta.push_back(runtime::Meta{"ctrl", ctrl, "", "", "", ""});

  codec::JsonWriter jw;
  std::string json = jw.writeDocument(doc);
  bool rawCtrl = false;
  bool inString = false;
  for (std::size_t i = 0; i < json.size(); ++i) {
    char c = json[i];
    if (c == '"' && (i == 0 || json[i - 1] != '\\'))
      inString = !inString;
    if (inString && static_cast<unsigned char>(c) < 0x20 && c != '\n')
      rawCtrl = true; // raw control byte inside a JSON string: RFC violation
  }
  check(!rawCtrl,
        "ENC-JSON-CTRL: no raw control bytes inside JSON string literals");

  codec::JsonReader jr;
  auto doc2 = jr.readDocument(json);
  bool found = false;
  for (const auto &m : doc2.head.meta)
    if (m.name == "ctrl" && m.content == ctrl)
      found = true;
  check(found, "ENC-JSON-CTRL: control chars survive the JSON round-trip");
}

// ---------------------------------------------------------------------------
// ENC-C14N-ATTR: tab/newline/CR in attribute values are emitted as character
// references so a conformant reader does not normalize them away.
// ---------------------------------------------------------------------------
void testCanonAttrWhitespace() {
  const char *kXml = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0"><Scene><WorldInfo title='t'/></Scene></X3D>
)XML";
  codec::XmlReader xml;
  auto doc = xml.readDocument(kXml);
  const std::string multi = "line1\nline2\ttab\rcr";
  doc.head.meta.push_back(runtime::Meta{"ws", multi, "", "", "", ""});

  codec::CanonicalXmlWriter cw;
  std::string canon = cw.writeDocument(doc);
  check(contains(canon, "&#xA;"),
        "ENC-C14N-ATTR: newline escaped as &#xA; in attribute value");
  check(contains(canon, "&#x9;"),
        "ENC-C14N-ATTR: tab escaped as &#x9; in attribute value");
  check(contains(canon, "&#xD;"),
        "ENC-C14N-ATTR: CR escaped as &#xD; in attribute value");

  auto doc2 = xml.readDocument(canon);
  bool found = false;
  for (const auto &m : doc2.head.meta)
    if (m.name == "ws" && m.content == multi)
      found = true;
  check(found,
        "ENC-C14N-ATTR: whitespace-bearing attribute survives reparse of the "
        "canonical form");
}

} // namespace

TEST_CASE("codec_string_hardening_test") {
  testVrmlStringEscaping();
  testMfStringUnknownEscape();
  testVrmlSfImageRead();
  testCdataSplit();
  testJsonUnit();
  testJsonControlChars();
  testCanonAttrWhitespace();
  std::cout << (failures == 0 ? "ALL OK" : "FAILURES") << "\n";
  CHECK(failures == 0);
}
