# NURBS curve + patch surface tessellation — design

- **Date:** 2026-06-27
- **Card:** Core — NURBS / 2D-geometry component (NURBS slice only)
- **Conformance:** closes NRB-1 (curve + patch); see deferrals below
- **Branch:** `feat/nurbs-curve-patch`

## Goal

Make `NurbsCurve` and `NurbsPatchSurface` produce renderable geometry through the
existing extraction pipeline. Today both are generated data-holding stubs that the
`MeshBuilder` does not recognize, so extract silently drops them (NRB-1, currently
DEFERRED, blocked on "a NURBS tessellation subsystem"). This builds that subsystem
as a focused, first-party, I/O-free math unit and wires two thin arms into
`MeshBuilder`.

**In scope:** `NurbsCurve` (→ line topology), `NurbsPatchSurface` (→ triangle mesh
with analytic normals + implicit param texcoords), full `closed`/`uClosed`/`vClosed`
periodic handling, control-point convex-hull bounds.

**Deferred to follow-up cards:** `NurbsTrimmedSurface`, `NurbsSweptSurface`,
`NurbsSwungSurface`, NURBS interpolators, authored `NurbsTextureCoordinate`,
double-precision CAD fidelity, and the entire 2D-geometry component (G2D-1).

## Why first-party, not a seam (binding rationale → ADR)

A seam in this codebase earns its genericity ritual (frozen interface + 2 independent
backends + CI swap-test) only when it clears **two** bars:

1. **It needs the outside world** the SDK refuses to touch — files, http, fonts on
   disk, an image decoder, a GPU context, a physics solver.
2. **Reasonable implementations genuinely disagree** — Jolt vs Bullet contact
   streams, FreeType vs stb metrics, PROJ vs WGS84. The swap-test proves no single
   backend's opinion leaked into the interface.

NURBS evaluation clears **neither**. It is pure in-memory math on `controlPoint` /
`knot` / `weight` arrays (no I/O), and X3D §27 *prescribes* the result — Cox–de Boor
basis, knot-defaulting, the `tessellation` field's meaning. Two correct evaluators
produce the same mesh at the same sampling; a swap-test would be tautological.
Manufacturing a "second backend" by vendoring a NURBS library would invent a
disagreement the spec already settled.

NURBS tessellation is the same **species** as Box/Sphere/Cone/Cylinder/Extrusion
tessellation — prescribed, I/O-free, parametric — all of which already live
first-party in `MeshBuilder`. `NurbsPatchSurface` is the general case of what `Box`
is a trivial case of. Putting it anywhere else would split geometry tessellation
across two conceptual homes for no genericity payoff.

The multi-backend escape hatch **already exists and stays**: `MeshBuildOptions::
externalGeometryResolver` (`std::function<PackedMesh(const X3DNode*, AssetResolver)>`)
is the override for embedders who *do* want a library — GPU tessellation, OpenNURBS
for double-precision CAD, proprietary geometry, trimming at scale. First-party arm =
works out of the box; seam = embedder override. They coexist (see precedence below).

`runtime/extract/` is installed SDK surface (`CMakeLists.txt` installs `runtime/`
wholesale); it is merely decoupled from *core* (core never includes extract). So this
is the extract subsystem doing its documented job — "convert a live scene graph into
renderer-consumable descriptors without coupling the SDK to any graphics API" — not a
new SDK/runtime boundary crossing.

A library *would* be justified for trimmed surfaces (clipping to 2D contour loops is
genuinely hard) and double-precision CAD fidelity — both explicitly deferred.

## Architecture

### Unit 1 — `runtime/extract/NurbsEval.hpp` (node-free math)

Namespace `nurbs`. No dependency on X3D nodes or `MeshBuilder`; takes plain arrays so
the math is unit-testable in isolation.

```cpp
namespace nurbs {

struct CurveDef {
  std::vector<SFVec3f> cp;     // control points
  std::vector<double>  w;      // weights; empty ⇒ all 1.0
  std::vector<double>  knot;   // len cp.size()+order; empty/wrong ⇒ generated
  int  order;                  // = degree + 1  (min 2)
  bool closed;
};
// segments+1 evenly-parameterized samples across the VALID DOMAIN
// [knot[order-1], knot[numCP]] (0-indexed) — NOT the full knot range (a classic bug:
// sampling the full knot range traces only part of the curve). Machine-verified.
std::vector<SFVec3f> tessellateCurve(const CurveDef&, int segments);

struct SurfaceDef {
  std::vector<SFVec3f> cp;     // uDim*vDim, u-index varies fastest (spec-pinned)
  std::vector<double>  w;
  std::vector<double>  uKnot, vKnot;
  int  uDim, vDim, uOrder, vOrder;
  bool uClosed, vClosed;
};
struct SurfaceSample { SFVec3f p; SFVec3f n; SFVec2f uv; };
// (uSeg+1)*(vSeg+1) grid of point + analytic normal + implicit param-uv
std::vector<SurfaceSample> tessellateSurface(const SurfaceDef&, int uSeg, int vSeg);

} // namespace nurbs
```

Internal building blocks (detail namespace) — the "new math":
- `defaultKnots(n, order, periodic)` — clamped uniform (open) or uniform unclamped
  (periodic).
- `findSpan(n, order, u, knot)` — knot-span location.
- de Boor point evaluation in **homogeneous** coordinates (rational weighting).
- de Boor with **first derivative** for surfaces.

**Repeated-knot guard (verified pitfall):** the Cox–de Boor base case is
`N_{i,0}=1 iff t_i ≤ t < t_{i+1} AND t_i < t_{i+1}`, and the recursion must take any
term with a zero denominator (a zero-length span at a repeated/clamped knot) as `0`
(the `0/0 → 0` convention). Clamped and periodic knot vectors both produce repeated
knots, so this guard is mandatory, not optional.

**Analytic normals** (surfaces): evaluate numerator `A(u,v)` and weight denominator
`w(u,v)` with their partials; `S = A/w`; quotient rule
`∂S/∂u = (A_u − w_u·S) / w` (same for `v`); `n = normalize(S_u × S_v)`. Smooth
normals for free — no creaseAngle post-pass.

### Unit 2 — `MeshBuilder` arms

Two arms in `runtime/extract/MeshBuilder.hpp`, plus registration in
`recognizedGeometryType()` (kept in lockstep per the file's own comment).

- **`NurbsCurve`** → resolve `controlPoint` SFNode (`Coordinate`/`MFVec3f` or
  `CoordinateDouble`/`MFVec3d`), read `knot`/`weight`/`order`/`tessellation`/
  `closed`, call `tessellateCurve`, emit `Topology::Lines`. Unlit, no normals,
  `solid=false` — matches the existing B4 line/point convention.
- **`NurbsPatchSurface`** → resolve control net + `u*/v*` fields, call
  `tessellateSurface`, emit `Topology::Triangles` (two tris per grid cell),
  populated normals, implicit normalized `(u,v)` texcoords. Authored
  `NurbsTextureCoordinate` deferred — implicit uv always for now.

**Seam relationship (unrecognized-fallback, not override).** The codebase wires
`externalGeometryResolver` as the fallback for geometry the builder does *not*
recognize — it is never consulted for a recognized type. We keep that contract: the
built-in arm owns `NurbsCurve`/`NurbsPatchSurface`, and the resolver continues to
serve the still-unrecognized NURBS nodes (trimmed/swept/swung). Making the resolver
win over recognized geometry would be a behavior change for *all* geometry (Box,
Sphere, …) that nobody asked for — explicitly out of scope.

Consequence: two tests currently use `NurbsPatchSurface` as their canonical
*unrecognized* specimen and must move to a still-deferred NURBS node
(`NurbsTrimmedSurface`):
- `runtime/extract/tests/external_geom_seam_test.cpp` (sections 0 and 1).
- `runtime/extract/tests/scene_extractor_audit_test.cpp` (the `skippedGeometryCounts`
  assertion, lines ~82–98).

### Unit 3 — `GeometryBounds.hpp`

NURBS nodes have no `coord` field, so they currently fall through bounds → wrong
culling/picking. Fix via the **convex-hull property**: a NURBS curve/surface lies
within the AABB of its control points. Add arms that resolve the `controlPoint` child
and union its points — cheap, conservative, exact-enough.

## Spec semantics (X3D §27 — exact constants pinned during planning)

- **`tessellation` → segment count** (`numCP` = control points):
  - `t > 0` → `segments = t`
  - `t < 0` → `segments = |t| · numCP`
  - `t == 0` → `segments = 2 · numCP` (default)
  - Surfaces apply per-axis via `uTessellation` / `vTessellation`.
  - The exact off-by-one (numCP vs numCP−1) is verified against the spec / Xj3D in
    the implementation plan.
- **Knot defaulting:** if authored knot length ≠ `numCP + order`, generate a
  **clamped uniform** knot vector (endpoints interpolated) — the Xj3D / X3DJSAIL
  convention.
- **Weights:** honored only if length == `numCP`, else treated as all-1.
- **`order`:** must be ≥ 2 and ≤ `numCP`; otherwise the node is *recognized* but
  produces an empty mesh (the existing B2 "recognized-but-legitimately-empty"
  semantics, distinct from an unrecognized type).

### `closed` / `uClosed` / `vClosed` (full periodic handling)

- `closed = false` → clamped uniform knots, open curve.
- `closed = true` **and first ≠ last control point** → **periodic**: wrap the first
  `order−1` control points (and their weights) onto the end, generate a uniform
  unclamped periodic knot vector, evaluate over the periodic domain. Produces a
  C^(order−2)-continuous closed loop — seam tangents/normals match automatically.
  Authored knots for the base count are superseded.
- `closed = true` **and first == last control point** → already-closed clamped loop,
  no wrap (the spec's exact wording).
- Surfaces apply this independently per parametric direction; the periodic basis makes
  seam normals continuous for free.

## Error / degenerate handling

- Empty / null control points → empty mesh.
- `numCP < order` → recognized, empty mesh.
- Mismatched weight length → all-1.
- Wrong-length knot vector → generated (clamped or periodic per `closed`).
- NaN/inf control points are out of scope here (tracked separately under MEM-3); eval
  stays defensive but does not sanitize inputs.

## Testing (doctest, in `runtime/extract/tests/`, mostly dependency-free)

**Pure-math unit tests (`NurbsEval`):**
1. Degree-1 curve == polyline (samples lie exactly on control segments).
2. Bézier special case vs. direct de Casteljau.
3. **Rational quadratic circle** — the exact unit-circle construction (verified
   against Wikipedia / Piegl–Tiller): order 3, 9 control points
   `(1,0),(1,1),(0,1),(−1,1),(−1,0),(−1,−1),(0,−1),(1,−1),(1,0)`, weights
   `1, √2/2, 1, √2/2, 1, √2/2, 1, √2/2, 1` (corner weight = cos45° = √2/2), knot
   vector `{0,0,0, ½,½, 1,1, 1½,1½, 2,2,2}` (the `π/2`-spaced vector, normalized).
   Assert every sample satisfies `x²+y²=1` within ε. The showpiece: proves rational
   weighting, not just the polynomial part.
4. Partition-of-unity invariant (basis sums to 1).
5. Bilinear surface over a planar control grid → all normals == plane normal, points
   coplanar.
6. Analytic surface normal vs. central-difference normal (validates the quotient-rule
   derivative).
7. Closed curve (endpoints differ) → start sample == end sample *and* tangent
   continuity across the seam; plus the endpoints-already-identical path.
8. Closed surface (cylinder-like wrap) → seam vertices coincide, normals continuous.

**Integration tests (`MeshBuilder`):**
9. `NurbsCurve` node → `Topology::Lines`, vertex count == segments+1, recognized=true.
10. `NurbsPatchSurface` node → `Topology::Triangles`, count == `uSeg·vSeg·6`, normals
    present, recognized=true.
11. `recognizedGeometryType` flips for both types.
12. Unrecognized-fallback unchanged: specimen swapped to `NurbsTrimmedSurface` in
    `external_geom_seam_test.cpp` + `scene_extractor_audit_test.cpp`; both still pass
    (resolver fires for the still-unrecognized type; recognized NURBS never hits it).
13. Degenerate `numCP < order` → recognized=true, empty mesh.

## Docs (part of the diff)

- **New ADR** capturing the "first-party, not a seam" rationale + the deferral /
  limitation set.
- `docs/conformance/findings.yaml`: NRB-1 DEFERRED → resolved for curve + patch, with
  the deferrals listed; then `mise run conformance` (never hand-edit the generated
  view).
- `docs/wiki/subsystems/extract.md`: add `NurbsEval` to the key-files table + a NURBS
  section.
- `docs/sdk/v1-capabilities.md`: NURBS curve + patch capability claim.
- `mise run docs-drift` after, to catch anything missed.

## Build / CI

- `NurbsEval.hpp` under `runtime/extract/` is installed automatically (the directory
  install rule).
- Register new test files on the `x3d_extract_tests` target + `add_test` (mirror the
  existing `mesh_builder_t*` wiring).
- Run full `mise run ci` (tests + golden + conformance-gate + build + cli-gate
  regression). We are *adding*, not relocating, so the generated-artifact tooling risk
  is low — but run the whole pipeline anyway.

## Math provenance (verified 2026-06-27)

Core formulas were checked against standard references (Piegl–Tiller, *The NURBS Book*;
published B-spline references) and then **independently machine-verified in a
computer-algebra system** — a strict upgrade from authority lookup to executable ground
truth:

- **Exact 9-point unit circle** (the showpiece): the spec's control points + weights
  (`1, √2/2, …`) + knots, evaluated as a degree-2 rational B-spline and sampled across
  the valid domain → max `|x²+y²−1| = 2.2×10⁻¹⁶` (machine ε), tracing the full circle
  `(1,0)→(0,1)→(−1,0)→(0,−1)→(1,0)`. Cross-checked against an independent 7-point
  construction (dev `5.6×10⁻¹⁷`).
- **Valid parameter domain** = `[knot[order−1], knot[numCP]]` (0-indexed), NOT the full
  knot range — confirmed by where the reference evaluator traces the whole curve.
- **Bézier reduction**: a clamped B-spline with no interior knots equals the Bézier
  curve on the same control points to `0.` exactly.
- **Derivative quotient rule** `C' = (A' − w'C)/w`: symbolic residual `≡ 0` — the
  formula behind the analytic surface normals.
- **Partition of unity**: `Σ` basis functions `≡ 1` over the domain (symbolic).
- **Knot count = numCP + order** (`n + d + 1` knots for `n` control points, degree `d`).

## Out of scope (explicit)

Trimmed/swept/swung surfaces, NURBS interpolators, authored NURBS texture coordinates,
double-precision CAD fidelity, and all 2D-geometry nodes. Each is a follow-up card.
