// StlReader.hpp — Binary STL → PackedMesh codec.
// Namespace: x3d::runtime::ext. Header-only; no core deps beyond PackedMesh/Aabb.
//
// One-way dep: ext→core (includes PackedMesh, Aabb from runtime/extract/).
// Core MUST NEVER include this header. Part of the runtime/ext/ quarantine
// (x3d_cpp_ext target, default OFF, gated by X3D_CPP_BUILD_EXT).
//
// Binary STL format (all little-endian):
//   [0,80)       = 80-byte freeform header (ignored)
//   [80,84)      = uint32_t triangle count N
//   then N × 50-byte facet records:
//     float32[3] = facet normal (one normal per triangle, replicated to 3 verts)
//     float32[3] = v0
//     float32[3] = v1
//     float32[3] = v2
//     uint16_t   = attribute byte count (non-zero is unusual; always ignored)
//
// Output: a non-indexed PackedMesh (index_count=0, vertex_count=N*3).
// Two Float32 attributes: Position (3 floats/vert) and Normal (3 floats/vert).
// Topology=Triangles, ccw=true, solid=true, bounds computed from all positions.
//
// Robustness: len < 84 or len < 84+N*50 → returns empty PackedMesh (no throw).
//
// Detection note: binary STLs may begin with "solid", identical to ASCII prefix.
// Prefer size-based detection (len == 84 + N*50) over keyword sniffing when
// distinguishing binary from ASCII. ASCII STL is deferred (see TODO below).
//
// TODO ASCII STL: add parseStlAscii(const char* text, std::size_t len) for the
//   ASCII variant ("solid name\n  facet normal … \n  outer loop\n …"); needs a
//   simple line-scanner. Not priority — binary is the common path for DCC tools.
#ifndef X3D_RUNTIME_EXT_CODECS_STL_READER_HPP
#define X3D_RUNTIME_EXT_CODECS_STL_READER_HPP

#include "PackedMesh.hpp" // x3d::runtime::extract::{PackedMesh, VertexBufferView, …}

#include <cstdint>
#include <cstring>
#include <vector>

namespace x3d::runtime::ext {

// ---------------------------------------------------------------------------
// parseStlBinary — core implementation.
// Returns an empty PackedMesh on malformed/truncated input (never throws).
// ---------------------------------------------------------------------------
inline x3d::runtime::extract::PackedMesh
parseStlBinary(const std::uint8_t* data, std::size_t len) noexcept {
    using namespace x3d::runtime::extract;

    // Empty mesh sentinel — returned on any malformed input.
    PackedMesh empty_result;

    // Need at least 80-byte header + 4-byte count.
    if (!data || len < 84) return empty_result;

    // Triangle count at offset 80 (little-endian uint32).
    uint32_t N;
    std::memcpy(&N, data + 80, sizeof(uint32_t));

    // Each record is 50 bytes; verify the buffer covers all N records.
    // Use 64-bit arithmetic to avoid overflow on a maliciously large N.
    const uint64_t required = static_cast<uint64_t>(84) + static_cast<uint64_t>(N) * 50u;
    if (required > static_cast<uint64_t>(len)) return empty_result;

    // Zero triangles → produce a valid-but-empty PackedMesh.
    if (N == 0) return empty_result;

    const uint32_t vertex_count = N * 3u;

    // -------------------------------------------------------------------------
    // Allocate attribute byte buffers.
    //   Position: vertex_count × 3 floats
    //   Normal:   vertex_count × 3 floats
    // -------------------------------------------------------------------------
    const std::size_t float3_bytes = static_cast<std::size_t>(vertex_count) * 3u * sizeof(float);

    std::vector<uint8_t> pos_bytes(float3_bytes);
    std::vector<uint8_t> nor_bytes(float3_bytes);

    x3d::runtime::Aabb bounds;

    // Decode each 50-byte facet record.
    const uint8_t* rec = data + 84;
    for (uint32_t t = 0; t < N; ++t, rec += 50) {
        // facet normal at rec[0..11]
        float nx, ny, nz;
        std::memcpy(&nx, rec +  0, 4);
        std::memcpy(&ny, rec +  4, 4);
        std::memcpy(&nz, rec +  8, 4);

        // 3 vertices at rec[12..47]; uint16 attribute bytes at rec[48..49] (ignored).
        for (int v = 0; v < 3; ++v) {
            float vx, vy, vz;
            std::memcpy(&vx, rec + 12 + v * 12 + 0, 4);
            std::memcpy(&vy, rec + 12 + v * 12 + 4, 4);
            std::memcpy(&vz, rec + 12 + v * 12 + 8, 4);

            const uint32_t vidx = t * 3u + static_cast<uint32_t>(v);
            const std::size_t byte_off = static_cast<std::size_t>(vidx) * 12u; // 3 floats × 4 bytes

            // Write position
            std::memcpy(pos_bytes.data() + byte_off + 0, &vx, 4);
            std::memcpy(pos_bytes.data() + byte_off + 4, &vy, 4);
            std::memcpy(pos_bytes.data() + byte_off + 8, &vz, 4);

            // Write normal (facet normal replicated)
            std::memcpy(nor_bytes.data() + byte_off + 0, &nx, 4);
            std::memcpy(nor_bytes.data() + byte_off + 4, &ny, 4);
            std::memcpy(nor_bytes.data() + byte_off + 8, &nz, 4);

            // Expand AABB
            bounds.expand({vx, vy, vz});
        }
    }

    // -------------------------------------------------------------------------
    // Populate the PackedMesh.
    // -------------------------------------------------------------------------
    PackedMesh mesh;
    mesh.vertex_count = vertex_count;
    mesh.index_count  = 0;          // non-indexed
    mesh.topology     = Topology::Triangles;
    mesh.ccw          = true;
    mesh.solid        = true;
    mesh.bounds       = bounds;

    // Position attribute
    {
        VertexBufferView pv;
        pv.component_type        = ComponentType::Float;
        pv.components_per_vertex = 3;
        pv.vertex_count          = vertex_count;
        pv.byte_offset           = 0;
        pv.byte_stride           = 0; // tightly packed
        pv.normalized            = false;
        mesh.set_attrib(VertexAttrib::Position, pv, std::move(pos_bytes));
    }

    // Normal attribute
    {
        VertexBufferView nv;
        nv.component_type        = ComponentType::Float;
        nv.components_per_vertex = 3;
        nv.vertex_count          = vertex_count;
        nv.byte_offset           = 0;
        nv.byte_stride           = 0; // tightly packed
        nv.normalized            = false;
        mesh.set_attrib(VertexAttrib::Normal, nv, std::move(nor_bytes));
    }

    return mesh;
}

// ---------------------------------------------------------------------------
// Convenience overload: accepts an owned byte vector (common in tests / loaders).
// ---------------------------------------------------------------------------
inline x3d::runtime::extract::PackedMesh
parseStlBinary(const std::vector<std::uint8_t>& data) noexcept {
    return parseStlBinary(data.data(), data.size());
}

} // namespace x3d::runtime::ext
#endif // X3D_RUNTIME_EXT_CODECS_STL_READER_HPP
