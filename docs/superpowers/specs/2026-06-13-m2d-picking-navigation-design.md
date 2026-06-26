# M2d — Picking / Ray Intersection + Navigation Math

**Date:** 2026-06-13
**Branch:** `modernize-x3d-spec`
**Status:** approved (user: "workflow for m2d all in one" — autonomous design)
**Milestone:** M2d. Roadmap §3 Step 2; depends on M2a (transforms) + M2b (bounds),
both complete. Unblocked geometric layer; pure math, fully headless-testable.

## Context

M2d adds the **ray/picking engine** — the geometric substrate every X3D pointing-
device and pick sensor builds on — plus the **navigation-math** primitive (the camera
view matrix from the bound Viewpoint). Both are renderer-agnostic and provable without
a renderer. The pick engine: cast a world-space ray, broad-phase cull by M2b world
AABBs, narrow-phase against geometry in its local frame, return the closest hit. The
nav math: derive the world→camera (view) matrix from the M2c-bound Viewpoint.

**Runtime-only, golden byte-identical** (`7226b3a07e744b4aea1c3d9f34897384e5d8c085cb0c79743b8a23872c7183c0`).
No codegen — everything is math + a traversal over the existing node graph.

## Verified surface

- `Mat4` (M2a) has `identity/operator*/transformPoint/translation/scale/rotation`.
  M2d adds `transformDirection` (3×3 part, w=0) and a general `inverse()`.
- M2b `BoundsSystem::localBounds(node)`; `Aabb`; `TransformSystem::localMatrix` (public).
- Geometry: `coord`→`Coordinate.point` (`MFVec3f`=`vector<SFVec3f>`); `IndexedFaceSet.coordIndex`
  (`MFInt32`=`vector<int>`, faces `-1`-delimited); `IndexedTriangleSet.index` (triples);
  `TriangleSet` (implicit consecutive triples); primitives `Box`/`Sphere`/`Cone`/`Cylinder`.
- `Viewpoint.getPosition()` (SFVec3f), `X3DViewpointNode::getOrientation()` (SFRotation);
  M2c `boundViewpoint()` returns the active one.
- `X3DExecutionContext`: built scene-graph state (transforms/bounds/bindings); M2d adds a
  `PickSystem` + `pick()`/`viewMatrix()` pull API.

## Design

### 1. Math additions

`runtime/math/Mat4.hpp` (extend):
- `SFVec3f transformDirection(const SFVec3f &d) const` — rotate/scale only (omit the
  translation column): `{m0·x+m4·y+m8·z, m1·x+m5·y+m9·z, m2·x+m6·y+m10·z}`.
- `Mat4 inverse() const` — general 4×4 inverse (adjugate / cofactor expansion; returns
  identity if singular). Needed to map a world ray into a node's local frame and to
  invert the camera pose for the view matrix.

`runtime/math/Ray.hpp` (new): `struct Ray { SFVec3f origin, direction; SFVec3f pointAt(float t) const; }`.

`runtime/math/Intersect.hpp` (new) — all return `std::optional<float>` (the entry
parameter `t ≥ 0` along the ray), in whatever frame the inputs share:
- `rayAabb(const Ray&, const Aabb&)` — slab method; entry `t` (handles ray origin inside).
- `raySphere(const Ray&, float radius)` — sphere at local origin; nearest positive root.
- `rayTriangle(const Ray&, v0, v1, v2)` — Möller–Trumbore; `t` of the hit (front+back).

### 2. `PickSystem` (`runtime/scene/PickSystem.hpp`)

Context-owned. Holds the scene roots (captured at build). A pick is an on-demand
traversal that **accumulates the world transform on the way down** (it does not need a
per-node world table — it composes `TransformSystem::localMatrix` of each Transform as
it descends, which also makes a Shape's world frame available where M2a stores only
per-Transform worlds).

- `void build(const Scene&)` — store root node pointers.
- `PickResult pickClosest(const Ray& worldRay, const BoundsSystem& bounds) const`:
  DFS from roots with an accumulated `worldM` (× each Transform's `localMatrix`); at each
  **Shape** (a node with a `geometry` field):
  1. **Broad phase:** `worldAabb = bounds.localBounds(shape).transformed(worldM)`; skip if
     `!rayAabb(worldRay, worldAabb)`.
  2. **Narrow phase in local frame:** `inv = worldM.inverse()`,
     `localRay = { inv.transformPoint(origin), inv.transformDirection(direction) }`;
     `narrowPhase(geometry, localRay)` → optional local `t`/point.
  3. On hit: `worldPoint = worldM.transformPoint(localPoint)`,
     `distance = |worldPoint − worldRay.origin|`; keep the minimum-distance hit.
  Returns `PickResult { bool hit; X3DNode* node; SFVec3f point; float distance; }`.
- `narrowPhase(geom, localRay)`:
  - `Sphere` → `raySphere(localRay, radius)`.
  - triangle mesh (`IndexedFaceSet` via `coordIndex` fan-triangulation; `IndexedTriangleSet`
    via `index` triples; `TriangleSet` implicit triples) → closest `rayTriangle` over the
    extracted triangles (points from `coord`→`Coordinate.point`).
  - else (Box/Cone/Cylinder/long-tail) → `rayAabb(localRay, localGeometryBounds(geom))`
    — a **local-AABB proxy** (exact for Box; conservative otherwise; backlog M2D-2).
- `Mat4 worldOf(X3DNode* target) const` — DFS from roots accumulating Transform local
  matrices, returns the accumulated world matrix at `target` (identity if not found).
  Used by `viewMatrix` for a (possibly nested) viewpoint.

### 3. `X3DExecutionContext` integration + pull API

- Own a `PickSystem pick_`.
- `buildSceneGraph(scene)` also calls `pick_.build(scene)`.
- Pull API:
  - `PickResult pick(const Ray& worldRay) const` → `pick_.pickClosest(worldRay, bounds_)`.
  - `Mat4 viewMatrix() const` — the world→camera matrix from the bound Viewpoint:
    `vp = boundViewpoint()`; if null → identity. Camera world pose =
    `pick_.worldOf(vp) * Mat4::translation(position) * Mat4::rotation(orientation)`;
    `viewMatrix = cameraWorld.inverse()`. (Reads `position`/`orientation` via reflection
    so it stays node-agnostic; falls back gracefully if absent.)

### Scope boundaries

- **In:** Mat4 `transformDirection`+`inverse`; Ray; ray-AABB/sphere/triangle; PickSystem
  (broad+narrow phase, triangle extraction for IFS/ITS/TriangleSet, AABB proxy otherwise,
  `worldOf`); context `pick()` + `viewMatrix()`; tests.
- **Out / backlog:**
  - **M2D-1** — X3D PickSensor nodes (LinePickSensor/PointPickSensor/PrimitivePickSensor/
    VolumePickSensor) and pointing-device sensors (TouchSensor…): the pick *engine* is here;
    the *sensor nodes* that consume it (Picking component) and the pointing-device sensors
    (need the M2.5 input seam) are deferred.
  - **M2D-2** — exact narrow-phase for Cone/Cylinder + long-tail geometry and triangle
    strip/fan extraction (AABB proxy + the three indexed/implicit triangle forms for now).
  - **M2D-3** — navigation *interaction* modes (examine/walk/fly driven by input): the view-
    matrix math is here; interactive navigation needs the M2.5 input seam.
- Inherits M2a/M2b's per-node identity (M2C-1) — picking traverses paths, so a USE'd node is
  tested under each path it appears (picking is actually path-correct via the traversal); the
  *bounds/transform table* approximation is unchanged.

## File structure

| File | Responsibility |
|------|----------------|
| `runtime/math/Mat4.hpp` (modify) | add `transformDirection` + `inverse` |
| `runtime/math/Ray.hpp` (new) | Ray + pointAt |
| `runtime/math/Intersect.hpp` (new) | rayAabb / raySphere / rayTriangle |
| `runtime/math/tests/intersect_test.cpp` (new) | the three intersections + inverse/transformDirection |
| `runtime/scene/PickSystem.hpp` (new) | traversal, broad+narrow phase, triangle extraction, worldOf |
| `runtime/scene/tests/pick_system_test.cpp` (new) | pick a Box/Sphere/IFS, closest-of-two, miss |
| `runtime/events/X3DExecutionContext.hpp` (modify) | own PickSystem; build hook; pick()/viewMatrix() |
| `runtime/events/tests/m2d_tick_test.cpp` (new) | end-to-end pick + viewMatrix via the context |
| `CMakeLists.txt` (modify) | register 3 new test executables |

## Testing

1. **Math:** `rayAabb` hit/miss + origin-inside; `raySphere` hit distance for a unit sphere
   from +Z; `rayTriangle` Möller–Trumbore hit + edge miss; `Mat4::inverse()` of a
   translate∘rotate∘scale gives `M·M⁻¹ ≈ I`; `transformDirection` ignores translation.
2. **PickSystem:** a `Transform(+5x) > Shape > Box(2)`; a ray from `(5,0,10)` toward `−Z`
   hits the box front face at world z≈1, distance≈9; a ray missing returns `hit=false`.
   A `Sphere` under a transform: analytic hit. An `IndexedFaceSet` quad: ray hits a triangle.
   Two shapes along the ray → the nearer one is returned.
3. **m2d_tick (end-to-end):** `buildSceneGraph`; `ctx.pick(ray)` returns the expected node +
   world point; `ctx.viewMatrix()` for a Viewpoint at `(0,0,10)` maps world `(0,0,0)` to camera
   `(0,0,−10)` (view = inverse of the camera pose).

## Verification

- `mise run build` + `uv run pytest` green; **golden byte-identical**; M2a/M2b/M2c/event
  tests unaffected (M2d only adds math + a build hook + pull API; no tick change).

## Risks / watch-items

- **General 4×4 inverse correctness:** pin it with a `M·inverse() ≈ I` test over a non-trivial
  TRS matrix; singular → identity (documented).
- **Ray frame discipline:** broad phase is in WORLD space (worldRay vs world AABB); narrow
  phase is in LOCAL space (ray transformed by `worldM.inverse()`). Keep them separate; the
  distance is always measured in world space from `worldRay.origin`.
- **Degenerate geometry:** empty coord / zero-area triangle → no hit (no NaN). `raySphere` of
  radius 0 → no hit. Guard divisions.
- **Traversal cost:** pick is O(nodes) per call (no spatial index in M2d) — fine for the
  foundation; a BVH is a future optimization (not a correctness gap).
