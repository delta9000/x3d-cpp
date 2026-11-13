// Binary STL decoding from synthesized little-endian facet records.
#include "doctest/doctest.h"
#include "StlReader.hpp"

#include "PackedMesh.hpp"  // VertexAttrib, ComponentType, PackedMesh

#include <array>
#include <cstring>
#include <cstdint>
#include <vector>

// Helper: write a little-endian uint32_t into a buffer at byte offset `off`.

static void write_u32(std::vector<uint8_t>& buf, std::size_t off, uint32_t v) {
    buf[off+0] = static_cast<uint8_t>(v      );
    buf[off+1] = static_cast<uint8_t>(v >>  8);
    buf[off+2] = static_cast<uint8_t>(v >> 16);
    buf[off+3] = static_cast<uint8_t>(v >> 24);
}

// Helper: write a float in little-endian at byte offset `off`.
static void write_f32(std::vector<uint8_t>& buf, std::size_t off, float v) {
    uint32_t bits;
    std::memcpy(&bits, &v, 4);
    write_u32(buf, off, bits);
}

// Helper: write a uint16_t in little-endian at byte offset `off`.
static void write_u16(std::vector<uint8_t>& buf, std::size_t off, uint16_t v) {
    buf[off+0] = static_cast<uint8_t>(v     );
    buf[off+1] = static_cast<uint8_t>(v >> 8);
}

// build_stl_binary — builds a synthetic binary STL with N=2 triangles.
// Triangle 0: normal (0,0,1), verts (0,0,0), (1,0,0), (0,1,0)
// Triangle 1: normal (0,1,0), verts (2,0,0), (3,0,0), (2,1,0)

static std::vector<uint8_t> build_stl_binary() {
    const uint32_t N = 2;
    std::vector<uint8_t> buf(80 + 4 + N * 50, 0);

    // Header: "STL test" in the first bytes, rest zero.
    const char* hdr = "STL test";
    std::memcpy(buf.data(), hdr, 8);

    // Triangle count
    write_u32(buf, 80, N);

    // Triangle 0 at offset 84
    std::size_t off = 84;
    write_f32(buf, off+ 0,  0.0f); // normal x
    write_f32(buf, off+ 4,  0.0f); // normal y
    write_f32(buf, off+ 8,  1.0f); // normal z
    write_f32(buf, off+12,  0.0f); // v0 x
    write_f32(buf, off+16,  0.0f); // v0 y
    write_f32(buf, off+20,  0.0f); // v0 z
    write_f32(buf, off+24,  1.0f); // v1 x
    write_f32(buf, off+28,  0.0f); // v1 y
    write_f32(buf, off+32,  0.0f); // v1 z
    write_f32(buf, off+36,  0.0f); // v2 x
    write_f32(buf, off+40,  1.0f); // v2 y
    write_f32(buf, off+44,  0.0f); // v2 z
    write_u16(buf, off+48,  0);    // attribute byte count (ignored)

    // Triangle 1 at offset 134
    off = 84 + 50;
    write_f32(buf, off+ 0,  0.0f); // normal x
    write_f32(buf, off+ 4,  1.0f); // normal y
    write_f32(buf, off+ 8,  0.0f); // normal z
    write_f32(buf, off+12,  2.0f); // v0 x
    write_f32(buf, off+16,  0.0f); // v0 y
    write_f32(buf, off+20,  0.0f); // v0 z
    write_f32(buf, off+24,  3.0f); // v1 x
    write_f32(buf, off+28,  0.0f); // v1 y
    write_f32(buf, off+32,  0.0f); // v1 z
    write_f32(buf, off+36,  2.0f); // v2 x
    write_f32(buf, off+40,  1.0f); // v2 y
    write_f32(buf, off+44,  0.0f); // v2 z
    write_u16(buf, off+48,  0);    // attribute byte count (ignored)

    return buf;
}

// Helper: read a float from a byte buffer at a given byte offset.

static float read_f32_at(const uint8_t* base, std::size_t byte_offset) {
    float v;
    std::memcpy(&v, base + byte_offset, sizeof(float));
    return v;
}

TEST_CASE("a nominal two-triangle binary STL") {
    using namespace x3d::runtime::ext;
    using namespace x3d::runtime::extract;

    // nominal two-triangle binary STL

    auto stl = build_stl_binary();
    PackedMesh mesh = parseStlBinary(stl.data(), stl.size());

    // vertex_count == N*3 (non-indexed, 3 verts per triangle)
    REQUIRE((mesh.vertex_count == 6 && "vertex_count must be N*3"));

    // Non-indexed: index_count == 0
    REQUIRE((mesh.index_count == 0 && "non-indexed: index_count must be 0"));

    // Position attribute present
    REQUIRE((mesh.has(VertexAttrib::Position) && "Position attribute must be set"));

    // Normal attribute present
    REQUIRE((mesh.has(VertexAttrib::Normal) && "Normal attribute must be set"));

    // Topology == Triangles
    REQUIRE((mesh.topology == Topology::Triangles && "topology must be Triangles"));

    // winding/culling defaults
    REQUIRE((mesh.ccw   == true  && "ccw must be true"));
    REQUIRE((mesh.solid == true  && "solid must be true"));

    // Verify known position values via VertexBufferView + attribute_data

    const VertexBufferView& pv = mesh.attribute_views[static_cast<uint8_t>(VertexAttrib::Position)];
    REQUIRE((pv.component_type == ComponentType::Float));
    REQUIRE((pv.components_per_vertex == 3));
    REQUIRE((pv.vertex_count == 6));

    const uint8_t* pos_base = pv.data_ptr(mesh.attribute_data) + pv.byte_offset;

    // Triangle 0, vertex 0 = (0,0,0)
    REQUIRE((read_f32_at(pos_base,  0) == 0.0f));
    REQUIRE((read_f32_at(pos_base,  4) == 0.0f));
    REQUIRE((read_f32_at(pos_base,  8) == 0.0f));
    // Triangle 0, vertex 1 = (1,0,0)
    REQUIRE((read_f32_at(pos_base, 12) == 1.0f));
    REQUIRE((read_f32_at(pos_base, 16) == 0.0f));
    REQUIRE((read_f32_at(pos_base, 20) == 0.0f));
    // Triangle 0, vertex 2 = (0,1,0)
    REQUIRE((read_f32_at(pos_base, 24) == 0.0f));
    REQUIRE((read_f32_at(pos_base, 28) == 1.0f));
    REQUIRE((read_f32_at(pos_base, 32) == 0.0f));
    // Triangle 1, vertex 0 = (2,0,0)
    REQUIRE((read_f32_at(pos_base, 36) == 2.0f));
    REQUIRE((read_f32_at(pos_base, 40) == 0.0f));
    REQUIRE((read_f32_at(pos_base, 44) == 0.0f));

    // Verify normal: the facet normal replicated to all 3 verts of each triangle
    // Triangle 0 normal = (0,0,1); Triangle 1 normal = (0,1,0)

    const VertexBufferView& nv = mesh.attribute_views[static_cast<uint8_t>(VertexAttrib::Normal)];
    REQUIRE((nv.component_type == ComponentType::Float));
    REQUIRE((nv.components_per_vertex == 3));
    REQUIRE((nv.vertex_count == 6));

    const uint8_t* nor_base = nv.data_ptr(mesh.attribute_data) + nv.byte_offset;

    // Triangle 0: all 3 verts share normal (0,0,1)
    for (int i = 0; i < 3; ++i) {
        REQUIRE((read_f32_at(nor_base, (i*3+0)*4) == 0.0f)); // nx
        REQUIRE((read_f32_at(nor_base, (i*3+1)*4) == 0.0f)); // ny
        REQUIRE((read_f32_at(nor_base, (i*3+2)*4) == 1.0f)); // nz
    }
    // Triangle 1: all 3 verts share normal (0,1,0)
    for (int i = 3; i < 6; ++i) {
        REQUIRE((read_f32_at(nor_base, (i*3+0)*4) == 0.0f)); // nx
        REQUIRE((read_f32_at(nor_base, (i*3+1)*4) == 1.0f)); // ny
        REQUIRE((read_f32_at(nor_base, (i*3+2)*4) == 0.0f)); // nz
    }

    // Verify AABB: positions span (0,0,0)-(3,1,0)

    const auto& bb = mesh.bounds;
    REQUIRE((!bb.empty && "bounds must not be empty"));
    REQUIRE((bb.min.x == 0.0f && bb.min.y == 0.0f && bb.min.z == 0.0f));
    REQUIRE((bb.max.x == 3.0f && bb.max.y == 1.0f && bb.max.z == 0.0f));
}

TEST_CASE("input shorter than the header gives an empty mesh") {
    using namespace x3d::runtime::ext;
    using namespace x3d::runtime::extract;

    // malformed / truncated input → empty PackedMesh, no crash

    // Buffer shorter than the 84-byte minimum header
    std::vector<uint8_t> too_short(10, 0);
    PackedMesh mesh = parseStlBinary(too_short.data(), too_short.size());
    REQUIRE((mesh.empty() && "truncated (<84 bytes) input must produce empty PackedMesh"));
    REQUIRE((mesh.vertex_count == 0));
    REQUIRE((mesh.index_count  == 0));
}

TEST_CASE("a truncated triangle list gives an empty mesh") {
    using namespace x3d::runtime::ext;
    using namespace x3d::runtime::extract;

    // Buffer has header + N=3 claimed triangles but only 1 triangle's worth of data
    std::vector<uint8_t> partial(80 + 4 + 1 * 50, 0);
    write_u32(partial, 80, 3); // claim 3 triangles
    PackedMesh mesh = parseStlBinary(partial.data(), partial.size());
    REQUIRE((mesh.empty() && "partially-truncated input must produce empty PackedMesh"));
}

TEST_CASE("null input gives an empty mesh") {
    using namespace x3d::runtime::ext;
    using namespace x3d::runtime::extract;

    // Null pointer + zero length: should return empty without crashing
    PackedMesh mesh = parseStlBinary(nullptr, 0);
    REQUIRE((mesh.empty() && "null/0-length input must produce empty PackedMesh"));
}

TEST_CASE("the vector overload matches") {
    using namespace x3d::runtime::ext;
    using namespace x3d::runtime::extract;

    // convenience overload — span-via-vector

    auto stl = build_stl_binary();
    PackedMesh mesh = parseStlBinary(stl);
    REQUIRE((mesh.vertex_count == 6 && "vector overload must produce same result"));
}
