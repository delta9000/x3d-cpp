// tools/x3d-cli/stl_write.hpp
// ─────────────────────────────────────────────────────────────────────────────
// Binary STL writer — the exact inverse of runtime/ext/codecs/StlReader.hpp.
//
// Format (all little-endian, per ISO STL binary spec):
//   [0,80)      80-byte freeform header ("X3D extract\0" + zero padding)
//   [80,84)     uint32_t triangle count N
//   then N × 50-byte facet records:
//     float32[3]   facet normal (geometric; computed from vertices if not supplied)
//     float32[3]   v0
//     float32[3]   v1
//     float32[3]   v2
//     uint16_t     attribute byte count = 0
//
// Usage:
//   std::string bytes = x3d::cli::writeStlBinary(positions, normals);
//   // positions: flat array of SFVec3f, 3 per triangle (world-space, expanded).
//   // normals:   flat array matching positions (one per vertex, same index);
//   //            may be empty — geometric normals are computed automatically.
//
// Core placement (NOT behind X3D_CPP_BUILD_EXT):
//   STL export is a consumer-side operation, not an X3D node/dialect extension.
//   No dependency on runtime/ext/. Depends only on X3Dtypes (SFVec3f).
//
// StlReader compatibility: every byte written here is readable by
//   parseStlBinary() in runtime/ext/codecs/StlReader.hpp (the self-oracle).
// ─────────────────────────────────────────────────────────────────────────────
#ifndef X3D_CLI_STL_WRITE_HPP
#define X3D_CLI_STL_WRITE_HPP

#include "x3d/sdk.hpp" // SFVec3f (via X3Dtypes)

#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace x3d::cli {

// ---------------------------------------------------------------------------
// writeStlBinary
//
// positions  — flat array of world-space SFVec3f; must have size() % 3 == 0.
//              Every 3 consecutive entries are v0, v1, v2 of one triangle.
// normals    — optional per-vertex normals (same length as positions).
//              If empty or shorter than positions, geometric normals are used.
//
// Returns the raw binary STL bytes as a std::string (may contain null bytes).
// Throws std::invalid_argument if positions.size() % 3 != 0.
// ---------------------------------------------------------------------------
inline std::string writeStlBinary(const std::vector<SFVec3f> &positions,
                                  const std::vector<SFVec3f> &normals = {}) {
    if (positions.size() % 3 != 0)
        throw std::invalid_argument(
            "writeStlBinary: positions.size() must be a multiple of 3");

    const uint32_t N = static_cast<uint32_t>(positions.size() / 3);

    // Total size: 80 (header) + 4 (count) + N * 50 (records).
    const std::size_t total = 84u + static_cast<std::size_t>(N) * 50u;
    std::string buf(total, '\0');
    uint8_t *out = reinterpret_cast<uint8_t *>(buf.data());

    // --- 80-byte header ---
    static const char kHeader[] = "X3D extract";
    std::memcpy(out, kHeader, sizeof(kHeader) - 1); // no null terminator needed
    // Rest of header is already zero-filled.

    // --- uint32_t triangle count at offset 80 ---
    std::memcpy(out + 80, &N, 4);

    const bool hasNormals = (normals.size() >= positions.size());

    // --- facet records starting at offset 84 ---
    uint8_t *rec = out + 84;
    for (uint32_t t = 0; t < N; ++t, rec += 50) {
        const SFVec3f &v0 = positions[t * 3 + 0];
        const SFVec3f &v1 = positions[t * 3 + 1];
        const SFVec3f &v2 = positions[t * 3 + 2];

        // Compute (or use supplied) facet normal.
        float nx, ny, nz;
        if (hasNormals) {
            // Average the three per-vertex normals to get one facet normal.
            nx = (normals[t * 3 + 0].x + normals[t * 3 + 1].x + normals[t * 3 + 2].x) / 3.0f;
            ny = (normals[t * 3 + 0].y + normals[t * 3 + 1].y + normals[t * 3 + 2].y) / 3.0f;
            nz = (normals[t * 3 + 0].z + normals[t * 3 + 1].z + normals[t * 3 + 2].z) / 3.0f;
            // Re-normalize to unit length.
            const float len2 = nx * nx + ny * ny + nz * nz;
            if (len2 > 1e-12f) {
                const float inv = 1.0f / std::sqrt(len2);
                nx *= inv; ny *= inv; nz *= inv;
            }
        } else {
            // Geometric normal: (v1-v0) × (v2-v0), normalized.
            const float ax = v1.x - v0.x, ay = v1.y - v0.y, az = v1.z - v0.z;
            const float bx = v2.x - v0.x, by = v2.y - v0.y, bz = v2.z - v0.z;
            nx = ay * bz - az * by;
            ny = az * bx - ax * bz;
            nz = ax * by - ay * bx;
            const float len2 = nx * nx + ny * ny + nz * nz;
            if (len2 > 1e-12f) {
                const float inv = 1.0f / std::sqrt(len2);
                nx *= inv; ny *= inv; nz *= inv;
            }
            // If degenerate (zero-area triangle), normal stays (0,0,0) — valid STL.
        }

        // Write facet normal at rec[0..11].
        std::memcpy(rec +  0, &nx, 4);
        std::memcpy(rec +  4, &ny, 4);
        std::memcpy(rec +  8, &nz, 4);

        // Write v0 at rec[12..23].
        std::memcpy(rec + 12, &v0.x, 4);
        std::memcpy(rec + 16, &v0.y, 4);
        std::memcpy(rec + 20, &v0.z, 4);

        // Write v1 at rec[24..35].
        std::memcpy(rec + 24, &v1.x, 4);
        std::memcpy(rec + 28, &v1.y, 4);
        std::memcpy(rec + 32, &v1.z, 4);

        // Write v2 at rec[36..47].
        std::memcpy(rec + 36, &v2.x, 4);
        std::memcpy(rec + 40, &v2.y, 4);
        std::memcpy(rec + 44, &v2.z, 4);

        // Attribute byte count at rec[48..49] = 0 (already zero-filled).
    }

    return buf;
}

} // namespace x3d::cli

#endif // X3D_CLI_STL_WRITE_HPP
