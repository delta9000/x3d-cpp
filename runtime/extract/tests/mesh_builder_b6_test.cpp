// mesh_builder_b6_test.cpp — B6 acceptance: creaseAngle smooth-normal generation.
//
// The flat per-face normal fill is replaced by a creaseAngle-aware post-pass when
// NO Normal node is authored. Adjacency is re-derived in SOURCE-index space
// (coordIndex for IFS; the lattice id for ElevationGrid / Extrusion /
// GeoElevationGrid) so coincident-but-distinct verts are NOT over-smoothed, and
// the expand layout is preserved (the averaged normal is scattered back onto each
// expanded corner; no re-indexing).
//
// Proofs:
//   1) NON-REGRESSIVE: creaseAngle default 0 reproduces today's exact FLAT
//      output. An IFS cube with creaseAngle absent/0 keeps per-face normals (each
//      triangle's three corners share its geometric face normal).
//   2) creaseAngle=PI smooths a sphere-of-quads (an octahedron IFS): every corner
//      at a shared coordIndex vertex gets the AVERAGE of all incident face
//      normals (== the outward radial direction for an origin-centred octahedron),
//      so adjacent shared-edge normals are fused, NOT faceted.
//   3) SOURCE-INDEX adjacency, not the expanded run: an IFS where two faces meet
//      at a vertex authored as TWO DISTINCT coordIndex entries (a hard seam) does
//      NOT fuse across the seam even at creaseAngle=PI (the design's
//      coincident-distinct guard).
//   4) ElevationGrid smooths keyed on the lattice id (row*xDim+col); creaseAngle=0
//      stays byte-identical to the pre-B6 flat output.
//   5) Authored Normal node WINS: creaseAngle is ignored when a Normal is present.
#include "MeshBuilder.hpp"

#include "x3d/nodes/X3DNodeFactory.hpp"

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
// Geometric (CCW) face normal of the triangle whose corners start at run `t`.
static SFVec3f triGeoNormal(const MeshData &m, std::size_t t) {
  const SFVec3f &a = m.positions[m.indices[t]];
  const SFVec3f &b = m.positions[m.indices[t + 1]];
  const SFVec3f &c = m.positions[m.indices[t + 2]];
  return normz(cross(sub(b, a), sub(c, a)));
}

static const float PI = 3.14159265358979323846f;

TEST_CASE("mesh_builder_b6_test") {
  // 8 unit-cube corners (centred at origin), CCW-outward faces.
  const std::vector<SFVec3f> cube = {
      {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}, // back  (z=-1)
      {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1},  // front (z=+1)
  };
  // 6 quad faces, CCW as seen from outside.
  const std::vector<int> cubeIdx = {
      4, 5, 6, 7, -1, // +Z
      1, 0, 3, 2, -1, // -Z
      0, 4, 7, 3, -1, // -X
      5, 1, 2, 6, -1, // +X
      3, 7, 6, 2, -1, // +Y
      0, 1, 5, 4, -1, // -Y
  };

  // ---- 1. NON-REGRESSIVE: creaseAngle absent (default 0) => FLAT --------------
  {
    auto g = createX3DNode("IndexedFaceSet");
    setF(g, "coord", std::any(std::static_pointer_cast<X3DNode>(makeCoord(cube))));
    setF(g, "coordIndex", std::any(cubeIdx));
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.hasNormals && m.normals.size() == m.positions.size()));
    // Every triangle's three corner normals == its own geometric face normal
    // (faceted). Each face's two tris point along the same axis.
    for (std::size_t t = 0; t + 2 < m.indices.size(); t += 3) {
      const SFVec3f fn = triGeoNormal(m, t);
      CHECK((vecEq(m.normals[m.indices[t]], fn)));
      CHECK((vecEq(m.normals[m.indices[t + 1]], fn)));
      CHECK((vecEq(m.normals[m.indices[t + 2]], fn)));
    }
  }

  // ---- 1b. creaseAngle=0 EXPLICIT is identical to the default ---------------
  {
    auto g0 = createX3DNode("IndexedFaceSet");
    setF(g0, "coord", std::any(std::static_pointer_cast<X3DNode>(makeCoord(cube))));
    setF(g0, "coordIndex", std::any(cubeIdx));
    MeshData a = buildLocalMesh(g0.get());

    auto g1 = createX3DNode("IndexedFaceSet");
    setF(g1, "coord", std::any(std::static_pointer_cast<X3DNode>(makeCoord(cube))));
    setF(g1, "coordIndex", std::any(cubeIdx));
    setF(g1, "creaseAngle", std::any(0.0f));
    MeshData b = buildLocalMesh(g1.get());

    CHECK((a.normals.size() == b.normals.size()));
    for (std::size_t i = 0; i < a.normals.size(); ++i)
      CHECK((vecEq(a.normals[i], b.normals[i]))); // byte-identical flat fill.
  }

  // ---- 1c. creaseAngle=PI on a CUBE: every shared corner FUSES (no longer
  // faceted). The exact averaged direction is triangle-count-weighted across the
  // three incident faces, so we assert only that the corner normal points
  // OUTWARD into the +++ octant and is NOT any single axis (i.e. it is smoothed).
  {
    auto g = createX3DNode("IndexedFaceSet");
    setF(g, "coord", std::any(std::static_pointer_cast<X3DNode>(makeCoord(cube))));
    setF(g, "coordIndex", std::any(cubeIdx));
    setF(g, "creaseAngle", std::any(PI));
    MeshData m = buildLocalMesh(g.get());
    bool sawCorner = false;
    for (std::size_t i = 0; i < m.positions.size(); ++i)
      if (vecEq(m.positions[i], SFVec3f{1, 1, 1})) {
        sawCorner = true;
        const SFVec3f &n = m.normals[i];
        // All three components positive (outward into the +++ octant) and no
        // single axis dominates to 1 — proof the three faces fused, not faceted.
        CHECK((n.x > 0.1f && n.y > 0.1f && n.z > 0.1f));
        CHECK((!vecEq(n, SFVec3f{1, 0, 0}) && !vecEq(n, SFVec3f{0, 1, 0}) &&
               !vecEq(n, SFVec3f{0, 0, 1})));
      }
    CHECK((sawCorner));
  }

  // ---- 2. creaseAngle=PI octahedron => per-vertex radial (sphere-of-quads) ----
  // Octahedron: 6 verts on the axes, 8 triangular faces. At PI every face at a
  // shared coord vertex fuses; the averaged normal == the outward radial dir.
  {
    const std::vector<SFVec3f> oct = {
        {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    // 8 CCW-outward triangular faces (each pairs +/-X/Y/Z apex with an equator
    // pair); coordIndex with -1 separators so each is its own face.
    const std::vector<int> octIdx = {
        4, 0, 2, -1, 4, 2, 1, -1, 4, 1, 3, -1, 4, 3, 0, -1,
        5, 2, 0, -1, 5, 1, 2, -1, 5, 3, 1, -1, 5, 0, 3, -1};
    auto g = createX3DNode("IndexedFaceSet");
    setF(g, "coord", std::any(std::static_pointer_cast<X3DNode>(makeCoord(oct))));
    setF(g, "coordIndex", std::any(octIdx));
    setF(g, "creaseAngle", std::any(PI));
    MeshData m = buildLocalMesh(g.get());
    // Each expanded corner at coord vertex V must carry the normalized V
    // (outward radial) — the smooth sphere-of-quads result.
    for (std::size_t i = 0; i < m.positions.size(); ++i) {
      const SFVec3f rad = normz(m.positions[i]);
      CHECK((vecEq(m.normals[i], rad)));
    }
    // And it is NOT the flat per-face normal anymore on at least one corner.
    bool differs = false;
    for (std::size_t t = 0; t + 2 < m.indices.size(); t += 3) {
      const SFVec3f fn = triGeoNormal(m, t);
      if (!vecEq(m.normals[m.indices[t]], fn)) { differs = true; break; }
    }
    CHECK((differs));
  }

  // ---- 3. SOURCE-INDEX adjacency: a hard seam (distinct coord entries) does
  // NOT fuse even at PI. Two coplanar-adjacent triangles whose shared edge is
  // authored with DUPLICATE coordinate points keep their own face normals. ----
  {
    // Two triangles forming a bent "tent": faces tilt opposite ways about the
    // shared edge along X. The shared edge is authored as DISTINCT coord ids
    // (2,3 for face A; 4,5 for face B) — a deliberate hard seam.
    const std::vector<SFVec3f> tent = {
        {0, 0, 0}, {0, 0, 0},   // apex-ish, two distinct entries
        {1, 1, 0}, {-1, 1, 0},  // face A edge (left tilt)
        {1, 1, 0}, {-1, 1, 0},  // face B edge DUPLICATE coords (seam)
    };
    // Face A: 0,2,3 ; Face B: 1,5,4  (winding chosen so normals differ).
    const std::vector<int> tIdx = {0, 2, 3, -1, 1, 5, 4, -1};
    auto g = createX3DNode("IndexedFaceSet");
    setF(g, "coord", std::any(std::static_pointer_cast<X3DNode>(makeCoord(tent))));
    setF(g, "coordIndex", std::any(tIdx));
    setF(g, "creaseAngle", std::any(PI));
    MeshData m = buildLocalMesh(g.get());
    // Faces A and B share NO coord id (2,3 vs 4,5 are distinct entries), so each
    // corner keeps its OWN face normal — no cross-seam fusing.
    const SFVec3f fnA = triGeoNormal(m, 0);
    const SFVec3f fnB = triGeoNormal(m, 3);
    for (std::size_t k = 0; k < 3; ++k) CHECK((vecEq(m.normals[k], fnA)));
    for (std::size_t k = 3; k < 6; ++k) CHECK((vecEq(m.normals[k], fnB)));
  }

  // ---- 4. ElevationGrid: creaseAngle=0 flat is byte-identical; PI smooths -----
  {
    auto flat = createX3DNode("ElevationGrid");
    setF(flat, "xDimension", std::any(3));
    setF(flat, "zDimension", std::any(3));
    setF(flat, "xSpacing", std::any(1.0f));
    setF(flat, "zSpacing", std::any(1.0f));
    // A non-planar bump so face normals genuinely differ cell-to-cell.
    setF(flat, "height",
         std::any(std::vector<float>{0, 0, 0, 0, 2, 0, 0, 0, 0}));
    MeshData a = buildLocalMesh(flat.get());
    CHECK((!a.positions.empty()));
    CHECK((a.latticeIndex.size() == a.positions.size()));
    // creaseAngle 0 => every corner == its triangle's geometric face normal.
    for (std::size_t t = 0; t + 2 < a.indices.size(); t += 3) {
      const SFVec3f fn = triGeoNormal(a, t);
      CHECK((vecEq(a.normals[a.indices[t]], fn)));
    }

    auto smooth = createX3DNode("ElevationGrid");
    setF(smooth, "xDimension", std::any(3));
    setF(smooth, "zDimension", std::any(3));
    setF(smooth, "xSpacing", std::any(1.0f));
    setF(smooth, "zSpacing", std::any(1.0f));
    setF(smooth, "height",
         std::any(std::vector<float>{0, 0, 0, 0, 2, 0, 0, 0, 0}));
    setF(smooth, "creaseAngle", std::any(PI));
    MeshData b = buildLocalMesh(smooth.get());
    // The raised centre vertex (lattice id 4) is shared by many cells; with PI
    // its corners are smoothed to the AVERAGE of incident faces — NOT the flat
    // per-face normal. Find a corner at the centre and confirm it changed.
    bool centreSmoothed = false;
    for (std::size_t i = 0; i < b.positions.size(); ++i)
      if (b.latticeIndex[i] == 4u) {
        // Its triangle's flat normal:
        const std::size_t t = (i / 3) * 3;
        const SFVec3f fn = triGeoNormal(b, t);
        if (!vecEq(b.normals[i], fn)) centreSmoothed = true;
      }
    CHECK((centreSmoothed));
    // Same vertex/triangle COUNT as the flat grid (expand layout preserved).
    CHECK((b.positions.size() == a.positions.size()));
  }

  // ---- 5. Authored Normal WINS over creaseAngle ------------------------------
  {
    auto g = createX3DNode("IndexedFaceSet");
    setF(g, "coord", std::any(std::static_pointer_cast<X3DNode>(makeCoord(cube))));
    setF(g, "coordIndex", std::any(cubeIdx));
    setF(g, "creaseAngle", std::any(PI));
    // Author an explicit Normal: a single +Z normal indexed by every corner.
    auto nrm = createX3DNode("Normal");
    setF(nrm, "vector", std::any(std::vector<SFVec3f>{{0, 0, 1}}));
    setF(g, "normal", std::any(std::static_pointer_cast<X3DNode>(nrm)));
    setF(g, "normalPerVertex", std::any(true));
    setF(g, "normalIndex", std::any(std::vector<int>{})); // coordIndex doubles.
    MeshData m = buildLocalMesh(g.get());
    // Every corner uses the authored normal (coord-indexed into a 1-entry list:
    // pickIndex returns coord which is >0 -> out of range -> flat fallback for
    // most corners; the point is creaseAngle did NOT run, so corners are NOT the
    // smooth radial). Assert at least one corner is the authored {0,0,1}.
    bool sawAuthored = false;
    for (const auto &n : m.normals)
      if (vecEq(n, SFVec3f{0, 0, 1})) sawAuthored = true;
    CHECK((sawAuthored));
    // And NO corner is the smooth diagonal radial (creaseAngle was skipped).
    const SFVec3f diag = normz(SFVec3f{1, 1, 1});
    for (const auto &n : m.normals) CHECK((!vecEq(n, diag)));
  }

  return;
}
