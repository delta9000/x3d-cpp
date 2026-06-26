// tools/x3d-cli/stl_write_test.cpp
// ─────────────────────────────────────────────────────────────────────────────
// TDD: binary-STL writer round-trip — writeStlBinary → parseStlBinary.
//
// Gated on X3D_CPP_BUILD_EXT (needs StlReader.hpp from runtime/ext/codecs/).
// The writer (stl_write.hpp) is core (no ext dep); only the read-back here is ext.
//
// Tests:
//   1. Empty triangle list → valid header (84 bytes), triCount=0, readback ok.
//   2. One triangle with known vertices → readback yields 3 vertices, bit-exact.
//   3. Two triangles → vertex_count=6, normals reconstructed and reasonable.
//   4. Geometric normal auto-computation (no normals supplied) — correct direction.
//   5. Supplied normals are carried through (averaged to facet normal, then stored).
// ─────────────────────────────────────────────────────────────────────────────
#include "stl_write.hpp"   // x3d::cli::writeStlBinary
#include "StlReader.hpp"   // x3d::runtime::ext::parseStlBinary

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

using namespace x3d::runtime::ext;
using namespace x3d::runtime::extract; // PackedMesh, VertexAttrib, ComponentType

// ---------------------------------------------------------------------------
// Helper: extract all float values for a given attribute from a PackedMesh.
// Returns a flat array [v0.x, v0.y, v0.z, v1.x, ...].
// ---------------------------------------------------------------------------
static std::vector<float> extractFloats(const PackedMesh &pm, VertexAttrib attr) {
    if (!pm.has(attr)) return {};
    const VertexBufferView &view = pm.attribute_views[static_cast<uint8_t>(attr)];
    const std::vector<uint8_t> &bytes = pm.attribute_data[view.buffer_index];

    const uint32_t n = view.vertex_count;
    const uint32_t comps = view.components_per_vertex;
    std::vector<float> out;
    out.reserve(n * comps);
    for (uint32_t v = 0; v < n; ++v) {
        const std::size_t base = view.byte_offset + v * (view.byte_stride == 0
            ? comps * sizeof(float) : view.byte_stride);
        for (uint32_t c = 0; c < comps; ++c) {
            float f;
            std::memcpy(&f, bytes.data() + base + c * sizeof(float), sizeof(float));
            out.push_back(f);
        }
    }
    return out;
}

static float fabsf_val(float v) { return v < 0.0f ? -v : v; }

static bool near(float a, float b, float tol = 1e-5f) {
    return fabsf_val(a - b) <= tol;
}

// ---------------------------------------------------------------------------
// Test 1: empty list → 84-byte buffer, triCount=0, readback gives empty mesh.
// ---------------------------------------------------------------------------
static void test_empty() {
    std::vector<SFVec3f> empty;
    std::string bytes = x3d::cli::writeStlBinary(empty);

    // Size must be exactly 84.
    assert(bytes.size() == 84 && "empty STL must be exactly 84 bytes");

    // triCount at offset 80 must be 0.
    uint32_t N = 0;
    std::memcpy(&N, bytes.data() + 80, 4);
    assert(N == 0 && "empty STL triCount must be 0");

    // parseStlBinary on an empty STL returns empty PackedMesh.
    PackedMesh pm = parseStlBinary(
        reinterpret_cast<const uint8_t *>(bytes.data()), bytes.size());
    // For N=0, StlReader returns an empty-but-valid PackedMesh.
    assert(pm.vertex_count == 0 && "readback vertex_count must be 0");

    std::cout << "ok:   test_empty\n";
}

// ---------------------------------------------------------------------------
// Test 2: one triangle — vertices round-trip bit-exactly.
// ---------------------------------------------------------------------------
static void test_one_triangle_bit_exact() {
    // CCW triangle in XY plane.
    std::vector<SFVec3f> pos = {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
    };

    std::string bytes = x3d::cli::writeStlBinary(pos);
    assert(bytes.size() == 84u + 50u && "one-tri STL must be 134 bytes");

    PackedMesh pm = parseStlBinary(
        reinterpret_cast<const uint8_t *>(bytes.data()), bytes.size());
    assert(pm.vertex_count == 3 && "one-tri readback vertex_count must be 3");

    std::vector<float> verts = extractFloats(pm, VertexAttrib::Position);
    assert(verts.size() == 9 && "one-tri readback must have 9 position floats");

    // Bit-exact round-trip.
    assert(near(verts[0], 1.0f) && near(verts[1], 0.0f) && near(verts[2], 0.0f));
    assert(near(verts[3], 0.0f) && near(verts[4], 1.0f) && near(verts[5], 0.0f));
    assert(near(verts[6], 0.0f) && near(verts[7], 0.0f) && near(verts[8], 1.0f));

    std::cout << "ok:   test_one_triangle_bit_exact\n";
}

// ---------------------------------------------------------------------------
// Test 3: two triangles — vertex_count = 6.
// ---------------------------------------------------------------------------
static void test_two_triangles_vertex_count() {
    std::vector<SFVec3f> pos = {
        // tri 0 (XY plane, +Z normal)
        {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
        // tri 1 (XZ plane, -Y normal would be one winding; let's use +Y)
        {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f},
    };

    std::string bytes = x3d::cli::writeStlBinary(pos);
    assert(bytes.size() == 84u + 100u && "two-tri STL must be 184 bytes");

    PackedMesh pm = parseStlBinary(
        reinterpret_cast<const uint8_t *>(bytes.data()), bytes.size());
    assert(pm.vertex_count == 6 && "two-tri readback vertex_count must be 6");

    std::cout << "ok:   test_two_triangles_vertex_count\n";
}

// ---------------------------------------------------------------------------
// Test 4: geometric normal auto-computation (no normals supplied).
//   Triangle in XY plane (z=0), CCW → normal should be +Z = (0,0,1).
// ---------------------------------------------------------------------------
static void test_geometric_normal() {
    std::vector<SFVec3f> pos = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
    };

    std::string bytes = x3d::cli::writeStlBinary(pos);
    // Normal is at offset 84 in the facet record.
    float nx, ny, nz;
    std::memcpy(&nx, bytes.data() + 84 +  0, 4);
    std::memcpy(&ny, bytes.data() + 84 +  4, 4);
    std::memcpy(&nz, bytes.data() + 84 +  8, 4);

    assert(near(nx, 0.0f, 1e-4f) && "geometric normal x should be 0");
    assert(near(ny, 0.0f, 1e-4f) && "geometric normal y should be 0");
    assert(near(nz, 1.0f, 1e-4f) && "geometric normal z should be +1");

    std::cout << "ok:   test_geometric_normal\n";
}

// ---------------------------------------------------------------------------
// Test 5: supplied normals → they appear (averaged) in the STL facet normal.
// ---------------------------------------------------------------------------
static void test_supplied_normals() {
    std::vector<SFVec3f> pos = {
        {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
    };
    // All three per-vertex normals point in -Z direction.
    std::vector<SFVec3f> nor = {
        {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f},
    };

    std::string bytes = x3d::cli::writeStlBinary(pos, nor);
    float nx, ny, nz;
    std::memcpy(&nx, bytes.data() + 84 +  0, 4);
    std::memcpy(&ny, bytes.data() + 84 +  4, 4);
    std::memcpy(&nz, bytes.data() + 84 +  8, 4);

    // Averaged + renormalized: still (0,0,-1).
    assert(near(nx, 0.0f, 1e-4f) && "supplied-normal x should be 0");
    assert(near(ny, 0.0f, 1e-4f) && "supplied-normal y should be 0");
    assert(near(nz, -1.0f, 1e-4f) && "supplied-normal z should be -1");

    std::cout << "ok:   test_supplied_normals\n";
}

// ---------------------------------------------------------------------------
// Test 6: invalid size (not divisible by 3) → exception.
// ---------------------------------------------------------------------------
static void test_invalid_size() {
    std::vector<SFVec3f> bad = {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    bool threw = false;
    try {
        x3d::cli::writeStlBinary(bad);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    assert(threw && "writeStlBinary should throw on non-multiple-of-3 positions");
    std::cout << "ok:   test_invalid_size\n";
}

int main() {
    test_empty();
    test_one_triangle_bit_exact();
    test_two_triangles_vertex_count();
    test_geometric_normal();
    test_supplied_normals();
    test_invalid_size();

    std::cout << "\nstl_write_test: all tests passed\n";
    return 0;
}
