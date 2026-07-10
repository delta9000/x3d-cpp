#include "assimp_source.hpp"
#include "cgltf_source.hpp"
#include "doctest/doctest.h"
#include <string>
using namespace x3d::asset_import;

static std::string fixture() {
  return std::string(X3D_ASSET_IMPORT_FIXTURE_DIR) + "/twobox.glb";
}
static size_t totalTris(const ImportScene& s) {
  size_t t = 0;
  for (const auto& m : s.meshes) t += m.indices.size() / 3;
  return t;
}

// The ImportSource seam is generic iff two independent backends agree on a
// scene's structural invariants for the same input. Comparison is tolerant to
// vertex ordering, mesh splitting, and material naming (which differ between
// parsers) — only the counts and PBR values that carry author intent must match.
TEST_CASE("cgltf and assimp agree on twobox.glb invariants") {
  ImportScene a = CgltfSource{}.load(fixture());
  ImportScene b = AssimpSource{}.load(fixture());

  CHECK(totalTris(a) == totalTris(b));            // order-/split-independent
  CHECK(a.cameras.size() == b.cameras.size());
  CHECK(a.lights.size() == b.lights.size());
  CHECK(a.nodes.size() >= 2);                      // both boxes present
  CHECK(b.nodes.size() >= 2);

  // Material COUNT is deliberately not asserted equal: assimp's glTF importer
  // appends an unnamed default material (metallic=1, roughness=1) that cgltf
  // does not, so the counts differ (cgltf 1, assimp 2) while both recover the
  // authored material identically at index 0 — which the PBR checks below prove.
  CHECK(a.materials.size() >= 1);
  CHECK(b.materials.size() >= 1);

  REQUIRE(!a.materials.empty());
  REQUIRE(!b.materials.empty());
  REQUIRE(a.materials[0].pbr.has_value());
  REQUIRE(b.materials[0].pbr.has_value());
  CHECK(a.materials[0].pbr->metallic ==
        doctest::Approx(b.materials[0].pbr->metallic).epsilon(1e-4));
  CHECK(a.materials[0].pbr->roughness ==
        doctest::Approx(b.materials[0].pbr->roughness).epsilon(1e-4));
}
