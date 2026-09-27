#include "MeshBuilder.hpp"
#include "GeometryBounds.hpp"
#include "GeoNodes.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "doctest/doctest.h"
#include <any>
#include <cmath>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;
using namespace x3d::runtime::extract;

static void setGeoField(const std::shared_ptr<X3DNode> &n, const char *name, std::any value) {
  for (auto &f : n->fields())
    if (f.x3dName == name && f.set) { f.set(*n, std::move(value)); return; }
}

static std::shared_ptr<X3DNode> localOrigin() {
  auto origin = createX3DNode("GeoOrigin");
  setGeoField(origin, "geoCoords", SFVec3d{0,0,0});
  setGeoField(origin, "rotateYUp", true);
  return origin;
}

TEST_CASE("GeoCoordinate IFS uses node geoSystem and GeoOrigin; CoordinateDouble stays Cartesian") {
  auto origin = localOrigin();
  auto coord = createX3DNode("GeoCoordinate");
  setGeoField(coord, "geoOrigin", origin);
  setGeoField(coord, "point", std::vector<SFVec3d>{{0,0,0}, {0,0.00001,0}, {0.00001,0,0}});
  auto ifs = createX3DNode("IndexedFaceSet");
  setGeoField(ifs, "coord", coord);
  setGeoField(ifs, "coordIndex", std::vector<int>{0,1,2,-1});
  MeshData mesh = buildLocalMesh(ifs.get());
  REQUIRE(mesh.positions.size() == 3);
  CHECK(mesh.positions[0].x == doctest::Approx(0).epsilon(0.01));
  CHECK(mesh.positions[1].x == doctest::Approx(1.1132).epsilon(0.01));
  CHECK(mesh.positions[2].z == doctest::Approx(-1.1057).epsilon(0.01));
  Aabb bounds = localGeometryBounds(ifs.get());
  CHECK(bounds.max.x == doctest::Approx(1.1132).epsilon(0.01));
  CHECK(bounds.min.z == doctest::Approx(-1.1057).epsilon(0.01));

  auto plain = createX3DNode("CoordinateDouble");
  setGeoField(plain, "point", std::vector<SFVec3d>{{2,3,4}});
  auto points = geombounds::getPointsLenient(*plain, "point");
  REQUIRE(points.size() == 1);
  CHECK(points[0].x == 2);
  CHECK(points[0].y == 3);
}

TEST_CASE("GeoElevationGrid uses geographic lattice, elevation, tangent normals, and bounds") {
  auto grid = createX3DNode("GeoElevationGrid");
  setGeoField(grid, "geoOrigin", localOrigin());
  setGeoField(grid, "xDimension", 2);
  setGeoField(grid, "zDimension", 2);
  setGeoField(grid, "xSpacing", 0.00001);
  setGeoField(grid, "zSpacing", 0.00001);
  setGeoField(grid, "yScale", 2.0f);
  setGeoField(grid, "height", std::vector<double>(4, 0));
  MeshData flat = buildLocalMesh(grid.get());
  REQUIRE(flat.normals.size() == 6);
  for (const auto &normal : flat.normals) CHECK(normal.y > 0.99f);
  setGeoField(grid, "height", std::vector<double>{0,0,0,5});
  MeshData mesh = buildLocalMesh(grid.get());
  REQUIRE(mesh.positions.size() == 6);
  CHECK(mesh.latticeIndex.size() == 6);
  CHECK(mesh.texcoords.size() == 6);
  bool foundRaised = false;
  for (std::size_t i = 0; i < mesh.positions.size(); ++i) {
    if (mesh.latticeIndex[i] == 3) {
      CHECK(mesh.positions[i].x == doctest::Approx(1.1132).epsilon(0.01));
      CHECK(mesh.positions[i].y == doctest::Approx(10).epsilon(0.01));
      CHECK(mesh.positions[i].z == doctest::Approx(-1.1057).epsilon(0.01));
      foundRaised = true;
    }
  }
  CHECK(foundRaised);
  Aabb bound = localGeometryBounds(grid.get());
  CHECK(bound.max.y == doctest::Approx(10).epsilon(0.01));
  CHECK(bound.max.x == doctest::Approx(1.1132).epsilon(0.01));

  auto normal = createX3DNode("Normal");
  setGeoField(normal, "vector", std::vector<SFVec3f>(4, {0,1,0}));
  setGeoField(grid, "normal", normal);
  MeshData lit = buildLocalMesh(grid.get());
  REQUIRE(lit.normals.size() == 6);
  for (const auto &n : lit.normals) CHECK(n.y == doctest::Approx(1).epsilon(0.001));
  setGeoField(grid, "geoGridOrigin", SFVec3d{0,1,0});
  setGeoField(normal, "vector", std::vector<SFVec3f>(4, {1,0,0}));
  MeshData angled = buildLocalMesh(grid.get());
  REQUIRE(angled.normals.size() == 6);
  CHECK(angled.normals[0].y == doctest::Approx(-0.0174524).epsilon(0.01));
}

TEST_CASE("GeoElevationGrid UTM spacing advances eastings and northings") {
  auto grid = createX3DNode("GeoElevationGrid");
  setGeoField(grid, "geoSystem", std::vector<std::string>{"UTM","Z31"});
  setGeoField(grid, "geoGridOrigin", SFVec3d{0,500000,0});
  setGeoField(grid, "xDimension", 2);
  setGeoField(grid, "zDimension", 2);
  setGeoField(grid, "xSpacing", 2.0);
  setGeoField(grid, "zSpacing", 3.0);
  setGeoField(grid, "height", std::vector<double>(4, 0));
  const auto a = geo::gridCoordinate(*grid, 1, 1, 0);
  CHECK(a.x == 3);
  CHECK(a.y == 500002);
  MeshData mesh = buildLocalMesh(grid.get());
  CHECK(mesh.positions.size() == 6);
}

TEST_CASE("height grid lattice indices and degenerate guards remain intact") {
  auto planar = createX3DNode("ElevationGrid");
  setGeoField(planar, "xDimension", 3);
  setGeoField(planar, "zDimension", 2);
  setGeoField(planar, "height", std::vector<float>(6, 0));
  MeshData plain = buildLocalMesh(planar.get());
  CHECK(plain.indices.size() == 12);
  CHECK(plain.latticeIndex.size() == plain.positions.size());
  for (auto id : plain.latticeIndex) CHECK(id < 6u);

  auto geoGrid = createX3DNode("GeoElevationGrid");
  setGeoField(geoGrid, "xDimension", 2);
  setGeoField(geoGrid, "zDimension", 2);
  setGeoField(geoGrid, "height", std::vector<double>{0});
  CHECK(buildLocalMesh(geoGrid.get()).positions.empty());
  setGeoField(geoGrid, "xDimension", 1);
  setGeoField(geoGrid, "zDimension", 1);
  CHECK(buildLocalMesh(geoGrid.get()).indices.empty());
}
