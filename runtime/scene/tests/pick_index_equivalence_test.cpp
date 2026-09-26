// pick_index_equivalence_test.cpp
// Characterization lock for the lazily-indexed PickSystem. Records pickClosest
// results (hit node, point, normal, path) for the scene shapes the pick index must
// reproduce EXACTLY: multi-mesh closest selection, DEF/USE per-path instancing,
// an animated Transform changed between picks, and a Billboard (view-dependent).
//
// This is the "same answers as the reflective whole-graph walk" contract: the
// index replaces the walk but must not change a single returned result.
#include "PickSystem.hpp"
#include "TransformSystem.hpp"
#include "BoundsSystem.hpp"
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

static bool feq(float a, float b, float eps = 1e-3f) { return std::fabs(a - b) < eps; }

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
static void addChild(const std::shared_ptr<X3DNode> &p, const std::shared_ptr<X3DNode> &c) {
  for (auto &f : p->fields())
    if (f.x3dName == "children" && f.set) {
      auto k = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*p));
      k.push_back(c);
      f.set(*p, std::any(std::move(k)));
      return;
    }
}
static std::shared_ptr<X3DNode> shapeWith(const std::shared_ptr<X3DNode> &geom) {
  auto s = createX3DNode("Shape");
  setF(s, "geometry", std::any(std::shared_ptr<X3DNode>(geom)));
  return s;
}
// A unit quad (two triangles) in the local z=0 plane over [-1,1]^2.
static std::shared_ptr<X3DNode> quadMesh() {
  auto ifs = createX3DNode("IndexedFaceSet");
  auto coord = createX3DNode("Coordinate");
  setF(coord, "point", std::any(std::vector<SFVec3f>{{-1,-1,0},{1,-1,0},{1,1,0},{-1,1,0}}));
  setF(ifs, "coord", std::any(std::static_pointer_cast<X3DNode>(coord)));
  setF(ifs, "coordIndex", std::any(std::vector<int>{0,1,2,3,-1}));
  return ifs;
}

TEST_CASE("pick_index_multi_mesh_closest") {
  auto root = createX3DNode("Group");
  auto nearShape = shapeWith(quadMesh());                 // at z=0
  auto farT = createX3DNode("Transform");
  setF(farT, "translation", std::any(SFVec3f{0, 0, -5}));
  auto farShape = shapeWith(quadMesh());
  addChild(farT, farShape);
  addChild(root, nearShape);
  addChild(root, farT);

  Scene sc; sc.addRootNode(root);
  TransformSystem ts; ts.buildIndex(sc);
  BoundsSystem bs; bs.buildBounds(sc, ts);
  PickSystem ps; ps.build(sc);

  // Nearest mesh wins along a ray that crosses both.
  auto hit = ps.pickClosest(Ray{{0, 0, 10}, {0, 0, -1}}, bs);
  CHECK(hit.hit);
  CHECK(hit.node == nearShape.get());
  CHECK((feq(hit.point.x, 0) && feq(hit.point.y, 0) && feq(hit.point.z, 0)));
  CHECK((feq(hit.normal.x, 0) && feq(hit.normal.y, 0) && feq(hit.normal.z, 1)));
  CHECK(hit.path.size() == 2);
  CHECK((hit.path[0] == root.get() && hit.path[1] == nearShape.get()));

  // The far instance is reachable on its own transform (z=-5) from behind.
  auto far = ps.pickClosest(Ray{{0, 0, -10}, {0, 0, 1}}, bs);
  CHECK(far.hit);
  CHECK(far.node == farShape.get());
  CHECK((feq(far.point.x, 0) && feq(far.point.y, 0) && feq(far.point.z, -5)));
  CHECK(far.path.size() == 3);
  CHECK((far.path[0] == root.get() && far.path[1] == farT.get() &&
         far.path[2] == farShape.get()));
}

TEST_CASE("pick_index_def_use_per_path") {
  // ONE Shape node placed under two Transforms (DEF/USE). Each placement keeps its
  // own PathKey and world AABB; the pick must resolve the RIGHT path, not collapse
  // the two instances to a single entry.
  auto root = createX3DNode("Group");
  auto box = createX3DNode("Box"); setF(box, "size", std::any(SFVec3f{2, 2, 2}));
  auto shared = shapeWith(box);
  auto t1 = createX3DNode("Transform"); setF(t1, "translation", std::any(SFVec3f{-2, 0, 0}));
  auto t2 = createX3DNode("Transform"); setF(t2, "translation", std::any(SFVec3f{ 2, 0, 0}));
  addChild(t1, shared);
  addChild(t2, shared);
  addChild(root, t1);
  addChild(root, t2);

  Scene sc; sc.addRootNode(root);
  TransformSystem ts; ts.buildIndex(sc);
  BoundsSystem bs; bs.buildBounds(sc, ts);
  PickSystem ps; ps.build(sc);

  auto right = ps.pickClosest(Ray{{2, 0, 10}, {0, 0, -1}}, bs);
  CHECK(right.hit);
  CHECK(right.node == shared.get());
  CHECK((feq(right.point.x, 2) && feq(right.point.z, 1)));
  CHECK(right.path.size() == 3);
  CHECK((right.path[0] == root.get() && right.path[1] == t2.get() &&
         right.path[2] == shared.get()));

  auto left = ps.pickClosest(Ray{{-2, 0, 10}, {0, 0, -1}}, bs);
  CHECK(left.hit);
  CHECK(left.node == shared.get());
  CHECK((feq(left.point.x, -2) && feq(left.point.z, 1)));
  CHECK((left.path[1] == t1.get())); // the OTHER placement's path
}

TEST_CASE("pick_index_animated_transform") {
  auto root = createX3DNode("Group");
  auto T = createX3DNode("Transform"); setF(T, "translation", std::any(SFVec3f{5, 0, 0}));
  auto box = createX3DNode("Box"); setF(box, "size", std::any(SFVec3f{2, 2, 2}));
  auto shape = shapeWith(box);
  addChild(T, shape);
  addChild(root, T);

  Scene sc; sc.addRootNode(root);
  TransformSystem ts; ts.buildIndex(sc);
  BoundsSystem bs; bs.buildBounds(sc, ts);
  PickSystem ps; ps.build(sc);

  auto first = ps.pickClosest(Ray{{5, 0, 10}, {0, 0, -1}}, bs);
  CHECK((first.hit && first.node == shape.get() && feq(first.point.x, 5) &&
         feq(first.point.z, 1)));

  // Move the Transform between picks: the index must see the new world frame.
  setF(T, "translation", std::any(SFVec3f{10, 0, 0}));

  auto moved = ps.pickClosest(Ray{{10, 0, 10}, {0, 0, -1}}, bs);
  CHECK((moved.hit && feq(moved.point.x, 10) && feq(moved.point.z, 1)));
  auto stale = ps.pickClosest(Ray{{5, 0, 10}, {0, 0, -1}}, bs);
  CHECK_FALSE(stale.hit); // the shape is no longer at x=5
}

TEST_CASE("pick_index_billboard_view_dependent") {
  auto bb = createX3DNode("Billboard");
  setF(bb, "axisOfRotation", std::any(SFVec3f{0, 1, 0}));
  auto box = createX3DNode("Box"); setF(box, "size", std::any(SFVec3f{1, 1, 4}));
  auto shape = shapeWith(box);
  addChild(bb, shape);

  Scene sc; sc.addRootNode(bb);
  TransformSystem ts; ts.buildIndex(sc);
  BoundsSystem bs; bs.buildBounds(sc, ts);
  PickSystem ps; ps.build(sc);

  // Viewer on +X: the billboard turns the box's depth (+Z) into world X, so a ray
  // at x=1.5 (outside the un-rotated [-0.5,0.5]) still hits the front face.
  auto hit = ps.pickClosest(Ray{{1.5f, 0, 10}, {0, 0, -1}}, bs, {10, 0, 0}, {0, 1, 0});
  CHECK(hit.hit);
  CHECK(hit.node == shape.get());
  CHECK((feq(hit.point.x, 1.5f) && feq(hit.point.z, 0.5f)));
  CHECK(hit.path.size() == 2);
  CHECK((hit.path[0] == bb.get() && hit.path[1] == shape.get()));

  // Same ray, viewer on +Z: the un-rotated box only spans x in [-0.5,0.5], so the
  // billboard geometry is NOT cached across camera poses — the pick now misses.
  auto missSameRay = ps.pickClosest(Ray{{1.5f, 0, 10}, {0, 0, -1}}, bs, {0, 0, 10}, {0, 1, 0});
  CHECK_FALSE(missSameRay.hit);

  // Beyond the rotated X extent even for the +X viewer.
  auto miss = ps.pickClosest(Ray{{2.5f, 0, 10}, {0, 0, -1}}, bs, {10, 0, 0}, {0, 1, 0});
  CHECK_FALSE(miss.hit);
}
