// ShaderBindingPlan.hpp — author-shader binding plan with vocabulary lookup,
// author-field matching, and edit-distance diagnostics.
//
// buildBindingPlan() classifies each uniform name declared in a ComposedShader
// into one of three categories:
//   1. Vocab match    — name is in kVocabulary; source is the semantic enum.
//   2. Author field   — name matches a <field> declared on ShaderProgramDesc.
//   3. Unrecognized   — neither; a nearest-vocab suggestion is computed via
//                       Levenshtein edit distance and a diagnostic is emitted.
//
// This header is standalone: it includes RenderItem.hpp (for ShaderProgramDesc)
// and ShaderUniformVocabulary.hpp (for kVocabulary + UniformSource).
#ifndef X3D_RUNTIME_EXTRACT_SHADER_BINDING_PLAN_HPP
#define X3D_RUNTIME_EXTRACT_SHADER_BINDING_PLAN_HPP

#include "RenderItem.hpp"                  // ShaderProgramDesc, ShaderFieldBinding
#include "ShaderUniformVocabulary.hpp"     // vocab::kVocabulary, UniformSource
#include <algorithm>
#include <string>
#include <vector>

namespace x3d::runtime::extract {

struct BindingEntry {
  std::string declaredName;
  vocab::UniformSource source = vocab::UniformSource::Unrecognized;
  int location = -1;
  bool isAuthorField = false;
  bool unrecognized = false;
  std::string nearestVocabMatch;  // non-empty only for unrecognized entries
};

struct ShaderBindingPlan {
  std::vector<BindingEntry> entries;
  std::vector<std::string> diagnostics;
};

// ---------------------------------------------------------------------------
// Edit-distance helper — standard Levenshtein (O(m*n) table).
// Used only for diagnostic suggestion; not on any hot path.
// ---------------------------------------------------------------------------
inline int editDistance(const std::string &a, const std::string &b) {
  std::vector<std::vector<int>> d(a.size() + 1, std::vector<int>(b.size() + 1));
  for (std::size_t i = 0; i <= a.size(); ++i) d[i][0] = static_cast<int>(i);
  for (std::size_t j = 0; j <= b.size(); ++j) d[0][j] = static_cast<int>(j);
  for (std::size_t i = 1; i <= a.size(); ++i)
    for (std::size_t j = 1; j <= b.size(); ++j)
      d[i][j] = std::min({
        d[i-1][j] + 1,
        d[i][j-1] + 1,
        d[i-1][j-1] + (a[i-1] == b[j-1] ? 0 : 1)
      });
  return d[a.size()][b.size()];
}

// Returns the nearest vocab entry name if within a loose distance threshold,
// otherwise returns an empty string.
inline std::string nearestVocabSuggestion(const std::string &name) {
  const auto *best = &vocab::kVocabulary[0];
  int bestDist = editDistance(name, std::string(best->name));
  for (const auto &e : vocab::kVocabulary) {
    int dist = editDistance(name, std::string(e.name));
    if (dist < bestDist) { bestDist = dist; best = &e; }
  }
  // Suggest only if within 1/3 of the name length + 1 (catches typos / camelCase drift).
  if (bestDist <= static_cast<int>(name.size()) / 3 + 1)
    return std::string(best->name);
  return {};
}

// ---------------------------------------------------------------------------
// buildBindingPlan — classify declared uniforms into vocab / author / unknown.
//
// declaredUniforms: pairs of {name, location} as reported by the driver
//   (glGetActiveUniform / SPIR-V reflection).
// program: the ShaderProgramDesc whose fields carry author <field> declarations.
// ---------------------------------------------------------------------------
inline ShaderBindingPlan buildBindingPlan(
    const std::vector<std::pair<std::string, int>> &declaredUniforms,
    const ShaderProgramDesc &program) {
  ShaderBindingPlan plan;
  for (const auto &[name, loc] : declaredUniforms) {
    BindingEntry e;
    e.declaredName = name;
    e.location = loc;

    // 1. Check vocabulary.
    for (const auto &v : vocab::kVocabulary) {
      if (name == v.name) {
        e.source = v.source;
        break;
      }
    }
    if (e.source != vocab::UniformSource::Unrecognized) {
      plan.entries.push_back(e);
      continue;
    }

    // 2. Check author <field> declarations on the shader.
    for (const auto &f : program.fields) {
      if (name == f.name) {
        e.isAuthorField = true;
        break;
      }
    }
    if (e.isAuthorField) {
      plan.entries.push_back(e);
      continue;
    }

    // 3. Unrecognized — compute suggestion and emit diagnostic.
    e.unrecognized = true;
    e.nearestVocabMatch = nearestVocabSuggestion(name);
    plan.entries.push_back(e);

    std::string diag = "shader declared `" + name + "` — not in vocabulary, ";
    if (!e.nearestVocabMatch.empty())
      diag += "not a declared <field>. Did you mean `" + e.nearestVocabMatch + "`?";
    else
      diag += "not a declared <field>. Will be unbound (location " +
              std::to_string(loc) + ").";
    plan.diagnostics.push_back(std::move(diag));
  }
  return plan;
}

} // namespace x3d::runtime::extract
#endif // X3D_RUNTIME_EXTRACT_SHADER_BINDING_PLAN_HPP
