---
title: "ADR-0031: Deterministic SCP Frames for Degenerate Extrusion Spines"
summary: "For a 2-distinct-point (or otherwise underdetermined) Extrusion spine, the in-plane SCP axes are aligned to the local coordinate system; spines with fewer than 2 distinct points render nothing."
tags: [adr, geometry, extrusion, scp, determinism, conformance]
updated: 2026-06-25
related:
  - ../architecture.md
  - ../subsystems/extract.md
  - ../../conformance/findings.yaml
---

# ADR-0031: Deterministic SCP Frames for Degenerate Extrusion Spines

## Status

Proposed — 2026-06-25. Conformance finding `EXTRUSION-SCP`.

## Context

Extrusion builds a Spine-aligned Cross-section Plane (SCP) per spine point. §13.3.5.4.5
(`geometry3D.md:435`) says that for a 2-distinct-point spine the SCP planes are "perpendicular to the
vector defined by these two points" — this fixes the plane **normal** but leaves the in-plane X/Z
orientation completely free (a full `[0,2π)` of rotation). Browsers therefore pick different in-plane
frames and the same model renders differently (the canonical symptom: X3DOM renders the Harrier wings
backwards). A single distinct spine point has no defined SCP at all, yet §13.3.5.4.5 also requires
that "fewer than 2 non-coincident spine points … no results are rendered."

x3d-cpp today (`runtime/extract/MeshBuilder.hpp` `tessellateExtrusion()`): the count guard `:951`
`if (ns < 2 || nc < 2) return;` counts spine *elements*, not *distinct* points, so a
one-distinct-point spine renders a degenerate mesh. For the 2-point case `haveZ` stays false and
control falls into an arbitrary `ref × Y` branch (`:1043-1049`) — deterministic but not model-frame
aligned, producing the sign flip ("wings backwards").

## Decision

Adopt a deterministic in-plane rule (Plesch's local-coordinate alignment) plus a distinct-point cull:

1. **Cull** when fewer than 2 *distinct* (coincident-collapsed using the existing `eps`) spine points
   — `return` an empty mesh, implementing §13.3.5.4.5 sentence 2.
2. **Local-axis-aligned in-plane fallback** for the 2-point / undefined-Z case. With `Y` = unit spine
   tangent: `Z = normalize(modelZ − (modelZ·Y)·Y)` (fall back to `modelX` when `Y ∥ modelZ`),
   `X = normalize(Y × Z)`, re-orthonormalize `Z = X × Y`. The SCP X/Z become the model X/Z (or the
   nearest in-plane approximation), with no `ref × Y` sign flip.

Only underdetermined sections change; spines with ≥3 non-collinear distinct points are untouched.

## Consequences

**Positive:**
- Deterministic, reproducible SCP for degenerate spines; agrees with conforming browsers (Harrier
  wings render correctly).
- A cross-implementation edge-case comparison for this exact 2-point/undefined-orientation scenario
  shows two independent conforming implementations (Castle Game Engine/view3dscene and X_ITE)
  already agreeing with each other on the rendered result, while X3DOM (noted above) renders it
  differently — corroborating that local-coordinate alignment matches where implementations have
  already converged.
- Implements the §13.3.5.4.5 "render nothing" rule for single-distinct-point spines.
- Non-regressive on well-defined spines (B3/B6/TC3 acceptance unaffected).

**Trade-offs:**
- A choice the spec leaves open is now fixed in the engine; if 4.1 later mandates a different in-plane
  convention this ADR is superseded.

## X3D 4.1 alignment

Partial. 4.1-CD added the coincident/distinct-spine-point sections (§13.3.5.4.3/.4.4) but the 2-point
**in-plane** axes remain unpinned (a "Frenet frame" idea was floated, not adopted). The engine rule is
a candidate ahead of the spec; an upstream erratum proposing the local-coordinate-projection rule is owed.
