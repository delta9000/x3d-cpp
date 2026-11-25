// TexturePlan.hpp
// Writes the images an ImportScene references next to the output document and
// records the relative url emitDocument() gives each ImageTexture.
#ifndef X3D_RUNTIME_IMPORT_TEXTURE_PLAN_HPP
#define X3D_RUNTIME_IMPORT_TEXTURE_PLAN_HPP

#include "ImportScene.hpp"
#include "TextureResolver.hpp"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace x3d::runtime::importer {

/// Texture key -> url relative to the output document ("assets/<hash>.png").
struct TexturePlan {
  std::unordered_map<std::string, std::string> urlByKey;
};

/// The key a TextureRef has in a TexturePlan: the external path, or
/// "embedded:<index>".
inline std::string textureKey(const TextureRef &ref) {
  if (ref.externalPath) return *ref.externalPath;
  if (ref.embeddedIndex) return "embedded:" + std::to_string(*ref.embeddedIndex);
  return "";
}

/// The url planned for `ref`, or "" when no image was written for it.
inline std::string urlForRef(const TexturePlan &plan, const TextureRef &ref) {
  const std::string key = textureKey(ref);
  if (key.empty()) return "";
  const auto it = plan.urlByKey.find(key);
  return it == plan.urlByKey.end() ? "" : it->second;
}

struct TexturePlanOptions {
  /// Images go to <assetsRoot>/assets/. Created when missing.
  std::string assetsRoot = ".";
  /// Directory of the source model; external texture paths resolve against it.
  std::string modelDir;
  /// Directory of the output document; urls are made relative to it.
  std::string documentDir = ".";
  /// Decode and re-encode PNG and JPEG images as PNG too (default: copy them).
  bool recompress = false;
  /// Decoder for images that are not PNG or JPEG (and for `recompress`). The
  /// image is then written as PNG. Without one, such an image is copied as it
  /// is and reported in TexturePlanReport::notWebSafe.
  x3d::runtime::extract::TextureResolver decoder;
};

struct TexturePlanReport {
  std::size_t written = 0;      ///< distinct image files written
  std::size_t deduplicated = 0; ///< references that reused an identical image
  std::size_t reencoded = 0;    ///< images written as PNG after decoding
  std::vector<std::string> missing;    ///< keys whose bytes could not be read
  std::vector<std::string> notWebSafe; ///< copied without conversion (no decoder)
  std::vector<std::string> failed;     ///< decode or write failures
};

/// Writes every texture the scene's materials reference, one file per distinct
/// content (FNV-1a hash), and returns the urls. Never throws for a single bad
/// image; it is listed in the report and left out of the plan.
TexturePlan planTextures(const ImportScene &scene, const TexturePlanOptions &opts,
                         TexturePlanReport *report = nullptr);

} // namespace x3d::runtime::importer

#endif // X3D_RUNTIME_IMPORT_TEXTURE_PLAN_HPP
