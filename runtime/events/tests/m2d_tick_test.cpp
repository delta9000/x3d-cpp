// m2d_tick_test.cpp — pick + viewMatrix via the context.
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp"
#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
#include <vector>
using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;
static bool feq(float a, float b) { return std::fabs(a - b) < 1e-2f; }
static void setF(const std::shared_ptr<X3DNode>& n, const char* nm, std::any v) {
  for (auto& f : n->fields()) if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
// (addChild removed: dead — every TEST_CASE below uses setF directly.)

TEST_CASE("m2d_tick_test") {
  auto shape = createX3DNode("Shape");
  auto box = createX3DNode("Box"); setF(box, "size", std::any(SFVec3f{2,2,2}));
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(box)));
  auto vp = createX3DNode("Viewpoint"); setF(vp, "position", std::any(SFVec3f{0,0,10}));
  Scene scene; scene.addRootNode(shape); scene.addRootNode(vp);

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);

  auto hit = ctx.pick(Ray{{0,0,10},{0,0,-1}});
  CHECK((hit.hit && hit.node == shape.get() && feq(hit.point.z, 1)));

  // Viewpoint at (0,0,10), default orientation -> view = translate(0,0,-10).
  Mat4 view = ctx.viewMatrix();
  SFVec3f origin = view.transformPoint({0,0,0});
  CHECK((feq(origin.z, -10) && feq(origin.x, 0) && feq(origin.y, 0)));
  return;
}
