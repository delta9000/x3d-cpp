---
title: "ADR-0027: A Single Reference Lighting Evaluator for the Spec-Normative Shading Path"
summary: "X3D §17 pins a normative lighting equation (per-light contribution, ambient/diffuse/specular terms, the shadowTest modulation), so unlike shadow *generation* (ADR-0028) the shading math HAS a reference result. This ADR names that result the responsibility of one reference lighting evaluator — the function the PoC/cpu_raster MaterialShader already approximates — and makes the open color-space, ambient, and shadow-modulation conventions decisions of that evaluator rather than per-consumer accidents. Proposed."
tags: [adr, lighting, materials, rendering, conformance, thesis, proposed]
updated: 2026-06-26
related:
  - ../architecture.md
  - 0021-material-shader-design.md
  - 0028-shadow-visibility-seam.md
  - ../subsystems/materials.md
---

# ADR-0027: Reference Lighting Evaluator

## Status

**Proposed** (2026-06-26). The evaluator exists today only as the reference
consumers' `MaterialShader` (PoC + cpu_raster); this ADR names it, fixes the
conventions it must pin, and is the decision ADR-0028 already cites as the home
of the spec-normative shadow modulation. No SDK API ships with this ADR.

## Context

ADR-0021 settled how a material is *described* (`MaterialDesc`: Phong / Physical /
Unlit, `ShaderUniformVocabulary`, `buildBindingPlan()`). It deliberately stopped
at the descriptor and left *evaluation* — turning a `MaterialDesc` + lights into a
pixel color — to the consumer. Two facts make that boundary need its own decision:

- **The shading math has a normative result.** X3D §17 (ISO/IEC 19775-1) pins the
  lighting equation: per-light ambient/diffuse/specular contributions, the
  attenuation and spotlight terms, and the `shadowTestᵢ` modulation
  (`shadows ? (occluded ? 1−shadowIntensity : 1) : 1`). This is the opposite of
  shadow *generation*, which the same spec leaves implementation-defined — the
  distinction ADR-0028 turns on. Because §17 pins a *result*, a *reference*
  evaluator can legitimately exist (ADR-0028 calls this "ADR-0027's premise").

- **The conventions around that math are currently unpinned and divergent.** The
  reference consumers' `MaterialShader.hpp` (and the matching GLSL) carry several
  decisions that no ADR records and that disagree with each other:
  - Phong **ambient squares diffuse** rather than being linear in it.
  - Authored colors are **not sRGB-decoded**, while only the *lit* paths
    re-encode — so the same authored `0.8` renders differently through
    `UnlitMaterial` vs `Material`.
  - The per-light **`shadowTest` is hardcoded to `1`** (the "no shadow backend
    wired" stand-in ADR-0028 describes), with no single place that owns applying
    it once ADR-0028's `ShadowQuery` seam lands.

  These are the substance of the rendering findings in the GitHub Project
  (color-space / ambient ADR; lighting/texture-model divergences) — symptoms of a
  missing owner, not independent bugs.

## Decision

**1. There is one reference lighting evaluator.** The §17 lighting equation is
evaluated in a single place — conceptually `shade*(MaterialDesc, surface, lights,
…) -> color` — and that place is the *reference result* for the spec-normative
shading path. The SDK still ships descriptors, not a renderer; the evaluator is
the reference consumers' shared shading core (PoC + cpu_raster `MaterialShader`),
elevated from "incidental code" to "the thing conformance is measured against."

**2. The evaluator owns the unpinned conventions.** The following are decisions of
this evaluator, pinned once, not per-consumer:
   - **Color space.** Decode authored sRGB colors to linear at build time, do all
     lighting in linear, and encode outputs back to sRGB — uniformly across
     Unlit/Phong/Physical so the same authored value matches.
   - **Ambient term.** The ambient contribution is *linear* in diffuse, not
     squared.
   - **Shadow modulation.** The evaluator takes a per-light `visibility ∈ [0,1]`
     (and `shadowIntensity`) and applies `shadowTestᵢ` exactly per §17. This is
     the API surface ADR-0028's `ShadowQuery` seam feeds; the current hardcoded
     `1` becomes the documented "no shadow backend wired" default.

**3. Conformance is proven by value where the spec pins a value.** Unlike the
shadow-*visibility* seam (ADR-0028, proven on structural invariants because the
spec gives no normative result), the lighting equation is checked against expected
colors for a fixture set — the evaluator is a reference precisely because §17
admits one.

## Scope / non-goals

- **Not a renderer.** No GPU pipeline, no SDK render API — this records *which*
  math is normative and *who* owns the conventions, so the reference consumers and
  any third-party consumer agree on results.
- **Shadow generation is out of scope** — that is ADR-0028's seam. This ADR owns
  only the *modulation input* to the equation.
- **IBL / EnvironmentLight remain deferred** (ADR-0021): the reserved
  `envDiffuse`/`envSpecular`/`envSH`/`brdfLUT` vocabulary entries are not part of
  the normative §17 path.

## Consequences

- **+** Gives ADR-0028 the evaluator it references — the dangling dependency is
  resolved, and the shadow modulation has a defined home.
- **+** Turns the color-space, ambient, and lighting-model divergences from
  scattered per-consumer accidents into one pinned, testable convention.
- **+** Restores a contiguous ADR sequence (0026 → 0027 → 0028) for the coverage
  manifest.
- **−** Promotes reference-consumer code to a conformance surface, so changing the
  shading math now requires updating fixtures and this ADR.
- **−** Pinning sRGB/linear and the ambient term will shift reference-render golden
  values when implemented (a deliberate, gated change, not free).

## Alternatives considered

1. **Leave evaluation entirely to consumers (status quo).** Rejected: ADR-0028
   needs a named owner for the normative modulation, and the unpinned conventions
   keep producing the divergences tracked in the Project.
2. **Fold these conventions into ADR-0021.** Rejected: ADR-0021 is explicitly the
   *descriptor* decision and stops at the seam; evaluation and its normative result
   are a distinct concern that ADR-0028 already cites by its own number.
3. **A full SDK rendering API.** Rejected: out of the headless, renderer-agnostic
   scope — the SDK ships descriptors; the evaluator is the reference *consumer's*
   shared core, not a product API.
