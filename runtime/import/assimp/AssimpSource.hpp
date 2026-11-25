// AssimpSource.hpp — Assimp backend (OBJ, FBX, Collada, PLY, STL, 3DS, ...).
//
// Built as the source-tree library x3d_import_assimp when
// -DX3D_CPP_BUILD_ASSIMP=ON finds a system Assimp. It is not installed with
// the SDK: link it and register it next to the built-in glTF backend.
//
//   auto reg = x3d::runtime::importer::builtinBackends();
//   x3d::runtime::importer::addAssimpBackend(reg);
#ifndef X3D_RUNTIME_IMPORT_ASSIMP_ASSIMP_SOURCE_HPP
#define X3D_RUNTIME_IMPORT_ASSIMP_ASSIMP_SOURCE_HPP

#include "ImportSource.hpp"

#include <string>

namespace x3d::runtime::importer {

class AssimpSource : public ImportSource {
public:
  ImportScene load(const std::string &path) override;
};

/// Registers "assimp" for every extension Assimp reads. It outranks nothing
/// for .gltf / .glb, which stay with the cgltf backend unless chosen by name.
void addAssimpBackend(BackendRegistry &registry);

} // namespace x3d::runtime::importer

#endif // X3D_RUNTIME_IMPORT_ASSIMP_ASSIMP_SOURCE_HPP
