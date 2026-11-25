// UsdSource.hpp — USD backend (USDA, USDC, USDZ) over tinyusdz.
//
// Built as the source-tree library x3d_import_usd when -DX3D_CPP_BUILD_USD=ON
// (tinyusdz is fetched at a pinned commit and patched, see tinyusdz.patch). It is
// not installed with the SDK: link it and register it next to the built-in
// glTF backend.
//
//   auto reg = x3d::runtime::importer::builtinBackends();
//   x3d::runtime::importer::addUsdBackend(reg);
#ifndef X3D_RUNTIME_IMPORT_USD_USD_SOURCE_HPP
#define X3D_RUNTIME_IMPORT_USD_USD_SOURCE_HPP

#include "ImportSource.hpp"

#include <string>

namespace x3d::runtime::importer {

class UsdSource : public ImportSource {
public:
  ImportScene load(const std::string &path) override;
};

/// Registers "usd" for .usd, .usda, .usdc and .usdz.
void addUsdBackend(BackendRegistry &registry);

} // namespace x3d::runtime::importer

#endif // X3D_RUNTIME_IMPORT_USD_USD_SOURCE_HPP
