// scene_extractor_collision_proxy_test.cpp —  (campaign wave-4 fix): a Collision node's
// `proxy` geometry is collision-only and must never be emitted as a visible render
// item (ISO/IEC 19775-1 §23.4.2). The Collision `children` still render normally.
#include "SceneExtractor.hpp"

#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DScene.hpp"

#include <any>
#include "doctest/doctest.h"
#include "scene_test_helpers.hpp"
#include <memory>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;
using x3d::test::setF;

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

TEST_CASE("scene_extractor_col2_test") {
  // Collision with one rendered child Shape + a proxy Shape (collision-only).
  auto collision = createX3DNode("Collision");
  setF(collision, "children",
       std::any(std::vector<std::shared_ptr<X3DNode>>{makeTriShape()}));
  setF(collision, "proxy", std::any(std::shared_ptr<X3DNode>(makeTriShape())));

  Scene scene;
  scene.addRootNode(collision);
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  extract::SceneExtractor ex(ctx, scene);
  auto snap = ex.fullSnapshot();

  // Only the child renders; the proxy is excluded (§23.4.2).
  CHECK((snap.added.size() == 1));
  return;
}
