// runtime/io/tests/font_metrics_tests.cpp — the FontMetrics seam genericity
// proof (ADR-0025), one grouped doctest binary.
//
//   per-backend: stb_truetype reads advances + rejects bad input
//   per-backend: FreeType reads advances + rejects bad input   (added Task 3)
//   swap-test:   stbtt vs FreeType return EXACT-equal advanceEm,
//                plus Failed-parity on absent codepoints         (added Task 4)
//
// ABI ISOLATION (the AssetResolver lesson, ADR-0023): this TU talks ONLY through
// the seam factories and includes NO backend headers' heavy deps. The factories
// exchange std types, so no heavy backend header can leak in.
#include "FontMetrics.hpp"
#include "FreetypeFontMetrics.hpp"
#include "StbttFontMetrics.hpp"
#include "doctest/doctest.h"

#include <map>
#include <string>

using namespace x3d::runtime::extract;

namespace {

std::string fontDir() { return std::string(FIXTURES_DIR); }

std::map<std::string, std::string> liberationFaces() {
  return {
      {"SERIF", fontDir() + "/LiberationSerif-Regular.ttf"},
      {"SANS", fontDir() + "/LiberationSans-Regular.ttf"},
      {"TYPEWRITER", fontDir() + "/LiberationMono-Regular.ttf"},
  };
}

// Codepoints present in Liberation's cmap, and one guaranteed absent.
constexpr std::uint32_t kPresent[] = {'A', 'i', 'M', ' ', '0'};
constexpr std::uint32_t kAbsentEmoji = 0x1F600;  // 😀 — not in Liberation.

}  // namespace

TEST_CASE("fontmetrics_backend_a_stbtt") {
  FontMetrics fm = x3d::runtime::io::stbtt::makeStbttFontMetrics(liberationFaces());
  CHECK((static_cast<bool>(fm)));

  for (std::uint32_t cp : kPresent) {
    CAPTURE(cp);
    const GlyphResult g = fm(FontKey{"SANS", "PLAIN", cp});
    CHECK((g.ready()));
    CHECK((g.metrics.advanceEm > 0.0f));
    CHECK((g.metrics.advanceEm < 2.0f));
    CHECK((g.metrics.hasAtlasUv == false));
  }

  CHECK((fm(FontKey{"SANS", "PLAIN", kAbsentEmoji}).failed()));   // .notdef → Failed
  CHECK((fm(FontKey{"UNMAPPED", "PLAIN", 'A'}).failed()));        // family not in map
  CHECK((fm(FontKey{"SANS", "BOLD", 'A'}).failed()));            // style out of scope
}

TEST_CASE("fontmetrics_backend_b_freetype") {
  FontMetrics fm = x3d::runtime::io::freetype::makeFreetypeFontMetrics(liberationFaces());
  CHECK((static_cast<bool>(fm)));

  for (std::uint32_t cp : kPresent) {
    CAPTURE(cp);
    const GlyphResult g = fm(FontKey{"SANS", "PLAIN", cp});
    CHECK((g.ready()));
    CHECK((g.metrics.advanceEm > 0.0f));
    CHECK((g.metrics.advanceEm < 2.0f));
    CHECK((g.metrics.hasAtlasUv == false));
  }

  CHECK((fm(FontKey{"SANS", "PLAIN", kAbsentEmoji}).failed()));
  CHECK((fm(FontKey{"UNMAPPED", "PLAIN", 'A'}).failed()));
  CHECK((fm(FontKey{"SANS", "BOLD", 'A'}).failed()));
}

TEST_CASE("fontmetrics_swap_advance_exact_equal") {
  FontMetrics a = x3d::runtime::io::stbtt::makeStbttFontMetrics(liberationFaces());
  FontMetrics b = x3d::runtime::io::freetype::makeFreetypeFontMetrics(liberationFaces());

  const char* families[] = {"SERIF", "SANS", "TYPEWRITER"};
  for (const char* fam : families) {
    for (std::uint32_t cp : kPresent) {
      CAPTURE(fam);
      CAPTURE(cp);
      const GlyphResult ga = a(FontKey{fam, "PLAIN", cp});
      const GlyphResult gb = b(FontKey{fam, "PLAIN", cp});
      CHECK((ga.ready()));
      CHECK((gb.ready()));
      // The proof: two independent libraries, bit-identical advanceEm.
      CHECK((ga.metrics.advanceEm == gb.metrics.advanceEm));
    }
  }
}

TEST_CASE("fontmetrics_swap_failed_parity") {
  FontMetrics a = x3d::runtime::io::stbtt::makeStbttFontMetrics(liberationFaces());
  FontMetrics b = x3d::runtime::io::freetype::makeFreetypeFontMetrics(liberationFaces());

  // Absent codepoint → glyph 0 → Failed on BOTH (the .notdef contract rule).
  CHECK((a(FontKey{"SANS", "PLAIN", kAbsentEmoji}).failed()));
  CHECK((b(FontKey{"SANS", "PLAIN", kAbsentEmoji}).failed()));

  // Unmapped family → Failed on both.
  CHECK((a(FontKey{"UNMAPPED", "PLAIN", 'A'}).failed()));
  CHECK((b(FontKey{"UNMAPPED", "PLAIN", 'A'}).failed()));

  // Out-of-scope style → Failed on both (synthetic BOLD/ITALIC not in v1).
  CHECK((a(FontKey{"SANS", "BOLD", 'A'}).failed()));
  CHECK((b(FontKey{"SANS", "BOLD", 'A'}).failed()));
}
