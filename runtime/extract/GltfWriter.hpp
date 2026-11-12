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
//   - url textures as external image references (the first PNG or JPEG url,
//     verbatim), for the base colour, normal, emissive, occlusion and
//     metallic-roughness slots. Texture coordinates are flipped to glTF's
//     top-left origin.
//
// Not written (each is counted in GltfWriteReport): packed/binary geometry
// items, PixelTexture and other non-url textures, MultiTexture, generated
// texture coordinates, textures with no PNG/JPEG url, back materials, author
// shaders. Skinned H-Anim items are written as static meshes in the mesh's
// stored pose. Lights, cameras and animation are not exported. Colour values
// are copied as authored, with no sRGB/linear conversion.
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
  std::size_t textures = 0;  ///< distinct glTF textures (external images)
  std::size_t triangles = 0; ///< triangles over all nodes (instances counted)

  std::size_t skippedEmpty = 0;    ///< items with no positions or indices
  std::size_t skippedPacked = 0;   ///< packed/binary geometry items
  std::size_t droppedTextures = 0; ///< texture slots that could not be written
  std::size_t staticSkins = 0;     ///< skinned items written without a skin
  std::size_t droppedBackMaterials = 0;
  std::size_t droppedShaders = 0;
};

/// Serialize `items` to GLB bytes (the string may contain NULs).
std::string writeGlb(const std::vector<const RenderItem *> &items,
                     const GltfWriteOptions &options = {},
                     GltfWriteReport *report = nullptr);

/// Convenience over an extractor and item ids (a snapshot's `added` list).
inline std::string writeGlb(const SceneExtractor &extractor,
                            const std::vector<RenderItemId> &ids,
                            const GltfWriteOptions &options = {},
                            GltfWriteReport *report = nullptr) {
  std::vector<const RenderItem *> items;
  items.reserve(ids.size());
  for (RenderItemId id : ids) items.push_back(&extractor.item(id));
  return writeGlb(items, options, report);
}

} // namespace x3d::runtime::extract

#endif // X3D_RUNTIME_EXTRACT_GLTF_WRITER_HPP
