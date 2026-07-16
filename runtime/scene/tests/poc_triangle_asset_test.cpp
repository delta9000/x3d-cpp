// poc_triangle_asset_test.cpp — M2.5 T10 (FIRST LIGHT) build-side acceptance.
//
// Parses the SHIPPED hand-written first-light asset (examples/poc_renderer/
// assets/triangle.x3d) through the real parse front door (x3d::codec::parseFile)
// and runs the T7a minimal SceneExtractor over it. Asserts the asset parses and
// yields >= 1 RenderItem whose mesh has exactly 3 vertices — i.e. the single
// IndexedFaceSet triangle survives parse -> bridge -> MeshBuilder -> extractor.
//
// This is the headless, NO-GL half of the T10 acceptance: it proves the asset is
// drawable independent of the user's on-screen visual verify. It deliberately
// uses the same asset the PoC binary loads by default, so the test and the GUI
// agree on what "first light" draws. The asset path is injected at compile time
// (X3D_POC_TRIANGLE_ASSET) so the test has no working-directory dependency.
#include "SceneExtractor.hpp"

#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DParse.hpp" // x3d::codec::parseFile
#include "X3DScene.hpp"

#include "doctest/doctest.h"
#include <cstdio>
#include <exception>
#include <string>

using namespace x3d::runtime;

TEST_CASE("poc_triangle_asset_test") {
#ifndef X3D_POC_TRIANGLE_ASSET
#error "X3D_POC_TRIANGLE_ASSET must be defined to point at the triangle.x3d asset"
#endif
  const std::string assetPath = X3D_POC_TRIANGLE_ASSET;

  // 1. Parse the shipped asset through the real front door.
  X3DDocument doc;
  try {
    doc = x3d::codec::parseFile(assetPath);
  } catch (const std::exception &e) {
    std::fprintf(stderr, "parseFile(%s) threw: %s\n", assetPath.c_str(), e.what());
    CHECK((false && "first-light asset must parse"));
    return;
  }

  Scene &scene = doc.getScene();
  X3DExecutionContext ctx;
  // buildSceneGraph + tick are all the extractor needs (it reads the transform /
  // bounds side tables, not the ROUTE graph). buildFrom is the renderer's
  // load-time ROUTE check and pulls in the bridge header — out of scope here.
  ctx.buildSceneGraph(scene);
  ctx.tick(0.0);

  // 2. Extract. The asset is a single IndexedFaceSet triangle, so the snapshot
  //    must contain >= 1 RenderItem, and the first item's mesh must have exactly
  //    3 vertices (one triangle, 3 corners; trivial 0..N-1 indices).
  extract::SceneExtractor ex(ctx, scene);
  extract::RenderDelta snap = ex.fullSnapshot();

  CHECK((!snap.added.empty() && "triangle.x3d must yield >= 1 RenderItem"));
  CHECK((ex.itemCount() >= 1));

  const extract::RenderItem &it = ex.item(snap.added.front());
  CHECK((it.mesh->positions.size() == 3 && "the one IndexedFaceSet face is a triangle"));
  CHECK((it.mesh->indices.size() == 3));

  // 3. The triangle is centered on the origin in the Z=0 plane (see asset). Its
  //    per-path world AABB must be non-empty so near/far fit has something to work
  //    with on the GL side.
  Aabb wb = ex.sceneWorldBounds();
  CHECK((!wb.empty));

  std::fprintf(stderr,
               "[ok] %s -> %zu render item(s), item[0] has %zu vertices\n",
               assetPath.c_str(), ex.itemCount(), it.mesh->positions.size());
  return;
}
