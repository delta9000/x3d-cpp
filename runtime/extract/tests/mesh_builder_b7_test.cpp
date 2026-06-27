// mesh_builder_b7_test.cpp — spec-conformance fixes for GENERATED normals and
// non-convex face triangulation in the composed-geometry funnel.
//
// MSH-1: the `ccw` field must reverse the direction of a GENERATED face normal
//   for IFS / triangle-set family / ElevationGrid, exactly as the Extrusion path
//   already does (rendering.md §11.4.x: "the normal shall be generated for the
//   triangle based on the ccw field"; geometry3D.md §13.3.4: ccw=FALSE reverses
//   the normal direction). Author-supplied Normal-node normals are NEVER flipped.
//
// MSH-2: the triangle/quad-set family (IndexedTriangleSet, TriangleSet,
//   IndexedTriangleStripSet, IndexedTriangleFanSet, QuadSet, IndexedQuadSet) has
//   NO creaseAngle field and normalPerVertex defaults TRUE, so §11.3.2/§11.4.9
//   mandate the generated per-vertex normal be the AVERAGE of all triangles
//   sharing that vertex — NOT the flat per-face normal. IFS and ElevationGrid
//   (which carry creaseAngle, default 0 = faceted) are EXEMPT and unchanged.
//
// MSH-3: the `convex` field (SFBool, default TRUE) must be honored for IFS. When
//   convex==FALSE a concave polygon must be triangulated with ear-clipping, not a
//   naive fan (which produces inverted/overlapping triangles). The total triangle
//   area must equal the polygon area and all triangles share one winding.
#include "MeshBuilder.hpp"

#include "X3DNodeFactory.hpp"

#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::runtime::extract;

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }
static bool vecEq(const SFVec3f &a, const SFVec3f &b) {
  return feq(a.x, b.x) && feq(a.y, b.y) && feq(a.z, b.z);
}
static SFVec3f sub(const SFVec3f &a, const SFVec3f &b) {
  return SFVec3f{a.x - b.x, a.y - b.y, a.z - b.z};
}
static SFVec3f cross(const SFVec3f &a, const SFVec3f &b) {
  return SFVec3f{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                 a.x * b.y - a.y * b.x};
}
static SFVec3f normz(const SFVec3f &v) {
  float l = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
  return l > 0 ? SFVec3f{v.x / l, v.y / l, v.z / l} : v;
}

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) {
      f.set(*n, std::move(v));
      return;
    }
}
static std::shared_ptr<X3DNode> makeCoord(std::vector<SFVec3f> pts) {
  auto c = createX3DNode("Coordinate");
  setF(c, "point", std::any(std::move(pts)));
  return c;
}

TEST_CASE("mesh_builder_b7_test") {
  // =====================================================================
  // MSH-1: ccw reverses the GENERATED normal for an IFS.
  // A single CCW-as-authored triangle in the z=0 plane has geometric face
  // normal +Z. With ccw=TRUE the generated normal must be +Z; with ccw=FALSE
  // it must be the EXACT opposite (-Z). The two runs differ only in `ccw`.
  // =====================================================================
  {
    const std::vector<SFVec3f> tri = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
    const std::vector<int> idx = {0, 1, 2, -1};

    auto gT = createX3DNode("IndexedFaceSet");
    setF(gT, "coord", std::any(std::static_pointer_cast<X3DNode>(makeCoord(tri))));
    setF(gT, "coordIndex", std::any(idx));
    setF(gT, "ccw", std::any(true));
    MeshData mT = buildLocalMesh(gT.get());

    auto gF = createX3DNode("IndexedFaceSet");
    setF(gF, "coord", std::any(std::static_pointer_cast<X3DNode>(makeCoord(tri))));
    setF(gF, "coordIndex", std::any(idx));
    setF(gF, "ccw", std::any(false));
    MeshData mF = buildLocalMesh(gF.get());

    CHECK((mT.hasNormals && mF.hasNormals));
    CHECK((mT.normals.size() == mF.normals.size() && !mT.normals.empty()));
    // ccw=TRUE => +Z (right-hand rule on CCW winding).
    CHECK((vecEq(mT.normals[0], SFVec3f{0, 0, 1})));
    // ccw=FALSE => generated normal reversed => -Z, opposite ccw=TRUE.
    for (std::size_t i = 0; i < mT.normals.size(); ++i)
      CHECK((vecEq(mF.normals[i], SFVec3f{-mT.normals[i].x, -mT.normals[i].y,
                                          -mT.normals[i].z})));
  }

  // MSH-1b: an AUTHORED Normal node is NEVER flipped by ccw=FALSE.
  {
    const std::vector<SFVec3f> tri = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
    const std::vector<int> idx = {0, 1, 2, -1};
    auto g = createX3DNode("IndexedFaceSet");
    setF(g, "coord", std::any(std::static_pointer_cast<X3DNode>(makeCoord(tri))));
    setF(g, "coordIndex", std::any(idx));
    setF(g, "ccw", std::any(false));
    auto nrm = createX3DNode("Normal");
    setF(nrm, "vector", std::any(std::vector<SFVec3f>{{0, 0, 1}, {0, 0, 1}, {0, 0, 1}}));
    setF(g, "normal", std::any(std::static_pointer_cast<X3DNode>(nrm)));
    setF(g, "normalPerVertex", std::any(true));
    MeshData m = buildLocalMesh(g.get());
    // Authored +Z stays +Z despite ccw=FALSE.
    for (const auto &n : m.normals) CHECK((vecEq(n, SFVec3f{0, 0, 1})));
  }

  // =====================================================================
  // MSH-2: triangle-set family gets VERTEX-AVERAGED normals (smooth), not
  // flat per-face. Two triangles sharing an edge with a real dihedral angle:
  // a "tent" / open book about the X axis. normalPerVertex defaults TRUE and
  // there is no creaseAngle field, so the shared-edge corners must carry the
  // average of the two face normals (NOT either flat face normal).
  // =====================================================================
  {
    // Shared edge along X at y=0,z=0 (verts 0,1). Two flaps tilt up in +/-Z.
    //  Tri A: 0,1,2  (flap toward +Z+Y)  Tri B: 1,0,3 (flap toward -Z+Y)
    const std::vector<SFVec3f> pts = {
        {-1, 0, 0}, {1, 0, 0},  // shared edge
        {0, 1, 1},              // A apex
        {0, 1, -1},             // B apex
    };
    const std::vector<int> idx = {0, 1, 2, 1, 0, 3};
    auto g = createX3DNode("IndexedTriangleSet");
    setF(g, "coord", std::any(std::static_pointer_cast<X3DNode>(makeCoord(pts))));
    setF(g, "index", std::any(idx));
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.hasNormals && m.normals.size() == m.positions.size()));

    // Flat face normals of each triangle (as authored / CCW).
    const SFVec3f fnA =
        normz(cross(sub(pts[1], pts[0]), sub(pts[2], pts[0])));
    const SFVec3f fnB =
        normz(cross(sub(pts[0], pts[1]), sub(pts[3], pts[1])));
    const SFVec3f avg = normz(SFVec3f{fnA.x + fnB.x, fnA.y + fnB.y, fnA.z + fnB.z});
    CHECK((!vecEq(fnA, fnB))); // genuine dihedral, faces differ.

    // Every corner sitting on the SHARED edge (coord 0 or 1) must carry the
    // averaged normal, NOT the flat face normal of its own triangle.
    int sharedChecked = 0;
    for (std::size_t i = 0; i < m.positions.size(); ++i) {
      if (vecEq(m.positions[i], pts[0]) || vecEq(m.positions[i], pts[1])) {
        CHECK((vecEq(m.normals[i], avg)));
        CHECK((!vecEq(m.normals[i], fnA) && !vecEq(m.normals[i], fnB)));
        ++sharedChecked;
      }
    }
    CHECK((sharedChecked == 4)); // both shared verts appear once per triangle.
  }

  // MSH-2b: an authored Normal still wins for triangle sets (no averaging).
  {
    const std::vector<SFVec3f> pts = {
        {-1, 0, 0}, {1, 0, 0}, {0, 1, 1}, {0, 1, -1}};
    const std::vector<int> idx = {0, 1, 2, 1, 0, 3};
    auto g = createX3DNode("IndexedTriangleSet");
    setF(g, "coord", std::any(std::static_pointer_cast<X3DNode>(makeCoord(pts))));
    setF(g, "index", std::any(idx));
    auto nrm = createX3DNode("Normal");
    setF(nrm, "vector", std::any(std::vector<SFVec3f>{
                            {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}}));
    setF(g, "normal", std::any(std::static_pointer_cast<X3DNode>(nrm)));
    MeshData m = buildLocalMesh(g.get());
    for (const auto &n : m.normals) CHECK((vecEq(n, SFVec3f{0, 0, 1})));
  }

  // =====================================================================
  // MSH-3: a CONCAVE polygon with convex=FALSE triangulates by ear-clipping,
  // so the total triangle area equals the polygon area and every triangle
  // shares one winding. An L / arrow polygon fanned naively would emit an
  // inverted triangle (area mismatch / flipped normal).
  //
  // Arrow / dart polygon in the z=0 plane (CCW), reflex vertex at index 2:
  //   (0,0) (4,2) (0,4) (1,2)      -- a concave "dart" (reflex at vertex 3).
  // =====================================================================
  {
    const std::vector<SFVec3f> poly = {
        {0, 0, 0}, {4, 2, 0}, {0, 4, 0}, {1, 2, 0}};
    const std::vector<int> idx = {0, 1, 2, 3, -1};

    // Shoelace area + a concavity check (one interior cross product flips sign).
    auto shoelace = [&](const std::vector<SFVec3f> &p) {
      float a = 0.0f;
      for (std::size_t i = 0; i < p.size(); ++i) {
        const SFVec3f &u = p[i];
        const SFVec3f &v = p[(i + 1) % p.size()];
        a += u.x * v.y - v.x * u.y;
      }
      return std::fabs(a) * 0.5f;
    };
    const float polyArea = shoelace(poly);
    // Genuinely concave: at least one vertex turns the opposite way (a fan from
    // vertex 0 would emit an inverted ear here).
    bool concave = false;
    for (std::size_t i = 0; i < poly.size(); ++i) {
      const SFVec3f &a = poly[(i + poly.size() - 1) % poly.size()];
      const SFVec3f &b = poly[i];
      const SFVec3f &c = poly[(i + 1) % poly.size()];
      if (cross(sub(b, a), sub(c, b)).z < 0) concave = true;
    }
    CHECK((concave));

    auto g = createX3DNode("IndexedFaceSet");
    setF(g, "coord", std::any(std::static_pointer_cast<X3DNode>(makeCoord(poly))));
    setF(g, "coordIndex", std::any(idx));
    setF(g, "convex", std::any(false));
    MeshData m = buildLocalMesh(g.get());
    CHECK((!m.indices.empty() && m.indices.size() % 3 == 0));

    // Sum signed triangle areas (vector area along +Z) and confirm every
    // triangle winds the same way (no inverted ear).
    float signedAreaZ = 0.0f;
    float absArea = 0.0f;
    bool allSameWinding = true;
    for (std::size_t t = 0; t + 2 < m.indices.size(); t += 3) {
      const SFVec3f &a = m.positions[m.indices[t]];
      const SFVec3f &b = m.positions[m.indices[t + 1]];
      const SFVec3f &c = m.positions[m.indices[t + 2]];
      const SFVec3f cr = cross(sub(b, a), sub(c, a));
      const float z = cr.z * 0.5f;
      signedAreaZ += z;
      absArea += std::fabs(z);
      if (z <= 1e-6f) allSameWinding = false; // an inverted/degenerate ear.
    }
    // Net area equals the polygon area (a fan with an inverted ear cancels and
    // |signed| != sum-of-abs).
    CHECK((feq(signedAreaZ, polyArea)));
    CHECK((feq(absArea, polyArea)));
    CHECK((allSameWinding));
  }

  // MSH-3b: convex=TRUE (default) is unchanged — a convex quad still fans and
  // covers its area with consistent winding.
  {
    const std::vector<SFVec3f> quad = {
        {0, 0, 0}, {2, 0, 0}, {2, 2, 0}, {0, 2, 0}};
    const std::vector<int> idx = {0, 1, 2, 3, -1};
    auto g = createX3DNode("IndexedFaceSet");
    setF(g, "coord", std::any(std::static_pointer_cast<X3DNode>(makeCoord(quad))));
    setF(g, "coordIndex", std::any(idx));
    MeshData m = buildLocalMesh(g.get());
    float area = 0.0f;
    for (std::size_t t = 0; t + 2 < m.indices.size(); t += 3) {
      const SFVec3f &a = m.positions[m.indices[t]];
      const SFVec3f &b = m.positions[m.indices[t + 1]];
      const SFVec3f &c = m.positions[m.indices[t + 2]];
      area += cross(sub(b, a), sub(c, a)).z * 0.5f;
    }
    CHECK((feq(area, 4.0f)));
  }

  return;
}
