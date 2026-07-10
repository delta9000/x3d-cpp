#ifndef X3D_ASSET_IMPORT_CGLTF_SOURCE_HPP
#define X3D_ASSET_IMPORT_CGLTF_SOURCE_HPP
#include "import_source.hpp"

namespace x3d::asset_import {

// glTF 2.0 (.gltf / .glb) import backend built on the vendored, header-only
// cgltf parser. Walks the parsed document into the shared ImportScene IR:
// meshes, PBR metallic-roughness materials, textures (external URI, GLB bin
// buffer-view, and base64 data: URI), node hierarchy with TRS, perspective
// cameras, and KHR_lights_punctual lights. Image bytes are carried verbatim —
// texel decode stays in the downstream stb texture pipeline.
class CgltfSource : public ImportSource {
public:
  ImportScene load(const std::string& path) override;
};

} // namespace x3d::asset_import
#endif
