// external_geom_seam_test.cpp — Phase 1: proves the externalGeometryResolver seam
// end-to-end WITHOUT any ext module. A NurbsPatchSurface (recognized==false)
// triggers the resolver; a non-empty PackedMesh causes emitPacked(); the
// RenderItem's geometry_ext.kind == Packed.
#include "SceneExtractor.hpp"
#include "MeshBuilder.hpp"
#include "x3d/nodes/X3DNode.hpp"
#include "PackedMesh.hpp"
#include "RenderItem.hpp"
#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DScene.hpp"

#include <any>
#include "doctest/doctest.h"
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;
using namespace x3d::runtime::extract;

static void setF(const std::shared_ptr<X3DNode>& n, const char* nm, std::any v) {
  for (auto& f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
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
}

// Build a minimal valid PackedMesh (one triangle, Float positions, UShort indices)
static PackedMesh makeTrianglePacked() {
  PackedMesh m;
  std::vector<uint8_t> pos(3 * 3 * sizeof(float), 0);
  float verts[9] = {0,0,0, 1,0,0, 0,1,0};
  std::memcpy(pos.data(), verts, pos.size());
  VertexBufferView pv;
  pv.component_type = ComponentType::Float;
  pv.components_per_vertex = 3;
  pv.vertex_count = 3;
  m.set_attrib(VertexAttrib::Position, pv, std::move(pos));
  m.vertex_count = 3;
  m.index_count = 0; // non-indexed is fine
  return m;
}

TEST_CASE("external_geom_seam_test") {
  // A NurbsTrimmedSurface (still unrecognized) triggers the resolver
  {
    bool rec = true;
    auto nurbs = createX3DNode("NurbsTrimmedSurface");
    auto mesh = buildLocalMesh(nurbs.get(), MeshBuildOptions{}, &rec);
    CHECK((!rec && mesh.indices.empty())); // still unrecognized
  }

  // === 1) Resolver NOT installed → NurbsPatchSurface still skipped (no change) =
  {
    auto root = createX3DNode("Group");
    auto nurbs = createX3DNode("NurbsTrimmedSurface");
    auto shape = createX3DNode("Shape");
    setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(nurbs)));
    addChild(root, shape);

    Scene scene;
    scene.addRootNode(root);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);
    auto snap = ex.fullSnapshot();
    CHECK((snap.added.empty())); // no resolver → still skipped
  }

  // === 2) Resolver installed → non-empty PackedMesh → RenderItem Packed emitted
  {
    auto root = createX3DNode("Group");
    auto nurbs = createX3DNode("NurbsTrimmedSurface");
    auto shape = createX3DNode("Shape");
    setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(nurbs)));
    addChild(root, shape);

    Scene scene;
    scene.addRootNode(root);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);

    MeshBuildOptions opts;
    opts.externalGeometryResolver = [](const X3DNode* /*node*/, AssetResolver /*resolver*/) -> PackedMesh {
      return makeTrianglePacked();
    };

    extract::SceneExtractor ex(ctx, scene, opts);
    auto snap = ex.fullSnapshot();
    CHECK((snap.added.size() == 1)); // resolver fired, emitPacked called
    const RenderItem& item = ex.item(snap.added[0]);
    CHECK((item.geometry_ext.is_packed())); // the new Geometry union field
  }

  // === 3) Resolver returning empty PackedMesh → Pending → NOT emitted ==========
  {
    auto root = createX3DNode("Group");
    auto nurbs = createX3DNode("NurbsTrimmedSurface");
    auto shape = createX3DNode("Shape");
    setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(nurbs)));
    addChild(root, shape);

    Scene scene;
    scene.addRootNode(root);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);

    MeshBuildOptions opts;
    opts.externalGeometryResolver = [](const X3DNode*, AssetResolver) -> PackedMesh {
      return PackedMesh{}; // empty = Pending
    };

    extract::SceneExtractor ex(ctx, scene, opts);
    auto snap = ex.fullSnapshot();
    CHECK((snap.added.empty())); // empty PackedMesh → Pending → not emitted
  }

  return;
}
