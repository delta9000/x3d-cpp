---
title: "Materials (§12 Shape + Appearance)"
summary: "MaterialDesc discriminated union (Phong/Physical/Unlit), backMaterial two-sided support, multi-UV texCoordMapping, and ORM channel-packing docs — the extraction seam between X3D Appearance nodes and the renderer-facing descriptor."
tags: [subsystem, materials, extract, appearance, pbr, phong]
updated: 2026-06-21
related:
  - ../architecture.md
  - ../subsystems/extract-textures.md
  - ../subsystems/shaders.md
  - ../decisions/0021-material-shader-design.md
---

# Materials (§12 Shape + Appearance)

## Purpose

Converts an X3D `Appearance` node (and its child `material`, `backMaterial`, and texture nodes) into a renderer-facing `MaterialDesc` discriminated union.  The conversion is reflection-generic — it reads every field by X3D name through the generated reflection layer and adds no members to generated nodes.  The result is a plain-data descriptor a consumer can bind without touching the X3D node graph.

## Key files

| File | Role |
|---|---|
| `runtime/extract/RenderItem.hpp` | Defines `MaterialDesc`, `PhongParams`, `PhysicalParams`, `TextureRef`, `MaterialModel`, `AlphaMode`, `ShaderProgramDesc`, `ShaderStageDesc`, `ShaderFieldBinding` — the full renderer contract |
| `runtime/extract/MaterialSystem.hpp` | `materialOf(appearance)` + `texturesOf(appearance)` — the extraction logic; header-only, no generated-node dependency |
| `runtime/extract/X3DFieldValue.hpp` | `X3DFieldValue` discriminated union for typed author-shader field values |

## MaterialDesc layout

```
MaterialDesc {
  MaterialModel model;          // Phong | Physical | Unlit
  SFColor       emissive;
  float         normalScale;
  float         transparency;
  AlphaMode     alphaMode;      // Opaque | Mask | Blend
  float         alphaCutoff;
  bool          doubleSided;    // true when backMaterial is present (MAT-006)
  PhongParams   phong;          // valid when model == Phong
  PhysicalParams physical;      // valid when model == Physical
  vector<TextureRef> textures;
  unique_ptr<MaterialDesc> backMaterial;        // MAT-006
  bool backMaterialConstraintMet;
  SFColorRGBA toRGBA() const;   // composes RGB + alpha = 1 - transparency
}
```

`MaterialModel` selects the active variant:

| X3D node | MaterialModel | Primary color field |
|---|---|---|
| `Material` | `Phong` | `diffuseColor` |
| `PhysicalMaterial` | `Physical` | `baseColor` |
| `UnlitMaterial` | `Unlit` | `emissiveColor` |
| null material under present Appearance | `Unlit` | white (1,1,1) — spec §12.4.2 |
| null Appearance entirely | `Unlit` | white — debug always-draws fallback |

## materialOf dispatch

`materialOf(const X3DNode *appearance)` in `runtime/extract/MaterialSystem.hpp` walks `Appearance.material` node type name and fills the correct variant block.  Every read goes through `geombounds::getField` so the function is free of generated-node `#include` dependencies.

Fields covered per model:

**Phong (Material):** `diffuseColor`, `emissiveColor`, `specularColor`, `shininess`, `ambientIntensity`, `occlusionStrength` (MAT-004), `normalScale` (MAT-005).

**Physical (PhysicalMaterial):** `baseColor`, `metallic`, `roughness`, `occlusionStrength`, `emissiveColor`, `normalScale`.

**Unlit (UnlitMaterial):** `emissiveColor`, `transparency`, `normalScale`.

`alphaMode` and `alphaCutoff` are always read from `Appearance` regardless of material type.

## backMaterial — two-sided surfaces (MAT-006)

When `Appearance.backMaterial` is present:

1. The field is read the same way as the front material — same three-way dispatch, same field reads.
2. A constraint check validates: `backMaterial.model == front.model` (same material model type).  The result is written to `backMaterialConstraintMet`; the check is diagnostic, not enforced by the SDK.  Same-texture-slot-set checking is a deferred follow-on — the back `MaterialDesc` is built inline without `textures` populated (Appearance-level textures are front-only), so a texture-set comparison would always compare N vs 0 and be inert.
3. `MaterialDesc::doubleSided` is set `true`.
4. The back descriptor is stored as `unique_ptr<MaterialDesc> backMaterial` (avoids the self-referential incomplete-type problem of `optional<MaterialDesc>`).

## Texture extraction and multi-UV (MAT-007)

`texturesOf(appearance)` collects `TextureRef` entries with **material-slot precedence over the legacy `Appearance.texture`** (design §D6):

- A material-borne texture slot (`Material.diffuseTexture`, `PhysicalMaterial.baseTexture`, etc.) wins.
- ONLY when no material slot is populated is `Appearance.texture` surfaced as `Slot::BaseColor`.
- `MultiTexture` in any slot expands to one `TextureRef` per channel, each carrying its stage index in `channel`.

Each `TextureRef` carries `texCoordMapping` (the X3D v4 `xxxTextureMapping` field value, empty = UV set 0).  `mappingOf(materialNode, textureFieldName)` reads the mapping by constructing `fieldName + "Mapping"` and using the reflection layer — no generated-node dependency.

## ORM channel-packing (MAT-008)

`Slot::MetallicRoughness` maps to the X3D `PhysicalMaterial.metallicRoughnessTexture` (or an ORM combined image).  The channel assignment follows glTF §3.9.4 / X3D §17:

| Channel | Semantic |
|---|---|
| R | Occlusion |
| G | Roughness |
| B | Metallic |

The SDK emits exactly **one** `MetallicRoughness` `TextureRef` per packed map.  Consumers must NOT request separate Metallic / Roughness / Occlusion textures from the same image.  The color-space convention for each slot is documented in the `TextureRef::Slot` enum comment in `runtime/extract/RenderItem.hpp`.

## toRGBA and gamma stance (MAT-009)

`MaterialDesc::toRGBA()` produces `SFColorRGBA{ rgb, 1.0 - transparency }` where `rgb` is `phong.diffuse`, `physical.baseColor`, or `emissive` depending on model.

**Gamma / sRGB stance:** the SDK emits colors in the space they were authored.  X3D uses linear-intensity floats for Material/PhysicalMaterial.  The PoC consumer in `examples/poc_renderer/` treats `BaseColor` and `Emissive` texture slots as sRGB (uses `GL_SRGB8_ALPHA8`), treats Normal/Occlusion/MetallicRoughness as linear, and applies `linearToSRGB()` gamma-correction at fragment output.  The SDK itself does not apply gamma; that is consumer policy.

## Seam boundary

`materialOf` and `texturesOf` are pure extraction functions — they consume `const X3DNode*` and return POD descriptors.  No renderer state, no OpenGL, no GLFW, no DynamicFieldStore.  The consumer drives the render loop; the SDK drives the descriptor.

## Related specs and ADRs

- [ADR-0021: Material + Shader Design](../decisions/0021-material-shader-design.md) — binding decisions: discriminated union, backMaterial unique_ptr, vocab+introspection seam, IBL deferred.
- [Texture, Material, and Light Extraction](extract-textures.md) — the broader extraction layer this sits within.
- [Shaders subsystem](shaders.md) — ComposedShader introspection, vocabulary, binding plan.
- Design spec: `docs/superpowers/specs/2026-06-21-material-shader-design.md`
- Source files: `runtime/extract/MaterialSystem.hpp`, `runtime/extract/RenderItem.hpp`, `runtime/extract/X3DFieldValue.hpp`
