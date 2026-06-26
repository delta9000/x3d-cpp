// render_item_geometry_test.cpp — Phase 1: Geometry union + TextureRef::Buffer.
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

  // TextureRef Source::Buffer
  TextureRef ref;
  ref.source = TextureRef::Source::Buffer;
  ref.bufferBytes = {0xDE, 0xAD, 0xBE, 0xEF};
  ref.mimeHint = "image/png";
  CHECK((ref.source == TextureRef::Source::Buffer));
  CHECK((ref.bufferBytes.size() == 4));
  CHECK((ref.mimeHint == "image/png"));
  CHECK((!ref.resolved_desc.has_value()));

  return;
}
