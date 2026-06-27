// external_geometry_e2e_test.cpp — Round 2 headline end-to-end lazy proof.
//
// Builds a Scene whose Shape.geometry is an ext ExternalGeometry node with
// url='m.stl'. A stub byte-provider returns a known small binary STL for 'm.stl'
// and counts calls. meshOptions.externalGeometryResolver is built via
// ext::makeExternalGeometryResolver(stub). Running SceneExtractor materializes
// the proxy lazily into a Packed RenderItem.
//
// Asserts:
//   (1) PACKED: a RenderItem with geometry_ext.kind == Packed whose PackedMesh
//       matches the STL (vertex_count == N*3; a known position reads back).
//   (2) CACHE:  a second extract reuses the cached mesh — the stub byte-provider
//       was called only ONCE.
//   (3) PENDING/skip: an ExternalGeometry with an unresolvable url (stub returns
//       empty) → NO Packed RenderItem emitted, no crash.
//
// Part of the runtime/ext/ quarantine (x3d_cpp_ext target, default OFF).
#include "ExternalGeometry.hpp"
#include "ExternalGeometryResolver.hpp"

#include "AssetResolver.hpp"
#include "MeshBuilder.hpp"
#include "PackedMesh.hpp"
#include "RenderItem.hpp"
#include "SceneExtractor.hpp"
#include "X3DDocument.hpp"          // inline Scene::addRootNode definition
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DScene.hpp"

#include <any>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::runtime::extract;
using x3d::runtime::ext::ExternalGeometry;
using x3d::runtime::ext::makeExternalGeometryResolver;

// --- helpers ---------------------------------------------------------------
static void setF(const std::shared_ptr<X3DNode>& n, const char* nm, std::any v) {
    for (auto& f : n->fields())
        if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
    assert(false && "field not found");
}
static void addChild(const std::shared_ptr<X3DNode>& p,
                     const std::shared_ptr<X3DNode>& c) {
    for (auto& f : p->fields())
        if (f.x3dName == "children" && f.set) {
            auto k = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*p));
            k.push_back(c);
            f.set(*p, std::any(std::move(k)));
            return;
        }
    assert(false && "children field not found");
}

// Build a binary STL with two triangles. Known v0 of triangle 0 = (1,2,3).
static std::vector<std::uint8_t> makeTwoTriangleStl() {
    const std::uint32_t N = 2;
    std::vector<std::uint8_t> buf(84 + N * 50, 0);
    std::memcpy(buf.data() + 80, &N, sizeof(std::uint32_t));

    auto writeFloat = [&](std::size_t off, float v) {
        std::memcpy(buf.data() + off, &v, sizeof(float));
    };
    // Triangle records start at offset 84; each 50 bytes:
    // [normal(3f)] [v0(3f)] [v1(3f)] [v2(3f)] [attr(u16)]
    // Triangle 0
    std::size_t r0 = 84;
    writeFloat(r0 + 0, 0); writeFloat(r0 + 4, 0); writeFloat(r0 + 8, 1);    // normal
    writeFloat(r0 + 12, 1); writeFloat(r0 + 16, 2); writeFloat(r0 + 20, 3); // v0 = (1,2,3)
    writeFloat(r0 + 24, 4); writeFloat(r0 + 28, 0); writeFloat(r0 + 32, 0); // v1
    writeFloat(r0 + 36, 0); writeFloat(r0 + 40, 5); writeFloat(r0 + 44, 0); // v2
    // Triangle 1
    std::size_t r1 = 84 + 50;
    writeFloat(r1 + 0, 0); writeFloat(r1 + 4, 1); writeFloat(r1 + 8, 0);    // normal
    writeFloat(r1 + 12, 0); writeFloat(r1 + 16, 0); writeFloat(r1 + 20, 0); // v0
    writeFloat(r1 + 24, 1); writeFloat(r1 + 28, 1); writeFloat(r1 + 32, 1); // v1
    writeFloat(r1 + 36, 2); writeFloat(r1 + 40, 2); writeFloat(r1 + 44, 2); // v2
    return buf;
}

// Read a Position float3 out of a PackedMesh at vertex index `vi`.
static void readPosition(const PackedMesh& m, std::uint32_t vi,
                         float& x, float& y, float& z) {
    assert(m.has(VertexAttrib::Position));
    const VertexBufferView& pv =
        m.attribute_views[static_cast<std::uint8_t>(VertexAttrib::Position)];
    const std::uint8_t* base = pv.data_ptr(m.attribute_data);
    const std::size_t off = static_cast<std::size_t>(vi) * 3u * sizeof(float);
    std::memcpy(&x, base + off + 0, 4);
    std::memcpy(&y, base + off + 4, 4);
    std::memcpy(&z, base + off + 8, 4);
}

// Build a Scene: Group -> Shape -> geometry = ExternalGeometry(url).
static std::shared_ptr<X3DNode> makeExtGeomShapeScene(const std::string& url) {
    auto root = createX3DNode("Group");
    auto shape = createX3DNode("Shape");
    auto eg = std::make_shared<ExternalGeometry>();
    eg->setUrl(MFString{url});
    setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(eg)));
    addChild(root, shape);
    return root;
}

int main() {
    const std::vector<std::uint8_t> stlBytes = makeTwoTriangleStl();
    const std::uint32_t expectedVerts = 2u * 3u; // N*3

    // === (1) PACKED + (2) CACHE =============================================
    {
        int byteCalls = 0;
        AssetResolver stub = [&](const std::string& url, AssetKind kind)
            -> AssetResult {
            assert(kind == AssetKind::ExternalGeometry);
            if (url == "m.stl") {
                ++byteCalls;
                return AssetResult::makeReady(stlBytes);
            }
            return AssetResult::makeFailed();
        };

        MeshBuildOptions opts;
        opts.externalGeometryResolver = makeExternalGeometryResolver(stub);

        auto root = makeExtGeomShapeScene("m.stl");
        Scene scene;
        scene.addRootNode(root);
        X3DExecutionContext ctx;
        ctx.buildSceneGraph(scene);

        SceneExtractor ex(ctx, scene, opts);

        // First extract → materialize.
        auto snap = ex.fullSnapshot();
        assert(snap.added.size() == 1 && "expected one Packed RenderItem");
        const RenderItem& item = ex.item(snap.added[0]);
        assert(item.geometry_ext.is_packed() && "geometry_ext.kind must be Packed");

        const PackedMesh& m = item.geometry_ext.packed;
        assert(m.vertex_count == expectedVerts && "vertex_count must be N*3");

        // Known position: triangle 0 vertex 0 = (1,2,3).
        float x, y, z;
        readPosition(m, 0, x, y, z);
        assert(x == 1.0f && y == 2.0f && z == 3.0f && "v0 position must read back");

        assert(byteCalls == 1 && "first extract must fetch bytes exactly once");

        // (2) CACHE — second extract must NOT re-fetch (cache hit).
        SceneExtractor ex2(ctx, scene, opts);
        auto snap2 = ex2.fullSnapshot();
        assert(snap2.added.size() == 1 && "second extract still emits the item");
        assert(ex2.item(snap2.added[0]).geometry_ext.is_packed());
        assert(byteCalls == 1 && "second extract must reuse cache — no new fetch");
    }

    // === (3) PENDING / skip ================================================
    {
        AssetResolver pendingStub = [&](const std::string& /*url*/, AssetKind)
            -> AssetResult {
            return AssetResult::makePending(); // never resolves
        };

        MeshBuildOptions opts;
        opts.externalGeometryResolver = makeExternalGeometryResolver(pendingStub);

        auto root = makeExtGeomShapeScene("missing.stl");
        Scene scene;
        scene.addRootNode(root);
        X3DExecutionContext ctx;
        ctx.buildSceneGraph(scene);

        SceneExtractor ex(ctx, scene, opts);
        auto snap = ex.fullSnapshot();
        assert(snap.added.empty() && "unresolvable url → Pending → no Packed item");
    }

    return 0;
}
