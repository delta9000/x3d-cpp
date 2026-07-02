#include "assimp_source.hpp"
#include "doctest/doctest.h"
using namespace x3d::asset_import;

TEST_CASE("assimp_loads_triangle_obj") {
  AssimpSource src;
  ImportScene s = src.load(FIXTURE_OBJ);  // -D FIXTURE_OBJ=".../tri.obj"
  CHECK(s.meshes.size() == 1);
  CHECK(s.meshes[0].indices.size() == 3);  // one triangle = 3 indices
  CHECK(s.meshes[0].positions.size() == 3);
  CHECK(s.rootNode >= 0);
  CHECK(s.materials.size() >= 1);
}

TEST_CASE("assimp_triangle_has_normals_and_uv") {
  AssimpSource src;
  ImportScene s = src.load(FIXTURE_OBJ);
  REQUIRE(s.meshes.size() == 1);
  // assimp's GenSmoothNormals / the obj's own vn produce per-vertex normals.
  CHECK_FALSE(s.meshes[0].normals.empty());
  CHECK_FALSE(s.meshes[0].uv.empty());
}

TEST_CASE("assimp_triangle_material_is_phong_with_diffuse") {
  AssimpSource src;
  ImportScene s = src.load(FIXTURE_OBJ);
  REQUIRE(s.materials.size() >= 1);
  // assimp always inserts its 'DefaultMaterial' first; the .mtl's tri_mat
  // follows. Find it by name so the test doesn't depend on slot order.
  const ImportMaterial* tri = nullptr;
  for (const auto& m : s.materials) {
    if (m.name == "tri_mat") { tri = &m; break; }
  }
  REQUIRE(tri != nullptr);
  // The .mtl declares Kd 0.8 0.2 0.2 — Phong, no PBR.
  CHECK_FALSE(tri->pbr.has_value());
  CHECK(tri->diffuse.x == doctest::Approx(0.8f).epsilon(0.01));
  CHECK(tri->diffuse.y == doctest::Approx(0.2f).epsilon(0.01));
  CHECK(tri->diffuse.z == doctest::Approx(0.2f).epsilon(0.01));
}

TEST_CASE("assimp_mesh_references_named_material") {
  AssimpSource src;
  ImportScene s = src.load(FIXTURE_OBJ);
  REQUIRE(s.meshes.size() == 1);
  REQUIRE(s.meshes[0].materialIndex >= 0);
  REQUIRE(static_cast<std::size_t>(s.meshes[0].materialIndex) < s.materials.size());
  CHECK(s.materials[s.meshes[0].materialIndex].name == "tri_mat");
}
