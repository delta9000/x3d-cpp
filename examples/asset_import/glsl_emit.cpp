#include "glsl_emit.hpp"
#include "generated/usd_preview_surface_embed.hpp"

#include <iomanip>
#include <map>
#include <regex>
#include <sstream>

namespace x3d::asset_import {
namespace {

// Formats a float as a GLSL literal that always carries a decimal point
// (GLSL requires this to distinguish a float literal from an int literal).
std::string glslFloat(float v) {
  std::ostringstream oss;
  oss << std::setprecision(9) << v;
  std::string s = oss.str();
  if (s.find('.') == std::string::npos && s.find('e') == std::string::npos &&
      s.find("inf") == std::string::npos && s.find("nan") == std::string::npos) {
    s += ".0";
  }
  return s;
}

std::string glslVec3(const Vec3& v) {
  return "vec3(" + glslFloat(v.x) + ", " + glslFloat(v.y) + ", " + glslFloat(v.z) + ")";
}

std::string glslVec4(const Vec4& v) {
  return "vec4(" + glslFloat(v.x) + ", " + glslFloat(v.y) + ", " + glslFloat(v.z) + ", " + glslFloat(v.w) + ")";
}

// ImportMaterial::AlphaMode enum order is {Opaque, Mask, Blend}, but the
// canonical shader's uAlphaMode convention (usd_preview_surface.frag) is
// 0=Opaque, 1=Blend, 2=Mask — NOT the same order, so map explicitly.
int glslAlphaMode(AlphaMode mode) {
  switch (mode) {
    case AlphaMode::Opaque: return 0;
    case AlphaMode::Blend: return 1;
    case AlphaMode::Mask: return 2;
  }
  return 0;
}

// One `uniform` -> `const` substitution.
struct BakedConst {
  std::string type;   // GLSL type as declared in the canonical shader
  std::string value;  // literal expression
};

// Builds the name -> const-value table for one material. Only the
// UsdPreviewSurface material-scalar uniforms are baked; sampler2D texture
// slots, uHasColors, and the light uniforms are left as `uniform`
// (host-bound at runtime) — see glsl_emit.hpp.
std::map<std::string, BakedConst> bakedConstsFor(const ImportMaterial& mat) {
  Vec4 baseColor;
  float metallic;
  float roughness;
  if (mat.pbr) {
    baseColor = Vec4{mat.pbr->baseColor.x, mat.pbr->baseColor.y, mat.pbr->baseColor.z, mat.opacity};
    metallic = mat.pbr->metallic;
    roughness = mat.pbr->roughness;
  } else {
    baseColor = Vec4{mat.diffuse.x, mat.diffuse.y, mat.diffuse.z, mat.opacity};
    metallic = 0.0f;
    roughness = 0.5f;
  }
  const float alphaCutoff = mat.opacityThreshold > 0.0f ? mat.opacityThreshold : 0.5f;

  std::map<std::string, BakedConst> out;
  out["uBaseColor"] = {"vec4", glslVec4(baseColor)};
  out["uMetallic"] = {"float", glslFloat(metallic)};
  out["uRoughness"] = {"float", glslFloat(roughness)};
  out["uEmissive"] = {"vec3", glslVec3(mat.emissive)};
  out["uNormalScale"] = {"float", glslFloat(1.0f)};
  out["uOcclusionStrength"] = {"float", glslFloat(1.0f)};
  out["uAlphaMode"] = {"int", std::to_string(glslAlphaMode(mat.alpha))};
  out["uAlphaCutoff"] = {"float", glslFloat(alphaCutoff)};
  out["uUseSpecularWorkflow"] = {"int", mat.useSpecularWorkflow ? "1" : "0"};
  out["uSpecularColor"] = {"vec3", glslVec3(mat.specular)};
  out["uIor"] = {"float", glslFloat(mat.ior)};
  out["uClearcoat"] = {"float", glslFloat(mat.clearcoat)};
  out["uClearcoatRoughness"] = {"float", glslFloat(mat.clearcoatRoughness)};
  out["uOpacityMode"] = {"int", std::to_string(static_cast<int>(mat.opacityMode))};
  out["uOpacityThreshold"] = {"float", glslFloat(mat.opacityThreshold)};
  out["uHasBaseColorTex"] = {"int", mat.textures.baseColor ? "1" : "0"};
  out["uHasNormalTex"] = {"int", mat.textures.normal ? "1" : "0"};
  out["uHasEmissiveTex"] = {"int", mat.textures.emissive ? "1" : "0"};
  out["uHasMetallicRoughnessTex"] = {"int", mat.textures.metallicRoughness ? "1" : "0"};
  out["uHasOcclusionTex"] = {"int", mat.textures.occlusion ? "1" : "0"};
  return out;
}

// Matches a single-line `uniform TYPE NAME;` (or `NAME[N];`) declaration,
// capturing indent/type/name/array-suffix/trailing text (e.g. a `//` comment)
// so non-baked declarations (sampler2D slots, light arrays, uHasColors) are
// left untouched and baked ones can be rewritten in place, comment included.
const std::regex kUniformLine(R"(^(\s*)uniform\s+(\w+)\s+(\w+)(\[[0-9]+\])?\s*;(.*)$)");

} // namespace

std::string emitMaterialGlsl(const ImportMaterial& mat) {
  const auto baked = bakedConstsFor(mat);

  std::istringstream in(kUsdPreviewSurfaceFragSource);
  std::ostringstream out;
  std::string line;
  bool first = true;
  while (std::getline(in, line)) {
    if (!first) out << '\n';
    first = false;

    std::smatch m;
    if (std::regex_match(line, m, kUniformLine)) {
      const std::string indent = m[1].str();
      const std::string name = m[3].str();
      const std::string arraySuffix = m[4].str();
      const std::string trailing = m[5].str();
      auto it = baked.find(name);
      // Only rewrite scalar (non-array) declarations that are in the bake
      // table; array uniforms (uLightDirEye[8], uLightColor[8]) and
      // sampler2D/uHasColors are never in the table, so they pass through
      // to the fallback append below, unchanged.
      if (it != baked.end() && arraySuffix.empty()) {
        out << indent << "const " << it->second.type << " " << name << " = "
            << it->second.value << ";" << trailing;
        continue;
      }
    }
    out << line;
  }
  return out.str();
}

} // namespace x3d::asset_import
