// scene_extractor_b2_test.cpp — Browser-level B2 acceptance: unsupported-geometry
// signal. The silent empty-mesh drop in SceneExtractor::walk() is replaced by an
// observable pull channel, skippedGeometryCounts() -> map<nodeTypeName,count>.
//
// Proves the load-bearing DISTINCTION:
//   1) A Shape over a NurbsPatchSurface (an UNRECOGNIZED geometry type, still
//      unsupported) is dropped AND increments
//      skippedGeometryCounts()["NurbsPatchSurface"]. (Extrusion became a
//      RECOGNIZED, drawable type at B3 and Text at T-TEXT, so the canonical
//      "unsupported" example moved to NurbsPatchSurface.)
//   2) A Shape over an empty-coordIndex IndexedFaceSet (a RECOGNIZED type that
//      legitimately tessellates to nothing) is dropped but does NOT appear in the
//      map — legitimate emptiness is not a coverage gap.
//   3) A supported Shape (TriangleSet) still emits and is never counted.
//   4) fullSnapshot() recounts from scratch (the map is cleared each full walk).
#include "SceneExtractor.hpp"

#include "MeshBuilder.hpp" // extract::recognizedGeometryType (the static oracle).
#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DNodeFactory.hpp"
#include "X3DScene.hpp"

#include <any>
#include "doctest/doctest.h"
#include <cstdint>
#include <memory>
#include <vector>

using namespace x3d::runtime;

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) {
      f.set(*n, std::move(v));
      return;
    }
}
static void addChild(const std::shared_ptr<X3DNode> &p,
                     const std::shared_ptr<X3DNode> &c) {
  for (auto &f : p->fields())
    if (f.x3dName == "children" && f.set) {
      auto k = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*p));
      k.push_back(c);
      f.set(*p, std::any(std::move(k)));
      return;
    }
}

// A geometry-bearing Shape over a one-triangle TriangleSet (supported).
static std::shared_ptr<X3DNode> makeTriShape() {
  auto coord = createX3DNode("Coordinate");
  setF(coord, "point",
       std::any(std::vector<SFVec3f>{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}));
  auto tri = createX3DNode("TriangleSet");
  setF(tri, "coord", std::any(std::shared_ptr<X3DNode>(coord)));
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(tri)));
  return shape;
}

// A Shape over a NurbsPatchSurface — an UNRECOGNIZED geometry type (no
// MeshBuilder arm). Extrusion became recognized + drawable at B3 and Text at
// T-TEXT, so NurbsPatchSurface is now the canonical still-unsupported example.
static std::shared_ptr<X3DNode> makeUnsupportedShape() {
  auto ex = createX3DNode("NurbsPatchSurface");
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(ex)));
  return shape;
}

// A Shape over an IndexedFaceSet with a coord node but EMPTY coordIndex — a
// RECOGNIZED type that legitimately produces no triangles.
static std::shared_ptr<X3DNode> makeEmptyIfsShape() {
  auto coord = createX3DNode("Coordinate");
  setF(coord, "point",
       std::any(std::vector<SFVec3f>{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}));
  auto ifs = createX3DNode("IndexedFaceSet");
  setF(ifs, "coord", std::any(std::shared_ptr<X3DNode>(coord)));
  // coordIndex left empty -> zero faces -> empty mesh, but RECOGNIZED.
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(ifs)));
  return shape;
}

TEST_CASE("scene_extractor_b2_test") {
  // === 0) The static oracle classifies types correctly =======================
  CHECK((extract::recognizedGeometryType("TriangleSet")));
  CHECK((extract::recognizedGeometryType("IndexedFaceSet")));
  CHECK((extract::recognizedGeometryType("Box")));
  CHECK((extract::recognizedGeometryType("ElevationGrid")));
  CHECK((extract::recognizedGeometryType("Extrusion"))); // recognized at B3.
  CHECK((extract::recognizedGeometryType("Text")));       // recognized at T-TEXT.
  CHECK((!extract::recognizedGeometryType("NurbsPatchSurface")));

  // buildLocalMesh sets `recognized` per the same oracle, independent of emptiness.
  {
    bool rec = true;
    auto ex = createX3DNode("NurbsPatchSurface");
    auto m = extract::buildLocalMesh(ex.get(), extract::MeshBuildOptions{}, &rec);
    CHECK((m.indices.empty() && !rec)); // unrecognized -> empty + recognized=false.

    auto coord = createX3DNode("Coordinate");
    setF(coord, "point", std::any(std::vector<SFVec3f>{{0, 0, 0}}));
    auto ifs = createX3DNode("IndexedFaceSet");
    setF(ifs, "coord", std::any(std::shared_ptr<X3DNode>(coord)));
    rec = false;
    auto m2 = extract::buildLocalMesh(ifs.get(), extract::MeshBuildOptions{}, &rec);
    CHECK((m2.indices.empty() && rec)); // recognized-but-empty -> recognized=true.
  }

  // === 1) NurbsPatchSurface increments the count; empty IFS does NOT ========
  {
    auto root = createX3DNode("Group");
    addChild(root, makeUnsupportedShape()); // unsupported -> counted.
    addChild(root, makeEmptyIfsShape());    // legitimately empty -> NOT counted.
    addChild(root, makeTriShape());         // supported -> emitted, never counted.

    Scene scene;
    scene.addRootNode(root);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);
    auto snap = ex.fullSnapshot();

    // Exactly the one TriangleSet emits.
    CHECK((snap.added.size() == 1));

    const auto &counts = ex.skippedGeometryCounts();
    // NurbsPatchSurface counted exactly once...
    auto it = counts.find("NurbsPatchSurface");
    CHECK((it != counts.end() && it->second == 1));
    // ...and the legitimately-empty IFS is NOT a coverage gap.
    CHECK((counts.find("IndexedFaceSet") == counts.end()));
    // ...and the supported TriangleSet never appears either.
    CHECK((counts.find("TriangleSet") == counts.end()));
    CHECK((counts.size() == 1));
  }

  // === 2) Two unsupported shapes of the same type accumulate =================
  {
    auto root = createX3DNode("Group");
    addChild(root, makeUnsupportedShape());
    addChild(root, makeUnsupportedShape());
    Scene scene;
    scene.addRootNode(root);
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    extract::SceneExtractor ex(ctx, scene);
    ex.fullSnapshot();
    CHECK((ex.skippedGeometryCounts().at("NurbsPatchSurface") == 2));

    // === 3) fullSnapshot() recounts from scratch (map cleared each full walk) =
    ex.fullSnapshot();
    CHECK((ex.skippedGeometryCounts().at("NurbsPatchSurface") == 2)); // not 4.
  }

  return;
}
