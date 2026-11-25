// ImportEmit.hpp
// Builds an X3D 4.0 document from an ImportScene.
//
// Node hierarchy -> Transform tree (DEF'd by node name); meshes -> Shape with
// IndexedTriangleSet / IndexedLineSet / PointSet; materials -> Appearance with
// PhysicalMaterial, Material or UnlitMaterial (DEF/USE shared); cameras ->
// Viewpoint / OrthoViewpoint and lights -> DirectionalLight / PointLight /
// SpotLight at their nodes; skins -> HAnimHumanoid; animations -> TimeSensor,
// interpolators and ROUTEs. docs/wiki/reference/subsystems/asset-import.md has
// the full mapping and its approximations.
#ifndef X3D_RUNTIME_IMPORT_IMPORT_EMIT_HPP
#define X3D_RUNTIME_IMPORT_IMPORT_EMIT_HPP

#include "ImportScene.hpp"
#include "TexturePlan.hpp"
#include "X3DDocument.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace x3d::runtime::importer {

/// Which TimeSensors start enabled. Every one loops; a host starts another by
/// setting its `enabled` to TRUE.
enum class Autoplay { First, All, None };

struct EmitOptions {
  /// Image urls from planTextures(). nullptr: no ImageTexture nodes.
  const TexturePlan *textures = nullptr;
  Autoplay autoplay = Autoplay::First;
  /// Linear steps per key interval when a curve X3D cannot interpolate the
  /// same way is sampled (cubic splines, colours, which X3D blends in HSV).
  int samplesPerInterval = 8;
};

struct EmitReport {
  std::size_t transforms = 0;
  std::size_t shapes = 0;
  std::size_t materials = 0;
  std::size_t textures = 0; ///< ImageTexture nodes
  std::size_t viewpoints = 0;
  std::size_t lights = 0;
  std::size_t humanoids = 0;
  std::size_t joints = 0;
  std::size_t timeSensors = 0;
  std::size_t interpolators = 0;
  std::size_t routes = 0;
  /// Values clamped or dropped, and channels that could not be mapped.
  std::vector<std::string> warnings;
};

/// Builds the document. Profile Full, version 4.0. Never throws on content it
/// cannot map; that is reported in `report`.
x3d::runtime::X3DDocument emitDocument(const ImportScene &scene,
                                       const EmitOptions &opts = {},
                                       EmitReport *report = nullptr);

} // namespace x3d::runtime::importer

#endif // X3D_RUNTIME_IMPORT_IMPORT_EMIT_HPP
