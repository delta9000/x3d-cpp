---
title: "ADR-0040: NURBS Curve + Patch Tessellation is First-Party, Not a Seam"
summary: NURBS curve and patch-surface tessellation ships as a first-party, I/O-free math unit (runtime/extract/NurbsEval.hpp) wired into MeshBuilder — the same species as Box/Sphere/Extrusion tessellation — rather than behind a swap-tested seam, because NURBS evaluation needs no outside world and the X3D §27 spec prescribes the result (no genericity payoff). The externalGeometryResolver stays the unrecognized-geometry fallback, now serving the deferred trimmed/swept/swung nodes. Control-point convex-hull AABB bounds; clamped-uniform default knots; full periodic closed/uClosed/vClosed handling. Closes NRB-1 for curve + patch (NRB-3 tracks the deferred surfaces).
tags: [adr, nurbs, extract, mesh-builder, tessellation, geometry, seam]
updated: 2026-06-27
related:
  - ../subsystems/extract.md
  - 0001-ext-firewall.md
  - 0015-extraction-pull-per-path.md
  - 0022-scriptengine-second-backend-swap-test.md
---

# ADR-0040: NURBS Curve + Patch Tessellation is First-Party, Not a Seam

## Status

Accepted — 2026-06-27

## Context

`NurbsCurve` and `NurbsPatchSurface` were generated data-holding stubs that
`MeshBuilder` did not recognize, so extraction silently dropped them (conformance
finding NRB-1, `major`, DEFERRED — "blocked on a NURBS tessellation subsystem").
Two corpus sweeps reported NURBS as one of the two remaining categories of skipped
geometry (the other being the §14 2D primitives, G2D-1).

The open design question was *where* NURBS tessellation belongs: a first-party arm
in `MeshBuilder` (like Box/Sphere/Cone/Cylinder/Extrusion), or behind a genericity
seam with the swap-test ritual (frozen interface + 2 independent backends + CI
swap-test) that this codebase applies to AssetResolver, TextureDecode, FontMetrics,
Audio, ScriptEngine, and Physics.

## Decision

**NURBS curve + patch tessellation is first-party.** A node-free math unit
`runtime/extract/NurbsEval.hpp` (namespace `x3d::runtime::extract::nurbs`) implements
Cox–de Boor basis evaluation, rational (weighted) curves and surfaces, periodic/closed
handling, and analytic surface normals (quotient rule) over plain arrays. Two thin arms
in `MeshBuilder.hpp` read the X3D fields and call it; `recognizedGeometryType()` and the
`buildLocalMesh()` dispatch change in lockstep (per that file's own comment).

**Why first-party, not a seam.** A seam earns its genericity ritual only when it clears
*two* bars: (1) it needs the outside world the SDK refuses to touch (files, http, fonts,
an image decoder, a GPU context, a physics solver), and (2) reasonable implementations
genuinely disagree (Jolt vs Bullet contact streams, FreeType vs stb metrics) so the
swap-test proves no backend's opinion leaked into the interface (ADR-0022). NURBS
evaluation clears **neither**. It is pure in-memory math on `controlPoint`/`knot`/
`weight` arrays (no I/O), and X3D §27 *prescribes* the result — Cox–de Boor basis,
knot-defaulting, the `tessellation` field's meaning. Two correct evaluators produce the
same mesh at the same sampling; a swap-test would be tautological. NURBS tessellation is
the same **species** as Box/Sphere/Cone/Cylinder/Extrusion tessellation — prescribed,
I/O-free, parametric — all already first-party in `MeshBuilder`. `NurbsPatchSurface` is
the general case of what `Box` is a trivial case of.

**The multi-backend escape hatch already exists and stays.** `MeshBuildOptions::
externalGeometryResolver` remains the override for embedders who *do* want a library
(GPU tessellation, OpenNURBS for double-precision CAD, proprietary geometry, trimming at
scale). Per the existing contract (ADR-0001 ext-firewall), the resolver is consulted
*only* for geometry the builder does not recognize — never for a recognized type. The
built-in arm owns `NurbsCurve`/`NurbsPatchSurface`; the resolver continues to serve the
still-unrecognized NURBS nodes (`NurbsTrimmedSurface`, `NurbsSweptSurface`,
`NurbsSwungSurface`). Making the resolver win over recognized geometry would be a
behavior change for *all* geometry (Box, Sphere, …) that nobody asked for.

**Supporting decisions:**

- **Bounds via the convex-hull property.** NURBS nodes have no `coord` field, so they
  fell through `localGeometryBounds`. A NURBS curve/surface lies within the AABB of its
  control points, so the bounds arm resolves `controlPoint` and unions its points —
  cheap, conservative, exact-enough for culling/picking.
- **Clamped-uniform default knots.** If the authored knot length ≠ `numCP + order`,
  generate a clamped uniform knot vector (endpoints interpolated) — the Xj3D / X3DJSAIL
  convention. Weights are honored only when length == `numCP`, else all-1. `order` must
  be ≥ 2 and ≤ `numCP`, otherwise the node is *recognized but legitimately empty* (the
  existing B2 semantics, distinct from an unrecognized type).
- **Full periodic `closed` handling.** `closed=true` with differing endpoints wraps the
  first `order−1` control points (and weights) and switches to a uniform unclamped
  periodic knot vector, giving a C^(order−2)-continuous seam — tangents/normals match
  automatically. `closed=true` with identical endpoints is an already-closed clamped
  loop (no wrap). Surfaces apply this independently per direction (`uClosed`/`vClosed`).
- **Analytic normals, no creaseAngle post-pass.** Surface normals come from the
  rational quotient rule `∂S/∂u = (A_u − w_u·S)/w`, `n = normalize(S_u × S_v)` —
  validated against a central-difference normal.
- **Implicit `(u,v)` texcoords.** Authored `NurbsTextureCoordinate` is deferred; the
  patch arm always emits normalized parametric uv for now.

## Consequences

**Positive:**

- `NurbsCurve` → `Topology::Lines`, `NurbsPatchSurface` → `Topology::Triangles` with
  analytic normals + implicit uv now extract out of the box; NRB-1 closes for curve +
  patch. The conformance view's `extractable_geometry` set (parsed from
  `recognizedGeometryType()`) flips both nodes to extractable automatically.
- The math unit is independently unit-testable (plain arrays, no X3D nodes): the
  rational-quadratic exact unit circle, Bézier reduction, planar/analytic-vs-finite-
  difference normals, and closed-curve/closed-cylinder seam continuity are all proven.
  All formulas were machine-verified against a computer-algebra system.
- The per-header isolation check (CI preset) compiles `NurbsEval.hpp` in isolation,
  catching a missing include.

**Trade-offs / costs:**

- Two tests that used `NurbsPatchSurface` as their canonical *unrecognized* specimen
  (`external_geom_seam_test.cpp`, `scene_extractor_audit_test.cpp`) had to move to a
  still-deferred NURBS node (`NurbsTrimmedSurface`).
- Double-precision CAD fidelity is not delivered: control points round to `SFVec3f`
  storage. A library (OpenNURBS) *would* be justified here and for trimmed surfaces
  (clipping to 2D contour loops is genuinely hard) — both explicitly deferred and
  reachable via the existing resolver seam.

**Deferred (follow-up cards):** `NurbsTrimmedSurface`, `NurbsSweptSurface`,
`NurbsSwungSurface` (tracked as NRB-3); the three NURBS interpolators (NRB-2); authored
`NurbsTextureCoordinate`; double-precision CAD fidelity; and the entire §14 2D-geometry
component (G2D-1).

## Related

- [Extraction Pipeline subsystem](../subsystems/extract.md) — `NurbsEval.hpp` + the two MeshBuilder arms
- [ADR-0001: Ext Firewall](0001-ext-firewall.md) — `externalGeometryResolver` is consulted only for unrecognized geometry
- [ADR-0015: Extraction pull per path](0015-extraction-pull-per-path.md) — the MeshBuilder/SceneExtractor design this extends
- [ADR-0022: Second-backend swap-test = genericity proof](0022-scriptengine-second-backend-swap-test.md) — the seam bar NURBS does not clear
- Primary implementation: `runtime/extract/NurbsEval.hpp`, `runtime/extract/MeshBuilder.hpp`, `runtime/scene/GeometryBounds.hpp`
- Tests: `runtime/extract/tests/nurbs_eval_test.cpp`, `runtime/extract/tests/mesh_builder_nurbs_test.cpp`
- Design record: `docs/superpowers/specs/2026-06-27-nurbs-curve-patch-design.md`
