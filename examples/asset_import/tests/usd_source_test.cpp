#include "usd_source.hpp"
#include "doctest/doctest.h"
#include <cmath>
using namespace x3d::asset_import;

TEST_CASE("usd_loads_simple_cube_usda") {
  UsdSource src;
  ImportScene s = src.load("/tmp/usd_test_scenes/simple_cube.usda");
  CHECK(s.meshes.size() == 1);
  // CubeMesh points: 8 vertices
  CHECK(s.meshes[0].positions.size() == 8);
  // faceVertexIndices: 24 indices for 6 quads (but triangulated is 36 indices)
  CHECK(s.meshes[0].indices.size() == 36);
  CHECK(s.rootNode >= 0);
}

#ifdef TEXTURE_USDZ
// Embedded textures in a USDZ must be extracted as the ORIGINAL compressed file
// bytes (the JPEG/PNG the archive stores), not tinyusdz's decoded raw texel
// buffer. The downstream texture pipeline pass-throughs these bytes to a
// `.jpg`/`.png` on disk and feeds them to an image decoder; decoded texels
// labelled `.jpg` produce giant invalid files. texture-cat-plane.usdz embeds a
// single 50234-byte JPEG (textures/texture-cat.jpg).
TEST_CASE("usdz_embedded_texture_keeps_original_compressed_bytes") {
  UsdSource src;
  ImportScene s = src.load(TEXTURE_USDZ);
  REQUIRE(s.embedded.size() >= 1);
  const EmbeddedTexture& emb = s.embedded[0];
  CHECK(emb.hintExt == "jpg");
  REQUIRE(emb.bytes.size() >= 3);
  // JPEG SOI marker FF D8 FF — present only if we kept the compressed file.
  CHECK(emb.bytes[0] == 0xFF);
  CHECK(emb.bytes[1] == 0xD8);
  CHECK(emb.bytes[2] == 0xFF);
  // Exact original file size; decoded RGBA texels would be far larger.
  CHECK(emb.bytes.size() == 50234u);
}

// Node transforms must convert USD's row-major/row-vector matrix4d into our
// column-major/column-vector Mat4 WITHOUT transposing. A stray transpose
// inverts every rotation (renders the model upside-down / mirrored) and drops
// translation into the always-zero last column. texture-cat-plane.usdz places
// its plane with a +90° rotation about X (Z-up->Y-up) and a translation of
// (0,-10,0); both must survive intact.
TEST_CASE("usdz_node_transform_not_transposed") {
  UsdSource src;
  ImportScene s = src.load(TEXTURE_USDZ);
  REQUIRE(!s.nodes.empty());

  // Find the node carrying the y=-10 translation (column-major: m[13]).
  const ImportNode* placed = nullptr;
  for (const ImportNode& n : s.nodes) {
    if (std::abs(n.localTransform.m[13] + 10.0f) < 1e-3f) { placed = &n; break; }
  }
  REQUIRE(placed != nullptr);  // transpose bug would zero the translation

  const auto& m = placed->localTransform.m;
  // Translation lives in the last column (m[12..14]) — not the last row.
  CHECK(m[12] == doctest::Approx(0.0f).epsilon(1e-3));
  CHECK(m[13] == doctest::Approx(-10.0f).epsilon(1e-3));
  CHECK(m[14] == doctest::Approx(0.0f).epsilon(1e-3));
  // +90° about X in column-major column-vector form: m[6]=+1, m[9]=-1.
  // The transposed (buggy) matrix would give the opposite signs.
  CHECK(m[6] == doctest::Approx(1.0f).epsilon(1e-3));
  CHECK(m[9] == doctest::Approx(-1.0f).epsilon(1e-3));
}
#endif

#ifdef MATERIAL_USDA
// UsdPreviewSurface fidelity fields must survive extraction into ImportMaterial.
// The material loop historically read only diffuse/emissive/specular/opacity/
// roughness/metallic and silently dropped ior/clearcoat/opacityThreshold/
// useSpecularWorkflow/specularColor — all of which tydra populates. This asserts
// each authored input round-trips, and that AlphaMode is derived from the opacity
// signals (opacityThreshold > 0 → Mask).
TEST_CASE("usd_material_preview_surface_fidelity_fields") {
  UsdSource src;
  ImportScene s = src.load(MATERIAL_USDA);
  REQUIRE(s.materials.size() >= 1);
  const ImportMaterial& m = s.materials[0];

  CHECK(m.useSpecularWorkflow == true);
  CHECK(m.ior == doctest::Approx(1.4f).epsilon(1e-3));
  CHECK(m.clearcoat == doctest::Approx(0.7f).epsilon(1e-3));
  CHECK(m.clearcoatRoughness == doctest::Approx(0.2f).epsilon(1e-3));
  CHECK(m.opacity == doctest::Approx(0.5f).epsilon(1e-3));
  CHECK(m.opacityThreshold == doctest::Approx(0.3f).epsilon(1e-3));
  CHECK(m.specular.x == doctest::Approx(0.9f).epsilon(1e-3));
  CHECK(m.specular.y == doctest::Approx(0.8f).epsilon(1e-3));
  CHECK(m.specular.z == doctest::Approx(0.7f).epsilon(1e-3));
  // opacityThreshold > 0 is a masked cutout, not a blend.
  CHECK(m.alpha == AlphaMode::Mask);
}
#endif

#ifdef COMPOSED_ROOT_USDA
// Composed USD (USDA/USDC) must resolve external references/payloads/subLayers,
// which tinyusdz' plain LoadUSDFromFile does not. root.usda uses `add references`
// (the ListEditQual::Add path) to piece.usda, which defers its geometry to a
// subLayer (geom.usda) — mirroring OpenChessSet's `add references` -> payload ->
// subLayer -> geom chain. Both features live in our vendored tinyusdz patch;
// without them this imports as empty Xforms with zero meshes.
TEST_CASE("usd_composed_add_references_and_sublayers_resolve_geometry") {
  UsdSource src;
  ImportScene s = src.load(COMPOSED_ROOT_USDA);
  REQUIRE(s.meshes.size() == 2);  // InstanceA + InstanceB
  for (const ImportMesh& m : s.meshes) {
    CHECK(m.positions.size() == 3);  // one triangle from geom.usda's subLayer
    CHECK(m.indices.size() == 3);
  }
}
#endif
