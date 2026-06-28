// runtime/io/stbtt/StbttGlyphAtlas.hpp — consumer-side glyph atlas for the
// T-TEXT seam (Backend A, stb_truetype). Companion to StbttFontMetrics.hpp.
//
// WHY THIS LIVES HERE, NOT IN CORE: the core SDK (x3d_cpp / sdk.hpp) is headless
// and renderer-agnostic and NEVER rasterizes — FontMetrics.hpp deliberately
// returns advances only (hasAtlasUv=false) and documents the atlas/raster path
// as "embedder-defined". But making every GL/CPU renderer re-vendor stb_truetype
// and re-derive advances to bake an atlas is wasteful duplication. So the
// rasterizing helper lives in the SAME isolated io module that already owns
// stb_truetype (x3d_stbtt, default OFF, decoder-free public header) — one font
// pipeline, reusable by any consumer (poc_renderer, cpu_raster), with core left
// pure. This header is decoder-free: it exchanges only std types + the seam, so
// linking x3d_stbtt pulls in no stb headers.
#ifndef X3D_RUNTIME_IO_STBTT_STBTT_GLYPH_ATLAS_HPP
#define X3D_RUNTIME_IO_STBTT_STBTT_GLYPH_ATLAS_HPP

#include "FontMetrics.hpp"       // x3d::runtime::extract::FontMetrics
#include "StbttFontMetrics.hpp"  // FontFaceMap

#include <cstdint>
#include <vector>

namespace x3d::runtime::io::stbtt {

// Single-channel coverage atlas, rows BOTTOM-UP (row 0 = bottom) so a consumer
// uploads it via glTexImage2D(GL_R8, ...) with NO vertical flip and the UVs the
// companion FontMetrics returns use the GL bottom-left origin the seam documents.
struct GlyphAtlas {
  int width = 0;
  int height = 0;
  std::vector<std::uint8_t> coverage;  // width*height, 0..255, row 0 = bottom.
  bool ok = false;
};

struct GlyphAtlasResult {
  GlyphAtlas atlas;
  // FontMetrics that returns the SAME advanceEm as makeStbttFontMetrics (raw
  // hmtx advance / raw unitsPerEm) PLUS per-glyph atlas UVs (hasAtlasUv=true).
  // Null if no face in `faces` could be loaded (atlas.ok is then false too).
  x3d::runtime::extract::FontMetrics fontMetrics;
};

/// Bake an ASCII (codepoints 32..126) coverage atlas for every family in `faces`
/// (family -> TTF/OTF path; all families packed into one atlas) and return it
/// alongside a ready FontMetrics. `emPx` is the rasterized cell height (one em)
/// in pixels. Out-of-range codepoints, unmapped families, and .notdef glyphs
/// resolve to Failed (the layout engine skips them). v1: style is ignored (all
/// faces treated as PLAIN) — the atlas/raster path is outside the cross-backend
/// equality surface (FontMetrics.hpp adapter contract item 4).
GlyphAtlasResult makeStbttGlyphAtlas(FontFaceMap faces, int emPx = 64);

}  // namespace x3d::runtime::io::stbtt

#endif  // X3D_RUNTIME_IO_STBTT_STBTT_GLYPH_ATLAS_HPP
