// packed_mesh_test.cpp — Phase 1: PackedMesh struct round-trip.
#include "PackedMesh.hpp"
#include "doctest/doctest.h"
#include <cstdint>
#include <cstring>

using namespace x3d::runtime::extract;

TEST_CASE("packed_mesh_test") {
  // Build a triangle: 3 Float32 positions + uint16 indices
  PackedMesh m;

  // Set up position attribute
  std::vector<uint8_t> pos_bytes(3 * 3 * sizeof(float));
  float positions[9] = {0,0,0, 1,0,0, 0,1,0};
  std::memcpy(pos_bytes.data(), positions, pos_bytes.size());

  VertexBufferView pos_view;
  pos_view.component_type = ComponentType::Float;
  pos_view.components_per_vertex = 3;
  pos_view.vertex_count = 3;
  pos_view.byte_offset = 0;
  pos_view.byte_stride = 0; // tightly packed
  m.set_attrib(VertexAttrib::Position, pos_view, std::move(pos_bytes));

  // Set up indices
  std::vector<uint8_t> idx_bytes(3 * sizeof(uint16_t));
  uint16_t indices[3] = {0, 1, 2};
  std::memcpy(idx_bytes.data(), indices, idx_bytes.size());
  m.index_data = std::move(idx_bytes);
  m.index_view.component_type = ComponentType::UShort;
  m.index_view.components_per_vertex = 1;
  m.index_view.vertex_count = 3;
  m.index_count = 3;
  m.vertex_count = 3;

  // Verify predicates
  CHECK((m.has(VertexAttrib::Position)));
  CHECK((!m.has(VertexAttrib::Normal)));
  CHECK((m.is_indexed()));
  CHECK((!m.empty()));

  // Verify component type values (glTF spec)
  CHECK((static_cast<uint16_t>(ComponentType::Byte) == 5120));
  CHECK((static_cast<uint16_t>(ComponentType::UByte) == 5121));
  CHECK((static_cast<uint16_t>(ComponentType::Short) == 5122));
  CHECK((static_cast<uint16_t>(ComponentType::UShort) == 5123));
  CHECK((static_cast<uint16_t>(ComponentType::UnsignedInt) == 5125));
  CHECK((static_cast<uint16_t>(ComponentType::Float) == 5126));

  // Verify data round-trip via data_ptr
  const VertexBufferView& pv = m.attribute_views[static_cast<uint8_t>(VertexAttrib::Position)];
  const uint8_t* ptr = pv.data_ptr(m.attribute_data);
  float read_pos[9];
  std::memcpy(read_pos, ptr + pv.byte_offset, 3 * 3 * sizeof(float));
  CHECK((read_pos[3] == 1.0f)); // x of vertex 1

  // -------------------------------------------------------------------------
  // Ownership-survival: COPY
  // Copy m into m2, then destroy m's data. m2 must remain independently valid.
  // -------------------------------------------------------------------------
  PackedMesh m2 = m; // deep copy (std::vector members own their bytes)
  {
    // Destroy source data to prove m2 is independent.
    m.attribute_data.clear();
    m.index_data.clear();
  }
  // m2's position view must still resolve correctly against m2.attribute_data.
  {
    const VertexBufferView& pv2 = m2.attribute_views[static_cast<uint8_t>(VertexAttrib::Position)];
    const uint8_t* ptr2 = pv2.data_ptr(m2.attribute_data);
    float read_pos2[9];
    std::memcpy(read_pos2, ptr2 + pv2.byte_offset, 3 * 3 * sizeof(float));
    CHECK((read_pos2[3] == 1.0f)); // x of vertex 1 — survives source destruction
  }
  CHECK((m2.has(VertexAttrib::Position)));
  CHECK((m2.is_indexed()));

  // -------------------------------------------------------------------------
  // Ownership-survival: MOVE
  // Move a freshly-built PackedMesh into m3; original views must still resolve.
  // -------------------------------------------------------------------------
  PackedMesh m3;
  {
    PackedMesh src;
    std::vector<uint8_t> pos3_bytes(3 * 3 * sizeof(float));
    float positions3[9] = {0,0,0, 2,0,0, 0,2,0};
    std::memcpy(pos3_bytes.data(), positions3, pos3_bytes.size());
    VertexBufferView pv3;
    pv3.component_type = ComponentType::Float;
    pv3.components_per_vertex = 3;
    pv3.vertex_count = 3;
    src.set_attrib(VertexAttrib::Position, pv3, std::move(pos3_bytes));
    src.vertex_count = 3;
    m3 = std::move(src); // src is now in a valid-but-unspecified state
  }
  // m3 now owns the data; resolve its view.
  {
    const VertexBufferView& pv3 = m3.attribute_views[static_cast<uint8_t>(VertexAttrib::Position)];
    const uint8_t* ptr3 = pv3.data_ptr(m3.attribute_data);
    float read_pos3[9];
    std::memcpy(read_pos3, ptr3 + pv3.byte_offset, 3 * 3 * sizeof(float));
    CHECK((read_pos3[3] == 2.0f)); // x of vertex 1 in the moved mesh
  }
  CHECK((m3.has(VertexAttrib::Position)));
  CHECK((!m3.empty()));

  // Default mesh is empty
  PackedMesh empty;
  CHECK((empty.empty()));
  CHECK((!empty.is_indexed()));
  CHECK((!empty.has(VertexAttrib::Position)));

  return;
}
