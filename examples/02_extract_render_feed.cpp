// examples/02_extract_render_feed.cpp
// ─────────────────────────────────────────────────────────────────────────────
// EXTRACT → FEED-A-RENDERER, headless, using only the x3d::sdk façade.
//
// This is the path a real renderer embeds: load a scene, build an execution
// context, take a full snapshot, then "upload" each RenderItem. Instead of
// issuing GL/Vulkan calls we print what a consumer would consume — the mesh
// topology, vertex/index counts, the resolved world transform, the material,
// the camera, and the lights. This mirrors examples/poc_renderer/main.cpp but
// stays headless (no GL) so it runs in CI.
//
// Run: 02_extract_render_feed [path/to/scene]
// ─────────────────────────────────────────────────────────────────────────────
#include "x3d/sdk.hpp"

#include <iostream>
#include <string>

namespace sdk = x3d::sdk;

static const char *kInlineScene = R"X3D(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0">
  <Scene>
    <Viewpoint position="0 0 12"/>
    <DirectionalLight direction="0 -1 -1" intensity="0.8"/>
    <Transform translation="-2 0 0">
      <Shape>
        <Appearance><Material diffuseColor="0.9 0.2 0.2"/></Appearance>
        <Box size="2 2 2"/>
      </Shape>
    </Transform>
    <Transform translation="2 0 0">
      <Shape>
        <Appearance><Material diffuseColor="0.2 0.2 0.9"/></Appearance>
        <Sphere radius="1.2"/>
      </Shape>
    </Transform>
  </Scene>
</X3D>)X3D";

static const char *topoName(sdk::Topology t) {
  switch (t) {
  case sdk::Topology::Triangles: return "Triangles";
  case sdk::Topology::Lines:     return "Lines";
  case sdk::Topology::Points:    return "Points";
  }
  return "?";
}

int main(int argc, char **argv) {
  // 1. Load.
  sdk::X3DDocument doc;
  try {
    doc = (argc > 1) ? sdk::parseFile(argv[1])
                     : sdk::parseDocument(kInlineScene, sdk::Encoding::XML);
  } catch (const std::exception &e) {
    std::cerr << "Load failed: " << e.what() << "\n";
    return 1;
  }

  // 2. Wire the runtime: index the graph, resolve routes.
  sdk::X3DExecutionContext ctx;
  ctx.buildSceneGraph(doc.scene);
  ctx.buildFrom(doc.scene);

  // 3. Full snapshot — this is the consumer's "upload everything" frame.
  //    MeshBuildOptions lets a renderer pick tessellation density.
  sdk::MeshBuildOptions opts;
  opts.sphereRings = 24;
  opts.sphereSegments = 24;
  sdk::SceneExtractor ex(ctx, doc.scene, opts);
  sdk::RenderDelta frame0 = ex.fullSnapshot();

  std::cout << "== Snapshot: " << frame0.added.size() << " render item(s) ==\n";
  for (sdk::RenderItemId id : frame0.added) {
    const sdk::RenderItem &ri = ex.item(id);
    const sdk::MeshData &m = ri.mesh;
    const sdk::MaterialDesc &mat = ri.material;
    std::cout << "  item #" << id << " : " << topoName(m.topology)
              << "  verts=" << m.positions.size()
              << " idx=" << m.indices.size()
              << " normals=" << (m.hasNormals ? "y" : "n")
              << "  baseColor=(" << mat.toRGBA().r << "," << mat.toRGBA().g
              << "," << mat.toRGBA().b << ")\n";
  }

  // 4. Camera + lights + scene bounds (per-frame descriptors).
  sdk::CameraDesc cam = ex.camera();
  std::cout << "\nCamera fov=" << cam.fieldOfView << " near=" << cam.nearPlane
            << " far=" << cam.farPlane << "\n";
  auto lights = ex.lights();
  std::cout << "Lights: " << lights.size() << "\n";
  sdk::Aabb b = ex.sceneWorldBounds();
  std::cout << "World bounds empty=" << (b.empty ? "y" : "n") << "\n";

  // 5. One tick + incremental delta (the steady-state per-frame loop).
  ctx.tick(0.016);
  sdk::RenderDelta d = ex.delta();
  std::cout << "\nDelta after 1 tick: added=" << d.added.size()
            << " removed=" << d.removed.size()
            << " updatedTransform=" << d.updatedTransform.size()
            << " updatedGeometry=" << d.updatedGeometry.size() << "\n";

  std::cout << "\nOK\n";
  return 0;
}
