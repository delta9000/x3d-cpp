// mesh_builder_geom2d_test.cpp — G2D-1 / G2D-2 / SEAM-2D-NURBS acceptance.
//
// The eight Geometry2D (§14) nodes: extraction into the XY plane (z=0) and
// exact local bounds. Per node: topology, vertex-count sanity, bounds, and a
// degenerate case.
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

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) {
      f.set(*n, std::move(v));
      return;
    }
}

static std::vector<SFVec2f> v2(std::initializer_list<SFVec2f> l) {
  return std::vector<SFVec2f>(l);
}

// --- local-bounds + mesh checks are separate so failures point at one node ---

TEST_CASE("geom2d_recognized") {
  for (const char *t :
       {"Arc2D", "ArcClose2D", "Circle2D", "Disk2D", "Polyline2D",
        "Polypoint2D", "Rectangle2D", "TriangleSet2D"})
    CHECK(recognizedGeometryType(t));
}

// ---------------------------------------------------------------------------
TEST_CASE("geom2d_arc2d") {
  auto a = createX3DNode("Arc2D");
  setF(a, "radius", std::any(2.0f)); // startAngle 0, endAngle pi/2
  bool rec = false;
  MeshData m = buildLocalMesh(a.get(), geo::builtinProjection(), MeshBuildOptions{}, &rec);
  CHECK(rec);
  CHECK(m.topology == Topology::Lines);
  CHECK(m.solid == false);
  CHECK(m.positions.size() == 2 * 16); // ceil((pi/2)/step) = 16 chords.
  CHECK(m.positions.size() % 2 == 0);
  for (const auto &p : m.positions) {
    CHECK(feq(p.z, 0.0f));
    CHECK(feq(std::sqrt(p.x * p.x + p.y * p.y), 2.0f)); // on the radius.
  }
  CHECK(m.texcoords.empty()); // lines carry no texcoords.

  Aabb b = localGeometryBounds(a.get(), geo::builtinProjection());
  CHECK(!b.empty);
  CHECK((feq(b.min.x, 0) && feq(b.min.y, 0) && feq(b.max.x, 2) && feq(b.max.y, 2)));

  // Degenerate: startAngle == endAngle -> a single point -> empty.
  auto d = createX3DNode("Arc2D");
  setF(d, "startAngle", std::any(1.0f));
  setF(d, "endAngle", std::any(1.0f));
  MeshData dm = buildLocalMesh(d.get(), geo::builtinProjection());
  CHECK(dm.positions.empty());
  CHECK(localGeometryBounds(d.get(), geo::builtinProjection()).empty);
}

// ---------------------------------------------------------------------------
TEST_CASE("geom2d_circle2d") {
  auto c = createX3DNode("Circle2D");
  setF(c, "radius", std::any(3.0f));
  MeshData m = buildLocalMesh(c.get(), geo::builtinProjection());
  CHECK(m.topology == Topology::Lines);
  CHECK(m.solid == false);
  CHECK(m.positions.size() == 2 * 64); // full circle -> 64 chords.

  Aabb b = localGeometryBounds(c.get(), geo::builtinProjection());
  CHECK((feq(b.min.x, -3) && feq(b.max.x, 3) && feq(b.min.y, -3) && feq(b.max.y, 3)));

  auto z = createX3DNode("Circle2D"); // radius 0 -> empty.
  setF(z, "radius", std::any(0.0f));
  CHECK(buildLocalMesh(z.get(), geo::builtinProjection()).positions.empty());
  CHECK(localGeometryBounds(z.get(), geo::builtinProjection()).empty);
}

// ---------------------------------------------------------------------------
TEST_CASE("geom2d_polyline2d") {
  auto p = createX3DNode("Polyline2D");
  setF(p, "lineSegments", std::any(v2({{0, 0}, {1, 0}, {1, 1}})));
  MeshData m = buildLocalMesh(p.get(), geo::builtinProjection());
  CHECK(m.topology == Topology::Lines);
  CHECK(m.solid == false);
  CHECK(m.positions.size() == 4); // 2 segments as endpoint pairs.
  CHECK(m.indices.size() == 4);

  Aabb b = localGeometryBounds(p.get(), geo::builtinProjection());
  CHECK((feq(b.min.x, 0) && feq(b.max.x, 1) && feq(b.max.y, 1)));

  auto one = createX3DNode("Polyline2D"); // single point -> no segment.
  setF(one, "lineSegments", std::any(v2({{2, 2}})));
  CHECK(buildLocalMesh(one.get(), geo::builtinProjection()).positions.empty());
  CHECK(localGeometryBounds(one.get(), geo::builtinProjection()).empty == false); // a point is a valid bound.
}

// ---------------------------------------------------------------------------
TEST_CASE("geom2d_polypoint2d") {
  auto p = createX3DNode("Polypoint2D");
  setF(p, "point", std::any(v2({{0, 0}, {2, 1}})));
  MeshData m = buildLocalMesh(p.get(), geo::builtinProjection());
  CHECK(m.topology == Topology::Points);
  CHECK(m.solid == false);
  CHECK(m.positions.size() == 2);
  CHECK(m.indices.size() == 2);

  Aabb b = localGeometryBounds(p.get(), geo::builtinProjection());
  CHECK((feq(b.min.x, 0) && feq(b.max.x, 2) && feq(b.max.y, 1)));

  auto e = createX3DNode("Polypoint2D"); // empty point array -> empty.
  CHECK(buildLocalMesh(e.get(), geo::builtinProjection()).positions.empty());
  CHECK(localGeometryBounds(e.get(), geo::builtinProjection()).empty);
}

// ---------------------------------------------------------------------------
TEST_CASE("geom2d_rectangle2d") {
  auto r = createX3DNode("Rectangle2D");
  setF(r, "size", std::any(SFVec2f{2.0f, 4.0f}));
  setF(r, "solid", std::any(true));
  MeshData m = buildLocalMesh(r.get(), geo::builtinProjection());
  CHECK(m.topology == Topology::Triangles);
  CHECK(m.solid == true); // carried from the field.
  CHECK(m.positions.size() == 6); // 2 triangles.
  CHECK(m.indices.size() == 6);
  CHECK(m.hasNormals);
  for (const auto &n : m.normals)
    CHECK((feq(n.x, 0) && feq(n.y, 0) && feq(n.z, 1)));
  CHECK(m.hasNormals);
  CHECK(m.texcoords.size() == 6); // bounding square -> [0,1].

  Aabb b = localGeometryBounds(r.get(), geo::builtinProjection());
  CHECK((feq(b.min.x, -1) && feq(b.max.x, 1) && feq(b.min.y, -2) && feq(b.max.y, 2)));

  // Degenerate: zero-size rectangle still yields 2 coplanar (zero-area) tris.
  auto z = createX3DNode("Rectangle2D");
  setF(z, "size", std::any(SFVec2f{0.0f, 0.0f}));
  MeshData zm = buildLocalMesh(z.get(), geo::builtinProjection());
  CHECK(zm.positions.size() == 6);
}

// ---------------------------------------------------------------------------
TEST_CASE("geom2d_triangleset2d") {
  auto t = createX3DNode("TriangleSet2D");
  setF(t, "vertices", std::any(v2({{0, 0}, {1, 0}, {0, 1}, {5, 5}, {6, 5}, {5, 6}})));
  MeshData m = buildLocalMesh(t.get(), geo::builtinProjection());
  CHECK(m.topology == Topology::Triangles);
  CHECK(m.positions.size() == 6); // 2 triangles.
  CHECK(m.hasNormals);
  CHECK(m.texcoords.size() == 6); // default from the bbox.
  for (const auto &n : m.normals)
    CHECK((feq(n.z, 1)));

  Aabb b = localGeometryBounds(t.get(), geo::builtinProjection());
  CHECK((feq(b.min.x, 0) && feq(b.max.x, 6) && feq(b.min.y, 0) && feq(b.max.y, 6)));

  // Degenerate: fewer than 3 vertices -> no triangle.
  auto d = createX3DNode("TriangleSet2D");
  setF(d, "vertices", std::any(v2({{0, 0}, {1, 1}})));
  CHECK(buildLocalMesh(d.get(), geo::builtinProjection()).positions.empty());
}

// ---------------------------------------------------------------------------
TEST_CASE("geom2d_disk2d") {
  // Solid fan (innerRadius 0).
  auto d = createX3DNode("Disk2D");
  setF(d, "outerRadius", std::any(2.0f));
  MeshData m = buildLocalMesh(d.get(), geo::builtinProjection());
  CHECK(m.topology == Topology::Triangles);
  CHECK(m.solid == false); // X3DGeometry2D solid default.
  CHECK(m.positions.size() == 64 * 3); // 64 fan triangles.
  CHECK(m.texcoords.size() == m.positions.size());
  for (const auto &n : m.normals)
    CHECK((feq(n.z, 1)));

  Aabb b = localGeometryBounds(d.get(), geo::builtinProjection());
  CHECK((feq(b.min.x, -2) && feq(b.max.x, 2) && feq(b.min.y, -2) && feq(b.max.y, 2)));

  // Annulus (innerRadius > 0).
  auto a = createX3DNode("Disk2D");
  setF(a, "outerRadius", std::any(2.0f));
  setF(a, "innerRadius", std::any(1.0f));
  MeshData am = buildLocalMesh(a.get(), geo::builtinProjection());
  CHECK(am.positions.size() == 64 * 2 * 3); // 128 quad triangles.
  Aabb ab = localGeometryBounds(a.get(), geo::builtinProjection());
  CHECK((feq(ab.min.x, -2) && feq(ab.max.x, 2)));

  // innerRadius == outerRadius -> a circle LINE.
  auto c = createX3DNode("Disk2D");
  setF(c, "outerRadius", std::any(2.0f));
  setF(c, "innerRadius", std::any(2.0f));
  MeshData cm = buildLocalMesh(c.get(), geo::builtinProjection());
  CHECK(cm.topology == Topology::Lines);
  CHECK(cm.positions.size() == 2 * 64);

  // Degenerate: outerRadius 0 -> empty.
  auto z = createX3DNode("Disk2D");
  setF(z, "outerRadius", std::any(0.0f));
  CHECK(buildLocalMesh(z.get(), geo::builtinProjection()).positions.empty());
  CHECK(localGeometryBounds(z.get(), geo::builtinProjection()).empty);
}

// ---------------------------------------------------------------------------
TEST_CASE("geom2d_arcclose2d") {
  // PIE (default): fan from the centre over the arc.
  auto p = createX3DNode("ArcClose2D"); // radius 1, 0..pi/2.
  MeshData pm = buildLocalMesh(p.get(), geo::builtinProjection());
  CHECK(pm.topology == Topology::Triangles);
  CHECK(pm.positions.size() == 16 * 3); // 16 pie triangles.
  CHECK(pm.hasNormals);
  Aabb pb = localGeometryBounds(p.get(), geo::builtinProjection());
  CHECK((feq(pb.min.x, 0) && feq(pb.min.y, 0) && feq(pb.max.x, 1) && feq(pb.max.y, 1)));

  // CHORD: fan from the first arc endpoint (one fewer triangle).
  auto c = createX3DNode("ArcClose2D");
  setF(c, "closureType", std::any(ClosureTypeChoices::CHORD));
  MeshData cm = buildLocalMesh(c.get(), geo::builtinProjection());
  CHECK(cm.topology == Topology::Triangles);
  CHECK(cm.positions.size() == 15 * 3);
  Aabb cb = localGeometryBounds(c.get(), geo::builtinProjection());
  CHECK((feq(cb.min.x, 0) && feq(cb.max.x, 1) && feq(cb.max.y, 1)));

  // Degenerate: startAngle == endAngle -> empty.
  auto d = createX3DNode("ArcClose2D");
  setF(d, "startAngle", std::any(0.5f));
  setF(d, "endAngle", std::any(0.5f));
  CHECK(buildLocalMesh(d.get(), geo::builtinProjection()).positions.empty());
  CHECK(localGeometryBounds(d.get(), geo::builtinProjection()).empty);
}
