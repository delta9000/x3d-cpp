// texture_plan_test.cpp — planTextures(): one file per distinct image,
// pass-through of web formats, PNG encoding of raw texels, and reporting.
#include "TexturePlan.hpp"

#include "doctest/doctest.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace imp = x3d::runtime::importer;
namespace fs = std::filesystem;

namespace {

// A valid 1x1 greyscale PNG.
std::vector<std::uint8_t> tinyPng() {
  return {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
          0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x3a, 0x7e, 0x9b,
          0x55, 0x00, 0x00, 0x00, 0x0a, 0x49, 0x44, 0x41, 0x54, 0x78, 0x5e, 0x63, 0xf8, 0x0f, 0x00, 0x01,
          0x01, 0x01, 0x00, 0x27, 0x09, 0xdd, 0xda, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae,
          0x42, 0x60, 0x82};
}

fs::path freshDir(const std::string &name) {
  const fs::path d = fs::temp_directory_path() / ("x3d_texture_plan_" + name);
  fs::remove_all(d);
  fs::create_directories(d);
  return d;
}

std::size_t fileCount(const fs::path &dir) {
  if (!fs::exists(dir)) return 0;
  return static_cast<std::size_t>(std::distance(fs::directory_iterator(dir), fs::directory_iterator()));
}

imp::TextureRef embedded(int i) {
  imp::TextureRef r;
  r.embeddedIndex = i;
  return r;
}

std::vector<std::uint8_t> readAll(const fs::path &p) {
  std::ifstream in(p, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

} // namespace

TEST_CASE("texture plan: identical images are written once") {
  imp::ImportScene s;
  imp::EmbeddedTexture t;
  t.key = "a";
  t.hintExt = "png";
  t.bytes = tinyPng();
  s.embedded = {t, t};
  imp::ImportMaterial m;
  m.textures.baseColor = embedded(0);
  m.textures.emissive = embedded(1);
  s.materials.push_back(m);
  const fs::path out = freshDir("dedup");
  imp::TexturePlanOptions o;
  o.assetsRoot = out.string();
  o.documentDir = out.string();
  imp::TexturePlanReport rep;
  const imp::TexturePlan plan = imp::planTextures(s, o, &rep);
  CHECK(fileCount(out / "assets") == 1);
  CHECK(rep.written == 1);
  CHECK(rep.deduplicated == 1);
  const std::string url = imp::urlForRef(plan, *m.textures.baseColor);
  CHECK(url.rfind("assets/", 0) == 0);
  CHECK(url.size() > 11);
  CHECK(url.substr(url.size() - 4) == ".png");
  CHECK(url == imp::urlForRef(plan, *m.textures.emissive));
}

TEST_CASE("texture plan: urls are relative to the document directory") {
  imp::ImportScene s;
  imp::EmbeddedTexture t;
  t.bytes = tinyPng();
  s.embedded = {t};
  imp::ImportMaterial m;
  m.textures.baseColor = embedded(0);
  s.materials.push_back(m);
  const fs::path out = freshDir("relative");
  imp::TexturePlanOptions o;
  o.assetsRoot = out.string();
  o.documentDir = (out / "scenes" / "deep").string();
  const imp::TexturePlan plan = imp::planTextures(s, o);
  CHECK(imp::urlForRef(plan, *m.textures.baseColor).rfind("../../assets/", 0) == 0);
}

TEST_CASE("texture plan: raw texels are encoded as PNG, external files are copied") {
  const fs::path out = freshDir("raw");
  const fs::path model = out / "model dir";
  fs::create_directories(model);
  {
    std::ofstream f(model / "my tex.png", std::ios::binary);
    const auto png = tinyPng();
    f.write(reinterpret_cast<const char *>(png.data()), static_cast<std::streamsize>(png.size()));
  }
  imp::ImportScene s;
  imp::EmbeddedTexture raw;
  raw.key = "raw";
  raw.width = 2;
  raw.height = 1;
  raw.bytes = {255, 0, 0, 255, 0, 255, 0, 128};
  s.embedded = {raw};
  imp::ImportMaterial m;
  m.textures.baseColor = embedded(0);
  m.textures.normal = imp::TextureRef{};
  m.textures.normal->externalPath = "my%20tex.png"; // URI-encoded, as glTF writes it
  m.textures.occlusion = imp::TextureRef{};
  m.textures.occlusion->externalPath = "missing.png";
  s.materials.push_back(m);
  imp::TexturePlanOptions o;
  o.assetsRoot = out.string();
  o.documentDir = out.string();
  o.modelDir = model.string();
  imp::TexturePlanReport rep;
  const imp::TexturePlan plan = imp::planTextures(s, o, &rep);
  const std::string rawUrl = imp::urlForRef(plan, *m.textures.baseColor);
  REQUIRE_FALSE(rawUrl.empty());
  const auto bytes = readAll(out / rawUrl);
  REQUIRE(bytes.size() > 8);
  CHECK(bytes[1] == 'P');
  CHECK(bytes[2] == 'N');
  CHECK(bytes[3] == 'G');
  const std::string extUrl = imp::urlForRef(plan, *m.textures.normal);
  REQUIRE_FALSE(extUrl.empty());
  CHECK(readAll(out / extUrl) == tinyPng());
  CHECK(imp::urlForRef(plan, *m.textures.occlusion).empty());
  REQUIRE(rep.missing.size() == 1);
  CHECK(rep.missing[0] == "missing.png");
}

TEST_CASE("texture plan: a format browsers cannot show is reported without a decoder") {
  imp::ImportScene s;
  imp::EmbeddedTexture t;
  t.key = "img.tga";
  t.hintExt = "tga";
  t.bytes = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 32, 8, 1, 2, 3, 4};
  s.embedded = {t};
  imp::ImportMaterial m;
  m.textures.baseColor = embedded(0);
  s.materials.push_back(m);
  const fs::path out = freshDir("tga");
  imp::TexturePlanOptions o;
  o.assetsRoot = out.string();
  o.documentDir = out.string();
  imp::TexturePlanReport rep;
  const imp::TexturePlan plan = imp::planTextures(s, o, &rep);
  CHECK(rep.notWebSafe.size() == 1);
  CHECK(imp::urlForRef(plan, *m.textures.baseColor).find(".tga") != std::string::npos);
}
