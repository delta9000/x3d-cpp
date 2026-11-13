#include "doctest/doctest.h"
// String escaping, image pixels, units, and attribute whitespace survive round-trips.

#include "ClassicVrmlReader.hpp"
#include "DynamicField.hpp"
#include "JsonReader.hpp"
#include "JsonLite.hpp"
#include "VrmlWriter.hpp"
#include "X3DCodecs.hpp"
#include "X3DRuntime.hpp"
#include "X3DParse.hpp"
#include "UnitConversion.hpp"

#include "x3d/nodes/PixelTexture.hpp"
#include "x3d/nodes/Script.hpp"
#include "x3d/nodes/Transform.hpp"
#include "x3d/nodes/WorldInfo.hpp"

#include <functional>
#include <algorithm>
#include <memory>
#include <string>

using namespace x3d;
using namespace x3d::nodes;

namespace {

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
// META + string fields with embedded quote/backslash survive
// the XML -> ClassicVRML -> reparse hop.
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
  CHECK_MESSAGE((doc.head.meta.size() == 1 &&
            doc.head.meta[0].content == "adding a \"red\" ball via c:\\tex"),
        "ENC-VRML-STRING: XML parse keeps quote+backslash in meta content");
  {
    auto wi0 = findNode<WorldInfo>(doc.scene);
    CHECK_MESSAGE((wi0 && wi0->getInfo().size() == 2),
          "ENC-VRML-STRING: fixture sanity — XML parse yields 2 MFString "
          "elements");
  }

  codec::VrmlWriter vw;
  std::string vrml = vw.writeDocument(doc);
  codec::ClassicVrmlReader cv;
  auto doc2 = cv.readDocument(vrml);

  CHECK_MESSAGE((doc2.head.meta.size() == 1),
        "ENC-VRML-STRING: META statement count survives the VRML hop");
  if (doc2.head.meta.size() == 1) {
    CHECK_MESSAGE((doc2.head.meta[0].name == "description"),
          "ENC-VRML-STRING: META name survives");
    CHECK_MESSAGE((doc2.head.meta[0].content == doc.head.meta[0].content),
          ("ENC-VRML-STRING: META content with embedded quote survives (got '" +
              doc2.head.meta[0].content + "')"));
  }
  auto wi = findNode<WorldInfo>(doc2.scene);
  CHECK_MESSAGE((wi != nullptr), "ENC-VRML-STRING: WorldInfo survives the VRML hop");
  if (wi) {
    CHECK_MESSAGE((wi->getTitle() == "say \"hi\""),
          ("ENC-VRML-STRING: SFString title with quote survives (got '" +
              wi->getTitle() + "')"));
    const auto &info = wi->getInfo();
    CHECK_MESSAGE((info.size() == 2),
          ("ENC-VRML-STRING: MFString element count survives (got " +
              std::to_string(info.size()) + ")"));
    if (info.size() == 2) {
      CHECK_MESSAGE((info[0] == "first \"quoted\" item"),
            ("ENC-VRML-STRING: MFString element with quote survives (got '" +
                info[0] + "')"));
      CHECK_MESSAGE((info[1] == "second item"),
            "ENC-VRML-STRING: following MFString element intact");
    }
  }
}
// an unknown escape keeps its backslash; the two defined
// escapes (\" and \\) still unescape. Exercised through the XML attribute
// MFString path (FieldValueIO::parseMFString).
void testMfStringUnknownEscape() {
  const char *kXml = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0"><Scene>
  <WorldInfo info='"c:\new\tex" "a\"b" "a\\b"'/>
</Scene></X3D>
)XML";
  codec::XmlReader xml;
  auto doc = xml.readDocument(kXml);
  auto wi = findNode<WorldInfo>(doc.scene);
  CHECK_MESSAGE((wi != nullptr), "ENC-MFSTRING-READ: WorldInfo parsed");
  if (!wi)
    return;
  const auto &info = wi->getInfo();
  CHECK_MESSAGE((info.size() == 3), "ENC-MFSTRING-READ: 3 MFString elements");
  if (info.size() == 3) {
    CHECK_MESSAGE((info[0] == "c:\\new\\tex"),
          ("ENC-MFSTRING-READ: unknown escape keeps backslash (got '" +
              info[0] + "')"));
    CHECK_MESSAGE((info[1] == "a\"b"), "ENC-MFSTRING-READ: \\\" still unescapes");
    CHECK_MESSAGE((info[2] == "a\\b"), "ENC-MFSTRING-READ: \\\\ still unescapes");
  }

  // Same rule on the ClassicVRML tokenizer path.
  const char *kVrml = "#X3D V4.0 utf8\n"
                      "PROFILE Interchange\n"
                      "WorldInfo { title \"c:\\new\\tex\" }\n";
  codec::ClassicVrmlReader cv;
  auto doc2 = cv.readDocument(kVrml);
  auto wi2 = findNode<WorldInfo>(doc2.scene);
  CHECK_MESSAGE((wi2 != nullptr), "ENC-MFSTRING-READ: VRML WorldInfo parsed");
  if (wi2)
    CHECK_MESSAGE((wi2->getTitle() == "c:\\new\\tex"),
          ("ENC-MFSTRING-READ: VRML tokenizer keeps backslash on unknown "
          "escape (got '" +
              wi2->getTitle() + "')"));
}
// an inline PixelTexture survives the ClassicVRML hop.
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
  CHECK_MESSAGE((orig != nullptr), "ENC-VRML-SFIMAGE: PixelTexture parsed from XML");
  if (!orig)
    return;
  const core::SFImage &img = orig->getImage();
  CHECK_MESSAGE((img.width == 2 && img.height == 2 && img.numComponents == 3 &&
            img.data.size() == 12),
        "ENC-VRML-SFIMAGE: XML parse yields a full 2x2x3 image");

  codec::VrmlWriter vw;
  std::string vrml = vw.writeDocument(doc);
  codec::ClassicVrmlReader cv;
  auto doc2 = cv.readDocument(vrml);
  std::shared_ptr<PixelTexture> re;
  for (const auto &n : doc2.scene.rootNodes)
    if ((re = dig(n)))
      break;
  CHECK_MESSAGE((re != nullptr), "ENC-VRML-SFIMAGE: PixelTexture survives the VRML hop");
  if (re)
    CHECK_MESSAGE((re->getImage() == img),
          ("ENC-VRML-SFIMAGE: SFImage identical after the VRML hop (got " +
              std::to_string(re->getImage().width) + " " +
              std::to_string(re->getImage().height) + " " +
              std::to_string(re->getImage().numComponents) + ", " +
              std::to_string(re->getImage().data.size()) + " bytes)"));
}
// Script source containing a literal ]]> survives the XML
// writer (split CDATA) and reparses intact (XmlLite concatenates consecutive
// CDATA sections).
void testCdataSplit() {
  const char *kXml = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0"><Scene>
  <Script DEF="S"><![CDATA[ecmascript: function initialize() {}]]></Script>
</Scene></X3D>
)XML";
  codec::XmlReader xml;
  auto doc = xml.readDocument(kXml);
  auto script = findNode<Script>(doc.scene);
  CHECK_MESSAGE((script != nullptr), "ENC-CDATA-SCRIPT: Script parsed");
  if (!script)
    return;
  const std::string source = "ecmascript: if (a]]>b) { mask(); }";
  script->setSourceCode(source);

  codec::XmlWriter xw;
  std::string out = xw.writeDocument(doc);
  CHECK_MESSAGE((!contains(out, "if (a]]>b)")),
        "ENC-CDATA-SCRIPT: writer does not emit a raw ]]> inside CDATA");

  auto doc2 = xml.readDocument(out);
  auto script2 = findNode<Script>(doc2.scene);
  CHECK_MESSAGE((script2 != nullptr), "ENC-CDATA-SCRIPT: Script survives round-trip");
  if (script2)
    CHECK_MESSAGE((contains(script2->getSourceCode(), "if (a]]>b) { mask(); }")),
          ("ENC-CDATA-SCRIPT: ]]> in source survives round-trip (got '" +
              script2->getSourceCode() + "')"));
}
// <unit> statements survive the JSON hop.
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
  CHECK_MESSAGE((doc.head.units.size() == 1), "ENC-JSON-UNIT: XML parse reads the unit");

  codec::JsonWriter jw;
  std::string json = jw.writeDocument(doc);
  CHECK_MESSAGE((contains(json, "\"unit\"")),
        "ENC-JSON-UNIT: JSON output has a unit array");

  codec::JsonReader jr;
  auto doc2 = jr.readDocument(json);
  CHECK_MESSAGE((doc2.head.units.size() == 1),
        "ENC-JSON-UNIT: unit survives the JSON round-trip");
  if (doc2.head.units.size() == 1) {
    CHECK_MESSAGE((doc2.head.units[0].category == "angle" &&
              doc2.head.units[0].name == "degrees"),
          "ENC-JSON-UNIT: category/name survive");
    CHECK_MESSAGE((doc2.head.units[0].conversionFactor > 0.017 &&
              doc2.head.units[0].conversionFactor < 0.018),
          "ENC-JSON-UNIT: conversionFactor survives");
  }
}
// control characters are escaped per RFC 8259 and round-trip.
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
  CHECK_MESSAGE((!rawCtrl),
        "ENC-JSON-CTRL: no raw control bytes inside JSON string literals");

  codec::JsonReader jr;
  auto doc2 = jr.readDocument(json);
  bool found = false;
  for (const auto &m : doc2.head.meta)
    if (m.name == "ctrl" && m.content == ctrl)
      found = true;
  CHECK_MESSAGE((found), "ENC-JSON-CTRL: control chars survive the JSON round-trip");
}
// 14N-attr: tab/newline/CR in attribute values are emitted as character
// references so a conformant reader does not normalize them away.
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
  CHECK_MESSAGE((contains(canon, "&#xA;")),
        "ENC-C14N-ATTR: newline escaped as &#xA; in attribute value");
  CHECK_MESSAGE((contains(canon, "&#x9;")),
        "ENC-C14N-ATTR: tab escaped as &#x9; in attribute value");
  CHECK_MESSAGE((contains(canon, "&#xD;")),
        "ENC-C14N-ATTR: CR escaped as &#xD; in attribute value");

  auto doc2 = xml.readDocument(canon);
  bool found = false;
  for (const auto &m : doc2.head.meta)
    if (m.name == "ws" && m.content == multi)
      found = true;
  CHECK_MESSAGE((found),
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

}

TEST_CASE("XML literal attribute whitespace normalizes but references survive") {
  for (const std::string quote : {"'", "\""}) {
    CAPTURE(quote);
    const auto raw = codec::XmlReader().readDocument(
        "<X3D version='4.0'><Scene><WorldInfo title=" + quote +
        "  a\tb\nc\rd\r\ne  " + quote + "/></Scene></X3D>");
    const auto refs = codec::XmlReader().readDocument(
        "<X3D version='4.0'><Scene><WorldInfo title=" + quote +
        "a&#10;b&#x9;c&#13;d&amp;#10;" + quote + "/></Scene></X3D>");
    const auto literal = findNode<WorldInfo>(raw.scene);
    const auto referenced = findNode<WorldInfo>(refs.scene);
    REQUIRE(literal);
    REQUIRE(referenced);
    CHECK(literal->getTitle() == "  a b c d e  ");
    CHECK(referenced->getTitle() == "a\nb\tc\rd&#10;");
  }
}

TEST_CASE("XmlWriter keeps tab, LF and CR in attribute values across reparse") {
  const auto before = codec::XmlReader().readDocument(
      "<X3D version='4.0'><Scene><WorldInfo title='a&#x9;b&#xA;c&#xD;d'/>"
      "</Scene></X3D>");
  const auto output = codec::XmlWriter().writeDocument(before);
  const auto after = codec::XmlReader().readDocument(output);
  const auto info = findNode<WorldInfo>(after.scene);
  REQUIRE(info);
  CHECK(info->getTitle() == "a\tb\nc\rd");
}

TEST_CASE("JSON retains an explicitly empty numeric MF field") {
  auto before = codec::XmlReader().readDocument(
      "<X3D version='4.0'><Scene><NavigationInfo avatarSize=''/></Scene></X3D>");
  auto a = before.scene.rootNodes.at(0);
  REQUIRE(a->nodeTypeName() == "NavigationInfo");
  const auto &field = *std::find_if(a->fields().begin(), a->fields().end(),
      [](const auto &f) { return f.x3dName == "avatarSize"; });
  CHECK(std::any_cast<core::MFFloat>(field.get(*a)).empty());
  auto after = codec::JsonReader().readDocument(codec::JsonWriter().writeDocument(before));
  auto b = after.scene.rootNodes.at(0);
  CHECK(std::any_cast<core::MFFloat>(field.get(*b)).empty());
}

TEST_CASE("Classic writer retains IMPORT and EXPORT") {
  auto before = codec::XmlReader().readDocument(
      "<X3D version='4.0'><Scene><Inline DEF='I' load='false'/><Group DEF='G'/>"
      "<IMPORT inlineDEF='I' importedDEF='X' AS='Remote'/>"
      "<IMPORT inlineDEF='I' importedDEF='Unaliased'/>"
      "<EXPORT localDEF='G' AS='Public'/><EXPORT localDEF='G'/></Scene></X3D>");
  REQUIRE(before.scene.imports.size() == 2);
  REQUIRE(before.scene.exports.size() == 2);
  auto after = codec::ClassicVrmlReader().readDocument(codec::VrmlWriter().writeDocument(before));
  REQUIRE(after.scene.imports.size() == 2);
  REQUIRE(after.scene.exports.size() == 2);
  CHECK(after.scene.imports[0].inlineDEF == "I");
  CHECK(after.scene.imports[0].importedDEF == "X");
  CHECK(after.scene.imports[0].as == "Remote");
  CHECK(after.scene.imports[1].importedDEF == "Unaliased");
  CHECK(after.scene.imports[1].as == "Unaliased");
  CHECK(after.scene.exports[0].localDEF == "G");
  CHECK(after.scene.exports[0].as == "Public");
  CHECK(after.scene.exports[1].localDEF == "G");
  CHECK(after.scene.exports[1].as == "G");
}

TEST_CASE("JSON retains extended metadata attributes") {
  auto before = codec::XmlReader().readDocument(
      "<X3D version='4.0'><head><meta name='description' content='salut' "
      "dir='ltr' lang='fr' scheme='urn:test' http-equiv='Content-Language'/>"
      "</head><Scene/></X3D>");
  auto after = codec::JsonReader().readDocument(codec::JsonWriter().writeDocument(before));
  REQUIRE(after.head.meta.size() == 1);
  CHECK(after.head.meta[0].name == "description");
  CHECK(after.head.meta[0].content == "salut");
  CHECK(after.head.meta[0].dir == "ltr");
  CHECK(after.head.meta[0].lang == "fr");
  CHECK(after.head.meta[0].scheme == "urn:test");
  CHECK(after.head.meta[0].httpEquiv == "Content-Language");
}

TEST_CASE("canonical output retains MFImage and every MFMatrix type") {
  const std::pair<const char *, const char *> fields[] = {
      {"MFImage", "1 1 3 16711680 1 1 4 4278255360"},
      {"MFMatrix3f", "1.25 2 3 4 5 6 7 8 9 9 8 7 6 5 4 3 2 1.5"},
      {"MFMatrix3d", "1.25 2 3 4 5 6 7 8 9 9 8 7 6 5 4 3 2 1.5"},
      {"MFMatrix4f", "1.25 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16"},
      {"MFMatrix4d", "1.25 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16"}};
  std::string source = "<X3D version='4.0'><Scene><Script>";
  for (const auto &[type, value] : fields)
    source += "<field name='" + std::string(type) + "' type='" + type +
        "' accessType='initializeOnly' value='" + value + "'/>";
  source += "</Script></Scene></X3D>";
  auto before = codec::XmlReader().readDocument(source);
  const auto output = codec::CanonicalXmlWriter().writeDocument(before);
  auto after = codec::XmlReader().readDocument(output);
  auto a = findNode<Script>(before.scene);
  auto b = findNode<Script>(after.scene);
  REQUIRE(a);
  REQUIRE(b);
  for (const auto &f : before.scene.authorFields->authorFields(*a)) {
    CAPTURE(f.x3dName);
    auto expected = codec::formatValue(f.type, f.get(*a));
    CHECK_FALSE(expected.empty());
    auto actual = after.scene.authorFields->getValue(*b, f.x3dName);
    REQUIRE(actual.has_value());
    CHECK(codec::formatValue(f.type, actual) == expected);
  }
  CHECK(codec::CanonicalXmlWriter().writeDocument(after) == output);
}

TEST_CASE("Script node defaults survive canonical JSON and Classic writers") {
  auto before = codec::XmlReader().readDocument(
      "<X3D version='4.0'><Scene><Script>"
      "<field name='target' type='SFNode' accessType='initializeOnly'><Group/></field>"
      "<field name='items' type='MFNode' accessType='inputOutput'><Shape><Box/></Shape></field>"
      "</Script></Scene></X3D>");
  const std::pair<std::string, codec::Encoding> outputs[] = {
      {codec::CanonicalXmlWriter().writeDocument(before), codec::Encoding::XML},
      {codec::JsonWriter().writeDocument(before), codec::Encoding::JSON},
      {codec::VrmlWriter().writeDocument(before), codec::Encoding::ClassicVRML}};
  for (const auto &[output, encoding] : outputs) {
    CAPTURE(output);
    if (encoding == codec::Encoding::JSON) {
      auto dom = json::parseComplete(output);
      auto children = dom->member("X3D")->member("Scene")->member("-children");
      REQUIRE(children->array.size() == 1);
      auto fields = children->array[0]->member("Script")->member("field");
      REQUIRE(fields);
      REQUIRE(fields->array.size() == 2);
      for (std::size_t i = 0; i < fields->array.size(); ++i) {
        auto defaults = fields->array[i]->member("-children");
        CHECK(defaults != nullptr);
        if (!defaults) continue;
        REQUIRE(defaults->array.size() == 1);
        CHECK(defaults->array[0]->member(i == 0 ? "Group" : "Shape") != nullptr);
      }
    } else if (encoding == codec::Encoding::ClassicVRML) {
      CHECK(output.find("SFNode target Group {") != std::string::npos);
      CHECK(output.find("MFNode items [") != std::string::npos);
      CHECK(output.find("Shape {") != std::string::npos);
      CHECK(output.find("Box {") != std::string::npos);
    }
    try {
      auto after = codec::parseDocument(output, encoding);
      auto script = findNode<Script>(after.scene);
      REQUIRE(script);
      auto one = after.scene.authorFields->getValue(*script, "target");
      auto many = after.scene.authorFields->getValue(*script, "items");
      auto p = std::any_cast<core::SFNode>(&one);
      auto v = std::any_cast<core::MFNode>(&many);
      const bool hasTarget = p && *p && (*p)->nodeTypeName() == "Group";
      const bool hasItems = v && v->size() == 1 && (*v)[0]->nodeTypeName() == "Shape";
      if (encoding == codec::Encoding::XML) {
        CHECK(hasTarget);
        CHECK(hasItems);
      } else {
        CHECK(hasTarget);
        CHECK(hasItems);
      }
    } catch (const std::exception &e) {
      CAPTURE(e.what());
      if (encoding == codec::Encoding::XML) CHECK(false);
      else CHECK(false);
    }
  }
}

TEST_CASE("shader author interfaces survive every writer") {
  for (const std::string type : {"ComposedShader", "PackagedShader", "ShaderProgram"}) {
    auto before = codec::XmlReader().readDocument(
        "<X3D version='4.0'><Scene><" + type + " DEF='Shader' language='GLSL'>"
        "<field name='tint' type='SFColor' accessType='inputOutput' value='1 0.25 0'/>"
        "<field name='pulse' type='SFFloat' accessType='inputOnly'/>"
        "</" + type + "></Scene></X3D>");
    auto original = before.scene.resolve("Shader");
    REQUIRE(original);
    REQUIRE(before.scene.authorFields->authorFields(*original).size() == 2);
    const std::pair<std::string, codec::Encoding> outputs[] = {
        {codec::XmlWriter().writeDocument(before), codec::Encoding::XML},
        {codec::CanonicalXmlWriter().writeDocument(before), codec::Encoding::XML},
        {codec::JsonWriter().writeDocument(before), codec::Encoding::JSON},
        {codec::VrmlWriter().writeDocument(before), codec::Encoding::ClassicVRML}};
    for (const auto &[output, encoding] : outputs) {
      CAPTURE(type);
      CAPTURE(output);
      if (encoding == codec::Encoding::JSON) {
        auto dom = json::parseComplete(output);
        auto children = dom->member("X3D")->member("Scene")->member("-children");
        REQUIRE(children->array.size() == 1);
        auto fields = children->array[0]->member(type)->member("field");
        CHECK(fields != nullptr);
        if (fields) CHECK(fields->array.size() == 2);
      } else if (encoding == codec::Encoding::ClassicVRML) {
        CHECK(output.find("inputOutput SFColor tint 1 0.25 0") != std::string::npos);
        CHECK(output.find("inputOnly SFFloat pulse") != std::string::npos);
      }
      auto after = codec::parseDocument(output, encoding);
      auto shader = after.scene.resolve("Shader");
      REQUIRE(shader);
      const auto fields = after.scene.authorFields->authorFields(*shader);
      if (encoding == codec::Encoding::XML) {
        CHECK(fields.size() == 2);
      } else {
        CHECK(fields.size() == 2);
      }
      if (fields.size() == 2) {
        CHECK(fields[0].x3dName == "tint");
        CHECK(fields[0].type == core::X3DFieldType::SFColor);
        CHECK(fields[0].access == core::AccessType::InputOutput);
        auto value = std::any_cast<core::SFColor>(fields[0].get(*shader));
        CHECK(value.r == 1.f);
        CHECK(value.g == 0.25f);
        CHECK(value.b == 0.f);
        CHECK(fields[1].x3dName == "pulse");
        CHECK(fields[1].access == core::AccessType::InputOnly);
      }
    }
  }
}

// UNIT conversion is deliberately one-way until writers can preserve both
// authored PROTO values and normalized built-in fields without double scaling.
TEST_CASE("Document writers reject normalized UNIT values but preserve parsed units") {
  const std::string source =
      "<X3D profile='Interchange' version='4.0'><head>"
      "<unit category='length' name='centimetre' conversionFactor='0.01'/>"
      "</head><Scene><Transform translation='100 0 0'/></Scene></X3D>";
  auto doc = codec::parseDocument(source);
  const auto checkWritable = [](const runtime::X3DDocument &d) {
    CHECK_NOTHROW(codec::XmlWriter().writeDocument(d));
    CHECK_NOTHROW(codec::CanonicalXmlWriter().writeDocument(d));
    CHECK_NOTHROW(codec::JsonWriter().writeDocument(d));
    CHECK_NOTHROW(codec::VrmlWriter().writeDocument(d));
  };
  checkWritable(doc);
  auto roundtrip = codec::parseDocument(codec::XmlWriter().writeDocument(doc));
  REQUIRE(roundtrip.head.units.size() == 1);
  CHECK(roundtrip.head.units[0].conversionFactor == doctest::Approx(0.01));
  CHECK(codec::XmlWriter().writeDocument(roundtrip) == codec::XmlWriter().writeDocument(doc));

  auto alias = doc; // Runtime builds may take a shallow Scene copy.
  auto transform = findNode<Transform>(doc.scene);
  REQUIRE(transform);
  CHECK(transform->getTranslation().x == doctest::Approx(100));
  REQUIRE(doc.scene.sourceUnits.size() == 1);
  runtime::normalizeRuntimeUnits(alias.scene);
  CHECK(transform->getTranslation().x == doctest::Approx(1));
  CHECK(*doc.scene.hasNormalizedUnitValues);
  CHECK(*alias.scene.hasNormalizedUnitValues);
  CHECK_THROWS_WITH(codec::XmlWriter().writeDocument(doc),
      doctest::Contains("nonidentity UNIT normalization"));
  CHECK_THROWS(codec::CanonicalXmlWriter().writeDocument(doc));
  CHECK_THROWS(codec::JsonWriter().writeDocument(doc));
  CHECK_THROWS(codec::VrmlWriter().writeDocument(doc));
  CHECK_THROWS(codec::XmlWriter().writeScene(doc.scene));
  CHECK_THROWS(codec::XmlWriter().writeDocument(alias));
  checkWritable(roundtrip); // Independent parsing has not entered runtime.
}

TEST_CASE("UNIT writer guard permits identity conversion and untouched defaults") {
  for (const auto &body : {"<Transform/>", "<WorldInfo title='units'/>"}) {
    auto doc = codec::parseDocument(
        std::string("<X3D profile='Interchange' version='4.0'><head>") +
        "<unit category='length' name='centimetre' conversionFactor='0.01'/>"
        "</head><Scene>" + body + "</Scene></X3D>");
    runtime::normalizeRuntimeUnits(doc.scene);
    CHECK_NOTHROW(codec::XmlWriter().writeDocument(doc));
  }
  auto doc = codec::parseDocument(
      "<X3D profile='Interchange' version='4.0'><head>"
      "<unit category='length' name='metre' conversionFactor='1'/>"
      "</head><Scene><Transform translation='100 0 0'/></Scene></X3D>");
  runtime::normalizeRuntimeUnits(doc.scene);
  CHECK_NOTHROW(codec::XmlWriter().writeDocument(doc));
}

TEST_CASE("UNIT writer guard does not reject retained Inline references") {
  auto parent = codec::parseDocument(
      "<X3D profile='Interchange' version='4.0'><Scene>"
      "<Inline url='\"child.x3d\"'/></Scene></X3D>");
  auto child = codec::parseDocument(
      "<X3D profile='Interchange' version='4.0'><head>"
      "<unit category='length' name='centimetre' conversionFactor='0.01'/>"
      "</head><Scene><Transform translation='100 0 0'/></Scene></X3D>");
  auto retained = parent.scene.rootNodes.front();
  auto expanded = child.scene.rootNodes.front();
  parent.scene.rootNodes = {expanded};
  parent.scene.expandedInlines[expanded.get()] = retained;
  parent.scene.expandedInlineScenes[expanded.get()] =
      std::make_shared<runtime::Scene>(child.scene);
  runtime::normalizeRuntimeUnits(parent.scene);
  auto transform = findNode<Transform>(child.scene);
  REQUIRE(transform);
  CHECK(transform->getTranslation().x == doctest::Approx(1));
  CHECK(*child.scene.hasNormalizedUnitValues);
  CHECK_FALSE(*parent.scene.hasNormalizedUnitValues);
  CHECK_THROWS(codec::XmlWriter().writeDocument(child));
  for (const auto &output : {
           codec::XmlWriter().writeDocument(parent),
           codec::CanonicalXmlWriter().writeDocument(parent),
           codec::JsonWriter().writeDocument(parent),
           codec::VrmlWriter().writeDocument(parent)}) {
    CHECK(contains(output, "child.x3d"));
    CHECK_FALSE(contains(output, "Transform"));
  }
}
