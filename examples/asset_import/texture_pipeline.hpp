#ifndef X3D_ASSET_IMPORT_TEXTURE_PIPELINE_HPP
#define X3D_ASSET_IMPORT_TEXTURE_PIPELINE_HPP

#include "import_source.hpp"

#include <string>
#include <unordered_map>

namespace x3d::asset_import {

// A resolved texture plan: maps a texture key (see textureKey) to the
// relative URL of the file written under <outDir>/assets/. Consumed by
// emit() via EmitOptions::textures to set ImageTexture::url.
struct TexturePlan {
  // key -> relative URL, e.g. "assets/<hash>.png".
  std::unordered_map<std::string, std::string> urlByKey;
};

// Canonical key for a TextureRef, used to look up its URL in a TexturePlan.
// External textures key on their source path; embedded textures key on
// "embedded:<index>". planTextures() and urlForRef() use the same derivation
// so emit() can resolve any TextureRef regardless of backend.
inline std::string textureKey(const TextureRef& ref) {
  if (ref.externalPath) return *ref.externalPath;
  if (ref.embeddedIndex) return "embedded:" + std::to_string(*ref.embeddedIndex);
  return "";
}

// Returns the resolved URL for a TextureRef from a plan, or "" if the ref is
// absent from the plan (no texture written). Inline so emit() — which links
// only the slim authoring target, not the pipeline TU — can call it without
// an extra link dependency.
inline std::string urlForRef(const TexturePlan& plan, const TextureRef& ref) {
  const std::string key = textureKey(ref);
  if (key.empty()) return "";
  const auto it = plan.urlByKey.find(key);
  return it == plan.urlByKey.end() ? "" : it->second;
}

// Collects every texture referenced by the scene's materials, decodes/
// re-encodes as needed, dedups by content hash, and writes each unique
// texture to <outDir>/assets/<hash>.<ext>. External textures are read
// relative to modelDir; embedded textures come from ImportScene::embedded.
//
// When recompress is false and the source bytes are web-safe (png/jpeg),
// the bytes are written through untouched. Otherwise the texture is
// decoded (via the x3d_stb seam) and re-encoded as PNG (stb_image_write).
//
// Returns the plan mapping each texture key to its relative URL.
TexturePlan planTextures(const ImportScene& scene, const std::string& outDir,
                         const std::string& modelDir, bool recompress);

}  // namespace x3d::asset_import

#endif  // X3D_ASSET_IMPORT_TEXTURE_PIPELINE_HPP
