// TexturePlan.cpp — writes imported textures under <assetsRoot>/assets/.
#include "TexturePlan.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

// stb_image_write is encode-only; this file is its single implementation site in
// the import library.
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#endif
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#include "stb_image_write.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace x3d::runtime::importer {

namespace {

namespace fs = std::filesystem;
using x3d::runtime::extract::TexturePixelResult;

std::uint64_t fnv1a64(const std::vector<std::uint8_t> &bytes) {
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

std::string lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return s;
}

std::string extensionOf(const std::string &name) {
  const std::string e = fs::path(name).extension().string();
  return e.empty() ? "" : lower(e.substr(1));
}

bool isWebSafe(const std::string &ext) {
  return ext == "png" || ext == "jpg" || ext == "jpeg";
}

/// Sniffs PNG/JPEG magic so a mislabelled embedded image still passes through.
std::string sniffExtension(const std::vector<std::uint8_t> &b) {
  if (b.size() >= 8 && b[0] == 0x89 && b[1] == 'P' && b[2] == 'N' && b[3] == 'G')
    return "png";
  if (b.size() >= 3 && b[0] == 0xFF && b[1] == 0xD8 && b[2] == 0xFF) return "jpg";
  return "";
}

bool readFile(const fs::path &path, std::vector<std::uint8_t> &out) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f) return false;
  const std::streamoff size = f.tellg();
  if (size < 0) return false;
  out.resize(static_cast<std::size_t>(size));
  f.seekg(0);
  return size == 0 || static_cast<bool>(f.read(reinterpret_cast<char *>(out.data()), size));
}

bool writeFile(const fs::path &path, const std::vector<std::uint8_t> &bytes) {
  std::ofstream f(path, std::ios::binary | std::ios::trunc);
  if (!f) return false;
  if (!bytes.empty())
    f.write(reinterpret_cast<const char *>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
  return static_cast<bool>(f);
}

/// Percent-decodes a glTF/URI path ("my%20tex.png" -> "my tex.png").
std::string uriDecode(const std::string &s) {
  std::string out;
  out.reserve(s.size());
  for (std::size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '%' && i + 2 < s.size() && std::isxdigit(static_cast<unsigned char>(s[i + 1])) &&
        std::isxdigit(static_cast<unsigned char>(s[i + 2]))) {
      out += static_cast<char>(std::stoi(s.substr(i + 1, 2), nullptr, 16));
      i += 2;
    } else {
      out += s[i];
    }
  }
  return out;
}

void collectRefs(const TextureSlots &slots, std::vector<const TextureRef *> &out) {
  for (const auto *f : {&slots.baseColor, &slots.normal, &slots.emissive,
                        &slots.occlusion, &slots.metallicRoughness, &slots.specular,
                        &slots.shininess, &slots.ambient})
    if (f->has_value()) out.push_back(&**f);
}

/// Encodes RGBA8 texels (top row first) as PNG.
std::vector<std::uint8_t> encodePng(std::uint32_t w, std::uint32_t h,
                                    const std::uint8_t *rgba, bool bottomUp) {
  std::vector<std::uint8_t> out;
  stbi_flip_vertically_on_write(bottomUp ? 1 : 0);
  stbi_write_png_to_func(
      [](void *ctx, void *data, int size) {
        auto *v = static_cast<std::vector<std::uint8_t> *>(ctx);
        const auto *p = static_cast<const std::uint8_t *>(data);
        v->insert(v->end(), p, p + size);
      },
      &out, static_cast<int>(w), static_cast<int>(h), 4, rgba, static_cast<int>(w) * 4);
  stbi_flip_vertically_on_write(0);
  return out;
}

} // namespace

TexturePlan planTextures(const ImportScene &scene, const TexturePlanOptions &opts,
                         TexturePlanReport *report) {
  TexturePlan plan;
  TexturePlanReport local;
  TexturePlanReport &rep = report ? *report : local;

  std::vector<const TextureRef *> refs;
  for (const ImportMaterial &mat : scene.materials) collectRefs(mat.textures, refs);
  if (refs.empty()) return plan;

  const fs::path assetsDir = fs::path(opts.assetsRoot.empty() ? "." : opts.assetsRoot) / "assets";
  std::error_code ec;
  fs::create_directories(assetsDir, ec);
  if (ec) {
    rep.failed.push_back("cannot create " + assetsDir.string() + ": " + ec.message());
    return plan;
  }

  std::string urlPrefix = "assets";
  {
    const auto rel = fs::absolute(assetsDir).lexically_normal().lexically_relative(
        fs::absolute(opts.documentDir.empty() ? "." : opts.documentDir).lexically_normal());
    if (!rel.empty()) urlPrefix = rel.generic_string();
  }

  std::unordered_map<std::uint64_t, std::string> urlByHash;

  for (const TextureRef *refp : refs) {
    const TextureRef &ref = *refp;
    const std::string key = textureKey(ref);
    if (key.empty() || plan.urlByKey.count(key)) continue;

    std::vector<std::uint8_t> bytes;
    std::string ext;
    std::optional<fs::path> sourcePath; // decodable in place
    std::uint32_t rawW = 0, rawH = 0;
    if (ref.externalPath) {
      fs::path p = fs::path(uriDecode(*ref.externalPath));
      if (p.is_relative() && !opts.modelDir.empty()) p = fs::path(opts.modelDir) / p;
      if (!readFile(p, bytes)) {
        rep.missing.push_back(*ref.externalPath);
        continue;
      }
      sourcePath = p;
      ext = extensionOf(p.string());
    } else {
      const int idx = *ref.embeddedIndex;
      if (idx < 0 || static_cast<std::size_t>(idx) >= scene.embedded.size()) {
        rep.missing.push_back(key);
        continue;
      }
      const EmbeddedTexture &emb = scene.embedded[static_cast<std::size_t>(idx)];
      bytes = emb.bytes;
      rawW = emb.width;
      rawH = emb.height;
      ext = lower(emb.hintExt);
      if (bytes.empty()) {
        rep.missing.push_back(emb.key.empty() ? key : emb.key);
        continue;
      }
    }
    if (const std::string sniffed = sniffExtension(bytes); !sniffed.empty() && !rawW)
      ext = sniffed;
    if (ext == "jpeg") ext = "jpg";

    const std::uint64_t hash = fnv1a64(bytes);
    if (auto seen = urlByHash.find(hash); seen != urlByHash.end()) {
      plan.urlByKey[key] = seen->second;
      ++rep.deduplicated;
      continue;
    }
    const std::string stem = hexHash(hash);

    std::vector<std::uint8_t> out;
    std::string outExt;
    if (rawW && rawH) {
      if (bytes.size() < static_cast<std::size_t>(rawW) * rawH * 4) {
        rep.failed.push_back(key + ": raw texture shorter than its size");
        continue;
      }
      out = encodePng(rawW, rawH, bytes.data(), false);
      outExt = "png";
      ++rep.reencoded;
    } else if (isWebSafe(ext) && !opts.recompress) {
      out = std::move(bytes);
      outExt = ext;
    } else if (opts.decoder) {
      fs::path decodePath;
      fs::path temp;
      if (sourcePath) {
        decodePath = *sourcePath;
      } else {
        temp = assetsDir / (".decode_" + stem + "." + (ext.empty() ? "bin" : ext));
        if (!writeFile(temp, bytes)) {
          rep.failed.push_back(key + ": cannot write a temporary file to decode");
          continue;
        }
        decodePath = temp;
      }
      const TexturePixelResult res = opts.decoder(decodePath.string());
      if (!temp.empty()) fs::remove(temp, ec);
      if (!res.ready() || !res.pixels || res.pixels->width == 0 || res.pixels->height == 0 ||
          res.pixels->rgba.size() < static_cast<std::size_t>(res.pixels->width) *
                                        res.pixels->height * 4) {
        rep.failed.push_back(key + ": could not decode the image");
        continue;
      }
      // TextureResolver rows are bottom-up (X3D's image origin).
      out = encodePng(res.pixels->width, res.pixels->height, res.pixels->rgba.data(), true);
      outExt = "png";
      ++rep.reencoded;
    } else {
      out = std::move(bytes);
      outExt = ext.empty() ? "bin" : ext;
      rep.notWebSafe.push_back(key);
    }
    if (out.empty()) {
      rep.failed.push_back(key + ": encoding produced no data");
      continue;
    }

    const std::string fileName = stem + "." + outExt;
    if (!writeFile(assetsDir / fileName, out)) {
      rep.failed.push_back(key + ": cannot write " + (assetsDir / fileName).string());
      continue;
    }
    ++rep.written;
    const std::string url = urlPrefix + "/" + fileName;
    urlByHash[hash] = url;
    plan.urlByKey[key] = url;
  }
  return plan;
}

} // namespace x3d::runtime::importer
