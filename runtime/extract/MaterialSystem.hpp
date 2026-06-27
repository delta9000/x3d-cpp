// MaterialSystem.hpp — M2.5 extraction (Layer A4): an Appearance node -> a tagged
// MaterialDesc + its TextureRefs. namespace x3d::runtime::extract. Header-only,
// golden-untouched, node-as-truth (every read goes through the geombounds
// reflection helpers / nodeTypeName() dispatch; adds NO members to generated
// nodes; keyed only by const X3DNode*).
//
// SCOPE (locked by the M2.5 design spec, 2026-06-14):
//   * materialOf(appearance) dispatches on the material node's nodeTypeName():
//       - Material         -> Phong   (diffuse/emissive/specular/shininess/
//                                       ambientIntensity/transparency)
//       - PhysicalMaterial -> Physical(baseColor/metallic/roughness/emissive/
//                                       occlusionStrength/transparency)
//       - UnlitMaterial    -> Unlit   (emissive as baseColor/transparency)
//   * null material under a PRESENT Appearance => Phong 0.8 grey (the spec
//     default Material). null Appearance entirely => Unlit white (an always-draws
//     debug fallback; non-spec, matches the SceneExtractor stub).
//   * alphaMode/alphaCutoff are read off the Appearance (alphaMode via the
//     reflection token string — no dependency on the generated enum type).
//   * toRGBA() alpha = 1 - transparency lives on MaterialDesc (RenderItem.hpp);
//     this file only fills `transparency` (and baseColor RGB).
//   * texturesOf(appearance) resolves textures with MATERIAL-SLOT PRECEDENCE over
//     the legacy Appearance.texture (locked D6): a material-borne texture slot
//     (e.g. Material.diffuseTexture / PhysicalMaterial.baseTexture) wins; only if
//     NO material slot is present is the legacy Appearance.texture surfaced.
//     PixelTexture -> Inline (pixels carried verbatim), MovieTexture -> Movie,
//     MultiTexture -> one TextureRef per channel. URLs are surfaced VERBATIM;
//     bytes are NOT loaded here (asset resolution is OUTSIDE the SDK).
#ifndef X3D_RUNTIME_EXTRACT_MATERIAL_SYSTEM_HPP
#define X3D_RUNTIME_EXTRACT_MATERIAL_SYSTEM_HPP

#include "GeometryBounds.hpp" // geombounds::getField/getNode/hasField
#include "RenderItem.hpp"     // MaterialDesc / TextureRef / SamplerParams
#include "TextureExtract.hpp" // extendedSamplerOf (§18.4.9 sampler descriptor)
#include "x3d/nodes/X3DNode.hpp"
#include "x3d/core/X3Dtypes.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace x3d::runtime::extract {
using namespace x3d::core;

namespace matsys {

// Read an SFEnum field as its X3D token string via FieldInfo::getEnumString, so
// this stays decoupled from the generated enum-class type. Returns dflt if the
// field is absent / not an enum / unreadable.
inline std::string getEnumToken(const X3DNode &n, const char *name,
                                const std::string &dflt) {
  for (const auto &f : n.fields())
    if (f.x3dName == name) {
      if (!f.getEnumString) return dflt;
      try {
        std::string s = f.getEnumString(n);
        return s.empty() ? dflt : s;
      } catch (...) {
        return dflt;
      }
    }
  return dflt;
}

// Map an X3D alphaMode token to the descriptor AlphaMode. AUTO => Opaque (the
// renderer infers from transparency); unknown tokens fall through to Opaque.
inline AlphaMode alphaModeFromToken(const std::string &t) {
  if (t == "MASK") return AlphaMode::Mask;
  if (t == "BLEND") return AlphaMode::Blend;
  return AlphaMode::Opaque; // AUTO / OPAQUE / unknown.
}

// Sampler parameters off an optional TextureProperties node. Defaults match the
// SamplerParams defaults when no TextureProperties is present.
inline SamplerParams samplerOf(const std::shared_ptr<X3DNode> &texNode) {
  SamplerParams s;
  if (!texNode) return s;
  s.repeatS = geombounds::getField<bool>(*texNode, "repeatS", true);
  s.repeatT = geombounds::getField<bool>(*texNode, "repeatT", true);
  if (auto tp = geombounds::getNode(*texNode, "textureProperties")) {
    s.generateMipmaps =
        geombounds::getField<bool>(*tp, "generateMipMaps", true);
    s.anisotropicDegree =
        geombounds::getField<float>(*tp, "anisotropicDegree", 1.0f);
  }
  return s;
}

// Read the X3D v4 xxxTextureMapping field from a material node for a given
// texture field name. The mapping field is <textureFieldName> + "Mapping" by
// X3D v4 convention: baseTexture -> baseTextureMapping, etc. Returns empty
// string (= UV set 0) when the mapping field is absent or has no value.
inline SFString mappingOf(const X3DNode &materialNode, const char *textureFieldName) {
  std::string mappingName = std::string(textureFieldName) + "Mapping";
  return geombounds::getField<SFString>(materialNode, mappingName.c_str(), SFString{});
}

// Resolve ONE concrete texture node into a TextureRef for the given slot.
// PixelTexture -> Inline (pixels verbatim), MovieTexture -> Movie, everything
// else (ImageTexture and the long tail) -> Url with the url list verbatim.
inline TextureRef refOf(const std::shared_ptr<X3DNode> &texNode,
                        TextureRef::Slot slot) {
  TextureRef ref;
  ref.slot = slot;
  if (!texNode) return ref;
  const std::string t = texNode->nodeTypeName();
  ref.sampler = samplerOf(texNode);
  ref.repeatS = ref.sampler.repeatS;
  ref.repeatT = ref.sampler.repeatT;
  // T-TEX (v1-closure): full §18.4.9 sampler descriptor (TextureProperties
  // boundary/filter/mipmap/anisotropy) co-located with the legacy sampler read.
  ref.extSampler = extendedSamplerOf(texNode);

  if (t == "PixelTexture") {
    ref.source = TextureRef::Source::Inline;
    ref.inlinePixels = geombounds::getField<SFImage>(*texNode, "image", {});
  } else if (t == "MovieTexture") {
    ref.source = TextureRef::Source::Movie;
    ref.url = geombounds::getField<MFString>(*texNode, "url", {});
  } else {
    // ImageTexture and the long tail: a URL list, surfaced verbatim.
    ref.source = TextureRef::Source::Url;
    ref.url = geombounds::getField<MFString>(*texNode, "url", {});
  }
  return ref;
}

// Append the texture from a material's SFNode slot, if present, into `out`.
// Returns true if a node was found in that slot (regardless of MultiTexture
// expansion), so the caller can record that a material slot was populated.
// Reads the X3D v4 xxxTextureMapping field (fieldName + "Mapping") and stores
// it in each TextureRef::texCoordMapping; empty = UV set 0.
inline bool appendSlot(const X3DNode &materialNode, const char *fieldName,
                       TextureRef::Slot slot, std::vector<TextureRef> &out) {
  if (!geombounds::hasField(materialNode, fieldName)) return false;
  auto node = geombounds::getNode(materialNode, fieldName);
  if (!node) return false;
  SFString mapping = mappingOf(materialNode, fieldName);
  if (node->nodeTypeName() == "MultiTexture") {
    // Expand a MultiTexture into one TextureRef per channel, carrying the stage.
    auto kids = geombounds::getField<std::vector<std::shared_ptr<X3DNode>>>(
        *node, "texture", {});
    int channel = 0;
    for (const auto &k : kids) {
      if (!k) continue;
      TextureRef r = refOf(k, slot);
      r.channel = channel++;
      r.texCoordMapping = mapping;
      out.push_back(std::move(r));
    }
  } else {
    TextureRef r = refOf(node, slot);
    r.texCoordMapping = mapping;
    out.push_back(std::move(r));
  }
  return true;
}

} // namespace matsys

// Forward decl: materialOf fills MaterialDesc::textures via texturesOf (defined
// below) so the two free functions can reference each other.
inline std::vector<TextureRef> texturesOf(const X3DNode *appearance);

// ---------------------------------------------------------------------------
// materialOf — resolve an Appearance node to a tagged MaterialDesc.
//
//   * appearance == nullptr        => Unlit white  (always-draws debug fallback).
//   * Appearance present, material null => Phong 0.8 grey (spec default Material).
//   * Material         => Phong.
//   * PhysicalMaterial => Physical.
//   * UnlitMaterial    => Unlit.
// alphaMode/alphaCutoff are read off the Appearance regardless of material type.
// ---------------------------------------------------------------------------
inline MaterialDesc materialOf(const X3DNode *appearance) {
  using namespace matsys;
  MaterialDesc m;

  if (!appearance) {
    // Null Appearance entirely => Unlit white (matches the SceneExtractor stub).
    m.model = MaterialModel::Unlit;
    m.emissive = SFColor{1.0f, 1.0f, 1.0f};
    return m;
  }

  // alphaMode / alphaCutoff off the Appearance (independent of material type).
  m.alphaMode = alphaModeFromToken(getEnumToken(*appearance, "alphaMode", "AUTO"));
  m.alphaCutoff = geombounds::getField<float>(*appearance, "alphaCutoff", 0.5f);

  auto material = geombounds::getNode(*appearance, "material");
  if (!material) {
    // Present Appearance, material=NULL => equivalent to a default UnlitMaterial:
    // lighting off, unlit object colour white (1,1,1), and Appearance.texture acts
    // as the EMISSIVE (unlit) texture (MAT-001; §12.4.2, §12.2.5 rule 4).
    m.model = MaterialModel::Unlit;
    m.emissive = SFColor{1.0f, 1.0f, 1.0f};
    for (TextureRef tr : texturesOf(appearance)) {
      tr.slot = TextureRef::Slot::Emissive; // unlit: legacy texture is emissive
      m.textures.push_back(tr);
    }
    return m;
  }

  const std::string t = material->nodeTypeName();
  if (t == "PhysicalMaterial") {
    m.model = MaterialModel::Physical;
    SFColor base = geombounds::getField<SFColor>(*material, "baseColor",
                                                 SFColor{1.0f, 1.0f, 1.0f});
    m.transparency = geombounds::getField<float>(*material, "transparency", 0.0f);
    m.physical.baseColor = base;
    m.physical.metallic = geombounds::getField<float>(*material, "metallic", 1.0f);
    m.physical.roughness = geombounds::getField<float>(*material, "roughness", 1.0f);
    m.physical.occlusionStrength =
        geombounds::getField<float>(*material, "occlusionStrength", 1.0f);
    m.emissive = geombounds::getField<SFColor>(*material, "emissiveColor",
                                               SFColor{0.0f, 0.0f, 0.0f});
  } else if (t == "UnlitMaterial") {
    m.model = MaterialModel::Unlit;
    SFColor emis = geombounds::getField<SFColor>(*material, "emissiveColor",
                                                 SFColor{1.0f, 1.0f, 1.0f});
    m.transparency = geombounds::getField<float>(*material, "transparency", 0.0f);
    // Unlit: the emissive color IS the surface color; toRGBA() reads emissive.
    m.emissive = emis;
  } else {
    // Material (Phong) — also the safe default for any other material-ish node.
    m.model = MaterialModel::Phong;
    SFColor diffuse = geombounds::getField<SFColor>(*material, "diffuseColor",
                                                    SFColor{0.8f, 0.8f, 0.8f});
    m.transparency = geombounds::getField<float>(*material, "transparency", 0.0f);
    m.phong.diffuse = diffuse;
    m.emissive = geombounds::getField<SFColor>(*material, "emissiveColor",
                                               SFColor{0.0f, 0.0f, 0.0f});
    m.phong.specular = geombounds::getField<SFColor>(*material, "specularColor",
                                               SFColor{0.0f, 0.0f, 0.0f});
    m.phong.shininess = geombounds::getField<float>(*material, "shininess", 0.2f);
    m.phong.ambientIntensity =
        geombounds::getField<float>(*material, "ambientIntensity", 0.2f);
    m.phong.occlusionStrength =
        geombounds::getField<float>(*material, "occlusionStrength", 1.0f); // MAT-004
  }

  // normalScale (X3DOneSidedMaterialNode, all lit/unlit one-sided materials) — MAT-005.
  m.normalScale = geombounds::getField<float>(*material, "normalScale", 1.0f);

  m.textures = texturesOf(appearance);

  // backMaterial (X3D v4 Appearance.backMaterial — MAT-006).
  if (auto backNode = geombounds::getNode(*appearance, "backMaterial")) {
    MaterialDesc bm;
    const std::string bt = backNode->nodeTypeName();
    if (bt == "PhysicalMaterial") {
      bm.model = MaterialModel::Physical;
      SFColor base = geombounds::getField<SFColor>(*backNode, "baseColor", SFColor{1.0f, 1.0f, 1.0f});
      bm.transparency = geombounds::getField<float>(*backNode, "transparency", 0.0f);
      bm.physical.baseColor = base;
      bm.physical.metallic = geombounds::getField<float>(*backNode, "metallic", 1.0f);
      bm.physical.roughness = geombounds::getField<float>(*backNode, "roughness", 1.0f);
      bm.physical.occlusionStrength = geombounds::getField<float>(*backNode, "occlusionStrength", 1.0f);
      bm.emissive = geombounds::getField<SFColor>(*backNode, "emissiveColor", SFColor{0.0f, 0.0f, 0.0f});
    } else if (bt == "UnlitMaterial") {
      bm.model = MaterialModel::Unlit;
      bm.emissive = geombounds::getField<SFColor>(*backNode, "emissiveColor", SFColor{1.0f, 1.0f, 1.0f});
      bm.transparency = geombounds::getField<float>(*backNode, "transparency", 0.0f);
    } else {
      // Material (Phong) default.
      bm.model = MaterialModel::Phong;
      SFColor diffuse = geombounds::getField<SFColor>(*backNode, "diffuseColor", SFColor{0.8f, 0.8f, 0.8f});
      bm.transparency = geombounds::getField<float>(*backNode, "transparency", 0.0f);
      bm.phong.diffuse = diffuse;
      bm.phong.specular = geombounds::getField<SFColor>(*backNode, "specularColor", SFColor{0,0,0});
      bm.phong.shininess = geombounds::getField<float>(*backNode, "shininess", 0.2f);
      bm.phong.ambientIntensity = geombounds::getField<float>(*backNode, "ambientIntensity", 0.2f);
      bm.phong.occlusionStrength = geombounds::getField<float>(*backNode, "occlusionStrength", 1.0f);
      bm.emissive = geombounds::getField<SFColor>(*backNode, "emissiveColor", SFColor{0.0f, 0.0f, 0.0f});
    }
    bm.normalScale = geombounds::getField<float>(*backNode, "normalScale", 1.0f);
    // Constraint check (design §2): backMaterial must be the same model type as
    // the front. Surfaced as a diagnostic; not enforced by the SDK.
    // NOTE: same-texture-slot-set check is deferred — bm.textures is not
    // populated at this point (Appearance-level textures are front-only), so
    // comparing against bm.textures would always compare N vs 0 (inert).
    m.backMaterialConstraintMet = (bm.model == m.model);
    m.backMaterial = std::make_unique<MaterialDesc>(std::move(bm));
    m.doubleSided = true;
  }

  return m;
}

// ---------------------------------------------------------------------------
// texturesOf — collect the Appearance's textures with MATERIAL-SLOT PRECEDENCE
// over the legacy Appearance.texture (locked D6).
//
//   * A material-borne texture slot wins: Material exposes diffuse/emissive/
//     normal/occlusion/specular textures; PhysicalMaterial exposes base/
//     metallicRoughness/normal/emissive/occlusion; UnlitMaterial exposes
//     emissive/normal. MultiTexture in any slot expands to one TextureRef per
//     channel.
//   * ONLY if no material slot is populated is the legacy Appearance.texture
//     surfaced (as a BaseColor slot).
// ---------------------------------------------------------------------------
inline std::vector<TextureRef> texturesOf(const X3DNode *appearance) {
  using namespace matsys;
  std::vector<TextureRef> out;
  if (!appearance) return out;

  bool anyMaterialSlot = false;
  if (auto material = geombounds::getNode(*appearance, "material")) {
    const std::string t = material->nodeTypeName();
    if (t == "PhysicalMaterial") {
      anyMaterialSlot |= appendSlot(*material, "baseTexture",
                                    TextureRef::Slot::BaseColor, out);
      anyMaterialSlot |= appendSlot(*material, "metallicRoughnessTexture",
                                    TextureRef::Slot::MetallicRoughness, out);
      anyMaterialSlot |= appendSlot(*material, "normalTexture",
                                    TextureRef::Slot::Normal, out);
      anyMaterialSlot |= appendSlot(*material, "emissiveTexture",
                                    TextureRef::Slot::Emissive, out);
      anyMaterialSlot |= appendSlot(*material, "occlusionTexture",
                                    TextureRef::Slot::Occlusion, out);
    } else if (t == "UnlitMaterial") {
      anyMaterialSlot |= appendSlot(*material, "emissiveTexture",
                                    TextureRef::Slot::Emissive, out);
      anyMaterialSlot |= appendSlot(*material, "normalTexture",
                                    TextureRef::Slot::Normal, out);
    } else {
      // Material (Phong) slots.
      anyMaterialSlot |= appendSlot(*material, "diffuseTexture",
                                    TextureRef::Slot::Diffuse, out);
      anyMaterialSlot |= appendSlot(*material, "emissiveTexture",
                                    TextureRef::Slot::Emissive, out);
      anyMaterialSlot |= appendSlot(*material, "normalTexture",
                                    TextureRef::Slot::Normal, out);
      anyMaterialSlot |= appendSlot(*material, "specularTexture",
                                    TextureRef::Slot::Specular, out);
      anyMaterialSlot |= appendSlot(*material, "occlusionTexture",
                                    TextureRef::Slot::Occlusion, out);
      anyMaterialSlot |= appendSlot(*material, "shininessTexture",
                                    TextureRef::Slot::Shininess, out); // MAT-002
      anyMaterialSlot |= appendSlot(*material, "ambientTexture",
                                    TextureRef::Slot::Ambient, out);   // MAT-003
    }
  }

  // Legacy Appearance.texture ONLY when no material slot was populated.
  if (!anyMaterialSlot)
    appendSlot(*appearance, "texture", TextureRef::Slot::BaseColor, out);

  return out;
}

} // namespace x3d::runtime::extract
#endif // X3D_RUNTIME_EXTRACT_MATERIAL_SYSTEM_HPP
