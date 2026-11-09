// Glyph metrics and failure handling with stb_truetype; see ADR-0025.
#include "FontMetrics.hpp"
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

  CHECK((fm(FontKey{"SANS", "PLAIN", kAbsentEmoji}).failed()));   //.notdef → Failed
  CHECK((fm(FontKey{"UNMAPPED", "PLAIN", 'A'}).failed()));        // family not in map
  CHECK((fm(FontKey{"SANS", "BOLD", 'A'}).failed()));            // style out of scope
}
