// AssetImport.hpp
// Mesh import front door: glTF, OBJ, FBX, USD, ... -> X3D 4.0 document.
//
//   namespace imp = x3d::runtime::importer;
//   imp::ImportOptions opts;
//   opts.outputPath = "out/model.x3d";          // textures go to out/assets/
//   imp::ImportResult r = imp::importFile("model.glb", opts);
//   std::string xml = x3d::codec::XmlWriter{}.writeDocument(r.document);
//
// The glTF backend (cgltf) is always built. Assimp (OBJ, FBX, Collada, PLY,
// STL, 3DS, ...) and USD (USDA, USDC, USDZ) are optional source-tree backends:
// link x3d_import_assimp / x3d_import_usd and call addAssimpBackend() /
// addUsdBackend() on the registry.
#ifndef X3D_RUNTIME_IMPORT_ASSET_IMPORT_HPP
#define X3D_RUNTIME_IMPORT_ASSET_IMPORT_HPP

#include "ImportEmit.hpp"
#include "ImportScene.hpp"
#include "ImportSource.hpp"
#include "MaterialGlsl.hpp"
#include "TexturePlan.hpp"
#include "TextureResolver.hpp"
#include "X3DDocument.hpp"

#include <string>
#include <vector>

namespace x3d::runtime::importer {

struct ImportOptions {
  /// Backend name ("gltf", "assimp", "usd"); empty picks by extension.
  std::string backend;
  /// Where the document will be written. Texture urls are relative to its
  /// directory; empty means the current directory.
  std::string outputPath;
  /// Textures go to <assetsRoot>/assets/; empty means the output directory.
  std::string assetsRoot;
  /// Write texture images and reference them. false: no ImageTexture nodes.
  bool textures = true;
  /// Re-encode PNG and JPEG images as PNG too (needs `decoder`).
  bool recompress = false;
  /// Decoder for images that are not PNG or JPEG; they are then written as
  /// PNG. x3d::runtime::io::stb::makeStbTextureResolver() is one.
  x3d::runtime::extract::TextureResolver decoder;
  Autoplay autoplay = Autoplay::First;
  int samplesPerInterval = 8;
};

struct ImportReport {
  std::string backend;               ///< the backend that read the file
  std::vector<std::string> warnings; ///< from the backend
  TexturePlanReport textures;
  EmitReport emit;
};

struct ImportResult {
  x3d::runtime::X3DDocument document;
  ImportScene scene; ///< the intermediate scene, for materialGlsl() and tools
  ImportReport report;
};

/// The glTF backend under the name "gltf" (.gltf, .glb).
void addGltfBackend(BackendRegistry &registry);

/// A registry holding the backends built into the import library (glTF).
BackendRegistry builtinBackends();

/// Reads `path` with the selected backend, writes its textures and builds the
/// document. Throws std::runtime_error when no backend reads the file or the
/// backend fails; content it cannot map is reported, not thrown.
ImportResult importFile(const std::string &path, const ImportOptions &opts = {},
                        const BackendRegistry &registry = builtinBackends());

/// The same, for a scene a caller already built or read.
ImportResult importScene(ImportScene scene, const std::string &modelDir,
                         const ImportOptions &opts = {});

} // namespace x3d::runtime::importer

#endif // X3D_RUNTIME_IMPORT_ASSET_IMPORT_HPP
