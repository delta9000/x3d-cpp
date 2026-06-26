# M2e — LOD / Visibility (view-dependent scene-graph runtime)

- **Date:** 2026-06-16
- **Branch:** `modernize-x3d-spec`
- **Status:** approved (pre-implementation)
- **Milestone:** M2e — the final scene-graph-runtime sub-milestone (after M2a dirty/world-transform, M2b bounds, M2c binding stacks, M2d picking/nav)
- **Depends on:** bound Viewpoint (M2c) for the viewer pose; world bounds (M2b); the per-path world accumulation idiom (M2C-1 resolution); the event cascade (M1) for sensor/LOD output events.

Every behavior below is grounded in the ISO/IEC 19775-1:2023 normative prose
(component pages mirrored at `$X3D_SPEC_PROSE/`,
searchable via `scripts/spec_rag.py`). Clause numbers are cited inline.

---

## 1. Context

M2e adds the **view-dependent** layer of the runtime: behavior that depends on
where the viewer is. Four concerns, all driven by the bound Viewpoint's world
pose each `tick()`:

1. **LOD** real distance-based level selection (§23.4.3) — today the extractor
   statically renders level-0; this computes the active level from viewer distance.
2. **Billboard** view-facing rotation (§23.4.1) — today a pass-through group;
   this closes the M2C-4 Billboard deferral.
3. **ProximitySensor / VisibilitySensor** (§22.4.1 / §22.4.3) — emit
   enter/exit/active (and position/orientation, for Proximity) as the viewer moves.
4. **Visibility culling** — `NavigationInfo.visibilityLimit` far cull (§23.4.4) and
   the X3D-4.0 `visible=FALSE` child-node skip.

`Switch` (§10, grouping) is already correctly handled by the extractor
(`whichChoice`) and is out of scope by being done.

## 2. Verified surface (what M2e builds on)

- `X3DExecutionContext::boundViewpoint()` / `viewMatrix()` — bound Viewpoint + world→camera matrix (M2c). Camera world position = inverse(viewMatrix) · origin.
- `worldBounds(node)` / `BoundsSystem` (M2b) — world-space AABB per node.
- `boundNavigationInfo()` — bound NavigationInfo for `visibilityLimit`.
- Per-path world accumulation in `SceneExtractor::walk` and `PickSystem::worldOfRec` (M2C-1) — the place view-dependent transforms (Billboard) and LOD selection plug in, re-accumulated per placement (never the static first-path `TransformSystem.world_` table).
- `X3DEventCascade` / `postEvent` — Systems emit output events through the cascade.

## 3. Approach

**One `ViewDependentSystem` + per-path integration in the extractor/pick walk.**
A single System, run each `tick()` from the bound Viewpoint, owns the
event-emitting view-dependent logic (LOD level tracking + `level_changed`, sensor
enter/exit). The *render-time selection* pieces (which LOD child, Billboard
rotation) plug into the per-path world accumulation the extractor and PickSystem
already perform — consistent with M2C-1 (world transform is per-path,
re-accumulated; never read from the static `world_` table). TransformSystem stays
camera-free.

*Rejected:* folding Billboard into TransformSystem (injects camera-dependence into
the clean static `world_` table, fights M2C-1's per-path model); a pure-pull design
with no events (drops spec'd outputs `level_changed` / `isActive` / `enterTime` /
`position_changed` — fails the conformance thesis).

## 4. Design

### 4.0 Camera pose
`ViewDependentSystem` reads the bound Viewpoint each tick and exposes the **viewer
world pose**: position, forward, up. Per §23.3.1 the default viewer looks down −Z
with +Y up; world pose = (Viewpoint world transform) · (position, orientation).
A shared `cameraWorldPosition()` helper feeds all four concerns.

### 4.1 LOD — distance-based level selection (§23.4.3)
- **Distance is computed in the LOD's LOCAL frame** (spec: "the distance is
  calculated from the viewer's location, transformed into the local coordinate
  system of the LOD node (including any scaling transformations), to the *center*
  point"). So: transform the camera world position by the inverse of the LOD's
  accumulated world transform, then `d = |cameraLocal − center|`.
- **Step function L(d)** (spec, verbatim): with `n` range values
  R₀<R₁<…<R₍ₙ₋₁₎ (monotonic, >0), `L(d)=L₀ if d<R₀; L₍ᵢ₊₁₎ if Rᵢ≤d<R₍ᵢ₊₁₎; L₍ₙ₋₁₎ if
  d≥R₍ₙ₋₁₎`. `children` are highest→lowest detail; N+1 children for N ranges.
  Empty `range` ⇒ browser-choice: we keep level-0 (highest detail) — documented.
- **Selection is render-only.** Spec §23.4.3: "All nodes under an LOD node continue
  to receive and send events regardless of which level is active." The extractor
  emits only the active level's subtree; the event cascade still attaches to and
  ticks ALL children (a TimeSensor in an inactive level keeps running). M2e never
  prunes the graph the cascade sees.
- **`level_changed` event.** `ViewDependentSystem` tracks the active level **per
  node** and `postEvent`s `level_changed = i` when it changes between ticks. (Per
  node, not per path: a USE'd LOD on two paths is the documented per-node-event /
  per-path-render split inherited from M2C-1; the event uses the primary-path
  distance.) The extractor does per-path selection for rendering.
- `forceTransitions`: we always transition exactly at range boundaries (no
  perf-driven level skipping), which satisfies the field's intent; effectively a
  no-op for us. Documented.

### 4.2 Billboard — view-facing rotation (§23.4.1, closes M2C-4 deferral)
Shared helper `billboardLocalMatrix(billboardWorldM, cameraWorldPos, axisOfRotation)`
returning a rotation applied during per-path world accumulation in BOTH the
extractor and `PickSystem::worldOfRec` (so a billboarded subtree picks correctly):
- Compute the **billboard→viewer** vector in the billboard's local frame (camera
  world pos transformed by inverse(billboardWorldM)).
- `axisOfRotation ≠ (0,0,0)`: rotate the local +Z into the plane defined by
  `axisOfRotation` and billboard→viewer, pivoting about `axisOfRotation` (spec steps 1–3).
- `axisOfRotation = (0,0,0)` (viewer-alignment): rotate +Z collinear with
  billboard→viewer toward the viewer, then rotate +Y parallel to the viewer's +Y
  (spec steps a–c).
- Axis-coincident-with-billboard→viewer ⇒ spec-"undefined"; return identity (or last
  rotation), guaranteed NaN-free.

### 4.3 ProximitySensor (§22.4.1) — `ViewDependentSystem`-owned System
- Box = `center` ± `size`/2 in the sensor's **local** frame. Transform the camera
  world position (and orientation) into sensor-local; test containment. `size` with
  any zero component ⇒ inactive (spec: zero-volume box cannot generate events).
- Emit `isActive` TRUE/FALSE on enter/exit edges; `enterTime`/`exitTime` on those
  same edges; `position_changed`/`orientation_changed` while inside (and at the
  enter/exit instants), in the sensor's coordinate system. Per-sensor `wasActive` +
  last-pose state.
- **Load-time initial events** (§22.2 + §22.4.1): if enabled and the viewer is
  already inside at scene build, emit `isActive TRUE` + `position_changed` +
  `orientation_changed` + `enterTime` (mirrors the M2c/RTC-9 load-time emit path).
- **`centerOfRotation_changed` is DEFERRED** (documented in BACKLOG): it fires only
  under `LOOKAT` navigation and depends on the viewer's center-of-rotation, a
  navigation concept owned by interactive nav (M2D-3, deferred to the M2.5 input seam).
- Timestamp = tick time; the spec's exact sub-frame intersection interpolation is a
  documented approximation. DEF/USE box-union is a documented approximation (single
  box per instance evaluated; overlapping-instance results are spec-undefined anyway).

### 4.4 VisibilitySensor (§22.4.3) — `ViewDependentSystem`-owned System
- Test the sensor's `center`+`size` box (local→world) against the **view volume**:
  - exact frustum if the consumer supplied one (seam §4.6);
  - else an aspect-free symmetric cone from the Viewpoint `fieldOfView` half-angle,
    clipped at the effective far distance (§4.5).
- Spec §22.4.3 explicitly permits liberal over-reporting ("X3D browsers may err
  liberally when *isActive* is TRUE … if *isActive* is FALSE, the box has absolutely
  no effect on the rendered view") and states occlusion is irrelevant — so the cone
  is a conformant conservative default, not a compromise.
- Emit `isActive` + `enterTime`/`exitTime` on edges; load-time initial events if
  visible at build (§22.4.3).

### 4.5 visibilityLimit culling (§23.4.4)
- **Effective far distance** = `Viewpoint.farDistance` if > 0 (§23.3.1), else
  `NavigationInfo.visibilityLimit` if > 0 (§23.4.4), else infinite (0 ⇒ infinite).
  Scaled by the bound Viewpoint's transform (uniform scale assumed; non-uniform is
  spec-undefined).
- The extractor tags each RenderItem whose world bounds lie entirely beyond the
  effective far distance from the viewer with `beyondVisibilityLimit = true` — a
  **hint** (spec: "may not be rendered"), not a drop. Culling stays the consumer's
  call.

### 4.6 `visible` field (X3D 4.0) and the frustum seam
- **`visible=FALSE`** on any X3DChildNode ⇒ the extractor skips that node and its
  subtree (not displayed). Cheap, squarely "visibility", surfaced by §23.4.x field
  blocks (`visible TRUE` on grouping nodes).
- **Frustum seam (optional):** `setViewVolume(...)` on the extractor/context lets the
  consumer supply aspect or a full frustum; absent ⇒ cone fallback. Matches the
  "projection is the consumer's choice" math-dependency policy.

### 4.7 Dirty integration
Sensors and LOD are re-evaluated every `tick()` (the viewer can move any frame;
cheap). No new dirty category. Billboard and LOD render-selection are recomputed in
the per-path walk that already runs at extraction time. `ViewDependentSystem` reads
the camera pose once per tick.

## 5. Scope out (documented deferrals → BACKLOG rows)
- `ProximitySensor.centerOfRotation_changed` (needs LOOKAT center-of-rotation; → M2D-3 / input seam).
- Pointing-device sensors (TouchSensor/PlaneSensor — pointer input; → M2D-1 / input seam).
- `TransformSensor` (object-tracked, not viewer-tracked — distinct, low corpus value; deferred).
- `Collision`/terrain following and interactive navigation (→ M2D-3).
- Per-layer view volumes (→ Layering component).
- Sub-frame `isActive` timestamp interpolation; DEF/USE sensor box-union (approximations, documented).

## 6. File plan (runtime-only — golden byte-identical)
- **New:** `runtime/scene/ViewDependentSystem.hpp` (camera pose, LOD level tracking + `level_changed`, Proximity/VisibilitySensor evaluation, `billboardLocalMatrix` + frustum/cone helpers). Tests `runtime/scene/tests/view_dependent_test.cpp`.
- **Modified:** `SceneExtractor.hpp` (LOD per-path local-frame selection, Billboard in world accumulation, `visible=FALSE` skip, `beyondVisibilityLimit` tag, `setViewVolume` seam); `PickSystem.hpp` (Billboard in `worldOfRec`); `X3DExecutionContext.hpp` (register the system, `cameraWorldPosition()`); `RenderItem.hpp` (`beyondVisibilityLimit` flag); root `CMakeLists.txt`.

## 7. Testing (TDD per feature)
- **LOD:** distance crossing a range boundary selects the right child and emits
  `level_changed=i`; local-frame distance correct under a scaling ancestor; empty
  range ⇒ level-0; inactive-level behaviors still tick (cascade unaffected).
- **Billboard:** both axis modes orient +Z toward the camera (verify the rotated
  axis vs the billboard→viewer direction); a picked billboarded subtree hits
  correctly; axis-coincident degenerate is NaN-free.
- **ProximitySensor:** enter/exit edges emit isActive+enterTime/exitTime;
  position/orientation_changed while inside in sensor coords; load-time inside ⇒
  initial events; zero-size ⇒ inert.
- **VisibilitySensor:** in/out via the cone fallback and via a supplied frustum;
  enter/exit edges; load-time visible ⇒ initial events.
- **visibility:** an item beyond the effective far gets `beyondVisibilityLimit`; a
  `visible=FALSE` subtree produces no RenderItems.
- **Gate:** full `mise run ci` — ctest green, golden byte-identical (runtime-only), pytest green.

## 8. Risks / watch-items
- LOD `level_changed` per-node vs per-path under USE — inherits the M2C-1 documented
  approximation; the extractor render-selection is per-path-correct.
- Billboard now makes a subtree's world transform camera-dependent: every camera
  move changes billboarded world transforms. Handled by per-path re-accumulation at
  extraction/pick (no stale static table); verify no consumer caches a billboard
  world transform across camera moves.
- Cone vs true frustum for VisibilitySensor: conformant by §22.4.3's liberal-TRUE
  allowance, but document the approximation so a consumer needing exactness supplies
  the frustum seam.
