---
title: "ADR-0021: Material + Shader Design (discriminated union + vocab + introspection)"
summary: MaterialDesc uses a tagged discriminated union (Phong/Physical/Unlit) for the material model; backMaterial is unique_ptr<MaterialDesc> for two-sided surfaces; ShaderUniformVocabulary + buildBindingPlan() provide a typed author-shader portability surface; EnvironmentLight / IBL is deferred (X3D 4.1 node, golden invariant conflict).
tags: [adr, materials, shaders, pbr, phong, extract, discriminated-union]
updated: 2026-06-21
related:
  - ../architecture.md
  - ../subsystems/materials.md
  - ../subsystems/shaders.md
  - ../subsystems/extract-textures.md
---

# ADR-0021: Material + Shader Design (discriminated union + vocab + introspection)

## Status

Accepted — Phases 1–3 + 5 implemented across the `material-shader` branch (2026-06-21).  Phase 4 (EnvironmentLight / IBL) reverted; see Deferred section.

## Context

The pre-material-shader extraction layer had a flat `MaterialDesc` with no variant structure (every field present on every material, regardless of model), no `backMaterial` support, and no author-shader introspection surface.  Three problems drove this ADR:

1. **Model ambiguity** — A consumer receiving a `MaterialDesc` with no model tag had to inspect which fields were non-default to guess whether to bind a Phong or PBR shader path.  This was fragile and produced wrong renders when materials were animated.

2. **Back-face materials** — X3D v4 `Appearance.backMaterial` (ISO §12.2.3, §12.4.2) was not extracted; two-sided surfaces were silently collapsed to one-sided.

3. **Author-shader binding** — `ComposedShader` GLSL programs declare their own uniform names.  There was no vocabulary mapping SDK-managed state (matrices, lights, material scalars, texture samplers) to those names, so every consumer had to hard-code its own convention.

## Decisions

### Decision 1: Discriminated union with explicit MaterialModel tag

`MaterialDesc::model` (enum `MaterialModel { Phong, Physical, Unlit }`) is the authoritative model selector.  The field groups `PhongParams phong` and `PhysicalParams physical` are present as value members (not as a `std::variant`) to avoid the visitor boilerplate a plain POD contract would otherwise impose on consumers.  Only the block for the active `model` is valid.  `materialOf()` always sets `model` before writing any variant fields; a consumer branches on `model` once and reads the appropriate block.

The alternative — a `std::variant<PhongParams, PhysicalParams, UnlitParams>` — was rejected because `variant` in a pure-POD header requires visitors or `get_if` everywhere a consumer reads a scalar, which multiplies boilerplate across a codebase that just wants `desc.phong.diffuse`.  The tag + dual-block approach is unambiguous and more readable at the consumer side.

### Decision 2: backMaterial as unique_ptr<MaterialDesc>

`Appearance.backMaterial` is represented as `std::unique_ptr<MaterialDesc> backMaterial` on the front `MaterialDesc`.  `std::optional<MaterialDesc>` was ruled out because `optional<MaterialDesc>` with `MaterialDesc` containing another `optional<MaterialDesc>` is an incomplete-type at the point of the field declaration — the compiler cannot size `MaterialDesc` before completing it.  `unique_ptr` breaks the cycle: the inner type is incomplete at declaration, only the pointer needs sizing.

A constraint check at extraction time (`backMaterialConstraintMet`) validates that `backMaterial.model == front.model` (same material model type).  The constraint is diagnostic — the SDK does not reject non-conformant inputs — because some X3D files in the wild use mismatched back materials for artistic effect.  Same-texture-slot-set checking is a deferred follow-on: the back `MaterialDesc` is constructed inline and its `textures` vector is not populated at that point (Appearance-level textures are front-only), so a texture-set comparison would always compare N vs 0 and be inert.

`MaterialDesc::doubleSided` is set `true` whenever `backMaterial` is non-null, giving the consumer a one-flag cull-disable signal.

### Decision 3: texCoordMapping on TextureRef (multi-UV)

X3D v4 adds `xxxTextureMapping` fields per material texture slot (e.g. `PhysicalMaterial.baseTextureMapping`).  These are read as `SFString` via `mappingOf(materialNode, textureFieldName)` which constructs `fieldName + "Mapping"` at call time and reads it through the reflection layer.  The value is stored as `TextureRef::texCoordMapping` (empty = UV set 0).  This gives consumers the per-texture UV-channel name without a separate multi-UV structure.

### Decision 4: ORM channel packing documented in the slot enum

`TextureRef::Slot::MetallicRoughness` maps to the ORM-packed image (`R=Occlusion, G=Roughness, B=Metallic`, per glTF §3.9.4 / X3D §17).  The channel assignment is normative: it is documented as a comment on the `Slot` enum in `runtime/extract/RenderItem.hpp` and enforced by the SDK emitting exactly one `MetallicRoughness` TextureRef per map.  Consumers must not split R/G/B into separate texture fetches from the same image.

### Decision 5: ShaderUniformVocabulary standalone constexpr table

The vocabulary is a `constexpr VocabEntry kVocabulary[]` in a standalone header (`ShaderUniformVocabulary.hpp`) that has no `RenderItem.hpp` include.  This keeps the include cost zero for consumers that want the vocabulary without the full extraction contract.  The table is intentionally a superset (including IBL entries reserved for future use) so new semantic names can be added here without breaking the binding-plan API.

### Decision 6: buildBindingPlan for three-bucket classification

`buildBindingPlan(declaredUniforms, program)` classifies each uniform into:

- **Vocab** — `UniformSource` enum value; consumer knows the SDK-managed datum.
- **Author field** — `ShaderFieldBinding` carries the typed `X3DFieldValue`; consumer reads directly.
- **Unrecognized** — Levenshtein edit-distance suggestion; diagnostic string; consumer skips or logs.

The three-bucket model was preferred over a flat name→location map because it separates SDK-owned state (vocab), author-owned state (field), and errors (unrecognized) into first-class categories.  A consumer that handles all three buckets is complete; one that only handles vocab gets correct bindings for the common case.

### Decision 7: PoC consumer four-program dispatch

The PoC consumer in `examples/poc_renderer/` demonstrates the dispatch:

| Condition | Program |
|---|---|
| `item.shader` non-null | author-shader via `buildBindingPlan` |
| `topology != Triangles OR !hasNormals` | unlit |
| `model == Physical` | PBR (analytic BRDF, no IBL) |
| `model == Phong` | Phong (Blinn + texture + normal map) |

`pbr.frag` implements Cook-Torrance NDF + Schlick Fresnel + Smith geometry with `linearToSRGB()` gamma-correction at output.

## Deferred: Phase 4 EnvironmentLight / IBL

Phase 4 of the material-shader plan proposed extracting `EnvironmentLight` (X3D 4.1 §39-contrib) into an environment block on `LightDesc` and wiring the six IBL vocabulary entries (`envDiffuse`, `envSpecular`, `envSH`, `brdfLUT`, `envIntensity`, `envRotation`).

**This phase was reverted and is deferred.**  The blocking reason: `EnvironmentLight` is an X3D 4.1 node, not present in the 4.0 UOM from which `generated_cpp_bindings/` is generated.  The generated layer is committed as byte-identical golden files (ADR-0005).  A hand-authored `EnvironmentLight` binding would either (a) live outside `generated_cpp_bindings/` in an ad-hoc file — inconsistent with the generation model — or (b) require touching `generated_cpp_bindings/`, producing a new golden SHA and potentially invalidating the golden invariant.

The path forward requires a defined architecture for X3D 4.1 extension nodes that either:
- Extends the generator to handle versioned node additions without full golden-regeneration.
- Defines a policy for hand-authored 4.1 bindings in a separate include tree outside the golden.

Until that architecture is defined, IBL stays deferred.  The vocabulary entries are reserved so author shaders can declare `envDiffuse` / `envSpecular` etc. without a vocab-breaking change when IBL ships.

## Consequences

**Positive:**

- A consumer reads `desc.model` once and branches cleanly; no field-sniffing to determine the material type.
- `doubleSided` + non-null `backMaterial` is a complete two-sided surface API; single-sided surfaces pay zero cost (null unique_ptr).
- `texCoordMapping` on each `TextureRef` supports multi-UV authoring without a separate structure.
- `buildBindingPlan` is call-once at shader-load time; per-frame draw cost is the same as before.
- The vocabulary is forward-compatible: adding a new entry to `kVocabulary[]` is non-breaking to existing consumers.

**Costs / trade-offs:**

- The dual PhongParams/PhysicalParams layout puts dead fields in every MaterialDesc.  For a POD contract the wasted bytes are acceptable; post-v1 a `std::variant` could replace them if the boilerplate cost becomes worth paying.
- `unique_ptr<MaterialDesc>` means `MaterialDesc` is not trivially copyable; explicit copy/move ops are defined on `MaterialDesc` to restore value semantics.
- IBL deferred means PBR output is analytic-only (specular highlight but no environment reflections) until the EnvironmentLight architecture decision is resolved.

## Related

- [Materials subsystem](../subsystems/materials.md)
- [Shaders subsystem](../subsystems/shaders.md)
- [Texture, Material, and Light Extraction](../subsystems/extract-textures.md)
- [ADR-0005: Golden Files Committed to Git](0005-golden-files-in-git.md)
- [ADR-0015: Extraction as Pull over an Incremental Dirty Set](0015-extraction-pull-per-path.md)
- Design spec: `docs/superpowers/specs/2026-06-21-material-shader-design.md`
- Key headers: `runtime/extract/RenderItem.hpp`, `runtime/extract/MaterialSystem.hpp`, `runtime/extract/ShaderUniformVocabulary.hpp`, `runtime/extract/ShaderBindingPlan.hpp`
