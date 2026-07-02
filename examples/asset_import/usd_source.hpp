#ifndef X3D_ASSET_IMPORT_USD_SOURCE_HPP
#define X3D_ASSET_IMPORT_USD_SOURCE_HPP

#include "import_source.hpp"

namespace x3d::asset_import {

// ImportSource backed by TinyUSDZ (compiled only when
// -DX3D_CPP_BUILD_USD=ON). Converts USDA/USDC/USDZ models
// into the ImportScene IR.
class UsdSource : public ImportSource {
public:
  ImportScene load(const std::string& path) override;
};

}  // namespace x3d::asset_import

#endif  // X3D_ASSET_IMPORT_USD_SOURCE_HPP
