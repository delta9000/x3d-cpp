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
