// render_item_geometry_test.cpp — Geometry payload selection.
#include "RenderItem.hpp"
#include "PackedMesh.hpp"
#include "doctest/doctest.h"
#include <cstdint>
#include <vector>

using namespace x3d::runtime::extract;

TEST_CASE("render_item_geometry_test") {
  // Default Geometry is AoS
  Geometry g;
  CHECK((g.kind == Geometry::Kind::AoS));
  CHECK((!g.is_packed()));

  // Packed geometry reports is_packed()
  Geometry gp;
  gp.kind = Geometry::Kind::Packed;
  CHECK((gp.is_packed()));

  return;
}
