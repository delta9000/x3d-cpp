// scene_extractor_m25_5_test.cpp — M25-5 acceptance: non-global DirectionalLight
// scope is honored by tagging RenderItems with in-scope lights only.
//
// X3D §17 / X3DLightNode semantics:
//   * global=TRUE  => light illuminates the WHOLE scene (all RenderItems).
//   * global=FALSE => light illuminates ONLY geometry in the scope of its parent
//     grouping node (the scopeRoot). A RenderItem is in-scope iff the light's
//     scopeRoot is an ANCESTOR (or equal to the last grouping node on the path)
//     of that item's PathKey.
//
// Scene under test:
//
//   Group (outerGroup)
//     ├─ Group (scopeGroup)            ← scopeRoot for the scoped DirectionalLight
//     │    ├─ DirectionalLight(global=false)   [scoped light: only inside scopeGroup]
//     │    └─ Shape (shapeInside)              [INSIDE scope]
//     ├─ Shape (shapeOutside)                  [OUTSIDE scope]
//     └─ PointLight(global=true)               [global light: illuminates all]
//
// Assertions:
//   A) shapeInside's RenderItem carries BOTH lights (scoped DL + global PL).
//   B) shapeOutside's RenderItem carries ONLY the global PointLight (not the DL).
//   C) A scene with ONLY a global=TRUE PointLight tags ALL render items.
#include "SceneExtractor.hpp"

#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DScene.hpp"

#include <any>
#include "doctest/doctest.h"
#include <memory>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;

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

TEST_CASE("scene_extractor_m25_5_test") {
  // ===== A+B) Scoped light tags INSIDE shape but NOT outside shape ===========
  {
    // Build: outerGroup > { scopeGroup > { DL(global=false), shapeInside },
    //                       shapeOutside, PointLight(global=true) }
    auto dl = createX3DNode("DirectionalLight");
    setF(dl, "global", std::any(false)); // explicit false (spec default for DL)

    auto shapeInside = makeTriShape();
    auto scopeGroup = createX3DNode("Group");
    addChild(scopeGroup, dl);
    addChild(scopeGroup, shapeInside);

    auto shapeOutside = makeTriShape();

    auto pl = createX3DNode("PointLight");
    setF(pl, "global", std::any(true)); // explicit true (spec default for PL)

    auto outerGroup = createX3DNode("Group");
    addChild(outerGroup, scopeGroup);
    addChild(outerGroup, shapeOutside);
    addChild(outerGroup, pl);

    Scene scene;
    scene.addRootNode(outerGroup);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);

    extract::SceneExtractor ex(ctx, scene);
    auto snap = ex.fullSnapshot();
    CHECK((snap.added.size() == 2)); // exactly two shapes emitted.

    // Find which RenderItemId corresponds to which shape.
    extract::RenderItemId idInside = extract::kInvalidRenderItemId;
    extract::RenderItemId idOutside = extract::kInvalidRenderItemId;
    for (extract::RenderItemId id : snap.added) {
      const extract::RenderItem &rec = ex.item(id);
      for (const X3DNode *n : rec.path) {
        if (n == shapeInside.get()) { idInside = id; break; }
        if (n == shapeOutside.get()) { idOutside = id; break; }
      }
    }
    CHECK((idInside != extract::kInvalidRenderItemId));
    CHECK((idOutside != extract::kInvalidRenderItemId));

    // Retrieve the tagged light lists. lightsOf() indices are positions in
    // snapshotLights() (the cached snapshot from fullSnapshot), NOT lights().
    const auto &allLights = ex.snapshotLights();
    CHECK((allLights.size() == 2)); // DL + PL collected.

    const std::vector<std::size_t> &insideLights = ex.lightsOf(idInside);
    const std::vector<std::size_t> &outsideLights = ex.lightsOf(idOutside);

    // A) shapeInside must be tagged with BOTH lights.
    CHECK((insideLights.size() == 2));

    // B) shapeOutside must be tagged with ONLY the global PointLight, NOT the DL.
    CHECK((outsideLights.size() == 1));
    // The one light tagging shapeOutside must be global.
    CHECK((allLights[outsideLights[0]].global == true));
    CHECK((allLights[outsideLights[0]].type == extract::LightDesc::Type::Point));

    // Double-check insideLights contains both types.
    bool hasDir = false, hasPoint = false;
    for (std::size_t li : insideLights) {
      if (allLights[li].type == extract::LightDesc::Type::Directional) hasDir = true;
      if (allLights[li].type == extract::LightDesc::Type::Point) hasPoint = true;
    }
    CHECK((hasDir && hasPoint));
  }

  // ===== C) global=TRUE PointLight alone tags ALL render items ===============
  {
    auto pl = createX3DNode("PointLight");
    setF(pl, "global", std::any(true));

    auto shape1 = makeTriShape();
    auto shape2 = makeTriShape();

    auto group = createX3DNode("Group");
    addChild(group, pl);
    addChild(group, shape1);
    addChild(group, shape2);

    Scene scene;
    scene.addRootNode(group);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);

    extract::SceneExtractor ex(ctx, scene);
    auto snap = ex.fullSnapshot();
    CHECK((snap.added.size() == 2));

    for (extract::RenderItemId id : snap.added) {
      const std::vector<std::size_t> &lit = ex.lightsOf(id);
      CHECK((lit.size() == 1)); // exactly the one global light.
    }
  }

  // ===== D) No lights => all items have empty light list =====================
  {
    auto shape = makeTriShape();
    Scene scene;
    scene.addRootNode(shape);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);

    extract::SceneExtractor ex(ctx, scene);
    auto snap = ex.fullSnapshot();
    CHECK((snap.added.size() == 1));
    CHECK((ex.lightsOf(snap.added[0]).empty()));
  }

  return;
}
