// mesh_builder_t2_test.cpp — M2.5 T2 acceptance: MeshBuilder widening to
// strips / fans / quads / ElevationGrid.
//
// Proofs:
//   1) Fan-triangulation counts — Indexed*FanSet and TriangleFanSet(fanCount)
//      both fan a k-vertex polygon into (k-2) triangles, sharing the apex.
//   2) Strip winding (EXPLICIT) — Indexed*StripSet and TriangleStripSet emit a
//      consistent winding: odd-index triangles in the strip are flipped so a
//      front-facing CCW strip stays CCW (a missing flip inverts every other
//      normal). We assert the exact emitted corner ORDER, not just the count.
//   3) Quad split — *QuadSet splits each quad into (0,1,2)+(0,2,3).
//   4) ElevationGrid — (xDim-1)*(zDim-1) cells -> 2 triangles each, positions
//      synthesized from the height grid (NO coord node required).
//   5) Malformed-index / count safety — out-of-range indices, ragged counts,
//      and too-short coord arrays never read out of bounds; they drop the bad
//      primitive and keep the valid ones.
#include "MeshBuilder.hpp"

#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DScene.hpp"

#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;
using namespace x3d::runtime::extract;

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-5f; }

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}

// Build a Coordinate node whose `point` is the given strip of vectors.
static std::shared_ptr<X3DNode> makeCoord(std::vector<SFVec3f> pts) {
  auto c = createX3DNode("Coordinate");
  setF(c, "point", std::any(std::move(pts)));
  return c;
}

static void attachCoord(const std::shared_ptr<X3DNode> &geom,
                        std::vector<SFVec3f> pts) {
  setF(geom, "coord", std::any(std::static_pointer_cast<X3DNode>(makeCoord(std::move(pts)))));
}

// A position in `mesh` equals point p?
static bool posEq(const MeshData &m, std::size_t i, const SFVec3f &p) {
  return feq(m.positions[i].x, p.x) && feq(m.positions[i].y, p.y) &&
         feq(m.positions[i].z, p.z);
}

TEST_CASE("mesh_builder_t2_test") {
  // Five distinct points; fans/strips index into these.
  const SFVec3f P0{0, 0, 0}, P1{1, 0, 0}, P2{1, 1, 0}, P3{0, 1, 0}, P4{-1, 1, 0};

  // ---- 1. IndexedTriangleFanSet: one 5-vertex fan -> 3 triangles ----------
  {
    auto g = createX3DNode("IndexedTriangleFanSet");
    attachCoord(g, {P0, P1, P2, P3, P4});
    setF(g, "index", std::any(std::vector<int>{0, 1, 2, 3, 4}));
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.indices.size() == 9)); // (5-2)=3 triangles
    // Fan corners: (0,1,2)(0,2,3)(0,3,4) — apex P0 repeated.
    CHECK((posEq(m, 0, P0) && posEq(m, 1, P1) && posEq(m, 2, P2)));
    CHECK((posEq(m, 3, P0) && posEq(m, 4, P2) && posEq(m, 5, P3)));
    CHECK((posEq(m, 6, P0) && posEq(m, 7, P3) && posEq(m, 8, P4)));
  }

  // ---- 1b. IndexedTriangleFanSet: two fans separated by -1 ----------------
  {
    auto g = createX3DNode("IndexedTriangleFanSet");
    attachCoord(g, {P0, P1, P2, P3, P4});
    setF(g, "index", std::any(std::vector<int>{0, 1, 2, -1, 2, 3, 4}));
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.indices.size() == 6)); // 1 + 1 triangles
  }

  // ---- 1c. TriangleFanSet(fanCount): implicit coords partitioned ----------
  {
    auto g = createX3DNode("TriangleFanSet");
    attachCoord(g, {P0, P1, P2, P3, P4});
    setF(g, "fanCount", std::any(std::vector<int>{3, 2}));
    // first fan = pts[0..2] -> 1 tri; second fan = pts[3..4] -> 0 tris (needs 3).
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.indices.size() == 3));
    CHECK((posEq(m, 0, P0) && posEq(m, 1, P1) && posEq(m, 2, P2)));
  }
  {
    auto g = createX3DNode("TriangleFanSet");
    attachCoord(g, {P0, P1, P2, P3, P4});
    setF(g, "fanCount", std::any(std::vector<int>{5}));
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.indices.size() == 9)); // 5-vertex fan -> 3 triangles
  }

  // ---- 2. IndexedTriangleStripSet: explicit winding flip ------------------
  // Strip of 4 vertices [0,1,2,3] -> 2 triangles. Vertex order in a strip:
  //   tri0 (even): v0,v1,v2
  //   tri1 (odd) : flipped so winding stays consistent => v1,v3,v2
  //                (i.e. swap the trailing pair of v1,v2,v3).
  {
    auto g = createX3DNode("IndexedTriangleStripSet");
    attachCoord(g, {P0, P1, P2, P3});
    setF(g, "index", std::any(std::vector<int>{0, 1, 2, 3}));
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.indices.size() == 6)); // (4-2)=2 triangles
    // tri0: P0,P1,P2
    CHECK((posEq(m, 0, P0) && posEq(m, 1, P1) && posEq(m, 2, P2)));
    // tri1 (odd): flipped -> P1,P3,P2  (NOT P1,P2,P3, which would invert normal)
    CHECK((posEq(m, 3, P1) && posEq(m, 4, P3) && posEq(m, 5, P2)));
  }

  // ---- 2b. TriangleStripSet(stripCount): two strips -----------------------
  {
    auto g = createX3DNode("TriangleStripSet");
    attachCoord(g, {P0, P1, P2, P3, P4});
    setF(g, "stripCount", std::any(std::vector<int>{4, 1}));
    // strip0 = pts[0..3] -> 2 tris; strip1 = pts[4] -> 0 tris.
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.indices.size() == 6));
    CHECK((posEq(m, 0, P0) && posEq(m, 1, P1) && posEq(m, 2, P2)));
    CHECK((posEq(m, 3, P1) && posEq(m, 4, P3) && posEq(m, 5, P2))); // odd flip
  }

  // ---- 3. IndexedQuadSet: each quad -> 2 triangles ------------------------
  {
    auto g = createX3DNode("IndexedQuadSet");
    attachCoord(g, {P0, P1, P2, P3});
    setF(g, "index", std::any(std::vector<int>{0, 1, 2, 3}));
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.indices.size() == 6));
    // (0,1,2) + (0,2,3)
    CHECK((posEq(m, 0, P0) && posEq(m, 1, P1) && posEq(m, 2, P2)));
    CHECK((posEq(m, 3, P0) && posEq(m, 4, P2) && posEq(m, 5, P3)));
  }

  // ---- 3b. QuadSet: implicit consecutive quads ----------------------------
  {
    auto g = createX3DNode("QuadSet");
    attachCoord(g, {P0, P1, P2, P3});
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.indices.size() == 6));
    CHECK((posEq(m, 0, P0) && posEq(m, 1, P1) && posEq(m, 2, P2)));
    CHECK((posEq(m, 3, P0) && posEq(m, 4, P2) && posEq(m, 5, P3)));
  }

  // ---- 4. ElevationGrid: 2x2 grid -> 1 cell -> 2 triangles ----------------
  // No coord node; positions synthesized from height + spacing.
  {
    auto g = createX3DNode("ElevationGrid");
    setF(g, "xDimension", std::any(2));
    setF(g, "zDimension", std::any(2));
    setF(g, "xSpacing", std::any(2.0f));
    setF(g, "zSpacing", std::any(3.0f));
    // height row-major: (x + z*xDim). Heights all 0 except corner.
    setF(g, "height", std::any(std::vector<float>{0.0f, 0.0f, 0.0f, 5.0f}));
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.indices.size() == 6)); // (2-1)*(2-1)=1 cell -> 2 tris
    // Grid vertex (i,j) at (i*xSpacing, height[j*xDim+i], j*zSpacing).
    // Corner (1,1) carries height 5 at world (2, 5, 3).
    bool sawCorner = false;
    for (std::size_t i = 0; i < m.positions.size(); ++i)
      if (posEq(m, i, SFVec3f{2.0f, 5.0f, 3.0f})) sawCorner = true;
    CHECK((sawCorner));
    // Origin vertex (0,0) present at (0,0,0).
    bool sawOrigin = false;
    for (std::size_t i = 0; i < m.positions.size(); ++i)
      if (posEq(m, i, SFVec3f{0, 0, 0})) sawOrigin = true;
    CHECK((sawOrigin));
  }

  // ---- 4b. ElevationGrid larger: 3x2 -> (3-1)*(2-1)=2 cells -> 4 tris ------
  {
    auto g = createX3DNode("ElevationGrid");
    setF(g, "xDimension", std::any(3));
    setF(g, "zDimension", std::any(2));
    setF(g, "xSpacing", std::any(1.0f));
    setF(g, "zSpacing", std::any(1.0f));
    setF(g, "height", std::any(std::vector<float>(6, 0.0f)));
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.indices.size() == 12));
  }

  // ---- 5. Malformed-index / count safety ----------------------------------
  // 5a. Out-of-range fan index dropped, valid fan kept.
  {
    auto g = createX3DNode("IndexedTriangleFanSet");
    attachCoord(g, {P0, P1, P2});
    setF(g, "index", std::any(std::vector<int>{0, 1, 99, -1, 0, 1, 2}));
    MeshData m = buildLocalMesh(g.get());
    // first fan has an OOB vertex (only its bad triangle is dropped);
    // second fan is fully valid -> at least the valid triangle survives.
    CHECK((m.indices.size() == 3));
    CHECK((posEq(m, 0, P0) && posEq(m, 1, P1) && posEq(m, 2, P2)));
  }
  // 5b. Strip OOB index never reads out of bounds.
  {
    auto g = createX3DNode("IndexedTriangleStripSet");
    attachCoord(g, {P0, P1, P2});
    setF(g, "index", std::any(std::vector<int>{0, 1, 2, 99}));
    MeshData m = buildLocalMesh(g.get());
    // tri0 (0,1,2) valid; tri1 (1,99,2) dropped.
    CHECK((m.indices.size() == 3));
  }
  // 5c. fanCount overruns the coord array -> guarded, no OOB.
  {
    auto g = createX3DNode("TriangleFanSet");
    attachCoord(g, {P0, P1, P2});
    setF(g, "fanCount", std::any(std::vector<int>{10})); // claims 10, only 3 exist
    MeshData m = buildLocalMesh(g.get());
    // Only the 3 real points form 1 triangle; the phantom run is clamped.
    CHECK((m.indices.size() == 3));
  }
  // 5d. QuadSet with a ragged tail (5 points) -> 1 quad, leftover ignored.
  {
    auto g = createX3DNode("QuadSet");
    attachCoord(g, {P0, P1, P2, P3, P4});
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.indices.size() == 6)); // exactly one quad
  }
  // 5e. ElevationGrid with too-short height array -> guarded, empty mesh.
  {
    auto g = createX3DNode("ElevationGrid");
    setF(g, "xDimension", std::any(2));
    setF(g, "zDimension", std::any(2));
    setF(g, "height", std::any(std::vector<float>{0.0f})); // need 4
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.positions.empty() && m.indices.empty()));
  }
  // 5f. Degenerate grid dims -> empty.
  {
    auto g = createX3DNode("ElevationGrid");
    setF(g, "xDimension", std::any(1));
    setF(g, "zDimension", std::any(1));
    setF(g, "height", std::any(std::vector<float>{0.0f}));
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.indices.empty()));
  }

  return;
}
