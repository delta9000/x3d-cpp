// MaterialGlsl.hpp
// A portable fragment shader per imported material, for the UsdPreviewSurface
// values X3D's PhysicalMaterial cannot hold (ior, clearcoat, the specular
// workflow, opacityMode).
#ifndef X3D_RUNTIME_IMPORT_MATERIAL_GLSL_HPP
#define X3D_RUNTIME_IMPORT_MATERIAL_GLSL_HPP

#include "ImportScene.hpp"

#include <string>

namespace x3d::runtime::importer {

/// The canonical runtime/import/shaders/usd_preview_surface.frag (embedded at
/// build time) with the material's scalar parameters baked in: each matching
/// `uniform` declaration becomes a `const`. Samplers, `uHas*Tex` gates,
/// varyings and light uniforms are left as they are, so a host binds them as
/// for the canonical shader. See ADR-0043.
std::string materialGlsl(const ImportMaterial &mat);

} // namespace x3d::runtime::importer

#endif // X3D_RUNTIME_IMPORT_MATERIAL_GLSL_HPP
