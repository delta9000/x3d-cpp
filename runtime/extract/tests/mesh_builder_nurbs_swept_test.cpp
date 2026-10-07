// mesh_builder_nurbs_swept_test.cpp — NurbsSweptSurface / NurbsSwungSurface.
#include "GeoFrame.hpp"
#include "MeshBuilder.hpp"
#include "GeometryBounds.hpp"
#include "x3d/nodes/X3DNode.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;
using namespace x3d::runtime::extract;

static void setF(const std::shared_ptr<X3DNode>& n, const char* nm, std::any v) {
  for (auto& f : n->fields()) if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}

// A degree-1 (order 2) closed polyline through `n` points on a radius-r circle,
// with tessellation=n so the tessellator samples exactly on the vertices.
static std::shared_ptr<X3DNode> circleCurve2D(float r, int n) {
  const double twoPi = 6.283185307179586;
  std::vector<SFVec2d> pts;
  for (int k = 0; k <= n; ++k) {
    const double a = twoPi * k / n;
    pts.push_back(SFVec2d{r * std::cos(a), r * std::sin(a)});
  }
  auto c = createX3DNode("NurbsCurve2D");
  setF(c, "controlPoint", std::any(pts));
  setF(c, "order", std::any(SFInt32{2}));
  setF(c, "tessellation", std::any(SFInt32{n}));
  return c;
}

TEST_CASE("nurbs_swept_circle_along_line_is_cylinder") {
  // Cross-section: a radius-2 circle; trajectory: a straight segment along +Z.
  auto cross = circleCurve2D(2.0f, 16);
  auto coord = createX3DNode("Coordinate");
  setF(coord, "point", std::any(std::vector<SFVec3f>{{0, 0, 0}, {0, 0, 2.5f}, {0, 0, 5}}));
  auto traj = createX3DNode("NurbsCurve");
  setF(traj, "controlPoint", std::any(std::shared_ptr<X3DNode>(coord)));
  setF(traj, "order", std::any(SFInt32{3}));
  setF(traj, "tessellation", std::any(SFInt32{4}));
  auto swept = createX3DNode("NurbsSweptSurface");
  setF(swept, "crossSectionCurve", std::any(std::shared_ptr<X3DNode>(cross)));
  setF(swept, "trajectoryCurve", std::any(std::shared_ptr<X3DNode>(traj)));

  bool rec = false;
  auto mesh = buildLocalMesh(swept.get(), geo::builtinProjection(), MeshBuildOptions{}, &rec);
  CHECK(rec);
  CHECK(mesh.topology == Topology::Triangles);
  CHECK(mesh.hasNormals);
  CHECK(mesh.solid == true);
  CHECK(!mesh.positions.empty());
  for (const auto& p : mesh.positions) {
    CHECK(std::fabs(std::sqrt(p.x * p.x + p.y * p.y) - 2.0f) < 1e-3f); // radius 2
    CHECK(p.z >= -1e-4f);
    CHECK(p.z <= 5.0f + 1e-4f);
  }
}

TEST_CASE("nurbs_swung_line_profile_around_circle_revolves") {
  // Profile: a straight line radius 1 -> 2 over height 0 -> 3 (a frustum).
  auto profile = createX3DNode("NurbsCurve2D");
  setF(profile, "controlPoint", std::any(std::vector<SFVec2d>{{1, 0}, {2, 3}}));
  setF(profile, "order", std::any(SFInt32{2}));
  setF(profile, "tessellation", std::any(SFInt32{1}));
  // Trajectory: a unit circle supplying the radial direction.
  auto traj = circleCurve2D(1.0f, 16);
  auto swung = createX3DNode("NurbsSwungSurface");
  setF(swung, "profileCurve", std::any(std::shared_ptr<X3DNode>(profile)));
  setF(swung, "trajectoryCurve", std::any(std::shared_ptr<X3DNode>(traj)));

  bool rec = false;
  auto mesh = buildLocalMesh(swung.get(), geo::builtinProjection(), MeshBuildOptions{}, &rec);
  CHECK(rec);
  CHECK(mesh.topology == Topology::Triangles);
  CHECK(mesh.hasNormals);
  CHECK(!mesh.positions.empty());
  for (const auto& p : mesh.positions) {
    // Radius of the revolved frustum is linear in z: r(z) = 1 + z/3.
    CHECK(std::fabs(std::sqrt(p.x * p.x + p.y * p.y) - (1.0f + p.z / 3.0f)) < 1e-3f);
    CHECK(p.z >= -1e-4f);
    CHECK(p.z <= 3.0f + 1e-4f);
  }
}

TEST_CASE("nurbs_swept_swung_recognized_oracle") {
  CHECK(recognizedGeometryType("NurbsSweptSurface"));
  CHECK(recognizedGeometryType("NurbsSwungSurface"));
  CHECK(recognizedGeometryType("NurbsCurve"));       // regression: still true.
  CHECK(recognizedGeometryType("NurbsPatchSurface")); // regression: still true.
}

TEST_CASE("nurbs_swept_swung_missing_curves_recognized_but_empty") {
  auto swept = createX3DNode("NurbsSweptSurface");
  bool rec = false;
  auto m1 = buildLocalMesh(swept.get(), geo::builtinProjection(), MeshBuildOptions{}, &rec);
  CHECK(rec);
  CHECK(m1.positions.empty());
  auto swung = createX3DNode("NurbsSwungSurface");
  auto m2 = buildLocalMesh(swung.get(), geo::builtinProjection(), MeshBuildOptions{}, &rec);
  CHECK(rec);
  CHECK(m2.positions.empty());
}

TEST_CASE("nurbs_swept_swung_bounds") {
  // Swept: cylinder radius 2 along +Z 0..5. The bound inflates the trajectory
  // hull by the cross-section radius on every axis (Extrusion's conservative
  // rule), so z over-bounds by +-2.
  auto cross = circleCurve2D(2.0f, 16);
  auto coord = createX3DNode("Coordinate");
  setF(coord, "point", std::any(std::vector<SFVec3f>{{0, 0, 0}, {0, 0, 2.5f}, {0, 0, 5}}));
  auto traj = createX3DNode("NurbsCurve");
  setF(traj, "controlPoint", std::any(std::shared_ptr<X3DNode>(coord)));
  setF(traj, "order", std::any(SFInt32{3}));
  auto swept = createX3DNode("NurbsSweptSurface");
  setF(swept, "crossSectionCurve", std::any(std::shared_ptr<X3DNode>(cross)));
  setF(swept, "trajectoryCurve", std::any(std::shared_ptr<X3DNode>(traj)));
  const Aabb sb = localGeometryBounds(swept.get(), geo::builtinProjection());
  CHECK(!sb.empty);
  CHECK(std::fabs(sb.min.x + 2.0f) < 1e-4f);
  CHECK(std::fabs(sb.max.x - 2.0f) < 1e-4f);
  CHECK(std::fabs(sb.min.z + 2.0f) < 1e-4f);
  CHECK(std::fabs(sb.max.z - 7.0f) < 1e-4f);

  // Swung: the frustum r(z)=1+z/3, z 0..3 -> x/y in [-2,2], z in [0,3].
  auto profile = createX3DNode("NurbsCurve2D");
  setF(profile, "controlPoint", std::any(std::vector<SFVec2d>{{1, 0}, {2, 3}}));
  setF(profile, "order", std::any(SFInt32{2}));
  auto traj2 = circleCurve2D(1.0f, 16);
  auto swung = createX3DNode("NurbsSwungSurface");
  setF(swung, "profileCurve", std::any(std::shared_ptr<X3DNode>(profile)));
  setF(swung, "trajectoryCurve", std::any(std::shared_ptr<X3DNode>(traj2)));
  const Aabb wb = localGeometryBounds(swung.get(), geo::builtinProjection());
  CHECK(!wb.empty);
  CHECK(std::fabs(wb.min.x + 2.0f) < 1e-4f);
  CHECK(std::fabs(wb.max.x - 2.0f) < 1e-4f);
  CHECK(std::fabs(wb.min.z - 0.0f) < 1e-4f);
  CHECK(std::fabs(wb.max.z - 3.0f) < 1e-4f);
}
