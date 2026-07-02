#ifndef X3D_ASSET_IMPORT_GLSL_EMIT_HPP
#define X3D_ASSET_IMPORT_GLSL_EMIT_HPP
#include "import_source.hpp"
#include <string>

namespace x3d::asset_import {

// Produces a self-contained, portable GLSL fragment shader for one imported
// material: the canonical examples/cpu_raster/shaders/usd_preview_surface.frag
// (embedded verbatim at build time — see CMakeLists.txt) with the material's
// UsdPreviewSurface scalar parameters baked in as `const` in place of the
// matching `uniform` declarations. This is the only faithful channel for
// fidelity (ior/clearcoat/specular-workflow/opacityMode) that the X3D
// PhysicalMaterial node cannot model — see ImportMaterial's doc comment.
//
// The BRDF body, texture samplers, `uHas*Tex` gates, varyings, and light
// uniforms are left byte-identical to the canonical shader; only the
// material-scalar `uniform` lines listed in emitMaterialGlsl()'s
// implementation are rewritten to `const`.
std::string emitMaterialGlsl(const ImportMaterial& mat);

} // namespace x3d::asset_import
#endif
