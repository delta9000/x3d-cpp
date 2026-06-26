# M2D Interaction Layer — Drag Sensors + Navigation (on the input seam)

**Date:** 2026-06-16
**Status:** DESIGNED (next big push; queued behind the in-flight `m25-input-seam-touchsensor` workflow)
**Closes:** M2D-1 (pointing-device sensors — drag sensors remainder) + M2D-3 (interactive navigation, collision-free modes)
**Builds on:** the M2.5 input seam + `PointingSensorSystem` + grab model (sibling spec `2026-06-16-m25-input-seam-touchsensor-design.md`)
**Branch:** `modernize-x3d-spec`

## 1. Goal

With the input seam + TouchSensor landed, add the two remaining input-driven consumers as one
big push: the **three drag sensors** (PlaneSensor / SphereSensor / CylinderSensor) and an
**interactive NavigationSystem** (collision-free modes). After this, the seam has its full set
of consumers and M2D is effectively closed except the explicitly-deferred collision subsystem.

## 2. Scope

**Track 1 — Drag sensors (closes M2D-1):** PlaneSensor, SphereSensor, CylinderSensor. Each is a
drag sensor (`X3DDragSensorNode`): on the existing grab (isActive TRUE…FALSE), it maps pointer
motion onto its "virtual geometry" and emits `trackPoint_changed` + its `<value>_changed`
(`translation_changed` / `rotation_changed`), honoring `offset`/`autoOffset` and clamps.

**Track 2 — Navigation (closes M2D-3, collision-free):** a `NavigationSystem` that consumes the
input seam (pointer-drag + keys) and the bound `NavigationInfo` to mutate the bound Viewpoint:
- **EXAMINE** — orbit the view about the Viewpoint's `centerOfRotation` (turntable); drag rotates, wheel/keys dolly.
- **FLY** — free flight: translate along the view direction + yaw/pitch from drag; `speed`-scaled; a fixed "up". **No** terrain/gravity.
- **LOOKAT** — animate the Viewpoint to frame a picked object's world bbox over `transitionTime` using `transitionType`.
- **NONE** — navigation disabled. **ANY** — browser picks a sensible default (we map to EXAMINE).
- **KeyState** seam extension (PDS-4) — keyboard drives FLY/EXAMINE movement.

**Out of scope (deferred, each with a real reason):**
- **NAV-COLLISION — WALK + terrain-following/gravity + the collision gate on FLY/NONE.** *Reason:*
  §23.2.4/§23.3.2 require avatar-volume collision against scene geometry — a separate large
  subsystem (volume sweep, not ray-pick; the Collision node; step/gravity). Shipping WALK without
  it is non-conformant. FLY ships as free-flight; the deferral note states collision is the gate
  for WALK and for FLY/NONE in collision-requiring profiles.
- **EXPLORE / LOOKAT-on-MPEG / transitionType ANIMATE curve** — niche; LINEAR + TELEPORT cover the
  common cases. *Reason:* YAGNI; the bound modes above cover the corpus.
- **Multi-axis 3D-wand drag interpretation** — single 2D-style world-ray pointer (seam contract).

## 3. Spec grounding (ISO/IEC 19775-1, verified via spec_rag + prose mirror this session)

**Drag sensors — common (§20.2.2):** while active, emit `trackPoint_changed` (unclamped point on
the virtual geometry) and `<value>_changed`. `<value>_changed` = relative change since initial
activation **+ `offset`**. On deactivation, if `autoOffset` TRUE, `offset` ← last `<value>_changed`
and an `offset_changed` event is sent. The active sensor **grabs** all motion (already implemented).

**PlaneSensor (§20.4.2):** maps motion into 2D translation in a *tracking plane* parallel to the
Z=0 plane of the local sensor frame (frame = local coords with `axisRotation` applied), coincident
with the initial intersection. `translation_changed` = in-plane relative translation + `offset`,
clamped by `minPosition`/`maxPosition` (per-component: min>max ⇒ unclamped; min==max ⇒ locked →
gives a 1-D line sensor). `trackPoint_changed` = unclamped plane intersection.

**SphereSensor (§20.4.3):** virtual sphere centered at local origin, radius = |initial hit − origin|,
fixed at activation. `rotation_changed` (SFRotation) = relative rotation from the initial point +
`offset`. `trackPoint_changed` = point on the sphere surface.

**CylinderSensor (§20.4.1):** the initial acute angle between the bearing and the local Y-axis vs
`diskAngle` selects mode: **≥ diskAngle ⇒ cylinder** (radius = shortest distance from the hit to
the Y-axis; rotation about Y; zero marks the activation point), **< diskAngle ⇒ disk** (infinite
disk in the Y=0 plane; rotation in-plane). `rotation_changed` clamped by `minAngle`/`maxAngle`
(unclamped if min>max). `axisRotation` defines the local sensor frame; `offset`/`autoOffset` as common.

**NavigationInfo (§23.4.4):** `type` (ANY/WALK/EXAMINE/FLY/LOOKAT/NONE/EXPLORE…), `speed`,
`avatarSize` [collisionRadius, height, stepHeight], `headlight`, `transitionType`/`transitionTime`,
`visibilityLimit`. Bindable; re-parented to the current Viewpoint.
**Modes (§23.2.x):** WALK = terrain-following + gravity + collision; FLY = like WALK but
terrain/gravity may be disabled, "up" preserved; both "shall strictly support collision detection"
in collision-requiring profiles (§23.2.4) → the deferral basis. EXAMINE/LOOKAT need no collision.
**Viewpoint pose** mutated = `position` (SFVec3f) + `orientation` (SFRotation); EXAMINE orbits
about `centerOfRotation`. (The implementing workflow spec-verifies the exact per-mode math + the
Viewpoint field set before coding — like the §13 texcoord spec-check in the seam workflow.)

## 4. Architecture

Both tracks are new/extended **Systems** driven by `tick(now)` (the established pattern), reading
the input seam (`PointerState`, new `KeyState`) and emitting via the cascade / mutating bound nodes.

```
PointerState + KeyState  ──(set by consumer)──▶ X3DExecutionContext
        tick(now) ──┬─▶ PointingSensorSystem  (extended: dispatch by sensor type)
                    │      TouchSensor (done) | PlaneSensor | SphereSensor | CylinderSensor
                    │      → drag math (per-sensor helper) → emit trackPoint_/<value>_changed
                    └─▶ NavigationSystem      (new)
                           reads pointer-drag deltas + keys + bound NavigationInfo
                           → mutates bound Viewpoint position/orientation (EXAMINE/FLY/LOOKAT)
```

### 4.1 Components

| Unit | File | Responsibility |
|---|---|---|
| Drag math (Plane) | `runtime/events/drag/PlaneDrag.hpp` (new) | Pure fn: (activation hit, sensor frame, current ray, offset, min/max) → {trackPoint, translation}. |
| Drag math (Sphere) | `runtime/events/drag/SphereDrag.hpp` (new) | Pure fn: virtual-sphere rotation → {trackPoint, rotation}. |
| Drag math (Cylinder) | `runtime/events/drag/CylinderDrag.hpp` (new) | Pure fn: disk/cylinder decision + rotation → {trackPoint, rotation}, min/max clamp. |
| Drag dispatch | `runtime/events/PointingSensorSystem.hpp` (extend) | On grab, dispatch by resolved sensor's `nodeTypeName()` to the right drag math; emit outputs; handle `offset`/`autoOffset` on deactivation. TouchSensor path unchanged. |
| `KeyState` | `runtime/events/PointerState.hpp` (extend) or new `KeyState.hpp` | Held keys (a small set/bitset) + revision; context setters `setKey(code,down)`. |
| `NavigationSystem` | `runtime/events/NavigationSystem.hpp` (new) | `update(now,ctx)`: read seam deltas + bound NavigationInfo → mutate bound Viewpoint per mode. Cross-tick state: last pointer, drag anchor, active LOOKAT transition. |
| Context wiring | `runtime/events/X3DExecutionContext.hpp` (extend) | `setKey`; register NavigationSystem so tick drives it (member-System pattern, like pick_). |

Boundaries: each drag-math header is a pure, independently-testable function (no node/System
deps) — that's what lets them be built in parallel. The Systems are the only stateful glue.

## 5. Testing strategy

C++ `check()` tests under `runtime/events/tests/`, wired in the root `CMakeLists.txt`.
- **Drag math units** (`x3d_drag_math`): each pure fn with hand-computed expected geometry —
  PlaneSensor translation + min/max clamp + line-sensor (min==max); SphereSensor rotation about
  origin; CylinderSensor disk-vs-cylinder selection at the `diskAngle` boundary + min/max clamp.
- **Drag dispatch** (extend `x3d_pointing_sensor`): a PlaneSensor over a Box → drag emits
  `translation_changed` + `trackPoint_changed`; deactivation with `autoOffset` updates `offset`;
  grab exclusivity already covered.
- **Navigation** (`x3d_navigation`): EXAMINE drag orbits the bound Viewpoint about
  `centerOfRotation` (assert pose); FLY key/drag translates+rotates scaled by `speed`; LOOKAT
  animates toward a target bbox over `transitionTime` (assert end pose + `transitionComplete`);
  NONE is inert; bound-NavigationInfo `type` switches behavior.
- Gates: golden byte-identical (codegen-free), full ctest green.

## 6. Deferred follow-ups (BACKLOG, with reasons)
- **NAV-COLLISION** — WALK + terrain/gravity/collision + the FLY/NONE collision gate (§23.2.4/§23.3.2):
  separate volume-collision subsystem; ray-pick ≠ volume sweep; WALK is non-conformant without it.
- **NAV-EXTRA** — EXPLORE mode, transitionType ANIMATE curve: niche; LINEAR/TELEPORT suffice now.

## 7. Workflow design (the big push)

A single fan-out workflow, mirroring the seam workflow's gating (adversarial review per integration
unit, golden+ctest gate). Structure:
1. **Spec-check (parallel)** — 6 agents verify exact math/behavior: PlaneSensor, SphereSensor,
   CylinderSensor (§20.4.x + §20.2.2), and EXAMINE, FLY, LOOKAT + the Viewpoint pose field set (§23).
   Structured formulas feed the implementers.
2. **Implement — parallel where files are disjoint, sequential where shared:**
   - Parallel: the 3 pure drag-math headers + the `KeyState` seam extension (4 independent files).
   - Then sequential (shared `PointingSensorSystem.hpp` / context): drag dispatch + tests; then
     `NavigationSystem` + context wiring + tests.
   Each integration unit: implement → adversarial review (build+ctest+golden-untouched) → ≤2 fixes.
3. **Verify** — golden gate + full ctest + BACKLOG (M2D-1 CLOSED; M2D-3 collision-free CLOSED;
   NAV-COLLISION / NAV-EXTRA logged).

Script: `docs/superpowers/plans/2026-06-16-m2d-interaction-layer-workflow.js` (ready to launch
after the seam workflow lands; do not run concurrently — it extends the same `PointingSensorSystem`).

## 8. Resolved decisions (made without back-and-forth; flag if you disagree)
1. **Scope = drag sensors + collision-free navigation in one push.** (Both consume the seam; "big push".)
2. **WALK + collision deferred (NAV-COLLISION).** (Separate subsystem; non-conformant without it.)
3. **Drag math as pure per-sensor headers** + thin System dispatch. (Parallelizable + testable.)
4. **KeyState added now** (FLY needs keys; not a gratuitous deferral-breaker — a real dependency).
5. **ANY → EXAMINE default.** (Sensible browser default; Core profile requires ANY/FLY/EXAMINE/NONE.)
