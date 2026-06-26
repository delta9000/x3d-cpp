---
title: "ADR-0028: Shadows — normative modulation in the evaluator, technique-defined visibility as a seam"
summary: X3D §17 prescribes the shadow MODULATION (shadowTestᵢ = shadows ? (occluded ? 1−shadowIntensity : 1) : 1, multiplied into each light's contribution) but explicitly leaves the shadow-GENERATION technique implementation-defined ("X3D browser implementations are allowed some variations in shadow rendering"). So shadows split across the boundary: the modulation is spec-normative and belongs in the reference lighting evaluator (ADR-0027), while the visibility query — "is point P occluded from light L by castShadow geometry?" — is a SEAM with no single normative result. This ADR defines that ShadowQuery seam, a first-party CPU ray-cast default backend over the extracted castShadow geometry, and a structural-invariant proof (modeled on the ADR-0026 spatial-audio proof, not bit-exact). The data dependency — surfacing X3DShapeNode.castShadow on RenderItem — ships with this ADR. Proposed.
tags: [adr, seam, shadows, lighting, conformance, thesis, proposed]
updated: 2026-06-24
related:
  - ../architecture.md
  - ../seam-status.md
  - ../subsystems/extract.md
  - 0027-reference-lighting-evaluator.md
  - 0026-audiobackend-second-backend-swap-test.md
  - 0021-material-shader-design.md
---

# ADR-0028: Shadow Visibility Seam

## Status

**Proposed** (2026-06-24). The `castShadow` extraction (below) is **implemented**
in the same change; the seam + default backend are the sketch.

## Context

Shadows look like one feature but are two, and they fall on opposite sides of the
SDK/consumer boundary — the spec draws the line itself. From X3D §17
(ISO/IEC 19775-1, verified):

- **Prescribed — the modulation.** `X3DLightNode` has `shadows` (SFBool, default
  FALSE) and `shadowIntensity` (SFFloat [0,1], default 1). The lighting equation
  multiplies each light's contribution by `shadowTestᵢ`, where `shadowTestᵢ = 1.0`
  when `shadows=FALSE` or the light is unobstructed, and `1.0 − shadowIntensity`
  when obscured by a shadow-caster. This is exact and normative. It is also
  **hard** (a binary occlusion scaled by intensity) — penumbra is not in the
  normative equation.
- **Not prescribed — the technique.** The spec says *"X3D browser implementations
  are allowed some variations in shadow rendering"* and only counts geometry
  whose `X3DShapeNode.castShadow` is TRUE. No shadow-map / ray-cast / shadow-volume
  algorithm, no bit-exact result, no required softness.

A "reference implementation" only exists where the spec pins a *result*
(ADR-0027's premise). The modulation has one; the visibility does not. So shadows cannot be a single reference — the
visibility half must be a **seam** (like AudioBackend / TextureResolver), and
the SDK can ship *a* default backend + prove it on **invariants**, not values.

## Decision

**1. Modulation → the reference evaluator (ADR-0027).** `shadowTest` is an input
to `shade*()`, not something the evaluator computes. ADR-0027's API gains a
per-light `visibility ∈ [0,1]` (and `shadowIntensity`); the evaluator applies
`shadowTestᵢ = lit ? 1 : (1 − shadowIntensity)` exactly per §17. Hardcoding it to
`1` (today's stand-in) becomes the "no shadow backend wired" default.

**2. Visibility → a ShadowQuery seam.** A renderer-agnostic interface:

```cpp
namespace x3d::runtime::shade {
// visibility ∈ [0,1]: 1 = light fully reaches the point, 0 = fully occluded.
// (A backend MAY return fractional values for soft shadows; the spec-normative
// hard result is {0,1} and the default backend returns those.)
using ShadowQuery =
    std::function<float(const SurfacePoint& p, const EvalLight& light)>;
}
```

**3. First-party default backend — CPU ray cast.** The extractor already hands
the consumer world-space triangles per `RenderItem`; a brute-force occlusion ray
from the surface point toward the light, tested against every `RenderItem` whose
`castShadow == true`, is a real, usable default — no GPU, fitting the headless
rasterizer's ethos — and doubles as the **oracle** for the invariants below. A
GPU consumer ignores it and supplies a shadow-map-backed `ShadowQuery`.

**4. Proof = structural invariants, not numeric agreement** (the ADR-0026
spatial-audio pattern, because two shadow techniques legitimately disagree on
bias/penumbra). A CI test asserts, over a fixture set, that any `ShadowQuery`:
- returns `1` for every light when `shadows == FALSE` (the gate);
- returns `< 1` for a point with a `castShadow` occluder squarely on the
  point→light segment, and `1` for an unobstructed point;
- ignores occluders with `castShadow == false`;
- is monotonic — adding an occluder never increases visibility;
- produces **no self-shadow acne** on a convex lit surface facing the light
  (bias invariant).

### Data dependency (implemented here)

The query needs to know which shapes occlude. `X3DShapeNode.castShadow` (X3D
default TRUE) is now **surfaced on `RenderItem.castShadow`** by the extractor
(`emit()` / `emitPacked()` read it reflection-generically; covered by
`castshadow_extract_test`). This was another "extracted-but-absent" field — the
SDK now carries it; the consumer/seam interprets it.

## Scope / non-goals

- **Hard shadows only** in the normative path and the default backend — the §17
  modulation is binary × `shadowIntensity`. **Soft shadows / penumbra** are a
  backend's prerogative (the seam permits fractional visibility) but are not
  required and not in the default.
- **No shadow maps** in the first-party default (it ray-casts); GPU consumers
  bring their own technique through the seam.
- **Self-shadow / receiver setup, light-frustum culling, cascade selection** —
  backend concerns, out of scope for the contract.

## Consequences

- **+** Resolves "how do you write a *reference* shadow impl" honestly: you
  don't — the reference is the *modulation*; the visibility is a seam with a
  default backend proven on invariants.
- **+** Makes shadows first-party without over-committing the SDK to one
  algorithm the spec deliberately leaves open.
- **+** `castShadow` is now available to every consumer (was dropped).
- **−** The default CPU ray cast is O(points × lights × occluder-tris) — a
  correctness/oracle tool, not a production renderer path; real-time consumers
  supply their own `ShadowQuery`.
- **−** "Allowed variations" means the invariant test can only assert structure,
  so two conformant backends can look visibly different (expected, per spec).

## Alternatives considered

1. **A single reference shadow algorithm in the SDK.** Rejected: the spec gives
   no normative result, so any choice (shadow maps vs rays vs volumes) would be
   arbitrary and mis-sold as "the" reference.
2. **Leave shadows entirely to consumers (no seam).** Rejected: then `castShadow`
   stays unsurfaced and every consumer re-invents the occluder-collection plumbing;
   the seam + default gives a shared contract and an oracle.
3. **Bake occlusion at extraction time.** Rejected: visibility is view/technique
   dependent and dynamic; baking it would be the wrong side of the boundary (the
   same baking-is-a-one-way-door caution from the texture-transform discussion).
