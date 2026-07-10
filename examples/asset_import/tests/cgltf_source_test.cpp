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

TEST_CASE("cgltf loads the full glTF scene surface") {
  CgltfSource src;
  ImportScene s = src.load(fixture());

  // One shared mesh (12 tris) referenced by both box nodes.
  CHECK(s.meshes.size() == 1);
  CHECK(totalTris(s) == 12);
  CHECK(s.meshes[0].positions.size() == 24);
  CHECK(s.meshes[0].normals.size() == 24);
  CHECK(s.meshes[0].uv.size() == 24);

  // PBR metallic-roughness material with an embedded baseColor texture.
  REQUIRE(s.materials.size() == 1);
  REQUIRE(s.materials[0].pbr.has_value());
  CHECK(s.materials[0].pbr->metallic == doctest::Approx(0.0f));
  CHECK(s.materials[0].pbr->roughness == doctest::Approx(0.6f));
  CHECK(s.materials[0].textures.baseColor.has_value());
  CHECK(s.embedded.size() == 1); // the 2x2 PNG carried from the GLB bin chunk

  // Hierarchy: parent, child, cam, lightnode.
  CHECK(s.nodes.size() == 4);
  CHECK(s.rootNode == 0);
  CHECK(s.nodes[0].childIndices.size() == 3);

  // Camera + KHR_lights_punctual light.
  REQUIRE(s.cameras.size() == 1);
  CHECK(s.cameras[0].yfov == doctest::Approx(0.7853982f));
  REQUIRE(s.lights.size() == 1);
  CHECK(s.lights[0].kind == ImportLight::Kind::Point);
  CHECK(s.lights[0].radius == doctest::Approx(20.0f));
}
