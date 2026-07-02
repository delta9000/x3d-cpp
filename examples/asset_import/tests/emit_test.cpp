#include "emit.hpp"
#include "fixture_source.hpp"
#include "x3d/authoring.hpp"
#include "doctest/doctest.h"
using namespace x3d::asset_import;

TEST_CASE("emit_cube_produces_shape_with_indexed_triangle_set") {
  ImportScene s = FixtureSource{}.load("cube");
  auto doc = emit(s, {});
  CHECK(doc.version == "4.0");
  CHECK_FALSE(doc.scene.rootNodes.empty());
  const std::string xml = x3d::authoring::XmlWriter{}.writeDocument(doc);
  CHECK(xml.find("<Shape") != std::string::npos);
  CHECK(xml.find("<IndexedTriangleSet") != std::string::npos);
  CHECK(xml.find("<Coordinate") != std::string::npos);
  CHECK(xml.find("<Normal") != std::string::npos);
}

TEST_CASE("emit_phong_material_when_no_pbr") {
  ImportScene s = FixtureSource{}.load("cube");     // cube material has no pbr
  const std::string xml = x3d::authoring::XmlWriter{}.writeDocument(emit(s, {}));
  CHECK(xml.find("<Material") != std::string::npos);
  CHECK(xml.find("<PhysicalMaterial") == std::string::npos);
}

TEST_CASE("emit_physical_material_when_pbr_present") {
  ImportScene s = FixtureSource{}.load("cube");
  s.materials[0].pbr = PbrParams{{0.2f, 0.4f, 0.8f, 1.0f}, 0.0f, 0.5f};
  const std::string xml = x3d::authoring::XmlWriter{}.writeDocument(emit(s, {}));
  CHECK(xml.find("<PhysicalMaterial") != std::string::npos);
}
