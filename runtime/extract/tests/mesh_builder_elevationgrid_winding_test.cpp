// mesh_builder_elevationgrid_winding_test.cpp — regression for the ElevationGrid
// / GeoElevationGrid surface winding.
//
// ISO/IEC 19775-1 §13.3.4: with the default ccw=TRUE the generated faces are
// wound so the surface is front-facing (and its generated normal points) toward
// the +Y side — i.e. a height grid is visible/lit from ABOVE by default. A
// reversed winding makes the generated normals point -Y, which (a) lights the
// top from below and (b) makes the whole surface back-facing, so a consumer's
// default back-face cull (solid=TRUE) erases the terrain when viewed from above.
// That was the Kelp Forest RockFloor "holes" regression.
#include "MeshBuilder.hpp"

#include "X3DNodeFactory.hpp"

#include <any>
#include "doctest/doctest.h"
#include <memory>
#include <string>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::runtime::extract;

static void setWindF(const std::shared_ptr<X3DNode> &n, const char *nm,
                     std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) {
      f.set(*n, std::move(v));
      return;
    }
}

TEST_CASE("elevationgrid_default_ccw_surface_faces_up") {
  // A flat 3x3 ElevationGrid (all heights 0), ccw left at its spec default TRUE.
  auto g = createX3DNode("ElevationGrid");
  setWindF(g, "xDimension", std::any(3));
  setWindF(g, "zDimension", std::any(3));
  setWindF(g, "xSpacing", std::any(1.0f));
  setWindF(g, "zSpacing", std::any(1.0f));
  setWindF(g, "height", std::any(std::vector<float>(9, 0.0f)));

  MeshData m = buildLocalMesh(g.get());
  REQUIRE((!m.normals.empty()));
  // For a flat grid every generated normal must point UP (+Y). A reversed
  // winding yields n.y < 0 (the bug).
  for (const SFVec3f &n : m.normals)
    CHECK((n.y > 0.5f));
}
