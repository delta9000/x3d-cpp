// MaterialShader.hpp — CPU ports of the PoC's three GLSL material programs,
// covering ALL THREE MaterialModels the extraction seam emits:
//
//   * Unlit    (unlit.frag)  — UnlitMaterial / lines / points / normal-less.
//   * Phong    (lit.frag)    — Material: Blinn-Phong + textures + normal map.
//   * Physical (pbr.frag)    — PhysicalMaterial: metallic-roughness GGX BRDF.
//
// Each is written against glsl.hpp so it reads as a near-verbatim transcription
// of the GLSL — this is the "GLSL emulation" of the fixed-function programs:
// same lighting math, same texture-slot semantics, same screen-space-derivative
// TBN normal mapping, same sRGB output encode, so a CPU frame matches the GL PoC.
//
// Each factory returns a Rasterizer::FragmentShader closure capturing the
// resolved material + lights + textures by value.
//
// Out-of-SDK consumer code. namespace x3d::cpuraster.
#ifndef X3D_CPURASTER_MATERIAL_SHADER_HPP
#define X3D_CPURASTER_MATERIAL_SHADER_HPP

#include "RenderItem.hpp"
#include "Rasterizer.hpp"
#include "Texture.hpp"
#include "glsl.hpp"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace x3d::cpuraster {

namespace ex = x3d::runtime::extract;

// A light reduced to what the lit/pbr shaders consume, in eye space, with the
// RGB color premultiplied by intensity. Mirrors the PoC's EyeLight, extended to
// carry positional (PointLight/SpotLight) lights as well as directional ones.
//
//   * Directional (positional=false): `dirEye` is the eye-space direction of
//     TRAVEL; the light vector L = normalize(-dirEye) is constant per fragment.
//   * Positional (positional=true): `posEye` is the eye-space light position;
//     per fragment L = normalize(posEye - fragPosEye), with X3D distance
//     attenuation 1/max(c0 + c1·d + c2·d², 1) and no contribution past `radius`.
//   * Spot (isSpot=true): additionally cones the contribution about `spotDirEye`
//     (eye-space beam axis, direction of travel) — full inside `beamWidth`,
//     linear falloff to zero at `cutOffAngle` (radians).
struct EyeLight {
  glsl::vec3 dirEye{0, 0, -1};
  glsl::vec3 color{1, 1, 1};
  bool positional = false;
  glsl::vec3 posEye{0, 0, 0};
  glsl::vec3 attenuation{1, 0, 0}; // X3D (c0,c1,c2)
  float radius = 100.0f;
  bool isSpot = false;
  glsl::vec3 spotDirEye{0, 0, -1};
  float beamWidth = 1.5708f;
  float cutOffAngle = 0.7854f;
};

namespace detail {
// Resolve a light at a fragment: returns false when the light does not reach it
// (beyond `radius`, or outside the spot `cutOffAngle`). On success sets `L` (unit
// vector from the fragment toward the light) and `atten` (the scalar multiplier
// folding distance attenuation and any spot-cone falloff).
inline bool resolveLight(const EyeLight &Lt, const glsl::vec3 &posEye,
                         glsl::vec3 &L, float &atten) {
  if (!Lt.positional) {
    L = glsl::normalize(-Lt.dirEye);
    atten = 1.0f;
    return true;
  }
  const glsl::vec3 toLight = Lt.posEye - posEye;
  const float dist = glsl::length(toLight);
  if (dist > Lt.radius) return false;
  L = (dist > 1e-6f) ? toLight * (1.0f / dist) : glsl::vec3{0, 0, 1};
  const float denom = Lt.attenuation.x + Lt.attenuation.y * dist +
                      Lt.attenuation.z * dist * dist;
  atten = 1.0f / glsl::maxf(denom, 1.0f);
  if (Lt.isSpot) {
    // Angle between the beam axis (direction of travel) and the light→fragment
    // direction (-L).
    const float cosAng =
        glsl::dot(glsl::normalize(Lt.spotDirEye), -L);
    const float ang = std::acos(glsl::clampf(cosAng, -1.0f, 1.0f));
    if (ang >= Lt.cutOffAngle) return false;
    if (ang > Lt.beamWidth && Lt.cutOffAngle > Lt.beamWidth)
      atten *= (Lt.cutOffAngle - ang) / (Lt.cutOffAngle - Lt.beamWidth);
  }
  return true;
}
} // namespace detail

// The texture set a material can bind, pre-resolved to CPU samplers with the
// correct color space per slot (matches the PoC's sRGB-vs-linear bind rules).
struct MaterialTextures {
  Texture base;      // BaseColor/Diffuse — sRGB.
  Texture normal;    // Normal map — linear.
  Texture emissive;  // Emissive — sRGB.
  Texture specular;  // Specular — sRGB.
  Texture mr;        // MetallicRoughness ORM — linear.
  Texture occlusion; // Occlusion — linear.
  // §18.4.8 TextureCoordinateGenerator(Sphere): when set, UVs are generated from
  // the camera-space normal rather than the authored texcoords (sphere/reflection
  // map). Applies to the textured-surface coordinate set (base/emissive/specular).
  bool sphereGen = false;
};

namespace detail {
// Sphere-map UV from the (front-facing-corrected, normalized) camera-space
// normal: u = Nx/2 + 0.5, v = Ny/2 + 0.5 (§18.4.8 SPHERE).
inline glsl::vec2 sphereGenUv(const glsl::vec3 &normalEye, bool frontFacing) {
  glsl::vec3 n = glsl::normalize(normalEye);
  if (!frontFacing) n = -n;
  return glsl::vec2{n.x * 0.5f + 0.5f, n.y * 0.5f + 0.5f};
}
} // namespace detail

inline const ex::TextureRef *
findSlot(const ex::MaterialDesc &m,
         std::initializer_list<ex::TextureRef::Slot> slots) {
  for (const ex::TextureRef &t : m.textures)
    for (ex::TextureRef::Slot s : slots)
      if (t.slot == s) return &t;
  return nullptr;
}

inline MaterialTextures buildTextures(const ex::MaterialDesc &m) {
  MaterialTextures tx;
  using Slot = ex::TextureRef::Slot;
  if (const auto *r = findSlot(m, {Slot::BaseColor, Slot::Diffuse})) {
    tx.base = Texture::fromRef(*r, /*srgb=*/true);
    tx.sphereGen =
        r->hasTexCoordGen && r->texCoordGen.mode == ex::TexCoordGenMode::Sphere;
  }
  if (const auto *r = findSlot(m, {Slot::Normal}))
    tx.normal = Texture::fromRef(*r, /*srgb=*/false);
  if (const auto *r = findSlot(m, {Slot::Emissive}))
    tx.emissive = Texture::fromRef(*r, /*srgb=*/true);
  if (const auto *r = findSlot(m, {Slot::Specular}))
    tx.specular = Texture::fromRef(*r, /*srgb=*/true);
  if (const auto *r = findSlot(m, {Slot::MetallicRoughness}))
    tx.mr = Texture::fromRef(*r, /*srgb=*/false);
  if (const auto *r = findSlot(m, {Slot::Occlusion}))
    tx.occlusion = Texture::fromRef(*r, /*srgb=*/false);
  return tx;
}

namespace detail {

// Tangent-space normal mapping via screen-space derivative TBN — the exact
// approach in lit.frag/pbr.frag (no precomputed tangents). Returns the perturbed
// eye-space normal, or Ngeo unchanged when the UV jacobian is degenerate.
inline glsl::vec3 applyNormalMap(const FragmentInput &f, glsl::vec3 Ngeo,
                                 const Texture &normalTex, float normalScale) {
  if (!normalTex.valid()) return Ngeo;
  glsl::vec3 tsN = normalTex.sample(f.texcoord).xyz() * 2.0f - glsl::vec3(1.0f);
  tsN.x *= normalScale;
  tsN.y *= normalScale;
  tsN = glsl::normalize(tsN);
  const glsl::vec2 dUVdx = f.dTexDx, dUVdy = f.dTexDy;
  const float det = dUVdx.x * dUVdy.y - dUVdx.y * dUVdy.x;
  if (std::fabs(det) <= 1e-6f) return Ngeo;
  glsl::vec3 T = glsl::normalize(
      (f.dPosEyeDx * dUVdy.y - f.dPosEyeDy * dUVdx.y) / det);
  T = glsl::normalize(T - glsl::dot(T, Ngeo) * Ngeo); // Gram-Schmidt.
  glsl::vec3 B = glsl::cross(Ngeo, T);
  return glsl::normalize(T * tsN.x + B * tsN.y + Ngeo * tsN.z);
}

// GGX helpers — identical to pbr.frag.
constexpr float kPI = 3.14159265358979323846f;
inline float D_GGX(float NdotH, float alpha2) {
  float denom = NdotH * NdotH * (alpha2 - 1.0f) + 1.0f;
  return alpha2 / (kPI * denom * denom);
}
inline float V_SmithGGX(float NdotL, float NdotV, float alpha2) {
  float GV = NdotL * std::sqrt(NdotV * NdotV * (1.0f - alpha2) + alpha2);
  float GL = NdotV * std::sqrt(NdotL * NdotL * (1.0f - alpha2) + alpha2);
  return 0.5f / glsl::maxf(GV + GL, 1e-5f);
}
inline glsl::vec3 F_Schlick(float VdotH, glsl::vec3 F0) {
  float f = 1.0f - VdotH;
  float f2 = f * f;
  return F0 + (glsl::vec3(1.0f) - F0) * (f2 * f2 * f);
}

} // namespace detail

// ---------------------------------------------------------------------------
// UNLIT (unlit.frag): per-vertex Color or material baseColor; no lighting.
// alpha = 1 - transparency carried on baseColor.a. Used for UnlitMaterial AND
// the lines/points/normal-less paths (caller forces it).
// ---------------------------------------------------------------------------
inline FragmentShader makeUnlitShader(const ex::MaterialDesc &m, bool hasColors) {
  const glsl::vec4 baseColor = glsl::vec4(m.toRGBA());
  return [=](const FragmentInput &f, glsl::vec4 &out) -> bool {
    glsl::vec3 rgb = hasColors ? f.color.xyz() : baseColor.xyz();
    float a = hasColors ? f.color.w : baseColor.w;
    out = glsl::vec4(rgb, a);
    return true;
  };
}

// ---------------------------------------------------------------------------
// PHONG (lit.frag): Blinn-Phong, two-sided, textures, normal map, sRGB output.
// ---------------------------------------------------------------------------
inline FragmentShader makePhongShader(const ex::MaterialDesc &m,
                                      std::vector<EyeLight> lights,
                                      bool hasColors) {
  const glsl::vec4 uDiffuse = glsl::vec4(m.toRGBA());
  const glsl::vec3 uEmissive = glsl::vec3(m.emissive);
  const float ai = m.phong.ambientIntensity;
  const glsl::vec3 uAmbient = uDiffuse.xyz() * ai;
  const glsl::vec3 uSpecular = glsl::vec3(m.phong.specular);
  const float uShininess = m.phong.shininess;
  // NB: compare the enum directly. The PoC GLSL hard-codes `uAlphaMode == 2`
  // for MASK, but AlphaMode is {Opaque=0, Mask=1, Blend=2} — so the int check
  // is a latent off-by-one. We discard on the actual Mask enum (correct).
  const bool maskMode = (m.alphaMode == ex::AlphaMode::Mask);
  const float alphaCutoff = m.alphaCutoff;
  const float normalScale = m.normalScale;
  const MaterialTextures tx = buildTextures(m);

  return [=](const FragmentInput &f, glsl::vec4 &out) -> bool {
    const glsl::vec2 uv =
        tx.sphereGen ? detail::sphereGenUv(f.normalEye, f.frontFacing) : f.texcoord;
    glsl::vec3 base = hasColors ? f.color.xyz() : uDiffuse.xyz();
    float alpha = uDiffuse.w;
    if (tx.base.valid()) {
      glsl::vec4 texel = tx.base.sample(uv);
      base = base * texel.xyz();
      alpha *= texel.w;
    }
    if (maskMode && alpha < alphaCutoff) return false; // MASK discard.

    glsl::vec3 Ngeo = glsl::normalize(f.normalEye);
    if (!f.frontFacing) Ngeo = -Ngeo;
    glsl::vec3 N = detail::applyNormalMap(f, Ngeo, tx.normal, normalScale);

    glsl::vec3 emissive = uEmissive;
    if (tx.emissive.valid()) emissive = emissive * tx.emissive.sample(uv).xyz();
    glsl::vec3 specCol = uSpecular;
    if (tx.specular.valid()) specCol = specCol * tx.specular.sample(uv).xyz();

    const glsl::vec3 V = glsl::normalize(-f.posEye);
    const float expo = glsl::maxf(uShininess * 128.0f, 1.0f);
    glsl::vec3 lit = uAmbient * base + emissive;
    for (const EyeLight &Lt : lights) {
      glsl::vec3 L;
      float atten;
      if (!detail::resolveLight(Lt, f.posEye, L, atten)) continue;
      float ndl = glsl::maxf(glsl::dot(N, L), 0.0f);
      lit = lit + base * Lt.color * (ndl * atten);
      if (ndl > 0.0f) {
        glsl::vec3 H = glsl::normalize(L + V);
        float ndh = glsl::maxf(glsl::dot(N, H), 0.0f);
        lit = lit + specCol * Lt.color * (std::pow(ndh, expo) * atten);
      }
    }
    lit = glsl::linearToSRGB(lit); // Phong enables gamma output (PoC parity).
    out = glsl::vec4(lit, alpha);
    return true;
  };
}

// ---------------------------------------------------------------------------
// PHYSICAL (pbr.frag): metallic-roughness analytic GGX over directional and
// positional (point/spot) lights (no IBL, matching the PoC). Two-sided, normal
// map, sRGB output.
// ---------------------------------------------------------------------------
inline FragmentShader makePbrShader(const ex::MaterialDesc &m,
                                    std::vector<EyeLight> lights, bool hasColors) {
  const glsl::vec4 uBaseColor =
      glsl::vec4(m.physical.baseColor.r, m.physical.baseColor.g,
                 m.physical.baseColor.b, 1.0f - m.transparency);
  const float uMetallic = m.physical.metallic;
  const float uRoughness = m.physical.roughness;
  const glsl::vec3 uEmissive = glsl::vec3(m.emissive);
  const float normalScale = m.normalScale;
  const float occlusionStrength = m.physical.occlusionStrength;
  const bool maskMode = (m.alphaMode == ex::AlphaMode::Mask);
  const float alphaCutoff = m.alphaCutoff;
  const MaterialTextures tx = buildTextures(m);

  return [=](const FragmentInput &f, glsl::vec4 &out) -> bool {
    // Sphere-map UV (reflection-style) for the base colour when the geometry
    // bound a TextureCoordinateGenerator(Sphere). ORM/occlusion stay on the
    // authored coords (they are packed material maps, not a reflection set).
    const glsl::vec2 uv =
        tx.sphereGen ? detail::sphereGenUv(f.normalEye, f.frontFacing) : f.texcoord;
    glsl::vec4 baseCol = uBaseColor;
    if (hasColors) { baseCol.x = f.color.x; baseCol.y = f.color.y; baseCol.z = f.color.z; }
    if (tx.base.valid()) baseCol = baseCol * tx.base.sample(uv);
    float alpha = baseCol.w;
    if (maskMode && alpha < alphaCutoff) return false; // MASK.

    float metallic = uMetallic, roughness = uRoughness;
    if (tx.mr.valid()) {
      glsl::vec3 orm = tx.mr.sample(f.texcoord).xyz();
      roughness *= orm.y; // G
      metallic *= orm.z;  // B
    }
    float alpha2 = glsl::maxf(roughness * roughness, 0.001f);
    alpha2 = alpha2 * alpha2;

    float ao = 1.0f;
    if (tx.mr.valid()) ao = tx.mr.sample(f.texcoord).x;       // ORM R.
    else if (tx.occlusion.valid()) ao = tx.occlusion.sample(f.texcoord).x;
    ao = glsl::mixf(1.0f, ao, occlusionStrength);

    glsl::vec3 Ngeo = glsl::normalize(f.normalEye);
    if (!f.frontFacing) Ngeo = -Ngeo;
    glsl::vec3 N = detail::applyNormalMap(f, Ngeo, tx.normal, normalScale);

    const glsl::vec3 V = glsl::normalize(-f.posEye);
    const float NdV = glsl::maxf(glsl::dot(N, V), 0.0f);
    const glsl::vec3 F0 = glsl::mix(glsl::vec3(0.04f), baseCol.xyz(), metallic);
    const glsl::vec3 diffColor = (1.0f - metallic) * baseCol.xyz();

    glsl::vec3 emissive = uEmissive;
    if (tx.emissive.valid()) emissive = emissive * tx.emissive.sample(uv).xyz();

    glsl::vec3 color = emissive;
    for (const EyeLight &Lt : lights) {
      glsl::vec3 L;
      float atten;
      if (!detail::resolveLight(Lt, f.posEye, L, atten)) continue;
      float NdL = glsl::maxf(glsl::dot(N, L), 0.0f);
      if (NdL <= 0.0f) continue;
      glsl::vec3 H = glsl::normalize(L + V);
      float NdH = glsl::maxf(glsl::dot(N, H), 0.0f);
      float VdH = glsl::maxf(glsl::dot(V, H), 0.0f);
      float D = detail::D_GGX(NdH, alpha2);
      float Vis = detail::V_SmithGGX(NdL, NdV, alpha2);
      glsl::vec3 F = detail::F_Schlick(VdH, F0);
      glsl::vec3 spec = D * Vis * F;
      glsl::vec3 kD = (glsl::vec3(1.0f) - F) * (1.0f - metallic);
      color = color + (kD * diffColor / detail::kPI + spec) * Lt.color * (NdL * atten);
    }
    color = color + 0.03f * diffColor * ao; // small ambient term (pbr.frag).
    color = glsl::linearToSRGB(color);
    out = glsl::vec4(color, alpha);
    return true;
  };
}

// One material model -> its fragment shader (no two-sided wrapping).
inline FragmentShader makeModelShader(const ex::MaterialDesc &m,
                                      const std::vector<EyeLight> &lights,
                                      bool hasColors, bool forceUnlit) {
  if (forceUnlit || m.model == ex::MaterialModel::Unlit)
    return makeUnlitShader(m, hasColors);
  if (m.model == ex::MaterialModel::Physical)
    return makePbrShader(m, lights, hasColors);
  return makePhongShader(m, lights, hasColors);
}

// ---------------------------------------------------------------------------
// Dispatcher: pick the shader for a material model. `forceUnlit` is the B4
// consumer contract (lines/points/normal-less route to Unlit regardless of
// model); the caller computes it from topology + hasNormals.
//
// X3D v4 Appearance.backMaterial (MAT-006): when present and model-compatible,
// back-facing fragments are shaded with the back material. The per-model shaders
// already flip the geometric normal for !frontFacing, so each side lights
// correctly.
// ---------------------------------------------------------------------------
inline FragmentShader makeMaterialShader(const ex::MaterialDesc &m,
                                         const std::vector<EyeLight> &lights,
                                         bool hasColors, bool forceUnlit) {
  FragmentShader front = makeModelShader(m, lights, hasColors, forceUnlit);
  if (m.backMaterial && m.backMaterialConstraintMet) {
    FragmentShader back =
        makeModelShader(*m.backMaterial, lights, hasColors, forceUnlit);
    return [front, back](const FragmentInput &f, glsl::vec4 &out) -> bool {
      return f.frontFacing ? front(f, out) : back(f, out);
    };
  }
  return front;
}

} // namespace x3d::cpuraster

#endif // X3D_CPURASTER_MATERIAL_SHADER_HPP
