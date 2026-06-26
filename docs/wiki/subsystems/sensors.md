---
title: Sensor Layer Overview
summary: Cross-cutting view of the sensor layer — time, pointing, key, and visibility/transform sensor wiring via the active-node model.
tags: [subsystem, sensors, active-node, cross-cutting]
updated: 2026-06-22
related:
  - ../architecture.md
  - ../subsystems/execution-context.md
  - ../subsystems/event-cascade.md
  - ../subsystems/system-time.md
  - ../subsystems/system-pointing.md
  - ../subsystems/system-keydevice.md
  - ../subsystems/system-viewdependent.md
---

# Sensor Layer Overview

## Purpose

The sensor layer is the collection of behavior Systems that translate external
stimuli (clock ticks, pointer motion, keyboard input, viewer position) into X3D
field events. Sensors are the upstream end of the X3D event model: they post
events into the execution context, and the cascade fans those events to
interpolators, scripts, and field-driven transforms.

Every sensor family is implemented as a `System` (see
`runtime/events/X3DSystem.hpp`). A System is the unit that a browser registers
with an `X3DExecutionContext` via `ctx.addSystem(sys)`. Each System has two
responsibilities:

- **`attach(node, ctx)`** — enroll one node. Time-driven systems stash the
  node pointer for per-tick iteration; event-driven systems wire an `inputOnly`
  handler here instead.
- **`update(now, ctx)`** — run per-tick work (only time-driven systems do real
  work here; event-driven systems leave this as the default no-op).

The `ActiveNode` interface in `runtime/events/X3DActiveNode.hpp` is the earlier,
per-node form of the same protocol: one `ActiveNode` per node instance, with a
single `update(now, ctx)` method. It has been superseded by `System` for all
production use but is retained for the `animation_test` (via `TimeSensorBehavior`)
and wrapped by `ActiveNodeAdapter` inside `X3DExecutionContext` so that legacy
callers still compile. New sensor families implement `System` directly.

## Key files

| File | Role |
|---|---|
| `runtime/events/X3DActiveNode.hpp` | `ActiveNode` — legacy per-node update protocol (superseded by `System`; retained for backward compat + one test). |
| `runtime/events/X3DSystem.hpp` | `System` — the current behavior-family abstraction: `attach(X3DNode*, X3DExecutionContext&)` + default no-op `update(double, X3DExecutionContext&)`. All sensor systems derive from this. |
| `runtime/events/X3DExecutionContext.hpp` | Owns the System list; `addSystem(shared_ptr<System>)` registers a family; `tick(now)` calls every system's `update` before draining the cascade. |
| `runtime/events/X3DTimeDependentSystem.hpp` | Reusable base for time-dependent nodes: the full ISO clock state machine (activation/pause/deactivation/elapsed-time/cycle-index). `TimeSensorSystem` extends it. |
| `runtime/events/TimeSensorSystem.hpp` | `TimeSensor` driver: guards `attach` to `TimeSensor*`, reads `enabled`/`loop`/`cycleInterval`, and implements `emitCycleOutputs` (→ `fraction_changed`, `time`) and `emitCycleTime` (→ `cycleTime`). |
| `runtime/events/TimeSensorBehavior.hpp` | Legacy `ActiveNode`: drives `fraction_changed` only, no `isActive`/pause/`cycleTime`. Used by `animation_test`; superseded by `TimeSensorSystem`. |
| `runtime/events/PointingSensorSystem.hpp` | Pointing-device sensor driver: `TouchSensor` + drag sensors (`PlaneSensor`, `SphereSensor`, `CylinderSensor`). Resolves pointer ray vs. scene each tick via `ctx.pick()`; manages the grab. No `attach` enrollment needed — sensors are resolved live from the pick path. |
| `runtime/events/KeyDeviceSensorSystem.hpp` | Key-device sensor driver: `KeySensor` (§21.4.1) + `StringSensor` (§21.4.2). Drains `ctx.keyState().events` on each tick and emits spec-mandated outputs to every enrolled enabled sensor. |
| `runtime/scene/ViewDependentSystem.hpp` | View-dependent sensor driver: `ProximitySensor` (viewer-in-box) + `VisibilitySensor` (box-in-cone frustum test) + `TransformSensor` (targetObject-AABB-in-sensor-box) enter/exit. Also drives LOD level selection. Runs in `update(now, ctx)` using `ctx.cameraWorldPosition()`, `ctx.viewMatrix()`, and `ctx.worldTransformAny(target)` to compute the target's world frame. |

## Interfaces and seams

### The `System` base

```cpp
// runtime/events/X3DSystem.hpp
namespace x3d::runtime {

class System {
public:
  virtual ~System() = default;

  // Enroll a node. Time-driven systems stash it; event-driven systems
  // wire an inputOnly handler here.
  virtual void attach(X3DNode *node, X3DExecutionContext &ctx) = 0;

  // Advance time-driven nodes to `now`, emitting events into `ctx`.
  // Default no-op — event-driven systems do all work from attach-wired handlers.
  virtual void update(double now, X3DExecutionContext &ctx) {
    (void)now; (void)ctx;
  }
};

} // namespace x3d::runtime
```

### The `ActiveNode` base (legacy)

```cpp
// runtime/events/X3DActiveNode.hpp
namespace x3d::runtime {

class ActiveNode {
public:
  virtual ~ActiveNode() = default;
  virtual void update(double now, X3DExecutionContext &ctx) = 0;
};

} // namespace x3d::runtime
```

One `ActiveNode` per node; the context wraps each in an `ActiveNodeAdapter` that
forwards to a one-element `System`. New code uses `System` directly.

### Registration and tick wiring

```cpp
// Register a behavior family (once per context):
auto sys = std::make_shared<TimeSensorSystem>();
ctx.addSystem(sys);

// Enroll scene nodes (called for each relevant node in the scene):
sys->attach(timeSensorNode, ctx);

// Drive the tick (the context calls every system's update internally):
ctx.tick(now);   // → calls sys->update(now, ctx) for every registered System
                 //    → then drains the event cascade to quiescence
```

### Event injection

All sensor output goes through `ctx.postEvent(node, fieldName, value)`. This
seeds an event into the cascade, which fans it out along ROUTEs within the
same timestamp quantum. Sensors never write node fields directly; they always
go through the cascade seam.

### Seam points

- **`X3DExecutionContext::addSystem` / `tick`** — the sole entry point. The
  context calls `sys->update(now, ctx)` for every registered System before each
  cascade drain. See [Execution Context](../subsystems/execution-context.md).

- **`ctx.postEvent`** — all sensor output events flow here. The cascade
  delivers them to ROUTE sinks in the same tick.
  See [Event Cascade](../subsystems/event-cascade.md).

- **`ctx.pointerState()`** — `PointingSensorSystem` reads `PointerState`
  (`revision`, `ray`, `buttonDown`, `present`) that the browser writes between
  ticks via `ctx.setPointer*`. The `revision` counter lets the system skip ticks
  where the pointer state has not changed (§20.4.4).

- **`ctx.keyState().events`** — `KeyDeviceSensorSystem` drains the discrete
  `KeyEvent` queue (filled via `ctx.pushKeyCharacter`, `ctx.pushActionKey`, etc.)
  and calls `ctx.clearKeyEvents()` after processing. `NavigationSystem` reads the
  separate `held` set from the same `KeyState`; the two consumers do not conflict.

- **`ctx.cameraWorldPosition()` / `ctx.viewMatrix()`** — `ViewDependentSystem`
  reads the camera pose here; the browser sets this by binding a `Viewpoint` (or
  by direct `viewMatrix` injection for head-tracked CAVE setups).

- **`ctx.worldTransform(node)`** — `ViewDependentSystem` transforms the viewer
  position into the sensor's local coordinate system for `ProximitySensor`'s
  inside-box test and for `VisibilitySensor`'s box-to-world projection.

- **`ctx.pick(ray)` / `ctx.worldOf(node)`** — `PointingSensorSystem` uses
  `pick()` to resolve the pointing ray against the scene's `PickSystem` and
  `worldOf(sensor)` to compute the sensor's coordinate frame for drag sensors
  and for `hitPoint`/`hitNormal` transformation.

- **`X3DTimeDependentSystem` extension seam** — `emitCycleOutputs(node, frac,
  now, elapsed, ctx)` and `emitCycleTime(node, cycleStart, ctx)` are overridable
  virtual methods. `TimeSensorSystem` overrides them to emit its continuous
  outputs. Future time-dependent nodes (`AudioClip`, `MovieTexture`) will supply
  their own subclass without touching the clock state machine.

- **`X3DSceneBridge.hpp` attach helpers** — `attachViewDependent`, `attachInterpolators`,
  `attachEventUtilities`, and `attachKeyDeviceSensors` walk the scene once via
  `detail::forEachNode` and offer every node to each System's `attach`. Browsers
  call these after `buildFrom(scene)` to fully wire a loaded scene.

## How it is tested

The sensor families each have a dedicated test binary:

- `ctest --preset dev -R x3d_events_tests` (doctest case: `timesensor_test`) — `runtime/events/tests/timesensor_test.cpp`.
  Full `TimeSensor`/`X3DTimeDependentNode` lifecycle: `enabled=false`, start gating,
  single-shot completion, looping wrap + `cycleTime` pulses, `stopTime` early
  deactivation, pause/resume (elapsed-time excludes the paused span), and the
  `cycleInterval <= 0` guard.

- `ctest --preset dev -R x3d_events_tests` (doctest case: `timesensor_rtc_test`) — `runtime/events/tests/timesensor_rtc_test.cpp`.
  Four spec-conformance regressions:
  - **RTC-1**: `cycleInterval <= 0` with `loop=TRUE` must not crash (divide-by-zero guard).
  - **RTC-2**: looping sensor emits `fraction_changed == 1.0` at each exact cycle boundary.
  - **RTC-3**: `cycleTime` == `startTime + cycleIndex * cycleInterval` (NOT the tick's `now`).
  - **RTC-4**: `set_enabled FALSE` while active emits the final outputs BEFORE `isActive=FALSE`.

- `ctest --preset dev -R x3d_events_tests` (doctest case: `pointing_sensor_test`) — `runtime/events/tests/pointing_sensor_test.cpp`.
  `PointingSensorSystem` over `TouchSensor` + `PlaneSensor`: `isOver` enter/leave,
  resolution to the lowest sensor on the path, nearest-geometry selection, hit-point/
  hit-normal in the sensor frame under a translated/rotated transform, `hitTexCoord`
  (barycentric + primitive), `isActive` + `touchTime` click semantics, grab exclusivity,
  and `enabled=FALSE` gate.

- `ctest --preset dev -R x3d_events_tests` (doctest case: `key_device_sensor_test`) — `runtime/events/tests/key_device_sensor_test.cpp`.
  `KeyDeviceSensorSystem` over `KeySensor` and `StringSensor`:
  `keyPress`/`keyRelease` (UTF-8 char), `actionKeyPress`/`actionKeyRelease` (Table 21.2),
  `shiftKey`/`controlKey`/`altKey`, `isActive` lifecycle (§21.4.1); `enteredText`
  accumulator, `finalText`+reset, `deletionAllowed` (§21.4.2); enabled gate.

- `ctest --preset dev -R x3d_geometry_scene` (doctest case: `view_dependent_test`) — `runtime/scene/tests/view_dependent_test.cpp`.
  `ViewDependentSystem` covering: LOD level selection (`lodSelectLevel` step function,
  child-count clamping, `level_changed` event emission), `ProximitySensor` enter/exit +
  `position_changed`/`orientation_changed` change-gating (ENV-04), `VisibilitySensor`
  forward-cone frustum test, `TransformSensor` targetObject-AABB-in-sensor-box
  enter/exit + position/orientation change-gating (resolves the target's world
  frame via the new `TransformSystem::worldTransformAny` so the target reachable
  through both `targetObject` and a Transform ancestor picks the Transform path),
  and the `enabled=FALSE` disable-deactivates edge (ENV-07).

## Related specs and ADRs

- [Architecture](../architecture.md)
- [Execution Context](../subsystems/execution-context.md)
- [Event Cascade](../subsystems/event-cascade.md)
- [Time System](../subsystems/system-time.md)
- [Pointing-Device Sensor System](../subsystems/system-pointing.md)
- [Key-Device Sensor System](../subsystems/system-keydevice.md)
- [View-Dependent System](../subsystems/system-viewdependent.md)
- ISO/IEC 19775-1 §8.2.4 — `X3DTimeDependentNode` clock semantics (activation/deactivation gating, `set_startTime` ignored while active, pause/resume).
- ISO/IEC 19775-1 §8.4.1 — `TimeSensor` field semantics (`fraction_changed` boundary rule, `cycleTime` definition).
- ISO/IEC 19775-1 §20.2.1–§20.4.4 — pointing-device sensor model: nearest-geometry resolution, grab exclusivity, `TouchSensor` outputs, drag-sensor `autoOffset`.
- ISO/IEC 19775-1 §21.4.1–§21.4.2 — `KeySensor` and `StringSensor` output contracts.
- ISO/IEC 19775-1 §22.4.1 — `ProximitySensor` inside-box test and `position_changed`/`orientation_changed` semantics.
- ISO/IEC 19775-1 §22.4.2 — `TransformSensor` targetObject-AABB-in-sensor-box test (sensor-local box; position/orientation emitted relative to `center`).
- ISO/IEC 19775-1 §22.4.3 — `VisibilitySensor` visibility test.
- `docs/superpowers/BACKLOG.md` (deprecated, historical) — M2.5 (input seam + TouchSensor), M2D (drag sensors + KeyState), M2e (view-dependent sensors + LOD); all closed.
