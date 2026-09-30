// m2b_tick_test.cpp — buildSceneGraph wires bounds; after a tick the world bounds
// of a translated Shape>Box are correct, and changing the box via the cascade
// updates them.
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp"
#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
#include <vector>
using namespace x3d::core;
using namespace x3d::nodes;
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

TEST_CASE("m2b_tick_geometry_replacement_refreshes_node_indexes") {
  auto root = createX3DNode("Transform");
  setF(root, "translation", SFVec3f{10, 0, 0});
  auto shape = createX3DNode("Shape");
  auto oldBox = createX3DNode("Box");
  std::weak_ptr<X3DNode> old = oldBox;
  setF(shape, "geometry", oldBox);
  addChild(root, shape);
  Scene scene; scene.addRootNode(root);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  oldBox.reset();

  auto box = createX3DNode("Box");
  setF(box, "size", SFVec3f{6, 6, 6});
  ctx.postEvent(shape.get(), "set_geometry", box);
  ctx.tick(0);
  CHECK(old.expired());
  CHECK((ctx.dirtyTracker().flags(shape.get()) & DirtyChildren));
  CHECK(feq(ctx.worldBounds(root.get()).min.x, 7));
  CHECK(feq(ctx.worldBounds(root.get()).max.x, 13));
  CHECK(feq(ctx.worldTransformAny(box.get()).transformPoint({0,0,0}).x, 10));

  ctx.postEvent(shape.get(), "set_geometry", std::shared_ptr<X3DNode>{});
  ctx.tick(1);
  CHECK(ctx.localBounds(root.get()).empty);
}

TEST_CASE("m2b_tick_coordinate_replacement_refreshes_bounds") {
  auto shape = createX3DNode("Shape");
  auto geometry = createX3DNode("PointSet");
  auto coord = createX3DNode("Coordinate");
  setF(coord, "point", MFVec3f{{-1,0,0}, {1,0,0}});
  setF(geometry, "coord", coord); setF(shape, "geometry", geometry);
  Scene scene; scene.addRootNode(shape);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  coord.reset();
  auto replacement = createX3DNode("Coordinate");
  setF(replacement, "point", MFVec3f{{-3,0,0}, {5,0,0}});
  ctx.postEvent(geometry.get(), "set_coord", replacement);
  ctx.tick(0);
  CHECK((ctx.dirtyTracker().flags(geometry.get()) & DirtyChildren));
  CHECK(feq(ctx.localBounds(shape.get()).min.x, -3));
  CHECK(feq(ctx.localBounds(shape.get()).max.x, 5));
  ctx.postEvent(replacement.get(), "set_point", MFVec3f{{-4,0,0}, {6,0,0}});
  ctx.tick(1);
  CHECK(feq(ctx.localBounds(shape.get()).min.x, -4));
  CHECK(feq(ctx.localBounds(shape.get()).max.x, 6));
}
