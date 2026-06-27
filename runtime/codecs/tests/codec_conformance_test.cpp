#include "doctest/doctest.h"
// codec_conformance_test.cpp
// Spec-conformance regression tests for the field-value codecs:
//   CDC-1: ClassicVRML emits TRUE/FALSE for SFBool/MFBool (ISO 19776-2).
//   CDC-2: ClassicVRML escapes \ and " inside SFString.
//   CDC-3: SFMatrix3f/3d/4f/4d and SFImage/MFImage format+parse round-trip.
//   CDC-4: Float/double formatting and parsing are locale-independent.
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "FieldValueIO.hpp"
#include "VrmlWriter.hpp"

#include "x3d/nodes/PixelTexture.hpp"    // SFImage field + SFBool repeatS/repeatT
#include "x3d/nodes/WorldInfo.hpp"       // SFString title for VRML escaping test
#include "X3DRuntime.hpp"

#include <any>
#include <clocale>
#include <iostream>
#include <locale>
#include <string>
#include <vector>

using namespace x3d;
using namespace x3d::codec;

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

// --- CDC-1: VRML bool casing -------------------------------------------------
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
  check(contains(out, "FALSE"), "CDC-1: VRML emits FALSE for SFBool=false");
  check(!contains(out, " false"), "CDC-1: VRML does not emit lowercase false");
  check(!contains(out, " true"), "CDC-1: VRML does not emit lowercase true");
}

// --- CDC-2: VRML SFString escaping ------------------------------------------
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
  check(contains(out, "a\\\"b\\\\c"),
        "CDC-2: VRML escapes \\ and \" inside SFString");
}

// --- CDC-3: Matrix round-trips ----------------------------------------------
void testMatrix3f() {
  SFMatrix3f m{};
  float k = 0.0f;
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c)
      m.matrix[r][c] = (k += 1.5f);
  std::string s = formatValue(X3DFieldType::SFMatrix3f, std::any(m));
  check(!s.empty(), "CDC-3: SFMatrix3f formats non-empty");
  auto back = parseValue(X3DFieldType::SFMatrix3f, s);
  check(back.has_value(), "CDC-3: SFMatrix3f parses non-empty");
  auto m2 = std::any_cast<SFMatrix3f>(back);
  bool eq = true;
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c)
      if (m.matrix[r][c] != m2.matrix[r][c]) eq = false;
  check(eq, "CDC-3: SFMatrix3f round-trips");
}

void testMatrix4d() {
  SFMatrix4d m{};
  double k = 0.0;
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 4; ++c)
      m.matrix[r][c] = (k += 0.25);
  std::string s = formatValue(X3DFieldType::SFMatrix4d, std::any(m));
  check(!s.empty(), "CDC-3: SFMatrix4d formats non-empty");
  auto m2 = std::any_cast<SFMatrix4d>(parseValue(X3DFieldType::SFMatrix4d, s));
  bool eq = true;
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 4; ++c)
      if (m.matrix[r][c] != m2.matrix[r][c]) eq = false;
  check(eq, "CDC-3: SFMatrix4d round-trips");
}

// --- CDC-3: Image round-trips -----------------------------------------------
void testImage() {
  // 2x1 RGB image: two pixels.
  SFImage img;
  img.width = 2;
  img.height = 1;
  img.numComponents = 3;
  img.data = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
  std::string s = formatValue(X3DFieldType::SFImage, std::any(img));
  check(!s.empty(), "CDC-3: SFImage formats non-empty");
  auto img2 = std::any_cast<SFImage>(parseValue(X3DFieldType::SFImage, s));
  check(img2.width == 2 && img2.height == 1 && img2.numComponents == 3,
        "CDC-3: SFImage header round-trips");
  check(img2.data == img.data, "CDC-3: SFImage data round-trips");

  // MFImage with one image.
  std::vector<SFImage> mf{img};
  std::string ms = formatValue(X3DFieldType::MFImage, std::any(mf));
  check(!ms.empty(), "CDC-3: MFImage formats non-empty");
  auto mf2 = std::any_cast<std::vector<SFImage>>(
      parseValue(X3DFieldType::MFImage, ms));
  check(mf2.size() == 1 && mf2[0].data == img.data,
        "CDC-3: MFImage round-trips");
}

// --- CDC-4: locale-independent float ----------------------------------------
void testFloatDot() {
  std::string s = formatValue(X3DFieldType::SFFloat, std::any(3.14159f));
  check(contains(s, "."), "CDC-4: SFFloat formats with '.' decimal point");
  check(!contains(s, ","), "CDC-4: SFFloat formats without ',' separator");
  float back = std::any_cast<float>(parseValue(X3DFieldType::SFFloat, "3.14159"));
  check(back > 3.14f && back < 3.15f, "CDC-4: SFFloat parses '.' input");
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
    std::cout << "skip: CDC-4 comma-locale not installed\n";
    return;
  }
  std::string s = formatValue(X3DFieldType::SFDouble, std::any(3.14159));
  check(contains(s, "."), "CDC-4: SFDouble uses '.' even under comma locale");
  check(!contains(s, ","), "CDC-4: SFDouble no ',' under comma locale");
  double back =
      std::any_cast<double>(parseValue(X3DFieldType::SFDouble, "2.71828"));
  check(back > 2.71 && back < 2.72,
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

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    CHECK(false);
    return;
  }
  std::cout << "all codec conformance checks passed\n";
  return;
}
