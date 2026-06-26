// scene_extractor_t7_test.cpp — M2.5 T7 acceptance: SceneExtractor FULL form.
//
// Proves the widenings T7 adds on top of the T7a spine:
//   1) VISIBILITY-aware DFS special-casing Switch by nodeTypeName BEFORE the
//      generic child loop:
//        * whichChoice default -1 => draw NOTHING (not "the first child", which a
//          blind forEachChild would produce).
//        * whichChoice = 1 => draw ONLY children[1].
//   2) LOD draws exactly ONE child (the level-0 static selection, documented).
//   3) MeshBuilder[T4] is wired for ALL geometry: a Box Shape emits a tessellated
//      (12-tri / 36-corner) RenderItem.
//   4) MaterialSystem[T5] is wired: a Shape with a real Material yields a Phong
//      MaterialDesc with the authored diffuse, NOT the Unlit-white stub.
//   5) lights()/background() read-outs + camera() surfaces OrthoViewpoint ortho.
//   6) The three reverse indices resolve a node -> its RenderItemIds.
#include "SceneExtractor.hpp"

#include "X3DDocument.hpp" // Scene::addRootNode definition.
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

// A geometry-bearing Shape over a one-triangle TriangleSet.
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

TEST_CASE("scene_extractor_t7_test") {
  // === 1) Switch whichChoice = -1 (default) draws NOTHING ===================
  {
    auto sw = createX3DNode("Switch");
    addChild(sw, makeTriShape());
    addChild(sw, makeTriShape());
    // whichChoice defaults to -1 — leave unset.
    Scene scene;
    scene.addRootNode(sw);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);
    auto snap = ex.fullSnapshot();
    CHECK((snap.added.empty())); // -1 => nothing (NOT the blind first child).
  }

  // === 2) Switch whichChoice = 1 draws ONLY children[1] =====================
  {
    auto sw = createX3DNode("Switch");
    auto child0 = makeTriShape();
    auto child1 = makeTriShape();
    addChild(sw, child0);
    addChild(sw, child1);
    setF(sw, "whichChoice", std::any(static_cast<std::int32_t>(1)));
    Scene scene;
    scene.addRootNode(sw);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);
    auto snap = ex.fullSnapshot();
    CHECK((snap.added.size() == 1)); // exactly one drawn child.
    // The drawn item's path passes through child1, NOT child0.
    const extract::RenderItem &it = ex.item(snap.added[0]);
    bool throughChild1 = false, throughChild0 = false;
    for (const X3DNode *n : it.path) {
      if (n == child1.get()) throughChild1 = true;
      if (n == child0.get()) throughChild0 = true;
    }
    CHECK((throughChild1 && !throughChild0));
  }

  // === 3) LOD draws exactly ONE child (level-0 static selection) ============
  {
    auto lod = createX3DNode("LOD");
    addChild(lod, makeTriShape()); // level 0 (highest detail)
    addChild(lod, makeTriShape()); // level 1
    addChild(lod, makeTriShape()); // level 2
    Scene scene;
    scene.addRootNode(lod);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);
    auto snap = ex.fullSnapshot();
    CHECK((snap.added.size() == 1)); // exactly one level drawn (not all three).
  }

  // === 4) Box now emits a tessellated RenderItem (MeshBuilder[T4] wired) =====
  {
    auto box = createX3DNode("Box");
    auto shape = createX3DNode("Shape");
    setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(box)));
    Scene scene;
    scene.addRootNode(shape);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);
    auto snap = ex.fullSnapshot();
    CHECK((snap.added.size() == 1));
    const extract::RenderItem &it = ex.item(snap.added[0]);
    CHECK((it.mesh.indices.size() == 36)); // 6 faces * 2 tris * 3 corners.
  }

  // === 5) Shape with a real Material gets the real MaterialDesc (T5 wired) ===
  {
    auto coord = createX3DNode("Coordinate");
    setF(coord, "point",
         std::any(std::vector<SFVec3f>{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}));
    auto tri = createX3DNode("TriangleSet");
    setF(tri, "coord", std::any(std::shared_ptr<X3DNode>(coord)));

    auto material = createX3DNode("Material");
    setF(material, "diffuseColor", std::any(SFColor{0.1f, 0.2f, 0.3f}));
    auto app = createX3DNode("Appearance");
    setF(app, "material", std::any(std::shared_ptr<X3DNode>(material)));

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
    // Real material: Phong, NOT the Unlit-white stub. (Material.diffuseColor
    // maps to phong.diffuse in the discriminated-union MaterialDesc.)
    CHECK((it.material.model == extract::MaterialModel::Phong));
    CHECK((feq(it.material.phong.diffuse.r, 0.1f)));
    CHECK((feq(it.material.phong.diffuse.g, 0.2f)));
    CHECK((feq(it.material.phong.diffuse.b, 0.3f)));

    // --- reverse indices: the material node resolves to this RenderItem ---
    const auto &mids = ex.materialDepsOf(material.get());
    CHECK((mids.size() == 1 && mids[0] == snap.added[0]));
    const auto &gids = ex.geomDepsOf(tri.get());
    CHECK((gids.size() == 1 && gids[0] == snap.added[0]));
  }

  // === 6) lights() + background() read-outs ================================
  {
    auto dl = createX3DNode("DirectionalLight");
    auto bg = createX3DNode("Background");
    setF(bg, "skyColor", std::any(std::vector<SFColor>{{0.2f, 0.3f, 0.4f}}));
    auto group = createX3DNode("Group");
    addChild(group, dl);
    addChild(group, bg);
    Scene scene;
    scene.addRootNode(group);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    // Bind the Background so boundBackground() resolves it.
    ctx.tick(0.0);
    extract::SceneExtractor ex(ctx, scene);
    auto lights = ex.lights();
    CHECK((lights.size() == 1));
    CHECK((lights[0].type == extract::LightDesc::Type::Directional));
    // Background read-out is present (skyColor surfaced when bound).
    auto bgDesc = ex.background();
    CHECK((bgDesc.backgroundChanged));
  }

  // === 7) camera() surfaces OrthoViewpoint ortho=true =======================
  {
    auto ovp = createX3DNode("OrthoViewpoint");
    Scene scene;
    scene.addRootNode(ovp);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    ctx.tick(0.0); // bind the viewpoint.
    extract::SceneExtractor ex(ctx, scene);
    auto cam = ex.camera();
    CHECK((cam.ortho == true)); // OrthoViewpoint surfaced ortho.
  }

  return;
}
