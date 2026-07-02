// texture_pipeline.cpp — consumer-owned texture handling for asset_import.
// Decodes via the x3d_stb seam (TextureResolver), re-encodes PNG via the
// vendored stb_image_write.h. Lives OUTSIDE the x3d_cpp::authoring slim
// target (emit.cpp links only the slim surface); the pipeline is consumer
// glue that the CLI (Task 11) threads into emit() via EmitOptions::textures.
#include "texture_pipeline.hpp"

#include "StbTextureResolver.hpp"  // x3d::runtime::io::stb — the decode seam
#include "TextureResolver.hpp"     // x3d::runtime::extract::TextureResolver

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

// stb_image_write is ENCODE-only and consumer-owned (this TU is the sole
// site of STB_IMAGE_WRITE_IMPLEMENTATION). Suppress its unused-function noise
// without softening warnings for the rest of the file.
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace x3d::asset_import {

namespace {

using x3d::runtime::extract::TexturePixelResult;
using x3d::runtime::extract::TextureResolver;
using x3d::runtime::io::stb::makeStbTextureResolver;

// FNV-1a 64-bit over raw bytes — content identity for dedup.
std::uint64_t fnv1a64(const std::vector<std::uint8_t>& bytes) {
  std::uint64_t h = 0xcbf29ce484222325ULL;
  for (auto b : bytes) {
    h ^= b;
    h *= 0x100000001b3ULL;
  }
  return h;
}

std::string hexHash(std::uint64_t h) {
  char buf[17];
  std::snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(h));
  return buf;
}

std::string toLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return s;
}

// File extension without the dot, lowercased. "" if none.
std::string extensionOf(const std::string& name) {
  const auto dot = name.find_last_of('.');
  if (dot == std::string::npos) return "";
  return toLower(name.substr(dot + 1));
}

bool isWebSafe(const std::string& ext) {
  return ext == "png" || ext == "jpg" || ext == "jpeg";
}

bool readFile(const std::string& path, std::vector<std::uint8_t>& out) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f) return false;
  const std::streamoff size = f.tellg();
  if (size < 0) return false;
  out.resize(static_cast<std::size_t>(size));
  f.seekg(0);
  if (size > 0 && !f.read(reinterpret_cast<char*>(out.data()), size)) return false;
  return true;
}

bool writeFile(const std::string& path, const std::vector<std::uint8_t>& bytes) {
  std::ofstream f(path, std::ios::binary);
  if (!f) return false;
  if (!bytes.empty() && !f.write(reinterpret_cast<const char*>(bytes.data()),
                                 static_cast<std::streamsize>(bytes.size()))) {
    return false;
  }
  return true;
}

// Join two path segments with a single separator.
std::string joinPath(const std::string& dir, const std::string& rel) {
  if (dir.empty()) return rel;
  if (dir.back() == '/') return dir + rel;
  return dir + "/" + rel;
}

// Resolves the raw bytes for one TextureRef.
// Returns false if the ref is empty or the bytes cannot be obtained.
bool resolveBytes(const TextureRef& ref, const ImportScene& scene,
                  const std::string& modelDir, std::vector<std::uint8_t>& out) {
  if (ref.externalPath) {
    return readFile(joinPath(modelDir, *ref.externalPath), out);
  }
  if (ref.embeddedIndex) {
    const int idx = *ref.embeddedIndex;
    if (idx < 0 || static_cast<std::size_t>(idx) >= scene.embedded.size()) return false;
    out = scene.embedded[idx].bytes;
    return !out.empty();
  }
  return false;
}

// Source extension for a TextureRef (embedded uses hintExt; external uses the
// path extension). Falls back to "" when unknowable.
std::string sourceExtension(const TextureRef& ref, const ImportScene& scene) {
  if (ref.externalPath) return extensionOf(*ref.externalPath);
  if (ref.embeddedIndex) {
    const int idx = *ref.embeddedIndex;
    if (idx >= 0 && static_cast<std::size_t>(idx) < scene.embedded.size())
      return toLower(scene.embedded[idx].hintExt);
  }
  return "";
}

// Collects every set TextureRef in a material's TextureSlots.
std::vector<const TextureRef*> collectRefs(const TextureSlots& slots) {
  std::vector<const TextureRef*> refs;
  const std::optional<TextureRef>* fields[] = {
      &slots.baseColor, &slots.normal,    &slots.emissive,
      &slots.occlusion, &slots.metallicRoughness, &slots.specular};
  for (auto* f : fields) {
    if (f->has_value()) refs.push_back(&**f);
  }
  return refs;
}

// Decodes bytes to RGBA via the seam and re-encodes as PNG to outPath.
// Writes bottom-left seam output flipped to a top-left-correct PNG.
bool reencodePng(const TextureResolver& resolver, const std::string& decodePath,
                  const std::string& outPath) {
  const TexturePixelResult res = resolver(decodePath);
  if (!res.ready()) return false;
  const auto& px = res.pixels;
  if (px.width == 0 || px.height == 0 || px.rgba.empty()) return false;
  stbi_flip_vertically_on_write(1);  // seam returns bottom-left; PNG is top-left.
  const int ok = stbi_write_png(outPath.c_str(), static_cast<int>(px.width),
                                static_cast<int>(px.height), /*comp=*/4,
                                px.rgba.data(),
                                static_cast<int>(px.width) * 4);
  stbi_flip_vertically_on_write(0);
  return ok != 0;
}

}  // namespace

TexturePlan planTextures(const ImportScene& scene, const std::string& outDir,
                         const std::string& modelDir, bool recompress) {
  TexturePlan plan;

  const std::filesystem::path assetsDir =
      std::filesystem::path(outDir) / "assets";
  std::error_code ec;
  std::filesystem::create_directories(assetsDir, ec);

  // Lazy-init the decode seam (only needed on the re-encode path).
  TextureResolver resolver;
  bool resolverInit = false;
  auto ensureResolver = [&]() -> const TextureResolver& {
    if (!resolverInit) {
      resolver = makeStbTextureResolver();
      resolverInit = true;
    }
    return resolver;
  };

  // hash -> relative URL ("assets/<hash>.<ext>"); dedups identical content to
  // one file on disk even when referenced by distinct keys.
  std::unordered_map<std::uint64_t, std::string> fileByHash;

  for (const ImportMaterial& mat : scene.materials) {
    for (const TextureRef* refp : collectRefs(mat.textures)) {
      const TextureRef& ref = *refp;
      const std::string key = textureKey(ref);
      if (key.empty()) continue;

      std::vector<std::uint8_t> bytes;
      if (!resolveBytes(ref, scene, modelDir, bytes)) continue;

      const std::uint64_t hash = fnv1a64(bytes);
      const auto seen = fileByHash.find(hash);
      if (seen != fileByHash.end()) {
        plan.urlByKey[key] = seen->second;  // identical content already written
        continue;
      }

      const std::string ext = sourceExtension(ref, scene);
      const std::string hashHex = hexHash(hash);
      std::string relUrl;
      const std::string outBase = assetsDir.string() + "/" + hashHex;

      if (isWebSafe(ext) && !recompress) {
        // Write the original bytes through (no decode/encode round-trip).
        const std::string outPath = outBase + "." + ext;
        if (!writeFile(outPath, bytes)) continue;
        relUrl = "assets/" + hashHex + "." + ext;
      } else {
        // Decode via the seam, re-encode as PNG.
        std::string decodePath;
        std::string tempPath;
        if (ref.externalPath) {
          decodePath = joinPath(modelDir, *ref.externalPath);
        } else {
          // Embedded: materialize bytes to a temp file the seam can read.
          tempPath = assetsDir.string() + "/.decode_" + hashHex + "." +
                     (ext.empty() ? "bin" : ext);
          if (!writeFile(tempPath, bytes)) continue;
          decodePath = tempPath;
        }
        const std::string outPath = outBase + ".png";
        const bool ok = reencodePng(ensureResolver(), decodePath, outPath);
        if (!tempPath.empty()) {
          std::error_code rmErr;
          std::filesystem::remove(tempPath, rmErr);
        }
        if (!ok) continue;
        relUrl = "assets/" + hashHex + ".png";
      }

      fileByHash[hash] = relUrl;
      plan.urlByKey[key] = relUrl;
    }
  }

  return plan;
}

}  // namespace x3d::asset_import
