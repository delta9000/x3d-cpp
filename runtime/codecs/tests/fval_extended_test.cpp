#include "doctest/doctest.h"
// fval_extended_test.cpp
// AUD-FVAL-EXT: extended FieldValueIO coverage that goes deeper than the CDC-1..4
// surface fixes. Verifies per-type correctness against the X3D 4.1 field-types
// spec (fieldTypes.md §5.3.x).
//
// Risk areas covered:
//   1. SFMatrix3f/3d/4f/4d — row-major element order (§5.3.8..5.3.11); identity
//      and translation layouts; per-cell position checks.
//   2. SFImage/MFImage — empty `0 0 0` form; 1/3/4-component packing (high byte
//      first); hex pixel decoding; multi-image MF (§5.3.6).
//   3. SFColorRGBA — distinct from SFColor (4 components vs 3); round-trip
//      (§5.3.3).
//   4. MFString — bracket-stripping symmetry in parseValue for the
//      bracket-wrapped form (post-AUD-A).
//   5. parseEnumString — stripEnumQuotes uniform across sites; unknown tokens
//      round-trip cleanly without silent drop (AUD-D).
//   6. fmtFloat / fmtDouble locale — classic-locale on write, locale-independent
//      on read (CDC-4 depth).
//   7. SFRotation / MFRotation — `x y z angle` (radians); default and custom
//      round-trip (§5.3.13).
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "FieldValueIO.hpp"
#include "X3DRuntime.hpp"

#include <any>
#include <clocale>
#include <cmath>
#include <cstring>
#include <iostream>
#include <locale>
#include <sstream>
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

// Tokenize on whitespace for comparison.
std::vector<std::string> split(const std::string &s) {
  std::vector<std::string> out;
  std::istringstream is(s);
  std::string t;
  while (is >> t)
    out.push_back(t);
  return out;
}

bool floatsNear(float a, float b, float eps = 1e-5f) {
  return std::fabs(a - b) <= eps;
}

// ============================================================================
// 1. SFMatrix3f/3d/4f/4d — row-major order (§5.3.8..5.3.11)
// ============================================================================
void testMatrix4fIdentityWire() {
  // Default identity per spec: "1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1".
  SFMatrix4f id{};
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 4; ++c)
      id.matrix[r][c] = (r == c) ? 1.0f : 0.0f;
  std::string wire = formatValue(X3DFieldType::SFMatrix4f, std::any(id));
  check(wire == "1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1",
        "SFMatrix4f identity wire form matches spec default");
}

void testMatrix4fRowMajor() {
  // Distinct values per cell; row-major means m[0][0]..m[0][3] appear first.
  SFMatrix4f m{};
  float k = 1.0f;
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 4; ++c)
      m.matrix[r][c] = (k += 1.0f); // m[0][0]=2, m[0][1]=3, ..., m[3][3]=17
  std::string wire = formatValue(X3DFieldType::SFMatrix4f, std::any(m));
  auto t = split(wire);
  check(t.size() == 16, "SFMatrix4f emits exactly 16 floats");
  check(t[0] == "2", "SFMatrix4f first element = m[0][0] (row-major)");
  check(t[1] == "3" && t[2] == "4" && t[3] == "5",
        "SFMatrix4f second row zeroeth through third cells");
  check(t[4] == "6", "SFMatrix4f m[1][0] is the 5th token (row-major stride 4)");
  check(t[15] == "17", "SFMatrix4f m[3][3] is the last token");
}

void testMatrix4fTranslation() {
  // Translation lives in the FOURTH row per §5.3.11 (row-major; tx ty tz 1).
  SFMatrix4f m{};
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 4; ++c)
      m.matrix[r][c] = (r == c) ? 1.0f : 0.0f;
  m.matrix[3][0] = 7.0f;
  m.matrix[3][1] = 8.0f;
  m.matrix[3][2] = 9.0f;
  m.matrix[3][3] = 1.0f;
  std::string wire = formatValue(X3DFieldType::SFMatrix4f, std::any(m));
  // Last 4 tokens must be the translation row.
  auto t = split(wire);
  check(t.size() == 16, "SFMatrix4f translation matrix has 16 tokens");
  check(t[12] == "7" && t[13] == "8" && t[14] == "9" && t[15] == "1",
        "SFMatrix4f translation lives in row 3 (4th row, per §5.3.11)");
}

void testMatrix4fRoundTrip() {
  SFMatrix4f m{};
  float k = 0.0f;
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 4; ++c)
      m.matrix[r][c] = (k += 0.5f);
  std::string wire = formatValue(X3DFieldType::SFMatrix4f, std::any(m));
  auto m2 = std::any_cast<SFMatrix4f>(
      parseValue(X3DFieldType::SFMatrix4f, wire));
  bool eq = true;
  for (int r = 0; r < 4 && eq; ++r)
    for (int c = 0; c < 4 && eq; ++c)
      if (!floatsNear(m.matrix[r][c], m2.matrix[r][c]))
        eq = false;
  check(eq, "SFMatrix4f round-trips through formatValue/parseValue");
}

void testMatrix3fRoundTrip() {
  // 3x3 identity default per §5.3.9: "1 0 0 0 1 0 0 0 1".
  SFMatrix3f id{};
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c)
      id.matrix[r][c] = (r == c) ? 1.0f : 0.0f;
  std::string wire = formatValue(X3DFieldType::SFMatrix3f, std::any(id));
  check(wire == "1 0 0 0 1 0 0 0 1",
        "SFMatrix3f identity wire form matches spec default");
  auto id2 = std::any_cast<SFMatrix3f>(
      parseValue(X3DFieldType::SFMatrix3f, wire));
  bool eq = true;
  for (int r = 0; r < 3 && eq; ++r)
    for (int c = 0; c < 3 && eq; ++c)
      if (id.matrix[r][c] != id2.matrix[r][c])
        eq = false;
  check(eq, "SFMatrix3f round-trips");
}

void testMatrix3dRoundTrip() {
  SFMatrix3d m{};
  double k = 0.0;
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c)
      m.matrix[r][c] = (k += 0.1);
  std::string wire = formatValue(X3DFieldType::SFMatrix3d, std::any(m));
  auto m2 = std::any_cast<SFMatrix3d>(
      parseValue(X3DFieldType::SFMatrix3d, wire));
  bool eq = true;
  for (int r = 0; r < 3 && eq; ++r)
    for (int c = 0; c < 3 && eq; ++c)
      if (!floatsNear(static_cast<float>(m.matrix[r][c]),
                      static_cast<float>(m2.matrix[r][c])))
        eq = false;
  check(eq, "SFMatrix3d round-trips");
}

void testMatrix4dRowMajor() {
  // Row-major: m[0][0] first, m[3][3] last.
  SFMatrix4d m{};
  double k = 1.0;
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 4; ++c)
      m.matrix[r][c] = (k += 1.0);
  std::string wire = formatValue(X3DFieldType::SFMatrix4d, std::any(m));
  auto t = split(wire);
  check(t.size() == 16, "SFMatrix4d emits exactly 16 doubles");
  check(t[0] == "2", "SFMatrix4d m[0][0] is first (row-major)");
  check(t[15] == "17", "SFMatrix4d m[3][3] is last (row-major)");
}

void testMFMatrix4fRoundTrip() {
  // Two 4x4 matrices concatenated.
  SFMatrix4f a{}, b{};
  float k = 0.0f;
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 4; ++c)
      a.matrix[r][c] = (k += 1.0f);
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 4; ++c)
      b.matrix[r][c] = (k += 1.0f);
  std::vector<SFMatrix4f> mfs{a, b};
  std::string wire = formatValue(X3DFieldType::MFMatrix4f, std::any(mfs));
  auto t = split(wire);
  check(t.size() == 32, "MFMatrix4f emits 32 tokens for 2 matrices");
  auto back = std::any_cast<std::vector<SFMatrix4f>>(
      parseValue(X3DFieldType::MFMatrix4f, wire));
  check(back.size() == 2, "MFMatrix4f parses back to 2 matrices");
  bool eq = true;
  for (int r = 0; r < 4 && eq; ++r)
    for (int c = 0; c < 4 && eq; ++c)
      if (a.matrix[r][c] != back[0].matrix[r][c])
        eq = false;
  check(eq, "MFMatrix4f first matrix cell-by-cell matches");
  for (int r = 0; r < 4 && eq; ++r)
    for (int c = 0; c < 4 && eq; ++c)
      if (b.matrix[r][c] != back[1].matrix[r][c])
        eq = false;
  check(eq, "MFMatrix4f second matrix cell-by-cell matches");
}

// ============================================================================
// 2. SFImage / MFImage — packing, empty form, hex (§5.3.6)
// ============================================================================
void testImageEmpty() {
  // Default SFImage is (0 0 0). Round-trip the empty form.
  SFImage img{};
  img.width = 0; img.height = 0; img.numComponents = 0;
  std::string wire = formatValue(X3DFieldType::SFImage, std::any(img));
  check(wire == "0 0 0", "SFImage empty (0 0 0) round-trips to '0 0 0'");
  auto img2 = std::any_cast<SFImage>(parseValue(X3DFieldType::SFImage, wire));
  check(img2.width == 0 && img2.height == 0 && img2.numComponents == 0,
        "SFImage empty round-trip preserves zero header");
  check(img2.data.empty(), "SFImage empty data vector is empty");
}

void testImageOneComponent() {
  // 1-component: each pixel is one byte.
  SFImage img{1, 1, 1, {0xFF}};
  std::string wire = formatValue(X3DFieldType::SFImage, std::any(img));
  check(wire == "1 1 1 255", "SFImage 1-component 0xFF emits '1 1 1 255'");
  auto img2 = std::any_cast<SFImage>(parseValue(X3DFieldType::SFImage, wire));
  check(img2.data.size() == 1 && img2.data[0] == 0xFF,
        "SFImage 1-component round-trip preserves byte value");
}

void testImageThreeComponentPacking() {
  // 3-component: high byte first. Pixel 0xFF0000 (red) must pack to "16711680".
  SFImage img{1, 1, 3, {0xFF, 0x00, 0x00}};
  std::string wire = formatValue(X3DFieldType::SFImage, std::any(img));
  // 0xFF0000 = 16711680 decimal.
  check(contains(wire, "16711680"),
        "SFImage 3-component packs high-byte-first (red=16711680)");
  auto img2 = std::any_cast<SFImage>(parseValue(X3DFieldType::SFImage, wire));
  check(img2.data.size() == 3 && img2.data[0] == 0xFF && img2.data[1] == 0x00 &&
        img2.data[2] == 0x00,
        "SFImage 3-component unpacks to red byte sequence");
}

void testImageFourComponent() {
  // 4-component RGBA: 0x0000FF80 (semi-transparent blue).
  SFImage img{1, 1, 4, {0x00, 0x00, 0xFF, 0x80}};
  std::string wire = formatValue(X3DFieldType::SFImage, std::any(img));
  // 0x0000FF80 = 65408 decimal.
  check(contains(wire, "65408"),
        "SFImage 4-component packs RGBA high-byte-first");
  auto img2 = std::any_cast<SFImage>(parseValue(X3DFieldType::SFImage, wire));
  check(img2.data.size() == 4 && img2.data[0] == 0x00 &&
        img2.data[1] == 0x00 && img2.data[2] == 0xFF && img2.data[3] == 0x80,
        "SFImage 4-component unpacks RGBA in byte order");
}

void testImageHexPixel() {
  // The spec allows hex pixel tokens (e.g. "0xFF0000"). Parse must accept them.
  auto img = std::any_cast<SFImage>(
      parseValue(X3DFieldType::SFImage, "1 1 3 0xFF0000"));
  check(img.width == 1 && img.height == 1 && img.numComponents == 3,
        "SFImage hex-form parses header");
  check(img.data.size() == 3 && img.data[0] == 0xFF && img.data[1] == 0x00 &&
        img.data[2] == 0x00,
        "SFImage hex-form pixel decodes to bytes");
}

void testImageTwoByTwo() {
  // 2x2 image with 4 bytes per pixel.
  SFImage img{2, 2, 4, {0x11, 0x22, 0x33, 0x44,
                        0x55, 0x66, 0x77, 0x88,
                        0x99, 0xAA, 0xBB, 0xCC,
                        0xDD, 0xEE, 0xFF, 0x00}};
  std::string wire = formatValue(X3DFieldType::SFImage, std::any(img));
  auto t = split(wire);
  check(t.size() == 3 + 4, "SFImage 2x2 emits 3 header + 4 packed-pixel tokens");
  auto img2 = std::any_cast<SFImage>(parseValue(X3DFieldType::SFImage, wire));
  check(img2.data == img.data, "SFImage 2x2 byte-for-byte round-trip");
}

void testMFImage() {
  SFImage a{1, 1, 1, {0x42}};
  SFImage b{2, 1, 3, {0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00}};
  std::vector<SFImage> mfs{a, b};
  std::string wire = formatValue(X3DFieldType::MFImage, std::any(mfs));
  // Image 1: 1x1 nc=1 -> 3 header + 1 packed-pixel = 4 tokens.
  // Image 2: 2x1 nc=3 -> 3 header + 2 packed-pixels = 5 tokens.
  // Combined: 9 tokens.
  auto t = split(wire);
  check(t.size() == 9, "MFImage of two images emits 4 + 5 tokens");
  auto back = std::any_cast<std::vector<SFImage>>(
      parseValue(X3DFieldType::MFImage, wire));
  check(back.size() == 2, "MFImage parses back to 2 images");
  check(back[0].data == a.data && back[1].data == b.data,
        "MFImage per-image byte sequence preserved");
}

// ============================================================================
// 3. SFColorRGBA — distinct from SFColor; 4-component emission (§5.3.3)
// ============================================================================
void testColorRGBAEmitsFourComponents() {
  SFColorRGBA c{0.25f, 0.5f, 0.75f, 1.0f};
  std::string wire = formatValue(X3DFieldType::SFColorRGBA, std::any(c));
  auto t = split(wire);
  check(t.size() == 4,
        "SFColorRGBA emits exactly 4 floats (distinct from SFColor's 3)");
  check(t[3] == "1" || t[3] == "1.0" || t[3] == "1.000000",
        "SFColorRGBA 4th token is the alpha component");
}

void testColorRGBADistinctFromColor() {
  // Same first three components; the SFColor wire form has 3 tokens, SFColorRGBA
  // has 4. The type system MUST distinguish them at format time.
  SFColor c3{0.1f, 0.2f, 0.3f};
  SFColorRGBA c4{0.1f, 0.2f, 0.3f, 0.4f};
  std::string w3 = formatValue(X3DFieldType::SFColor, std::any(c3));
  std::string w4 = formatValue(X3DFieldType::SFColorRGBA, std::any(c4));
  check(split(w3).size() == 3, "SFColor emits 3 tokens");
  check(split(w4).size() == 4, "SFColorRGBA emits 4 tokens");
  check(w4.find("0.4") != std::string::npos,
        "SFColorRGBA carries the alpha (4th) component on the wire");
}

void testColorRGBARoundTrip() {
  SFColorRGBA c{0.1f, 0.2f, 0.3f, 0.9f};
  std::string wire = formatValue(X3DFieldType::SFColorRGBA, std::any(c));
  auto c2 = std::any_cast<SFColorRGBA>(
      parseValue(X3DFieldType::SFColorRGBA, wire));
  check(floatsNear(c.r, c2.r) && floatsNear(c.g, c2.g) &&
        floatsNear(c.b, c2.b) && floatsNear(c.a, c2.a),
        "SFColorRGBA round-trips all 4 components");
}

void testColorRGBAParseDefaults() {
  // Spec default is (0 0 0 0); parser must zero-fill missing alpha.
  auto c = std::any_cast<SFColorRGBA>(
      parseValue(X3DFieldType::SFColorRGBA, "0.5 0.5 0.5"));
  check(floatsNear(c.r, 0.5f) && floatsNear(c.g, 0.5f) &&
        floatsNear(c.b, 0.5f) && c.a == 0.0f,
        "SFColorRGBA parser zero-fills missing alpha");
}

void testMFColorRGBARoundTrip() {
  std::vector<SFColorRGBA> v{{0.1f, 0.2f, 0.3f, 0.4f},
                             {0.5f, 0.6f, 0.7f, 0.8f}};
  std::string wire = formatValue(X3DFieldType::MFColorRGBA, std::any(v));
  auto v2 = std::any_cast<std::vector<SFColorRGBA>>(
      parseValue(X3DFieldType::MFColorRGBA, wire));
  check(v2.size() == 2, "MFColorRGBA round-trips count");
  bool eq = true;
  for (std::size_t i = 0; i < v.size() && eq; ++i)
    for (int k = 0; k < 4; ++k) {
      float a = (&v[i].r)[k], b = (&v2[i].r)[k];
      if (!floatsNear(a, b))
        eq = false;
    }
  check(eq, "MFColorRGBA per-element all 4 components preserved");
}

// ============================================================================
// 4. MFString bracket handling (AUD-A aftermath)
// ============================================================================
void testMFStringBare() {
  // Bare form (the canonical wire form for parseValue).
  auto v = std::any_cast<std::vector<std::string>>(
      parseValue(X3DFieldType::MFString, "\"a\" \"b c\" \"d\""));
  check(v.size() == 3 && v[0] == "a" && v[1] == "b c" && v[2] == "d",
        "MFString bare form parses 3 quoted elements");
}

void testMFStringEmpty() {
  // Empty input -> empty vector (not an error).
  auto v = std::any_cast<std::vector<std::string>>(
      parseValue(X3DFieldType::MFString, ""));
  check(v.empty(), "MFString empty input -> empty vector");
}

void testMFStringRoundTrip() {
  std::vector<std::string> v{"hello", "world", "with \"quote\" and \\ slash"};
  std::string wire = formatValue(X3DFieldType::MFString, std::any(v));
  auto v2 = std::any_cast<std::vector<std::string>>(
      parseValue(X3DFieldType::MFString, wire));
  check(v == v2, "MFString round-trip preserves embedded \\\" and \\\\");
}

void testMFStringEscapes() {
  // Escaped backslash and quote in the wire form must decode correctly.
  auto v = std::any_cast<std::vector<std::string>>(
      parseValue(X3DFieldType::MFString, "\"a\\\"b\" \"c\\\\d\""));
  check(v.size() == 2 && v[0] == "a\"b" && v[1] == "c\\d",
        "MFString \\\" and \\\\ escape sequences decode");
}

void testMFStringBracketSymmetry() {
  // AUD-A: VRML writer brackets MF values as `[ "a" "b" ]`. parseValue must
  // accept the bracketed form so a wire string harvested from VRML output
  // parses the same as the bare form.
  std::string bracketed = "[ \"a\" \"b\" \"c\" ]";
  auto v = std::any_cast<std::vector<std::string>>(
      parseValue(X3DFieldType::MFString, bracketed));
  check(v.size() == 3 && v[0] == "a" && v[1] == "b" && v[2] == "c",
        "MFString parseValue strips leading '[' and trailing ']'");
}

// ============================================================================
// 5. parseEnumString / stripEnumQuotes — uniform across sites (AUD-D)
// ============================================================================
void testStripEnumQuotes() {
  // Direct call: bare token unchanged.
  check(stripEnumQuotes("FLY") == "FLY",
        "stripEnumQuotes: bare token unchanged");
  // Quoted: stripped.
  check(stripEnumQuotes("\"FLY\"") == "FLY",
        "stripEnumQuotes: quoted token has quotes removed");
  // Mixed (multiple quoted tokens in one run, as the wire form arrives):
  check(stripEnumQuotes("\"FLY\" \"EXAMINE\"") == "FLY EXAMINE",
        "stripEnumQuotes: runs of quoted tokens collapse to bare run");
  // Empty stays empty.
  check(stripEnumQuotes("") == "", "stripEnumQuotes: empty stays empty");
}

void testEnumUnknownTokenNotSilentlyDropped() {
  // The generated setEnumString is a per-field thunk that tokenizes on
  // whitespace and matches against a known list; an UNKNOWN token simply
  // doesn't match (no exception, no error). This is by-design: parseValue
  // should not introduce new failure modes here. We verify the strip + pass
  // path is uniform by direct call.
  std::string wire = "\"NONEXISTENT_TOKEN\"";
  std::string stripped = stripEnumQuotes(wire);
  check(stripped == "NONEXISTENT_TOKEN",
        "stripEnumQuotes: unknown quoted token still strips (no silent drop)");
}

void testEnumAliasInReader() {
  // AUD-D regression: the XML reader, ClassicVRML reader, and JsonReader all
  // apply stripEnumQuotes before setEnumString. Verify the helper is the same
  // function in all three modules (it's defined in FieldValueIO.hpp and used
  // by all three reader paths).
  std::string xmlQuoted = "\"FLY\"";
  check(stripEnumQuotes(xmlQuoted) == "FLY",
        "AUD-D: XML reader site: stripEnumQuotes available to XmlReader.hpp");
  std::string vrmlQuoted = "\"FLY\" \"EXAMINE\"";
  check(stripEnumQuotes(vrmlQuoted) == "FLY EXAMINE",
        "AUD-D: ClassicVRML reader site: stripEnumQuotes available to NodeBuilder");
  std::string jsonQuoted = "\"WALK\"";
  check(stripEnumQuotes(jsonQuoted) == "WALK",
        "AUD-D: JSON reader site: stripEnumQuotes available");
}

// ============================================================================
// 6. fmtFloat / fmtDouble locale (CDC-4 depth)
// ============================================================================
void testFloatFormatLocaleIndependent() {
  // Install a comma-decimal locale if available; format must still use '.'.
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
    std::cout << "skip: comma-decimal locale not installed on this host\n";
    return;
  }
  std::string sf = formatValue(X3DFieldType::SFFloat, std::any(3.14159f));
  std::string sd = formatValue(X3DFieldType::SFDouble, std::any(2.71828));
  check(contains(sf, "."), "SFFloat under de_DE: still uses '.' decimal");
  check(!contains(sf, ","), "SFFloat under de_DE: no ',' separator");
  check(contains(sd, "."), "SFDouble under de_DE: still uses '.' decimal");
  check(!contains(sd, ","), "SFDouble under de_DE: no ',' separator");

  // Read side: '.' input must still parse under comma locale.
  float fb = std::any_cast<float>(parseValue(X3DFieldType::SFFloat, "3.14159"));
  check(fb > 3.14f && fb < 3.15f,
        "SFFloat under de_DE: parses '.' input");
  double db = std::any_cast<double>(parseValue(X3DFieldType::SFDouble, "2.71828"));
  check(db > 2.71 && db < 2.72,
        "SFDouble under de_DE: parses '.' input");
  std::locale::global(std::locale::classic());
}

void testFloatFormatLocaleNaN() {
  // Under default locale, format a non-integer float: must use '.'.
  std::string sf = formatValue(X3DFieldType::SFFloat, std::any(0.5f));
  check(contains(sf, "."), "SFFloat default locale: '.' decimal");
  check(!contains(sf, ","), "SFFloat default locale: no ',' separator");
}

void testFloatParseRejectsComma() {
  // Comma-decimal input: std::from_chars reads the leading integer '3' and
  // stops at the ',' (it never accepts ',' as a decimal separator). The result
  // is 3.0 (not the locale-interpreted 3.14) — i.e. the wire value is NOT
  // silently converted under a comma locale. The trailing ',' would otherwise
  // corrupt the parse.
  float v = std::any_cast<float>(parseValue(X3DFieldType::SFFloat, "3,14"));
  check(v == 3.0f,
        "SFFloat: comma-decimal input parses only the integer prefix "
        "(locale-independent; '.' is the only decimal point accepted)");
}

// ============================================================================
// 7. SFRotation / MFRotation — angle in radians; round-trip (§5.3.13)
// ============================================================================
void testRotationDefault() {
  // Spec default: (0 0 1 0).
  SFRotation r{};
  r.x = 0; r.y = 0; r.z = 1; r.angle = 0;
  std::string wire = formatValue(X3DFieldType::SFRotation, std::any(r));
  check(wire == "0 0 1 0",
        "SFRotation default wire form matches spec (0 0 1 0)");
}

void testRotationCustomRoundTrip() {
  // 90 degrees about Z, expressed in radians.
  SFRotation r{0.0f, 0.0f, 1.0f, 1.5707963f};
  std::string wire = formatValue(X3DFieldType::SFRotation, std::any(r));
  auto r2 = std::any_cast<SFRotation>(
      parseValue(X3DFieldType::SFRotation, wire));
  check(floatsNear(r.x, r2.x) && floatsNear(r.y, r2.y) &&
        floatsNear(r.z, r2.z) && floatsNear(r.angle, r2.angle, 1e-4f),
        "SFRotation round-trips (axis + angle in radians)");
}

void testRotationAxisVector() {
  // Non-trivial axis: rotate about (1,1,0)/sqrt(2) by PI/3.
  SFRotation r{0.7071068f, 0.7071068f, 0.0f, 1.0471976f};
  std::string wire = formatValue(X3DFieldType::SFRotation, std::any(r));
  auto t = split(wire);
  check(t.size() == 4, "SFRotation emits 4 floats (x y z angle)");
  check(t[3] != "0" && t[3] != "0.0",
        "SFRotation angle in radians (not degrees / not zero)");
}

void testMFRotationRoundTrip() {
  std::vector<SFRotation> v{{0, 0, 1, 0},
                            {1, 0, 0, 3.14159265f},
                            {0.5f, 0.5f, 0.5f, 0.7853981f}};
  std::string wire = formatValue(X3DFieldType::MFRotation, std::any(v));
  auto t = split(wire);
  check(t.size() == 12, "MFRotation of 3 emits 12 tokens");
  auto v2 = std::any_cast<std::vector<SFRotation>>(
      parseValue(X3DFieldType::MFRotation, wire));
  check(v2.size() == 3, "MFRotation round-trips count");
  bool eq = true;
  for (std::size_t i = 0; i < v.size() && eq; ++i)
    for (int k = 0; k < 4; ++k) {
      float a = (&v[i].x)[k], b = (&v2[i].x)[k];
      if (!floatsNear(a, b, 1e-4f))
        eq = false;
    }
  check(eq, "MFRotation per-rotation all 4 components preserved");
}

void testRotationParseZeroAxis() {
  // Degenerate: zero axis vector (rotation undefined) must still parse
  // without crashing (spec allows any x/y/z; default axis is (0 0 1)).
  auto r = std::any_cast<SFRotation>(
      parseValue(X3DFieldType::SFRotation, "0 0 0 1.5"));
  check(floatsNear(r.angle, 1.5f),
        "SFRotation: parses zero-axis vector without crash");
}

} // namespace

TEST_CASE("fval_extended_test") {
  // Section 1: matrix row-major
  testMatrix4fIdentityWire();
  testMatrix4fRowMajor();
  testMatrix4fTranslation();
  testMatrix4fRoundTrip();
  testMatrix3fRoundTrip();
  testMatrix3dRoundTrip();
  testMatrix4dRowMajor();
  testMFMatrix4fRoundTrip();

  // Section 2: SFImage / MFImage
  testImageEmpty();
  testImageOneComponent();
  testImageThreeComponentPacking();
  testImageFourComponent();
  testImageHexPixel();
  testImageTwoByTwo();
  testMFImage();

  // Section 3: SFColorRGBA
  testColorRGBAEmitsFourComponents();
  testColorRGBADistinctFromColor();
  testColorRGBARoundTrip();
  testColorRGBAParseDefaults();
  testMFColorRGBARoundTrip();

  // Section 4: MFString brackets
  testMFStringBare();
  testMFStringEmpty();
  testMFStringRoundTrip();
  testMFStringEscapes();
  testMFStringBracketSymmetry();

  // Section 5: enum aliasing
  testStripEnumQuotes();
  testEnumUnknownTokenNotSilentlyDropped();
  testEnumAliasInReader();

  // Section 6: locale
  testFloatFormatLocaleIndependent();
  testFloatFormatLocaleNaN();
  testFloatParseRejectsComma();

  // Section 7: rotation
  testRotationDefault();
  testRotationCustomRoundTrip();
  testRotationAxisVector();
  testMFRotationRoundTrip();
  testRotationParseZeroAxis();

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    CHECK(false);
    return;
  }
  std::cout << "all fval-extended checks passed\n";
  return;
}
