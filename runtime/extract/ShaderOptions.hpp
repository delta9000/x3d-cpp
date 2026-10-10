// ShaderOptions.hpp — embedder seams for §31 programmable shaders. namespace
// x3d::runtime::extract. A leaf (RenderItem + AssetResolver only) so the
// extractor's option bag (MeshBuildOptions) can carry it without pulling node
// headers; the selection logic lives in ShaderExtract.hpp.
#ifndef X3D_RUNTIME_EXTRACT_SHADER_OPTIONS_HPP
#define X3D_RUNTIME_EXTRACT_SHADER_OPTIONS_HPP

#include "AssetResolver.hpp"
#include "RenderItem.hpp"

#include <functional>
#include <string>

namespace x3d::runtime::extract {

/// A host's verdict on an assembled program (e.g. a GL compile + link).
struct ShaderValidation {
  bool valid = false;
  std::string error; // compiler/linker log when !valid
};
using ShaderValidator = std::function<ShaderValidation(const ShaderProgramDesc &)>;

/// Embedder seams for §31 shaders. Both are optional.
struct ShaderOptions {
  /// Bytes for non-`data:` part urls (AssetKind::Shader). Null = such urls fail.
  AssetResolver resolver = nullptr;
  /// Compile/link check. Null = structuralShaderValidation().
  ShaderValidator validator = nullptr;
};

/// The default validator: one VERTEX and one FRAGMENT stage, no duplicates.
inline ShaderValidation structuralShaderValidation(const ShaderProgramDesc &p) {
  int counts[6] = {};
  for (const auto &s : p.stages) ++counts[static_cast<int>(s.stage)];
  for (int c : counts)
    if (c > 1) return {false, "more than one part of the same type"};
  if (counts[static_cast<int>(ShaderStageDesc::Stage::Vertex)] != 1)
    return {false, "no VERTEX part"};
  if (counts[static_cast<int>(ShaderStageDesc::Stage::Fragment)] != 1)
    return {false, "no FRAGMENT part"};
  return {true, {}};
}

} // namespace x3d::runtime::extract

#endif // X3D_RUNTIME_EXTRACT_SHADER_OPTIONS_HPP
