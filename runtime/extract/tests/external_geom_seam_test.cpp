// external_geom_seam_test.cpp — proves the externalGeometryResolver seam
// end-to-end without any ext module. A NurbsPatchSurface (recognized==false)
// triggers the resolver; a non-empty PackedMesh causes emitPacked(); the
// RenderItem's geometry_ext.kind == Packed.
#include "GeoFrame.hpp"
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
#include "scene_test_helpers.hpp"
#include <cstdint>
#include <cstring>
#include <memory>
#include <map>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;
using namespace x3d::runtime::extract;
using x3d::test::setF;
using x3d::test::addChild;

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

TEST_CASE("external geom seam: an unrecognized NurbsTrimmedSurface triggers the resolver") {
  bool rec = true;
  auto nurbs = createX3DNode("NurbsTrimmedSurface");
  auto mesh = buildLocalMesh(nurbs.get(), geo::builtinProjection(), MeshBuildOptions{}, &rec);
  CHECK((!rec && mesh.indices.empty())); // still unrecognized
}

TEST_CASE("external geom seam: without a resolver an unsupported surface is skipped") {
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

TEST_CASE("external geom seam: a resolved PackedMesh is emitted as a packed item") {
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
  // Packed payloads still satisfy MeshRef's never-null contract. Consumers
  // that inspect the AoS channel see the shared empty mesh, not a null pointer.
  CHECK(item.mesh != nullptr);
  if (item.mesh) {
    CHECK(item.mesh == emptyMeshRef());
    CHECK(item.mesh->positions.empty());
    CHECK(ex.sceneWorldBounds().empty); // packed bounds are a separate channel
  }
}

TEST_CASE("external geom seam: an empty PackedMesh is pending and not emitted") {
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

TEST_CASE("delta oracle: packed geometry field edits refresh payload and membership") {
  auto geom = createX3DNode("NurbsTrimmedSurface"), shape = createX3DNode("Shape");
  setF(geom, "uTessellation", SFInt32{1}); setF(shape, "geometry", geom);
  Scene scene; scene.rootNodes = {shape};
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  bool pending = false;
  SUBCASE("ready baseline") {}
  SUBCASE("pending baseline") { pending = true; }
  int calls = 0;
  MeshBuildOptions options;
  options.externalGeometryResolver = [&](const X3DNode *n, AssetResolver) {
    ++calls;
    if (pending) return PackedMesh{};
    auto mesh = makeTrianglePacked();
    float x = float(geombounds::getField<int>(*n, "uTessellation", 0));
    std::memcpy(mesh.attribute_data[0].data(), &x, sizeof x);
    return mesh;
  };
  SceneExtractor ex(ctx, scene, options);
  auto initial = ex.fullSnapshot();
  CHECK(initial.added.size() == (pending ? 0 : 1));
  std::map<RenderItemId, RenderItem> mirror;
  for (auto id : initial.added) mirror[id] = ex.item(id);
  for (int edit : {2, 3, 4}) {
    pending = edit == 3;
    ctx.postEvent(geom.get(), "uTessellation", SFInt32{edit}); ctx.tick(1);
    const int before = calls;
    auto d = ex.delta();
    CHECK(calls == before + 1);
    for (auto id : d.removed) mirror.erase(id);
    for (const auto *ids : {&d.added, &d.updatedGeometry})
      for (auto id : *ids) mirror[id] = ex.item(id);
    SceneExtractor oracle(ctx, scene, options);
    auto full = oracle.fullSnapshot();
    REQUIRE(mirror.size() == full.added.size());
    if (!pending) {
      REQUIRE(mirror.size() == 1);
      CHECK(mirror.begin()->second.geometry_ext.packed.attribute_data ==
            oracle.item(full.added[0]).geometry_ext.packed.attribute_data);
    }
  }
}

TEST_CASE("delta oracle: pending packed completion retries without a field edit") {
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", createX3DNode("NurbsTrimmedSurface"));
  Scene scene; scene.rootNodes = {shape};
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  bool ready = false;
  int calls = 0;
  MeshBuildOptions options;
  options.externalGeometryResolver = [&](const X3DNode *, AssetResolver) {
    ++calls;
    return ready ? makeTrianglePacked() : PackedMesh{};
  };
  SceneExtractor ex(ctx, scene, options);
  CHECK(ex.fullSnapshot().added.empty());
  ready = true; ctx.tick(1);
  auto d = ex.delta();
  REQUIRE(d.added.size() == 1);
  const auto payload = ex.item(d.added[0]).geometry_ext.packed.attribute_data;
  CHECK(calls == 2);
  ctx.tick(2);
  CHECK(ex.delta().updatedGeometry.empty());
  CHECK(calls == 2);
  SceneExtractor oracle(ctx, scene, options);
  auto full = oracle.fullSnapshot();
  REQUIRE(full.added.size() == 1);
  CHECK(payload == oracle.item(full.added[0]).geometry_ext.packed.attribute_data);
}

TEST_CASE("external geom seam: Unsupported and Failed are counted and not retried") {
  using Status = ExternalGeometryResult::Status;
  for (Status answer : {Status::Unsupported, Status::Failed}) {
    CAPTURE(int(answer));
    auto geom = createX3DNode("NurbsTrimmedSurface"), shape = createX3DNode("Shape");
    setF(geom, "uTessellation", SFInt32{1}); setF(shape, "geometry", geom);
    Scene scene; scene.rootNodes = {shape};
    X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
    int calls = 0;
    bool ready = false;
    MeshBuildOptions options;
    options.externalGeometryResolver = [&](const X3DNode *, AssetResolver) {
      ++calls;
      if (ready) return ExternalGeometryResult(makeTrianglePacked());
      return answer == Status::Unsupported ? ExternalGeometryResult::unsupported()
                                           : ExternalGeometryResult::failed();
    };
    SceneExtractor ex(ctx, scene, options);
    CHECK(ex.fullSnapshot().added.empty());
    CHECK(calls == 1);
    const auto &counted = answer == Status::Unsupported ? ex.skippedGeometryCounts()
                                                        : ex.failedGeometryCounts();
    const auto &other = answer == Status::Unsupported ? ex.failedGeometryCounts()
                                                      : ex.skippedGeometryCounts();
    CHECK(counted.at("NurbsTrimmedSurface") == 1);
    CHECK(other.empty());

    // No retry without an edit, unlike Pending.
    ctx.tick(1);
    CHECK(ex.delta().added.empty());
    CHECK(calls == 1);

    // A field edit asks again, and a Ready answer then emits.
    ready = true;
    ctx.postEvent(geom.get(), "uTessellation", SFInt32{2}); ctx.tick(2);
    auto d = ex.delta();
    CHECK(calls == 2);
    REQUIRE(d.added.size() == 1);
    CHECK(ex.item(d.added[0]).geometry_ext.is_packed());

    // A full walk recounts.
    ex.fullSnapshot();
    CHECK(ex.skippedGeometryCounts().empty());
    CHECK(ex.failedGeometryCounts().empty());
  }
}

TEST_CASE("external geom seam: a PackedMesh-returning resolver keeps its meaning") {
  CHECK(ExternalGeometryResult(PackedMesh{}).status == ExternalGeometryResult::Status::Pending);
  CHECK(ExternalGeometryResult(makeTrianglePacked()).ready());
  // Ready with no vertices is Pending, not an empty item.
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", createX3DNode("NurbsTrimmedSurface"));
  Scene scene; scene.rootNodes = {shape};
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  int calls = 0;
  MeshBuildOptions options;
  options.externalGeometryResolver = [&](const X3DNode *, AssetResolver) {
    ++calls;
    ExternalGeometryResult r;
    r.status = ExternalGeometryResult::Status::Ready;
    return r;
  };
  SceneExtractor ex(ctx, scene, options);
  CHECK(ex.fullSnapshot().added.empty());
  ctx.tick(1);
  CHECK(ex.delta().added.empty());
  CHECK(calls == 2); // retried as Pending
  CHECK(ex.skippedGeometryCounts().empty());
}
