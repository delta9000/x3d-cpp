#include "fixture_source.hpp"
#include "doctest/doctest.h"
using namespace x3d::asset_import;

TEST_CASE("fixture_cube_has_one_mesh_and_material") {
  FixtureSource src;
  ImportScene s = src.load("cube");
  CHECK(s.meshes.size() == 1);
  CHECK(s.meshes[0].indices.size() == 36);      // 12 triangles
  CHECK(s.meshes[0].positions.size() == 24);     // 6 faces * 4 verts
  CHECK(s.materials.size() == 1);
  CHECK(s.rootNode >= 0);
}
TEST_CASE("fixture_lit_has_camera_and_lights") {
  FixtureSource src;
  ImportScene s = src.load("lit");
  CHECK(s.cameras.size() >= 1);
  CHECK(s.lights.size() >= 1);
}
