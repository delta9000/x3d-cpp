// PackedMesh.hpp — Phase 1 binary geometry descriptor. namespace
// x3d::runtime::extract. Header-only, no node deps. An owned-slab GPU-upload-
// ready mesh that an embedder's external-geometry resolver (externalGeometryResolver
// seam) can return for geometry types the AoS MeshBuilder cannot tessellate.
//
// Layout follows glTF 2.0 accessor/bufferView conventions so a consumer can
// feed the slabs directly into GL buffer objects without further conversion.
// Each attribute occupies its own buffer (attribute_data[i]) so attributes can
// be uploaded independently; index_data is a separate owned slab.
//
// DESIGN: this is a DESCRIPTOR, not a renderer. The consumer owns the GPU side.
// The PackedMesh is allocated once, passed to the consumer via the RenderItem,
// and may be moved/copied freely (std::vector<uint8_t> owns the bytes).
#ifndef X3D_RUNTIME_EXTRACT_PACKED_MESH_HPP
#define X3D_RUNTIME_EXTRACT_PACKED_MESH_HPP

#include "Aabb.hpp"     // x3d::runtime::Aabb
#include "Topology.hpp" // x3d::runtime::extract::Topology

#include <array>
#include <cassert>
#include <cstdint>
#include <vector>

namespace x3d::runtime::extract {

// ---------------------------------------------------------------------------
// ComponentType — glTF 2.0 accessor component types (§5.1.3).
// Values match the glTF spec constants so a consumer can pass them directly
// to GL (e.g. GL_FLOAT = 0x1406 = 5126). No HalfFloat: not supported at
// Phase 1 (avoids a GL_HALF_FLOAT dependency; add in Phase 2+ if needed).
// ---------------------------------------------------------------------------
enum class ComponentType : uint16_t {
  Byte        = 5120, // GL_BYTE         — signed 8-bit integer
  UByte       = 5121, // GL_UNSIGNED_BYTE — unsigned 8-bit integer
  Short       = 5122, // GL_SHORT         — signed 16-bit integer
  UShort      = 5123, // GL_UNSIGNED_SHORT — unsigned 16-bit integer
  UnsignedInt = 5125, // GL_UNSIGNED_INT  — unsigned 32-bit integer (5124 = HalfFloat, skipped)
  Float       = 5126, // GL_FLOAT         — 32-bit IEEE 754 float
};

// ---------------------------------------------------------------------------
// VertexAttrib — semantic attribute slot. 8 slots (fits a uint8_t bitmask).
// The enum value is ALSO the index into PackedMesh::attribute_views and the
// bit position in PackedMesh::attrib_mask.
// ---------------------------------------------------------------------------
enum class VertexAttrib : uint8_t {
  Position  = 0, // XYZ (3 floats by default)
  Normal    = 1, // XYZ unit normal (3 floats)
  Tangent   = 2, // XYZW tangent (4 floats; W = ±1 for bitangent sign)
  TexCoord0 = 3, // UV set 0 (2 floats)
  TexCoord1 = 4, // UV set 1 (2 floats)
  Color     = 5, // RGBA per-vertex color (4 floats or normalized UByte)
  Joints    = 6, // skinning joint indices (Phase 2+)
  Weights   = 7, // skinning blend weights (Phase 2+)
};

// ---------------------------------------------------------------------------
// VertexBufferView — a typed slice into one of PackedMesh::attribute_data.
// Mirrors glTF accessor+bufferView fused into one struct.
// ---------------------------------------------------------------------------
struct VertexBufferView {
  uint32_t buffer_index       = 0;                    // Index into PackedMesh::attribute_data.
  uint32_t byte_offset        = 0;                    // Byte offset within the buffer.
  uint32_t byte_stride        = 0;                    // 0 = tightly packed (compute from component).
  ComponentType component_type = ComponentType::Float; // Underlying scalar type.
  uint8_t  components_per_vertex = 3;                 // How many scalars per vertex (1..4).
  bool     normalized          = false;               // If true, int types map to [0,1]/[-1,1].
  uint32_t vertex_count        = 0;                   // Number of vertices in this view.

  // Convenience: get a raw pointer into the owning PackedMesh's attribute_data.
  // For attribute views, buffer_index identifies the entry in attribute_data.
  // Index bytes are ALWAYS accessed via PackedMesh::index_data directly — do
  // NOT pass index_view here; there is no buffer_index sentinel for index_data.
  const uint8_t* data_ptr(const std::vector<std::vector<uint8_t>>& bufs) const {
    assert(buffer_index < bufs.size() && "buffer_index out of range — do not pass index_view to data_ptr");
    return bufs[buffer_index].data();
  }
};

// ---------------------------------------------------------------------------
// PackedMesh — owned-slab binary mesh. All attribute bytes live in
// attribute_data (one sub-vector per attribute); index bytes in index_data.
// The attribute_views array is parallel to VertexAttrib; valid entries are
// flagged by attrib_mask.
// ---------------------------------------------------------------------------
struct PackedMesh {
  // Owned per-attribute byte buffers. Each set_attrib() call appends one entry.
  std::vector<std::vector<uint8_t>> attribute_data;

  // Owned index bytes. Empty = non-indexed draw. If present, index_view describes
  // the layout (component_type = UShort or UnsignedInt).
  std::vector<uint8_t> index_data;

  // Attribute views — one per VertexAttrib slot. Valid only when the corresponding
  // bit in attrib_mask is set (i.e. has(VertexAttrib::X) returns true).
  std::array<VertexBufferView, 8> attribute_views{};

  // Bitmask of present attributes. Bit i set ⟺ attribute_views[i] is valid.
  uint8_t attrib_mask = 0;

  // Index view. Describes the layout of index_data (component_type = UShort or
  // UnsignedInt). Index bytes are read from index_data directly — do NOT pass
  // index_view to VertexBufferView::data_ptr. Ignored when index_count == 0.
  VertexBufferView index_view{};

  uint32_t vertex_count = 0;
  uint32_t index_count  = 0;

  Topology topology = Topology::Triangles;

  // Bounding box in the mesh's LOCAL frame. The extractor does NOT compute
  // this from the slab — it is the responsibility of the resolver that produced
  // the PackedMesh to fill it in (or leave it at default empty Aabb).
  x3d::runtime::Aabb bounds{};

  bool ccw   = true;  // winding order (X3D default CCW front-face)
  bool solid = true;  // true = back-face cullable

  // TODO: skinning fields (joints/weights/inverse_bind_matrices) — Phase 2+
  //       when glTF skinned mesh lands.

  // ---------------------------------------------------------------------------
  // Predicate helpers
  // ---------------------------------------------------------------------------
  bool has(VertexAttrib a) const { return (attrib_mask >> uint8_t(a)) & 1; }
  bool is_indexed()         const { return index_count > 0; }
  bool empty()              const { return vertex_count == 0; }

  // ---------------------------------------------------------------------------
  // set_attrib — convenience builder method.
  // Appends `data` to attribute_data, sets view.buffer_index, stores the view
  // in attribute_views[slot], and sets the corresponding bit in attrib_mask.
  // ---------------------------------------------------------------------------
  void set_attrib(VertexAttrib a, VertexBufferView view, std::vector<uint8_t> data) {
    view.buffer_index = static_cast<uint32_t>(attribute_data.size());
    attribute_data.push_back(std::move(data));
    attribute_views[static_cast<uint8_t>(a)] = view;
    attrib_mask |= static_cast<uint8_t>(1u << static_cast<uint8_t>(a));
  }
};

} // namespace x3d::runtime::extract
#endif // X3D_RUNTIME_EXTRACT_PACKED_MESH_HPP
