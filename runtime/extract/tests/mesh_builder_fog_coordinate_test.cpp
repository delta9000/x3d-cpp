// mesh_builder_fog_coordinate_test.cpp — REQ-LOCALFOG: §24.4.3 FogCoordinate.
//
// "One depth value per vertex shall be supplied. If the user does not provide a
// sufficient number of depth values, the last value defined shall be replicated
// for any further vertices. If too many depth values are supplied, the excess
// depth values shall be ignored." MeshData::fogDepths carries the authored
// depth of each emitted corner: the depth of its Coordinate point (composed,
// line and point sets) or of its height-grid vertex (ElevationGrid). Before the
// fix the fogCoord field was never read and fogDepths did not exist.
#include "GeoFrame.hpp"
#include "MeshBuilder.hpp"
#include "SceneExtractor.hpp"

#include "X3DExecutionContext.hpp"
#include "X3DScene.hpp"
#include "x3d/nodes/X3DNode.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"

#include <any>
#include "doctest/doctest.h"
#include <memory>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::runtime::extract;
using namespace x3d::core;
using namespace x3d::nodes;

namespace {

void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) {
      f.set(*n, std::move(v));
      return;
    }
}

std::shared_ptr<X3DNode> fogCoordinate(std::vector<float> depth) {
  auto n = createX3DNode("FogCoordinate");
  setF(n, "depth", std::any(std::move(depth)));
  return n;
}

// Four points, two triangles through coordIndex 0 1 2 -1 0 2 3.
std::shared_ptr<X3DNode> quad() {
  auto coord = createX3DNode("Coordinate");
  setF(coord, "point",
       std::any(std::vector<SFVec3f>{{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}}));
  auto ifs = createX3DNode("IndexedFaceSet");
  setF(ifs, "coord", std::any(coord));
  setF(ifs, "coordIndex", std::any(std::vector<int>{0, 1, 2, -1, 0, 2, 3}));
  return ifs;
}

// The authored depth each corner should carry, by its source point.
void checkDepths(const MeshData &m, const std::vector<float> &byPoint) {
  REQUIRE(m.fogDepths.size() == m.positions.size());
  REQUIRE(m.sourceCoordIndex.size() == m.positions.size());
  for (std::size_t i = 0; i < m.positions.size(); ++i)
    CHECK(m.fogDepths[i] == doctest::Approx(byPoint[m.sourceCoordIndex[i]]));
}

} // namespace

TEST_CASE("FogCoordinate depths follow coordIndex onto every corner") {
  auto ifs = quad();
  CHECK(buildLocalMesh(ifs.get(), geo::builtinProjection()).fogDepths.empty());

  setF(ifs, "fogCoord", std::any(fogCoordinate({0.0f, 0.25f, 0.5f, 1.0f})));
  checkDepths(buildLocalMesh(ifs.get(), geo::builtinProjection()),
              {0.0f, 0.25f, 0.5f, 1.0f});
}

TEST_CASE("A short depth list repeats its last value; extra values are ignored") {
  auto ifs = quad();
  setF(ifs, "fogCoord", std::any(fogCoordinate({0.75f, 0.5f})));
  checkDepths(buildLocalMesh(ifs.get(), geo::builtinProjection()),
              {0.75f, 0.5f, 0.5f, 0.5f});

  setF(ifs, "fogCoord", std::any(fogCoordinate({0.1f, 0.2f, 0.3f, 0.4f, 0.9f, 0.9f})));
  checkDepths(buildLocalMesh(ifs.get(), geo::builtinProjection()),
              {0.1f, 0.2f, 0.3f, 0.4f});

  // Depths outside the field's [0,1] range are clamped.
  setF(ifs, "fogCoord", std::any(fogCoordinate({-1.0f, 2.0f, 0.5f, 0.5f})));
  checkDepths(buildLocalMesh(ifs.get(), geo::builtinProjection()),
              {0.0f, 1.0f, 0.5f, 0.5f});

  // An empty depth list means no authored depths.
  setF(ifs, "fogCoord", std::any(fogCoordinate({})));
  CHECK(buildLocalMesh(ifs.get(), geo::builtinProjection()).fogDepths.empty());
}

TEST_CASE("Line sets and ElevationGrid carry FogCoordinate depths") {
  auto coord = createX3DNode("Coordinate");
  setF(coord, "point", std::any(std::vector<SFVec3f>{{0, 0, 0}, {1, 0, 0}, {2, 0, 0}}));
  auto lines = createX3DNode("IndexedLineSet");
  setF(lines, "coord", std::any(coord));
  setF(lines, "coordIndex", std::any(std::vector<int>{2, 1, 0}));
  setF(lines, "fogCoord", std::any(fogCoordinate({0.2f, 0.4f, 0.6f})));
  checkDepths(buildLocalMesh(lines.get(), geo::builtinProjection()), {0.2f, 0.4f, 0.6f});

  // A 2x2 grid: depth i belongs to height i (lattice id row * xDimension + col).
  auto grid = createX3DNode("ElevationGrid");
  setF(grid, "xDimension", std::any(2));
  setF(grid, "zDimension", std::any(2));
  setF(grid, "height", std::any(std::vector<float>(4, 0.0f)));
  setF(grid, "fogCoord", std::any(fogCoordinate({0.1f, 0.2f, 0.3f, 0.4f})));
  const MeshData m = buildLocalMesh(grid.get(), geo::builtinProjection());
  REQUIRE(m.fogDepths.size() == m.positions.size());
  REQUIRE(m.latticeIndex.size() == m.positions.size());
  for (std::size_t i = 0; i < m.positions.size(); ++i)
    CHECK(m.fogDepths[i] == doctest::Approx(0.1f * (m.latticeIndex[i] + 1)));
}

TEST_CASE("A FogCoordinate depth event rebuilds the geometry") {
  auto ifs = quad();
  auto fog = fogCoordinate({0.0f, 0.0f, 0.0f, 0.0f});
  setF(ifs, "fogCoord", std::any(fog));
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", std::any(ifs));
  Scene scene;
  scene.rootNodes.push_back(shape);
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  SceneExtractor ex(ctx, scene);
  const auto snapshot = ex.fullSnapshot();
  REQUIRE(snapshot.added.size() == 1);
  CHECK(ex.item(snapshot.added[0]).mesh->fogDepths.front() == 0.0f);

  ctx.postEvent(fog.get(), "depth", MFFloat{0.5f});
  ctx.tick(1);
  const auto delta = ex.delta();
  REQUIRE(delta.updatedGeometry.size() == 1);
  for (float d : ex.item(snapshot.added[0]).mesh->fogDepths)
    CHECK(d == doctest::Approx(0.5f));
}
