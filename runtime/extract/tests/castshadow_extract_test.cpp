// castshadow_extract_test.cpp — X3DShapeNode.castShadow is surfaced on
// RenderItem: default TRUE (X3D default), FALSE when authored. Feeds the
// shadow-visibility seam (ADR-0028): occluders count only when castShadow=TRUE.
#include "SceneExtractor.hpp"

#include "X3DExecutionContext.hpp"
#include "X3DNodeFactory.hpp"
#include "X3DScene.hpp"

#include <any>
#include "doctest/doctest.h"
#include <memory>
#include <vector>

using namespace x3d::runtime;

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) {
      f.set(*n, std::move(v));
      return;
    }
}

// A geometry-bearing Shape over a one-triangle TriangleSet (emits one item).
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

static const extract::RenderItem &soleItem(extract::SceneExtractor &ex) {
  auto snap = ex.fullSnapshot();
  REQUIRE(snap.added.size() == 1);
  return ex.item(snap.added[0]);
}

TEST_CASE("castshadow_extract_test") {
  // Default: castShadow unauthored -> RenderItem.castShadow == true (X3D default).
  {
    auto shape = makeTriShape();
    Scene scene;
    scene.addRootNode(shape);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);
    CHECK((soleItem(ex).castShadow == true));
  }

  // Authored castShadow=FALSE -> RenderItem.castShadow == false.
  {
    auto shape = makeTriShape();
    setF(shape, "castShadow", std::any(SFBool{false}));
    Scene scene;
    scene.addRootNode(shape);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);
    CHECK((soleItem(ex).castShadow == false));
  }
}
