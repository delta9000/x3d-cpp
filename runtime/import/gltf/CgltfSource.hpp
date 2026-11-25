// CgltfSource.hpp — glTF 2.0 (.gltf, .glb) backend over the vendored cgltf.
#ifndef X3D_RUNTIME_IMPORT_GLTF_CGLTF_SOURCE_HPP
#define X3D_RUNTIME_IMPORT_GLTF_CGLTF_SOURCE_HPP

#include "ImportSource.hpp"

#include <string>

namespace x3d::runtime::importer {

class CgltfSource : public ImportSource {
public:
  ImportScene load(const std::string &path) override;
};

} // namespace x3d::runtime::importer

#endif // X3D_RUNTIME_IMPORT_GLTF_CGLTF_SOURCE_HPP
