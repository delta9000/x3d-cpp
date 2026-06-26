#include "doctest/doctest.h"
// encoding_lex_audit_test.cpp
// Audit unit AUD-PARSE-LEX: regression coverage for the parse front-door and
// its lexers (Encoding.hpp, Inflate.hpp, VrmlTokenizer.hpp, JsonLite.hpp).
//
// Each section corresponds to one risk area flagged in the audit scope:
//   1. Encoding::sniff precedence (content vs. extension ties, charset-suffix
//      header, gzip suffix stripping, mislabelled extensions).
//   2. Inflate::inflateGzip round-trip + malformed-input rejection (bad magic,
//      wrong CM, truncated, CRC/ISIZE mismatch, FEXTRA/FNAME/FCOMMENT/FHCRC
//      header flags).
//   3. VrmlTokenizer punctuation, comment-to-EOL, comma-as-delimiter, header
//      skip, bare-token number lexing (".5" "+.5" "-1.5" "1."), negative-with-
//      space split, quoted-string unescape ("\"", "\\").
//   4. JsonLite numberLexeme fidelity, leading BOM, trailing whitespace,
//      standard escapes, and \uXXXX / surrogate-pair decoding to valid UTF-8
//      (the surrogate-pair round-trip exposes a real bug in appendUtf8()).
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "Encoding.hpp"
#include "Inflate.hpp"
#include "JsonLite.hpp"
#include "VrmlTokenizer.hpp"

#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using x3d::codec::Encoding;
using x3d::codec::VrmlToken;
using x3d::codec::VrmlTokenizer;
using x3d::codec::inflateGzip;
using x3d::codec::isGzip;
using x3d::codec::sniff;
using x3d::codec::sniffByContent;
using x3d::codec::sniffByExtension;
using x3d::json::parse;

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

bool threwRuntimeError(const std::function<void()> &fn) {
  try {
    fn();
  } catch (const std::runtime_error &) {
    return true;
  } catch (...) {
  }
  return false;
}

// ===========================================================================
// 1. Encoding::sniff precedence.
// ===========================================================================
void testSniffPrecedence() {
  std::cout << "----- Encoding::sniff precedence -----\n";

  // Content overrides a wrong extension (.wrl file that is really X3D
  // ClassicVRML).
  check(sniff("mislabeled.wrl", "#X3D V4.0 utf8\n") == Encoding::ClassicVRML,
        "content overrides .wrl extension (really #X3D -> ClassicVRML)");

  // JSON content with .x3d extension -> content wins -> JSON.
  check(sniff("mislabeled.x3d",
              "{\"X3D\":{\"@version\":\"4.0\"}}") == Encoding::JSON,
        "content overrides .x3d extension (really JSON -> JSON)");

  // Pure #X3D V3.0 utf8 charset-suffix header is recognised as ClassicVRML.
  check(sniffByContent("#X3D V3.0 utf8\n") == Encoding::ClassicVRML,
        "#X3D V3.0 utf8 charset-suffix header -> ClassicVRML");

  // Extension fallback when content is inconclusive.
  check(sniff("plain.json", "this is not any X3D encoding") == Encoding::JSON,
        "extension is the fallback when content is inconclusive");

  // Extension stripping: .wrl.gz -> VRML97, .x3dvz -> ClassicVRML.
  check(sniffByExtension("model.wrl.gz") == Encoding::VRML97,
        ".wrl.gz strips .gz -> VRML97");
  check(sniffByExtension("model.x3dvz") == Encoding::ClassicVRML,
        ".x3dvz -> ClassicVRML (compressed extension variant)");

  // gzip magic detected by content sniff, classified as Unknown (caller
  // inflates then re-sniffs).
  std::string gzMagic = std::string("\x1f\x8b\x08\x00", 4);
  check(isGzip(gzMagic), "isGzip: magic 1f 8b detected");
  check(sniffByContent(gzMagic) == Encoding::Unknown,
        "gzip payload classified Unknown (caller inflates first)");
}

// ===========================================================================
// 2. Inflate::inflateGzip round-trip + malformed-input rejection.
// ===========================================================================

// Bytes produced by: printf "hello synthetic" | gzip -n -c | xxd -p
// Hand-checked: gzip header (1f 8b 08 00 00 00 00 00 00 03) + DEFLATE body +
// CRC32 (4) + ISIZE (4) = 35 bytes.
const unsigned char kSyntheticGzip[] = {
    0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xcb, 0x48,
    0xcd, 0xc9, 0xc9, 0x57, 0x28, 0xae, 0xcc, 0x2b, 0xc9, 0x48, 0x2d, 0xc9,
    0x4c, 0x06, 0x00, 0x3c, 0x5e, 0xb7, 0xc0, 0x0f, 0x00, 0x00, 0x00,
};

void testInflate() {
  std::cout << "----- Inflate::inflateGzip -----\n";

  // Synthetic round-trip.
  std::string gz(reinterpret_cast<const char *>(kSyntheticGzip),
                 sizeof(kSyntheticGzip));
  check(isGzip(gz), "inflate: synthetic stream has gzip magic");
  std::string out = inflateGzip(gz);
  check(out == "hello synthetic",
        "inflate: synthetic stream round-trips to 'hello synthetic'");
  check(out.size() == 15,
        "inflate: inflated size matches input length (15 bytes)");

  // Bad magic (1f 8c instead of 1f 8b).
  std::string badMagic = gz;
  badMagic[1] = 0x8c;
  check(threwRuntimeError([&] { inflateGzip(badMagic); }),
        "inflate: bad magic throws runtime_error");

  // Wrong CM (0x00 instead of DEFLATE/0x08).
  std::string wrongCm = gz;
  wrongCm[2] = 0x00;
  check(threwRuntimeError([&] { inflateGzip(wrongCm); }),
        "inflate: wrong CM (0x00) throws runtime_error");

  // Truncated (less than 18 bytes: header + footer minimum).
  std::string truncated(gz.data(), 10);
  check(threwRuntimeError([&] { inflateGzip(truncated); }),
        "inflate: truncated (<18 bytes) throws runtime_error");

  // CRC mismatch: flip a bit in the trailing CRC32.
  std::string badCrc = gz;
  badCrc[gz.size() - 8] ^= 0xFF; // toggle all bits in the low CRC byte
  check(threwRuntimeError([&] { inflateGzip(badCrc); }),
        "inflate: CRC32 mismatch throws runtime_error");

  // ISIZE mismatch: flip a bit in the trailing ISIZE (last 4 bytes).
  std::string badISize = gz;
  badISize[gz.size() - 1] ^= 0xFF;
  check(threwRuntimeError([&] { inflateGzip(badISize); }),
        "inflate: ISIZE mismatch throws runtime_error");

  // FEXTRA / FNAME / FCOMMENT / FHCRC header flags: construct a stream whose
  // gzip header sets each optional flag in turn and ensure inflateGzip walks
  // past them rather than choking on the extra bytes.
  auto setFlag = [](std::string base, unsigned char flag) {
    base[3] |= flag;
    return base;
  };
  for (unsigned char flag : {0x02, 0x04, 0x08, 0x10}) {
    std::string g = setFlag(gz, flag);
    bool threw = threwRuntimeError([&] { inflateGzip(g); });
    // FEXTRA / FNAME / FCOMMENT / FHCRC inflate fine ONLY when the optional
    // section data is well-formed. For our bare 35-byte fixture, setting any
    // of these flags without appending the section data MUST throw (truncated
    // header). This proves the parser walks the flag bits instead of assuming
    // a minimal header.
    check(threw, "inflate: flag 0x" + std::to_string(flag) +
                     " on a minimal stream throws (header walker active)");
  }
}

// ===========================================================================
// 3. VrmlTokenizer: punctuation, comments, comma, numbers, escapes.
// ===========================================================================
void testVrmlTokenizer() {
  std::cout << "----- VrmlTokenizer -----\n";

  // Header-line skip via firstLineIsHeader.
  VrmlTokenizer h("#X3D V4.0 utf8\nPROFILE Interchange\n",
                  /*firstLineIsHeader=*/true);
  check(h.next().isWord("PROFILE"), "tok: header-line skipped -> PROFILE");
  check(h.next().isWord("Interchange"),
        "tok: PROFILE arg read as Identifier");

  // Comment-to-EOL and comma-as-delimiter in one stream.
  VrmlTokenizer m("Shape { # size comment\n size 2, 2 2 }");
  check(m.next().isWord("Shape"), "tok: Shape identifier");
  check(m.next().isPunct('{'), "tok: { punct");
  check(m.next().isWord("size"), "tok: comment-to-EOL skipped before 'size'");
  check(m.next().text == "2", "tok: number 2 (1st)");
  check(m.next().text == "2", "tok: comma is whitespace -> number 2 (2nd)");
  check(m.next().text == "2", "tok: bare token run -> number 2 (3rd)");
  check(m.next().isPunct('}'), "tok: } punct");
  check(m.atEnd(), "tok: stream exhausted");

  // Quoted-string unescape: \" -> ", \\ -> \.
  VrmlTokenizer s(R"(name "he said \"hi\" \\ end")");
  check(s.next().isWord("name"), "tok: bare word before string");
  VrmlToken str = s.next();
  check(str.kind == VrmlToken::Kind::String && str.isString,
        "tok: quoted string flagged isString");
  check(str.text == "he said \"hi\" \\ end",
        "tok: \\\" -> \", \\\\ -> \\ in string lexeme");

  // Bare-token number lexing: ".5" "+.5" "-1.5" "1.".
  auto single = [](const std::string &s) {
    VrmlTokenizer t(s);
    return t.next();
  };
  VrmlToken a = single(".5");
  check(a.kind == VrmlToken::Kind::Number && a.text == ".5",
        "tok: '.5' lexes as single Number token");
  VrmlToken b = single("+.5");
  check(b.kind == VrmlToken::Kind::Number && b.text == "+.5",
        "tok: '+.5' lexes as single Number token");
  VrmlToken c = single("-1.5");
  check(c.kind == VrmlToken::Kind::Number && c.text == "-1.5",
        "tok: '-1.5' lexes as single Number token");
  VrmlToken d = single("1.");
  check(d.kind == VrmlToken::Kind::Number && d.text == "1.",
        "tok: '1.' lexes as single Number token");

  // Negative-with-space "- 1" lexes as TWO tokens (documented behaviour:
  // whitespace separates; a bare '-' is an identifier token, not a number).
  VrmlTokenizer negSp("- 1 tail");
  VrmlToken dash = negSp.next();
  VrmlToken one = negSp.next();
  VrmlToken tail = negSp.next();
  check(dash.kind == VrmlToken::Kind::Identifier && dash.text == "-",
        "tok: '- 1' splits: bare '-' is Identifier (not Number)");
  check(one.kind == VrmlToken::Kind::Number && one.text == "1",
        "tok: '- 1' splits: '1' is Number");
  check(tail.isWord("tail"), "tok: '- 1' splits: 'tail' follows");
}

// ===========================================================================
// 4. JsonLite: numberLexeme, BOM, trailing ws, escapes, \uXXXX, surrogate.
// ===========================================================================
void testJsonLite() {
  std::cout << "----- JsonLite -----\n";

  // Integer lexeme preservation: '3' not '3.0'.
  {
    auto v = parse(R"({"n":3})");
    check(v && v->member("n") && v->member("n")->isNumber(),
          "json: '3' parses as Number");
    check(v->member("n")->numberLexeme == "3",
          "json: integer lexeme preserved as '3' (not '3.0')");
    check(v->member("n")->number == 3.0,
          "json: numeric value parsed to 3.0");
  }

  // Float lexeme preserved.
  {
    auto v = parse(R"({"f":1.5e2})");
    check(v->member("f")->numberLexeme == "1.5e2",
          "json: float lexeme '1.5e2' preserved verbatim");
  }

  // Leading UTF-8 BOM is accepted.
  {
    auto v = parse(std::string("\xEF\xBB\xBF") + R"({"n":3})");
    check(v && v->member("n") && v->member("n")->number == 3.0,
          "json: leading UTF-8 BOM is stripped before parsing");
  }

  // Trailing whitespace tolerated.
  {
    auto v = parse(R"({"n":3}   )");
    check(v && v->member("n") && v->member("n")->number == 3.0,
          "json: trailing whitespace tolerated");
  }

  // Standard string escapes decoded correctly.
  {
    auto v = parse(R"({"s":"a\"b\\c\/d\n\te\tf\rg"})");
    const std::string &s = v->member("s")->str;
    std::string expected = "a\"b\\c/d\n\te\tf\rg";
    check(s == expected,
          "json: \\\" \\\\ \\/ \\n \\t \\r decoded verbatim");
  }

  // \uXXXX 2-byte UTF-8 (Latin-1 supplement): \u00E9 -> 'é' (0xC3 0xA9).
  {
    auto v = parse(R"({"k":"\u00E9"})");
    const std::string &s = v->member("k")->str;
    check(s == std::string("\xC3\xA9"),
          "json: \\u00E9 -> 2-byte UTF-8 (é)");
  }

  // \uXXXX 3-byte UTF-8 (BMP): \u4E2D -> '中' (0xE4 0xB8 0xAD).
  {
    auto v = parse(R"({"k":"\u4E2D"})");
    const std::string &s = v->member("k")->str;
    check(s == std::string("\xE4\xB8\xAD"),
          "json: \\u4E2D -> 3-byte UTF-8 (中)");
  }

  // Surrogate-pair decoding: \uD83D\uDE00 -> U+1F600 (4-byte UTF-8).
  // This is the real gap in JsonLite's appendUtf8: a lone surrogate or an
  // unpaired high surrogate produces invalid UTF-8; a properly-paired
  // supplementary-plane code point should emit a 4-byte sequence.
  {
    auto v = parse(R"({"k":"\uD83D\uDE00"})");
    const std::string &s = v->member("k")->str;
    std::string expected = "\xF0\x9F\x98\x80"; // U+1F600 GRINNING FACE
    check(s == expected,
          "json: \\uD83D\\uDE00 surrogate pair -> 4-byte UTF-8 (U+1F600)");
    check(s.size() == 4,
          "json: supplementary-plane codepoint encodes to exactly 4 UTF-8 bytes");
  }
}

} // namespace

TEST_CASE("encoding_lex_audit_test") {
  std::cout << "===== AUD-PARSE-LEX: sniff precedence =====\n";
  testSniffPrecedence();
  std::cout << "===== AUD-PARSE-LEX: inflate =====\n";
  testInflate();
  std::cout << "===== AUD-PARSE-LEX: tokenizer =====\n";
  testVrmlTokenizer();
  std::cout << "===== AUD-PARSE-LEX: json =====\n";
  testJsonLite();

  CHECK(failures == 0);
  if (failures == 0) {
    std::cout << "\nALL AUD-PARSE-LEX CHECKS PASSED\n";
    return;
  }
  std::cerr << "\n" << failures << " CHECK(S) FAILED\n";
  return;
}
