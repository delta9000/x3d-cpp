// ShaderUniformVocabulary.hpp — typed portability surface for author-shader
// uniform name resolution.
//
// The vocabulary maps well-known uniform names (modelViewMatrix, baseColor,
// lightColor[], etc.) to their semantic source in the extraction descriptors,
// their GLSL type, and whether they are array-typed. A consumer calls
// buildBindingPlan() (ShaderBindingPlan.hpp) to classify each uniform declared
// in a ComposedShader into vocab / author-field / unrecognized buckets.
//
// This header is standalone (no RenderItem.hpp include); it only needs
// X3DReflection.hpp for X3DFieldType and <string_view>.
#ifndef X3D_RUNTIME_EXTRACT_SHADER_UNIFORM_VOCABULARY_HPP
#define X3D_RUNTIME_EXTRACT_SHADER_UNIFORM_VOCABULARY_HPP

#include "x3d/core/X3DReflection.hpp"  // X3DFieldType
#include <cstdint>
#include <string_view>

namespace x3d::runtime::extract::vocab {

enum class UniformSource : uint8_t {
  ModelViewMatrix, ProjectionMatrix, NormalMatrix, ModelViewProjectionMatrix,
  ModelMatrix, ViewMatrix, TextureMatrix,
  NumLights, LightType, LightColor, LightDirection, LightLocation,
  LightAttenuation, LightRadius, LightAmbientIntensity,
  LightBeamWidth, LightCutOffAngle, LightGlobal, LightScopeRoot,
  EnvDiffuse, EnvSpecular, EnvSH, BrdfLUT, EnvIntensity, EnvRotation,
  BaseColor, DiffuseColor, SpecularColor, EmissiveColor,
  Metallic, Roughness, Shininess, AmbientIntensity,
  OcclusionStrength, NormalScale, Transparency, AlphaMode, AlphaCutoff,
  BaseColorTex, DiffuseTex, SpecularTex, EmissiveTex, NormalTex,
  OcclusionTex, MetallicRoughnessTex, ShininessTex, AmbientTex,
  HasBaseColorTex, HasDiffuseTex, HasSpecularTex, HasEmissiveTex,
  HasNormalTex, HasOcclusionTex, HasMetallicRoughnessTex,
  HasShininessTex, HasAmbientTex,
  FogColor, FogType, FogRange, FogVisibilityRange,
  Time, ViewportSize, NearFar,
  NumClipPlanes, ClipPlane,
  Unrecognized,
};

struct VocabEntry {
  std::string_view name;
  UniformSource source;
  X3DFieldType glsl_type;
  bool is_array;
  std::string_view doc;
};

inline constexpr VocabEntry kVocabulary[] = {
  // --- Transform matrices --------------------------------------------------
  {"modelViewMatrix",     UniformSource::ModelViewMatrix,     X3DFieldType::SFMatrix4f, false,
   "RenderItem.worldTransform * CameraDesc.viewMatrix"},
  {"projectionMatrix",    UniformSource::ProjectionMatrix,    X3DFieldType::SFMatrix4f, false,
   "CameraDesc perspective/ortho projection"},
  {"normalMatrix",        UniformSource::NormalMatrix,        X3DFieldType::SFMatrix3f, false,
   "inverse-transpose of view*model"},
  {"modelViewProjectionMatrix", UniformSource::ModelViewProjectionMatrix, X3DFieldType::SFMatrix4f, false,
   "proj * view * model"},
  {"modelMatrix",         UniformSource::ModelMatrix,         X3DFieldType::SFMatrix4f, false,
   "RenderItem.worldTransform"},
  {"viewMatrix",          UniformSource::ViewMatrix,          X3DFieldType::SFMatrix4f, false,
   "CameraDesc.viewMatrix"},
  {"textureMatrix",       UniformSource::TextureMatrix,       X3DFieldType::SFMatrix4f, false,
   "TextureTransform2D baked matrix"},
  // --- Lights --------------------------------------------------------------
  {"numLights",           UniformSource::NumLights,           X3DFieldType::SFInt32,   false,
   "active LightDesc count"},
  {"lightType",           UniformSource::LightType,           X3DFieldType::SFInt32,   true,
   "LightDesc::Type enum"},
  {"lightColor",          UniformSource::LightColor,          X3DFieldType::SFColor,   true,
   "LightDesc.color * intensity"},
  {"lightDirection",      UniformSource::LightDirection,      X3DFieldType::SFVec3f,   true,
   "LightDesc.worldDirection (eye-space)"},
  {"lightLocation",       UniformSource::LightLocation,       X3DFieldType::SFVec3f,   true,
   "LightDesc.worldLocation (eye-space)"},
  {"lightAttenuation",    UniformSource::LightAttenuation,    X3DFieldType::SFVec3f,   true,
   "LightDesc.attenuation"},
  {"lightRadius",         UniformSource::LightRadius,         X3DFieldType::SFFloat,   true,
   "LightDesc.radius"},
  {"lightAmbientIntensity", UniformSource::LightAmbientIntensity, X3DFieldType::SFFloat, true,
   "LightDesc.ambientIntensity"},
  {"lightBeamWidth",      UniformSource::LightBeamWidth,      X3DFieldType::SFFloat,   true,
   "LightDesc.beamWidth"},
  {"lightCutOffAngle",    UniformSource::LightCutOffAngle,    X3DFieldType::SFFloat,   true,
   "LightDesc.cutOffAngle"},
  // --- Material (Phong) ----------------------------------------------------
  {"diffuseColor",        UniformSource::DiffuseColor,        X3DFieldType::SFColor,   false,
   "MaterialDesc.phong.diffuse"},
  {"specularColor",       UniformSource::SpecularColor,       X3DFieldType::SFColor,   false,
   "MaterialDesc.phong.specular"},
  {"emissiveColor",       UniformSource::EmissiveColor,       X3DFieldType::SFColor,   false,
   "MaterialDesc.emissive"},
  {"shininess",           UniformSource::Shininess,           X3DFieldType::SFFloat,   false,
   "MaterialDesc.phong.shininess"},
  {"ambientIntensity",    UniformSource::AmbientIntensity,    X3DFieldType::SFFloat,   false,
   "MaterialDesc.phong.ambientIntensity"},
  // --- Material (Physical) -------------------------------------------------
  {"baseColor",           UniformSource::BaseColor,           X3DFieldType::SFColor,   false,
   "MaterialDesc.physical.baseColor"},
  {"metallic",            UniformSource::Metallic,            X3DFieldType::SFFloat,   false,
   "MaterialDesc.physical.metallic"},
  {"roughness",           UniformSource::Roughness,           X3DFieldType::SFFloat,   false,
   "MaterialDesc.physical.roughness"},
  // --- Material (shared) ---------------------------------------------------
  {"occlusionStrength",   UniformSource::OcclusionStrength,   X3DFieldType::SFFloat,   false,
   "MaterialDesc.phong/physical.occlusionStrength"},
  {"normalScale",         UniformSource::NormalScale,         X3DFieldType::SFFloat,   false,
   "MaterialDesc.normalScale"},
  {"transparency",        UniformSource::Transparency,        X3DFieldType::SFFloat,   false,
   "MaterialDesc.transparency"},
  {"alphaMode",           UniformSource::AlphaMode,           X3DFieldType::SFInt32,   false,
   "MaterialDesc.alphaMode enum"},
  {"alphaCutoff",         UniformSource::AlphaCutoff,         X3DFieldType::SFFloat,   false,
   "MaterialDesc.alphaCutoff"},
  // --- Texture samplers ----------------------------------------------------
  {"baseColorTex",        UniformSource::BaseColorTex,        X3DFieldType::SFNode,    false,
   "TextureRef slot BaseColor sampler"},
  {"diffuseTex",          UniformSource::DiffuseTex,          X3DFieldType::SFNode,    false,
   "TextureRef slot Diffuse sampler"},
  {"specularTex",         UniformSource::SpecularTex,         X3DFieldType::SFNode,    false,
   "TextureRef slot Specular sampler"},
  {"normalTex",           UniformSource::NormalTex,           X3DFieldType::SFNode,    false,
   "TextureRef slot Normal sampler"},
  {"emissiveTex",         UniformSource::EmissiveTex,         X3DFieldType::SFNode,    false,
   "TextureRef slot Emissive sampler"},
  {"occlusionTex",        UniformSource::OcclusionTex,        X3DFieldType::SFNode,    false,
   "TextureRef slot Occlusion sampler"},
  {"metallicRoughnessTex",UniformSource::MetallicRoughnessTex,X3DFieldType::SFNode,    false,
   "TextureRef slot MetallicRoughness sampler"},
  {"shininessTex",        UniformSource::ShininessTex,        X3DFieldType::SFNode,    false,
   "TextureRef slot Shininess sampler"},
  {"ambientTex",          UniformSource::AmbientTex,          X3DFieldType::SFNode,    false,
   "TextureRef slot Ambient sampler"},
  // --- Texture presence flags ----------------------------------------------
  {"hasBaseColorTex",     UniformSource::HasBaseColorTex,     X3DFieldType::SFInt32,   false,
   "1 if BaseColor texture bound, else 0"},
  {"hasDiffuseTex",       UniformSource::HasDiffuseTex,       X3DFieldType::SFInt32,   false,
   "1 if Diffuse texture bound, else 0"},
  {"hasSpecularTex",      UniformSource::HasSpecularTex,      X3DFieldType::SFInt32,   false,
   "1 if Specular texture bound, else 0"},
  {"hasNormalTex",        UniformSource::HasNormalTex,        X3DFieldType::SFInt32,   false,
   "1 if Normal texture bound, else 0"},
  {"hasEmissiveTex",      UniformSource::HasEmissiveTex,      X3DFieldType::SFInt32,   false,
   "1 if Emissive texture bound, else 0"},
  {"hasOcclusionTex",     UniformSource::HasOcclusionTex,     X3DFieldType::SFInt32,   false,
   "1 if Occlusion texture bound, else 0"},
  {"hasMetallicRoughnessTex", UniformSource::HasMetallicRoughnessTex, X3DFieldType::SFInt32, false,
   "1 if MetallicRoughness texture bound, else 0"},
  {"hasShininessTex",     UniformSource::HasShininessTex,     X3DFieldType::SFInt32,   false,
   "1 if Shininess texture bound, else 0"},
  {"hasAmbientTex",       UniformSource::HasAmbientTex,       X3DFieldType::SFInt32,   false,
   "1 if Ambient texture bound, else 0"},
  // --- Environment / IBL ---------------------------------------------------
  {"envDiffuse",          UniformSource::EnvDiffuse,          X3DFieldType::SFNode,    false,
   "LightDesc.environment.diffuseTexture cubemap"},
  {"envSpecular",         UniformSource::EnvSpecular,         X3DFieldType::SFNode,    false,
   "LightDesc.environment.specularTexture cubemap"},
  {"envSH",               UniformSource::EnvSH,               X3DFieldType::MFColor,   false,
   "LightDesc.environment.diffuseCoefficients[9]"},
  {"brdfLUT",             UniformSource::BrdfLUT,             X3DFieldType::SFNode,    false,
   "consumer-managed BRDF integration LUT"},
  {"envIntensity",        UniformSource::EnvIntensity,        X3DFieldType::SFFloat,   false,
   "LightDesc.environment.intensity"},
  {"envRotation",         UniformSource::EnvRotation,         X3DFieldType::SFVec4f,   false,
   "LightDesc.environment.rotation (quaternion)"},
  // --- Fog -----------------------------------------------------------------
  {"fogColor",            UniformSource::FogColor,            X3DFieldType::SFColor,   false,
   "FogDesc.color"},
  {"fogType",             UniformSource::FogType,             X3DFieldType::SFInt32,   false,
   "FogDesc.fogType enum (LINEAR=0, EXPONENTIAL=1)"},
  {"fogRange",            UniformSource::FogRange,            X3DFieldType::SFFloat,   false,
   "FogDesc.visibilityRange / max fog distance"},
  {"fogVisibilityRange",  UniformSource::FogVisibilityRange,  X3DFieldType::SFFloat,   false,
   "FogDesc.visibilityRange"},
  // --- Clip planes ---------------------------------------------------------
  {"numClipPlanes",       UniformSource::NumClipPlanes,       X3DFieldType::SFInt32,   false,
   "active ClipPlane count"},
  {"clipPlane",           UniformSource::ClipPlane,           X3DFieldType::SFVec4f,   true,
   "ClipPlane equation (ax+by+cz+d=0, eye-space)"},
  // --- Per-frame / viewport ------------------------------------------------
  {"time",                UniformSource::Time,                X3DFieldType::SFFloat,   false,
   "wall-clock seconds since viewer start"},
  {"viewportSize",        UniformSource::ViewportSize,        X3DFieldType::SFVec2f,   false,
   "viewport width/height in pixels"},
  {"nearFar",             UniformSource::NearFar,             X3DFieldType::SFVec2f,   false,
   "CameraDesc.nearPlane, farPlane"},
};

} // namespace x3d::runtime::extract::vocab
#endif // X3D_RUNTIME_EXTRACT_SHADER_UNIFORM_VOCABULARY_HPP
