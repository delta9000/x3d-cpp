// geometry_bounds_test.cpp
#include "GeometryBounds.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
#include <vector>
using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;
static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }
static void setF(const std::shared_ptr<X3DNode>& n, const char* name, std::any v) {
  for (auto& f : n->fields()) if (f.x3dName == name && f.set) { f.set(*n, std::move(v)); return; }
}

TEST_CASE("geometry_bounds_test") {
  auto box = createX3DNode("Box");
  setF(box, "size", std::any(SFVec3f{2,4,6}));
  Aabb b = localGeometryBounds(box.get());
  CHECK((feq(b.min.x,-1) && feq(b.max.y,2) && feq(b.max.z,3)));

  auto sph = createX3DNode("Sphere");
  setF(sph, "radius", std::any(2.5f));
  Aabb s = localGeometryBounds(sph.get());
  CHECK((feq(s.min.x,-2.5f) && feq(s.max.z,2.5f)));

  auto cyl = createX3DNode("Cylinder");
  setF(cyl, "radius", std::any(1.0f)); setF(cyl, "height", std::any(10.0f));
  Aabb c = localGeometryBounds(cyl.get());
  CHECK((feq(c.min.x,-1) && feq(c.max.y,5) && feq(c.min.y,-5)));

  // IndexedFaceSet with a Coordinate -> AABB over points
  auto ifs = createX3DNode("IndexedFaceSet");
  auto coord = createX3DNode("Coordinate");
  setF(coord, "point", std::any(std::vector<SFVec3f>{{0,0,0},{1,2,3},{-1,-1,0}}));
  setF(ifs, "coord", std::any(std::shared_ptr<X3DNode>(coord)));
  Aabb m = localGeometryBounds(ifs.get());
  CHECK((feq(m.min.x,-1) && feq(m.min.y,-1) && feq(m.max.y,2) && feq(m.max.z,3)));

  // unknown / empty geometry -> empty
  auto txt = createX3DNode("WorldInfo"); // not geometry
  CHECK((localGeometryBounds(txt.get()).empty));

  // ElevationGrid: x in [0,(xDim-1)*xSp], z in [0,(zDim-1)*zSp], y over height.
  auto eg = createX3DNode("ElevationGrid");
  setF(eg, "xDimension", std::any(3)); setF(eg, "zDimension", std::any(2));
  setF(eg, "xSpacing", std::any(2.0f)); setF(eg, "zSpacing", std::any(5.0f));
  setF(eg, "height", std::any(std::vector<float>{0,1,2,3,4,5}));
  Aabb g = localGeometryBounds(eg.get());
  CHECK((feq(g.min.x,0) && feq(g.max.x,4) && feq(g.max.z,5) && feq(g.min.y,0) && feq(g.max.y,5)));

  // Extrusion: conservative spine±maxSectionRadius. Square section radius ~1.414,
  // spine along y from 0..10.
  auto ex = createX3DNode("Extrusion");
  setF(ex, "crossSection", std::any(std::vector<SFVec2f>{{1,1},{-1,1},{-1,-1},{1,-1},{1,1}}));
  setF(ex, "spine", std::any(std::vector<SFVec3f>{{0,0,0},{0,10,0}}));
  Aabb x = localGeometryBounds(ex.get());
  CHECK((!x.empty && x.max.y >= 10.0f && x.max.x >= 1.0f && x.min.x <= -1.0f));

  // Text: non-empty, and shrinks to maxExtent when set.
  auto te = createX3DNode("Text");
  setF(te, "string", std::any(std::vector<std::string>{"hello","world"}));
  Aabb tb = localGeometryBounds(te.get());
  CHECK((!tb.empty));
  setF(te, "maxExtent", std::any(3.0f));
  Aabb tb2 = localGeometryBounds(te.get());
  CHECK((tb2.size().x <= 6.01f)); // width capped by 2*maxExtent (symmetric box)

  // GEO-2 sibling: a GeoCoordinate coord (MFVec3d) must yield real bounds, not
  // an empty AABB (it was read as MFVec3f and silently dropped).
  auto gifs = createX3DNode("IndexedFaceSet");
  auto gcoord = createX3DNode("GeoCoordinate");
  setF(gcoord, "point",
       std::any(std::vector<SFVec3d>{{0, 0, 0}, {2, 4, 6}, {-1, -1, 0}}));
  setF(gifs, "coord", std::any(std::shared_ptr<X3DNode>(gcoord)));
  Aabb gm = localGeometryBounds(gifs.get());
  CHECK((feq(gm.min.x, -1) && feq(gm.max.y, 4) && feq(gm.max.z, 6)));

  // GeoElevationGrid (SFDouble spacing, MFDouble height): bounds from the
  // double-precision grid, not the float default.
  auto geg = createX3DNode("GeoElevationGrid");
  setF(geg, "xDimension", std::any(3));
  setF(geg, "zDimension", std::any(2));
  setF(geg, "xSpacing", std::any(SFDouble{2.0}));
  setF(geg, "zSpacing", std::any(SFDouble{5.0}));
  setF(geg, "height", std::any(MFDouble{0, 1, 2, 3, 4, 5}));
  Aabb gg = localGeometryBounds(geg.get());
  CHECK((feq(gg.max.x, 4) && feq(gg.max.z, 5) && feq(gg.max.y, 5)));

  // NurbsCurve / NurbsPatchSurface: AABB over control points (convex hull).
  {
    auto cc = createX3DNode("Coordinate");
    setF(cc, "point", std::any(std::vector<SFVec3f>{{-2,0,0},{0,5,0},{3,0,1}}));
    auto nc = createX3DNode("NurbsCurve");
    setF(nc, "controlPoint", std::any(std::shared_ptr<X3DNode>(cc)));
    Aabb b = localGeometryBounds(nc.get());
    CHECK((feq(b.min.x,-2) && feq(b.max.x,3) && feq(b.max.y,5) && feq(b.max.z,1)));

    auto cs = createX3DNode("Coordinate");
    setF(cs, "point", std::any(std::vector<SFVec3f>{{0,0,0},{1,0,0},{0,1,4}}));
    auto np = createX3DNode("NurbsPatchSurface");
    setF(np, "controlPoint", std::any(std::shared_ptr<X3DNode>(cs)));
    Aabb p = localGeometryBounds(np.get());
    CHECK((feq(p.min.x,0) && feq(p.max.x,1) && feq(p.max.y,1) && feq(p.max.z,4)));
  }
  return;
}
