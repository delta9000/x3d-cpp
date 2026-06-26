// runtime/io/stbtt/StbttFontMetrics.cpp — the ONE translation unit where
// stb_truetype meets the FontMetrics seam. STB_TRUETYPE_IMPLEMENTATION is
// defined here and nowhere else; stb_truetype.h is included PRIVATE (see CMake)
// so it never leaks. The factory returns a std::function exchanging only std
// types, so linking x3d_stbtt pulls in no stb headers.
#include "StbttFontMetrics.hpp"

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

namespace x3d::runtime::io::stbtt {
namespace {

bool readFile(const std::string& path, std::vector<unsigned char>& out) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f) return false;
  const std::streamoff size = f.tellg();
  if (size <= 0) return false;
  out.resize(static_cast<std::size_t>(size));
  f.seekg(0);
  return static_cast<bool>(f.read(reinterpret_cast<char*>(out.data()), size));
}

// One initialized face: the byte buffer (stbtt_fontinfo points into it, so it
// must outlive the info) + the parsed font info.
struct Face {
  std::vector<unsigned char> bytes;
  stbtt_fontinfo info{};
  bool ok = false;
};

// Lazily load + init the face for a family path; cache by path.
struct State {
  FontFaceMap faces;
  std::unordered_map<std::string, std::shared_ptr<Face>> cache;

  std::shared_ptr<Face> faceFor(const std::string& family) {
    const auto it = faces.find(family);
    if (it == faces.end()) return nullptr;
    const std::string& path = it->second;
    auto cached = cache.find(path);
    if (cached != cache.end()) return cached->second;

    auto face = std::make_shared<Face>();
    if (readFile(path, face->bytes)) {
      const int off = stbtt_GetFontOffsetForIndex(face->bytes.data(), 0);
      if (off >= 0 && stbtt_InitFont(&face->info, face->bytes.data(), off)) {
        face->ok = true;
      }
    }
    cache.emplace(path, face);
    return face;
  }
};

// Raw unitsPerEm: the uint16 at head + 18 (big-endian). info.head is the public
// byte offset of the 'head' table within info.data. Reading it directly (rather
// than 1/stbtt_ScaleForMappingEmToPixels) keeps the value an exact integer.
std::uint16_t unitsPerEm(const stbtt_fontinfo& info) {
  const unsigned char* p = info.data + info.head + 18;
  return static_cast<std::uint16_t>((p[0] << 8) | p[1]);
}

}  // namespace

x3d::runtime::extract::FontMetrics makeStbttFontMetrics(FontFaceMap faces) {
  using x3d::runtime::extract::GlyphMetrics;
  using x3d::runtime::extract::GlyphResult;
  using x3d::runtime::extract::FontKey;

  auto state = std::make_shared<State>();
  state->faces = std::move(faces);

  return [state](const FontKey& key) -> GlyphResult {
    if (!key.style.empty() && key.style != "PLAIN") return GlyphResult::makeFailed();

    std::shared_ptr<Face> face = state->faceFor(key.family);
    if (!face || !face->ok) return GlyphResult::makeFailed();

    const int glyph = stbtt_FindGlyphIndex(&face->info, static_cast<int>(key.codepoint));
    if (glyph == 0) return GlyphResult::makeFailed();  // .notdef → Failed (rule 3)

    int advance = 0, lsb = 0;
    stbtt_GetGlyphHMetrics(&face->info, glyph, &advance, &lsb);  // unscaled (rule 1)

    const std::uint16_t upm = unitsPerEm(face->info);            // raw uint16 (rule 2)
    if (upm == 0) return GlyphResult::makeFailed();

    const float advanceEm = static_cast<float>(advance) / static_cast<float>(upm);
    return GlyphResult::makeReady(GlyphMetrics{advanceEm, false, 0.f, 0.f, 0.f, 0.f});
  };
}

}  // namespace x3d::runtime::io::stbtt
