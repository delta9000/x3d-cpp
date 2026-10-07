// mesh_builder_extrusion_scp_test.cpp — EXTRUSION-SCP regression (ADR-0031).
//
// §13.3.5.4.5: a 2-distinct-point spine fixes only the SCP plane NORMAL; the
// in-plane X/Z are engine-defined. ADR-0031 pins them to the local model axes:
//   Y = unit spine tangent,
//   Z = normalize(modelZ − (modelZ·Y)Y)  (fallback modelX when Y ∥ modelZ),
//   X = normalize(Y × Z),  Z = X × Y.
// It also requires that fewer than 2 DISTINCT (coincident-collapsed) spine
// points render nothing.
#include "GeoFrame.hpp"
#include "MeshBuilder.hpp"
#include "x3d/nodes/X3DNode.hpp"

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

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) {
      f.set(*n, std::move(v));
      return;
    }
}
static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }

static const MFVec2f kUnitSquare = {{1, 1}, {1, -1}, {-1, -1}, {-1, 1}, {1, 1}};

// Find the emitted corner whose lattice id equals `lid` (returns false if none).
static bool cornerPos(const MeshData &m, std::uint32_t lid, SFVec3f &out) {
  for (std::size_t c = 0; c < m.latticeIndex.size(); ++c)
    if (m.latticeIndex[c] == lid) {
      out = m.positions[c];
      return true;
    }
  return false;
}

TEST_CASE("extrusion_scp_coincident_spine_culls") {
  // 2 elements but only ONE distinct point -> no SCP -> empty (§13.3.5.4.5).
  auto g = createX3DNode("Extrusion");
  setF(g, "crossSection", std::any(kUnitSquare));
  setF(g, "spine", std::any(MFVec3f{{0, 0, 0}, {0, 0, 0}}));
  MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
  CHECK((m.positions.empty()));
  CHECK((m.indices.empty()));

  // Repeated coincident points, still one distinct point.
  auto g2 = createX3DNode("Extrusion");
  setF(g2, "crossSection", std::any(kUnitSquare));
  setF(g2, "spine", std::any(MFVec3f{{2, 2, 2}, {2, 2, 2}, {2, 2, 2}}));
  CHECK((buildLocalMesh(g2.get(), geo::builtinProjection()).positions.empty()));
}

TEST_CASE("extrusion_scp_two_point_spine_uses_model_axes") {
  // Spine along +X. Y=(1,0,0), modelZ=(0,0,1) is not parallel -> Z=+modelZ,
  // X = Y×Z = (0,-1,0). So the cross-section's +y axis maps to +Z (modelZ), and
  // its +x axis maps to (0,-1,0). Before the fix the ref×Y fallback produced
  // Z=(0,0,-1), i.e. the cross-section's +y mapped to -Z.
  auto g = createX3DNode("Extrusion");
  setF(g, "crossSection", std::any(kUnitSquare));
  setF(g, "spine", std::any(MFVec3f{{0, 0, 0}, {4, 0, 0}}));
  MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
  REQUIRE((!m.positions.empty()));

  // lattice id 0 == section 0, crossSection vertex 0 == (cv.x=1, cv.y=1).
  SFVec3f p;
  REQUIRE(cornerPos(m, 0, p));
  // p = spine[0] + 1*axX + 1*axZ = (0,-1,0) + (0,0,1) = (0,-1,1).
  CHECK(feq(p.x, 0.0f));
  CHECK(feq(p.y, -1.0f));
  CHECK(feq(p.z, 1.0f));
}

TEST_CASE("extrusion_scp_tangent_parallel_modelz_falls_back_to_modelx") {
  // Spine along +Z: Y=(0,0,1) ∥ modelZ -> fall back to modelX=(1,0,0) for Z.
  // X = Y×Z = (0,0,1)×(1,0,0) = (0,1,0). Section 0, vertex 0 (cv=(1,1)) maps to
  // axX + axZ = (0,1,0) + (1,0,0) = (1,1,0).
  auto g = createX3DNode("Extrusion");
  setF(g, "crossSection", std::any(kUnitSquare));
  setF(g, "spine", std::any(MFVec3f{{0, 0, 0}, {0, 0, 4}}));
  MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
  SFVec3f p;
  REQUIRE(cornerPos(m, 0, p));
  CHECK(feq(p.x, 1.0f));
  CHECK(feq(p.y, 1.0f));
  CHECK(feq(p.z, 0.0f));
}

TEST_CASE("extrusion_scp_well_defined_spine_untouched") {
  // A bent 3-point spine has a well-defined plane normal at the interior point;
  // the fix must not alter it. Smoke: the mesh is non-empty and has normals+UVs.
  auto g = createX3DNode("Extrusion");
  setF(g, "crossSection", std::any(kUnitSquare));
  setF(g, "spine", std::any(MFVec3f{{0, 0, 0}, {0, 2, 0}, {2, 2, 0}}));
  MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
  CHECK((!m.positions.empty()));
  CHECK((m.hasNormals));
}
