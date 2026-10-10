// ShaderExtract.hpp — §31 programmable-shader selection and program assembly.
// namespace x3d::runtime::extract. Header-only and IO-free.
//
// selectShader() walks an Appearance's `shaders` list in order (§31.2.2.3) and
// returns the first candidate the host can run, assembled into the
// ShaderProgramDesc a renderer binds:
//
//   * Supported nodes: ComposedShader (ShaderPart children) and ProgramShader
//     (ShaderProgram children), with language "GLSL" (case-insensitive). Every
//     other node or language is skipped and stays inert, which §31.2.2.3
//     permits: a browser picks the first shader it supports. PackagedShader is
//     not supported.
//   * Source of each part/program, in order: an inline body (`sourceCode`, the
//     XML CDATA) if non-empty, else the first `url` that resolves. A `data:`
//     url (RFC 2397, percent-encoded or ;base64) is decoded here; any other url
//     goes to ShaderOptions::resolver with AssetKind::Shader. load=FALSE skips
//     the url list. A Pending url makes the candidate not-yet-valid
//     (`pending`), so a System can retry it later.
//   * Validity: every part resolved with a known `type`, then
//     ShaderOptions::validator (a host compile/link check). The default
//     validator is structural: one VERTEX and one FRAGMENT stage, and at most
//     one stage of each type. The SDK cannot compile GLSL itself.
//   * Uniforms: the shader's author <field>s (ProgramShader: each program's, in
//     order) with their CURRENT values from the DynamicFieldStore. SF scalar,
//     vector, colour and matrix fields map onto X3DFieldValue; SFDouble/SFTime
//     narrow to float, SFRotation becomes (x,y,z,angle) and SFVec*d/SFMatrix*d
//     narrow to float. SFNode/MF fields are listed with an empty value: the
//     descriptor has no texture or array channel.
#ifndef X3D_RUNTIME_EXTRACT_SHADER_EXTRACT_HPP
#define X3D_RUNTIME_EXTRACT_SHADER_EXTRACT_HPP

#include "AssetResolver.hpp"
#include "DynamicField.hpp"
#include "RenderItem.hpp"
#include "ShaderOptions.hpp"
#include "x3d/nodes/Appearance.hpp"
#include "x3d/nodes/ComposedShader.hpp"
#include "x3d/nodes/ProgramShader.hpp"
#include "x3d/nodes/ShaderPart.hpp"
#include "x3d/nodes/ShaderProgram.hpp"
#include "x3d/nodes/X3DShaderNode.hpp"
#include "x3d/nodes/X3DUrlObject.hpp"

#include <any>
#include <cctype>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace x3d::runtime::extract {

using namespace x3d::core;

/// One evaluated entry of an Appearance's shaders list.
struct ShaderCandidate {
  const X3DNode *node = nullptr;
  bool supported = false; // node type + language this host runs
  bool valid = false;     // meaningful only when supported
  bool pending = false;   // a url is still loading; retry later
  std::string error;
};

/// selectShader()'s result. `candidates` lists entries in order up to and
/// including the selected one; later entries were not evaluated.
struct ShaderSelection {
  const X3DNode *selected = nullptr;
  std::optional<ShaderProgramDesc> program; // set iff selected
  std::vector<ShaderCandidate> candidates;
};

namespace shader_detail {

inline std::string upperAscii(std::string s) {
  for (char &c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  return s;
}

inline std::optional<ShaderStageDesc::Stage> stageOf(const std::string &type) {
  using S = ShaderStageDesc::Stage;
  const std::string t = upperAscii(type);
  if (t == "VERTEX") return S::Vertex;
  if (t == "FRAGMENT") return S::Fragment;
  if (t == "GEOMETRY") return S::Geometry;
  if (t == "TESS_CONTROL") return S::TessControl;
  if (t == "TESS_EVALUATION") return S::TessEval;
  return std::nullopt;
}

inline int hexValue(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

inline std::optional<std::string> decodeBase64(const std::string &in) {
  std::string out;
  std::uint32_t acc = 0;
  int bits = 0;
  for (char c : in) {
    int v;
    if (c >= 'A' && c <= 'Z') v = c - 'A';
    else if (c >= 'a' && c <= 'z') v = c - 'a' + 26;
    else if (c >= '0' && c <= '9') v = c - '0' + 52;
    else if (c == '+' || c == '-') v = 62;
    else if (c == '/' || c == '_') v = 63;
    else if (c == '=' || std::isspace(static_cast<unsigned char>(c))) continue;
    else return std::nullopt;
    acc = (acc << 6) | static_cast<std::uint32_t>(v);
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      out.push_back(static_cast<char>((acc >> bits) & 0xFFu));
    }
  }
  return out;
}

/// RFC 2397 `data:[<mediatype>][;base64],<data>`. Nullopt when malformed.
inline std::optional<std::string> decodeDataUrl(const std::string &url) {
  const std::size_t comma = url.find(',');
  if (comma == std::string::npos) return std::nullopt;
  const std::string meta = url.substr(5, comma - 5);
  const std::string payload = url.substr(comma + 1);
  const bool base64 = meta.size() >= 7 &&
                      upperAscii(meta.substr(meta.size() - 7)) == ";BASE64";
  if (base64) return decodeBase64(payload);
  std::string out;
  for (std::size_t i = 0; i < payload.size(); ++i) {
    if (payload[i] == '%' && i + 2 < payload.size()) {
      const int hi = hexValue(payload[i + 1]), lo = hexValue(payload[i + 2]);
      if (hi >= 0 && lo >= 0) {
        out.push_back(static_cast<char>(hi * 16 + lo));
        i += 2;
        continue;
      }
    }
    out.push_back(payload[i]);
  }
  return out;
}

enum class SourceState { Ready, Pending, Failed };

/// Resolve one ShaderPart/ShaderProgram's source text.
inline SourceState resolveSource(const X3DNode &part, const SFString &inlineSource,
                                 const ShaderOptions &opts, std::string &out) {
  if (!inlineSource.empty()) {
    out = inlineSource;
    return SourceState::Ready;
  }
  const auto *uo = dynamic_cast<const x3d::nodes::X3DUrlObject *>(&part);
  if (!uo || !uo->getLoad()) return SourceState::Failed;
  bool pending = false;
  for (const std::string &url : uo->getUrl()) {
    if (url.size() >= 5 && upperAscii(url.substr(0, 5)) == "DATA:") {
      if (auto text = decodeDataUrl(url)) {
        out = std::move(*text);
        return SourceState::Ready;
      }
      continue;
    }
    if (!opts.resolver) continue;
    AssetResult r = opts.resolver(url, AssetKind::Shader);
    if (r.ready()) {
      out.assign(r.bytes.begin(), r.bytes.end());
      return SourceState::Ready;
    }
    // §9.2.1-style fallback: a pending earlier entry blocks the later ones, so
    // the preferred url wins once it arrives.
    if (r.pending()) {
      pending = true;
      break;
    }
  }
  return pending ? SourceState::Pending : SourceState::Failed;
}

inline X3DFieldValue fieldValueOf(X3DFieldType type, const std::any &v) {
  X3DFieldValue out;
  out.type = type;
  if (!v.has_value()) return out;
  using T = X3DFieldType;
  auto as = [&](auto tag) -> const decltype(tag) * {
    return std::any_cast<decltype(tag)>(&v);
  };
  switch (type) {
    case T::SFFloat: if (auto *p = as(SFFloat{})) out.value = *p; break;
    case T::SFDouble: if (auto *p = as(SFDouble{})) out.value = static_cast<float>(*p); break;
    case T::SFTime: if (auto *p = as(SFTime{})) out.value = static_cast<float>(*p); break;
    case T::SFInt32: if (auto *p = as(SFInt32{})) out.value = *p; break;
    case T::SFBool: if (auto *p = as(SFBool{})) out.value = *p; break;
    case T::SFColor: if (auto *p = as(SFColor{})) out.value = *p; break;
    case T::SFColorRGBA: if (auto *p = as(SFColorRGBA{})) out.value = *p; break;
    case T::SFVec2f: if (auto *p = as(SFVec2f{})) out.value = *p; break;
    case T::SFVec3f: if (auto *p = as(SFVec3f{})) out.value = *p; break;
    case T::SFVec4f: if (auto *p = as(SFVec4f{})) out.value = *p; break;
    case T::SFVec2d:
      if (auto *p = as(SFVec2d{}))
        out.value = SFVec2f{static_cast<float>(p->x), static_cast<float>(p->y)};
      break;
    case T::SFVec3d:
      if (auto *p = as(SFVec3d{}))
        out.value = SFVec3f{static_cast<float>(p->x), static_cast<float>(p->y),
                            static_cast<float>(p->z)};
      break;
    case T::SFVec4d:
      if (auto *p = as(SFVec4d{}))
        out.value = SFVec4f{static_cast<float>(p->x), static_cast<float>(p->y),
                            static_cast<float>(p->z), static_cast<float>(p->w)};
      break;
    case T::SFRotation:
      if (auto *p = as(SFRotation{})) out.value = SFVec4f{p->x, p->y, p->z, p->angle};
      break;
    case T::SFMatrix3f: if (auto *p = as(SFMatrix3f{})) out.value = *p; break;
    case T::SFMatrix4f: if (auto *p = as(SFMatrix4f{})) out.value = *p; break;
    case T::SFMatrix3d:
      if (auto *p = as(SFMatrix3d{})) {
        SFMatrix3f m{};
        for (int r = 0; r < 3; ++r)
          for (int c = 0; c < 3; ++c) m.matrix[r][c] = static_cast<float>(p->matrix[r][c]);
        out.value = m;
      }
      break;
    case T::SFMatrix4d:
      if (auto *p = as(SFMatrix4d{})) {
        SFMatrix4f m{};
        for (int r = 0; r < 4; ++r)
          for (int c = 0; c < 4; ++c) m.matrix[r][c] = static_cast<float>(p->matrix[r][c]);
        out.value = m;
      }
      break;
    case T::SFString: if (auto *p = as(SFString{})) out.value = *p; break;
    default: break; // SFNode, SFImage and MF types: no descriptor channel.
  }
  return out;
}

inline void appendFields(const X3DNode &node, const DynamicFieldStore *store,
                         std::vector<ShaderFieldBinding> &out) {
  if (!store) return;
  for (const FieldInfo &f : store->authorFields(node)) {
    ShaderFieldBinding b;
    b.name = f.x3dName;
    b.type = f.type;
    b.access = f.access;
    b.value = fieldValueOf(f.type, store->getValue(node, f.x3dName));
    out.push_back(std::move(b));
  }
}

/// Assemble + validate one supported candidate.
inline void evaluate(const X3DNode &shader, const DynamicFieldStore *store,
                     const ShaderOptions &opts, ShaderCandidate &cand,
                     ShaderProgramDesc &desc) {
  desc.language = "GLSL";
  auto addStage = [&](const X3DNode &part, const SFString &type,
                      const SFString &inlineSource) -> bool {
    const auto stage = stageOf(type);
    if (!stage) {
      cand.error = "unknown part type '" + type + "'";
      return false;
    }
    std::string source;
    switch (resolveSource(part, inlineSource, opts, source)) {
      case SourceState::Ready: break;
      case SourceState::Pending:
        cand.pending = true;
        cand.error = "part source still loading";
        return false;
      case SourceState::Failed:
        cand.error = "part source unavailable";
        return false;
    }
    ShaderStageDesc s;
    s.stage = *stage;
    s.source = std::move(source);
    desc.stages.push_back(std::move(s));
    return true;
  };

  if (auto *cs = dynamic_cast<const x3d::nodes::ComposedShader *>(&shader)) {
    for (const auto &p : cs->getParts()) {
      auto *part = dynamic_cast<const x3d::nodes::ShaderPart *>(p.get());
      if (!part) continue;
      if (!addStage(*part, part->getType(), part->getSourceCode())) return;
    }
    appendFields(shader, store, desc.fields);
  } else if (auto *ps = dynamic_cast<const x3d::nodes::ProgramShader *>(&shader)) {
    for (const auto &p : ps->getPrograms()) {
      auto *prog = dynamic_cast<const x3d::nodes::ShaderProgram *>(p.get());
      if (!prog) continue;
      if (!addStage(*prog, prog->getType(), prog->getSourceCode())) return;
      appendFields(*prog, store, desc.fields);
    }
  }
  if (desc.stages.empty()) {
    cand.error = "no parts";
    return;
  }
  const ShaderValidation v =
      opts.validator ? opts.validator(desc) : structuralShaderValidation(desc);
  cand.valid = v.valid;
  cand.error = v.error;
}

} // namespace shader_detail

/// True for the languages this SDK assembles (GLSL).
inline bool isSupportedShaderLanguage(const std::string &language) {
  return shader_detail::upperAscii(language) == "GLSL";
}

/// §31.2.2.3: the first supported, valid shader in `appearance.shaders`.
inline ShaderSelection selectShader(const X3DNode *appearance,
                                    const DynamicFieldStore *authorFields,
                                    const ShaderOptions &opts) {
  ShaderSelection sel;
  auto *app = dynamic_cast<const x3d::nodes::Appearance *>(appearance);
  if (!app) return sel;
  for (const auto &s : app->getShaders()) {
    auto *shader = dynamic_cast<const x3d::nodes::X3DShaderNode *>(s.get());
    if (!shader) continue;
    ShaderCandidate cand;
    cand.node = s.get();
    cand.supported = (dynamic_cast<const x3d::nodes::ComposedShader *>(shader) ||
                      dynamic_cast<const x3d::nodes::ProgramShader *>(shader)) &&
                     isSupportedShaderLanguage(shader->getLanguage());
    if (!cand.supported) {
      sel.candidates.push_back(std::move(cand));
      continue;
    }
    ShaderProgramDesc desc;
    shader_detail::evaluate(*s, authorFields, opts, cand, desc);
    const bool chosen = cand.valid;
    sel.candidates.push_back(cand);
    if (chosen) {
      desc.isSelected = true;
      desc.isValid = true;
      sel.selected = s.get();
      sel.program = std::move(desc);
      break;
    }
  }
  return sel;
}

} // namespace x3d::runtime::extract

#endif // X3D_RUNTIME_EXTRACT_SHADER_EXTRACT_HPP
