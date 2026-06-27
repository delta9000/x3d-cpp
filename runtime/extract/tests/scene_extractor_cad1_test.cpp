// scene_extractor_cad1_test.cpp — CAD-1 (2026-06-25 sweep): CADFace.shape accepts
// only Shape|LOD|Transform (ISO/IEC 19775-1 §32.4.2). A non-conforming node placed
// in the shape slot must not be traversed/rendered, mirroring the Collision.proxy
// and Switch guards.
#include "SceneExtractor.hpp"

#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DScene.hpp"

#include <any>
#include "doctest/doctest.h"
#include <memory>
#include <vector>

using namespace x3d::runtime;

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
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

static std::size_t renderedUnder(const std::shared_ptr<X3DNode> &shapeChild) {
  auto cadFace = createX3DNode("CADFace");
  setF(cadFace, "shape", std::any(shapeChild));
  Scene scene;
  scene.addRootNode(cadFace);
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  extract::SceneExtractor ex(ctx, scene);
  return ex.fullSnapshot().added.size();
}

TEST_CASE("scene_extractor_cad1_test") {
  // Conforming: CADFace.shape = Shape -> renders.
  CHECK((renderedUnder(makeTriShape()) == 1));

  // Non-conforming: a Group is not Shape|LOD|Transform; the guard must skip it,
  // so its renderable descendant is NOT emitted (would be 1 without the guard).
  auto group = createX3DNode("Group");
  setF(group, "children",
       std::any(std::vector<std::shared_ptr<X3DNode>>{makeTriShape()}));
  CHECK((renderedUnder(group) == 0));
}
