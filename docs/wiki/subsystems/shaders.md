---
title: "Shaders (selection, program descriptors + binding plan)"
summary: "§31 shaders end to end: selectShader() picks each Appearance's first valid GLSL ComposedShader/ProgramShader and SceneExtractor puts it on RenderItem::shaderProgram; ShaderSystem emits isSelected/isValid and handles activate; ShaderUniformVocabulary + buildBindingPlan() classify uniforms; cpu_raster and the GL PoC run the program."
tags: [subsystem, shaders, extract, composedshader, vocab, binding-plan]
updated: 2026-10-10
related:
  - ../architecture.md
  - ../subsystems/materials.md
  - ../subsystems/extract-textures.md
  - ../decisions/0021-material-shader-design.md
---

# Shaders (selection, program descriptors + binding plan)

## Purpose

This subsystem carries §31 programmable shaders from the scene graph to a
renderer. `selectShader()` (`runtime/extract/ShaderExtract.hpp`) chooses each
Appearance's program, `SceneExtractor` puts it on `RenderItem::shaderProgram`,
and `ShaderSystem` (`runtime/events/ShaderSystem.hpp`) reports the choice to
the scene as `isSelected`/`isValid` events. A consumer binds the program and
calls `buildBindingPlan()` to classify each uniform name into three buckets:
**vocab match** (a known semantic from the vocabulary header), **author field**
(a `<field>` declared on the shader), or **unrecognized** (a diagnostic with a
nearest-vocab suggestion).

## Selection and sources (§31.2.2.3, §31.2.4)

`selectShader(appearance, authorFields, ShaderOptions)` walks
`Appearance.shaders` in order and returns the first candidate that is:

1. a `ComposedShader` (ShaderPart children) or `ProgramShader` (ShaderProgram
   children) — `PackagedShader` and other nodes are skipped;
2. in language `GLSL` (case-insensitive) — HLSL/Cg candidates stay inert, as
   §31.2.2.3 permits;
3. fully sourced: each part's inline body (`sourceCode`, filled from the XML
   CDATA for ShaderPart and ShaderProgram), else its first url that resolves.
   `data:` urls (RFC 2397, percent-encoded or `;base64`) decode in the SDK;
   other urls go to `ShaderOptions::resolver` with `AssetKind::Shader`, and a
   Pending result marks the candidate `pending`. `load` FALSE skips the urls;
4. valid according to `ShaderOptions::validator` — the host's compile check.
   The default, `structuralShaderValidation`, wants one VERTEX and one FRAGMENT
   part. The SDK cannot compile GLSL, so a host that can should supply its own.

The result lists the candidates it evaluated (with each host error) and, when
one wins, the assembled `ShaderProgramDesc`. With no winner the item keeps the
fixed-function material path.

`ShaderOptions` lives on `MeshBuildOptions::shaders`. `RuntimeSession` hands the
same options to `attachStandardRuntime`, so the extractor and `ShaderSystem`
agree on the selection.

## Events and updates (§31.3.2)

`ShaderSystem` attaches to every Appearance, shader and part. It re-runs the
selection after a write to `Appearance.shaders`, a shader's `parts`/`programs`,
or a part's `url`/`load`/`sourceCode`/`type`, after `activate` TRUE, and each
tick while a url is Pending. It emits:

- `isValid` for each evaluated candidate when first evaluated, when its validity
  changes and after every `activate` TRUE (nothing while its url is Pending);
- `isSelected` TRUE when the shader becomes the selection of at least one
  Appearance and FALSE when it stops being selected anywhere.

Edits take effect immediately; `activate` TRUE additionally re-resolves urls
and re-runs the validator (an interpretation recorded in the
[requirements audit](../guides/x3d4-requirements-audit.md#shader-selection-and-execution-2026-10-10)).
On the extraction side, shaders, parts and programs are appearance-subtree
nodes, so uniform events, source edits and the emitted outputs all reach
`delta()` as `updatedMaterial`, and `refreshMaterial()` re-selects.

Composed-geometry `attrib` children are extracted separately from the uniform
binding plan. `MeshData::vertexAttributes` contains one stream per supported
`FloatVertexAttribute`, `Matrix3VertexAttribute`, or `Matrix4VertexAttribute`,
with its authored `name`, scalar `components` count (1–4, 9, or 16), and
vertex-major float `values`. Values are expanded in lockstep with mesh
positions using the coordinate vertex index (including `coordIndex` for
`IndexedFaceSet`). A renderer binds these named streams as vertex attributes;
`ShaderBindingPlan` remains a uniform classifier and does not bind vertex
inputs.

## Key files

| File | Role |
|---|---|
| `runtime/extract/ShaderExtract.hpp` | `selectShader()` — §31.2.2.3 selection, part source resolution (`data:` decode, resolver), field values |
| `runtime/extract/ShaderOptions.hpp` | `ShaderOptions` (resolver + validator), `ShaderValidation`, `structuralShaderValidation()` |
| `runtime/events/ShaderSystem.hpp` | `isSelected` / `isValid` events and `activate`; attached by `attachShaders()` / `attachStandardRuntime()` |
| `runtime/extract/RenderItem.hpp` | Defines `ShaderStageDesc`, `ShaderFieldBinding`, `ShaderProgramDesc` (the extraction descriptors), and `X3DFieldValue` (discriminated union for author `<field>` values) |
| `runtime/extract/X3DFieldValue.hpp` | `X3DFieldValue` variant covering all non-node SF types — `float`, `int`, `bool`, `SFColor`, `SFColorRGBA`, `SFVec2f/3f/4f`, `SFMatrix3f/4f`, `SFString` |
| `runtime/extract/ShaderUniformVocabulary.hpp` | `kVocabulary[]` constexpr table + `UniformSource` enum — the typed portability surface |
| `runtime/extract/ShaderBindingPlan.hpp` | `buildBindingPlan()` — classifies uniforms; `BindingEntry`, `ShaderBindingPlan`, `nearestVocabSuggestion()` |

## Extraction descriptors

```cpp
// runtime/extract/RenderItem.hpp
struct ShaderStageDesc {
  enum class Stage { Vertex, Fragment, Geometry, TessControl, TessEval, Compute };
  Stage stage;
  std::string source;      // GLSL source text
  std::string entryPoint;  // default "main"
};

struct ShaderFieldBinding {
  std::string name;        // uniform name matching the <field> name attribute
  X3DFieldType type;       // the declared X3D field type
  X3DFieldValue value;     // typed value from the DynamicFieldStore
  AccessType access;       // inputOutput / initializeOnly / ...
};

struct ShaderProgramDesc {
  std::vector<ShaderStageDesc>    stages;      // resolved source per part, in order
  std::vector<ShaderFieldBinding> fields;      // author <field>s with current values
  bool isSelected = false;                     // true on an extracted item
  bool isValid = false;                        // true on an extracted item
  std::string lastError;
  std::vector<std::string> attributeBindings;
  std::string language;                        // "GLSL"
};
```

A non-null `ShaderProgramDesc` on a `RenderItem` signals the consumer to bind the author program instead of the fixed-function material path.

Field values: SF scalar, vector, colour and matrix types map directly;
SFDouble/SFTime narrow to float, SFRotation becomes `(x, y, z, angle)` and the
double vectors/matrices narrow to float. SFNode (texture) and MF (array)
fields are listed with an empty value: `X3DFieldValue` has no channel for them
yet (finding REQ-SHADER-2).

## ShaderUniformVocabulary

`runtime/extract/ShaderUniformVocabulary.hpp` defines `kVocabulary[]` — a `constexpr` table of `VocabEntry { name, UniformSource, X3DFieldType glsl_type, bool is_array, doc }`.  The `UniformSource` enum covers every uniform a consumer would plausibly need to populate from SDK-managed state:

| Group | Examples |
|---|---|
| Transform matrices | `modelViewMatrix`, `projectionMatrix`, `normalMatrix`, `modelMatrix`, `viewMatrix`, `textureMatrix` |
| Lights | `numLights`, `lightColor[]`, `lightDirection[]`, `lightAttenuation[]`, `lightAmbientIntensity[]`, `lightBeamWidth[]`, `lightCutOffAngle[]` |
| Material (Phong) | `diffuseColor`, `specularColor`, `shininess`, `ambientIntensity` |
| Material (Physical) | `baseColor`, `metallic`, `roughness` |
| Material (shared) | `emissiveColor`, `occlusionStrength`, `normalScale`, `transparency`, `alphaMode`, `alphaCutoff` |
| Texture samplers | `baseColorTex`, `diffuseTex`, `normalTex`, `emissiveTex`, `occlusionTex`, `metallicRoughnessTex`, `shininessTex`, `ambientTex` |
| Texture presence flags | `hasBaseColorTex`, `hasDiffuseTex`, `hasNormalTex`, … |
| Environment / IBL slots | `envDiffuse`, `envSpecular`, `envSH`, `brdfLUT`, `envIntensity`, `envRotation` (**reserved in the vocabulary; EnvironmentLight / IBL is deferred — see below**) |
| Fog | `fogColor`, `fogType`, `fogVisibilityRange` |
| Clip planes | `numClipPlanes`, `clipPlane[]` |
| Per-frame | `time`, `viewportSize`, `nearFar` |

The vocabulary is intentionally a superset of what the PoC consumer binds today.  New names are added here first; consumers pick up semantics without SDK changes.

## buildBindingPlan

```cpp
// runtime/extract/ShaderBindingPlan.hpp
ShaderBindingPlan buildBindingPlan(
    const std::vector<std::pair<std::string, int>> &declaredUniforms,
    const ShaderProgramDesc &program);
```

`declaredUniforms` is the list of `{name, location}` pairs from driver introspection (e.g. `glGetActiveUniform`).  For each uniform:

1. **Vocab match** — name found in `kVocabulary[]`; `BindingEntry::source` = the `UniformSource` enum value; consumer uses this to locate the SDK-managed datum.
2. **Author field** — name found in `program.fields` (the `ShaderFieldBinding` list from the `DynamicFieldStore`); `isAuthorField = true`; consumer reads the `X3DFieldValue` directly.
3. **Unrecognized** — neither; `unrecognized = true`; `nearestVocabMatch` is computed via Levenshtein edit-distance (threshold: `len/3 + 1`, catches typos and camelCase drift); a diagnostic string is appended to `ShaderBindingPlan::diagnostics`.

The result `ShaderBindingPlan::entries` is a classified, ordered list parallel to `declaredUniforms`.

## PoC consumer dispatch (Phase 5)

The PoC consumer in `examples/poc_renderer/main.cpp` demonstrates the four-program dispatch. Its `makeGlShaderValidator` compiles and links each candidate in the GL context, so a program that fails to compile is skipped for the next shader or the material (`tests/author_shader_gl_test.py`):

| Condition | Program | Shader files |
|---|---|---|
| `item.shaderProgram` set and valid | author | compiled per program, cached by source (PATH 4); every SF author field is uploaded |
| `topology != Triangles OR !hasNormals` | unlit | `unlit.vert` / `unlit.frag` |
| `model == Physical` | PBR | `lit.vert` / `pbr.frag` |
| `model == Phong` | Phong | `lit.vert` / `lit.frag` |

`pbr.frag` implements a metallic-roughness **analytic BRDF** (Cook-Torrance NDF + Schlick Fresnel + Smith geometry, sRGB output).  **IBL (image-based lighting) is not implemented** — `EnvironmentLight` is an X3D 4.1 node and the generated binding layer is locked to X3D 4.0; IBL is deferred (see deferred note below).

## Lighting model (reference evaluator)

The Phong/PBR programs (GLSL and their cpu_raster CPU ports, `cpuraster/MaterialShader.hpp`) are the ADR-0027 reference evaluator. Two §17/§23 facts are wired there:

- **Colour space (ADR-0027).** PhysicalMaterial (`pbr.frag`) uses a linear workflow: colour textures are sRGB-decoded, factors are linear, output is sRGB-encoded. Material (`lit.frag`) and UnlitMaterial stay in display space: no decode, no encode (`uGammaOutput = 0`), so the same authored colour renders alike through Phong and Unlit, and X3D 3.x content keeps its look. The spec says nothing about colour space; this mirrors common X3D browser practice for PBR vs classic materials.
- **Per-light `ambientIntensity` (§17.2.2.4).** Each light contributes `ambientIntensity_i · diffuseColor · material.ambientIntensity` (PhysicalMaterial has no ambientIntensity, so the ambient surface is `diffColor`), gated by attenuation/spot like the light's other terms. `LightDesc.ambientIntensity` (extracted by `runtime/extract/LightSystem.hpp`) surfaces as `EyeLight::ambientIntensity`; the PoC uploads it as `uLightAmbient[]` (vocab `lightAmbientIntensity[]`). This keeps today's squared-diffuse ambient convention (an ADR-0027 open question, card RND-2) unchanged.
- **`NavigationInfo.headlight` (§23.4.4).** headlight TRUE (default) turns the camera-space headlight ON regardless of the scene's own lights; FALSE turns it OFF. Both consumers' `buildEyeLights()` add it whenever the flag is on (reserving a `kMaxLights` slot so it is never dropped), not only as a no-lights fallback. §23.4.4 pins the headlight exactly: intensity 1, color (1 1 1), `ambientIntensity` 0.0, direction (0 0 −1).

## IBL / EnvironmentLight — DEFERRED

The vocabulary reserves `EnvDiffuse`, `EnvSpecular`, `EnvSH`, `BrdfLUT`, `EnvIntensity`, `EnvRotation` entries so author shaders can declare them today without a vocab change when IBL ships.  However `EnvironmentLight` itself is an X3D 4.1 node and the generated binding layer (`generated_cpp_bindings/`) is code-generated from the X3D 4.0 UOM and committed as byte-identical golden files.  Hand-authoring a 4.1 binding would invalidate the golden invariant.  The IBL work is tracked as a follow-on that requires a defined strategy for 4.1 extension nodes.

## Two-channel split

The shader seam is intentionally split into two concerns:

- **Vocabulary header** (`ShaderUniformVocabulary.hpp`) — standalone, no `RenderItem.hpp` include; pure constexpr data; include cost is zero.
- **Binding plan** (`ShaderBindingPlan.hpp`) — includes `RenderItem.hpp` for `ShaderProgramDesc`/`ShaderFieldBinding`; computes the plan at consumer introspection time, not per-frame.

This keeps the vocabulary includable in any header without dragging in the full extraction contract.

## Headless CPU GLSL emulation (out-of-SDK)

`examples/cpu_raster/` is a dependency-free CPU rasterizer that consumes this seam
headlessly (no GPU/GLFW). It carries **CPU ports** of `lit.frag`/`pbr.frag`/
`unlit.frag` (all three material models) written against a small `glsl::` value
layer, **and** a GLSL-subset **interpreter** (`cpuraster/GlslInterpreter.hpp`)
that *executes* the FRAGMENT stage of each item's `RenderItem::shaderProgram`
with its author fields as uniforms (`cpuraster/AuthorShader.hpp`). Its
`interpreterShaderValidator()` makes "the fragment stage compiles in the
interpreter" the host's isValid check; the vertex stage is accepted but not run
(the rasterizer supplies the varyings). `--frag` still forces one fragment
shader onto every item. It binds the same `ShaderUniformVocabulary` names this
seam defines, making the shader seam testable as a GPU-free golden-image
harness (`tests/author_shader_test.cpp`); see `examples/cpu_raster/README.md`.

Its `cpuraster/Texture.hpp` sampler consumes the §18.4.9 state surfaced on
`TextureRef::extSampler` (see [Texture extraction](extract-textures.md)):
REPEAT, CLAMP, CLAMP_TO_EDGE, CLAMP_TO_BOUNDARY and MIRRORED_REPEAT wrap modes,
plus magnification/minification filters, generated box-filter mip levels and
footprint sampling with anisotropy up to 16 (TXF-4). Each texture stage and material
slot uses its own mapped/generated/transformed coordinates. The CPU also applies
per-light triangle shadow queries and keeps direct intensity separate from ambient
emission; the OpenGL PoC retains the remaining limitations recorded in the ledger.

## Related specs and ADRs

- [ADR-0021: Material + Shader Design](../decisions/0021-material-shader-design.md) — discriminated union + vocab + introspection binding decisions.
- [Materials subsystem](materials.md) — `MaterialDesc` discriminated union this seam sits alongside.
- [Texture, Material, and Light Extraction](extract-textures.md) — the broader extraction pipeline.
- Design spec: `docs/superpowers/specs/2026-06-21-material-shader-design.md`
- Source files: `runtime/extract/ShaderExtract.hpp`, `runtime/extract/ShaderOptions.hpp`, `runtime/events/ShaderSystem.hpp`, `runtime/extract/ShaderUniformVocabulary.hpp`, `runtime/extract/ShaderBindingPlan.hpp`, `runtime/extract/RenderItem.hpp`, `runtime/extract/X3DFieldValue.hpp`
- PoC consumer: `examples/poc_renderer/main.cpp`, `examples/poc_renderer/shaders/pbr.frag`
