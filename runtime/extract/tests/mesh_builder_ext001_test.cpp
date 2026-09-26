// mesh_builder_ext001_test.cpp — EXT-001 regression: ElevationGrid (and the
// GeoElevationGrid shared emitHeightGrid path) must honour authored Color and
// Normal nodes and the colorPerVertex / normalPerVertex flags per §13.3.4.
//
// Before the fix the grid emitter only ever produced generated flat normals and
// no colors: authored nodes were dropped entirely. §13.3.4 maps a per-vertex
// value to the LATTICE vertex (row*xDim + col) and a per-quad value
// (colorPerVertex/normalPerVertex FALSE) to the CELL (row*(xDim-1) + col).
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

static void attachColor(const std::shared_ptr<X3DNode> &g, std::vector<SFColor> c) {
  auto n = createX3DNode("Color");
  setF(n, "color", std::any(std::move(c)));
  setF(g, "color", std::any(std::static_pointer_cast<X3DNode>(n)));
}
static void attachNormal(const std::shared_ptr<X3DNode> &g, std::vector<SFVec3f> v) {
  auto n = createX3DNode("Normal");
  setF(n, "vector", std::any(std::move(v)));
  setF(g, "normal", std::any(std::static_pointer_cast<X3DNode>(n)));
}
static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }
static bool vecEq(const SFVec3f &a, const SFVec3f &b) {
  return feq(a.x, b.x) && feq(a.y, b.y) && feq(a.z, b.z);
}
static bool colEq(const SFColorRGBA &a, const SFColorRGBA &b) {
  return feq(a.r, b.r) && feq(a.g, b.g) && feq(a.b, b.b) && feq(a.a, b.a);
}

// A 3x3 flat height grid (9 zero heights) => 4 cells, 8 triangles, 24 corners.
static std::shared_ptr<X3DNode> makeGrid() {
  auto g = createX3DNode("ElevationGrid");
  setF(g, "xDimension", std::any(3));
  setF(g, "zDimension", std::any(3));
  setF(g, "xSpacing", std::any(1.0f));
  setF(g, "zSpacing", std::any(1.0f));
  setF(g, "height", std::any(std::vector<float>(9, 0.0f)));
  return g;
}

static int latticeOf(int i, int j) { return j * 3 + i; }

TEST_CASE("ext001_elevationgrid_authored_normal_per_vertex") {
  auto g = makeGrid();
  std::vector<SFVec3f> nrm(9);
  for (int j = 0; j < 3; ++j)
    for (int i = 0; i < 3; ++i)
      nrm[latticeOf(i, j)] = SFVec3f{static_cast<float>(i),
                                     static_cast<float>(j), 1.0f};
  attachNormal(g, nrm);
  setF(g, "normalPerVertex", std::any(true));

  MeshData m = buildLocalMesh(g.get());
  REQUIRE((!m.normals.empty()));
  REQUIRE((m.latticeIndex.size() == m.normals.size()));
  // Every emitted corner normal must equal the AUTHORED normal of its lattice
  // vertex (a generated flat grid normal would be +Y here).
  for (std::size_t c = 0; c < m.normals.size(); ++c)
    CHECK(vecEq(m.normals[c], nrm[m.latticeIndex[c]]));
}

TEST_CASE("ext001_elevationgrid_authored_normal_per_quad") {
  auto g = makeGrid();
  std::vector<SFVec3f> nrm = {{0, 0, 1}, {0, 0, -1}, {1, 0, 0}, {-1, 0, 0}};
  attachNormal(g, nrm);
  setF(g, "normalPerVertex", std::any(false));

  MeshData m = buildLocalMesh(g.get());
  REQUIRE((!m.normals.empty()));
  // Emitted cell order is (0,0),(1,0),(0,1),(1,1) -> 2 triangles (6 corners)
  // per cell, so every corner normal equals its CELL's entry.
  for (std::size_t c = 0; c < m.normals.size(); ++c)
    CHECK(vecEq(m.normals[c], nrm[c / 6]));
}

TEST_CASE("ext001_elevationgrid_authored_color_per_vertex") {
  auto g = makeGrid();
  std::vector<SFColor> col(9);
  for (int j = 0; j < 3; ++j)
    for (int i = 0; i < 3; ++i)
      col[latticeOf(i, j)] = SFColor{static_cast<float>(i) * 0.5f,
                                     static_cast<float>(j) * 0.5f, 0.25f};
  attachColor(g, col);
  setF(g, "colorPerVertex", std::any(true));

  MeshData m = buildLocalMesh(g.get());
  CHECK((m.hasColors));
  REQUIRE((m.colors.size() == m.positions.size()));
  for (std::size_t c = 0; c < m.colors.size(); ++c) {
    const SFColor &src = col[m.latticeIndex[c]];
    CHECK(colEq(m.colors[c], SFColorRGBA{src.r, src.g, src.b, 1.0f}));
  }
}

TEST_CASE("ext001_elevationgrid_authored_color_per_quad") {
  auto g = makeGrid();
  std::vector<SFColor> col = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 1, 1}};
  attachColor(g, col);
  setF(g, "colorPerVertex", std::any(false));

  MeshData m = buildLocalMesh(g.get());
  CHECK((m.hasColors));
  REQUIRE((m.colors.size() == m.positions.size()));
  for (std::size_t c = 0; c < m.colors.size(); ++c) {
    CHECK(colEq(m.colors[c], SFColorRGBA{col[c / 6].r, col[c / 6].g,
                                         col[c / 6].b, 1.0f}));
  }
}

TEST_CASE("ext001_geoelevationgrid_authored_color_per_vertex") {
  auto g = createX3DNode("GeoElevationGrid");
  setF(g, "xDimension", std::any(3));
  setF(g, "zDimension", std::any(3));
  setF(g, "xSpacing", std::any(1.0));
  setF(g, "zSpacing", std::any(1.0));
  setF(g, "height", std::any(std::vector<double>(9, 0.0)));
  std::vector<SFColor> col(9);
  for (int j = 0; j < 3; ++j)
    for (int i = 0; i < 3; ++i)
      col[latticeOf(i, j)] = SFColor{0.1f * i, 0.1f * j, 0.5f};
  attachColor(g, col);

  MeshData m = buildLocalMesh(g.get());
  CHECK((m.hasColors));
  REQUIRE((!m.colors.empty()));
  REQUIRE((m.colors.size() == m.positions.size()));
  for (std::size_t c = 0; c < m.colors.size(); ++c) {
    const SFColor &src = col[m.latticeIndex[c]];
    CHECK(colEq(m.colors[c], SFColorRGBA{src.r, src.g, src.b, 1.0f}));
  }
}
