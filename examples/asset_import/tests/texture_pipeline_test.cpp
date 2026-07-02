#include "texture_pipeline.hpp"
#include "emit.hpp"
#include "fixture_source.hpp"
#include "x3d/authoring.hpp"
#include "doctest/doctest.h"
#include <filesystem>
using namespace x3d::asset_import;

// A known-valid 1x1 RGBA PNG (red), produced via stb_image_write.
// Used so the write-through path has real bytes to hash and dedup.
static std::vector<std::uint8_t> makeTinyPng() {
  return {
      0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
      0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
      0x08, 0x00, 0x00, 0x00, 0x00, 0x3a, 0x7e, 0x9b, 0x55, 0x00, 0x00, 0x00,
      0x0a, 0x49, 0x44, 0x41, 0x54, 0x78, 0x5e, 0x63, 0xf8, 0x0f, 0x00, 0x01,
      0x01, 0x01, 0x00, 0x27, 0x09, 0xdd, 0xda, 0x00, 0x00, 0x00, 0x00, 0x49,
      0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82,
  };
}

TEST_CASE("texture_pipeline_dedups_identical_embedded_textures") {
  ImportScene s;
  EmbeddedTexture t;
  t.key = "a";
  t.hintExt = "png";
  t.bytes = makeTinyPng();
  s.embedded = {t, t};  // two identical embedded textures

  // Reference both from one material so they get collected.
  ImportMaterial mat;
  mat.textures.baseColor = TextureRef{};
  mat.textures.baseColor->embeddedIndex = 0;
  mat.textures.emissive = TextureRef{};
  mat.textures.emissive->embeddedIndex = 1;
  s.materials.push_back(mat);

  auto out = std::filesystem::temp_directory_path() / "x3d_tex_test";
  std::filesystem::remove_all(out);
  TexturePlan plan = planTextures(s, out.string(), /*modelDir=*/"",
                                  /*recompress=*/false);

  // Both embedded refs collapse to one file on disk (same content hash).
  int files = 0;
  for (auto& e : std::filesystem::directory_iterator(out / "assets")) {
    (void)e;
    ++files;
  }
  CHECK(files == 1);
  // And both refs resolve to the same URL.
  const std::string url0 = urlForRef(plan, *mat.textures.baseColor);
  const std::string url1 = urlForRef(plan, *mat.textures.emissive);
  CHECK_FALSE(url0.empty());
  CHECK(url0 == url1);
}

TEST_CASE("texture_pipeline_distinct_textures_get_distinct_files") {
  ImportScene s;
  EmbeddedTexture a;
  a.key = "a";
  a.hintExt = "png";
  a.bytes = makeTinyPng();
  EmbeddedTexture b = a;
  b.key = "b";
  b.bytes[67 - 5] ^= 0xFF;  // perturb a byte -> different content hash
  s.embedded = {a, b};

  ImportMaterial mat;
  mat.textures.baseColor = TextureRef{};
  mat.textures.baseColor->embeddedIndex = 0;
  mat.textures.normal = TextureRef{};
  mat.textures.normal->embeddedIndex = 1;
  s.materials.push_back(mat);

  auto out = std::filesystem::temp_directory_path() / "x3d_tex_test2";
  std::filesystem::remove_all(out);
  TexturePlan plan = planTextures(s, out.string(), "", /*recompress=*/false);

  int files = 0;
  for (auto& e : std::filesystem::directory_iterator(out / "assets")) {
    (void)e;
    ++files;
  }
  CHECK(files == 2);
  CHECK(urlForRef(plan, *mat.textures.baseColor) !=
        urlForRef(plan, *mat.textures.normal));
}

// ── emit integration: texture wiring through the slim authoring surface ──

TEST_CASE("emit_wires_image_texture_when_plan_supplied") {
  ImportScene s = FixtureSource{}.load("cube");
  EmbeddedTexture t;
  t.key = "diff";
  t.hintExt = "png";
  t.bytes = makeTinyPng();
  s.embedded.push_back(t);
  s.materials[0].textures.baseColor = TextureRef{};
  s.materials[0].textures.baseColor->embeddedIndex = 0;

  auto out = std::filesystem::temp_directory_path() / "x3d_emit_tex";
  std::filesystem::remove_all(out);
  TexturePlan plan = planTextures(s, out.string(), "", /*recompress=*/false);

  EmitOptions opts;
  opts.textures = &plan;
  const std::string xml = x3d::authoring::XmlWriter{}.writeDocument(emit(s, opts));
  CHECK(xml.find("<ImageTexture") != std::string::npos);
  const std::string url = urlForRef(plan, *s.materials[0].textures.baseColor);
  CHECK_FALSE(url.empty());
  CHECK(xml.find(url) != std::string::npos);
}

TEST_CASE("emit_no_image_texture_without_plan") {
  ImportScene s = FixtureSource{}.load("cube");
  s.materials[0].textures.baseColor = TextureRef{};
  s.materials[0].textures.baseColor->embeddedIndex = 0;
  // No plan supplied -> no ImageTexture emitted, even though the slot is set.
  const std::string xml = x3d::authoring::XmlWriter{}.writeDocument(emit(s, {}));
  CHECK(xml.find("<ImageTexture") == std::string::npos);
}
