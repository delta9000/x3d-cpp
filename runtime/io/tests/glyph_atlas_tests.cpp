// The optional raster helper must supply actual, unclipped glyph coverage,
// not just a Ready status. This oracle rasterizes each fixture glyph separately
// and compares it to the atlas rectangle, including its bottom-up orientation.
#include "StbttFontMetrics.hpp"
#include "StbttGlyphAtlas.hpp"
#include "TextExtract.hpp"
#include "x3d/nodes/Text.hpp"
#include "doctest/doctest.h"
#include "stb_truetype.h" // declarations only; test-private raster oracle

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using namespace x3d::runtime::extract;
using namespace x3d::runtime::io::stbtt;

namespace {
FontFaceMap faces() {
  const std::string dir = FIXTURES_DIR;
  return {{"SERIF", dir + "/LiberationSerif-Regular.ttf"},
          {"SANS", dir + "/LiberationSans-Regular.ttf"},
          {"TYPEWRITER", dir + "/LiberationMono-Regular.ttf"}};
}

bool supported(std::uint32_t cp) {
  return (cp >= 0x20 && cp <= 0x7e) || (cp >= 0xa0 && cp <= 0xff);
}

std::size_t inkCount(const std::vector<std::uint8_t>& bitmap) {
  return std::count_if(bitmap.begin(), bitmap.end(), [](auto p) { return p != 0; });
}
} // namespace

TEST_CASE("glyph_atlas_latin1_coverage_and_metrics") {
  const auto fontFaces = faces();
  const auto metrics = makeStbttFontMetrics(fontFaces);
  for (const int requestedEm : {0, 17, 64}) {
    const int emPx = std::max(8, requestedEm);
    CAPTURE(emPx);
    const auto result = makeStbttGlyphAtlas(fontFaces, requestedEm);
    REQUIRE(result.atlas.ok);
    REQUIRE(static_cast<bool>(result.fontMetrics));
    const auto& atlas = result.atlas;
    REQUIRE(atlas.coverage.size() == static_cast<std::size_t>(atlas.width) * atlas.height);
    std::size_t totalInk = 0;
    for (const auto& [family, path] : fontFaces) {
      CAPTURE(family);
      std::ifstream file(path, std::ios::binary);
      const std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(file), {}};
      REQUIRE(!bytes.empty());
      stbtt_fontinfo info{};
      REQUIRE(stbtt_InitFont(&info, bytes.data(), stbtt_GetFontOffsetForIndex(bytes.data(), 0)));
      const float scale = stbtt_ScaleForMappingEmToPixels(&info, static_cast<float>(emPx));
      for (std::uint32_t cp = 0x20; cp <= 0xff; ++cp) {
        if (!supported(cp)) continue;
        CAPTURE(cp);
        const auto reference = metrics({family, "PLAIN", cp});
        const auto glyph = result.fontMetrics({family, "PLAIN", cp});
        REQUIRE(reference.ready()); // all of this repertoire exists in the fixtures
        REQUIRE(glyph.ready());
        const auto& gm = glyph.metrics;
        CHECK(gm.advanceEm == reference.metrics.advanceEm);
        CHECK(gm.hasAtlasUv);
        CHECK(gm.hasGlyphBox);
        CHECK(gm.ascent == reference.metrics.ascent);
        CHECK(gm.descent == reference.metrics.descent);
        CHECK(gm.u0 >= 0.0f);
        CHECK(gm.v0 >= 0.0f);
        CHECK(gm.u1 <= 1.0f);
        CHECK(gm.v1 <= 1.0f);
        int x0, y0, x1, y1;
        const int index = stbtt_FindGlyphIndex(&info, static_cast<int>(cp));
        REQUIRE(index != 0);
        stbtt_GetGlyphBitmapBox(&info, index, scale, scale, &x0, &y0, &x1, &y1);
        const int width = x1 - x0, height = y1 - y0;
        // Raster bounds include the fractional outline's edge pixels. Geometry
        // must map that same pixel rectangle back into em units without stretch.
        CHECK(gm.bearingX == static_cast<float>(x0) / emPx);
        CHECK(gm.top == static_cast<float>(-y0) / emPx);
        CHECK(gm.sizeX == static_cast<float>(width) / emPx);
        CHECK(gm.sizeY == static_cast<float>(height) / emPx);
        const int left = static_cast<int>(std::lround(gm.u0 * atlas.width));
        const int bottom = static_cast<int>(std::lround(gm.v0 * atlas.height));
        CHECK(static_cast<int>(std::lround(gm.u1 * atlas.width)) - left == width);
        CHECK(static_cast<int>(std::lround(gm.v1 * atlas.height)) - bottom == height);
        REQUIRE(left >= 0);
        REQUIRE(bottom >= 0);
        REQUIRE(left + width <= atlas.width);
        REQUIRE(bottom + height <= atlas.height);
        if (width == 0 || height == 0) {
          CHECK((cp == 0x20 || cp == 0xa0)); // space and non-breaking space
          CHECK(gm.advanceEm > 0.0f);
          continue;
        }
        std::vector<std::uint8_t> expected(static_cast<std::size_t>(width) * height);
        stbtt_MakeGlyphBitmap(&info, expected.data(), width, height, width, scale, scale, index);
        const std::size_t ink = inkCount(expected);
        CHECK(ink > 0);
        totalInk += ink;
        bool pixelsEqual = true;
        for (int row = 0; row < height; ++row)
          for (int col = 0; col < width; ++col)
            pixelsEqual &= expected[static_cast<std::size_t>(row) * width + col] ==
                atlas.coverage[static_cast<std::size_t>(bottom + height - 1 - row) * atlas.width + left + col];
        CHECK(pixelsEqual);
      }
    }
    // No glyph collisions, lost pixels, or stray ink outside the glyph rectangles.
    CHECK(inkCount(atlas.coverage) == totalInk);
  }
}

TEST_CASE("glyph_atlas_bounded_repertoire_and_failures") {
  const auto result = makeStbttGlyphAtlas(faces());
  REQUIRE(result.atlas.ok);
  for (const std::uint32_t cp : {0x20u, 0x7eu, 0xa0u, 0xa1u, 0xe9u, 0xffu}) {
    CAPTURE(cp);
    CHECK(result.fontMetrics({"SERIF", "PLAIN", cp}).ready());
  }
  for (const std::uint32_t cp : {0u, 0x1fu, 0x7fu, 0x80u, 0x9fu, 0x100u, 0x1f600u}) {
    CAPTURE(cp);
    CHECK(result.fontMetrics({"SERIF", "PLAIN", cp}).failed());
  }
  CHECK(result.fontMetrics({"UNMAPPED", "PLAIN", 'A'}).failed());
  // Preserve the helper's documented style fallback; this is not a style renderer.
  CHECK(result.fontMetrics({"SERIF", "BOLD", 0xe9}).ready());
  const auto empty = makeStbttGlyphAtlas({});
  CHECK_FALSE(empty.atlas.ok);
  CHECK_FALSE(static_cast<bool>(empty.fontMetrics));
  const auto missing = makeStbttGlyphAtlas({{"SERIF", "no-such-font.ttf"}});
  CHECK_FALSE(missing.atlas.ok);
  CHECK_FALSE(static_cast<bool>(missing.fontMetrics));
  CHECK(missing.atlas.coverage.empty());
}

TEST_CASE("glyph_atlas_utf8_text_geometry") {
  const auto result = makeStbttGlyphAtlas(faces());
  REQUIRE(result.atlas.ok);
  x3d::nodes::Text text;
  text.setString({"A\xc3\xa9\xc3\x85\xc3\xbf\xc2\xa0Z"}); // AéÅÿ NBSP Z
  TextLayoutResult layout;
  const auto mesh = buildTextMesh(text, result.fontMetrics, &layout);
  const std::uint32_t codepoints[] = {'A', 0xe9, 0xc5, 0xff, 0xa0, 'Z'};
  REQUIRE(mesh.positions.size() == 4 * std::size(codepoints));
  REQUIRE(mesh.texcoords.size() == mesh.positions.size());
  REQUIRE(mesh.indices.size() == 6 * std::size(codepoints));
  REQUIRE(layout.lineBaselineOrigins.size() == 1);
  float pen = layout.lineBaselineOrigins[0][0];
  const float baseline = layout.lineBaselineOrigins[0][1];
  for (std::size_t i = 0; i < std::size(codepoints); ++i) {
    CAPTURE(codepoints[i]);
    const auto glyph = result.fontMetrics({"SERIF", "PLAIN", codepoints[i]});
    REQUIRE(glyph.ready());
    const auto& gm = glyph.metrics;
    const auto& bottomLeft = mesh.positions[4 * i];
    const auto& topRight = mesh.positions[4 * i + 2];
    CHECK(bottomLeft.x == doctest::Approx(pen + gm.bearingX));
    CHECK(bottomLeft.y == doctest::Approx(baseline + gm.top - gm.sizeY));
    CHECK(topRight.x == doctest::Approx(pen + gm.bearingX + gm.sizeX));
    CHECK(topRight.y == doctest::Approx(baseline + gm.top));
    CHECK(mesh.texcoords[4 * i].x == gm.u0);
    CHECK(mesh.texcoords[4 * i].y == gm.v0);
    CHECK(mesh.texcoords[4 * i + 2].x == gm.u1);
    CHECK(mesh.texcoords[4 * i + 2].y == gm.v1);
    pen += gm.advanceEm;
  }
}
