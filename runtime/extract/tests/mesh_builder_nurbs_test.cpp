// mesh_builder_nurbs_test.cpp — NurbsCurve/NurbsPatchSurface -> MeshData.
#include "MeshBuilder.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include <any>
#include "doctest/doctest.h"
#include <memory>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime::extract;

static void setF(const std::shared_ptr<X3DNode>& n, const char* nm, std::any v) {
  for (auto& f : n->fields()) if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}

TEST_CASE("nurbs_curve_arm_emits_lines") {
  auto coord = createX3DNode("Coordinate");
  setF(coord, "point", std::any(std::vector<SFVec3f>{{0,0,0},{1,0,0},{1,1,0},{2,1,0}}));
  auto curve = createX3DNode("NurbsCurve");
  setF(curve, "controlPoint", std::any(std::shared_ptr<X3DNode>(coord)));
  setF(curve, "order", std::any(SFInt32{3}));
  setF(curve, "tessellation", std::any(SFInt32{8})); // 8 segments => 9 sample points
  bool rec = false;
  auto mesh = buildLocalMesh(curve.get(), MeshBuildOptions{}, &rec);
  CHECK(rec);
  CHECK(mesh.topology == Topology::Lines);
  CHECK(mesh.solid == false);
  // 9 samples => 8 segments => 16 expanded line-pair positions
  CHECK(mesh.positions.size() == 16);
  CHECK(mesh.indices.size() == 16);
}

TEST_CASE("nurbs_curve_recognized_oracle") {
  CHECK(recognizedGeometryType("NurbsCurve"));
}

TEST_CASE("nurbs_curve_degenerate_recognized_but_empty") {
  auto coord = createX3DNode("Coordinate");
  setF(coord, "point", std::any(std::vector<SFVec3f>{{0,0,0}})); // 1 pt < order
  auto curve = createX3DNode("NurbsCurve");
  setF(curve, "controlPoint", std::any(std::shared_ptr<X3DNode>(coord)));
  setF(curve, "order", std::any(SFInt32{3}));
  bool rec = false;
  auto mesh = buildLocalMesh(curve.get(), MeshBuildOptions{}, &rec);
  CHECK(rec);                      // recognized type...
  CHECK(mesh.positions.empty());   // ...but legitimately empty
}
