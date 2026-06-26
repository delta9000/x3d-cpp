---
title: "ADR-0027: Reference Lighting Evaluator — spec-normative shading math as an SDK reference + conformance oracle"
summary: The extraction seam surfaces light/material PARAMETERS (LightDesc, MaterialDesc) but the lighting EQUATIONS are re-implemented per consumer — the CPU rasterizer's shader "mirrors the PoC's buildEyeLights" — and the math is spec-pinned (X3D §17 fully defines the Phong model; PhysicalMaterial defers the BRDF to glTF 2.0). Add a renderer-agnostic, header-only reference evaluator under runtime/shade that implements those equations once, returning LINEAR radiance (gamma/tonemap/textures/projection stay consumer-side). It is OPTIONAL — GPU consumers reimplement in GLSL for performance — and its correctness is proven the same way as the engine seams: a CI agreement-test that a consumer's shading matches the reference within tolerance, making the reference a conformance oracle. Proposed, not yet implemented.
tags: [adr, seam, lighting, shading, pbr, phong, reference, conformance, thesis, proposed]
updated: 2026-06-24
related:
  - ../architecture.md
  - ../seam-status.md
  - ../subsystems/shaders.md
  - 0021-material-shader-design.md
  - 0024-textureresolver-second-backend-swap-test.md
---

# ADR-0027: Reference Lighting Evaluator

## Status

**Proposed** (2026-06-24). Sketch — no code yet. Motivated by the headless CPU
rasterizer (`examples/cpu_raster/`) acting as a *second* consumer of the
extraction seam alongside the PoC GL renderer (`examples/poc_renderer/`), which
exposed that the lighting **equations** are duplicated per consumer.

## Context

The product thesis is that x3d-cpp is unopinionated and pluggable: the runtime
core stays spec-correct and backend-free, and every renderer/engine boundary is
an abstract **seam**. The extraction seam ([ADR-0021](0021-material-shader-design.md))
already does this for lighting *inputs* — `LightSystem` resolves
`LightDesc` (type, world position/direction, color, intensity, attenuation,
radius, beam cone) and `MaterialDesc` carries the Phong / PhysicalMaterial /
Unlit parameters. That half of the boundary is healthy: when a consumer renders
wrong, it is almost always because it **ignored** a descriptor the SDK already
populated (point lights, the `Background` ramp, `TextureCoordinateGenerator`),
not because the descriptor was missing.

What is **not** behind a seam is the lighting **evaluation** — the equation that
turns those descriptors into radiance. Today each consumer re-derives it:
`examples/cpu_raster/cpuraster/MaterialShader.hpp` states it "mirrors the PoC's
EyeLight exactly (buildEyeLights in main.cpp)." Two hand-written copies of the
same math, with no shared source and no oracle. That is a drift risk and a
correctness blind spot — and it is avoidable, because the math is **normative**:

- **Phong (`Material`)** — X3D §17 (ISO/IEC 19775-1) defines it in full:
  `lightContributionᵢ = onᵢ × shadowTestᵢ × lightColorᵢ × attenuationᵢ × spotᵢ ×
  (ambientᵢ + diffuseᵢ + specularᵢ)`, with `diffuseᵢ = intensity × diffuseParam ×
  (N·L)`, `specularᵢ = intensity × specularParam × (N·((L+V)/|L+V|))^shininess`,
  and `fragmentColor = applyFog(emissive + occlusion(Σᵢ lightContributionᵢ))`.
  X3D is the normative source.
- **PBR (`PhysicalMaterial`)** — X3D **defers** the BRDF: the Shape component
  says PhysicalMaterial is "deliberately consistent with the glTF 2.0 material
  definition" and gives no microfacet equations. The GGX distribution, Smith
  visibility, and Schlick-Fresnel kernel are normatively **glTF 2.0's**, not
  X3D's. glTF is the normative source (by X3D's own deferral).

Spec-pinned math that two consumers re-derive is exactly the kind of thing the
SDK should own one reference for.

### Why this is NOT an engine-backend seam

Unlike AudioBackend / TextureResolver / ScriptEngine, this has no external
engine to plug in and no second independent implementation to "prove genericity"
against. It is a **reference implementation of normative math**. The seam aspect
is replaceability: a GPU consumer will reimplement these equations in GLSL for
performance and will not call CPU code in its hot path. So the proof is not
"backend A vs backend B agree" but "**consumer vs reference agree**" — which
doubles as a conformance oracle for every consumer.

## Decision

Add a renderer-agnostic, header-only **reference lighting evaluator** in the
runtime (proposed `runtime/shade/ReferenceLighting.hpp`, namespace
`x3d::runtime::shade`). Pure math: no GPU, no I/O, no texture sampling, no
gamma/tonemap. Core-style header-only, zero third-party deps.

**Boundary contract (this is the point):** the evaluator owns the spec equation;
the consumer owns everything presentational. Specifically the consumer does
geometry/projection, **texture sampling + filtering**, normal-mapping, and the
final **linear→display encode (gamma/tonemap/sRGB)**. The evaluator takes
already-sampled material values and returns **linear radiance**. This mirrors
the decode/encode split we settled in the PNG work: the SDK owns what the pixels
*should be*; the consumer owns how they are *presented*.

**Space contract:** the evaluator is frame-agnostic — all vectors (surface
normal, view, light position/direction) must be in ONE caller-chosen frame (eye
or world). The caller resolves `LightDesc` (world-frame) into that frame.

Sketch (illustrative — names/types to be finalized; `vec3` is the runtime vector
type):

```cpp
namespace x3d::runtime::shade {

struct Surface { vec3 position, normal, view; }; // one consistent frame; unit N,V

// Material values already sampled at the fragment (textures resolved by caller).
struct PhongInputs    { vec3 diffuse, specular, emissive, ambient; float shininess; };
struct PhysicalInputs { vec3 baseColor, emissive; float metallic, roughness, occlusion; };

// A LightDesc resolved into the eval frame (caller applies its view transform).
struct EvalLight {
  enum class Type { Directional, Point, Spot } type;
  vec3 color; float intensity, ambientIntensity;
  vec3 direction;            // direction of travel (Directional/Spot)
  vec3 position;             // (Point/Spot)
  vec3 attenuation;          // (c0,c1,c2)
  float radius, beamWidth, cutOffAngle;
  bool  shadows;             // X3DLightNode.shadows
  float shadowIntensity;     // X3DLightNode.shadowIntensity [0,1]
  float visibility;          // §17 shadowTest input: 1 = lit, 0 = occluded.
                             // Supplied by the caller's ShadowQuery (ADR-0028);
                             // defaults to 1 when no shadow backend is wired.
};
// The evaluator applies §17 verbatim: shadowTestᵢ = light.shadows
//   ? mix(1.0, 1.0 - shadowIntensity, 1.0 - visibility) : 1.0, folded into the
// per-light contribution. It NEVER computes visibility — that is the seam.

// X3D §17 Phong, verbatim.
vec3 shadePhong(const PhongInputs&, const std::vector<EvalLight>&, const Surface&);

// glTF 2.0 metallic-roughness BRDF (per X3D's PhysicalMaterial deferral).
vec3 shadePhysical(const PhysicalInputs&, const std::vector<EvalLight>&, const Surface&);

// Both return LINEAR radiance. Unlit is trivial (emissive/baseColor passthrough).
} // namespace x3d::runtime::shade
```

**Conformance proof (reuses the swap-test discipline):** a CI test renders a
fixture set through the reference evaluator and through the CPU rasterizer's
shader and asserts agreement within tolerance (the same shape as the audio /
texture swap-tests, but "consumer vs reference" rather than "backend vs
backend"). Once green, the reference is the oracle any future consumer is
checked against, and the CPU rasterizer can optionally call it directly to
collapse the duplication.

**Normative sourcing is documented in-code:** the Phong path cites X3D §17; the
Physical path cites a pinned glTF 2.0 BRDF revision.

## Scope / non-goals (deferred, not dropped)

- **Shadows — modulation IN, visibility OUT.** The §17 shadow *modulation*
  (`shadowTest`, scaled by `shadowIntensity`) is normative and lives here, as the
  per-light `visibility` input above. The shadow *visibility query* (the
  technique X3D leaves implementation-defined) is a separate seam — ADR-0028 —
  whose first-party backend supplies `visibility`. With no backend wired,
  `visibility = 1` (the current no-shadows behavior).
- **IBL / `EnvironmentLight`** — blocked by the X3D 4.0 generated-binding lock
  (4.1 node); see `shaders.md`. Out of scope until that is resolved.
- **Fog** — `applyFog` is named in the §17 fragment equation; a sibling
  reference (`FogModel`) can land alongside once `FogDesc` is extracted.
- **Texture sampling, normal-mapping resolution, gamma/tonemap** — consumer-side
  by contract (above).

## Consequences

- **+** Removes the per-consumer lighting duplication; one spec-normative source.
- **+** Gives a **conformance oracle** + regression guard for any consumer, and
  sharpens the SDK/consumer boundary (meaning vs presentation) with a concrete
  artifact rather than a convention.
- **+** A useful teaching/reference: the equations live in one readable,
  spec-cited place.
- **−** A CPU evaluator is **not** what a GPU consumer runs in production — there
  its value is as an oracle in tests, not a runtime dependency. "Optional" is
  load-bearing: some consumers use it only under test.
- **−** The PBR path tracks an **external moving normative source** (glTF); pin a
  revision and treat glTF spec bumps as a deliberate update.
- **Risk:** the frame/space contract must be crisp or consumers feed inconsistent
  inputs and the oracle disagrees for the wrong reason.

## Alternatives considered

1. **Status quo — duplicate per consumer.** Rejected: drift, no oracle, and the
   math is spec-pinned so re-derivation is pure cost.
2. **A shared helper inside the example/consumer** (e.g. promote
   `MaterialShader.hpp`'s math into a cpu_raster utility). Rejected: it is spec
   math, not consumer math — another consumer cannot reuse a consumer's helper,
   and it would not be a runtime-blessed reference/oracle.
3. **Model it as a swappable engine backend** (AudioBackend-style, with a second
   independent backend for the genericity proof). Rejected as a mismatch: there
   is no external engine and no meaningful "second backend" — the honest proof is
   consumer-vs-reference agreement, not backend-vs-backend.
