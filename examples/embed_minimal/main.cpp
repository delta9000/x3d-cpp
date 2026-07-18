// The README render-consumer example (§Quickstart 2): RuntimeSession +
// fullSnapshot upload, then tick + delta. Consumes the hello.x3d the authoring
// half writes (scene path overridable via argv[1]), "uploads" by printing, and
// runs three fixed ticks in place of a real frame loop. Exit code asserts the
// authored sphere actually came out as extracted mesh data.
#include "x3d/sdk.hpp"

#include <cstddef>
#include <iostream>

namespace sdk = x3d::sdk;

namespace {
std::size_t uploadedVertices = 0;

// A real renderer would create GPU buffers here.
void uploadMesh(const sdk::MeshData &mesh, const sdk::Mat4 &,
                const sdk::MaterialDesc &material) {
  uploadedVertices += mesh.positions.size();
  const char *model = material.model == sdk::MaterialModel::Physical ? "Physical"
                      : material.model == sdk::MaterialModel::Unlit  ? "Unlit"
                                                                     : "Phong";
  std::cout << "upload: " << mesh.positions.size() << " vertices, "
            << mesh.indices.size() << " indices, material=" << model << "\n";
}
} // namespace

int main(int argc, char **argv) {
  const char *scenePath = argc > 1 ? argv[1] : "hello.x3d";
  auto session = sdk::RuntimeSession::create(sdk::parseFile(scenePath));

  // Frame 0: everything, once.
  sdk::RenderDelta f0 = session->fullSnapshot();
  for (sdk::RenderItemId id : f0.added) {
    const sdk::RenderItem &item = session->extractor().item(id);
    const sdk::MeshData &mesh = *item.mesh; // positions/normals/texcoords/indices
    for (const auto &tex : item.material.textures) {
      // tex.slot: BaseColor / Normal / MetallicRoughness / ...
      // tex.url (resolve via your loader) or tex.inlinePixels (PixelTexture)
      (void)tex;
    }
    uploadMesh(mesh, item.worldTransform, item.material);
  }

  // Every frame after: only what changed (three fixed frames stand in for a
  // real render loop; `now` is seconds since start on a monotonic clock).
  for (double now : {0.0, 1.0 / 60.0, 2.0 / 60.0}) {
    session->tick(now);
    sdk::RenderDelta d = session->delta();
    // d.added / d.removed        -> create / destroy GPU objects
    // d.updatedTransform         -> re-upload the model matrix
    // d.updatedGeometry          -> re-upload vertex data (morph/animation)
    // d.updatedMaterial          -> update uniforms
    // d.cameraChanged / d.lightsChanged / d.backgroundChanged
    (void)d;
  }

  std::cout << "x3d-cpp embedded: version=" << session->document().version
            << " render_items=" << f0.added.size()
            << " vertices=" << uploadedVertices << "\n";
  return (!f0.added.empty() && uploadedVertices > 0) ? 0 : 1;
}
