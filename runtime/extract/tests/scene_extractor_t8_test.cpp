// scene_extractor_t8_test.cpp — M2.5 T8 acceptance: SceneExtractor delta()
// incremental engine. Proves the four pillars of the per-path-correct,
// pollution-safe delta channel:
//
//   1) A TRANSFORM-ONLY tick yields ONLY updatedTransform (no added/removed/
//      geometry/material churn) AND a USE-shared placement under two Transforms
//      re-accumulates each placement's world matrix correctly along its STORED
//      PathKey — never the first-path ctx.worldTransform() table.
//   2) PARENT SCALE animation does NOT mark updatedGeometry (the DirtyBounds-
//      pollution guard: updatedGeometry fires on a content DirtyField, never the
//      bare DirtyBounds bit ancestor-scale sets).
//   3) A MATERIAL-COLOR change (diffuseColor delivered onto the Material) yields
//      updatedMaterial with the new color re-read into the RenderItem.
//   4) DirtyChildren on a grouping node yields added/removed via the cached
//      entry-matrix subtree re-walk.
#include "SceneExtractor.hpp"

#include "X3DDocument.hpp" // Scene::addRootNode definition.
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DScene.hpp"

#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;

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

// A geometry-bearing Shape over a one-triangle TriangleSet. Returns the shape;
// out-params surface the geometry + coord nodes for content-change tests.
static std::shared_ptr<X3DNode>
makeTriShape(std::shared_ptr<X3DNode> *geomOut = nullptr,
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

TEST_CASE("scene_extractor_t8_test") {
  // === 1) Transform-only tick => ONLY updatedTransform; USE-shared placement ==
  //        re-accumulates each placement correctly (per-path, not first-path).
  {
    // A single DEF'd Shape USE'd under TWO Transforms => two RenderItems sharing
    // one GeomId but distinct PathKeys / worldTransforms.
    auto shape = makeTriShape();
    auto tA = createX3DNode("Transform");
    setF(tA, "translation", std::any(SFVec3f{10, 0, 0}));
    addChild(tA, shape);
    auto tB = createX3DNode("Transform");
    setF(tB, "translation", std::any(SFVec3f{0, 20, 0}));
    addChild(tB, shape); // SAME shared_ptr — the USE placement.

    Scene scene;
    scene.addRootNode(tA);
    scene.addRootNode(tB);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);

    auto snap = ex.fullSnapshot();
    CHECK((snap.added.size() == 2)); // two placements of the shared geometry.

    // Identify which item is under tA vs tB by inspecting its path.
    auto underTransform = [&](const X3DNode *t) -> extract::RenderItemId {
      for (extract::RenderItemId id : snap.added)
        for (const X3DNode *n : ex.item(id).path)
          if (n == t) return id;
      return extract::kInvalidRenderItemId;
    };
    extract::RenderItemId idA = underTransform(tA.get());
    extract::RenderItemId idB = underTransform(tB.get());
    CHECK((idA != extract::kInvalidRenderItemId));
    CHECK((idB != extract::kInvalidRenderItemId));
    CHECK((idA != idB));

    // Sanity: initial world translations are the authored per-path values.
    CHECK((feq(ex.item(idA).worldTransform.m[12], 10.0f)));
    CHECK((feq(ex.item(idB).worldTransform.m[13], 20.0f)));

    // Animate ONLY tA's translation, then ONE tick + ONE delta().
    ctx.postEvent(tA.get(), "translation", std::any(SFVec3f{99, 0, 0}));
    ctx.tick(1.0);
    auto d = ex.delta();

    // ONLY the transform channel fired — no re-walk, no geometry/material churn.
    CHECK((d.added.empty()));
    CHECK((d.removed.empty()));
    CHECK((d.updatedGeometry.empty()));
    CHECK((d.updatedMaterial.empty()));
    CHECK((d.updatedTransform.size() == 1));
    CHECK((d.updatedTransform[0] == idA));

    // The re-accumulated world matrix reflects the NEW translation; the OTHER
    // USE placement (under tB) is UNTOUCHED — proving per-path correctness, not a
    // first-path-collapsed ctx.worldTransform() read.
    CHECK((feq(ex.item(idA).worldTransform.m[12], 99.0f)));
    CHECK((feq(ex.item(idB).worldTransform.m[13], 20.0f)));
    CHECK((feq(ex.item(idB).worldTransform.m[12], 0.0f)));
  }

  // === 2) Parent SCALE animation does NOT mark updatedGeometry ===============
  {
    auto shape = makeTriShape();
    auto parent = createX3DNode("Transform");
    addChild(parent, shape);
    Scene scene;
    scene.addRootNode(parent);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);
    auto snap = ex.fullSnapshot();
    CHECK((snap.added.size() == 1));

    // Animate the PARENT's scale: classifyDirty sets DirtyLocalTransform|
    // DirtyBounds on the Transform and BoundsSystem walks DirtyBounds UP — the
    // geometry leaf is never DirtyField'd. updatedGeometry must stay empty.
    ctx.postEvent(parent.get(), "scale", std::any(SFVec3f{2, 2, 2}));
    ctx.tick(1.0);
    auto d = ex.delta();
    CHECK((d.updatedGeometry.empty())); // the pollution guard.
    CHECK((d.updatedTransform.size() == 1)); // scale IS a transform update.
  }

  // === 2b) An ACTUAL geometry content change DOES mark updatedGeometry =======
  {
    std::shared_ptr<X3DNode> geom, coord;
    auto shape = makeTriShape(&geom, &coord);
    Scene scene;
    scene.addRootNode(shape);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);
    auto snap = ex.fullSnapshot();
    CHECK((snap.added.size() == 1));
    std::uint32_t v0 = ex.item(snap.added[0]).geometry.contentVersion;

    // Change the TriangleSet's coord (content) => DirtyField on the geometry.
    ctx.postEvent(coord.get(), "point",
                  std::any(std::vector<SFVec3f>{{0, 0, 0}, {2, 0, 0}, {0, 2, 0}}));
    ctx.tick(1.0);
    auto d = ex.delta();
    CHECK((d.updatedGeometry.size() == 1));
    CHECK((d.updatedGeometry[0] == snap.added[0]));
    // contentVersion bumped so the consumer's GeomId-keyed cache orphans.
    CHECK((ex.item(snap.added[0]).geometry.contentVersion == v0 + 1));
  }

  // === 3) Material-color change yields updatedMaterial =======================
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
    CHECK((feq(ex.item(snap.added[0]).material.phong.diffuse.r, 0.1f)));

    // Drive a diffuseColor change onto the Material (the dirty-channel effect of a
    // material-color ROUTE) => DirtyField on the Material node => updatedMaterial.
    ctx.postEvent(material.get(), "diffuseColor",
                  std::any(SFColor{0.9f, 0.8f, 0.7f}));
    ctx.tick(1.0);
    auto d = ex.delta();
    CHECK((d.updatedMaterial.size() == 1));
    CHECK((d.updatedMaterial[0] == snap.added[0]));
    CHECK((d.updatedGeometry.empty()));
    CHECK((d.updatedTransform.empty()));
    // The new color was re-read into the RenderItem.
    CHECK((feq(ex.item(snap.added[0]).material.phong.diffuse.r, 0.9f)));
    CHECK((feq(ex.item(snap.added[0]).material.phong.diffuse.g, 0.8f)));
    CHECK((feq(ex.item(snap.added[0]).material.phong.diffuse.b, 0.7f)));
  }

  // === 4) DirtyChildren on a grouping node yields added/removed ==============
  {
    auto group = createX3DNode("Group");
    auto shape0 = makeTriShape();
    addChild(group, shape0);
    Scene scene;
    scene.addRootNode(group);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);
    auto snap = ex.fullSnapshot();
    CHECK((snap.added.size() == 1));
    extract::RenderItemId id0 = snap.added[0];

    // --- 4a) ADD a child: replace children with {shape0, shape1} ------------
    auto shape1 = makeTriShape();
    {
      // Build the new children vector and deliver it as a children change.
      std::vector<std::shared_ptr<X3DNode>> kids{shape0, shape1};
      ctx.postEvent(group.get(), "children", std::any(kids));
      ctx.tick(1.0);
      auto d = ex.delta();
      CHECK((d.added.size() == 1));   // shape1's new placement.
      CHECK((d.removed.empty()));     // shape0 still present.
      CHECK((d.added[0] != id0));     // a genuinely new RenderItemId.
    }

    // --- 4b) REMOVE a child: children back to {shape1} ----------------------
    {
      std::vector<std::shared_ptr<X3DNode>> kids{shape1};
      ctx.postEvent(group.get(), "children", std::any(kids));
      ctx.tick(2.0);
      auto d = ex.delta();
      // shape0's placement (id0) is no longer re-emitted => removed.
      CHECK((!d.removed.empty()));
      bool removedId0 = false;
      for (extract::RenderItemId id : d.removed)
        if (id == id0) removedId0 = true;
      CHECK((removedId0));
    }
  }

  // === 5) Switch.whichChoice change yields added/removed (incremental delta) ==
  //        Regression: whichChoice was classified DirtyField, which delta()
  //        ignores for a grouping node (it is in neither geomDeps_ nor
  //        materialDeps_), so incremental consumers (the OpenGL PoC) never saw
  //        the active-child swap — only full-snapshot consumers (cpuraster) did.
  //        classifyDirty now maps whichChoice -> DirtyChildren -> subtree re-walk.
  {
    auto sw = createX3DNode("Switch");
    auto shape0 = makeTriShape();
    auto shape1 = makeTriShape();
    addChild(sw, shape0);
    addChild(sw, shape1);
    setF(sw, "whichChoice", std::any(SFInt32{0})); // show child 0.
    Scene scene;
    scene.addRootNode(sw);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);
    auto snap = ex.fullSnapshot();
    CHECK((snap.added.size() == 1)); // only the active child (0) is drawn.
    extract::RenderItemId id0 = snap.added[0];

    // Flip to child 1: child 0's placement must be REMOVED, child 1's ADDED.
    ctx.postEvent(sw.get(), "whichChoice", std::any(SFInt32{1}));
    ctx.tick(1.0);
    auto d = ex.delta();
    CHECK((d.added.size() == 1)); // child 1's new placement.
    CHECK((d.added[0] != id0));   // a genuinely new RenderItemId.
    bool removedId0 = false;
    for (extract::RenderItemId id : d.removed)
      if (id == id0) removedId0 = true;
    CHECK((removedId0)); // child 0's placement is gone.

    // And flipping to whichChoice = -1 removes everything (draw nothing).
    extract::RenderItemId id1 = d.added[0];
    ctx.postEvent(sw.get(), "whichChoice", std::any(SFInt32{-1}));
    ctx.tick(2.0);
    auto d2 = ex.delta();
    CHECK((d2.added.empty()));
    bool removedId1 = false;
    for (extract::RenderItemId id : d2.removed)
      if (id == id1) removedId1 = true;
    CHECK((removedId1));
  }

  return;
}
