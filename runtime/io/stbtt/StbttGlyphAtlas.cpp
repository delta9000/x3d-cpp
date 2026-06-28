// runtime/io/stbtt/StbttGlyphAtlas.cpp — see StbttGlyphAtlas.hpp. Rasterizes an
// ASCII glyph atlas via stb_truetype and returns it with a ready FontMetrics
// (advances + UVs). stb_truetype.h is included WITHOUT the implementation define
// — StbttFontMetrics.cpp is the one TU that defines STB_TRUETYPE_IMPLEMENTATION;
// this TU links against those symbols within the x3d_stbtt lib. The public
// header stays decoder-free, so no stb header leaks to consumers.
#include "StbttGlyphAtlas.hpp"

#include <algorithm>
#include <cmath>
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

constexpr std::uint32_t kFirstCp = 32;   // space
constexpr std::uint32_t kLastCp = 126;   // tilde
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
  int cw = 0;            // cell width (px) = round(advanceEm * emPx)
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

  const int baselineFromTop = static_cast<int>(std::lround(0.8f * emPx));

  // Gather + measure every (family, codepoint) cell.
  std::vector<Cell> cells;
  for (std::size_t f = 0; f < fonts.size(); ++f) {
    stbtt_fontinfo& fi = fonts[f];
    const float scalePx = stbtt_ScaleForMappingEmToPixels(&fi, static_cast<float>(emPx));
    const float upm = static_cast<float>(unitsPerEm(fi));
    for (std::uint32_t cp = kFirstCp; cp <= kLastCp; ++cp) {
      const int glyph = stbtt_FindGlyphIndex(&fi, static_cast<int>(cp));
      if (glyph == 0) continue;  // .notdef → Failed (no cell, no entry)
      int adv = 0, lsb = 0;
      stbtt_GetGlyphHMetrics(&fi, glyph, &adv, &lsb);  // unscaled
      Cell c;
      c.family = families[f];
      c.cp = cp;
      c.fontIdx = static_cast<int>(f);
      c.advanceEm = static_cast<float>(adv) / upm;     // raw int / raw uint16
      c.cw = (std::max)(1, static_cast<int>(std::lround(c.advanceEm * emPx)));
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

  // Shelf-pack into a fixed-width atlas; each row is `emPx` tall (one em cell).
  int x = kPad, yTop = kPad, atlasH = 0;
  for (Cell& c : cells) {
    if (x + c.cw + kPad > kAtlasWidth) {
      x = kPad;
      yTop += emPx + kPad;
    }
    c.x = x;
    c.yTop = yTop;
    x += c.cw + kPad;
    atlasH = (std::max)(atlasH, yTop + emPx + kPad);
  }

  // Rasterize into a TOP-DOWN buffer, then flip to bottom-up for GL upload.
  std::vector<std::uint8_t> top(static_cast<std::size_t>(kAtlasWidth) * atlasH, 0);
  for (const Cell& c : cells) {
    if (c.gw == 0 || c.gh == 0) continue;  // space / blank glyph
    const int baseline = c.yTop + baselineFromTop;
    int gx = c.x + (std::max)(0, c.ix0);
    int gy = baseline + c.iy0;
    if (gx < 0) gx = 0;
    if (gy < 0) gy = 0;
    if (gx + c.gw > kAtlasWidth || gy + c.gh > atlasH) continue;  // clip-safe
    stbtt_fontinfo& fi = fonts[c.fontIdx];
    const float scalePx = stbtt_ScaleForMappingEmToPixels(&fi, static_cast<float>(emPx));
    const int glyph = stbtt_FindGlyphIndex(&fi, static_cast<int>(c.cp));
    stbtt_MakeGlyphBitmap(&fi, top.data() + static_cast<std::size_t>(gy) * kAtlasWidth + gx,
                          c.gw, c.gh, kAtlasWidth, scalePx, scalePx, glyph);
  }

  GlyphAtlas& atlas = result.atlas;
  atlas.width = kAtlasWidth;
  atlas.height = atlasH;
  atlas.coverage.assign(static_cast<std::size_t>(kAtlasWidth) * atlasH, 0);
  for (int r = 0; r < atlasH; ++r) {  // vertical flip: top-down -> bottom-up
    std::copy_n(top.data() + static_cast<std::size_t>(atlasH - 1 - r) * kAtlasWidth,
                kAtlasWidth, atlas.coverage.data() + static_cast<std::size_t>(r) * kAtlasWidth);
  }
  atlas.ok = true;

  // Per-(family, codepoint) GlyphMetrics with bottom-up UVs.
  auto table =
      std::make_shared<std::map<std::string, std::map<std::uint32_t, GlyphMetrics>>>();
  const float W = static_cast<float>(kAtlasWidth), H = static_cast<float>(atlasH);
  for (const Cell& c : cells) {
    GlyphMetrics gm;
    gm.advanceEm = c.advanceEm;
    gm.hasAtlasUv = true;
    gm.u0 = static_cast<float>(c.x) / W;
    gm.u1 = static_cast<float>(c.x + c.cw) / W;
    gm.v0 = 1.0f - static_cast<float>(c.yTop + emPx) / H;  // descender (bottom)
    gm.v1 = 1.0f - static_cast<float>(c.yTop) / H;         // ascender (top)
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
