#ifndef X3D_ASSET_IMPORT_ASSIMP_SOURCE_HPP
#define X3D_ASSET_IMPORT_ASSIMP_SOURCE_HPP

#include "import_source.hpp"

namespace x3d::asset_import {

// ImportSource backed by assimp (compiled only when
// -DX3D_CPP_BUILD_ASSIMP=ON). Converts the assimp aiScene into the
// ImportScene IR: meshes (positions/normals/uv[0]/colors[0]/triangulated
// indices + materialIndex), materials (Phong + PBR), the node tree with
// local transforms, cameras, lights, and embedded textures.
class AssimpSource : public ImportSource {
public:
  ImportScene load(const std::string& path) override;
};

}  // namespace x3d::asset_import

#endif  // X3D_ASSET_IMPORT_ASSIMP_SOURCE_HPP
