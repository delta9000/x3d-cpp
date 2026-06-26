// scene_extractor_audit_test.cpp — AUD-EXTRACT regression audit.
//
// Covers the five risk areas of the scene-extraction walk:
//   1) Null/empty node handling (null children in MFNode fields).
//   2) Unsupported node type skip logic (graceful, no crash).
//   3) World transform accumulation under scaling + rotation.
//   4) Material/geometry pairing when one is missing.
//   5) USE-node extraction (shared geometry extracted once, not duplicated).
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

static std::shared_ptr<X3DNode> makeTriShape() {
  auto coord = createX3DNode("Coordinate");
  setF(coord, "point",
       std::any(std::vector<SFVec3f>{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}));
  auto tri = createX3DNode("TriangleSet");
  setF(tri, "coord", std::any(std::shared_ptr<X3DNode>(coord)));
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(tri)));
  return shape;
}

TEST_CASE("scene_extractor_audit_test") {
  // === 1) Null/empty node handling in extraction walk =======================
  {
    auto group = createX3DNode("Group");
    // Manually construct an MFNode children vector containing a nullptr.
    // This can happen with malformed scene construction.
    std::vector<std::shared_ptr<X3DNode>> kids;
    kids.push_back(nullptr);
    kids.push_back(makeTriShape());
    kids.push_back(nullptr);
    setF(group, "children", std::any(kids));

    Scene scene;
    scene.addRootNode(group);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);

    // Null children must not crash; the valid Shape must still emit.
    auto snap = ex.fullSnapshot();
    CHECK((snap.added.size() == 1));
  }

  // === 2) Unsupported node type skip logic (graceful, no crash) =============
  {
    auto root = createX3DNode("Group");
    addChild(root, makeTriShape()); // supported

    auto unsupported = createX3DNode("NurbsPatchSurface");
    auto badShape = createX3DNode("Shape");
    setF(badShape, "geometry", std::any(std::shared_ptr<X3DNode>(unsupported)));
    addChild(root, badShape);

    Scene scene;
    scene.addRootNode(root);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);

    auto snap = ex.fullSnapshot();
    // Only the TriangleSet emits; unsupported is skipped silently.
    CHECK((snap.added.size() == 1));
    const auto &counts = ex.skippedGeometryCounts();
    CHECK((counts.find("NurbsPatchSurface") != counts.end()));
    CHECK((counts.at("NurbsPatchSurface") == 1));
  }

  // === 3) World transform accumulation under scaling + rotation =============
  {
    // Build a transform chain: root -> T(scale) -> T(rotate 90° Z) -> shape.
    // Expected world transform of a point: scale first, then rotate.
    auto shape = makeTriShape();

    auto scaleT = createX3DNode("Transform");
    setF(scaleT, "scale", std::any(SFVec3f{2.0f, 2.0f, 2.0f}));
    addChild(scaleT, shape);

    auto rotT = createX3DNode("Transform");
    // 90° rotation about Z axis.
    setF(rotT, "rotation", std::any(SFRotation{0.0f, 0.0f, 1.0f, 1.5707963f}));
    addChild(rotT, scaleT);

    auto root = createX3DNode("Group");
    addChild(root, rotT);

    Scene scene;
    scene.addRootNode(root);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);

    auto snap = ex.fullSnapshot();
    CHECK((snap.added.size() == 1));
    const extract::RenderItem &it = ex.item(snap.added[0]);

    // Transform a local point (1,0,0) through the accumulated world matrix.
    //   scale(1,0,0) = (2,0,0)
    //   rotateZ90(2,0,0) = (0,2,0)
    SFVec3f worldP = it.worldTransform.transformPoint(SFVec3f{1.0f, 0.0f, 0.0f});
    CHECK((feq(worldP.x, 0.0f)));
    CHECK((feq(worldP.y, 2.0f)));
    CHECK((feq(worldP.z, 0.0f)));
  }

  // === 4) Material/geometry pairing when one is missing =====================
  {
    // 4a) Shape with NO geometry field set => nothing emits.
    {
      auto shape = createX3DNode("Shape");
      // geometry left unset.
      Scene scene;
      scene.addRootNode(shape);
      X3DExecutionContext ctx;
      ctx.buildSceneGraph(scene);
      extract::SceneExtractor ex(ctx, scene);
      auto snap = ex.fullSnapshot();
      CHECK((snap.added.empty()));
    }

    // 4b) Shape with geometry but NO Appearance => Unlit white fallback.
    {
      auto shape = makeTriShape();
      Scene scene;
      scene.addRootNode(shape);
      X3DExecutionContext ctx;
      ctx.buildSceneGraph(scene);
      extract::SceneExtractor ex(ctx, scene);
      auto snap = ex.fullSnapshot();
      CHECK((snap.added.size() == 1));
      const extract::RenderItem &it = ex.item(snap.added[0]);
      CHECK((it.material.model == extract::MaterialModel::Unlit));
      CHECK((feq(it.material.toRGBA().r, 1.0f)));
      CHECK((feq(it.material.toRGBA().g, 1.0f)));
      CHECK((feq(it.material.toRGBA().b, 1.0f)));
    }

    // 4c) Shape with Appearance present but material null => Unlit white (MAT-001).
    {
      auto coord = createX3DNode("Coordinate");
      setF(coord, "point",
           std::any(std::vector<SFVec3f>{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}));
      auto tri = createX3DNode("TriangleSet");
      setF(tri, "coord", std::any(std::shared_ptr<X3DNode>(coord)));

      auto app = createX3DNode("Appearance");
      // material left null.
      auto shape = createX3DNode("Shape");
      setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(tri)));
      setF(shape, "appearance", std::any(std::shared_ptr<X3DNode>(app)));

      Scene scene;
      scene.addRootNode(shape);
      X3DExecutionContext ctx;
      ctx.buildSceneGraph(scene);
      extract::SceneExtractor ex(ctx, scene);
      auto snap = ex.fullSnapshot();
      CHECK((snap.added.size() == 1));
      const extract::RenderItem &it = ex.item(snap.added[0]);
      CHECK((it.material.model == extract::MaterialModel::Unlit));
      CHECK((feq(it.material.toRGBA().r, 1.0f)));
      CHECK((feq(it.material.toRGBA().g, 1.0f)));
      CHECK((feq(it.material.toRGBA().b, 1.0f)));
    }
  }

  // === 5) USE-node extraction (shared geometry extracted once) ==============
  {
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
    setF(tA, "translation", std::any(SFVec3f{10, 0, 0}));
    addChild(tA, shapeA);
    auto tB = createX3DNode("Transform");
    setF(tB, "translation", std::any(SFVec3f{-10, 0, 0}));
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

    const extract::RenderItem &itA = ex.item(snap.added[0]);
    const extract::RenderItem &itB = ex.item(snap.added[1]);

    // Distinct PathKeys => distinct RenderItemIds.
    CHECK((snap.added[0] != snap.added[1]));

    // But both reference the SAME geometry node (shared, uploaded once).
    CHECK((itA.geometry.node == itB.geometry.node));
    CHECK((itA.geometry.node == sharedGeom.get()));

    // Same contentVersion at first extraction.
    CHECK((itA.geometry.contentVersion == itB.geometry.contentVersion));
    CHECK((itA.geometry.contentVersion == 0));
  }

  return;
}
