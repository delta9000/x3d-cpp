---
title: "Shaders (ComposedShader introspection + binding plan)"
summary: "Author-shader binding INFRASTRUCTURE: ShaderProgramDesc / ShaderStageDesc / ShaderFieldBinding descriptors; ShaderUniformVocabulary typed portability surface; buildBindingPlan() vocab/author-field/unrecognized dispatch. ComposedShader extraction wiring (populating RenderItem::shaderProgram) is a deferred follow-on."
tags: [subsystem, shaders, extract, composedshader, vocab, binding-plan]
updated: 2026-06-21
related:
  - ../architecture.md
  - ../subsystems/materials.md
  - ../subsystems/extract-textures.md
  - ../decisions/0021-material-shader-design.md
---

# Shaders (ComposedShader introspection + binding plan)

## Purpose

This subsystem provides the **infrastructure** for `ComposedShader` author-shader support: `ShaderProgramDesc` / `ShaderUniformVocabulary` / `buildBindingPlan()` are fully defined and tested, and the PoC consumer's PATH 4 dispatch is wired to drive them.  **However, the ComposedShader extraction itself — populating `RenderItem::shaderProgram` from the scene graph — is not yet wired.**  `RenderItem::shaderProgram` is never set by any extractor codepath, so PATH 4 is currently unreachable.  ComposedShader extraction is a tracked follow-on.

When extraction is wired, the design intent is: the extraction layer surfaces a `ComposedShader` as a `ShaderProgramDesc` on the `RenderItem`; the consumer calls `buildBindingPlan()` to classify each uniform name into three buckets: **vocab match** (a known semantic from the vocabulary header), **author field** (a `<field>` declared on the `ComposedShader`), or **unrecognized** (a diagnostic with a nearest-vocab suggestion).  This gives the consumer a typed portability surface for binding SDK-managed state to author shaders without re-inventing the naming convention.

## Key files

| File | Role |
|---|---|
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
  std::vector<ShaderStageDesc>    stages;
  std::vector<ShaderFieldBinding> fields;
  std::string                     language;  // "GLSL" / "CG" / ""
};
```

A non-null `ShaderProgramDesc` on a `RenderItem` signals the consumer to bind the author program instead of the fixed-function material path.

## ShaderUniformVocabulary

`runtime/extract/ShaderUniformVocabulary.hpp` defines `kVocabulary[]` — a `constexpr` table of `VocabEntry { name, UniformSource, X3DFieldType glsl_type, bool is_array, doc }`.  The `UniformSource` enum covers every uniform a consumer would plausibly need to populate from SDK-managed state:

| Group | Examples |
|---|---|
| Transform matrices | `modelViewMatrix`, `projectionMatrix`, `normalMatrix`, `modelMatrix`, `viewMatrix`, `textureMatrix` |
| Lights | `numLights`, `lightColor[]`, `lightDirection[]`, `lightAttenuation[]`, `lightBeamWidth[]`, `lightCutOffAngle[]` |
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

The PoC consumer in `examples/poc_renderer/main.cpp` demonstrates the four-program dispatch:

| Condition | Program | Shader files |
|---|---|---|
| `item.shader` non-null | author | compiled per-ComposedShader, cached by source hash (PATH 4; currently unreachable — ComposedShader extraction not yet wired) |
| `topology != Triangles OR !hasNormals` | unlit | `unlit.vert` / `unlit.frag` |
| `model == Physical` | PBR | `lit.vert` / `pbr.frag` |
| `model == Phong` | Phong | `lit.vert` / `lit.frag` |

`pbr.frag` implements a metallic-roughness **analytic BRDF** (Cook-Torrance NDF + Schlick Fresnel + Smith geometry, sRGB output).  **IBL (image-based lighting) is not implemented** — `EnvironmentLight` is an X3D 4.1 node and the generated binding layer is locked to X3D 4.0; IBL is deferred (see deferred note below).

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
that *executes* author `ComposedShader` fragment source on the CPU — the
`RenderItem::shaderProgram` path, reachable today via the binary's `--frag` flag
even before the SDK wires ComposedShader extraction. It binds the same
`ShaderUniformVocabulary` names this seam defines, making the shader seam testable
as a GPU-free golden-image harness; see `examples/cpu_raster/README.md`.

## Related specs and ADRs

- [ADR-0021: Material + Shader Design](../decisions/0021-material-shader-design.md) — discriminated union + vocab + introspection binding decisions.
- [Materials subsystem](materials.md) — `MaterialDesc` discriminated union this seam sits alongside.
- [Texture, Material, and Light Extraction](extract-textures.md) — the broader extraction pipeline.
- Design spec: `docs/superpowers/specs/2026-06-21-material-shader-design.md`
- Source files: `runtime/extract/ShaderUniformVocabulary.hpp`, `runtime/extract/ShaderBindingPlan.hpp`, `runtime/extract/RenderItem.hpp`, `runtime/extract/X3DFieldValue.hpp`
- PoC consumer: `examples/poc_renderer/main.cpp`, `examples/poc_renderer/shaders/pbr.frag`
