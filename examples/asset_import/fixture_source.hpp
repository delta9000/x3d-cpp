#ifndef X3D_ASSET_IMPORT_FIXTURE_SOURCE_HPP
#define X3D_ASSET_IMPORT_FIXTURE_SOURCE_HPP
#include "import_source.hpp"
namespace x3d::asset_import {
// Dependency-free ImportSource backend producing synthetic ImportScenes.
// Always available (no third-party parser); used for tests, CI without
// assimp, and `main` (`fixture:<name>`).
class FixtureSource : public ImportSource {
public:
  // name: "cube" | "hierarchy" | "lit". Throws std::runtime_error on any
  // other name.
  ImportScene load(const std::string& name) override;
};
} // namespace x3d::asset_import
#endif
