// scene_extractor_instancing_test.cpp — PERF-INSTANCING regression.
//
// RenderItem.hpp documents the instancing contract: "positions/normals are
// LOCAL-frame — NEVER world-baked ... so one mesh is uploaded once and instanced
// across placements." GeomId is the content-identity key that makes that true
// for the CONSUMER (the PoC keys its GpuMesh cache on it).
//
// The SDK side did not honour its own key. buildLocalMesh ran inside walk(), so
// N USEs of one DEF'd geometry tessellated the same node N times and retained N
// full MeshData copies in host RAM. Measured before this test existed: 200 USEs
// of one 19,602-triangle IndexedFaceSet cost 493 MiB of mesh bytes and ~1.08 s
// of redundant tessellation, against ~2.5 MiB / ~5.6 ms of distinct content.
//
// These cases pin BOTH halves of the fix so it cannot regress:
//   * build-once — buildLocalMeshCallCount() is O(distinct geometry), not
//     O(placements).
//   * share-once — every placement of one GeomId hands the consumer the SAME
//     MeshData allocation, so host RAM is O(distinct content).
//
// The negative control (distinct geometry nodes must NOT share) stops the cache
// from being "fixed" into a global one-mesh-for-everything bug.

#include "SceneExtractor.hpp"

#include "MeshBuilder.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DScene.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"

#include <any>
#include "doctest/doctest.h"
#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::runtime::extract;
using namespace x3d::core;
using namespace x3d::nodes;

namespace {

void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
void addChild(const std::shared_ptr<X3DNode> &p,
              const std::shared_ptr<X3DNode> &c) {
  for (auto &f : p->fields())
    if (f.x3dName == "children" && f.set) {
      auto k = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*p));
      k.push_back(c);
      f.set(*p, std::any(std::move(k)));
      return;
    }
}

// A Shape whose IndexedFaceSet carries authored normals, so the builder keeps it
// indexed (no flat-normal expansion) and the byte counts stay predictable.
std::shared_ptr<X3DNode> makeQuadShape() {
  auto coord = createX3DNode("Coordinate");
  setF(coord, "point",
       std::any(std::vector<SFVec3f>{{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}}));
  auto normal = createX3DNode("Normal");
  setF(normal, "vector",
       std::any(std::vector<SFVec3f>{{0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}}));
  auto ifs = createX3DNode("IndexedFaceSet");
  setF(ifs, "coord", std::any(std::shared_ptr<X3DNode>(coord)));
  setF(ifs, "normal", std::any(std::shared_ptr<X3DNode>(normal)));
  setF(ifs, "coordIndex", std::any(std::vector<std::int32_t>{0, 1, 2, 3, -1}));
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(ifs)));
  return shape;
}

// N Transforms, each USE-ing the SAME Shape node (the DEF/USE instancing shape).
Scene makeInstancedScene(const std::shared_ptr<X3DNode> &shape, int n) {
  Scene scene;
  for (int i = 0; i < n; ++i) {
    auto t = createX3DNode("Transform");
    setF(t, "translation", std::any(SFVec3f{static_cast<float>(i), 0, 0}));
    addChild(t, shape);
    scene.rootNodes.push_back(t);
  }
  return scene;
}

} // namespace

TEST_CASE("instancing: N USEs of one DEF'd geometry tessellate ONCE") {
  const int kInstances = 64;
  auto shape = makeQuadShape();
  Scene scene = makeInstancedScene(shape, kInstances);

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  SceneExtractor ex(ctx, scene);

  const std::uint64_t before = buildLocalMeshCallCount();
  RenderDelta f0 = ex.fullSnapshot();
  const std::uint64_t built = buildLocalMeshCallCount() - before;

  REQUIRE(ex.itemCount() == static_cast<std::size_t>(kInstances));
  // The whole point: one DISTINCT geometry node => exactly one tessellation,
  // however many placements reference it.
  CHECK(built == 1);
}

TEST_CASE("instancing: every placement shares ONE MeshData allocation") {
  const int kInstances = 64;
  auto shape = makeQuadShape();
  Scene scene = makeInstancedScene(shape, kInstances);

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  SceneExtractor ex(ctx, scene);
  ex.fullSnapshot();

  REQUIRE(ex.itemCount() == static_cast<std::size_t>(kInstances));

  const MeshData *first = ex.item(0).mesh.get();
  REQUIRE((first != nullptr));
  REQUIRE_FALSE(first->positions.empty());

  std::set<const MeshData *> distinctAllocations;
  std::set<const X3DNode *> distinctGeomNodes;
  for (std::size_t i = 0; i < ex.itemCount(); ++i) {
    distinctAllocations.insert(ex.item(i).mesh.get());
    distinctGeomNodes.insert(ex.item(i).geometry.node);
  }
  // Identity key and storage must agree: one GeomId <=> one allocation.
  CHECK(distinctGeomNodes.size() == 1);
  CHECK(distinctAllocations.size() == 1);

  // Per-placement world transforms stay DISTINCT — sharing the mesh must not
  // collapse the placements themselves (the M2C-1 first-path hazard).
  CHECK(ex.item(0).worldTransform.m[12] != ex.item(1).worldTransform.m[12]);
}

TEST_CASE("instancing: DISTINCT geometry nodes do NOT share a mesh") {
  // Negative control. A cache keyed too coarsely (or a global singleton) would
  // hand both shapes the same allocation and silently render one geometry twice.
  Scene scene;
  auto a = makeQuadShape();
  auto b = makeQuadShape();
  auto ta = createX3DNode("Transform");
  addChild(ta, a);
  auto tb = createX3DNode("Transform");
  addChild(tb, b);
  scene.rootNodes.push_back(ta);
  scene.rootNodes.push_back(tb);

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  SceneExtractor ex(ctx, scene);

  const std::uint64_t before = buildLocalMeshCallCount();
  ex.fullSnapshot();
  const std::uint64_t built = buildLocalMeshCallCount() - before;

  REQUIRE(ex.itemCount() == 2);
  CHECK(built == 2);
  CHECK(ex.item(0).mesh.get() != ex.item(1).mesh.get());
}

TEST_CASE("instancing: one texture URL decodes ONCE across N placements") {
  // Decoded RGBA is the biggest payload the feed carries. The resolver was
  // invoked per PLACEMENT, so 200 USEs of one textured Shape meant 200 embedder
  // decode calls and 200 retained surfaces of the same image.
  const int kInstances = 32;
  auto shape = makeQuadShape();
  auto tex = createX3DNode("ImageTexture");
  setF(tex, "url", std::any(std::vector<std::string>{"shared.png"}));
  auto app = createX3DNode("Appearance");
  setF(app, "texture", std::any(std::shared_ptr<X3DNode>(tex)));
  setF(app, "material", std::any(std::shared_ptr<X3DNode>(createX3DNode("Material"))));
  setF(shape, "appearance", std::any(std::shared_ptr<X3DNode>(app)));

  Scene scene = makeInstancedScene(shape, kInstances);

  int resolverCalls = 0;
  TextureResolver counting = [&resolverCalls](const std::string &) {
    ++resolverCalls;
    TexturePixels p;
    p.width = 4;
    p.height = 4;
    p.rgba.assign(4 * 4 * 4, 200);
    return TexturePixelResult::makeReady(std::move(p));
  };

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  SceneExtractor ex(ctx, scene, MeshBuildOptions{}, counting);
  ex.fullSnapshot();

  REQUIRE(ex.itemCount() == static_cast<std::size_t>(kInstances));
  // One distinct URL => exactly one embedder decode, however many placements.
  CHECK(resolverCalls == 1);

  std::set<const TexturePixels *> surfaces;
  for (std::size_t i = 0; i < ex.itemCount(); ++i) {
    const auto &textures = ex.item(i).material.textures;
    REQUIRE_FALSE(textures.empty());
    REQUIRE(textures[0].resolvedPixels.ready());
    surfaces.insert(textures[0].resolvedPixels.pixels.get());
  }
  // ...and every placement shares that one decoded surface.
  CHECK(surfaces.size() == 1);
}

TEST_CASE("instancing: Pending texture resolves are NOT memoized (retry survives)") {
  // A Pending result means "not decoded yet, try again next frame". Memoizing it
  // would freeze the retry and the texture would never appear.
  auto shape = makeQuadShape();
  auto tex = createX3DNode("ImageTexture");
  setF(tex, "url", std::any(std::vector<std::string>{"slow.png"}));
  auto app = createX3DNode("Appearance");
  setF(app, "texture", std::any(std::shared_ptr<X3DNode>(tex)));
  setF(app, "material", std::any(std::shared_ptr<X3DNode>(createX3DNode("Material"))));
  setF(shape, "appearance", std::any(std::shared_ptr<X3DNode>(app)));
  Scene scene = makeInstancedScene(shape, 8);

  int calls = 0;
  TextureResolver pending = [&calls](const std::string &) {
    ++calls;
    return TexturePixelResult::makePending();
  };

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  SceneExtractor ex(ctx, scene, MeshBuildOptions{}, pending);
  ex.fullSnapshot();

  // Every placement re-offered the URL, because nothing was cached.
  CHECK(calls == 8);
  CHECK(ex.item(0).material.textures[0].resolvedPixels.pending());
  // Pending carries no payload but is still safe to dereference (never null).
  CHECK(ex.item(0).material.textures[0].resolvedPixels.pixels->rgba.empty());
}

TEST_CASE("instancing: geometry content change rebuilds once and RE-shares") {
  // The delta() re-extract path had the same defect in copy form: it built the
  // mesh once then assigned a full COPY into every dependent record.
  const int kInstances = 16;
  auto shape = makeQuadShape();
  Scene scene = makeInstancedScene(shape, kInstances);

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  SceneExtractor ex(ctx, scene);
  ex.fullSnapshot();

  const MeshData *before = ex.item(0).mesh.get();

  // Mutate the shared Coordinate => DirtyField on the geometry's content child.
  std::shared_ptr<X3DNode> ifs, coord;
  for (auto &f : shape->fields())
    if (f.x3dName == "geometry")
      ifs = std::any_cast<std::shared_ptr<X3DNode>>(f.get(*shape));
  REQUIRE((ifs != nullptr));
  for (auto &f : ifs->fields())
    if (f.x3dName == "coord")
      coord = std::any_cast<std::shared_ptr<X3DNode>>(f.get(*ifs));
  REQUIRE((coord != nullptr));

  ctx.tick(1.0);
  REQUIRE(ctx.writeField(coord.get(), "point",
                         std::any(std::vector<SFVec3f>{
                             {0, 0, 0}, {2, 0, 0}, {2, 2, 0}, {0, 2, 0}})) ==
          FieldWriteResult::Ok);

  const std::uint64_t beforeCalls = buildLocalMeshCallCount();
  RenderDelta d = ex.delta();
  const std::uint64_t rebuilt = buildLocalMeshCallCount() - beforeCalls;

  CHECK(rebuilt == 1);
  CHECK(d.updatedGeometry.size() == static_cast<std::size_t>(kInstances));

  std::set<const MeshData *> after;
  for (std::size_t i = 0; i < ex.itemCount(); ++i)
    after.insert(ex.item(i).mesh.get());
  // Still exactly one allocation across all placements...
  CHECK(after.size() == 1);
  // ...and it is a NEW one, so a consumer holding the old pointer is not
  // mutated underneath it (contentVersion bumped => orphan the old GPU buffer).
  CHECK(*after.begin() != before);
  CHECK(ex.item(0).geometry.contentVersion == 1);
}
