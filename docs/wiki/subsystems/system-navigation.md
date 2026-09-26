---
title: Navigation System
summary: EXAMINE, FLY, WALK, LOOKAT and NONE navigation modes, avatar collision (Collision node) and terrain following, plus head-pose integration for the CAVE consumer.
tags: [subsystem, navigation, examine, fly, walk, lookat, collision, head-pose]
updated: 2026-09-26
related:
  - ../architecture.md
  - ../subsystems/system-viewpointbind.md
  - ../subsystems/system-pointing.md
  - ../subsystems/execution-context.md
---

# Navigation System

## Purpose

The Navigation System drives the bound Viewpoint from the consumer-supplied input seam each tick. It reads the `PointerState` drag deltas and `KeyState` held keys together with the bound `NavigationInfo` and mutates the bound Viewpoint's *user offset* — never the authored `position`/`orientation` fields (ISO/IEC 19775-1:2023 §23.2.3). It implements the four collision-free modes: **EXAMINE** (orbit/turntable), **FLY** (free-flight with keyboard translation), **LOOKAT** (animated frame-and-zoom to a picked object), and **NONE** (inert). WALK, terrain-following, gravity, and collision are deferred (NAV-COLLISION backlog item).

The system also provides the `HeadPose` seam through which a CAVE consumer can inject a per-wall head-tracking pose that is composed onto the effective view after the navigation offset.

## Key files

| File | Role |
|---|---|
| `runtime/events/NavigationSystem.hpp` | The `System` subclass — the mode implementations, collision + terrain following, Collision-node events, cross-tick state, vector helpers, LOOKAT transition state machine |
| `runtime/scene/PickSystem.hpp` | `castCollidable` — the collision query (Collision gating, proxy, Switch/LOD selection), reusing the pick narrow phase |
| `runtime/events/HeadPose.hpp` | Pure-data struct: `position`, `orientation`, `revision`; set by the consumer for head-tracking |
| `runtime/events/ViewpointOffset.hpp` | Per-viewpoint user offset (`Mat4 local`); navigation accumulates into this rather than the authored fields |
| `runtime/events/tests/navigation_test.cpp` | Unit test (doctest case `navigation_test` in the `x3d_events_tests` target) covering the modes |
| `runtime/events/tests/collision_test.cpp` | Unit test (doctest case `collision_test`): FLY/WALK collision, Collision events, enabled gating, proxy, step height, line geometry |

## Interfaces and seams

### Exposed interface

`NavigationSystem` inherits `x3d::runtime::System` and is registered with `X3DExecutionContext::addSystem`:

```cpp
// Registration (application / scene startup):
ctx.addSystem(std::make_shared<NavigationSystem>());

// Opaque key codes — consumer maps its native key codes to these via ctx.setKey:
static constexpr int NavigationSystem::kKeyForward = 1; // UP arrow / W
static constexpr int NavigationSystem::kKeyBack    = 2; // DOWN arrow / S
static constexpr int NavigationSystem::kKeyLeft    = 3; // LEFT arrow / A
static constexpr int NavigationSystem::kKeyRight   = 4; // RIGHT arrow / D
```

`HeadPose` is a plain struct the consumer fills and hands to the execution context:

```cpp
struct HeadPose {
    SFVec3f    position{0, 0, 0};
    SFRotation orientation{0, 0, 1, 0};
    unsigned long revision = 0;
    void set(const SFVec3f &pos, const SFRotation &ori); // bumps revision
};
```

### Seam points

- **`X3DExecutionContext::boundViewpoint()`** — returns the currently bound `X3DViewpointNode`-derived node (Viewpoint / OrthoViewpoint / GeoViewpoint). Navigation reads its `position`, `orientation`, `centerOfRotation`, and `fieldOfView` via the reflection accessor `geombounds::getField<T>`, and writes the effective pose back through `ctx.setViewpointOffset`. It never casts to a concrete type.

- **`X3DExecutionContext::boundNavigationInfo()`** — returns the bound `NavigationInfo`. Navigation reads `getType()` to resolve the active mode (first recognized value wins, §23.4.4), `getSpeed()` for FLY translation scaling, and `getTransitionTime()`/`getTransitionType()` for LOOKAT animation. It calls `ctx.postEvent(nav, "transitionComplete", …)` at the end of a LOOKAT transition.

- **`X3DExecutionContext::pointerState()`** — `PointerState` struct: `present`, `buttonDown`, `ray` (world-space `Ray`, used for picking/LOOKAT), `screenX`/`screenY` (normalized cursor, x right / y up), and `revision`. Navigation forms its drag deltas from `screenX`/`screenY` — the camera-independent screen cursor the consumer feeds via `ctx.setPointerScreen(x, y)` — **not** from the ray. The ray is recomputed from the camera each frame, so using it would feed orbit back on itself (a single click spins the view) and scale rotation by the scene's near-plane size. See [Pointing System](../subsystems/system-pointing.md) for how the consumer populates this.

- **`X3DExecutionContext::keyState()`** — `KeyState` holding set via `ctx.setKey(kKey*, bool)`. FLY mode polls `ks.isHeld(kKeyForward)` etc. each tick. See [Key-Device Sensor System](../subsystems/system-keydevice.md).

- **`X3DExecutionContext::viewpointOffset(vp)` / `setViewpointOffset(vp, off)`** — reads and writes the `ViewpointOffset` (a `Mat4 local`) for a given viewpoint node. Navigation accumulates the orbit / fly / lookat delta here; it is composited as `worldOf(vp) · authoredPose · navOffset · headPose` by the execution context to form the final camera matrix.

- **`X3DExecutionContext::pick(ray)`** — used by LOOKAT on a button-down edge to find the picked node. Returns a `PickResult` with `hit`, `node`, and a world-space hit point.

- **`X3DExecutionContext::worldBounds(node)`** — returns the world-space `Aabb` of the picked node; LOOKAT uses the bbox center and half-diagonal to compute the framing standoff distance.

- **`X3DExecutionContext::writeField(vp, "centerOfRotation", …)`** — LOOKAT writes `centerOfRotation` on the Viewpoint node (as a field event, not a bypass) so that subsequent EXAMINE sessions orbit the newly framed pivot (§23.4.4).

- **Mode resolution rule** — `NavigationSystem::resolveMode(nav)` iterates `nav->getType()` and returns on the first recognized value (`NONE`, `EXAMINE`, `FLY`, `LOOKAT`). `ANY` and any unrecognized value map to `EXAMINE`. With no bound `NavigationInfo`, `EXAMINE` is the default.

### EXAMINE orbit algorithm

Drag deltas in normalized-screen units drive a yaw (about world +Y) and pitch (about the local right vector, clamped near the poles). The eye is rotated about `centerOfRotation`; `commitEye` recomputes the offset so the effective local eye becomes the new position/orientation pair. Angular sensitivity: dragging one full screen unit (the whole viewport width/height) maps to `π` radians (`kRotScale`) — scene-scale independent, since the screen cursor is normalized.

### FLY flight algorithm

FLY holds the orientation as yaw/pitch scalars (`flyYaw_`/`flyPitch_`) and reconstructs `q = Rpitch · Ryaw` each step — roll-free by construction (Ryaw is about world-up; Rpitch is about the yawed horizontal right). Drag accumulates the scalars (`-dx * kRotScale` yaw, `-dy * kRotScale` pitch); pitch is clamped to ±(π/2 − ε) to prevent gimbal flip. On mode switch or viewpoint bind change, the scalars are re-decomposed from the current effective orientation via `decomposeLookRotation` (forward direction → yaw/pitch). Held forward/back keys translate along the effective view direction, left/right keys strafe, all scaled by `NavigationInfo.speed * dt` (§23.4.4). Speed of zero locks position.

### Collision, WALK and terrain following (§23.4.2, §23.4.4)

FLY and WALK moves are collision-checked; EXAMINE orbits and LOOKAT transitions are not. `NavigationInfo.avatarSize` gives the collision radius, the eye height above the terrain and the step height (defaults 0.25, 1.6, 0.75); like `speed`, it is scaled by the bound Viewpoint's transform hierarchy.

- **Blocking.** `resolveMove` casts the move from the eye (and, in WALK, from step height — the body below the eye) with `X3DExecutionContext::collide`, and stops the avatar `radius` short of the first hit. Only the motion direction is probed: a wall parallel to the motion does not push the avatar away.
- **What collides.** `PickSystem::castCollidable` walks the scene with the Collision rules: all geometry collides except line and point geometry; a Collision with `enabled` FALSE removes its whole subtree, nested Collision nodes included; a Collision with a `proxy` collides only through the proxy; a Switch contributes its chosen child and an LOD its first level. Billboards collide unrotated.
- **Events.** When contact with geometry inside enabled Collision nodes begins, each enclosing group gets `isActive` TRUE and `collideTime`; a stationary avatar resting against the geometry stays in contact; `isActive` FALSE follows when the collision no longer occurs. Geometry outside any Collision node blocks but fires nothing.
- **WALK.** Motion stays in the viewpoint-local horizontal plane. Each tick a downward ray keeps the eye `avatarSize[1]` above the ground: rising ground (a step no taller than `avatarSize[2]`; taller ones were blocked) lifts the eye at once, and falling integrates gravity (9.81, scaled into the viewpoint frame) until the eye is back at height. With no ground below, the avatar hovers.

### LOOKAT transition

On button-down edge, the system picks the ray (`ctx.pick`), computes the target world position as `bbox_center - dir * (radius / tan(fov/2))`, converts to viewpoint-local frame, and starts a linear position/`slerp`-rotation animation from the current effective eye to the target. The transition runs over `NavigationInfo.transitionTime` seconds. `TELEPORT` transition type completes in a single tick. `transitionComplete` is posted to the bound `NavigationInfo` at the end.

## How it is tested

- `ctest --preset dev -R x3d_events_tests` (doctest case: `navigation_test`) — ten cases, all driven entirely in code (no file fixtures):
  1. **EXAMINE** — drag orbits the eye; asserts distance-to-pivot is preserved and camera aims at pivot; authored `position` field is unchanged (BIND-01 invariant).
  2. **FLY** — forward key translates by `speed * dt` along the view direction; drag rotates the effective view direction; `speed = 0` locks position; authored `position` field is unchanged.
  3. **LOOKAT** — click on a 2×2×2 Box at the origin from z=10; asserts pose moves during transition, ends at a framing distance in front of the box, `centerOfRotation` is set to the bbox center, and `transitionComplete` is `TRUE`.
  4. **NONE** — drag and key input produce no effective camera change.
  5. **ANY** — resolves to EXAMINE; eye orbits on drag.
  6. **LOOKAT under non-uniform ancestor scale** (NAV-LOOKAT-SCALE) — a Box under a Transform with scale (2, 0.5, 1); asserts `worldBounds` returns the scaled AABB and the camera ends at the scaled framing distance.
  7. **FLY roll-free: yaw-only** (NAV-FLY-ROLL) — 10 yaw drag steps; asserts the camera right vector stays horizontal (no roll accumulation).
  8. **FLY roll-free: combined yaw+pitch** — alternating yaw/pitch drags; asserts the right vector stays horizontal.
  9. **FLY pitch clamp** — extreme downward drag; asserts forward.y ≈ +1 (clamped at +π/2) and right stays horizontal.
  10. **FLY re-decompose on mode switch** — FLY → NONE → FLY; asserts orientation is preserved and drag continues from the current yaw.

- `ctest --preset dev -R x3d_events_tests` (doctest case: `collision_test`) — FLY into a wall inside a Collision group (stops `radius` short; `isActive`/`collideTime` once; resting contact; FALSE on backing away), `enabled` FALSE over a nested enabled Collision, `proxy` (invisible wall; proxy replaces children), WALK gravity / step climbing / tall-obstacle blocking, and a non-collidable IndexedLineSet.

No golden files: navigation output is numeric pose values asserted with floating-point tolerances.

## Related specs and ADRs

- `docs/superpowers/specs/2026-06-07-architecture-validation-and-resequencing.md` — M2D architecture decision placing NavigationSystem in the interaction layer (ECS-flavored, System pattern)
- ISO/IEC 19775-1:2023 §23.2.3 (navigation relative to viewpoint location), §23.3.1 (centerOfRotation + up-vector convention), §23.4.4 (NavigationInfo type list / mode semantics / LOOKAT / FLY / NONE), §23.4.6 (Viewpoint fields), Annex G.3 (informative arrow-key mapping)
- [Execution Context](../subsystems/execution-context.md) — provides all seams this system reads and writes
- [Viewpoint Bind System](../subsystems/system-viewpointbind.md) — manages the binding stacks for Viewpoint and NavigationInfo that this system reads
- [Pointing System](../subsystems/system-pointing.md) — populates `PointerState` from consumer input; LOOKAT depends on the ray for picking
- [Architecture](../architecture.md)
