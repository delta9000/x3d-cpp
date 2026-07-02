#ifndef X3D_ASSET_IMPORT_EMIT_HPP
#define X3D_ASSET_IMPORT_EMIT_HPP
#include "import_source.hpp"
#include "x3d/authoring.hpp"
namespace x3d::asset_import {
// Options controlling emit().
struct EmitOptions { bool includeTextures = true; };
// Converts an ImportScene (Task 3 IR) into an x3d::runtime::X3DDocument.
// Recursively walks the ImportNode hierarchy from ImportScene::rootNode,
// emitting nested Transforms (translation/rotation/scale decomposed from
// each node's localTransform) whose children are that node's mesh
// Shape/IndexedTriangleSet nodes plus its own children's Transforms. Shapes
// built for a shared mesh index are DEF'd once and reused (shared_ptr) so
// the writer emits USE for subsequent nodes referencing the same mesh.
x3d::runtime::X3DDocument emit(const ImportScene& scene, const EmitOptions& opts);
} // namespace x3d::asset_import
#endif
