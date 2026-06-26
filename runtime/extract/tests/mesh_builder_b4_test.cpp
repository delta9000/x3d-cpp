// mesh_builder_b4_test.cpp — Browser-level B4 acceptance: MeshData Topology enum
// + IndexedLineSet / LineSet / PointSet flow through the seam as Lines/Points,
// always-unlit (hasNormals=false, solid=false), with vertex Color honored.
//
// Proofs:
//   1) Default topology is Triangles and a triangle mesh is unchanged (the
//      enum default keeps every pre-B4 path byte-identical).
//   2) IndexedLineSet: each -1-delimited coordIndex run becomes consecutive
//      vertex PAIRS (a 3-vertex polyline -> 2 segments -> 4 emitted indices),
//      topology=Lines, hasNormals=false, solid=false. recognizedGeometryType
//      ("IndexedLineSet") is true.
//   3) IndexedLineSet honors per-vertex Color (colorPerVertex default true) so
//      colors are parallel to positions and the segment endpoints carry the
//      authored colors; ColorRGBA alpha is preserved.
//   4) LineSet: vertexCount partitions the implicit coord run into polylines;
//      a {3} count -> one polyline -> 2 segments.
//   5) PointSet: the whole point array becomes a 0..N-1 GL_POINTS run,
//      topology=Points, hasNormals=false, solid=false; Color honored.
//   6) Empty/degenerate guards: a 1-vertex polyline / empty coordIndex / empty
//      point array yields an empty mesh with NO OOB read, and the type stays
//      recognized (legitimately empty, not an unsupported drop).
#include "MeshBuilder.hpp"

#include "X3DNodeFactory.hpp"

#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::runtime::extract;

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-5f; }

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
static void attachCoord(const std::shared_ptr<X3DNode> &g,
                        std::vector<SFVec3f> pts) {
  auto c = createX3DNode("Coordinate");
  setF(c, "point", std::any(std::move(pts)));
  setF(g, "coord", std::any(std::static_pointer_cast<X3DNode>(c)));
}
static void attachColor(const std::shared_ptr<X3DNode> &g,
                        std::vector<SFColor> c) {
  auto n = createX3DNode("Color");
  setF(n, "color", std::any(std::move(c)));
  setF(g, "color", std::any(std::static_pointer_cast<X3DNode>(n)));
}
static void attachColorRGBA(const std::shared_ptr<X3DNode> &g,
                            std::vector<SFColorRGBA> c) {
  auto n = createX3DNode("ColorRGBA");
  setF(n, "color", std::any(std::move(c)));
  setF(g, "color", std::any(std::static_pointer_cast<X3DNode>(n)));
}
static bool posEq(const MeshData &m, std::size_t i, const SFVec3f &p) {
  return feq(m.positions[i].x, p.x) && feq(m.positions[i].y, p.y) &&
         feq(m.positions[i].z, p.z);
}
static bool rgbaEq(const SFColorRGBA &a, float r, float g, float b, float al) {
  return feq(a.r, r) && feq(a.g, g) && feq(a.b, b) && feq(a.a, al);
}

TEST_CASE("mesh_builder_b4_test") {
  const SFVec3f P0{0, 0, 0}, P1{1, 0, 0}, P2{2, 0, 0}, P3{3, 0, 0};

  // recognizedGeometryType now lists all three line/point sets.
  CHECK((recognizedGeometryType("IndexedLineSet")));
  CHECK((recognizedGeometryType("LineSet")));
  CHECK((recognizedGeometryType("PointSet")));

  // ---- 1. Default topology is Triangles; triangle path unchanged ------------
  {
    auto g = createX3DNode("TriangleSet");
    attachCoord(g, {P0, P1, SFVec3f{1, 1, 0}});
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.topology == Topology::Triangles));
    CHECK((m.solid == true));     // composed-geometry default.
    CHECK((m.hasNormals == true)); // flat normals generated.
    CHECK((m.indices.size() == 3));
  }

  // ---- 2. IndexedLineSet: one 3-vertex polyline -> 2 segments ---------------
  {
    auto g = createX3DNode("IndexedLineSet");
    attachCoord(g, {P0, P1, P2});
    // coordIndex 0,1,2,-1 => polyline (P0,P1,P2) => segments P0-P1, P1-P2.
    setF(g, "coordIndex", std::any(std::vector<int>{0, 1, 2, -1}));
    MeshData m = buildLocalMesh(g.get());

    CHECK((m.topology == Topology::Lines));
    CHECK((m.hasNormals == false));
    CHECK((m.normals.empty()));
    CHECK((m.solid == false)); // line meshes are non-solid (cull-disabled / unlit).
    // 2 segments * 2 endpoints = 4 expanded vertices, indices 0..3.
    CHECK((m.positions.size() == 4));
    CHECK((m.indices.size() == 4));
    for (std::size_t i = 0; i < m.indices.size(); ++i)
      CHECK((m.indices[i] == static_cast<std::uint32_t>(i)));
    // Segment 1: P0,P1.  Segment 2: P1,P2.
    CHECK((posEq(m, 0, P0) && posEq(m, 1, P1)));
    CHECK((posEq(m, 2, P1) && posEq(m, 3, P2)));
  }

  // ---- 3a. IndexedLineSet per-vertex Color (RGB promoted alpha=1) -----------
  {
    auto g = createX3DNode("IndexedLineSet");
    attachCoord(g, {P0, P1});
    attachColor(g, {SFColor{1, 0, 0}, SFColor{0, 1, 0}});
    setF(g, "coordIndex", std::any(std::vector<int>{0, 1, -1}));
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.topology == Topology::Lines));
    CHECK((m.hasColors == true));
    CHECK((m.colors.size() == 2));
    CHECK((rgbaEq(m.colors[0], 1, 0, 0, 1))); // P0 red
    CHECK((rgbaEq(m.colors[1], 0, 1, 0, 1))); // P1 green
  }

  // ---- 3b. IndexedLineSet ColorRGBA alpha preserved -------------------------
  {
    auto g = createX3DNode("IndexedLineSet");
    attachCoord(g, {P0, P1});
    attachColorRGBA(g, {SFColorRGBA{0.2f, 0.4f, 0.6f, 0.5f},
                        SFColorRGBA{0.1f, 0.2f, 0.3f, 0.25f}});
    setF(g, "coordIndex", std::any(std::vector<int>{0, 1, -1}));
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.hasColors == true));
    CHECK((rgbaEq(m.colors[0], 0.2f, 0.4f, 0.6f, 0.5f)));
    CHECK((rgbaEq(m.colors[1], 0.1f, 0.2f, 0.3f, 0.25f)));
  }

  // ---- 4. LineSet: vertexCount {3} partitions into one 3-vertex polyline ----
  {
    auto g = createX3DNode("LineSet");
    attachCoord(g, {P0, P1, P2});
    setF(g, "vertexCount", std::any(std::vector<int>{3}));
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.topology == Topology::Lines));
    CHECK((m.hasNormals == false));
    CHECK((m.solid == false));
    CHECK((m.positions.size() == 4)); // 2 segments * 2 endpoints.
    CHECK((posEq(m, 0, P0) && posEq(m, 1, P1)));
    CHECK((posEq(m, 2, P1) && posEq(m, 3, P2)));
  }

  // ---- 4b. LineSet two polylines {2,2} --------------------------------------
  {
    auto g = createX3DNode("LineSet");
    attachCoord(g, {P0, P1, P2, P3});
    setF(g, "vertexCount", std::any(std::vector<int>{2, 2}));
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.topology == Topology::Lines));
    CHECK((m.positions.size() == 4)); // 2 segments, 1 per polyline.
    CHECK((posEq(m, 0, P0) && posEq(m, 1, P1)));
    CHECK((posEq(m, 2, P2) && posEq(m, 3, P3)));
  }

  // ---- 5. PointSet: whole array -> 0..N-1 GL_POINTS, Color honored ----------
  {
    auto g = createX3DNode("PointSet");
    attachCoord(g, {P0, P1, P2});
    attachColor(g, {SFColor{1, 0, 0}, SFColor{0, 1, 0}, SFColor{0, 0, 1}});
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.topology == Topology::Points));
    CHECK((m.hasNormals == false));
    CHECK((m.solid == false));
    CHECK((m.positions.size() == 3));
    CHECK((m.indices.size() == 3));
    for (std::size_t i = 0; i < 3; ++i)
      CHECK((m.indices[i] == static_cast<std::uint32_t>(i)));
    CHECK((posEq(m, 0, P0) && posEq(m, 1, P1) && posEq(m, 2, P2)));
    CHECK((m.hasColors == true));
    CHECK((rgbaEq(m.colors[2], 0, 0, 1, 1)));
  }

  // ---- 6. Degenerate guards: no OOB, empty mesh, still recognized -----------
  {
    // 6a: 1-vertex "polyline" (no segment) + an out-of-range index.
    auto g = createX3DNode("IndexedLineSet");
    attachCoord(g, {P0, P1});
    setF(g, "coordIndex", std::any(std::vector<int>{0, -1, 5, 6, -1}));
    bool recognized = false;
    MeshData m = buildLocalMesh(g.get(), {}, &recognized);
    CHECK((recognized));           // recognized type...
    CHECK((m.indices.empty()));    // ...but legitimately empty (no valid segment).
    CHECK((m.topology == Topology::Lines));
  }
  {
    // 6b: empty PointSet (no coord) -> empty mesh, recognized.
    auto g = createX3DNode("PointSet");
    bool recognized = false;
    MeshData m = buildLocalMesh(g.get(), {}, &recognized);
    CHECK((recognized));
    CHECK((m.positions.empty() && m.indices.empty()));
  }

  return;
}
