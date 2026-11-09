// Glyph rasterization and atlas construction outside the core SDK.
// Font library headers stay private to the adapter; see ADR-0025.
#ifndef X3D_RUNTIME_IO_STBTT_STBTT_GLYPH_ATLAS_HPP
#define X3D_RUNTIME_IO_STBTT_STBTT_GLYPH_ATLAS_HPP

#include "FontMetrics.hpp"       // x3d::runtime::extract::FontMetrics
#include "StbttFontMetrics.hpp"  // FontFaceMap

#include <cstdint>
#include <vector>

namespace x3d::runtime::io::stbtt {

// Single-channel coverage atlas, rows bottom-up (row 0 = bottom) so a consumer
// uploads it via glTexImage2D(GL_R8,...) with no vertical flip and the UVs the
// companion FontMetrics returns use the GL bottom-left origin the seam documents.
struct GlyphAtlas {
  int width = 0;
  int height = 0;
  std::vector<std::uint8_t> coverage;  // width*height, 0..255, row 0 = bottom.
  bool ok = false;
};

struct GlyphAtlasResult {
  GlyphAtlas atlas;
  // FontMetrics that returns the same advanceEm as makeStbttFontMetrics (raw
  // hmtx advance / raw unitsPerEm) plus per-glyph atlas UVs (hasAtlasUv=true).
  // Null if no face in `faces` could be loaded (atlas.ok is then false too).
  x3d::runtime::extract::FontMetrics fontMetrics;
};

/// Bake Basic Latin (U+0020..U+007E) and Latin-1 Supplement (U+00A0..U+00FF)
/// coverage for every family in `faces`
/// (family -> TTF/OTF path; all families packed into one atlas) and return it
/// alongside a ready FontMetrics. `emPx` is the raster resolution in pixels per
/// em (minimum 8). Glyph boxes/UVs enclose each complete bitmap, including
/// accents and overhangs; advances retain the exact raw-font metric. DEL/C1
/// controls, out-of-range codepoints, unmapped families, and.notdef glyphs
/// resolve to Failed (the layout engine skips them). Style is ignored (all
/// faces treated as PLAIN) — the atlas/raster path is outside the cross-backend
/// equality surface (FontMetrics.hpp adapter contract item 4).
GlyphAtlasResult makeStbttGlyphAtlas(FontFaceMap faces, int emPx = 64);

}  // namespace x3d::runtime::io::stbtt

#endif  // X3D_RUNTIME_IO_STBTT_STBTT_GLYPH_ATLAS_HPP
