---
title: Pointing-device sensors
summary: TouchSensor and drag sensors (PlaneSensor, SphereSensor, CylinderSensor) — pointer hit-test, grab lifecycle, and drag math, wired through the pointer input seam.
tags: [subsystem, pointing, touchsensor, drag, plane-sensor, sphere-sensor, cylinder-sensor, events]
updated: 2026-09-26
related:
  - ../../explanation/architecture.md
  - sensors.md
  - system-keydevice.md
  - dirty-bounds-transform.md
  - execution-context.md
  - event-cascade.md
---

# Pointing-device sensors

## Purpose

This subsystem drives the four X3D pointing-device sensors — `TouchSensor`, `PlaneSensor`, `SphereSensor`, and `CylinderSensor` — from a world-space pointer bearing supplied by the consumer (embedder) each tick.

## Behavior

It owns three concerns:

1. Pick resolution. Each pointer update asks `PickSystem` for the closest hit on an active `Switch`/`LOD` path, then finds every enabled sensor tied at the lowest group on that path (§20.2.1, §20.2.3).
2. Grab lifecycle. A button-down edge activates all tied lowest sensors, each with its own drag state; together they own pointer motion until button-up (§20.2.1). Other sensors receive no events during the grab.
3. Per-sensor behavior dispatch. Within the grab the system dispatches to the appropriate pure drag-math function (`planeDrag`, `sphereDrag`, `cylinderDrag`) or emits `TouchSensor` hit outputs. On deactivation it applies `autoOffset` for drag sensors (§20.2.2).

The pure drag geometry lives in three header-only functions under `runtime/events/drag/`; `PointingSensorSystem` itself holds no per-motion geometry — only the cross-tick activation state and the dispatch.

## Key files

| File / directory | Role |
|---|---|
| `runtime/events/PointingSensorSystem.hpp` | The stateful `System` subclass that runs each tick; owns grab bookkeeping and dispatch |
| `runtime/events/PointerState.hpp` | Plain-data snapshot of the pointer for one frame (`Ray`, `buttonDown`, `present`, monotonic `revision()`) |
| `runtime/events/drag/PlaneDrag.hpp` | Pure PlaneSensor drag math: tracking-plane intersection + per-component clamp (§20.4.2) |
| `runtime/events/drag/SphereDrag.hpp` | Pure SphereSensor drag math: virtual-sphere intersection + relative rotation composed with offset (§20.4.3) |
| `runtime/events/drag/CylinderDrag.hpp` | Pure CylinderSensor drag math: disk/cylinder mode decision + Y-axis angle + clamp (§20.4.1) |
| `runtime/scene/PickSystem.hpp` | Ray pick over cached geometry placements; checks live `Switch`/`LOD` selection and `Layer.pickable` on each path, then broad/narrow phase; produces hit point, normal, tex coord, and `PathKey` |
| `runtime/events/X3DExecutionContext.hpp` | Context that owns `PointerState` and exposes `setPointer` / `setPointerButton` / `setPointerPresent` + `pick()` / `worldOf()` to the system |

## Interfaces and seams

### Consumer input seam

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

> No-sensor pick skip. `ctx.pick(ps.ray)` is a whole-scene pick; a consumer
> that re-feeds the pointer every frame (the OpenGL PoC does) would otherwise run it
> on every tick. The pick is index-backed (see *Lazy pick index* below), so on an
> unchanged scene it costs only the broad-phase over candidate leaves rather than a
> full graph walk. `attachInteractive` takes a one-time inventory pass (`attach` over
> every scene node) and, when it finds zero pointing-device sensors, the system
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

### Host-owned navigation

Set `SessionOptions::pointingSensors = true` to install pointing-device sensors
without SDK navigation or Anchor activation. `interactive = true` keeps its
existing meaning: sensors, anchors, and navigation in arbitration order. Setting
both flags installs the sensors once. Low-level hosts can call
`attachPointingSensors(scene, ctx)` after building the scene graph.

Before claiming a trigger, use the same resolver that drives sensor activation:

```cpp
sdk::SessionOptions options;
options.pointingSensors = true;
auto session = sdk::RuntimeSession::create(std::move(document), options);
auto hit = session->context().pick(worldRay);
auto sensors = session->pointingSensors(hit);
if (!sensors.empty()) {
    // Claim the trigger, then feed the pointer ray and button through context().
}
```

- The query returns every enabled sensor tied at the deepest applicable group,
  including `GeoTouchSensor`, with duplicates removed in child traversal
  order.
- A miss or geometry with no applicable sensor returns an empty vector.
- The query reads current enablement and works even when neither input system
  is installed.
- It doesn't emit events, change pointer state, or acquire a grab.
- It describes a fresh press; an existing grab retains its own targets.
- Changes between query and tick can change activation.
- Anchors aren't pointing sensors.

Use a current pick from this session. Picks and returned node pointers borrow
scene storage; discard them before removing their nodes or destroying the
session. Re-pick after scene changes that affect the hit or path. See
[ADR-0064](../../explanation/decisions/0064-pointing-without-navigation.md).

The focused cases in `runtime/extract/tests/pointing_without_navigation_test.cpp`
are registered in `x3d_extract_tests`. They cover installation, opt-in defaults,
resolution, activation agreement, and absence of SDK navigation. They haven't
been run for this proposal.

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

The private narrow phase returns an optional hit with distance, normal, and
texture coordinates. A miss returns an empty optional. The AABB fallback reuses
the geometry bound for both intersection and the proxy normal.

### Lazy pick index

`pickClosest` no longer walks the whole scene graph per pick. `PickSystem` owns an index of geometry-bearing placements — one per PATH, so a DEF/USE node keeps a distinct entry and world AABB for each placement — and tests them with the same `rayAabb` broad phase and existing `narrowPhase`. The index is built lazily on the first pick and thereafter:

- re-enumerated only when `TransformSystem::revision()` changes (a structural re-index or a transform re-accumulation) or the caller passes a different `maxVisits` budget;
- refit only when `BoundsSystem::revision()` changes (a geometry/bounds edit with no transform change);
- Billboard-affected placements are view-dependent and therefore never cached: they live in a separate list whose world frame is re-resolved from the stored path on every pick, so a new camera pose produces a new result.
- Switch and LOD selection is live: each retained path is checked against `whichChoice` and the LOD level selected from the camera in that LOD's local frame. Selection changes take effect without rebuilding the placement index (ADR-0034).

- Passing the live `TransformSystem*` (as `X3DExecutionContext::pick` does)
  enables the cache; a call with `nullptr` rebuilds each time (the old walk's
  result, no cache), so the direct-call API stays correct with no lifetime
  coupling.
- The `WalkBudget`/`budgetExceeded` semantics and the depth/cycle guards are
  preserved: the enumeration spends one visit per node and stops on the same
  doubling-DAG fan-out, returning the same partial best-so-far.
- Placement order follows the DFS order, so exact-distance ties break exactly
  as before.

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

- PlaneSensor: tracking plane is `z = p0Local.z` in the sensor frame; delta relative to the activation hit is clamped per-component (`min > max` → unclamped; `min == max` → line sensor locked to that value). `translation_changed` and `trackPoint_changed` are expressed in the local sensor coordinate system that `axisRotation` creates (§20.4.2), so with a 90° `axisRotation` about Z a vertical drag reports translation along sensor X. That is our reading of the published text, which never names the output frame explicitly.
- SphereSensor: virtual sphere of radius `|p0Local|`; relative rotation from `p0_hat` to `p_hat` composed as `q_rel * q_offset` (offset is the base, drag on top).
- CylinderSensor: mode (disk vs cylinder) decided once at activation from the acute angle between the activation bearing direction and the sensor +Y axis versus `diskAngle`; disk mode uses the `y = 0` plane of the sensor frame (not the activation hit's Y); cylinder mode uses the infinite cylinder `x² + z² = r²`; the angle is the right-handed rotation about +Y from the activation point to the current one (a drag from +Z toward +X is positive); angle clamped between `minAngle` and `maxAngle` when `minAngle <= maxAngle`.

On a degenerate intersection (bearing parallel to plane, missed sphere/cylinder) each function returns a `valid=false` result holding the last valid value, so the system can emit a continuous output stream (spec-allowed behavior).

### Sensor resolution algorithm

Resolution follows §20.2.1: `resolve()` walks the hit path upward and returns all enabled sensor siblings at the deepest group that has any. These tied sensors receive independent hover, activation, drag, and release events.

A sensor is considered enabled via `X3DSensorNode::getEnabled()`. A disabled sensor is skipped; the walk continues upward to find the next candidates. A grabbed sensor disabled mid-drag deactivates on the next tick even without pointer motion; other tied sensors continue their grab.

## Anchor activation (§9.4.1)

- `runtime/events/AnchorSystem.hpp` makes an Anchor's children
  pointer-sensitive.
- `attachInteractive` registers it after `PointingSensorSystem` and before
  navigation.
- A click (press and release over the same Anchor's geometry, the innermost
  Anchor on the pick path) activates it, unless a pointing-device sensor
  grabbed the pointer that tick; while the press is held the Anchor owns the
  pointer, so navigation does not also drag or LOOKAT.
- The url list is tried in order: `"#Name"` binds the viewpoint DEF'd `Name`
  (`set_bind` TRUE); any other url is passed with the `parameter` list to the
  embedder's `AnchorSystem::setAnchorHandler` callback, which loads a
  replacement world or opens a window (cases b/c).
- A `RuntimeSession` host installs it through `SessionOptions::anchorHandler`;
  `attachInteractive` takes the same handler as an optional argument.
- With no handler, a non-fragment url does nothing: the runtime is headless.

## Related

- [Architecture overview](../../explanation/architecture.md)
- [Sensors overview](sensors.md)
- [Key-device sensors](system-keydevice.md)
- [Dirty-bounds-transform](dirty-bounds-transform.md)
- [Execution context](execution-context.md)
- [Event cascade](event-cascade.md)

- §20.2.1 — sensor activation, grab exclusivity, lowest-sensor resolution
- §20.2.2 — drag-sensor common protocol: tracking geometry, `autoOffset`, `trackPoint_changed`
- §20.2.3 — nearest geometry determines which sensor resolves on a pick
- §20.4.1 — CylinderSensor: `diskAngle`, disk/cylinder mode, angle output
- §20.4.2 — PlaneSensor: tracking plane, translation output, min/max clamp
- §20.4.3 — SphereSensor: virtual sphere, rotation output, offset composition
- §20.4.4 — TouchSensor: `isOver`, `isActive`, `touchTime`, hit outputs (point/normal/texCoord), no events on still pointer
- §13 — texture-coordinate parameterization for analytic primitives (Sphere §13.3.7, Box §13.3.1, Cone §13.3.2, Cylinder §13.3.3)
