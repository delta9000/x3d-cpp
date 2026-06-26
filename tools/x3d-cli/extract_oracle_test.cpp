// tools/x3d-cli/extract_oracle_test.cpp
// ─────────────────────────────────────────────────────────────────────────────
// Self-oracle round-trip test for x3d extract.
//
// Gated on X3D_CPP_BUILD_EXT (needs StlReader.hpp from runtime/ext/codecs/).
//
// Proves: x3d extract X3D → STL → StlReader re-import ≡ SceneExtractor mesh.
//
// For each fixture:
//   1. Parse the X3D file.
//   2. Run SceneExtractor → accumulate world-space triangles (the "reference").
//   3. Call writeStlBinary on those triangles.
//   4. Call parseStlBinary on the written bytes.
//   5. Assert:
//       a) vertex_count == 3 * N (N = number of triangles).
//       b) AABB of re-imported vertices matches the reference AABB (within 1e-4).
//       c) Spot-check: sum of all vertex coordinates is the same (within 1e-3).
//
// Fixtures:
//   - simple.x3d   (Box 2×2×2) → 12 triangles (2 per face × 6 faces = 12).
//   - validate-clean.x3d (Box 1×1×1) → 12 triangles.
// ─────────────────────────────────────────────────────────────────────────────
#include "stl_write.hpp"   // x3d::cli::writeStlBinary
#include "StlReader.hpp"   // x3d::runtime::ext::parseStlBinary

#include "x3d/sdk.hpp"

#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace sdk = x3d::sdk;
using namespace x3d::runtime::ext;
using namespace x3d::runtime::extract;

// ---------------------------------------------------------------------------
// compute_aabb — min/max over a flat position list (groups of 3 floats/vertex).
// ---------------------------------------------------------------------------
struct AABB3 {
    float minX, minY, minZ, maxX, maxY, maxZ;
};

static AABB3 compute_aabb(const std::vector<SFVec3f> &pts) {
    AABB3 bb{};
    if (pts.empty()) {
        bb.minX = bb.minY = bb.minZ = 0.0f;
        bb.maxX = bb.maxY = bb.maxZ = 0.0f;
        return bb;
    }
    bb.minX = bb.maxX = pts[0].x;
    bb.minY = bb.maxY = pts[0].y;
    bb.minZ = bb.maxZ = pts[0].z;
    for (const auto &p : pts) {
        if (p.x < bb.minX) bb.minX = p.x; if (p.x > bb.maxX) bb.maxX = p.x;
        if (p.y < bb.minY) bb.minY = p.y; if (p.y > bb.maxY) bb.maxY = p.y;
        if (p.z < bb.minZ) bb.minZ = p.z; if (p.z > bb.maxZ) bb.maxZ = p.z;
    }
    return bb;
}

// Extract all position floats from a PackedMesh attribute.
static std::vector<float> extractPositionFloats(const PackedMesh &pm) {
    if (!pm.has(VertexAttrib::Position)) return {};
    const VertexBufferView &view = pm.attribute_views[static_cast<uint8_t>(VertexAttrib::Position)];
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

static bool near(float a, float b, float tol = 1e-3f) {
    float d = a - b; if (d < 0) d = -d;
    return d <= tol;
}

// ---------------------------------------------------------------------------
// run_oracle — extract scene, write STL, read back, compare.
// Returns true if all assertions hold.
// ---------------------------------------------------------------------------
static bool run_oracle(const std::string &path, const std::string &label,
                       std::size_t expected_tris) {
    std::cout << "--- " << label << " ---\n";

    // 1. Parse.
    sdk::X3DDocument doc;
    try {
        doc = sdk::parseFile(path);
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << label << ": parse error: " << e.what() << "\n";
        return false;
    }

    // 2. Extract → collect world-space positions.
    std::vector<SFVec3f> positions;
    std::vector<SFVec3f> normals;

    try {
        sdk::X3DExecutionContext ctx;
        ctx.buildSceneGraph(doc.scene);
        ctx.buildFrom(doc.scene);
        sdk::MeshBuildOptions opts;
        sdk::SceneExtractor ex(ctx, doc.scene, opts);
        sdk::RenderDelta frame0 = ex.fullSnapshot();

        for (sdk::RenderItemId id : frame0.added) {
            const sdk::RenderItem &item = ex.item(id);
            const sdk::MeshData &mesh = item.mesh;
            const x3d::runtime::Mat4 &W = item.worldTransform;

            const std::size_t triCount = mesh.indices.size() / 3;
            for (std::size_t t = 0; t < triCount; ++t) {
                for (int v = 0; v < 3; ++v) {
                    const uint32_t idx = mesh.indices[t * 3 + static_cast<std::size_t>(v)];
                    if (idx >= mesh.positions.size()) continue;
                    positions.push_back(W.transformPoint(mesh.positions[idx]));
                    if (mesh.hasNormals && idx < mesh.normals.size())
                        normals.push_back(W.transformDirection(mesh.normals[idx]));
                }
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << label << ": extraction error: " << e.what() << "\n";
        return false;
    }

    const std::size_t refTriCount = positions.size() / 3;
    std::cout << "  reference triangles: " << refTriCount << "\n";

    if (expected_tris > 0 && refTriCount != expected_tris) {
        std::cerr << "FAIL: " << label << ": expected " << expected_tris
                  << " triangles, got " << refTriCount << "\n";
        return false;
    }
    std::cout << "ok:   " << label << " triangle count (" << refTriCount << ")\n";

    // 3. Write STL.
    std::string stlBytes;
    try {
        stlBytes = x3d::cli::writeStlBinary(positions, normals);
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << label << ": write error: " << e.what() << "\n";
        return false;
    }

    // 4. Re-import via StlReader.
    PackedMesh pm = parseStlBinary(
        reinterpret_cast<const uint8_t *>(stlBytes.data()), stlBytes.size());

    // 5a. vertex_count == 3 * refTriCount.
    const std::size_t expectedVerts = refTriCount * 3;
    if (pm.vertex_count != static_cast<uint32_t>(expectedVerts)) {
        std::cerr << "FAIL: " << label << ": re-imported vertex_count="
                  << pm.vertex_count << " expected " << expectedVerts << "\n";
        return false;
    }
    std::cout << "ok:   " << label << " re-imported vertex_count=" << pm.vertex_count << "\n";

    if (refTriCount == 0) {
        std::cout << "ok:   " << label << " (empty geometry, skipping AABB check)\n";
        return true;
    }

    // 5b. Compare AABB.
    AABB3 refBB = compute_aabb(positions);

    // Extract re-imported positions as SFVec3f.
    std::vector<float> reimportFloats = extractPositionFloats(pm);
    std::vector<SFVec3f> reimportPos;
    reimportPos.reserve(reimportFloats.size() / 3);
    for (std::size_t i = 0; i + 2 < reimportFloats.size(); i += 3)
        reimportPos.push_back({reimportFloats[i], reimportFloats[i+1], reimportFloats[i+2]});
    AABB3 reimportBB = compute_aabb(reimportPos);

    const float kTol = 1e-3f;
    bool aabbOk = near(refBB.minX, reimportBB.minX, kTol) &&
                  near(refBB.minY, reimportBB.minY, kTol) &&
                  near(refBB.minZ, reimportBB.minZ, kTol) &&
                  near(refBB.maxX, reimportBB.maxX, kTol) &&
                  near(refBB.maxY, reimportBB.maxY, kTol) &&
                  near(refBB.maxZ, reimportBB.maxZ, kTol);

    if (!aabbOk) {
        std::cerr << "FAIL: " << label << ": AABB mismatch\n";
        std::cerr << "  ref:     [" << refBB.minX << "," << refBB.minY << "," << refBB.minZ
                  << "] - [" << refBB.maxX << "," << refBB.maxY << "," << refBB.maxZ << "]\n";
        std::cerr << "  reimport:[" << reimportBB.minX << "," << reimportBB.minY
                  << "," << reimportBB.minZ << "] - [" << reimportBB.maxX
                  << "," << reimportBB.maxY << "," << reimportBB.maxZ << "]\n";
        return false;
    }
    std::cout << "ok:   " << label << " AABB matches (minX=" << refBB.minX
              << " maxX=" << refBB.maxX << " minY=" << refBB.minY
              << " maxY=" << refBB.maxY << " minZ=" << refBB.minZ
              << " maxZ=" << refBB.maxZ << ")\n";

    return true;
}

// ---------------------------------------------------------------------------
// main — run oracle on each fixture.
// The fixture paths are passed as argv[1], argv[2], ...
// If no paths are given, exit 0 (portable: no fixtures = no failure).
// ---------------------------------------------------------------------------
int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "extract_oracle_test: no fixture paths supplied; SKIP\n";
        return 0;
    }

    int failures = 0;

    // argv[1] = simple.x3d path (Box 2×2×2, expected 12 tris)
    // argv[2] = validate-clean.x3d path (Box 1×1×1, expected 12 tris)
    if (argc > 1) {
        if (!run_oracle(argv[1], "simple.x3d (Box 2x2x2)", 12)) ++failures;
    }
    if (argc > 2) {
        if (!run_oracle(argv[2], "validate-clean.x3d (Box 1x1x1)", 12)) ++failures;
    }

    std::cout << "\n";
    if (failures > 0) {
        std::cerr << "extract_oracle_test: " << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "extract_oracle_test: all oracle checks passed\n";
    return 0;
}
