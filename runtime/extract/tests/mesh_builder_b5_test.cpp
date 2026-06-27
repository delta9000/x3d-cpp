// mesh_builder_b5_test.cpp — Browser-level B5 acceptance: GeoElevationGrid
// extracts geometry via the FLAT-FALLBACK, the lattice-index-retaining form is
// preserved on both grid types, and the GeoProjection embedder seam (threaded
// via MeshBuildOptions) is honored when wired.
//
// Proofs:
//   1) GeoElevationGrid with NO projection wired -> flat-fallback emits
//      (xDim-1)*(zDim-1)*2 triangles (the single biggest corpus unlock: a
//      terrain tile goes 0 -> >0 items). recognizedGeometryType("GeoElevation
//      Grid") is now true.
//   2) Flat-fallback geometry: elevation (yScale-applied) lands on +Y; the
//      planar XZ layout steps by xSpacing/zSpacing offset by geoGridOrigin.
//   3) LATTICE-INDEX-RETAINING form: MeshData.latticeIndex is parallel to
//      positions and carries row*xDim+col per corner (what B6 needs). The
//      existing ElevationGrid emitter retains it the same way.
//   4) GeoProjection SEAM: a wired projection std::function is invoked per
//      lattice vertex and its returned LOCAL position is what lands in the mesh
//      (the SDK does no geodesy itself); the GeoSystemDesc carries the raw
//      geoSystem + geoGridOrigin verbatim.
//   5) Degenerate/short-height grids are guarded (empty mesh, no OOB read).
#include "MeshBuilder.hpp"

#include "x3d/nodes/X3DNodeFactory.hpp"

#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
#include <string>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::runtime::extract;

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}

static bool posEq(const MeshData &m, std::size_t i, const SFVec3f &p) {
  return feq(m.positions[i].x, p.x) && feq(m.positions[i].y, p.y) &&
         feq(m.positions[i].z, p.z);
}

TEST_CASE("mesh_builder_b5_test") {
  // recognizedGeometryType now lists GeoElevationGrid.
  CHECK((recognizedGeometryType("GeoElevationGrid")));

  // ---- 1+2. GeoElevationGrid flat-fallback: 2x2 grid -> 1 cell -> 2 tris ----
  {
    auto g = createX3DNode("GeoElevationGrid");
    setF(g, "xDimension", std::any(2));
    setF(g, "zDimension", std::any(2));
    setF(g, "xSpacing", std::any(2.0));       // SFDouble
    setF(g, "zSpacing", std::any(3.0));       // SFDouble
    setF(g, "yScale", std::any(1.0f));        // SFFloat
    setF(g, "geoSystem", std::any(std::vector<std::string>{"GD"}));
    setF(g, "geoGridOrigin", std::any(SFVec3d{0.0, 0.0, 0.0}));
    // height MFDouble, row-major (col + row*xDim). One raised corner.
    setF(g, "height", std::any(std::vector<double>{0.0, 0.0, 0.0, 5.0}));

    MeshData m = buildLocalMesh(g.get()); // NO projection -> flat-fallback.
    CHECK((m.indices.size() == 6)); // (2-1)*(2-1)=1 cell -> 2 tris.
    CHECK((!m.positions.empty())); // the corpus unlock: 0 -> >0.

    // Flat-fallback: X=col*xSpacing, Y=elevation, Z=row*zSpacing.
    // Raised corner (i=1,j=1) -> (2, 5, 3).
    bool sawCorner = false, sawOrigin = false;
    for (std::size_t i = 0; i < m.positions.size(); ++i) {
      if (posEq(m, i, SFVec3f{2.0f, 5.0f, 3.0f})) sawCorner = true;
      if (posEq(m, i, SFVec3f{0.0f, 0.0f, 0.0f})) sawOrigin = true;
    }
    CHECK((sawCorner && sawOrigin));

    // ---- 3. lattice-index-retaining form ----------------------------------
    CHECK((m.latticeIndex.size() == m.positions.size()));
    // Every corner's latticeIndex < xDim*zDim, and the corner whose position is
    // the raised (1,1) vertex must carry lattice id 1*2+1 = 3.
    for (std::size_t i = 0; i < m.positions.size(); ++i) {
      CHECK((m.latticeIndex[i] < 4u));
      if (posEq(m, i, SFVec3f{2.0f, 5.0f, 3.0f})) CHECK((m.latticeIndex[i] == 3u));
      if (posEq(m, i, SFVec3f{0.0f, 0.0f, 0.0f})) CHECK((m.latticeIndex[i] == 0u));
    }
  }

  // ---- 2b. yScale scales the elevation -------------------------------------
  {
    auto g = createX3DNode("GeoElevationGrid");
    setF(g, "xDimension", std::any(2));
    setF(g, "zDimension", std::any(2));
    setF(g, "xSpacing", std::any(1.0));
    setF(g, "zSpacing", std::any(1.0));
    setF(g, "yScale", std::any(10.0f));
    setF(g, "height", std::any(std::vector<double>{0.0, 0.0, 0.0, 2.0}));
    MeshData m = buildLocalMesh(g.get());
    bool sawScaled = false;
    for (std::size_t i = 0; i < m.positions.size(); ++i)
      if (feq(m.positions[i].y, 20.0f)) sawScaled = true; // 2 * yScale 10.
    CHECK((sawScaled));
  }

  // ---- 4. GeoProjection seam is invoked + GeoSystemDesc carried verbatim ----
  {
    auto g = createX3DNode("GeoElevationGrid");
    setF(g, "xDimension", std::any(2));
    setF(g, "zDimension", std::any(2));
    setF(g, "xSpacing", std::any(1.0));
    setF(g, "zSpacing", std::any(1.0));
    setF(g, "geoSystem", std::any(std::vector<std::string>{"UTM", "Z17"}));
    setF(g, "geoGridOrigin", std::any(SFVec3d{100.0, 200.0, 0.0}));
    setF(g, "height", std::any(std::vector<double>{1.0, 1.0, 1.0, 1.0}));

    int calls = 0;
    bool sawSystem = false, sawOrigin = false;
    MeshBuildOptions opt;
    opt.geoProjection = [&](const SFVec3d &geoCoord, double elevation,
                            const GeoSystemDesc &sys) -> SFVec3f {
      ++calls;
      if (sys.geoSystem.size() == 2 && sys.geoSystem[0] == "UTM" &&
          sys.geoSystem[1] == "Z17")
        sawSystem = true;
      if (feq(static_cast<float>(sys.geoGridOrigin.x), 100.0f) &&
          feq(static_cast<float>(sys.geoGridOrigin.y), 200.0f))
        sawOrigin = true;
      // Map to a recognizable local frame: scale elevation onto +Y, sentinel X.
      return SFVec3f{42.0f, static_cast<float>(elevation), 7.0f};
      (void)geoCoord;
    };

    MeshData m = buildLocalMesh(g.get(), opt);
    CHECK((m.indices.size() == 6));
    CHECK((calls > 0 && sawSystem && sawOrigin));
    // Every emitted position came from the projection (sentinel X=42, Z=7).
    for (std::size_t i = 0; i < m.positions.size(); ++i)
      CHECK((feq(m.positions[i].x, 42.0f) && feq(m.positions[i].z, 7.0f) &&
             feq(m.positions[i].y, 1.0f)));
  }

  // ---- 3b. ElevationGrid ALSO retains its lattice map (refactor parity) -----
  {
    auto g = createX3DNode("ElevationGrid");
    setF(g, "xDimension", std::any(3));
    setF(g, "zDimension", std::any(2));
    setF(g, "xSpacing", std::any(1.0f));
    setF(g, "zSpacing", std::any(1.0f));
    setF(g, "height", std::any(std::vector<float>(6, 0.0f)));
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.indices.size() == 12)); // (3-1)*(2-1)=2 cells -> 4 tris.
    CHECK((m.latticeIndex.size() == m.positions.size()));
    for (std::uint32_t lid : m.latticeIndex) CHECK((lid < 6u)); // < xDim*zDim.
  }

  // ---- 5. Degenerate / short-height guards ---------------------------------
  {
    auto g = createX3DNode("GeoElevationGrid");
    setF(g, "xDimension", std::any(2));
    setF(g, "zDimension", std::any(2));
    setF(g, "height", std::any(std::vector<double>{0.0})); // need 4.
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.positions.empty() && m.indices.empty()));
  }
  {
    auto g = createX3DNode("GeoElevationGrid");
    setF(g, "xDimension", std::any(1)); // degenerate dim.
    setF(g, "zDimension", std::any(1));
    setF(g, "height", std::any(std::vector<double>{0.0}));
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.indices.empty()));
  }

  return;
}
