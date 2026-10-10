// AuthorShader.hpp — the CPU reference host's §31 author-shader path.
//
// The SDK selects each Appearance's shader and hands the program over on
// RenderItem::shaderProgram (ShaderExtract.hpp). This host runs the program's
// FRAGMENT stage through the GLSL-subset interpreter (GlslInterpreter.hpp),
// with the item's author <field> values as uniforms; the VERTEX stage is
// accepted but not executed (the rasterizer's fixed transform supplies the
// interpreter's varyings vPosEye/vNormalEye/vColor/vTexCoord).
//
//   * interpreterShaderValidator(): the host's §31.3.2 isValid check — the
//     structural one (a VERTEX and a FRAGMENT part) plus "the fragment stage
//     compiles in the interpreter". Pass it as MeshBuildOptions::shaders.
//     validator so selection falls through to the next shader, or to the
//     material, when this host cannot run a program.
//   * AuthorShaderCache::hook(): a RenderOptions::authorShaderFor that binds
//     an item's selected program, compiling each distinct source once.
//
// Out-of-SDK consumer code. namespace x3d::cpuraster.
#ifndef X3D_CPURASTER_AUTHOR_SHADER_HPP
#define X3D_CPURASTER_AUTHOR_SHADER_HPP

#include "GlslInterpreter.hpp"
#include "SceneRender.hpp"
#include "ShaderOptions.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace x3d::cpuraster {

namespace author_detail {
inline const std::string *fragmentSource(const x3d::runtime::extract::ShaderProgramDesc &p) {
  for (const auto &s : p.stages)
    if (s.stage == x3d::runtime::extract::ShaderStageDesc::Stage::Fragment) return &s.source;
  return nullptr;
}
} // namespace author_detail

inline x3d::runtime::extract::ShaderValidator interpreterShaderValidator() {
  namespace ex = x3d::runtime::extract;
  // Selection re-runs on every uniform event; compile each source once.
  auto verdicts = std::make_shared<std::unordered_map<std::string, ex::ShaderValidation>>();
  return [verdicts](const ex::ShaderProgramDesc &p) -> ex::ShaderValidation {
    ex::ShaderValidation v = ex::structuralShaderValidation(p);
    if (!v.valid) return v;
    const std::string &src = *author_detail::fragmentSource(p);
    if (auto it = verdicts->find(src); it != verdicts->end()) return it->second;
    InterpretedProgram prog;
    std::string err;
    v = prog.compile(src, &err) ? ex::ShaderValidation{true, {}} : ex::ShaderValidation{false, err};
    (*verdicts)[src] = v;
    return v;
  };
}

class AuthorShaderCache {
public:
  // A RenderOptions::authorShaderFor for items that carry a selected program.
  // The cache must outlive the returned hook.
  auto hook() {
    return [this](const x3d::runtime::extract::RenderItem &it,
                  const std::vector<EyeLight> &lights, bool hasColors) -> FragmentShader {
      if (!it.shaderProgram || !it.shaderProgram->isValid) return {};
      const std::string *src = author_detail::fragmentSource(*it.shaderProgram);
      if (!src) return {};
      const InterpretedProgram *prog = get(*src);
      if (!prog) return {};
      return makeInterpretedShader(*prog, it.material, lights, hasColors,
                                   &it.shaderProgram->fields);
    };
  }

  // The compiled program for `source`, or null if it does not compile.
  const InterpretedProgram *get(const std::string &source) {
    auto &slot = programs_[source];
    if (!slot) {
      slot = std::make_shared<InterpretedProgram>();
      slot->compile(source);
    }
    return slot->compiled() ? slot.get() : nullptr;
  }

private:
  std::unordered_map<std::string, std::shared_ptr<InterpretedProgram>> programs_;
};

} // namespace x3d::cpuraster

#endif // X3D_CPURASTER_AUTHOR_SHADER_HPP
