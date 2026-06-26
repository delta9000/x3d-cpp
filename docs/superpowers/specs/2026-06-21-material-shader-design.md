# Material & shader system — discriminated MaterialDesc, vocabulary + introspection author shaders, IBL (design)

**Date:** 2026-06-21  **Status:** approved (autonomous execution)
**Closes:** MAT-006 (`backMaterial` / `TwoSidedMaterial`); opens MAT-007 (texCoordMapping),
MAT-008 (ORM channel packing), MAT-009 (gamma stance). Predecessor context: the
flat-superset `MaterialDesc` shipped in M2.5 (`runtime/extract/RenderItem.hpp:307-348`)
with Physical fields populated then ignored by the Phong-only `lit.frag`.

## Problem

The shipped `MaterialDesc` is a flat tagged superset: every field any model might need
sits on the struct, and `MaterialModel` selects which subset is meaningful. In practice
this means a Phong `Material` carries `metallic=1.0f` and `roughness=1.0f` that *look*
meaningful but are ignored by the Phong-only fragment shader — the exact
"populated-then-ignored" muddle the design review flagged. The same flat struct has no
seam for material extensions (`KHR_materials_*`, X_ITE's `DispersionMaterialExtension`),
no `backMaterial` (the open MAT-006 finding), and no author-shader override channel.

The Shaders component (`docs/conformance/components/Shaders.md`) is L1 data-only today:
`ComposedShader`/`PackagedShader`/`ProgramShader`/`ShaderPart`/`ShaderProgram` parse and
store, but nothing binds their fields to GLSL uniforms. Author-shader uniform binding is
an underspecified area across X3D runtimes (x3dom, X_ITE, Castle Game Engine) and in this
SDK: each relies on an implicit naming convention for uniform names, with no diagnostics on
mismatch, so a typo like `uModelViewMat` yields a uniform location of -1 and a black screen.

### Prior-art survey (real runtime decisions)

Three implementations use three different conventions; in each, a mistyped uniform name
binds silently (no diagnostic):

- **x3dom** — unprefixed names (`modelViewMatrix`, `light0_Direction`). The contract is
  a wiki tutorial table, untyped and implicit; `glGetUniformLocation`
  returns -1 silently on typo.
- **X_ITE / Titania** — `x3d_`-prefixed reserved namespace (`x3d_ModelViewMatrix`,
  `x3d_TextureMatrix[x3d_MaxTextures]`, structured `x3d_LineProperties.*`). Reserved, and
  implicit (no introspection or typed contract).
- **Castle Game Engine** — `castle_`-prefixed (`castle_ModelViewMatrix`). Explicitly
  non-portable; CGE's advice is "use `Effect`/`EffectPart`, not `ComposedShader`."

The X3D spec gives a mechanism these conventions leave on the table: **SAI `<field>`
declarations (§31.2.2.5) + Annex I Table I.3** (field type → GLSL type mapping). Field
name → GLSL uniform of identical name. Existing implementations bind §31.2.3.2
"lights/material/matrices shall be made available" to fixed, implementation-specific
uniform names rather than to author `<field>` declarations.
The spec-faithful reading is: **the engine publishes a vocabulary of recognized field
names; the author's declarations drive the binding.**

### Design goals

Three properties, taken together:

1. **Published `ShaderUniformVocabulary`** — a typed, versioned header, not a wiki page.
2. **Introspection-driven `ShaderBindingPlan`** — cross declared uniforms against the
   vocabulary at link time; emit diagnostics on typo/mismatch instead of silent black.
3. **Two channels, one mechanism** — vocabulary names auto-bound from scene state
   (§31.2.3.2 "make available" without declaration ceremony); author `<field>` params
   driven by the SAI cascade (§31.2.2.5). Declared uniforms in neither channel are a
   diagnosed error, not a silent one.

That's the usability goal. Every uniform the shader declares is accounted for.

## Scope

Both the SDK descriptor contract **and** the PoC reference consumer co-designed. X3D v4
spec-faithful: Phong stays first-class (not lowered to PBR); the `X3DOneSidedMaterialNode`
→ `{Material, PhysicalMaterial, UnlitMaterial}` hierarchy mirrors the spec type tree.
Full SAI author-shader plumbing in scope (`ComposedShader`/`PackagedShader`/
`ProgramShader`/`ShaderPart`/`ShaderProgram` with field I/O and the register-compile-link-
activate lifecycle). Full IBL via `EnvironmentLight` (X3D 4.1): cubemap pre-filtering,
spherical harmonics, BRDF LUT.

glTF 2.0 is the *north star* for the Physical path and IBL (the glTF spec mandates what
X3D 4.1 defers), but the contract stays X3D-shaped: the discriminator is the X3D type
hierarchy, not the glTF material model.

## 1. Architecture & shape

**Discriminated union, not flat superset.** `MaterialDesc` mirrors the spec hierarchy —
shared inherited fields (`emissive`, `normalScale`, `transparency`, `textures`,
`alphaMode`, `alphaCutoff`) + per-variant structs (`Phong{diffuse,specular,shininess,
ambientIntensity}`, `Physical{baseColor,metallic,roughness,occlusionStrength}`,
`Unlit{}`). Eliminates today's "fields populated then ignored" pain.

**Phong stays first-class** (not lowered to PBR) — spec-faithful, and the user's explicit
choice.

**`extensions` MFNode is the extensibility seam** — `MaterialExtensionDesc[]` carries
typed `X3DMaterialExtensionNode` capabilities (clearcoat, sheen, dispersion, etc.) without
touching the base union. Matches X_ITE's design.

**Author shaders are an override, not a fourth model.** `RenderItem` carries
`MaterialDesc` (always populated, fallback) + `optional<ShaderProgramDesc>` (when present,
replaces fixed-function per spec §31.2.3.2).

**IBL lands on `LightDesc::Type::Environment`, not `MaterialDesc`.** `EnvironmentLight`
(X3D 4.1) carries `diffuseTexture` + `specularTexture` cubemaps + `diffuseCoefficients`
(spherical harmonics). Consumer precomputes prefiltered mip chain + BRDF LUT.

**`backMaterial` is `optional<MaterialDesc>`** with documented constraint: same model +
same texture set as front (closes MAT-006).

**`doubleSided` is derived:** `solid==false OR backMaterial present`.

**`TextureRef` gains `texCoordMapping`** string (X3D v4 `xxxTextureMapping` label, glTF
`texCoord` generalized to string).

**Three-layer uniform split for author shaders:**

- **Layer 1 (on contract, spec-mandated):** SAI `<field>` bindings as typed entries (X3D
  field type → GLSL type per Annex I Table I.3, texture→sampler mapping), per-vertex
  attribute names, `activate`/`isSelected`/`isValid` lifecycle bits with `isValid` as
  consumer→SDK back-channel.
- **Layer 2 (documented PoC convention, not SDK contract):** `u`-prefixed uniforms
  matching existing repo convention — matrices, material, lights (arrays), environment/
  IBL, fog, time, viewport. Today's `main.cpp` bind block (`examples/poc_renderer/main.cpp:
  1185-1200`) is the baseline.
- **Layer 3 (consumer's own, out of scope):** no prefix mandate from SDK; spec does not
  define this; no cross-engine portability promise.

## 2. MaterialDesc contract

Reorganizes the existing flat superset into a discriminated union mirroring
`X3DOneSidedMaterialNode` → `{Material, PhysicalMaterial, UnlitMaterial}`. Header-only,
POD, golden-untouched — same constraints as today's `RenderItem.hpp`. Every field below is
either already populated by `MaterialSystem.hpp` (just moved) or is a documented new seam.

```cpp
// --- discriminator ---
enum class MaterialModel : uint8_t { Phong, Physical, Unlit };

// --- per-variant field blocks (only the spec fields for that model) ---
struct PhongParams {
  SFColor  diffuse           {0.8f, 0.8f, 0.8f};   // Material.diffuseColor
  SFColor  specular          {0.0f, 0.0f, 0.0f};   // Material.specularColor
  float    ambientIntensity  = 0.2f;                // Material.ambientIntensity
  float    shininess         = 0.2f;                // Material.shininess  [0,1]
  float    occlusionStrength = 1.0f;                // Material.occlusionStrength (MAT-004 closed)
};
struct PhysicalParams {
  SFColor  baseColor         {1.0f, 1.0f, 1.0f};   // PhysicalMaterial.baseColor      (= glTF baseColorFactor)
  float    metallic          = 1.0f;                // PhysicalMaterial.metallic       (= glTF metallicFactor)
  float    roughness         = 1.0f;                // PhysicalMaterial.roughness      (= glTF roughnessFactor)
  float    occlusionStrength = 1.0f;                // PhysicalMaterial.occlusionStrength
};
// Unlit: no extra fields — emissive-as-baseColor IS the model.

// --- inherited from X3DOneSidedMaterialNode (shared by all three) ---
// (lives directly on MaterialDesc, not in a variant)

// --- extension seam (X3DMaterialExtensionNode, X_ITE's `extensions` MFNode) ---
struct MaterialExtensionField {
  std::string name;          // field name (SAI model — like author fields)
  X3DFieldType type;         // SFColor / SFFloat / SFNode ... (Annex I typing)
  X3DFieldValue value;       // variant-held value; SFNode → TextureRef
};
struct MaterialExtensionDesc {
  std::string typeName;                   // e.g. "DispersionMaterialExtension"
  std::vector<MaterialExtensionField> fields;
};

// --- the descriptor ---
struct MaterialDesc {
  MaterialModel model = MaterialModel::Phong;

  // X3DOneSidedMaterialNode (shared):
  SFColor  emissive      {0.0f, 0.0f, 0.0f};
  float    normalScale   = 1.0f;            // MAT-005 closed
  float    transparency  = 0.0f;            // X3D convention; alpha = 1 - transparency

  // X3DMaterialNode + Appearance (shared):
  AlphaMode alphaMode   = AlphaMode::Opaque;   // off Appearance (glTF-borrowed)
  float     alphaCutoff = 0.5f;                // off Appearance
  bool      doubleSided = false;               // DERIVED: solid==false OR backMaterial present

  // per-variant (only the block for `model` is valid):
  PhongParams     phong;
  PhysicalParams  physical;

  // textures — slot-tagged, includes Emissive + Normal (shared by all models):
  std::vector<TextureRef> textures;

  // two-sided (closes MAT-006):
  // SPEC CONSTRAINT (Michalis): backMaterial MUST be the same model type and
  // the same texture set as the front. Flagged on the descriptor; enforcement
  // is the consumer's call. When absent, doubleSided derives from `solid` only.
  std::optional<MaterialDesc> backMaterial;

  // extension seam — additive, reflection-read, never re-shapes the base union:
  std::vector<MaterialExtensionDesc> extensions;

  // --- convenience (kept; documented as derived, not source) ---
  // Composes the per-model RGB surface with alpha = 1 - transparency.
  // Phong → diffuse, Physical → baseColor, Unlit → emissive.
  SFColorRGBA toRGBA() const;
};
```

### What changed vs. today's `RenderItem.hpp:307-348`

- `metallic`/`roughness` move off the flat struct into `PhysicalParams`;
  `specular`/`shininess`/`ambientIntensity` move into `PhongParams`. Today's
  "populated-then-ignored" muddle is gone — a Phong material no longer carries
  `metallic=1.0f` looking meaningful.
- `baseColor` is no longer a shared union field. It was always ambiguous (diffuse? base?
  emissive?). Now: `PhysicalParams::baseColor` is the spec field; `toRGBA()` is the
  documented derived convenience that picks the right per-model RGB.
- `unlit` bool dropped — `model == MaterialModel::Unlit` is the check (was a redundant
  mirror).
- `backMaterial` is new (closes MAT-006). `doubleSided` is new as a derived bool.
- `extensions` is new — the extensibility seam. Today there are no populated extensions in
  `MaterialSystem.hpp` (X_ITE has `DispersionMaterialExtension`; glTF has
  `KHR_materials_*`); the seam ships empty but typed, so adding one doesn't reshape the
  base.

### What `MaterialSystem.hpp` does differently

- `materialOf()` still dispatches on `material->nodeTypeName()`. The Physical branch fills
  `m.physical` (not `m.metallic`/`m.roughness`); the Phong branch fills `m.phong`. Shared
  fields (`emissive`, `normalScale`, `transparency`, `alphaMode`, `alphaCutoff`,
  `textures`) filled as today.
- `backMaterial` populated from `Appearance.backMaterial` when present (reflection-read via
  `geombounds::getNode`); recursion reuses `materialOf` on the back node. Same-model/
  same-textures constraint flagged as `backMaterialConstraintMet` (computed at extract,
  surfaced for diagnostics).
- `doubleSided` derived: `!solid || backMaterial.has_value()`. `solid` comes off the
  geometry (already read by `MeshBuilder`), so this is computed in `SceneExtractor` when
  the `RenderItem` is assembled, not in `materialOf` (which doesn't see geometry).
  Documented.
- `extensions` populated by scanning `Appearance` children for
  `X3DMaterialExtensionNode`-typed nodes (none today, but the dispatch is
  `nodeTypeName()`-based like the rest). Field reads use the existing
  `geombounds::getField` reflection path.

### Golden/test impact

- `RenderItem.hpp` is golden-tracked (byte-identical regeneration check). This restructure
  changes the header, so `mise run golden` regenerates the committed bindings — expected,
  one-time.
- `x3d_material` ctest (existing) asserts `toRGBA().a == 1.0f` on default Material and
  field reads per model. Extended to assert: Phong material has `physical.metallic == 1.0f`
  (default, but the field is in the Physical block, ignored on Phong — the test pins that
  the discriminator, not the field presence, selects the model).
- New `x3d_material_back` test: `Appearance.backMaterial` present →
  `backMaterial.has_value()`, `doubleSided == true`, same-model constraint flagged.
- New `x3d_material_extension` test: a synthetic extension node → `extensions[0].typeName`
  populated, fields reflection-read.

### Shared dependency

`MaterialExtensionField::value` (`X3DFieldValue`) needs to cover the same X3D field types
as Annex I Table I.3 (SFColor, SFFloat, SFNode→TextureRef, etc.). That type already wants
to exist for the shader binding (§4) — so `X3DFieldValue` is a shared dependency between
the material-extension seam and the author-shader field-binding seam. Defined once in §4
and back-referenced here.

## 3. TextureRef & multi-UV

The codebase already has most of the pieces: `TextureRef::Slot` (9 slots incl.
`MetallicRoughness`, `Shininess`, `Ambient`), `KhrTextureTransform` on `TextureDesc` (with
`texcoord_index`), `ExtendedSamplerParams`, `TexCoordGenDesc`. The C++ bindings already
generate `xxxTextureMapping` SFString fields on every material node (`baseTextureMapping`,
`diffuseTextureMapping`, `emissiveTextureMapping`, `metallicRoughnessTextureMapping`,
`normalTextureMapping`, `occlusionTextureMapping`, `shininessTextureMapping`,
`specularTextureMapping`, `ambientTextureMapping` — see `generated_cpp_bindings/Material.cpp`
and `X3DOneSidedMaterialNode.hpp`). Section 3 closes the gap between the spec's string-label
UV selection and the existing integer-index `texcoord_index`, and documents the
channel-packing convention.

```cpp
// Added to TextureRef (RenderItem.hpp):
SFString texCoordMapping;  // X3D v4 xxxTextureMapping label; empty = UV set 0.
```

### How UV selection resolves (the synthesized decision)

X3D v4 generalized UV selection from integer indices to **string labels**
(`baseTextureMapping`, `normalTextureMapping`, etc.). glTF uses integer `texCoord`
(0, 1, 2...) with `KHR_texture_transform` overriding. The synthesis:

1. `TextureRef::texCoordMapping` is the **primary** UV selector. Empty string → UV set 0
   (default). Non-empty → match against `TextureCoordinate::name` (X3D v4 named texCoord
   sets) on the geometry.
2. `KhrTextureTransform::texcoord_index` (already on `TextureDesc`) is the **glTF
   fallback** when `texCoordMapping` is empty but a non-zero UV set is needed.
   `MaterialSystem::texturesOf()` populates `texCoordMapping` from the material node's
   `xxxTextureMapping` field; if that's empty and the texture came from a glTF import path,
   `texcoord_index` carries the integer.
3. The consumer's bind logic: `if (!texCoordMapping.empty()) look up named UV set; else
   use texcoord_index (default 0)`. One path, both conventions honored.

### Channel-packing convention (documented, not enforced)

glTF's ORM packing is normative: `MetallicRoughness` slot texture has R=occlusion,
G=roughness, B=metallic (all linear, not sRGB). The existing `Slot::MetallicRoughness`
already implies this. `TextureDesc::color_space` must be `Linear` for this slot (the
consumer checks: slot == MetallicRoughness || slot == Normal || slot == Occlusion → force
Linear regardless of `color_space`). `MaterialSystem::texturesOf()` sets `color_space`
correctly per slot when constructing the `TextureDesc`; the consumer's upload path
validates.

`Slot::Occlusion` as a separate slot is for the X3D case where occlusion is a standalone
texture (R channel, linear). When glTF packs it into the ORM texture, `texturesOf()`
emits one `MetallicRoughness` slot entry and the consumer reads R from it for occlusion —
no duplicate `Occlusion` slot entry. Documented in the slot enum comment.

### What `MaterialSystem::texturesOf()` changes

- Reads `xxxTextureMapping` from the material node per texture slot (e.g.,
  `baseTextureMapping` for PhysicalMaterial's base texture, `diffuseTextureMapping` for
  Material's diffuse). Populates `TextureRef::texCoordMapping`.
- For `PhysicalMaterial`: emits `MetallicRoughness` slot (not separate Metallic +
  Roughness) when the source has the packed ORM texture; emits separate slots only when
  the source has standalone textures.
- Sets `TextureDesc::color_space` per slot: sRGB for BaseColor/Diffuse/Emissive/Specular;
  Linear for Normal/Occlusion/MetallicRoughness/Shininess/Ambient.

## 4. ShaderProgramDesc — SAI fields, binding plan, vocabulary

This is the cut-above section. Three artifacts: the **vocabulary header** (SDK-owned,
typed), the **binding plan** (produced per-shader-program by introspection), and the
**field-binding channel** (author `<field>` params via `DynamicFieldStore`).

### 4.1 The vocabulary header (SDK-owned, typed, versioned)

```cpp
// New header: runtime/extract/ShaderUniformVocabulary.hpp
// SDK-owned, typed, versioned. The portability surface for author shaders.
namespace x3d::runtime::extract::vocab {

enum class UniformSource : uint8_t {
  // Matrices (§31.2.3.2 "shall be made available")
  ModelViewMatrix, ProjectionMatrix, NormalMatrix, ModelViewProjectionMatrix,
  ModelMatrix, ViewMatrix, TextureMatrix,    // per-TextureRef
  // Lights (arrays)
  NumLights, LightType, LightColor, LightDirection, LightLocation,
  LightAttenuation, LightRadius, LightAmbientIntensity,
  LightBeamWidth, LightCutOffAngle, LightGlobal, LightScopeRoot,
  // Environment/IBL
  EnvDiffuse, EnvSpecular, EnvSH, BrdfLUT, EnvIntensity, EnvRotation,
  // Material (per-model fields, see §2)
  BaseColor, DiffuseColor, SpecularColor, EmissiveColor,
  Metallic, Roughness, Shininess, AmbientIntensity,
  OcclusionStrength, NormalScale, Transparency, AlphaMode, AlphaCutoff,
  // Textures (per-slot samplers)
  BaseColorTex, DiffuseTex, SpecularTex, EmissiveTex, NormalTex,
  OcclusionTex, MetallicRoughnessTex, ShininessTex, AmbientTex,
  HasBaseColorTex, HasDiffuseTex, /* ... one Has* per slot */
  // Fog
  FogColor, FogType, FogRange, FogVisibilityRange,
  // Time/viewport
  Time, ViewportSize, NearFar,
  // Clip planes
  NumClipPlanes, ClipPlane,
};

struct VocabEntry {
  std::string_view name;        // GLSL uniform name (e.g., "modelViewMatrix")
  UniformSource source;         // what scene state to upload
  X3DFieldType glsl_type;       // SFMatrix4f → mat4, SFColor → vec3 (V4), etc.
  bool is_array = false;        // lights[], clipPlane[]
  std::string_view doc;         // one-line source description
};

// The full table — a single constexpr array, the source of truth.
inline constexpr VocabEntry kVocabulary[] = {
  {"modelViewMatrix",        UniformSource::ModelViewMatrix,        X3DFieldType::SFMatrix4f, false,
   "RenderItem.worldTransform * CameraDesc.viewMatrix"},
  {"projectionMatrix",       UniformSource::ProjectionMatrix,       X3DFieldType::SFMatrix4f, false,
   "CameraDesc perspective/ortho projection"},
  // ... (full table, ~60 entries)
};

} // namespace vocab
```

### 4.2 The descriptor and field-binding channel

```cpp
// Added to RenderItem (SceneExtractor.hpp), not MaterialDesc:
struct ShaderStageDesc {
  enum class Stage { Vertex, Fragment, Geometry, TessControl, TessEval, Compute };
  Stage stage = Stage::Fragment;
  std::string source;         // GLSL source text (ShaderPart::url resolved to text)
  std::string entryPoint = "main";
};

struct ShaderFieldBinding {
  std::string name;           // SAI <field> name (§31.2.2.5)
  X3DFieldType type;          // SAI field type → GLSL type per Annex I Table I.3
  X3DFieldValue value;        // current value (driven by DynamicFieldStore cascade)
  AccessType access;          // inputOutput / initializeOnly (outputOnly = back-channel)
};

struct ShaderProgramDesc {
  // SAI lifecycle (§31.2.4): register → compile → link → activate
  std::vector<ShaderStageDesc> stages;
  std::vector<ShaderFieldBinding> fields;   // author <field> declarations

  // Back-channel: consumer → SDK (§31.2.4 isSelected/isValid)
  bool isSelected = false;    // consumer sets: program is the active selection
  bool isValid = false;       // consumer sets: last link succeeded (SDK-readable)
  std::string lastError;      // consumer sets: compile/link error text (SDK-readable)

  // Per-vertex attribute names (§31.2.2.5: field names map to attributes)
  // Empty = use defaults (coord, normal, texCoord, color)
  std::vector<std::string> attributeBindings;
};

// On RenderItem:
std::optional<ShaderProgramDesc> shaderProgram;  // absent = fixed-function material path
```

### 4.3 The binding plan (the introspection loop — the cut-above piece)

The SDK doesn't compile GLSL; the consumer does. But after link, the consumer reports the
**declared** uniform names back. The SDK crosses them against the vocabulary:

```cpp
// New: runtime/extract/ShaderBindingPlan.hpp
struct BindingEntry {
  std::string declaredName;          // what the GLSL declared
  vocab::UniformSource source;       // matched vocab entry, or Unrecognized
  int location;                      // consumer-provided uniform location
  bool isAuthorField;                // true = driven by ShaderFieldBinding cascade
  bool unrecognized;                 // true = not in vocab, not a declared field
  std::string nearestVocabMatch;     // edit-distance suggestion when unrecognized
};

struct ShaderBindingPlan {
  std::vector<BindingEntry> entries;
  std::vector<std::string> diagnostics;  // "declared `uModelViewMat` — did you mean `modelViewMatrix`?"
};

ShaderBindingPlan buildBindingPlan(
    const std::vector<std::pair<std::string, int>>& declaredUniforms, // (name, location)
    const ShaderProgramDesc& program);
```

The consumer calls `buildBindingPlan(declaredUniforms, shaderProgram)` after link. The SDK
returns the plan. The consumer executes it each frame: for each entry, upload from the
named source. `unrecognized` entries get a diagnostic and are skipped (location -1). This
is the silent-failure killer.

### 4.4 Two-channel split (restated, now typed)

- **Vocabulary uniforms** → auto-bound from scene state. Author writes
  `uniform mat4 modelViewMatrix;` with no `<field>` declaration. SDK's binding plan tells
  the consumer what to upload.
- **Author `<field>` uniforms** → declared as `inputOutput SFFloat hue` in the X3D file.
  SDK surfaces as `ShaderFieldBinding{type=SFFloat, value=...}`. Consumer uploads to the
  uniform of identical name (§31.2.2.5). Driven by `DynamicFieldStore` cascade (ROUTE/
  animation updates flow through — see ADR-0014).
- **Unrecognized** → diagnostic, skipped. Not a third channel — a diagnosed error.

### 4.5 `X3DFieldValue` (shared with §2's `MaterialExtensionField`)

```cpp
// New: runtime/extract/X3DFieldValue.hpp (or fold into X3Dtypes.hpp)
// A variant covering the X3D field types that can flow into GLSL (Annex I Table I.3).
struct X3DFieldValue {
  X3DFieldType type;
  std::variant<std::monostate, float, int, bool, SFColor, SFColorRGBA,
               SFVec2f, SFVec3f, SFVec4f, SFMatrix3f, SFMatrix4f,
               TextureRef /* SFNode → sampler */> value;
};
```

One type, used by both material extensions and shader field bindings. The `TextureRef`
variant arm handles `SFNode` fields pointing to texture nodes (mapped to samplers per
Annex I).

### 4.6 Spec grounding

- GLSL binding annex (V3.2 §I, normative in V4): texture→sampler type mapping (2D→
  sampler2D, 3D→sampler3D, env→samplerCube), X3D field→GLSL type table (SFColor→vec4 in
  V3.2, corrected to vec3 in V4 per CGE), register-compile-link lifecycle (`url` change →
  recompile not auto-relink; `activate` → relink; `attrib` change → relink).
- §31.2.2.5: field names map to GLSL uniforms of identical name.
- §31.2.3.2: lights/material/matrices "shall be made available" but naming is
  implementation-defined (annex doesn't define them). The vocabulary header is the SDK's
  implementation-defined answer, published as a typed contract.
- §31.2.4: `activate`/`isSelected`/`isValid` lifecycle — `isValid` is the consumer→SDK
  back-channel for compile/link failure reporting.

## 5. EnvironmentLight / IBL

`EnvironmentLight` (X3D 4.1) carries the IBL data. It lands on
`LightDesc::Type::Environment`, not on `MaterialDesc` — it's a light, not a material
property. The consumer precomputes the prefiltered mip chain + BRDF LUT (the SDK doesn't
decode or prefilter — same boundary as texture resolution).

```cpp
// Extended LightDesc (RenderItem.hpp):
struct LightDesc {
  enum class Type { Directional, Point, Spot, Environment };  // + Environment

  // ... existing fields unchanged ...

  // Environment-only (populated when type == Environment):
  struct Environment {
    TextureRef diffuseTexture;     // cubemap: irradiance (or SH coefficients below)
    TextureRef specularTexture;    // cubemap: prefiltered mip chain (specularImageSize + N mips)
    // Spherical harmonics (l=2, 9x3 = 27 floats) — EXT_lights_image_based format.
    // When present, consumer may skip sampling diffuseTexture and use SH directly
    // (cheaper, no cubemap bind for the diffuse term).
    std::array<float, 27> diffuseCoefficients{};  // 9 * vec3
    bool hasSH = false;

    float intensity = 1.0f;        // brightness multiplier
    // Rotation quaternion (applies to cubemap sampling direction).
    float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    // Consumer-managed (not populated by SDK): the prefiltered mip chain's
    // base size + mip count, so the shader can pick the mip for roughness.
    // Populated by the consumer after it uploads the specularTexture cubemap.
    uint32_t specularImageSize = 0;  // 0 = not yet uploaded
    uint32_t specularMipCount = 0;
  };
  Environment environment;  // valid only when type == Environment
};
```

### What `LightSystem` does

- `lightType()` gains `"EnvironmentLight" → Type::Environment` dispatch.
- `makeLight()` for Environment reads `diffuseTexture`, `specularTexture`,
  `diffuseCoefficients`, `intensity`, `rotation` via `geombounds::getField`. The cubemap
  `TextureRef`s are populated like any other texture (URL/source + resolved pixels threaded
  by `TextureExtract`).
- `global` is forced `true` for Environment (IBL is scene-wide by definition —
  `EnvironmentLight` has no `global` field; it's always global). Documented.
- `scopeRoot` is `nullptr` (no scoping).

### Consumer precompute pipeline (§6 detail)

1. On first `Environment` light seen: take `specularTexture` cubemap (consumer decodes via
   `TextureResolver`), upload as `GL_TEXTURE_CUBE_MAP` with full mip chain. If the source
   only has mip 0, consumer generates the prefiltered mip chain (importance-sampled GGX
   convolution — the Epic split-sum approximation).
2. Generate the BRDF LUT once (2D texture, 512×512, RG16F) — consumer-owned, uploaded once
   at startup.
3. If `hasSH`, use the 27 floats directly as `shCoeffs[9]` uniform (no diffuse cubemap bind
   needed). If not, convolve the diffuse cubemap to an irradiance map (or use SH from the
   cubemap — consumer's choice).

### glTF `EXT_lights_image_based` alignment

The `Environment` struct mirrors the glTF extension exactly:
`irradianceCoefficients` (9x3 SH), `specularImages` (Nx6 cube faces, prefiltered),
`specularImageSize`, `intensity`, `rotation`. This means a glTF importer can populate
`LightDesc::Environment` with zero translation. The RGBD HDR packing (4-channel PNG) is
handled by the consumer's texture decode — the SDK surfaces the `TextureRef` and the
consumer decodes to float (same boundary as KTX2/Basis).

## 6. PoC consumer — three programs + author path + gamma

The PoC renderer (`examples/poc_renderer/`) extends from two programs (lit/unlit) to four
+ the author-shader path. The bind block at `main.cpp:1139-1241` factors into a per-program
dispatch.

### Programs

| Program | When | Shader file | Material fields consumed |
|---------|------|-------------|--------------------------|
| `unlitProg` | topology != Triangles OR !hasNormals OR `model==Unlit` | `unlit.frag` (exists) | `emissive`/`baseColor`, `alphaMode` |
| `phongProg` | `model==Phong` AND lit | `lit.frag` (exists, extend) | `phong.diffuse/specular/shininess/ambientIntensity`, `emissive`, `normalScale` |
| `pbrProg` | `model==Physical` AND lit | `pbr.frag` (new) | `physical.baseColor/metallic/roughness/occlusionStrength`, `emissive`, `normalScale` + IBL |
| `authorProg` | `shaderProgram.has_value()` | per-ShaderProgramDesc | vocab + field bindings (§4) |

### Author-shader dispatch (the introspection loop in practice)

```cpp
// In the PoC, after compiling an author shader for the first time:
if (it.shaderProgram && !it.shaderProgram->isValid) {
  GLuint prog = compileAndLink(*it.shaderProgram);  // consumer compiles
  auto declared = queryUniforms(prog);               // glGetProgramResource
  auto plan = buildBindingPlan(declared, *it.shaderProgram);
  for (const auto& d : plan.diagnostics)
    fprintf(stderr, "shader: %s\n", d.c_str());
  cachePlan(it, prog, plan);
  it.shaderProgram->isValid = true;  // back-channel
}
// Each frame, walk the cached plan and upload:
for (const auto& e : plan.entries) {
  if (e.unrecognized) continue;
  if (e.isAuthorField) uploadField(e, shaderProgram->fields);
  else uploadVocab(e, it, camera, lights, env);
}
```

### Gamma/sRGB decision (synthesized from research)

Michalis's wiki (`x3d-tests/wiki/Gamma-correction-in-X3D-and-glTF`) + glTF spec + CGE +
x3dom all converge on the same rule. The synthesis:

1. **Color textures** (BaseColor/Diffuse/Emissive/Specular slots) → sRGB. Consumer uploads
   with `GL_SRGB8_ALPHA8` internal format (hardware does the pow(2.2) decode on sample).
   No shader-side `SRGBtoLINEAR` needed.
2. **Data textures** (Normal/Occlusion/MetallicRoughness/Shininess/Ambient) → Linear.
   Consumer uploads with linear internal format.
3. **Material parameter colors** (`baseColor`, `diffuseColor`, `emissiveColor`,
   `specularColor`) → assumed linear (no conversion). Matches glTF + x3dom + CGE.
4. **Output encoding** → `pow(color, 1/2.2)` in the fragment shader (or
   `GL_FRAMEBUFFER_SRGB` if the consumer prefers hardware). The PoC uses shader-side
   `LINEARtoSRGB` for explicitness.
5. **When gamma applies**: `Physical` (always — glTF mandates), `Phong` (opt-in via
   `Environment.gammaCorrectionDefault` equivalent — PoC defaults ON), `Unlit` (OFF — unlit
   is pass-through, no lighting math to get wrong). This matches CGE's "default gamma for
   PhysicalMaterial only" refined to "Physical always, Phong opt-in, Unlit never."

### Texture binding extension (§3 → PoC)

The existing `textureForMaterial()` (unit 0 only) extends to a per-slot bind:

- Unit 0: BaseColor/Diffuse (sRGB)
- Unit 1: Normal (linear)
- Unit 2: Emissive (sRGB)
- Unit 3: MetallicRoughness (linear, ORM packed)
- Unit 4: Occlusion (linear, standalone — only when not packed into MR)
- Unit 5: Specular/Shininess/Ambient (Phong-only slots)

Each slot gets a `Has*Tex` uniform (int). The shader branches on it. The PoC's existing
`uHasTexture`/`uTexture` (unit 0) becomes `hasBaseColorTex`/`baseColorTex` — vocabulary
names.

## 7. Verification, drift, coverage

### Tests (new + extended)

| Test | What it pins |
|------|-------------|
| `x3d_material` (exists) | Extended: Phong material has `physical.metallic==1.0f` (default, in Physical block, ignored on Phong); discriminator selects model |
| `x3d_material_back` (new) | `Appearance.backMaterial` present → `backMaterial.has_value()`, `doubleSided==true`, same-model constraint flagged |
| `x3d_material_extension` (new) | Synthetic extension node → `extensions[0].typeName` populated, fields reflection-read |
| `x3d_texture_mapping` (new) | `baseTextureMapping="uv1"` → `TextureRef::texCoordMapping=="uv1"`; empty → UV set 0 |
| `x3d_texture_orm` (new) | `MetallicRoughness` slot → `color_space==Linear`, single entry (not separate Metallic + Roughness) |
| `x3d_shader_binding_plan` (new) | Mock declared uniforms `[modelViewMatrix, uTypo, hue]` → plan has 2 vocab + 1 unrecognized + 1 field; diagnostic contains "did you mean" |
| `x3d_environment_light` (new) | `EnvironmentLight` node → `LightDesc::Type::Environment`, SH populated, cubemaps threaded |
| `x3d_author_shader_lifecycle` (new) | `ShaderProgramDesc` with invalid GLSL → `isValid==false`, `lastError` non-empty |

### Drift / docs updates (per CLAUDE.md anti-drift discipline)

| Doc | Change |
|-----|--------|
| `docs/wiki/subsystems/materials.md` (new or update) | Discriminated union, extension seam, backMaterial, MAT-006 closure |
| `docs/wiki/subsystems/shaders.md` (new) | SAI lifecycle, vocab header, binding plan, two-channel split |
| `docs/wiki/subsystems/ibl.md` (new) | EnvironmentLight, prefilter pipeline, SH vs cubemap |
| `docs/conformance/components/Shaders.md` | Update from "L1 data-only" to "L1 + binding plan" |
| `docs/conformance/findings.yaml` | Close MAT-006; add MAT-007 (texCoordMapping), MAT-008 (ORM packing), MAT-009 (gamma stance) |
| `docs/sdk/v1-capabilities.md` | Material/shader/IBL capability rows |
| `docs/wiki/coverage.md` | New subsystem rows + counts |
| `docs/wiki/decisions/NNNN-material-shader-design.md` | ADR for the discriminated union + vocab+introspection decision |
| `mkdocs.yml` | Nav entries for new pages |

### Gates

- `mise run ci` — tests + golden + conformance-gate + build + cli-gate-regression. The
  `RenderItem.hpp` restructure triggers a golden regen (expected, one-time). The new tests
  must pass.
- `mise run docs-drift working` — must flag the touched code; review the `CITES` hits on
  `materials.md`/`shaders.md`/`Shaders.md` first.
- `mise run docs-build` — dead links / nav orphans (new pages must be in `mkdocs.yml`).
- `mise run code-ingest` + `mise run docs-ingest` — refresh RAG stores after symbols move.

### Migration risk

- `RenderItem.hpp` restructure (flat → discriminated) is the breaking change. Every
  consumer of `MaterialDesc::metallic`/`roughness`/`specular`/`shininess`/`baseColor`
  breaks at compile time — that's the point (the old fields were "populated then ignored").
  The PoC is the only consumer today; its bind block (`main.cpp:1185-1200`) is the
  migration target.
- `toRGBA()` stays as the migration shim: it reads the right per-model field internally,
  so consumers that only need "the surface color" don't break.

## Open questions deferred to implementation plan

- Exact `kVocabulary[]` table contents (the ~60 entries) — drafted in §4.1, finalized
  during implementation with the PoC consumer as the first client.
- ADR number allocation (`NNNN-material-shader-design.md`) — next free slot in
  `docs/wiki/decisions/`.
- Whether `X3DFieldValue` lives in its own header or folds into `X3Dtypes.hpp` —
  implementation decision based on include-graph cycle check.
