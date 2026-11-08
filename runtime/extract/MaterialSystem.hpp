// Extract material descriptors from Appearance fields without loading assets.
// Material texture slots take precedence over Appearance.texture. A missing
// material uses default Phong grey; a missing Appearance uses unlit white.
#ifndef X3D_RUNTIME_EXTRACT_MATERIAL_SYSTEM_HPP
#define X3D_RUNTIME_EXTRACT_MATERIAL_SYSTEM_HPP

#include "GeometryBounds.hpp"
#include "RenderItem.hpp"
#include "TextureExtract.hpp"
#include "FieldRead.hpp"
#include "x3d/nodes/X3DNode.hpp"
#include "x3d/core/X3Dtypes.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace x3d::runtime::extract {
using namespace x3d::core;

namespace matsys {

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

// Resolve one texture node into a TextureRef for the given slot, recursively
// retaining cube faces and MultiTexture stages.
inline TextureRef refOf(const std::shared_ptr<X3DNode> &texNode,
                        TextureRef::Slot slot) {
  TextureRef ref;
  ref.slot = slot;
  if (!texNode) return ref;
  ref.sourceNode = texNode;
  const std::string t = texNode->nodeTypeName();
  ref.sampler = samplerOf(texNode);
  ref.repeatS = ref.sampler.repeatS;
  ref.repeatT = ref.sampler.repeatT;
  // full §18.4.9 sampler descriptor (TextureProperties
  // boundary/filter/mipmap/anisotropy) co-located with the legacy sampler read.
  ref.extSampler = extendedSamplerOf(texNode);

  if (t == "PixelTexture") {
    ref.source = TextureRef::Source::Inline;
    ref.inlinePixels = geombounds::getField<SFImage>(*texNode, "image", {});
  } else if (t == "MovieTexture") {
    ref.source = TextureRef::Source::Movie;
    // §9.3.2: load FALSE defers reading and displaying URL content.
    if (geombounds::getField<bool>(*texNode, "load", true))
      ref.url = geombounds::getField<MFString>(*texNode, "url", {});
  } else if (t == "ComposedCubeMapTexture") {
    // §34.4.1: six SFNode face textures, no url ().
    ref.source = TextureRef::Source::Cube;
    for (const char *face : {"frontTexture", "backTexture", "leftTexture", "rightTexture",
                             "topTexture", "bottomTexture"})
      ref.cubeFaces.push_back(refOf(geombounds::getNode(*texNode, face), slot));
  } else if (t == "GeneratedCubeMapTexture") {
    // §34.4.2: rendered at run time by the consumer (TextureRef::generatedCube).
    ref.source = TextureRef::Source::Cube;
    ref.generatedCube.node = texNode.get();
    ref.generatedCube.update = ::x3d::runtime::enumToken(*texNode, "update", "NONE");
    ref.generatedCube.size =
        std::max<int>(1, geombounds::getField<SFInt32>(*texNode, "size", 128));
  } else if (t == "ImageCubeMapTexture") {
    // §34.4.3: one image file holds all six faces (e.g. a DDS cube map). A
    // Source::Cube ref with a url and no cubeFaces; the TextureResolver decodes
    // it to six layers (TexturePixels::layers). load FALSE defers reading.
    ref.source = TextureRef::Source::Cube;
    if (geombounds::getField<bool>(*texNode, "load", true))
      ref.url = geombounds::getField<MFString>(*texNode, "url", {});
  } else if (t == "PixelTexture3D") {
    // §33.4.3: inline voxel data — no IO, materialised verbatim.
    ref.source = TextureRef::Source::Tex3D;
    ref.repeatR = geombounds::getField<bool>(*texNode, "repeatR", true);
    ref.tex3d = pixelTexture3DDesc(
        geombounds::getField<MFInt32>(*texNode, "image", {}));
  } else if (t == "ComposedTexture3D") {
    // §33.4.2: a stack of 2D texture nodes, one per depth slice. Reuses
    // the 2D refOf path per slice; width/height resolve when the slices decode.
    ref.source = TextureRef::Source::Tex3D;
    ref.repeatR = geombounds::getField<bool>(*texNode, "repeatR", true);
    for (const auto &slice : geombounds::getField<MFNode>(*texNode, "texture", {}))
      ref.tex3dSlices.push_back(refOf(slice, slot));
    ref.tex3d.depth = static_cast<uint32_t>(ref.tex3dSlices.size());
  } else if (t == "MultiTexture") {
    ref.source = TextureRef::Source::Multi;
    const auto kids = geombounds::getField<MFNode>(*texNode, "texture", {});
    const auto modes = geombounds::getField<MFString>(*texNode, "mode", {});
    const auto sources = geombounds::getField<MFString>(*texNode, "source", {});
    const auto functions = geombounds::getField<MFString>(*texNode, "function", {});
    const auto color = geombounds::getField<SFColor>(*texNode, "color", {1, 1, 1});
    const auto alpha = geombounds::getField<float>(*texNode, "alpha", 1.0f);
    for (std::size_t i = 0; i < kids.size(); ++i) {
      if (!kids[i]) continue;
      TextureRef stage = refOf(kids[i], slot);
      stage.channel = static_cast<int>(i);
      // §18.4.3: missing stage entries use the specified defaults.
      stage.multiMode = i < modes.size() ? modes[i] : "MODULATE";
      stage.multiSource = i < sources.size() ? sources[i] : "";
      stage.multiFunction = i < functions.size() ? functions[i] : "";
      stage.multiColor = color;
      stage.multiAlpha = alpha;
      ref.multiStages.push_back(std::move(stage));
    }
  } else {
    // ImageTexture and the long tail: a URL list, surfaced verbatim.
    ref.source = TextureRef::Source::Url;
    if (geombounds::getField<bool>(*texNode, "load", true))
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
    // Keep the existing per-channel material shape while carrying §18.4.3 controls.
    TextureRef multi = refOf(node, slot);
    for (auto &r : multi.multiStages) {
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

// Populate `out` with the texture slots authored on a material NODE (the same
// slot dispatch texturesOf uses for the front material). Returns true if any
// slot was populated. Used for both the front material and Appearance.backMaterial
// so the §12.2.3 texture-set match can be compared.
inline bool materialSlotTextures(const X3DNode &material,
                                 std::vector<TextureRef> &out) {
  const std::string t = material.nodeTypeName();
  bool any = false;
  if (t == "PhysicalMaterial") {
    any |= appendSlot(material, "baseTexture", TextureRef::Slot::BaseColor, out);
    any |= appendSlot(material, "metallicRoughnessTexture",
                      TextureRef::Slot::MetallicRoughness, out);
    any |= appendSlot(material, "normalTexture", TextureRef::Slot::Normal, out);
    any |= appendSlot(material, "emissiveTexture", TextureRef::Slot::Emissive, out);
    any |= appendSlot(material, "occlusionTexture", TextureRef::Slot::Occlusion, out);
  } else if (t == "UnlitMaterial") {
    any |= appendSlot(material, "emissiveTexture", TextureRef::Slot::Emissive, out);
    any |= appendSlot(material, "normalTexture", TextureRef::Slot::Normal, out);
  } else {
    // Material (Phong) slots.
    any |= appendSlot(material, "diffuseTexture", TextureRef::Slot::Diffuse, out);
    any |= appendSlot(material, "emissiveTexture", TextureRef::Slot::Emissive, out);
    any |= appendSlot(material, "normalTexture", TextureRef::Slot::Normal, out);
    any |= appendSlot(material, "specularTexture", TextureRef::Slot::Specular, out);
    any |= appendSlot(material, "occlusionTexture", TextureRef::Slot::Occlusion, out);
    any |= appendSlot(material, "shininessTexture", TextureRef::Slot::Shininess, out);
    any |= appendSlot(material, "ambientTexture", TextureRef::Slot::Ambient, out);
  }
  return any;
}

// §12.2.3 backMaterial texture-set match: same (slot, channel) multiset. The
// channel distinguishes MultiTexture stages, so a front MultiTexture of 2 stages
// does not match a back single texture.
inline bool sameTextureSlotSet(const std::vector<TextureRef> &a,
                               const std::vector<TextureRef> &b) {
  if (a.size() != b.size()) return false;
  auto key = [](const TextureRef &t) {
    return static_cast<int>(t.slot) * 64 + t.channel;
  };
  std::vector<int> ka, kb;
  ka.reserve(a.size());
  kb.reserve(b.size());
  for (const auto &t : a) ka.push_back(key(t));
  for (const auto &t : b) kb.push_back(key(t));
  std::sort(ka.begin(), ka.end());
  std::sort(kb.begin(), kb.end());
  return ka == kb;
}

} // namespace matsys

// Forward decl: materialOf fills MaterialDesc::textures via texturesOf (defined
// below) so the two free functions can reference each other.
inline std::vector<TextureRef> texturesOf(const X3DNode *appearance);

// materialOf — resolve an Appearance node to a tagged MaterialDesc.
//
//   * appearance == nullptr        => Unlit white  (always-draws debug fallback).
//   * Appearance present, material null => Phong 0.8 grey (spec default Material).
//   * Material         => Phong.
//   * PhysicalMaterial => Physical.
//   * UnlitMaterial    => Unlit.
// alphaMode/alphaCutoff are read off the Appearance regardless of material type.
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
  m.alphaMode = alphaModeFromToken(::x3d::runtime::enumToken(*appearance, "alphaMode", "AUTO"));
  m.alphaCutoff = geombounds::getField<float>(*appearance, "alphaCutoff", 0.5f);

  // §12.4.3: an absent node does not enable hatching; a present node's
  // filled/hatched fields both default to TRUE.
  if (auto fp = geombounds::getNode(*appearance, "fillProperties")) {
    m.fill.filled = geombounds::getField<bool>(*fp, "filled", true);
    m.fill.hatched = geombounds::getField<bool>(*fp, "hatched", true);
    m.fill.hatchColor = geombounds::getField<SFColor>(
        *fp, "hatchColor", SFColor{1.0f, 1.0f, 1.0f});
    m.fill.hatchStyle = geombounds::getField<int>(*fp, "hatchStyle", 1);
  }

  // §12.4.6 / §12.4.8: LineProperties / PointProperties children of the
  // Appearance — read before the material dispatch so they are
  // surfaced even when the Appearance has no material (the usual lines/points
  // case, which hits the early "default UnlitMaterial" return below).
  if (auto lp = geombounds::getNode(*appearance, "lineProperties")) {
    m.line.applied = geombounds::getField<bool>(*lp, "applied", true);
    m.line.linetype = geombounds::getField<int>(*lp, "linetype", 1);
    m.line.linewidthScaleFactor =
        geombounds::getField<float>(*lp, "linewidthScaleFactor", 0.0f);
  }
  if (auto pp = geombounds::getNode(*appearance, "pointProperties")) {
    m.point.attenuation =
        geombounds::getField<SFVec3f>(*pp, "attenuation", SFVec3f{1.0f, 0.0f, 0.0f});
    m.point.pointSizeScaleFactor =
        geombounds::getField<float>(*pp, "pointSizeScaleFactor", 1.0f);
    m.point.pointSizeMinValue =
        geombounds::getField<float>(*pp, "pointSizeMinValue", 1.0f);
    m.point.pointSizeMaxValue =
        geombounds::getField<float>(*pp, "pointSizeMaxValue", 1.0f);
  }

  auto material = geombounds::getNode(*appearance, "material");
  if (!material) {
    // Present Appearance, material=NULL => equivalent to a default UnlitMaterial:
    // lighting off, unlit object colour white (1,1,1), and Appearance.texture acts
    // as the emissive (unlit) texture (; §12.4.2, §12.2.5 rule 4).
    m.model = MaterialModel::Unlit;
    m.emissive = SFColor{1.0f, 1.0f, 1.0f};
    for (TextureRef tr : texturesOf(appearance)) {
      tr.slot = TextureRef::Slot::Emissive; // unlit: legacy texture is emissive
      m.textures.push_back(tr);
    }
    return m;
  }

  const auto decodeMaterial = [](const X3DNode &node, MaterialDesc &m) {
    const std::string t = node.nodeTypeName();
    if (t == "PhysicalMaterial") {
      m.model = MaterialModel::Physical;
      SFColor base = geombounds::getField<SFColor>(node, "baseColor",
                                                SFColor{1.0f, 1.0f, 1.0f});
      m.transparency = geombounds::getField<float>(node, "transparency", 0.0f);
      m.physical.baseColor = base;
      m.physical.metallic = geombounds::getField<float>(node, "metallic", 1.0f);
      m.physical.roughness = geombounds::getField<float>(node, "roughness", 1.0f);
      m.physical.occlusionStrength =
          geombounds::getField<float>(node, "occlusionStrength", 1.0f);
      m.emissive = geombounds::getField<SFColor>(node, "emissiveColor",
                                               SFColor{0.0f, 0.0f, 0.0f});
    } else if (t == "UnlitMaterial") {
      m.model = MaterialModel::Unlit;
      SFColor emis = geombounds::getField<SFColor>(node, "emissiveColor",
                                                SFColor{1.0f, 1.0f, 1.0f});
      m.transparency = geombounds::getField<float>(node, "transparency", 0.0f);
      // Unlit: the emissive color is the surface color; toRGBA() reads emissive.
      m.emissive = emis;
    } else {
      // Material (Phong) — also the safe default for any other material-ish node.
      m.model = MaterialModel::Phong;
      SFColor diffuse = geombounds::getField<SFColor>(node, "diffuseColor",
                                                   SFColor{0.8f, 0.8f, 0.8f});
      m.transparency = geombounds::getField<float>(node, "transparency", 0.0f);
      m.phong.diffuse = diffuse;
      m.emissive = geombounds::getField<SFColor>(node, "emissiveColor",
                                               SFColor{0.0f, 0.0f, 0.0f});
      m.phong.specular = geombounds::getField<SFColor>(node, "specularColor",
                                               SFColor{0.0f, 0.0f, 0.0f});
      m.phong.shininess = geombounds::getField<float>(node, "shininess", 0.2f);
      m.phong.ambientIntensity =
          geombounds::getField<float>(node, "ambientIntensity", 0.2f);
      m.phong.occlusionStrength =
          geombounds::getField<float>(node, "occlusionStrength", 1.0f);
    }

    // normalScale (X3DOneSidedMaterialNode, all lit/unlit one-sided materials) — .
    m.normalScale = geombounds::getField<float>(node, "normalScale", 1.0f);
  };
  decodeMaterial(*material, m);

  m.textures = texturesOf(appearance);

  // X3D v4 Appearance.backMaterial.
  if (auto backNode = geombounds::getNode(*appearance, "backMaterial")) {
    MaterialDesc bm;
    decodeMaterial(*backNode, bm);
    // §12.2.3: the back MaterialDesc carries ITS own texture set (from the back
    // material node's slots), so the constraint can compare both the model type
    // and the texture set against the front.
    materialSlotTextures(*backNode, bm.textures);
    // Constraint (design §2): backMaterial must be the same model type and the
    // same texture-slot set as the front. Surfaced as a diagnostic; not enforced.
    m.backMaterialConstraintMet =
        (bm.model == m.model) && sameTextureSlotSet(m.textures, bm.textures);
    m.backMaterial = std::make_unique<MaterialDesc>(std::move(bm));
    m.doubleSided = true;
  }

  return m;
}

// texturesOf — collect the Appearance's textures with material-slot precedence
// over the legacy Appearance.texture .
//
//   * A material-borne texture slot wins: Material exposes diffuse/emissive/
//     normal/occlusion/specular textures; PhysicalMaterial exposes base/
//     metallicRoughness/normal/emissive/occlusion; UnlitMaterial exposes
//     emissive/normal. MultiTexture in any slot expands to one TextureRef per
//     channel.
//   * only if no material slot is populated is the legacy Appearance.texture
//     surfaced (as a BaseColor slot).
inline std::vector<TextureRef> texturesOf(const X3DNode *appearance) {
  using namespace matsys;
  std::vector<TextureRef> out;
  if (!appearance) return out;

  bool anyMaterialSlot = false;
  if (auto material = geombounds::getNode(*appearance, "material"))
    anyMaterialSlot = materialSlotTextures(*material, out);

  // Legacy Appearance.texture only when no material slot was populated.
  if (!anyMaterialSlot)
    appendSlot(*appearance, "texture", TextureRef::Slot::BaseColor, out);

  return out;
}

} // namespace x3d::runtime::extract
#endif // X3D_RUNTIME_EXTRACT_MATERIAL_SYSTEM_HPP
