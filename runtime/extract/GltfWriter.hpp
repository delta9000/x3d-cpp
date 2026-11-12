// GltfWriter.hpp
// Writes extracted render items as a binary glTF 2.0 asset (.glb).
//
// The input is what SceneExtractor already produces: one RenderItem per
// placed shape, with a world matrix, a shared mesh and a material descriptor.
// So the exporter sees the scene after PROTO/Inline expansion, Switch/LOD
// selection and tessellation, and every geometry MeshBuilder supports comes
// out as triangles, lines or points.
//
//   sdk::SceneExtractor ex(ctx, doc.scene);
//   auto frame = ex.fullSnapshot();
//   std::string glb = extract::writeGlb(ex, frame.added);
//
// Written:
//   - one glTF node per item, carrying the item's world matrix (the X3D
//     grouping hierarchy is flattened), named after the nearest DEF on its path;
//   - one glTF mesh per distinct MeshData and material, so DEF/USE instances
//     share buffers: POSITION, NORMAL, TEXCOORD_n, COLOR_0, uint32 indices;
//   - materials: PhysicalMaterial maps directly to pbrMetallicRoughness;
//     Material (Phong) is approximated (diffuse -> baseColor, metallic 0,
//     roughness from shininess); UnlitMaterial and unlit lines/points use
//     KHR_materials_unlit. transparency becomes alpha with BLEND.
//     solid=FALSE becomes doubleSided;
//   - textures for the base colour, normal, emissive, occlusion and
//     metallic-roughness slots. An ImageTexture with a PNG or JPEG url is an
//     external image reference (the url, verbatim). A PixelTexture, and any
//     texture the host's TextureResolver has decoded, is embedded as a PNG.
//     Texture coordinates are flipped to glTF's top-left origin;
//   - lights as KHR_lights_punctual (directional, point, spot), each on a node
//     placed and aimed in world space;
//   - Viewpoint and OrthoViewpoint nodes as cameras, the bound one first.
//
// Approximations to know about:
//   - X3D light intensity is a unitless [0,1] factor; it is written unchanged
//     as glTF intensity (lux / candela). ambientIntensity, the attenuation
//     coefficients and light scoping (global=FALSE) have no glTF equivalent
//     and are dropped; radius becomes range.
//   - X3D fieldOfView is the angle across the smaller viewport dimension;
//     it is written as yfov, which is the same thing for a landscape viewport.
//     An OrthoViewpoint's off-centre extent is centred.
//   - Embedded PNGs are stored uncompressed.
//   - Colour values are copied as authored, with no sRGB/linear conversion.
//
// Not written (each is counted in GltfWriteReport): packed/binary geometry
// items, MovieTexture, MultiTexture, cube and 3D textures, textures on
// generated texture coordinates, undecoded textures with no PNG/JPEG url,
// back materials, author shaders, GeoViewpoint. Skinned H-Anim items are
// written as static meshes in the mesh's stored pose. Animation is not
// exported.
#ifndef X3D_RUNTIME_EXTRACT_GLTF_WRITER_HPP
#define X3D_RUNTIME_EXTRACT_GLTF_WRITER_HPP

#include "SceneExtractor.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace x3d::runtime::extract {

struct GltfWriteOptions {
  /// asset.generator in the output.
  std::string generator = "x3d-cpp";
};

struct GltfWriteReport {
  std::size_t nodes = 0;     ///< items written
  std::size_t meshes = 0;    ///< distinct glTF meshes
  std::size_t materials = 0; ///< distinct glTF materials
  std::size_t textures = 0;  ///< distinct glTF textures
  std::size_t embeddedImages = 0; ///< images embedded as PNG
  std::size_t lights = 0;    ///< KHR_lights_punctual lights
  std::size_t cameras = 0;   ///< cameras
  std::size_t triangles = 0; ///< triangles over all nodes (instances counted)

  std::size_t skippedEmpty = 0;    ///< items with no positions or indices
  std::size_t skippedPacked = 0;   ///< packed/binary geometry items
  std::size_t droppedTextures = 0; ///< texture slots that could not be written
  std::size_t staticSkins = 0;     ///< skinned items written without a skin
  std::size_t droppedBackMaterials = 0;
  std::size_t droppedShaders = 0;
  std::size_t skippedViewpoints = 0; ///< GeoViewpoint
};

/// One camera to export. `cameraToWorld` places it; like an X3D viewpoint a
/// glTF camera looks down its local -Z with +Y up.
struct GltfCamera {
  std::string name;
  Mat4 cameraToWorld = Mat4::identity();
  bool ortho = false;
  float yfov = 0.7854f;    ///< perspective: vertical field of view, radians
  float xmag = 1.0f, ymag = 1.0f; ///< orthographic half extents
  float znear = 0.1f;
  float zfar = 0.0f;       ///< <= 0: infinite (perspective) / 10000 (ortho)
};

/// Scene content beyond the render items.
struct GltfSceneExtras {
  std::vector<LightDesc> lights;
  std::vector<GltfCamera> cameras;
};

/// The lights and viewpoints of the extractor's scene: `extractor.lights()`
/// and every Viewpoint/OrthoViewpoint, the bound one first. GeoViewpoints are
/// counted in `report->skippedViewpoints`.
GltfSceneExtras gltfExtrasFrom(const SceneExtractor &extractor,
                               GltfWriteReport *report = nullptr);

/// Serialize `items`, plus the lights and cameras in `extras`, to GLB bytes
/// (the string may contain NULs).
std::string writeGlb(const std::vector<const RenderItem *> &items,
                     const GltfSceneExtras &extras,
                     const GltfWriteOptions &options = {},
                     GltfWriteReport *report = nullptr);

/// Convenience over an extractor and item ids (a snapshot's `added` list);
/// includes the scene's lights and viewpoints (see gltfExtrasFrom).
std::string writeGlb(const SceneExtractor &extractor,
                     const std::vector<RenderItemId> &ids,
                     const GltfWriteOptions &options = {},
                     GltfWriteReport *report = nullptr);

} // namespace x3d::runtime::extract

#endif // X3D_RUNTIME_EXTRACT_GLTF_WRITER_HPP
