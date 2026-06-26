// sdk_facade_test.cpp — the SDK façade must expose a usable v1 surface through
// ONE include ("x3d/sdk.hpp") and ONE namespace (x3d::sdk). This test drives the
// whole spine — load -> wire context -> extract -> tick -> delta — using ONLY
// x3d::sdk::* names. If a re-export drifts or a symbol is dropped, this fails to
// compile (the point of the façade is a frozen, self-contained surface).
#include "x3d/sdk.hpp"

#include <iostream>
#include <string>

namespace sdk = x3d::sdk;

static int failures = 0;
static void check(bool cond, const char *what) {
  if (!cond) {
    std::cerr << "FAIL: " << what << "\n";
    ++failures;
  }
}

// A tiny self-contained X3D-XML scene: one Transform with a Shape (Box +
// emissive Material) plus a Viewpoint, so extraction yields a render item,
// a camera, and the bound viewpoint exercises the runtime pull surface.
static const char *kScene = R"X3D(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0">
  <Scene>
    <Viewpoint position="0 0 10"/>
    <Transform DEF="T" translation="1 2 3">
      <Shape>
        <Appearance><Material emissiveColor="1 0 0"/></Appearance>
        <Box size="2 2 2"/>
      </Shape>
    </Transform>
  </Scene>
</X3D>)X3D";

int main() {
  // 1. Load through the façade (parseDocument + Encoding live in x3d::sdk).
  sdk::X3DDocument doc =
      sdk::parseDocument(kScene, sdk::Encoding::XML);
  check(doc.version == "4.0", "version parsed");
  check(doc.profile == sdk::Profile::Interchange, "profile parsed");
  check(!doc.scene.rootNodes.empty(), "scene has roots");
  check(doc.rangeWarnings.empty(), "no range warnings on a clean scene");

  // 2. Wire an execution context (all setup methods via x3d::sdk).
  sdk::X3DExecutionContext ctx;
  ctx.buildSceneGraph(doc.scene);
  sdk::BridgeResult br = ctx.buildFrom(doc.scene);
  check(br.rejected.empty(), "no rejected routes");

  // 3. Extract a full snapshot — every supported descriptor type is reachable.
  sdk::SceneExtractor ex(ctx, doc.scene);
  sdk::RenderDelta f0 = ex.fullSnapshot();
  check(!f0.added.empty(), "snapshot produced at least one render item");

  sdk::RenderItemId id = f0.added.front();
  check(id != sdk::kInvalidRenderItemId, "valid render item id");
  const sdk::RenderItem &ri = ex.item(id);
  check(!ri.mesh.positions.empty(), "box mesh has positions");
  check(ri.mesh.topology == sdk::Topology::Triangles, "box is triangle topology");

  // Material descriptor: emissive red, alpha = 1 - transparency.
  sdk::MaterialDesc mat = ri.material;
  (void)mat.toRGBA();
  check(mat.emissive.r > 0.5f, "emissive red survived extraction");

  // Camera + background descriptors.
  sdk::CameraDesc cam = ex.camera();
  (void)cam.viewMatrix;
  sdk::BackgroundDesc bg = ex.background();
  (void)bg;
  sdk::Aabb worldBounds = ex.sceneWorldBounds();
  check(!worldBounds.empty, "scene world bounds non-empty");

  // 4. Runtime pull surface through the façade.
  sdk::X3DNode *vp = ctx.boundViewpoint();
  check(vp != nullptr, "a viewpoint is bound");
  sdk::Mat4 view = ctx.viewMatrix();
  (void)view;

  // 5. Tick once (to a time distinct from the snapshot's t=0) and request a
  //    delta (one-delta-per-tick contract).
  ctx.tick(0.016);
  sdk::RenderDelta d = ex.delta();
  (void)d;

  // 6. Input setters compile and run (interaction seam).
  sdk::Ray ray;
  ray.origin = {0, 0, 10};
  ray.direction = {0, 0, -1};
  ctx.setPointer(ray);
  ctx.setPointerButton(false);
  ctx.setKey(65, true);
  sdk::PickResult pr = ctx.pick(ray);
  (void)pr;

  // 7. Seam TYPES are nameable through the façade (compile-time surface check).
  sdk::MeshBuildOptions opts;
  opts.sphereRings = 24;
  sdk::TextureResolver texResolver =
      [](const std::string &) { return sdk::TexturePixelResult{}; };
  (void)texResolver;
  sdk::FontMetrics fm = sdk::makeMonospaceStub();
  (void)fm;
  sdk::AssetResolver ar =
      [](const std::string &, sdk::AssetKind) { return sdk::AssetResult{}; };
  (void)ar;

  if (failures == 0)
    std::cout << "sdk_facade_test: OK\n";
  return failures == 0 ? 0 : 1;
}
