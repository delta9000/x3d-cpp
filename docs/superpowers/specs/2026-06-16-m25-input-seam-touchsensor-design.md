# M2.5 Input Seam + TouchSensor — Design Spec

**Date:** 2026-06-16
**Status:** DESIGNED (awaiting user review → implementation plan)
**Milestone:** M2.5 (the input seam unblocks M2D-1 pointing-device sensors and M2D-3 navigation)
**Branch:** `modernize-x3d-spec` (trunk)

## 1. Goal

Add the **consumer→runtime input seam** — the channel by which a renderer feeds pointer
state into the runtime — and prove it end-to-end with the canonical pointing-device sensor,
**TouchSensor**. This is the thinnest vertical slice that exercises the full
pointer → pick → sensor-resolution → ROUTE-cascade chain. Drag sensors (PlaneSensor /
SphereSensor / CylinderSensor, the rest of M2D-1) and interactive navigation (M2D-3) are
**separate later increments** that build on this seam.

## 2. Scope

**In scope:**
- A `PointerState` input holder (current world ray, primary-button-down, pointer-present).
- A `PointingSensorSystem` that resolves and drives TouchSensor outputs at `tick(now)`.
- Extending `PickResult` with the root→hit-geometry `PathKey` (the only change to `PickSystem`).
- Adding a **geometric hit normal** AND a **hit texture coordinate** to the pick narrow-phase
  (for `hitNormal_changed` / `hitTexCoord_changed`).
- Driving the (already generated) **TouchSensor** node's outputs — **all five**: `isOver`,
  `isActive`, `touchTime`, `hitPoint_changed`, `hitNormal_changed`, `hitTexCoord_changed`.
  TouchSensor ships **fully spec-complete**.

**hitTexCoord coverage** (§20.4.4 "computed as appropriate for the associated shape"):
- **Mesh geometry** (IndexedFaceSet/IndexedTriangleSet/strips/fans/ElevationGrid/…):
  barycentric interpolation of the hit triangle's vertex texcoords — the mesh already carries
  them (authored or default TC1–TC4). The narrow-phase returns the barycentric weights.
- **Analytic primitives** (Sphere/Box/Cone/Cylinder): the §13 texture-coordinate-generation
  parameterization for that primitive, evaluated at the local hit point (the implementation
  workflow spec-verifies each primitive's formula against §13 before coding it).

**Out of scope (deferred, named — each with a real reason):**
- Drag sensors (PlaneSensor/SphereSensor/CylinderSensor) — M2D-1 remainder, next increment
  (the seam + grab model here is built so they slot in).
- Navigation (NavigationSystem, examine/walk/fly) — M2D-3, separate increment.
- Keyboard input — not needed by TouchSensor; the seam is shaped so a `KeyState` can be
  added later without rework, but no key API ships now (YAGNI).
- Multi-pointer / 3D-wand bearing models — single primary 2D-style pointer (a world ray).

## 3. Spec grounding (ISO/IEC 19775-1 §20, verified against the prose mirror)

Every behavioral rule below is cited; verified via `spec_rag` + the prose mirror during design.

- **Influence / which geometry a sensor watches** (§20.4.4, §20.2.1): a TouchSensor detects
  pointing at geometry that is a **descendant of its parent group**; the sensor is typically a
  *sibling* of that geometry. (Anchor is its own parent group — not relevant to this slice.)
- **Nearest geometry is the indicated one** (§20.2.3): "If the bearing intersects multiple
  sensors' geometries, only the sensor nearest to the pointer will be eligible for activation."
  → satisfied by `PickSystem::pickClosest` (closest hit).
- **Lowest enabled sensor in the hierarchy is activated** (§20.2.1): "the lowest enabled
  pointing device sensor in the hierarchy is activated. All other pointing device sensors above
  the lowest enabled pointing device sensor are ignored." → walk *up* the hit's path; first
  enabled TouchSensor wins.
- **isOver** (§20.4.4): TRUE when the bearing intersects the resolved sensor's geometry, FALSE
  when it leaves (or other geometry obstructs). Generated **only when the pointer moved and the
  over-state changed** — not when geometry animates under a still pointer.
- **hitPoint_changed / hitNormal_changed / hitTexCoord_changed** (§20.4.4): each pointer
  movement while `isOver` is TRUE. `hitPoint` is the 3D surface point **in the TouchSensor's
  coordinate system**; `hitNormal` is the surface normal at that point; `hitTexCoord` is the
  texture coordinate of the surface at the hit, "computed as appropriate for the associated
  shape" (mesh: barycentric interp; primitive: §13 parameterization).
- **isActive + grab** (§20.4.4): if `isOver`, activating the primary button → `isActive TRUE`;
  the sensor then **grabs all motion until release** (`isActive FALSE`); other pointing-device
  sensors generate no events during the grab. 2D device: `isActive` tracks the primary button.
- **touchTime** (§20.4.4): emitted on deactivation iff (1) was over the geometry when initially
  activated, (2) is currently over (`isOver` TRUE), (3) is being deactivated.
- **enabled** (§20.4.4, §20.2.1): a disabled sensor tracks nothing; enabling a sensor mid-drag
  does **not** activate it until after the current deactivation.

## 4. Architecture

```
Consumer (renderer)                          Runtime (header-only)
  builds world ray ─ setPointer(Ray) ───────▶ PointerState   (dumb holder)
  button up/down  ─ setPointerButton(bool) ─▶   · ray
  pointer in/out  ─ setPointerPresent(bool) ─▶   · buttonDown
                                                 · present
  tick(now) ───────────────────────────────────────┐
                                                    ▼
                       PointingSensorSystem.update(now, pickSystem, ctx)   ← new
                         if PointerState changed since last update:
                           1. pick = pickSystem.pickClosest(ray, bounds, …)
                           2. resolve lowest enabled TouchSensor on pick.path
                           3. diff vs last tick → seed events (isOver / hit* / isActive / touchTime)
                                                    ▼
                       X3DExecutionContext cascade (existing) → ROUTEs fire @ now
```

### 4.1 Components & boundaries

| Unit | File | Responsibility | Depends on |
|---|---|---|---|
| `PointerState` | `runtime/events/PointerState.hpp` (new) | Hold the latest input: world `Ray`, `buttonDown`, `present`, plus a monotonic `revision` bumped on any setter (so the System cheaply detects "changed since last tick"). Pure data + setters. | `Ray` |
| `PointingSensorSystem` | `runtime/events/PointingSensorSystem.hpp` (new) | All pointing logic: pick, sensor resolution, transition computation, event seeding, grab tracking. Stateful across ticks. TouchSensor-specific for this slice. | `PointerState`, `PickSystem`, `X3DExecutionContext`, TouchSensor reflection |
| `PickResult.path` | `runtime/.../PickSystem.hpp` (modify) | Add `PathKey path` (root→hit-geometry node chain), filled during the existing `pickNode` recursion. | `PathKey` |
| pick narrow-phase normal + texcoord | `runtime/.../PickSystem.hpp` (modify) | Narrow-phase returns a geometric **normal** and a **texture coordinate** alongside point+distance. Mesh: face normal + barycentric-interpolated texcoord from the hit triangle's vertices. Analytic (sphere/box/cone/cylinder): surface normal + §13-parameterized texcoord at the local hit. Stored in `PickResult.normal` (world space) and `PickResult.texCoord` (`SFVec2f`). | existing intersect math + §13 |
| TouchSensor outputs | generated (no change) | The System writes all five outputs via the existing reflection `emit`/handler path. **No codegen → golden byte-identical.** | reflection |

Boundary test: a reader can understand `PointerState` (a holder), `PointingSensorSystem`
(the logic), and `PickSystem` (geometry queries) independently; the System is the only place
that knows TouchSensor semantics.

## 5. Data flow & the per-tick algorithm

`PointingSensorSystem` holds cross-tick state: `lastRevision`, `overSensor*` (the sensor
currently `isOver`), `activeSensor*` + `pressWasOver` (grab bookkeeping).

On `update(now, pick, ctx)`:

1. **Skip if unchanged:** if `PointerState.revision == lastRevision` → return (matches the
   spec: no over events when the pointer hasn't moved). Update `lastRevision`.
2. **Grabbed?** If a sensor is active (grab in effect), route only to it: emit
   `hitPoint_changed`/`hitNormal_changed` while still over; on button-up, emit `isActive FALSE`,
   compute `touchTime` (if still over), clear the grab. No other sensor gets events (§20.4.4).
3. **Not grabbed:**
   a. If `!present` → resolved sensor = none.
   b. Else `pick = pickClosest(ray)`. If `pick.hit`, walk `pick.path` from the hit geometry
      upward; the first **enabled** TouchSensor encountered (its parent group is the deepest
      ancestor on the path) is the resolved sensor. Else none.
   c. **isOver transition:** if resolved ≠ `overSensor`: emit `isOver FALSE` to the old (if any),
      `isOver TRUE` to the new (if any); update `overSensor`.
   d. **Hit outputs:** if resolved sensor present, emit `hitPoint_changed` (world point →
      sensor frame, §5.1), `hitNormal_changed` (world normal → sensor frame, normalized), and
      `hitTexCoord_changed` (the pick's surface texcoord directly — a surface attribute, not
      spatial, so no frame transform; §5.1).
   e. **Activation:** on a button-down edge while `overSensor` present → emit `isActive TRUE`,
      set `activeSensor = overSensor`, `pressWasOver = true` (begins the grab).
4. **Drain:** the seeded events run in `ctx`'s existing single-timestamp cascade at `now`.

### 5.1 Coordinate system

`hitPoint`/`hitNormal` are emitted in the **TouchSensor's coordinate system** (§20.4.4) — the
frame the sensor node sits in, i.e. its parent group's accumulated world transform. Compute
`M = PickSystem::worldOf(sensorNode)` (the existing accessor; a sensor has no transform of its
own, so this is its parent-group frame). Then `hitPoint_local = inverse(M) · worldPoint`;
`hitNormal_local = normalize(inverseTranspose(M) · worldNormal)`. `hitTexCoord` is a **surface
attribute** (the texture coordinate of the shape's surface), not a spatial quantity — it is
**not** transformed by `M`; it is emitted as the narrow-phase computed it.

## 6. Public API (consumer-facing)

On the context (or a thin input facade it owns):
```cpp
void setPointer(const Ray& worldRay);   // current bearing in world space
void setPointerButton(bool down);        // primary button state
void setPointerPresent(bool present);    // pointer in/out of the view
```
Each bumps `PointerState.revision`. The consumer calls these between ticks, then `tick(now)`
as today. No projection/near/far/aspect ever crosses the seam — the renderer owns un-projection.

## 7. Testing strategy

C++ `check()`-harness test(s) under `runtime/events/tests/`, wired in the root `CMakeLists.txt`
(e.g. `x3d_pointing_sensor`). Build a small scene graph in code (a Transform → group with a
TouchSensor sibling + a Box geometry), wire a ROUTE from the sensor, set pointer state, tick,
assert via reflection / ROUTE side-effects:

1. **isOver enter/leave:** ray hitting the Box → `isOver TRUE`; ray off → `isOver FALSE`; no
   event when the ray is unchanged across ticks.
2. **Sensor resolution = lowest on path:** nested TouchSensors (outer group + inner group both
   with a sensor over the same Box) → the inner (lowest) one resolves; the outer gets nothing.
3. **Nearest geometry:** two Boxes at different depths under different sensors → the nearer
   geometry's sensor resolves.
4. **hitPoint/hitNormal in sensor frame:** Box under a translated/rotated Transform → assert the
   emitted local point/normal match the inverse-transform of the known world hit.
4b. **hitTexCoord:** (a) mesh — an IndexedFaceSet with known texCoords, hit at a known point →
   assert the barycentric-interpolated texcoord; (b) primitive — a Box/Sphere face hit at a
   known local point → assert the §13-parameterized texcoord. Texcoord is invariant under the
   sensor-frame transform (unlike point/normal).
5. **isActive + touchTime click:** button-down while over → `isActive TRUE`; button-up still
   over → `isActive FALSE` + `touchTime == now`. Button-up after leaving → `isActive FALSE`,
   **no** `touchTime`.
6. **Grab exclusivity:** during an active drag, moving onto a second sensor's geometry emits no
   events to the second sensor.
7. **enabled=FALSE:** disabled sensor produces no events; the next-lowest enabled sensor (or
   none) resolves instead.
8. **PickResult.path regression:** `pickClosest` returns the full root→hit chain.

Verification gates: golden byte-identical (no codegen), full ctest green, plus the new test.

## 8. Deferred follow-ups (to log in BACKLOG at implementation)

Each deferral has a real reason (per "don't defer without a good reason"):
- **PDS-2** Drag sensors (PlaneSensor/SphereSensor/CylinderSensor) on this seam + grab model.
  *Reason:* separate sensor TYPES with their own virtual-geometry tracking; the increment was
  explicitly scoped to "seam + TouchSensor" (user decision), and the grab model here is built to
  receive them next.
- **PDS-3** NavigationSystem (M2D-3) driven by the same seam + NavigationInfo. *Reason:* a
  distinct M2D-3 feature (camera manipulation), not part of pointing-device sensing.
- **PDS-4** Keyboard input (`KeyState`). *Reason:* YAGNI — nothing in this slice (or TouchSensor)
  consumes keys; the seam is shaped so a `KeyState` slots in later without rework.

(`hitTexCoord_changed` is **no longer deferred** — TouchSensor ships fully complete, §2.)

## 9. Resolved decisions

1. **Increment scope = seam + TouchSensor only.** RESOLVED. (Thinnest end-to-end slice.)
2. **Pointer input = consumer-supplied world ray.** RESOLVED. (Runtime stays projection-agnostic
   — matches the headless, renderer-agnostic SDK end goal.)
3. **Processing model = level/state-based, sampled at `tick(now)`.** RESOLVED. (Matches the
   existing single-timestamp cascade; transitions computed vs the previous tick.)
4. **Logic location = dedicated `PointingSensorSystem`.** RESOLVED. (One concern per file,
   matches the established System pattern; keeps `X3DExecutionContext` thin.)
5. **Hit outputs = all five (`isOver`/`isActive`/`touchTime`/`hitPoint`/`hitNormal`/
   `hitTexCoord`).** RESOLVED — TouchSensor ships fully spec-complete; mesh texcoord via
   barycentric interpolation, primitive texcoord via §13 parameterization.
6. **Sensor resolution = `pickClosest` (nearest geometry, §20.2.3) → lowest enabled sensor on
   the hit's path (§20.2.1).** RESOLVED, spec-verified.
