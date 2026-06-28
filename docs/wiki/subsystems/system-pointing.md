---
title: Pointing-Device Sensor System
summary: TouchSensor and drag sensors (PlaneSensor, SphereSensor, CylinderSensor) — pointer hit-test, grab lifecycle, and drag math, wired through the M2.5 input seam.
tags: [subsystem, pointing, touchsensor, drag, plane-sensor, sphere-sensor, cylinder-sensor, events]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/sensors.md
  - ../subsystems/system-keydevice.md
  - ../subsystems/dirty-bounds-transform.md
  - ../subsystems/execution-context.md
  - ../subsystems/event-cascade.md
---

# Pointing-Device Sensor System

## Purpose

This subsystem drives the four X3D pointing-device sensors — `TouchSensor`, `PlaneSensor`, `SphereSensor`, and `CylinderSensor` — from a world-space pointer bearing supplied by the consumer (embedder) each tick.

It owns three concerns:

1. **Pick resolution.** Each tick it asks the scene's `PickSystem` for the closest geometry hit under the current pointer bearing, then walks the hit's root-to-geometry node path upward to find the lowest enabled pointing-device sensor sibling (ISO/IEC 19775-1 §20.2.1, §20.2.3).
2. **Grab lifecycle.** A button-down edge while over a resolved sensor starts a grab; the grabbed sensor exclusively owns all pointer motion until button-up (§20.2.1). During a grab no other sensor receives events.
3. **Per-sensor behavior dispatch.** Within the grab the system dispatches to the appropriate pure drag-math function (`planeDrag`, `sphereDrag`, `cylinderDrag`) or emits `TouchSensor` hit outputs. On deactivation it applies `autoOffset` for drag sensors (§20.2.2).

The pure drag geometry lives in three header-only functions under `runtime/events/drag/`; `PointingSensorSystem` itself holds no per-motion geometry — only the cross-tick activation state and the dispatch.

## Key files

| File / directory | Role |
|---|---|
| `runtime/events/PointingSensorSystem.hpp` | The stateful `System` subclass that runs each tick; owns grab bookkeeping and dispatch |
| `runtime/events/PointerState.hpp` | Plain-data snapshot of the pointer for one frame (`Ray`, `buttonDown`, `present`, monotonic `revision`) |
| `runtime/events/drag/PlaneDrag.hpp` | Pure PlaneSensor drag math: tracking-plane intersection + per-component clamp (§20.4.2) |
| `runtime/events/drag/SphereDrag.hpp` | Pure SphereSensor drag math: virtual-sphere intersection + relative rotation composed with offset (§20.4.3) |
| `runtime/events/drag/CylinderDrag.hpp` | Pure CylinderSensor drag math: disk/cylinder mode decision + Y-axis angle + clamp (§20.4.1) |
| `runtime/scene/PickSystem.hpp` | Ray pick over the scene graph (broad-phase AABB + narrow-phase analytic/mesh); produces `PickResult` with hit point, world normal, tex coord, and root-to-node `PathKey` |
| `runtime/events/X3DExecutionContext.hpp` | Context that owns `PointerState` and exposes `setPointer` / `setPointerButton` / `setPointerPresent` + `pick()` / `worldOf()` to the system |

## Interfaces and seams

### Consumer input seam (M2.5)

The embedder (renderer) drives pointer input between ticks via three methods on `X3DExecutionContext`:

```cpp
// All three bump PointerState::revision, which the system uses to skip
// unchanged ticks (§20.4.4 — isOver events fire only on motion).
ctx.setPointer(const Ray &worldRay);        // world-space bearing
ctx.setPointerButton(bool down);            // primary button state
ctx.setPointerPresent(bool present);        // pointer in active region
```

The `Ray` carries a world-space origin and direction. The consumer is responsible for constructing this from its viewport / window coordinates before calling `tick`.

### PointerState

`PointerState` is a plain struct owned by `X3DExecutionContext`. `PointingSensorSystem` reads it via `ctx.pointerState()` each tick and skips processing when `ps.revision == lastRevision_`.

> **No-sensor pick skip.** `ctx.pick(ps.ray)` is a whole-scene ray walk; a consumer
> that re-feeds the pointer every frame (the OpenGL PoC does) would otherwise run it
> on every tick. `attachInteractive` takes a one-time inventory pass (`attach` over
> every scene node) and, when it finds **zero** pointing-device sensors, the system
> skips the pick entirely — the pick could never resolve to a sensor. The skip is
> conservative: if the inventory was never taken (a test that registers the system
> directly via `addSystem`), the system picks exactly as before. Regression:
> `runtime/events/tests/pointing_sensor_skip_test.cpp`.

```cpp
struct PointerState {
    Ray  ray;
    bool buttonDown = false;
    bool present    = false;
    unsigned long revision = 0;   // monotonic; bumped by every setter
    void setRay(const Ray &);
    void setButtonDown(bool);
    void setPresent(bool);
};
```

### PickSystem seam

`PointingSensorSystem` does not traverse the scene graph itself. It calls `ctx.pick(ps.ray)` (which delegates to `PickSystem::pickClosest`) to get a `PickResult`:

```cpp
struct PickResult {
    bool            hit      = false;
    X3DNode        *node     = nullptr;   // geometry-bearing node
    SFVec3f         point    {0,0,0};     // world-space hit point
    float           distance = 0.0f;
    extract::PathKey path;                // root→hit-geometry chain
    SFVec3f         normal   {0,0,0};    // world-space surface normal
    SFVec2f         texCoord {0,0};      // surface tex coord (not spatially transformed)
};
```

The `path` field is the root-to-geometry `PathKey` that the resolution walk uses to locate sensor siblings (§20.2.1 "lowest enabled sensor").

`ctx.worldOf(node)` (delegating to `PickSystem::worldOf`) retrieves the accumulated world matrix for any node; the system uses it to transform hit point and normal into the sensor's local frame for `hitPoint_changed` / `hitNormal_changed` emission.

### System registration

`PointingSensorSystem` subclasses the `System` base (from `runtime/events/X3DSystem.hpp`). It is registered by the embedder after building the scene graph:

```cpp
ctx.buildSceneGraph(scene);
ctx.addSystem(std::make_shared<PointingSensorSystem>());
```

`attach` is a no-op — sensors are resolved live from the pick path each tick rather than pre-enrolled. No per-node registration is needed.

### Event emission

All field writes go through `ctx.postEvent(node, fieldName, value)`, which enqueues the write into the event cascade so that author `ROUTE`s from outputOnly fields fan out during the cascade drain at tick time. Fields written this way include `isOver`, `isActive`, `touchTime`, `hitPoint_changed`, `hitNormal_changed`, `hitTexCoord_changed`, `trackPoint_changed`, `translation_changed` (Plane), `rotation_changed` (Sphere/Cylinder), and `offset` (autoOffset on deactivation).

### Pure drag-math functions

Each drag header exposes one free function and a result struct; no node or system coupling:

```cpp
// PlaneDrag.hpp
PlaneDragResult planeDrag(const Mat4 &sensorFrame, const SFVec3f &p0Local,
                          const Ray &worldRay, const SFVec3f &offset,
                          const SFVec2f &minPosition, const SFVec2f &maxPosition);

// SphereDrag.hpp
SphereDragResult sphereDrag(const Mat4 &sensorFrame, const SFVec3f &p0Local,
                            const Ray &worldRay, const SFRotation &offset);

// CylinderDrag.hpp
CylinderDragResult cylinderDrag(const Mat4 &sensorFrame, const SFVec3f &p0Local,
                                const SFVec3f &bearingDirLocal, const Ray &worldRay,
                                float diskAngle, float offset,
                                float minAngle, float maxAngle);
```

All math runs in the sensor-local frame. The `sensorFrame` matrix is `worldOf(sensor) * Mat4::rotation(axisRotation)` for Plane and Cylinder sensors; for Sphere it is just `worldOf(sensor)` (SphereSensor has no `axisRotation`). The frame is frozen at activation and reused for the whole drag (§20.2.2).

Key implementation details:

- **PlaneSensor**: tracking plane is `z = p0Local.z` in the sensor frame; delta relative to the activation hit is clamped per-component (`min > max` → unclamped; `min == max` → line sensor locked to that value).
- **SphereSensor**: virtual sphere of radius `|p0Local|`; relative rotation from `p0_hat` to `p_hat` composed as `q_rel * q_offset` (offset is the base, drag on top).
- **CylinderSensor**: mode (disk vs cylinder) decided once at activation from the acute angle between the activation bearing direction and the sensor +Y axis versus `diskAngle`; disk mode uses the `y = 0` plane of the sensor frame (not the activation hit's Y — conformance finding DS-1); cylinder mode uses the infinite cylinder `x² + z² = r²`; angle clamped between `minAngle` and `maxAngle` when `minAngle <= maxAngle`.

On a degenerate intersection (bearing parallel to plane, missed sphere/cylinder) each function returns a `valid=false` result holding the last valid value, so the system can emit a continuous output stream (spec-allowed behavior).

### Sensor resolution algorithm

Resolution follows §20.2.1: the `resolve()` helper walks the `PickResult::path` from the hit geometry node upward, and at each ancestor inspects its direct children for the first enabled pointing-device sensor sibling. The deepest such ancestor's sensor is "lowest" and wins. Child traversal reuses `X3DNode::fields()` reflection (same SFNode/MFNode iteration as `PickSystem::forEachChild`).

A sensor is considered enabled via `X3DSensorNode::getEnabled()`. A disabled sensor is skipped; the walk continues upward to find the next candidate. If a grabbed sensor is disabled mid-drag, the system deactivates it immediately (emits `isActive FALSE`, drops `isOver`) and releases the grab without further drag output (conformance finding DS-2).

## How it is tested

Two dedicated ctest targets cover this subsystem:

- `ctest --preset dev -R x3d_events_tests` (doctest case: `pointing_sensor_test`) — integration tests for the full `PointingSensorSystem` over in-code scene graphs. Covers: `isOver` enter/leave and no-event-when-revision-unchanged (§20.4.4); lowest-sensor-wins on nested groups (§20.2.1); nearest-geometry selection (§20.2.3); `hitPoint_changed` and `hitNormal_changed` in the sensor local frame under translated and rotated `Transform` ancestors; `hitTexCoord_changed` for both analytic primitives (Box §13.3.1) and barycentric-interpolated mesh geometry; `isActive` + `touchTime` click semantics (down-over → up-over fires; up-after-leave does not); grab exclusivity (second sensor receives nothing during an active grab); `enabled=FALSE` causes the disabled sensor to be skipped and the next outer enabled sensor to resolve; `PlaneSensor` drag dispatch + `autoOffset` accumulation; `PlaneSensor` offset accumulation + per-component min/max clamp including the line-sensor (min==max) case; `enabled=FALSE` mid-drag (DS-2 deactivation).
  Source: `runtime/events/tests/pointing_sensor_test.cpp`

- `ctest --preset dev -R x3d_events_tests` (doctest case: `drag_math_test`) — unit tests for the three pure drag-math functions in isolation (no node, no context). Hand-computed expected geometry for: PlaneSensor unclamped translation, offset pass-through (including `offset.z`), both-axes clamp, line-sensor (Y locked), `axisRotation` reorientation; SphereSensor trackpoint on sphere surface, 90° rotation axis/angle, identity on no-motion, offset composition; CylinderSensor cylinder mode, disk mode, DS-1 disk-plane-at-Y=0 invariant, `theta0 == diskAngle` boundary (selects cylinder), min/max clamp, offset addition.
  Source: `runtime/events/tests/drag_math_test.cpp`

No golden files are used for this subsystem; correctness is verified analytically against spec-derived expected values.

## Related specs and ADRs

- [Architecture overview](../architecture.md)
- [Sensors overview](../subsystems/sensors.md)
- [Key-device sensor system](../subsystems/system-keydevice.md)
- [Dirty-bounds-transform](../subsystems/dirty-bounds-transform.md) — `BoundsSystem` feeds the broad-phase AABB that `PickSystem` uses before the narrow phase
- [Execution context](../subsystems/execution-context.md) — owns `PointerState`; exposes `setPointer*`, `pick()`, `worldOf()`, and `postEvent()`
- [Event cascade](../subsystems/event-cascade.md) — drains `postEvent` writes and fans out author ROUTEs each tick

Spec grounding (ISO/IEC 19775-1:2023):

- §20.2.1 — sensor activation, grab exclusivity, lowest-sensor resolution
- §20.2.2 — drag-sensor common protocol: tracking geometry, `autoOffset`, `trackPoint_changed`
- §20.2.3 — nearest geometry determines which sensor resolves on a pick
- §20.4.1 — CylinderSensor: `diskAngle`, disk/cylinder mode, angle output
- §20.4.2 — PlaneSensor: tracking plane, translation output, min/max clamp
- §20.4.3 — SphereSensor: virtual sphere, rotation output, offset composition
- §20.4.4 — TouchSensor: `isOver`, `isActive`, `touchTime`, hit outputs (point/normal/texCoord), no events on still pointer
- §13 — texture-coordinate parameterization for analytic primitives (Sphere §13.3.7, Box §13.3.1, Cone §13.3.2, Cylinder §13.3.3)

M2.5 input-seam design: `docs/superpowers/specs/2026-06-14-m25-extraction-poc-renderer-design.md`

M2D interaction-layer design (drag sensors + NavigationSystem): committed alongside `runtime/events/drag/` (see the deprecated, historical `docs/superpowers/BACKLOG.md` M2D-1 / M2D-3 entries).
