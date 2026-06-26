// render_feed_audit_test.cpp — AUD-RENDER-FEED regression audit.
//
// Covers the five risk areas of render-feed consistency:
//   1) Render item identity stability across frames.
//   2) Path-key hashing correctness (collision behavior).
//   3) Material descriptor consistency (same material -> same descriptor).
//   4) Geometry buffer pointer stability.
//   5) Dirty flag propagation to render feed.
#include "SceneExtractor.hpp"

#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DNodeFactory.hpp"
#include "X3DScene.hpp"

#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
#include <vector>

using namespace x3d::runtime;

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
static void addChild(const std::shared_ptr<X3DNode> &p,
                     const std::shared_ptr<X3DNode> &c) {
  for (auto &f : p->fields())
    if (f.x3dName == "children" && f.set) {
      auto k = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*p));
      k.push_back(c);
      f.set(*p, std::any(std::move(k)));
      return;
    }
}

static std::shared_ptr<X3DNode> makeTriShape(std::shared_ptr<X3DNode> *geomOut = nullptr,
                                             std::shared_ptr<X3DNode> *coordOut = nullptr) {
  auto coord = createX3DNode("Coordinate");
  setF(coord, "point",
       std::any(std::vector<SFVec3f>{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}));
  auto tri = createX3DNode("TriangleSet");
  setF(tri, "coord", std::any(std::shared_ptr<X3DNode>(coord)));
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(tri)));
  if (geomOut) *geomOut = tri;
  if (coordOut) *coordOut = coord;
  return shape;
}

TEST_CASE("render_feed_audit_test") {
  // === 1) Render item identity stability across frames ======================
  {
    auto shape = makeTriShape();
    Scene scene;
    scene.addRootNode(shape);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);

    auto snap1 = ex.fullSnapshot();
    CHECK((snap1.added.size() == 1));
    extract::RenderItemId id1 = snap1.added[0];

    // A second full snapshot must reuse the SAME dense id for the same path.
    auto snap2 = ex.fullSnapshot();
    CHECK((snap2.added.size() == 1));
    extract::RenderItemId id2 = snap2.added[0];
    CHECK((id1 == id2));

    // The item itself must still be valid and contain the same geometry node.
    CHECK((ex.item(id2).geometry.node == ex.item(id1).geometry.node));
  }

  // === 2) Path-key hashing correctness (collision behavior) =================
  {
    // Construct two distinct paths that could conceivably hash-collide.
    // We test the bucket+equal contract: different paths must NEVER alias.
    auto shapeA = makeTriShape();
    auto shapeB = makeTriShape();
    auto tA = createX3DNode("Transform");
    addChild(tA, shapeA);
    auto tB = createX3DNode("Transform");
    addChild(tB, shapeB);

    auto root = createX3DNode("Group");
    addChild(root, tA);
    addChild(root, tB);

    Scene scene;
    scene.addRootNode(root);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);

    auto snap = ex.fullSnapshot();
    CHECK((snap.added.size() == 2));
    extract::RenderItemId idA = snap.added[0];
    extract::RenderItemId idB = snap.added[1];

    // Distinct paths => distinct ids even if bucket hashes collide.
    CHECK((idA != idB));

    // Verify the paths are actually different node chains.
    const extract::PathKey &pathA = ex.item(idA).path;
    const extract::PathKey &pathB = ex.item(idB).path;
    CHECK((pathA != pathB));
  }

  // === 3) Material descriptor consistency ===================================
  {
    // Two Shapes sharing the SAME Appearance+Material node must yield
    // equivalent MaterialDesc values (the descriptor is content-keyed).
    auto coord = createX3DNode("Coordinate");
    setF(coord, "point",
         std::any(std::vector<SFVec3f>{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}));
    auto triA = createX3DNode("TriangleSet");
    setF(triA, "coord", std::any(std::shared_ptr<X3DNode>(coord)));
    auto triB = createX3DNode("TriangleSet");
    // triB needs its own Coordinate (can't share coord parent).
    auto coordB = createX3DNode("Coordinate");
    setF(coordB, "point",
         std::any(std::vector<SFVec3f>{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}));
    setF(triB, "coord", std::any(std::shared_ptr<X3DNode>(coordB)));

    auto material = createX3DNode("Material");
    setF(material, "diffuseColor", std::any(SFColor{0.1f, 0.2f, 0.3f}));
    auto app = createX3DNode("Appearance");
    setF(app, "material", std::any(std::shared_ptr<X3DNode>(material)));

    auto shapeA = createX3DNode("Shape");
    setF(shapeA, "geometry", std::any(std::shared_ptr<X3DNode>(triA)));
    setF(shapeA, "appearance", std::any(std::shared_ptr<X3DNode>(app)));

    auto shapeB = createX3DNode("Shape");
    setF(shapeB, "geometry", std::any(std::shared_ptr<X3DNode>(triB)));
    setF(shapeB, "appearance", std::any(std::shared_ptr<X3DNode>(app)));

    auto root = createX3DNode("Group");
    addChild(root, shapeA);
    addChild(root, shapeB);

    Scene scene;
    scene.addRootNode(root);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);

    auto snap = ex.fullSnapshot();
    CHECK((snap.added.size() == 2));

    const extract::MaterialDesc &matA = ex.item(snap.added[0]).material;
    const extract::MaterialDesc &matB = ex.item(snap.added[1]).material;

    // Same material pulled for both paths => identical descriptor state.
    CHECK((matA.model == matB.model));
    CHECK((feq(matA.toRGBA().r, matB.toRGBA().r)));
    CHECK((feq(matA.toRGBA().g, matB.toRGBA().g)));
    CHECK((feq(matA.toRGBA().b, matB.toRGBA().b)));
    CHECK((feq(matA.toRGBA().a, matB.toRGBA().a)));
    CHECK((feq(matA.transparency, matB.transparency)));
  }

  // === 4) Geometry buffer pointer stability =================================
  {
    // A USE'd geometry node under two placements: GeomId.node must be the
    // SAME raw pointer across both RenderItems.
    auto sharedGeom = createX3DNode("TriangleSet");
    auto coord = createX3DNode("Coordinate");
    setF(coord, "point",
         std::any(std::vector<SFVec3f>{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}));
    setF(sharedGeom, "coord", std::any(std::shared_ptr<X3DNode>(coord)));

    auto shapeA = createX3DNode("Shape");
    setF(shapeA, "geometry", std::any(std::shared_ptr<X3DNode>(sharedGeom)));
    auto shapeB = createX3DNode("Shape");
    setF(shapeB, "geometry", std::any(std::shared_ptr<X3DNode>(sharedGeom)));

    auto tA = createX3DNode("Transform");
    addChild(tA, shapeA);
    auto tB = createX3DNode("Transform");
    addChild(tB, shapeB);

    auto root = createX3DNode("Group");
    addChild(root, tA);
    addChild(root, tB);

    Scene scene;
    scene.addRootNode(root);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);

    auto snap = ex.fullSnapshot();
    CHECK((snap.added.size() == 2));

    const extract::GeomId &gA = ex.item(snap.added[0]).geometry;
    const extract::GeomId &gB = ex.item(snap.added[1]).geometry;

    // Pointer stability: both items key the same geometry node.
    CHECK((gA.node == gB.node));
    CHECK((gA.node == sharedGeom.get()));

    // contentVersion must also match at snapshot time.
    CHECK((gA.contentVersion == gB.contentVersion));
  }

  // === 5) Dirty flag propagation to render feed =============================
  {
    std::shared_ptr<X3DNode> geom, coord;
    auto shape = makeTriShape(&geom, &coord);
    auto parent = createX3DNode("Transform");
    addChild(parent, shape);

    Scene scene;
    scene.addRootNode(parent);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);

    auto snap = ex.fullSnapshot();
    CHECK((snap.added.size() == 1));
    extract::RenderItemId id0 = snap.added[0];

    // --- 5a) Transform dirty => updatedTransform only -----------------------
    ctx.postEvent(parent.get(), "translation", std::any(SFVec3f{5, 0, 0}));
    ctx.tick(1.0);
    auto d1 = ex.delta();
    CHECK((d1.updatedTransform.size() == 1));
    CHECK((d1.updatedTransform[0] == id0));
    CHECK((d1.updatedGeometry.empty()));
    CHECK((d1.updatedMaterial.empty()));
    CHECK((d1.added.empty()));
    CHECK((d1.removed.empty()));

    // --- 5b) Geometry content dirty => updatedGeometry only -----------------
    ctx.postEvent(coord.get(), "point",
                  std::any(std::vector<SFVec3f>{{0, 0, 0}, {2, 0, 0}, {0, 2, 0}}));
    ctx.tick(2.0);
    auto d2 = ex.delta();
    CHECK((d2.updatedGeometry.size() == 1));
    CHECK((d2.updatedGeometry[0] == id0));
    CHECK((d2.updatedTransform.empty()));
    CHECK((d2.updatedMaterial.empty()));
    CHECK((d2.added.empty()));
    CHECK((d2.removed.empty()));
    // contentVersion bumped.
    CHECK((ex.item(id0).geometry.contentVersion == 1));

    // --- 5c) Full snapshot resets dirty state; stable id preserved ----------
    auto snap2 = ex.fullSnapshot();
    CHECK((snap2.added.size() == 1));
    CHECK((snap2.added[0] == id0)); // stable identity.
  }

  return;
}
