#ifndef X3D_ASSET_IMPORT_EMIT_HPP
#define X3D_ASSET_IMPORT_EMIT_HPP
#include "import_source.hpp"
#include "x3d/authoring.hpp"
namespace x3d::asset_import {
// Options controlling emit(); geometry-only for Task 5 (materials/textures,
// full hierarchy TRS, DEF/USE land in later tasks).
struct EmitOptions { bool includeTextures = true; };
// Converts an ImportScene (Task 3 IR) into an x3d::runtime::X3DDocument. Task
// 5 covers the geometry core: each root ImportNode's meshes become
// Shape/IndexedTriangleSet nodes wrapped in a Transform.
x3d::runtime::X3DDocument emit(const ImportScene& scene, const EmitOptions& opts);
} // namespace x3d::asset_import
#endif
