#include "GeoFrame.hpp"
#include "MeshBuilder.hpp"
#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DNodeFactory.hpp"

#include <any>
#include "doctest/doctest.h"
#include "scene_test_helpers.hpp"
#include <cmath>
#include <memory>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;
using namespace x3d::runtime::extract;
using x3d::test::setF;

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-5f; }

static std::shared_ptr<X3DNode> makeCoord(std::vector<SFVec3f> pts) {
  auto c = createX3DNode("Coordinate");
  setF(c, "point", std::any(std::move(pts)));
  return c;
}
static void attachCoord(const std::shared_ptr<X3DNode> &g, std::vector<SFVec3f> pts) {
  setF(g, "coord", std::any(std::static_pointer_cast<X3DNode>(makeCoord(std::move(pts)))));
}
static void attachTexCoord(const std::shared_ptr<X3DNode> &g, std::vector<SFVec2f> p) {
  auto n = createX3DNode("TextureCoordinate");
  setF(n, "point", std::any(std::move(p)));
  setF(g, "texCoord", std::any(std::static_pointer_cast<X3DNode>(n)));
}

// True iff `tc` matches (sx,ty) at any expanded corner whose position equals p.
static bool hasUvAt(const MeshData &m, const SFVec3f &p, float sx, float ty) {
  for (std::size_t i = 0; i < m.positions.size(); ++i)
    if (feq(m.positions[i].x, p.x) && feq(m.positions[i].y, p.y) &&
        feq(m.positions[i].z, p.z) && feq(m.texcoords[i].x, sx) &&
        feq(m.texcoords[i].y, ty))
      return true;
  return false;
}

TEST_CASE("mesh_builder_tc1_test") {
  const SFVec3f P0{0, 0, 0}, P1{1, 0, 0}, P2{1, 1, 0}, P3{0, 1, 0};

  // 1. Unit-square IFS, XY plane, no TextureCoordinate
  // size = (1,1,0). X & Y tie for largest -> S=X, T=Y. s=x, t=y. Corners span
  // s in {0,1}, t in {0,1}.
  {
    auto g = createX3DNode("IndexedFaceSet");
    attachCoord(g, {P0, P1, P2, P3});
    setF(g, "coordIndex", std::any(std::vector<int>{0, 1, 2, 3, -1}));
    MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
    CHECK((!m.texcoords.empty()));
    CHECK((m.texcoords.size() == m.positions.size())); // one per expanded corner.
    CHECK((hasUvAt(m, P0, 0.0f, 0.0f)));
    CHECK((hasUvAt(m, P1, 1.0f, 0.0f)));
    CHECK((hasUvAt(m, P2, 1.0f, 1.0f)));
    CHECK((hasUvAt(m, P3, 0.0f, 1.0f)));
  }

  // 2. 2x1 quad (X largest) -> aspect preserved: t in {0,0.5}
  // points (0,0),(2,0),(2,1),(0,1): size=(2,1,0). S=X (size 2), T=Y (size 1).
  // s = x/2 -> {0,1}; t = y/2 -> {0,0.5}.
  {
    const SFVec3f Q0{0, 0, 0}, Q1{2, 0, 0}, Q2{2, 1, 0}, Q3{0, 1, 0};
    auto g = createX3DNode("IndexedFaceSet");
    attachCoord(g, {Q0, Q1, Q2, Q3});
    setF(g, "coordIndex", std::any(std::vector<int>{0, 1, 2, 3, -1}));
    MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
    CHECK((!m.texcoords.empty()));
    CHECK((hasUvAt(m, Q0, 0.0f, 0.0f)));
    CHECK((hasUvAt(m, Q1, 1.0f, 0.0f)));   // s = 2/2
    CHECK((hasUvAt(m, Q2, 1.0f, 0.5f)));   // s=1, t = 1/2
    CHECK((hasUvAt(m, Q3, 0.0f, 0.5f)));   // t = 1/2 (aspect preserved)
  }

  // 3. Authored TextureCoordinate wins — byte-unchanged
  // Build the same IFS twice: once with an authored TextureCoordinate, once with
  // the matching authored UVs, and a control with no texCoord. The authored case
  // must equal the authored UVs (not the bbox projection).
  {
    auto g = createX3DNode("IndexedFaceSet");
    attachCoord(g, {P0, P1, P2, P3});
    setF(g, "coordIndex", std::any(std::vector<int>{0, 1, 2, 3, -1}));
    // Author deliberately non-bbox UVs (all 0.25,0.75) so a regression that lets
    // generation overwrite them would change the values.
    attachTexCoord(g, {SFVec2f{0.25f, 0.75f}, SFVec2f{0.25f, 0.75f},
                       SFVec2f{0.25f, 0.75f}, SFVec2f{0.25f, 0.75f}});
    MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
    CHECK((m.texcoords.size() == m.positions.size()));
    for (const auto &tc : m.texcoords)
      CHECK((feq(tc.x, 0.25f) && feq(tc.y, 0.75f))); // authored, NOT projected.
  }

  // 4. Generation spans every composed/triangle geometry type
  {
    // TriangleSet: 3 implicit coords -> 3 corners, all get a texcoord.
    auto g = createX3DNode("TriangleSet");
    attachCoord(g, {P0, P1, P2});
    MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
    CHECK((m.texcoords.size() == m.positions.size() && m.texcoords.size() == 3));
  }
  {
    auto g = createX3DNode("IndexedTriangleSet");
    attachCoord(g, {P0, P1, P2});
    setF(g, "index", std::any(std::vector<int>{0, 1, 2}));
    MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
    CHECK((m.texcoords.size() == m.positions.size() && !m.texcoords.empty()));
  }
  {
    auto g = createX3DNode("QuadSet");
    attachCoord(g, {P0, P1, P2, P3});
    MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
    CHECK((m.texcoords.size() == m.positions.size() && !m.texcoords.empty()));
  }
  {
    auto g = createX3DNode("IndexedQuadSet");
    attachCoord(g, {P0, P1, P2, P3});
    setF(g, "index", std::any(std::vector<int>{0, 1, 2, 3}));
    MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
    CHECK((m.texcoords.size() == m.positions.size() && !m.texcoords.empty()));
  }

  // 5. Line/point sets get no generated texcoords
  {
    auto g = createX3DNode("IndexedLineSet");
    attachCoord(g, {P0, P1, P2});
    setF(g, "coordIndex", std::any(std::vector<int>{0, 1, 2, -1}));
    MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
    CHECK((m.topology == Topology::Lines));
    CHECK((m.texcoords.empty())); // no implicit UVs for lines.
  }
  {
    auto g = createX3DNode("PointSet");
    attachCoord(g, {P0, P1, P2});
    MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
    CHECK((m.topology == Topology::Points));
    CHECK((m.texcoords.empty()));
  }

  // 6. Sdim/Tdim selection: Z largest, X second (Y smallest)
  // points span X=2, Y=0, Z=4 -> S=Z (size4), T=X (size2). s=z/4, t=x/4.
  {
    const SFVec3f R0{0, 0, 0}, R1{2, 0, 0}, R2{2, 0, 4}, R3{0, 0, 4};
    auto g = createX3DNode("IndexedFaceSet");
    attachCoord(g, {R0, R1, R2, R3});
    setF(g, "coordIndex", std::any(std::vector<int>{0, 1, 2, 3, -1}));
    MeshData m = buildLocalMesh(g.get(), geo::builtinProjection());
    CHECK((hasUvAt(m, R0, 0.0f, 0.0f)));   // z=0,x=0
    CHECK((hasUvAt(m, R2, 1.0f, 0.5f)));   // s=z/4=1, t=x/4=0.5
    CHECK((hasUvAt(m, R3, 1.0f, 0.0f)));   // s=z/4=1, t=x/4=0
  }

  return;
}
