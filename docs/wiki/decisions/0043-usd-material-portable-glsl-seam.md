---
title: "ADR-0043: USD Material Port — UsdPreviewSurface Ceiling & Portable-GLSL Seam"
summary: Port USD/USDZ materials to realtime by treating UsdPreviewSurface as the input ceiling and a single portable GLSL fragment shader (run by both the CPU interpreter and desktop GL) as the frozen seam contract, with a specialized --emit-glsl codegen path carrying the fidelity the X3D PhysicalMaterial node cannot model.
tags: [adr, asset-import, materials, shaders, usd, glsl, seam]
updated: 2026-07-02
related:
  - ../subsystems/asset-import.md
  - ../subsystems/materials.md
  - ../subsystems/shaders.md
  - 0042-authoring-target.md
  - 0021-material-shader-design.md
---

# ADR-0043: USD Material Port — UsdPreviewSurface Ceiling & Portable-GLSL Seam

## Status

Accepted

## Context

The USD/USDZ import backend (tinyusdz → tydra `RenderScene` → `ImportScene`,
see [asset-import](../subsystems/asset-import.md)) reliably carries geometry, but
the question of *how much material data survives, and how to render it in realtime*
was unresolved. An earlier investigation concluded "materials are stubs all the way
down — only geometry carries over." A per-parameter audit found that conclusion was
an artifact of a **stale, textureless `DamagedHelmet.x3d`** left in the tree;
re-converting the real `.usdz` with current code extracts all five PBR texture slots
plus scalars correctly. Materials **do** port. The real limits are a stack of four
distinct ceilings, and naming them is the substance of this decision.

1. **Input ceiling — the shading *model*, not the parameters.** tinyusdz's tydra
   converter resolves `UsdPreviewSurface` fully (13 inputs + one-hop `UsdUVTexture`
   connections: wrap/scale/bias/colorspace/UV-primvar), but hard-requires
   `outputs:surface` to connect to a `UsdPreviewSurface` node — any other network
   (Arnold `PxrSurface`, a general UsdShade graph) fails the whole material, and
   MaterialX `standard_surface` / `ND_standard_surface` is a literal no-op stub.
   Porting arbitrary MaterialX node graphs to hand-written GLSL is intractable in
   general (it is why Pixar's own Storm delegates to MaterialX's `GlslShaderGenerator`
   rather than reimplementing it). The ecosystem's own answer — Houdini's
   `mtlxstandard_surface_to_UsdPreviewSurface`, OpenUSD's downgrade translation
   graphs — is to *flatten MaterialX down to UsdPreviewSurface, else skip*.

2. **IR ceiling — our own consumer was the tighter bottleneck.** The material loop
   in `usd_source.cpp` read only diffuse/emissive/specular/opacity/roughness/metallic
   and dropped fields tydra already populates (`ior`, `clearcoat`,
   `clearcoatRoughness`, `opacityThreshold`, `useSpecularWorkflow`, the specular
   texture, `opacityMode`).

3. **Realtime BRDF.** UsdPreviewSurface's spec text prescribes no BRDF math; the
   canonical realtime reference is Pixar Storm's `previewSurface.glslfx` — GGX
   (Trowbridge-Reitz) `D` + Schlick-GGX Smith `G` + Schlick `F`, `α = roughness²`,
   `F0 = mix(((1-ior)/(1+ior))², baseColor, metallic)` — which is near-identical to
   glTF 2.0 Appendix B. The repo already shades this in two places: the CPU
   rasterizer's `MaterialShader` (fixed-function) and `poc_renderer/pbr.frag`
   (desktop GL). The CPU rasterizer also ships a GLSL *interpreter*
   (`GlslInterpreter.hpp`, the `--frag` path), so generated GLSL is executable, not
   just emittable.

4. **X3D material-model ceiling.** The natural emit target — the X3D 4.0
   `PhysicalMaterial` node — models only baseColor/metallic/roughness/emissive/
   occlusionStrength/transparency + texture slots (i.e. glTF metallic-roughness). It
   has **no** `ior`, `clearcoat`, `specularColor`/`useSpecularWorkflow`, or
   `opacityMode`. So the `USD → X3D → renderer` path is structurally capped at
   glTF-MR fidelity regardless of what the IR carries.

## Decision

Port USD materials to realtime as a **portable-GLSL seam**, consistent with the
repo's seam-genericity discipline (frozen contract + two independent backends +
a swap-test — see the seam-status matrix):

1. **UsdPreviewSurface is the input ceiling.** MaterialX / non-preview networks are
   flatten-or-skip; we do not vendor a MaterialX code generator.

2. **A single portable reference shader is the frozen contract.**
   `examples/cpu_raster/shaders/usd_preview_surface.frag` implements the Storm/glTF
   reference BRDF and is written to run **byte-identically** in *both* backends: the
   CPU rasterizer's GLSL interpreter and desktop OpenGL (`poc_renderer`). It uses
   only the shared varying/uniform contract and the interpreter's builtin subset —
   e.g. a scalar `pow(1/2.2)` sRGB encode rather than the vector-relational
   `lessThan`/`bvec3` piecewise form, which is not in the interpreter's envelope.
   `glslangValidator -S frag` confirms it is legal desktop GLSL 330; at spec-default
   uniforms it is pixel-parity (MAD < 1/255) with the native fixed-function PBR.

3. **The interpreter binds the full texture-slot set + fidelity uniforms.**
   `seedUniforms` previously exposed only the base-color sampler to author GLSL; it
   now binds normal/emissive/metallic-roughness/occlusion samplers (+ `uHas*Tex`
   guards + `uOcclusionStrength`) and seeds the UsdPreviewSurface fidelity uniforms
   (`uUseSpecularWorkflow`/`uSpecularColor`/`uIor`/`uClearcoat`/`uClearcoatRoughness`/
   `uOpacityMode`/`uOpacityThreshold`) to their spec fallbacks. At those fallbacks the
   shader collapses to the plain metallic-roughness path, so the direct render path
   is unchanged; the extras only engage when a host binds real values.

4. **`ImportMaterial` is promoted to a UsdPreviewSurface-grade IR** carrying the
   fields tydra already gives us, with `AlphaMode` derived from `opacityThreshold`/
   `opacity`.

5. **Fidelity past the X3D ceiling ships as specialized codegen.** Because the X3D
   `PhysicalMaterial` node cannot express `ior`/`clearcoat`/specular-workflow/
   `opacityMode`, the asset-import `--emit-glsl` mode emits a self-contained
   fragment shader per material with those parameters **baked as `const`** — a pure
   text transform over the one canonical shader (uniform → const substitution), so
   there is a single BRDF source of truth. This is the only faithful realtime channel
   for the fidelity the X3D intermediate drops.

## Consequences

### Positive
- **One shader, two backends, proven.** The same source renders in the CPU
  interpreter and desktop GL; the CPU path matches the native fixed-function PBR to
  < 1/255 MAD, and the full `USDZ → X3D → interpreter` loop renders the real
  DamagedHelmet with normal/MR/occlusion maps landing.
- **Honest fidelity story.** Two rendering paths with clearly stated ceilings: the
  `USD → X3D → renderer` path is glTF-MR-faithful; `--emit-glsl` is the escape hatch
  for full UsdPreviewSurface fidelity.
- **No new heavy dependency.** UsdPreviewSurface is a fixed 14-input uber-shader; we
  hand-map it rather than vendoring MaterialX shader generation.

### Trade-offs / costs
- **Cross-example coupling.** `--emit-glsl` embeds the canonical shader that lives in
  the `cpu_raster` example; the asset-import build references it across example
  directories.
- **MaterialX and general UsdShade graphs are unsupported** (flatten-or-skip), and
  a material that connects roughness and metallic to *different* textures collapses to
  one metallic-roughness slot (the single-slot limit shared with X3D).
- **Host-contract obligation.** Both backends must bind the fidelity uniforms; an
  unbound `uIor` defaults to 0 on GL and yields a mirror F0 — the swap-test exists to
  keep that contract honest.

## Related

- [Asset-Import subsystem](../subsystems/asset-import.md) · [Materials](../subsystems/materials.md) · [Shaders](../subsystems/shaders.md)
- [ADR-0042: Slim Authoring Target](0042-authoring-target.md) — the import/emit pipeline this extends
- [ADR-0021: Material & Shader Model](0021-material-shader-design.md) — the `MaterialDesc` / reference-shader model reused here
- `examples/cpu_raster/shaders/usd_preview_surface.frag` — the frozen seam contract
- `examples/cpu_raster/cpuraster/GlslInterpreter.hpp` — `seedUniforms` slot/fidelity binding
- `examples/asset_import/glsl_emit.{hpp,cpp}` — `--emit-glsl` specialized codegen
