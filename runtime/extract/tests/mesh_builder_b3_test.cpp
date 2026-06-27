// mesh_builder_b3_test.cpp — B3 acceptance: MeshBuilder Extrusion tessellation.
//
// Proofs:
//   1) A unit-square cross-section swept along a straight 2-point spine yields a
//      box-like solid: 4 side-wall quads (8 tris) + begin/end caps (2 tris each)
//      = 12 triangles / 36 corners, with the (section,crossSectionVertex)
//      lattice map retained parallel to positions and flat normals present.
//   2) WINDING: with the default ccw=true, the begin cap (at spine[0], the -Y
//      end) faces away from the end cap (at spine[last], the +Y end). The two
//      caps' geometric normals are anti-parallel and roughly along the spine.
//   3) ccw=false flips every emitted triangle's winding (geometric normal sign
//      inverts relative to ccw=true), same triangle/vertex counts.
//   4) <2 spine points => empty mesh.
//   5) recognizedGeometryType("Extrusion") is true.
//   6) Per-section scale shrinks the cross-section: a 2x-scaled section's verts
//      sit at twice the radius of an unscaled one.
#include "MeshBuilder.hpp"

#include "x3d/nodes/X3DNodeFactory.hpp"

#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::runtime::extract;
using namespace x3d::core;
using namespace x3d::nodes;

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) {
      f.set(*n, std::move(v));
      return;
    }
}

static SFVec3f sub(const SFVec3f &a, const SFVec3f &b) {
  return SFVec3f{a.x - b.x, a.y - b.y, a.z - b.z};
}
static SFVec3f cross(const SFVec3f &a, const SFVec3f &b) {
  return SFVec3f{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                 a.x * b.y - a.y * b.x};
}
static float dot(const SFVec3f &a, const SFVec3f &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Geometric (CCW) normal of triangle t (by index triple).
static SFVec3f triGeoNormal(const MeshData &m, std::size_t t) {
  const SFVec3f &a = m.positions[m.indices[t]];
  const SFVec3f &b = m.positions[m.indices[t + 1]];
  const SFVec3f &c = m.positions[m.indices[t + 2]];
  return cross(sub(b, a), sub(c, a));
}

TEST_CASE("mesh_builder_b3_test") {
  // The X3D default crossSection: unit square, closed (first==last) -> 5 verts.
  const MFVec2f unitSquare = {{1, 1}, {1, -1}, {-1, -1}, {-1, 1}, {1, 1}};

  // ---- 1. Box-like counts + lattice map + normals -----------------------
  {
    auto g = createX3DNode("Extrusion");
    setF(g, "crossSection", std::any(unitSquare));
    setF(g, "spine", std::any(MFVec3f{{0, 0, 0}, {0, 1, 0}}));
    MeshData m = buildLocalMesh(g.get());
    // sides: (ns-1)*(nc-1)*2 = 1*4*2 = 8 tris. caps: closed -> capCount=4,
    // fan = 2 tris each, begin+end = 4 tris. total 12 tris.
    CHECK((m.indices.size() == 12 * 3));
    CHECK((m.positions.size() == 36));
    CHECK((m.hasNormals));
    CHECK((m.normals.size() == m.positions.size()));
    // lattice-index-retaining form for B6.
    CHECK((m.latticeIndex.size() == m.positions.size()));
    // lattice ids are section*nc + crossSectionVertex, within [0, ns*nc).
    for (auto id : m.latticeIndex) CHECK((id < 2u * 5u));
  }

  // ---- 2. Cap winding: begin and end caps face opposite ways ------------
  {
    auto g = createX3DNode("Extrusion");
    setF(g, "crossSection", std::any(unitSquare));
    setF(g, "spine", std::any(MFVec3f{{0, 0, 0}, {0, 1, 0}}));
    MeshData m = buildLocalMesh(g.get());
    // The last 4 triangles are the caps (2 begin then 2 end). Collect a normal
    // for a begin-cap tri and an end-cap tri.
    std::size_t ntri = m.indices.size() / 3;
    // begin cap = tris[8],[9]; end cap = tris[10],[11].
    SFVec3f nBegin = triGeoNormal(m, 8 * 3);
    SFVec3f nEnd = triGeoNormal(m, 10 * 3);
    (void)ntri;
    // Caps are in the X-Z spine plane => normals along the spine (+/-Y).
    CHECK((std::fabs(nBegin.y) > 1e-3f));
    CHECK((std::fabs(nEnd.y) > 1e-3f));
    // Opposite facing: begin faces -Y, end faces +Y (anti-parallel).
    CHECK((dot(nBegin, nEnd) < 0.0f));
  }

  // ---- 3. ccw=false flips winding ---------------------------------------
  {
    auto g1 = createX3DNode("Extrusion");
    setF(g1, "crossSection", std::any(unitSquare));
    setF(g1, "spine", std::any(MFVec3f{{0, 0, 0}, {0, 1, 0}}));
    MeshData mc = buildLocalMesh(g1.get());

    auto g2 = createX3DNode("Extrusion");
    setF(g2, "crossSection", std::any(unitSquare));
    setF(g2, "spine", std::any(MFVec3f{{0, 0, 0}, {0, 1, 0}}));
    setF(g2, "ccw", std::any(SFBool(false)));
    MeshData mcw = buildLocalMesh(g2.get());

    CHECK((mc.indices.size() == mcw.indices.size()));
    // Every triangle's geometric normal sign should invert between the two.
    for (std::size_t t = 0; t < mc.indices.size(); t += 3) {
      SFVec3f na = triGeoNormal(mc, t);
      SFVec3f nb = triGeoNormal(mcw, t);
      CHECK((dot(na, nb) < 1e-3f)); // opposite (or both ~0 for degenerate).
    }
  }

  // ---- 4. <2 spine points => empty --------------------------------------
  {
    auto g = createX3DNode("Extrusion");
    setF(g, "crossSection", std::any(unitSquare));
    setF(g, "spine", std::any(MFVec3f{{0, 0, 0}}));
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.positions.empty()));
    CHECK((m.indices.empty()));
  }

  // ---- 5. recognizedGeometryType -----------------------------------------
  {
    bool recognized = false;
    auto g = createX3DNode("Extrusion");
    setF(g, "crossSection", std::any(unitSquare));
    setF(g, "spine", std::any(MFVec3f{{0, 0, 0}, {0, 1, 0}}));
    buildLocalMesh(g.get(), {}, &recognized);
    CHECK((recognized));
    CHECK((recognizedGeometryType("Extrusion")));
  }

  // ---- 6. Per-section scale widens the cross-section ---------------------
  {
    auto g = createX3DNode("Extrusion");
    setF(g, "crossSection", std::any(unitSquare));
    setF(g, "spine", std::any(MFVec3f{{0, 0, 0}, {0, 1, 0}}));
    // section 0 unit-scale, section 1 doubled.
    setF(g, "scale", std::any(MFVec2f{{1, 1}, {2, 2}}));
    MeshData m = buildLocalMesh(g.get());
    // Straight spine on +Y: section i sits at y = i. Find the max horizontal
    // radius at y~0 (section 0) and y~1 (section 1).
    float r0 = 0.0f, r1 = 0.0f;
    for (const auto &p : m.positions) {
      float r = std::sqrt(p.x * p.x + p.z * p.z);
      if (feq(p.y, 0.0f)) r0 = std::max(r0, r);
      if (feq(p.y, 1.0f)) r1 = std::max(r1, r);
    }
    CHECK((r0 > 0.0f));
    CHECK((feq(r1, 2.0f * r0))); // doubled scale => doubled radius.
  }

  return;
}
