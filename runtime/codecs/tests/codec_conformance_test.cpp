#include "doctest/doctest.h"
// Codec value formatting follows ISO/IEC 19776-2 and preserves round-trip values.

#include "FieldValueIO.hpp"
#include "ClassicVrmlReader.hpp"
#include "VrmlWriter.hpp"
#include "XmlReader.hpp"

#include "x3d/nodes/PixelTexture.hpp"    // SFImage field + SFBool repeatS/repeatT
#include "x3d/nodes/WorldInfo.hpp"       // SFString title for VRML escaping test
#include "X3DRuntime.hpp"

#include <any>
#include <clocale>
#include <locale>
#include <string>
#include <vector>

using namespace x3d;
using namespace x3d::codec;
using namespace x3d::core;
using namespace x3d::nodes;

TEST_CASE("XML and Classic readers accept signed decimal and hexadecimal numbers") {
  for (bool xml : {true, false}) {
    CAPTURE(xml);
    auto doc = xml ? XmlReader().readDocument(
        "<X3D version='4.0'><Scene>"
        "<Transform translation='+1 +.5 -2'/>"
        "<TimeSensor cycleInterval='+2.5'/>"
        "<IntegerTrigger triggerValue='+42'/>"
        "<IntegerTrigger triggerValue='0x2A'/>"
        "<MetadataInteger value='+42 0x2A 0X2a -0x2A +0X2A -0x80000000 0x7fffffff'/>"
        "</Scene></X3D>") : ClassicVrmlReader().readDocument(
        "#X3D V4.0 utf8\nPROFILE Full\n"
        "Transform { translation +1 +.5 -2 }\n"
        "TimeSensor { cycleInterval +2.5 }\n"
        "IntegerTrigger { triggerValue +42 }\n"
        "IntegerTrigger { triggerValue 0x2A }\n"
        "MetadataInteger { value [ +42 0x2A 0X2a -0x2A +0X2A -0x80000000 0x7fffffff ] }\n");
    REQUIRE(doc.scene.rootNodes.size() == 5);
    auto field = [&](std::size_t index, const char *name) -> std::any {
      const auto &node = *doc.scene.rootNodes.at(index);
      for (const auto &f : node.fields())
        if (f.x3dName == name) return f.get(node);
      return {};
    };
    const auto translation = std::any_cast<SFVec3f>(field(0, "translation"));
    CHECK(translation.x == 1.f);
    CHECK(translation.y == .5f);
    CHECK(translation.z == -2.f);
    CHECK(std::any_cast<double>(field(1, "cycleInterval")) == 2.5);
    CHECK(std::any_cast<int>(field(2, "triggerValue")) == 42);
    CHECK(std::any_cast<int>(field(3, "triggerValue")) == 42);
    CHECK(std::any_cast<std::vector<int>>(field(4, "value")) ==
          std::vector<int>{42, 42, 42, -42, 42, -2147483647 - 1, 2147483647});
  }
}

TEST_CASE("Numeric lexeme parsing preserves zero fallback for invalid input") {
  for (const std::string token : {"", "bad", "+", "++1", "+-1", "--1"}) {
    CAPTURE(token);
    CHECK(parseFloat(token) == 0.f);
    CHECK(parseDouble(token) == 0.);
    CHECK(parseInt(token) == 0);
  }
  for (const std::string token : {"0x", "0x-1", "0x80000000", "-0x80000001",
                                  "2147483648", "-2147483649"}) {
    CAPTURE(token);
    CHECK(parseInt(token) == 0);
  }
}

namespace {

bool contains(const std::string &hay, const std::string &needle) {
  return hay.find(needle) != std::string::npos;
}
void testVrmlBoolCasing() {
  runtime::X3DDocument doc;
  doc.version = "4.0";
  // PixelTexture has repeatS/repeatT (SFBool, default true) -> set false so
  // they are emitted by the writer (default-equal fields are suppressed).
  auto tex = std::make_shared<PixelTexture>();
  tex->setRepeatSUnchecked(false); // InitializeOnly; default true
  tex->setRepeatTUnchecked(false);
  doc.scene.rootNodes.push_back(tex);

  VrmlWriter w;
  std::string out = w.writeDocument(doc);
  CHECK_MESSAGE((contains(out, "FALSE")), "CDC-1: VRML emits FALSE for SFBool=false");
  CHECK_MESSAGE((!contains(out, " false")), "CDC-1: VRML does not emit lowercase false");
  CHECK_MESSAGE((!contains(out, " true")), "CDC-1: VRML does not emit lowercase true");
}
void testVrmlStringEscape() {
  runtime::X3DDocument doc;
  doc.version = "4.0";
  auto wi = std::make_shared<WorldInfo>();
  // Embedded double-quote and backslash must be escaped in ClassicVRML.
  wi->setTitle(std::string("a\"b\\c"));
  doc.scene.rootNodes.push_back(wi);

  VrmlWriter w;
  std::string out = w.writeDocument(doc);
  // Expect the escaped form: a\"b\\c  (backslash before " and before \).
  CHECK_MESSAGE((contains(out, "a\\\"b\\\\c")),
        "CDC-2: VRML escapes \\ and \" inside SFString");
}
void testMatrix3f() {
  SFMatrix3f m{};
  float k = 0.0f;
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c)
      m.matrix[r][c] = (k += 1.5f);
  std::string s = formatValue(X3DFieldType::SFMatrix3f, std::any(m));
  CHECK_MESSAGE((!s.empty()), "CDC-3: SFMatrix3f formats non-empty");
  auto back = parseValue(X3DFieldType::SFMatrix3f, s);
  CHECK_MESSAGE((back.has_value()), "CDC-3: SFMatrix3f parses non-empty");
  auto m2 = std::any_cast<SFMatrix3f>(back);
  bool eq = true;
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c)
      if (m.matrix[r][c] != m2.matrix[r][c]) eq = false;
  CHECK_MESSAGE((eq), "CDC-3: SFMatrix3f round-trips");
}

void testMatrix4d() {
  SFMatrix4d m{};
  double k = 0.0;
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 4; ++c)
      m.matrix[r][c] = (k += 0.25);
  std::string s = formatValue(X3DFieldType::SFMatrix4d, std::any(m));
  CHECK_MESSAGE((!s.empty()), "CDC-3: SFMatrix4d formats non-empty");
  auto m2 = std::any_cast<SFMatrix4d>(parseValue(X3DFieldType::SFMatrix4d, s));
  bool eq = true;
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 4; ++c)
      if (m.matrix[r][c] != m2.matrix[r][c]) eq = false;
  CHECK_MESSAGE((eq), "CDC-3: SFMatrix4d round-trips");
}
void testImage() {
  // 2x1 RGB image: two pixels.
  SFImage img;
  img.width = 2;
  img.height = 1;
  img.numComponents = 3;
  img.data = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
  std::string s = formatValue(X3DFieldType::SFImage, std::any(img));
  CHECK_MESSAGE((!s.empty()), "CDC-3: SFImage formats non-empty");
  auto img2 = std::any_cast<SFImage>(parseValue(X3DFieldType::SFImage, s));
  CHECK_MESSAGE((img2.width == 2 && img2.height == 1 && img2.numComponents == 3),
        "CDC-3: SFImage header round-trips");
  CHECK_MESSAGE((img2.data == img.data), "CDC-3: SFImage data round-trips");

  // MFImage with one image.
  std::vector<SFImage> mf{img};
  std::string ms = formatValue(X3DFieldType::MFImage, std::any(mf));
  CHECK_MESSAGE((!ms.empty()), "CDC-3: MFImage formats non-empty");
  auto mf2 = std::any_cast<std::vector<SFImage>>(
      parseValue(X3DFieldType::MFImage, ms));
  CHECK_MESSAGE((mf2.size() == 1 && mf2[0].data == img.data),
        "CDC-3: MFImage round-trips");
}
void testFloatDot() {
  std::string s = formatValue(X3DFieldType::SFFloat, std::any(3.14159f));
  CHECK_MESSAGE((contains(s, ".")), "CDC-4: SFFloat formats with '.' decimal point");
  CHECK_MESSAGE((!contains(s, ",")), "CDC-4: SFFloat formats without ',' separator");
  float back = std::any_cast<float>(parseValue(X3DFieldType::SFFloat, "3.14159"));
  CHECK_MESSAGE((back > 3.14f && back < 3.15f), "CDC-4: SFFloat parses '.' input");
}

void testFloatDotUnderCommaLocale() {
  // Try to install a comma-decimal locale; skip gracefully if unavailable.
  bool installed = false;
  try {
    std::locale::global(std::locale("de_DE.UTF-8"));
    installed = true;
  } catch (...) {
    try {
      std::locale::global(std::locale("de_DE"));
      installed = true;
    } catch (...) {
      installed = false;
    }
  }
  if (!installed) {

    return;
  }
  std::string s = formatValue(X3DFieldType::SFDouble, std::any(3.14159));
  CHECK_MESSAGE((contains(s, ".")), "CDC-4: SFDouble uses '.' even under comma locale");
  CHECK_MESSAGE((!contains(s, ",")), "CDC-4: SFDouble no ',' under comma locale");
  double back =
      std::any_cast<double>(parseValue(X3DFieldType::SFDouble, "2.71828"));
  CHECK_MESSAGE((back > 2.71 && back < 2.72),
        "CDC-4: SFDouble parses '.' input under comma locale");
  std::locale::global(std::locale::classic());
}

} // namespace

TEST_CASE("codec_conformance_test") {
  testVrmlBoolCasing();
  testVrmlStringEscape();
  testMatrix3f();
  testMatrix4d();
  testImage();
  testFloatDot();
  testFloatDotUnderCommaLocale();

  return;
}
