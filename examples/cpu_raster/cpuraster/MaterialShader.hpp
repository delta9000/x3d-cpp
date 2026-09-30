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
// TBN normal mapping, same colour-space rules (ADR-0027: Phong/Unlit in display
// space, PhysicalMaterial linear with sRGB output), so a CPU frame matches the GL PoC.
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

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <string>
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
  // §17.2.2.4 per-light ambientIntensity. Scales only the ambient term, gated
  // by attenuation/spot like the rest of the light's contribution.
  float ambientIntensity = 0.0f;
  bool positional = false;
  glsl::vec3 posEye{0, 0, 0};
  glsl::vec3 attenuation{1, 0, 0}; // X3D (c0,c1,c2)
  float radius = 100.0f;
  bool isSpot = false;
  glsl::vec3 spotDirEye{0, 0, -1};
  float beamWidth = 1.5708f;
  float cutOffAngle = 0.7854f;
};

// The bound Fog reduced to what the fragment shaders consume (§24.4.2). The
// viewer distance passed to applyFog is the eye-space length of the fragment
// position; `visibilityRange` is already in world units (the extractor scaled
// the node-local field by the Fog's world scale). visibilityRange <= 0 disables
// fog (no blending).
struct FogParams {
  glsl::vec3 color{1, 1, 1};
  int type = 0; // 0 = LINEAR, 1 = EXPONENTIAL.
  float visibilityRange = 0.0f;
};

namespace detail {
// §17 Table 17.5 fogInterpolant(d) then applyFog. d = eye-space distance to the
// viewer, V = visibilityRange. LINEAR: f = (V-d)/V for d<V else 0. EXPONENTIAL:
// f = exp(-d/(V-d)) for d<V else 0. result = f*color + (1-f)*fogColor. A
// non-positive V disables fog entirely (returns `color` unchanged).
inline glsl::vec3 applyFog(const glsl::vec3 &color, float d,
                           const FogParams &fog) {
  const float V = fog.visibilityRange;
  if (V <= 0.0f) return color; // fog disabled.
  float f = 0.0f;              // d >= V => fully fogged.
  if (d < V)
    f = (fog.type == 1) ? std::exp(-d / (V - d)) : (V - d) / V;
  return color * f + fog.color * (1.0f - f);
}
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
  Texture mr;        // MetallicRoughness — G=roughness, B=metallic (linear).
  Texture occlusion; // Occlusion — R channel (linear); AO source (§12.4.6).
  // §18.4.3 MultiTexture: the ordered stages bound to the base-colour slot,
  // combined by detail::combineBaseStages. Empty for an ordinary single texture
  // (which still binds `base`). MODULATE with a white FACTOR reduces to the
  // previous single-texture multiply, so a one-stage scene is unchanged.
  struct MultiStage {
    Texture tex;
    std::string mode = "MODULATE"; // §18.4.3 mode; OpenGL TexEnv operator.
    std::string source;            // DEFAULT/DIFFUSE/SPECULAR/FACTOR.
    std::string function;          // COMPLEMENT/ALPHAREPLICATE.
    glsl::vec4 factor{1, 1, 1, 1}; // MultiTexture color.rgb + alpha.
  };
  std::vector<MultiStage> baseStages;
  // §18.4.8 TextureCoordinateGenerator: when set, UVs are generated per fragment
  // from eye-space state rather than the authored texcoords. Applies to the
  // textured-surface coordinate set (base/emissive/specular). TXF-2 covers the
  // view-dependent modes: SPHERE / CAMERASPACENORMAL / CAMERASPACEPOSITION /
  // CAMERASPACEREFLECTIONVECTOR.
  bool hasTexCoordGen = false;
  ex::TexCoordGenMode texCoordGenMode = ex::TexCoordGenMode::Sphere;
};

namespace detail {
// §18.4.8 TextureCoordinateGenerator UVs from eye-space state. `normalEye` is the
// front-facing-corrected, normalized camera-space normal; `posEye` the camera-
// space position (eye at the origin, so normalize(posEye) = eye→fragment).
//   SPHERE                      : u = Nx/2+0.5, v = Ny/2+0.5.
//   CAMERASPACENORMAL           : (Nx, Ny).
//   CAMERASPACEPOSITION         : (Px, Py).
//   CAMERASPACEREFLECTIONVECTOR : R = reflect(−V, N) = 2·dot(V,N)·N − V → (Rx, Ry).
inline glsl::vec2 texCoordGenUv(ex::TexCoordGenMode mode, const glsl::vec3 &posEye,
                                const glsl::vec3 &normalEye, bool frontFacing) {
  using Mode = ex::TexCoordGenMode;
  glsl::vec3 n = glsl::normalize(normalEye);
  if (!frontFacing) n = -n;
  switch (mode) {
    case Mode::Sphere:
      return glsl::vec2{n.x * 0.5f + 0.5f, n.y * 0.5f + 0.5f};
    case Mode::CameraSpaceNormal:
      return glsl::vec2{n.x, n.y};
    case Mode::CameraSpacePosition:
      return glsl::vec2{posEye.x, posEye.y};
    case Mode::CameraSpaceReflectionVector: {
      const glsl::vec3 V = glsl::normalize(posEye);
      const glsl::vec3 R = glsl::reflect(-V, n);
      return glsl::vec2{R.x, R.y};
    }
    default:
      return glsl::vec2{n.x * 0.5f + 0.5f, n.y * 0.5f + 0.5f}; // SPHERE default.
  }
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

// `linearWorkflow`: PhysicalMaterial decodes its colour textures from sRGB and
// shades in linear; Phong stays in display space and samples them as authored
// (ADR-0027). Data textures (normal, metallic-roughness, occlusion) are never
// decoded.
inline MaterialTextures buildTextures(const ex::MaterialDesc &m, bool linearWorkflow) {
  MaterialTextures tx;
  using Slot = ex::TextureRef::Slot;
  const bool colour = linearWorkflow;
  // §18.4.3 MultiTexture: gather every base-colour-slot stage in channel order.
  // A plain single texture yields a one-stage list, and MODULATE-with-white
  // reproduces the old `base = base * texel` path exactly.
  std::vector<const ex::TextureRef *> baseRefs;
  for (const ex::TextureRef &t : m.textures)
    if (t.slot == Slot::BaseColor || t.slot == Slot::Diffuse) baseRefs.push_back(&t);
  std::stable_sort(baseRefs.begin(), baseRefs.end(),
                   [](const ex::TextureRef *a, const ex::TextureRef *b) {
                     return a->channel < b->channel;
                   });
  for (const ex::TextureRef *r : baseRefs) {
    MaterialTextures::MultiStage st;
    st.tex = Texture::fromRef(*r, colour);
    st.mode = r->multiMode;
    st.source = r->multiSource;
    st.function = r->multiFunction;
    st.factor = {r->multiColor.r, r->multiColor.g, r->multiColor.b, r->multiAlpha};
    tx.baseStages.push_back(std::move(st));
  }
  if (!baseRefs.empty()) {
    tx.base = tx.baseStages.front().tex;
    tx.hasTexCoordGen = baseRefs.front()->hasTexCoordGen;
    tx.texCoordGenMode = baseRefs.front()->texCoordGen.mode;
  }
  if (const auto *r = findSlot(m, {Slot::Normal}))
    tx.normal = Texture::fromRef(*r, /*srgb=*/false);
  if (const auto *r = findSlot(m, {Slot::Emissive}))
    tx.emissive = Texture::fromRef(*r, /*srgb=*/colour);
  if (const auto *r = findSlot(m, {Slot::Specular}))
    tx.specular = Texture::fromRef(*r, /*srgb=*/colour);
  if (const auto *r = findSlot(m, {Slot::MetallicRoughness}))
    tx.mr = Texture::fromRef(*r, /*srgb=*/false);
  if (const auto *r = findSlot(m, {Slot::Occlusion}))
    tx.occlusion = Texture::fromRef(*r, /*srgb=*/false);
  return tx;
}

namespace detail {

// ---------------------------------------------------------------------------
// §18.4.3 MultiTexture stage combiner. The X3D `mode` names are the OpenGL /
// D3D fixed-function texture-environment operators, so this follows that
// convention (an ISO-vs-convention caveat is recorded in REQ-MULTITEXTURE):
// arg1 = this stage's texture operand, arg2 = the running accumulation from the
// previous stage (seeded with the surface base colour). Each stage output is
// clamped to [0,1], as the fixed-function pipeline does.
// ---------------------------------------------------------------------------
inline glsl::vec4 clamp01(const glsl::vec4 &v) {
  return {glsl::clampf(v.x, 0.0f, 1.0f), glsl::clampf(v.y, 0.0f, 1.0f),
          glsl::clampf(v.z, 0.0f, 1.0f), glsl::clampf(v.w, 0.0f, 1.0f)};
}
inline glsl::vec4 multiCombine(const std::string &mode, const std::string &func,
                               const glsl::vec4 &arg1, const glsl::vec4 &arg2) {
  glsl::vec4 r;
  if (mode == "OFF") r = arg2;                                    // texture disabled
  else if (mode == "REPLACE" || mode == "SELECTARG1") r = arg1;
  else if (mode == "SELECTARG2") r = arg2;
  else if (mode == "MODULATE2X") r = (arg1 * arg2) * 2.0f;
  else if (mode == "MODULATE4X") r = (arg1 * arg2) * 4.0f;
  else if (mode == "ADD") r = arg1 + arg2;
  else if (mode == "ADDSIGNED") r = arg1 + arg2 - glsl::vec4(0.5f);
  else if (mode == "ADDSIGNED2X") r = (arg1 + arg2 - glsl::vec4(0.5f)) * 2.0f;
  else if (mode == "SUBTRACT") r = arg1 - arg2;
  else r = arg1 * arg2; // MODULATE, the §18.4.3 default (and any unknown token).
  if (func == "COMPLEMENT") r = glsl::vec4(1.0f) - r;         // (1 - x)
  else if (func == "ALPHAREPLICATE") r = glsl::vec4(r.w);     // a -> rgb
  return clamp01(r);
}
// `source` selects how the stage texel forms arg1 (convention, see finding):
// DEFAULT uses the texel; DIFFUSE premultiplies by the surface diffuse colour;
// SPECULAR by the surface specular colour; FACTOR by the MultiTexture
// color/alpha. alpha is taken from the texel (unscaled except for FACTOR).
inline glsl::vec4 multiArg1(const MaterialTextures::MultiStage &st,
                            const glsl::vec4 &texel, const glsl::vec3 &diffuse,
                            const glsl::vec3 &specular) {
  if (st.source == "FACTOR")
    return {texel.x * st.factor.x, texel.y * st.factor.y, texel.z * st.factor.z,
            texel.w * st.factor.w};
  if (st.source == "SPECULAR")
    return {texel.x * specular.x, texel.y * specular.y, texel.z * specular.z, texel.w};
  if (st.source == "DIFFUSE")
    return {texel.x * diffuse.x, texel.y * diffuse.y, texel.z * diffuse.z, texel.w};
  return texel; // DEFAULT / empty.
}
// Fold every base-colour stage into `initial` (the surface base colour) and
// return the combined RGBA. An empty stage list returns `initial` unchanged.
inline glsl::vec4 combineBaseStages(const MaterialTextures &tx,
                                    const glsl::vec4 &initial,
                                    const glsl::vec2 &uv,
                                    const glsl::vec3 &diffuse,
                                    const glsl::vec3 &specular) {
  glsl::vec4 acc = initial;
  for (const auto &st : tx.baseStages) {
    // An unresolved/multisource stage samples white, so MODULATE is a no-op.
    acc = multiCombine(st.mode, st.function,
                       multiArg1(st, st.tex.sample(uv), diffuse, specular), acc);
  }
  return acc;
}

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
inline FragmentShader makeUnlitShader(const ex::MaterialDesc &m, bool hasColors,
                                      const FogParams &fog = {}) {
  const glsl::vec4 baseColor = glsl::vec4(m.toRGBA());
  const MaterialTextures tx = buildTextures(m, /*linearWorkflow=*/false);
  return [=](const FragmentInput &f, glsl::vec4 &out) -> bool {
    glsl::vec3 rgb = hasColors ? f.color.xyz() : baseColor.xyz();
    float a = hasColors ? f.color.w : baseColor.w;
    if (!tx.baseStages.empty()) { // §18.4.3 MultiTexture.
      glsl::vec4 c = detail::combineBaseStages(tx, glsl::vec4(rgb, a), f.texcoord,
                                               rgb, glsl::vec3(1.0f));
      rgb = c.xyz();
      a = c.w;
    }
    // §17: fog is the final step, applied to unlit output too.
    rgb = detail::applyFog(rgb, glsl::length(f.posEye), fog);
    out = glsl::vec4(rgb, a);
    return true;
  };
}

// ---------------------------------------------------------------------------
// PHONG (lit.frag): Blinn-Phong, two-sided, textures, normal map. Display
// space: no sRGB decode or encode, so it matches UnlitMaterial (ADR-0027).
// ---------------------------------------------------------------------------
inline FragmentShader makePhongShader(const ex::MaterialDesc &m,
                                      std::vector<EyeLight> lights,
                                      bool hasColors,
                                      const FogParams &fog = {}) {
  const glsl::vec4 uDiffuse = glsl::vec4(m.toRGBA());
  const glsl::vec3 uEmissive = glsl::vec3(m.emissive);
  const float ai = m.phong.ambientIntensity;
  const glsl::vec3 uSpecular = glsl::vec3(m.phong.specular);
  const float uShininess = m.phong.shininess;
  // Compare the enum directly. AlphaMode is {Opaque=0, Mask=1, Blend=2}; the PoC
  // GLSL now gates MASK on `uAlphaMode == 1` to match (RenderItem.hpp static_asserts
  // pin the wire contract). This path is convention-agnostic — it never touches the int.
  const bool maskMode = (m.alphaMode == ex::AlphaMode::Mask);
  const float alphaCutoff = m.alphaCutoff;
  const float normalScale = m.normalScale;
  const MaterialTextures tx = buildTextures(m, /*linearWorkflow=*/false);

  return [=](const FragmentInput &f, glsl::vec4 &out) -> bool {
    const glsl::vec2 uv =
        tx.hasTexCoordGen
            ? detail::texCoordGenUv(tx.texCoordGenMode, f.posEye, f.normalEye,
                                    f.frontFacing)
            : f.texcoord;
    glsl::vec3 base = hasColors ? f.color.xyz() : uDiffuse.xyz();
    float alpha = uDiffuse.w;
    if (!tx.baseStages.empty()) { // §18.4.3 MultiTexture (one stage == old path).
      glsl::vec4 c = detail::combineBaseStages(
          tx, glsl::vec4(base, alpha), uv,
          hasColors ? f.color.xyz() : uDiffuse.xyz(), uSpecular);
      base = c.xyz();
      alpha = c.w;
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
    glsl::vec3 lit = emissive;
    for (const EyeLight &Lt : lights) {
      glsl::vec3 L;
      float atten;
      if (!detail::resolveLight(Lt, f.posEye, L, atten)) continue;
      // §17 ambient: light.ambientIntensity × ambientParameter, where
      // ambientParameter = material ambientIntensity × diffuseParameter (the
      // textured/vertex-coloured base) — linear in diffuse (ADR-0027). Gated by
      // attenuation/spot like the light's other terms.
      lit = lit + (base * ai) * Lt.color * (Lt.ambientIntensity * atten);
      float ndl = glsl::maxf(glsl::dot(N, L), 0.0f);
      lit = lit + base * Lt.color * (ndl * atten);
      if (ndl > 0.0f) {
        glsl::vec3 H = glsl::normalize(L + V);
        float ndh = glsl::maxf(glsl::dot(N, H), 0.0f);
        lit = lit + specCol * Lt.color * (std::pow(ndh, expo) * atten);
      }
    }
    // §17: fog is the final step, in the shader's output (display) space.
    lit = detail::applyFog(lit, glsl::length(f.posEye), fog);
    out = glsl::vec4(lit, alpha); // display space: no encode (ADR-0027).
    return true;
  };
}

// ---------------------------------------------------------------------------
// PHYSICAL (pbr.frag): metallic-roughness analytic GGX over directional and
// positional (point/spot) lights (no IBL, matching the PoC). Two-sided, normal
// map, sRGB output.
// ---------------------------------------------------------------------------
inline FragmentShader makePbrShader(const ex::MaterialDesc &m,
                                    std::vector<EyeLight> lights, bool hasColors,
                                    const FogParams &fog = {}) {
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
  const MaterialTextures tx = buildTextures(m, /*linearWorkflow=*/true);

  return [=](const FragmentInput &f, glsl::vec4 &out) -> bool {
    // §18.4.8 generated UV for the base colour when the geometry bound a
    // TextureCoordinateGenerator. ORM/occlusion stay on the authored coords (they
    // are packed material maps, not a reflection set).
    const glsl::vec2 uv =
        tx.hasTexCoordGen
            ? detail::texCoordGenUv(tx.texCoordGenMode, f.posEye, f.normalEye,
                                    f.frontFacing)
            : f.texcoord;
    glsl::vec4 baseCol = uBaseColor;
    if (hasColors) { baseCol.x = f.color.x; baseCol.y = f.color.y; baseCol.z = f.color.z; }
    if (!tx.baseStages.empty()) // §18.4.3 MultiTexture (one stage == old path).
      baseCol = detail::combineBaseStages(tx, baseCol, uv, baseCol.xyz(),
                                          glsl::vec3(1.0f));
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

    // AO comes ONLY from the occlusion slot (PhysicalMaterial.occlusionTexture,
    // §12.4.6). The metallic-roughness texture carries roughness in G and
    // metallic in B; its R channel is NOT an occlusion source unless the map is
    // explicitly ORM-packed (the extractor emits no such marker, so we never
    // derive AO from tx.mr). Matches pbr.frag.
    float ao = 1.0f;
    if (tx.occlusion.valid()) ao = tx.occlusion.sample(f.texcoord).x;
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
      // §17.2.2.4 per-light ambient (normal-independent, so applied before the
      // NdL gate below). PhysicalMaterial has no ambientIntensity field, so the
      // ambient surface is diffColor; gated by attenuation/spot like the rest.
      color = color + diffColor * Lt.color * (Lt.ambientIntensity * atten);
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
    // §17: fog is the final step, in the shader's output (display) space,
    // matching lit.frag/unlit.frag.
    color = detail::applyFog(color, glsl::length(f.posEye), fog);
    out = glsl::vec4(color, alpha);
    return true;
  };
}

// One material model -> its fragment shader (no two-sided wrapping).
inline FragmentShader makeModelShader(const ex::MaterialDesc &m,
                                      const std::vector<EyeLight> &lights,
                                      bool hasColors, bool forceUnlit,
                                      const FogParams &fog = {}) {
  if (forceUnlit || m.model == ex::MaterialModel::Unlit)
    return makeUnlitShader(m, hasColors, fog);
  if (m.model == ex::MaterialModel::Physical)
    return makePbrShader(m, lights, hasColors, fog);
  return makePhongShader(m, lights, hasColors, fog);
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
                                         bool hasColors, bool forceUnlit,
                                         const FogParams &fog = {}) {
  FragmentShader front = makeModelShader(m, lights, hasColors, forceUnlit, fog);
  if (m.backMaterial && m.backMaterialConstraintMet) {
    FragmentShader back =
        makeModelShader(*m.backMaterial, lights, hasColors, forceUnlit, fog);
    return [front, back](const FragmentInput &f, glsl::vec4 &out) -> bool {
      return f.frontFacing ? front(f, out) : back(f, out);
    };
  }
  return front;
}

// §12.4.3 overlay: the base shader retains alpha and discard coverage. Hatch
// color replaces only RGB, then receives fog in display space.
inline FragmentShader withFillProperties(FragmentShader shader,
                                         ex::FillPropertiesDesc fill,
                                         FogParams fog) {
  if (!fill.hatched) return shader;
  return [shader, fill, fog](const FragmentInput &f, glsl::vec4 &out) -> bool {
    if (!shader(f, out)) return false;
    if (f.hatch) {
      glsl::vec3 color{fill.hatchColor.r, fill.hatchColor.g, fill.hatchColor.b};
      color = detail::applyFog(color, glsl::length(f.posEye), fog);
      out.x = color.x; out.y = color.y; out.z = color.z;
    }
    return true;
  };
}

} // namespace x3d::cpuraster

#endif // X3D_CPURASTER_MATERIAL_SHADER_HPP
