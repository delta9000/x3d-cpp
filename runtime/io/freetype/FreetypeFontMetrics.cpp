// runtime/io/freetype/FreetypeFontMetrics.cpp — the ONE translation unit where
// FreeType meets the FontMetrics seam. <ft2build.h> is included PRIVATE (see
// CMake) so it never leaks. The factory returns a std::function exchanging only
// std types, so linking x3d_freetype pulls in no FreeType headers.
#include "FreetypeFontMetrics.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ADVANCES_H

namespace x3d::runtime::io::freetype {
namespace {

// Owns the FT_Library + lazily-opened faces; tears them down on destruction.
struct State {
  FontFaceMap faces;
  FT_Library lib = nullptr;
  std::unordered_map<std::string, FT_Face> cache;  // path -> face (nullptr = failed)

  ~State() {
    for (auto& kv : cache)
      if (kv.second) FT_Done_Face(kv.second);
    if (lib) FT_Done_FreeType(lib);
  }

  FT_Face faceFor(const std::string& family) {
    const auto it = faces.find(family);
    if (it == faces.end()) return nullptr;
    const std::string& path = it->second;
    auto cached = cache.find(path);
    if (cached != cache.end()) return cached->second;

    FT_Face face = nullptr;
    if (FT_New_Face(lib, path.c_str(), 0, &face) != 0) face = nullptr;
    cache.emplace(path, face);
    return face;
  }
};

}  // namespace

x3d::runtime::extract::FontMetrics makeFreetypeFontMetrics(FontFaceMap faces) {
  using x3d::runtime::extract::GlyphMetrics;
  using x3d::runtime::extract::GlyphResult;
  using x3d::runtime::extract::FontKey;

  auto state = std::make_shared<State>();
  state->faces = std::move(faces);
  if (FT_Init_FreeType(&state->lib) != 0) state->lib = nullptr;

  return [state](const FontKey& key) -> GlyphResult {
    if (!state->lib) return GlyphResult::makeFailed();
    if (!key.style.empty() && key.style != "PLAIN") return GlyphResult::makeFailed();

    FT_Face face = state->faceFor(key.family);
    if (!face) return GlyphResult::makeFailed();

    const FT_UInt glyph = FT_Get_Char_Index(face, key.codepoint);
    if (glyph == 0) return GlyphResult::makeFailed();  // .notdef → Failed (rule 3)

    FT_Fixed advance = 0;  // font units when FT_LOAD_NO_SCALE is set (rule 1)
    if (FT_Get_Advance(face, glyph, FT_LOAD_NO_SCALE, &advance) != 0)
      return GlyphResult::makeFailed();

    const std::uint16_t upm = static_cast<std::uint16_t>(face->units_per_EM);  // raw (rule 2)
    if (upm == 0) return GlyphResult::makeFailed();

    const float advanceEm = static_cast<float>(advance) / static_cast<float>(upm);
    return GlyphResult::makeReady(GlyphMetrics{advanceEm, false, 0.f, 0.f, 0.f, 0.f});
  };
}

}  // namespace x3d::runtime::io::freetype
