// m2b_tick_test.cpp — buildSceneGraph wires bounds; after a tick the world bounds
// of a translated Shape>Box are correct, and changing the box via the cascade
// updates them.
#include "X3DExecutionContext.hpp"
#include "X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp"
#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
#include <vector>
using namespace x3d::runtime;
static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }
static void setF(const std::shared_ptr<X3DNode>& n, const char* nm, std::any v) {
  for (auto& f : n->fields()) if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
static void addChild(const std::shared_ptr<X3DNode>& p, const std::shared_ptr<X3DNode>& c) {
  for (auto& f : p->fields()) if (f.x3dName == "children" && f.set) {
    auto k = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*p));
    k.push_back(c); f.set(*p, std::any(std::move(k))); return;
  }
}

TEST_CASE("m2b_tick_test") {
  auto T = createX3DNode("Transform"); setF(T, "translation", std::any(SFVec3f{5,0,0}));
  auto shape = createX3DNode("Shape");
  auto box = createX3DNode("Box"); setF(box, "size", std::any(SFVec3f{2,2,2}));
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(box)));
  addChild(T, shape);
  Scene scene; scene.addRootNode(T);

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  ctx.tick(0.0);

  Aabb wb = ctx.worldBounds(T.get());        // [4,6]x[-1,1]x[-1,1]
  CHECK((feq(wb.min.x,4) && feq(wb.max.x,6) && feq(wb.max.y,1)));
  Aabb lb = ctx.localBounds(shape.get());    // box bounds in shape's frame
  CHECK((feq(lb.max.x,1) && feq(lb.min.x,-1)));
  return;
}
