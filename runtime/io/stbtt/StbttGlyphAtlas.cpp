// runtime/io/stbtt/StbttGlyphAtlas.cpp — see StbttGlyphAtlas.hpp. Rasterizes a
// Basic Latin + Latin-1 Supplement atlas via stb_truetype and returns it with
// ready FontMetrics (advances + glyph boxes + UVs). stb_truetype.h is included WITHOUT the implementation define
// — StbttFontMetrics.cpp is the one TU that defines STB_TRUETYPE_IMPLEMENTATION;
// this TU links against those symbols within the x3d_stbtt lib. The public
// header stays decoder-free, so no stb header leaks to consumers.
#include "StbttGlyphAtlas.hpp"

#include <algorithm>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "stb_truetype.h"  // declarations only (impl is in StbttFontMetrics.cpp)

namespace x3d::runtime::io::stbtt {
namespace {

using x3d::runtime::extract::FontKey;
using x3d::runtime::extract::FontMetrics;
using x3d::runtime::extract::GlyphMetrics;
using x3d::runtime::extract::GlyphResult;

constexpr std::uint32_t kFirstCp = 0x20;  // space
constexpr std::uint32_t kLastCp = 0xff;   // Latin-1 Supplement end
constexpr int kAtlasWidth = 512;
constexpr int kPad = 2;

bool readFile(const std::string& path, std::vector<unsigned char>& out) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f) return false;
  const std::streamoff size = f.tellg();
  if (size <= 0) return false;
  out.resize(static_cast<std::size_t>(size));
  f.seekg(0);
  return static_cast<bool>(f.read(reinterpret_cast<char*>(out.data()), size));
}

// Raw unitsPerEm (head + 18, big-endian) — same exact-integer path as
// StbttFontMetrics so the atlas's advanceEm bit-matches the metrics backend.
std::uint16_t unitsPerEm(const stbtt_fontinfo& info) {
  const unsigned char* p = info.data + info.head + 18;
  return static_cast<std::uint16_t>((p[0] << 8) | p[1]);
}

// One glyph slated for packing.
struct Cell {
  std::string family;
  std::uint32_t cp = 0;
  int fontIdx = 0;
  float advanceEm = 0.0f;
  float ascent = 0.0f, descent = 0.0f;  // font metrics (em)
  int ix0 = 0, iy0 = 0;  // glyph bbox vs baseline (px, top-down)
  int gw = 0, gh = 0;    // glyph bitmap size (px)
  int x = 0, yTop = 0;   // assigned atlas origin (top-down)
};

}  // namespace

GlyphAtlasResult makeStbttGlyphAtlas(FontFaceMap faces, int emPx) {
  GlyphAtlasResult result;
  if (emPx < 8) emPx = 8;

  // Load every distinct face; keep byte buffers alive for the whole bake (each
  // stbtt_fontinfo points INTO its buffer).
  std::vector<std::vector<unsigned char>> fontBytes;
  std::vector<stbtt_fontinfo> fonts;
  std::vector<std::string> families;
  for (auto& [family, path] : faces) {
    std::vector<unsigned char> bytes;
    if (!readFile(path, bytes)) continue;
    stbtt_fontinfo fi{};
    const int off = stbtt_GetFontOffsetForIndex(bytes.data(), 0);
    if (off < 0 || !stbtt_InitFont(&fi, bytes.data(), off)) continue;
    if (unitsPerEm(fi) == 0) continue;
    fontBytes.push_back(std::move(bytes));
    fonts.push_back(fi);
    families.push_back(family);
  }
  if (fonts.empty()) return result;  // atlas.ok false, fontMetrics null

  // Gather + measure every (family, codepoint) cell.
  std::vector<Cell> cells;
  for (std::size_t f = 0; f < fonts.size(); ++f) {
    stbtt_fontinfo& fi = fonts[f];
    const float scalePx = stbtt_ScaleForMappingEmToPixels(&fi, static_cast<float>(emPx));
    const float upm = static_cast<float>(unitsPerEm(fi));
    int ascent = 0, descent = 0, lineGap = 0;
    stbtt_GetFontVMetrics(&fi, &ascent, &descent, &lineGap);
    for (std::uint32_t cp = kFirstCp; cp <= kLastCp; ++cp) {
      if (cp >= 0x7f && cp < 0xa0) continue;  // DEL + C1 controls have no display cells
      const int glyph = stbtt_FindGlyphIndex(&fi, static_cast<int>(cp));
      if (glyph == 0) continue;  // .notdef → Failed (no cell, no entry)
      int adv = 0, lsb = 0;
      stbtt_GetGlyphHMetrics(&fi, glyph, &adv, &lsb);  // unscaled
      Cell c;
      c.family = families[f];
      c.cp = cp;
      c.fontIdx = static_cast<int>(f);
      c.advanceEm = static_cast<float>(adv) / upm;     // raw int / raw uint16
      c.ascent = static_cast<float>(ascent) / upm;
      c.descent = static_cast<float>(descent) / upm;
      int ix0, iy0, ix1, iy1;
      stbtt_GetGlyphBitmapBox(&fi, glyph, scalePx, scalePx, &ix0, &iy0, &ix1, &iy1);
      c.ix0 = ix0;
      c.iy0 = iy0;
      c.gw = (std::max)(0, ix1 - ix0);
      c.gh = (std::max)(0, iy1 - iy0);
      cells.push_back(c);
    }
  }
  if (cells.empty()) return result;

  // Pack the actual bitmap boxes, not advance-width by one-em cells. Accents
  // can rise above 0.8 em and glyphs can overhang their advance on either side.
  // A transparent border keeps adjacent glyphs separate under linear sampling.
  int atlasW = kAtlasWidth;
  for (const Cell& c : cells) atlasW = (std::max)(atlasW, c.gw + 2 * kPad);
  int x = kPad, yTop = kPad, rowH = 0;
  for (Cell& c : cells) {
    const int cellW = (std::max)(1, c.gw);
    const int cellH = (std::max)(1, c.gh);
    if (x + cellW + kPad > atlasW) {
      x = kPad;
      yTop += rowH + kPad;
      rowH = 0;
    }
    c.x = x;
    c.yTop = yTop;
    x += cellW + kPad;
    rowH = (std::max)(rowH, cellH);
  }
  const int atlasH = yTop + rowH + kPad;

  // Rasterize into a TOP-DOWN buffer, then flip to bottom-up for GL upload.
  std::vector<std::uint8_t> top(static_cast<std::size_t>(atlasW) * atlasH, 0);
  for (const Cell& c : cells) {
    if (c.gw == 0 || c.gh == 0) continue;  // space / blank glyph
    stbtt_fontinfo& fi = fonts[c.fontIdx];
    const float scalePx = stbtt_ScaleForMappingEmToPixels(&fi, static_cast<float>(emPx));
    const int glyph = stbtt_FindGlyphIndex(&fi, static_cast<int>(c.cp));
    stbtt_MakeGlyphBitmap(&fi, top.data() + static_cast<std::size_t>(c.yTop) * atlasW + c.x,
                          c.gw, c.gh, atlasW, scalePx, scalePx, glyph);
  }

  GlyphAtlas& atlas = result.atlas;
  atlas.width = atlasW;
  atlas.height = atlasH;
  atlas.coverage.assign(static_cast<std::size_t>(atlasW) * atlasH, 0);
  for (int r = 0; r < atlasH; ++r) {  // vertical flip: top-down -> bottom-up
    std::copy_n(top.data() + static_cast<std::size_t>(atlasH - 1 - r) * atlasW,
                atlasW, atlas.coverage.data() + static_cast<std::size_t>(r) * atlasW);
  }
  atlas.ok = true;

  // Per-(family, codepoint) GlyphMetrics with bottom-up UVs.
  auto table =
      std::make_shared<std::map<std::string, std::map<std::uint32_t, GlyphMetrics>>>();
  const float W = static_cast<float>(atlasW), H = static_cast<float>(atlasH);
  for (const Cell& c : cells) {
    GlyphMetrics gm;
    gm.advanceEm = c.advanceEm;
    gm.hasAtlasUv = true;
    gm.u0 = static_cast<float>(c.x) / W;
    gm.u1 = static_cast<float>(c.x + c.gw) / W;
    gm.v0 = 1.0f - static_cast<float>(c.yTop + c.gh) / H;
    gm.v1 = 1.0f - static_cast<float>(c.yTop) / H;
    // Match the rasterized rectangle (including fractional-outline edge pixels)
    // to its true baseline-relative position. Blank glyphs keep their advance
    // but have zero-size boxes/UV rectangles and therefore contribute no ink.
    const float pxPerEm = static_cast<float>(emPx);
    gm.bearingX = static_cast<float>(c.ix0) / pxPerEm;
    gm.top = static_cast<float>(-c.iy0) / pxPerEm;
    gm.sizeX = static_cast<float>(c.gw) / pxPerEm;
    gm.sizeY = static_cast<float>(c.gh) / pxPerEm;
    gm.ascent = c.ascent;
    gm.descent = c.descent;
    gm.hasGlyphBox = true;
    (*table)[c.family][c.cp] = gm;
  }

  result.fontMetrics = [table](const FontKey& k) -> GlyphResult {
    const auto fit = table->find(k.family);
    if (fit == table->end()) return GlyphResult::makeFailed();
    const auto git = fit->second.find(k.codepoint);
    if (git == fit->second.end()) return GlyphResult::makeFailed();
    return GlyphResult::makeReady(git->second);  // style ignored (PoC: all PLAIN)
  };
  return result;
}

}  // namespace x3d::runtime::io::stbtt
