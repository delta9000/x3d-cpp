// BuiltinFont.hpp — a built-in glyph atlas + FontMetrics callback so the CPU
// rasterizer renders Text nodes as READABLE LETTERS without an embedder-supplied
// font.
//
// WHY: the SDK's default FontMetrics is makeMonospaceStub() — metrics only,
// hasAtlasUv=false. TextExtract still emits one quad per glyph, but with no atlas
// UVs every glyph cell fills solid, so text renders as bars, not letters. The
// embedder is expected to supply a real FontMetrics (atlas UVs) + the matching
// atlas texture. This provides exactly that, using a vendored public-domain 8x8
// font (font8x8.hpp), so the headless rasterizer shows real text out of the box.
//
// Pipeline: makeBuiltinFont() builds (a) an RGBA atlas (white ink, transparent
// background; 16x8 grid of 8x8 cells = 128x64) and (b) a FontMetrics lambda whose
// per-codepoint UVs index that atlas. Pass the metrics to the extractor via
// MeshBuildOptions.fontMetrics; bind the atlas in the glyph draw path (the quads
// then sample real letter shapes — see SceneRender.hpp).
//
// Out-of-SDK consumer code. namespace x3d::cpuraster.
#ifndef X3D_CPURASTER_BUILTIN_FONT_HPP
#define X3D_CPURASTER_BUILTIN_FONT_HPP

#include "FontMetrics.hpp" // x3d::runtime::extract::FontMetrics / FontKey / Glyph*
#include "Texture.hpp"
#include "font8x8.hpp"

#include <cstdint>
#include <vector>

namespace x3d::cpuraster {

struct BuiltinFont {
  static constexpr int kCols = 16, kRows = 8, kCell = 8;
  static constexpr int kW = kCols * kCell; // 128
  static constexpr int kH = kRows * kCell; // 64
  Texture atlas;
  x3d::runtime::extract::FontMetrics metrics;
};

// advanceEm matches the SDK stub's 0.6 so glyph PLACEMENT is unchanged; only the
// fill (solid cell -> real glyph) differs. The 8x8 glyphs carry their own
// left/right padding, so 0.6 advance yields readable inter-letter spacing.
inline BuiltinFont makeBuiltinFont(float advanceEm = 0.6f) {
  using namespace x3d::runtime::extract;
  constexpr int C = BuiltinFont::kCols, R = BuiltinFont::kRows, S = BuiltinFont::kCell;
  constexpr int W = BuiltinFont::kW, H = BuiltinFont::kH;

  // Atlas is bottom-up (GL origin), matching Texture + the glyph quad UVs
  // (emitQuad writes (u0,v0) at the bottom-left corner). Grid row 0 sits at the
  // TOP (high v); within a cell, the glyph's top row (byte 0) is at the top.
  std::vector<std::uint8_t> rgba(static_cast<std::size_t>(W) * H * 4, 0);
  for (int cp = 0; cp < 128; ++cp) {
    const int col = cp % C, gridRow = cp / C;
    const int cellBottom = (R - 1 - gridRow) * S; // bottom-up data row of cell bottom.
    for (int lyr = 0; lyr < S; ++lyr) {           // 0 = bottom row of the cell.
      const std::uint8_t bits = kFont8x8[cp][S - 1 - lyr]; // byte 0 = top row.
      const int dataY = cellBottom + lyr;
      for (int lx = 0; lx < S; ++lx) {
        if (bits & (1u << lx)) {                  // bit 0 = leftmost column.
          const std::size_t i =
              (static_cast<std::size_t>(dataY) * W + (col * S + lx)) * 4;
          rgba[i] = rgba[i + 1] = rgba[i + 2] = 255;
          rgba[i + 3] = 255;
        }
      }
    }
  }

  BuiltinFont f;
  f.atlas = Texture::fromRGBA8(rgba.data(), W, H, /*repeatS=*/false,
                               /*repeatT=*/false, /*srgb=*/false);
  f.metrics = [advanceEm](const FontKey &k) -> GlyphResult {
    std::uint32_t cp = k.codepoint;
    if (cp >= 128) cp = 63; // map non-ASCII to '?'.
    const int col = static_cast<int>(cp) % C, gridRow = static_cast<int>(cp) / C;
    const int cellBottom = (R - 1 - gridRow) * S;
    GlyphMetrics m;
    m.advanceEm = advanceEm;
    m.hasAtlasUv = true;
    m.u0 = (col * S) / float(W);
    m.u1 = (col * S + S) / float(W);
    m.v0 = cellBottom / float(H);
    m.v1 = (cellBottom + S) / float(H);
    return GlyphResult::makeReady(m);
  };
  return f;
}

} // namespace x3d::cpuraster

#endif // X3D_CPURASTER_BUILTIN_FONT_HPP
