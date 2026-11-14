---
title: Time
summary: TimeSensor system and the time-dependent node base that drives all time-indexed behaviors.
tags: [subsystem, time, timesensor, time-dependent]
updated: 2026-09-26
related:
  - ../../explanation/architecture.md
  - execution-context.md
  - system-interpolators.md
  - routes.md
  - sensors.md
---

# Time

## Purpose

The Time System advances time-dependent nodes one tick at a time and emits their spec-mandated output events into the cascade. It owns the X3D clock state machine for every node that carries `startTime`/`stopTime`/`pauseTime`/`resumeTime` fields (`X3DTimeDependentNode`), driving the full lifecycle: idle → active → (paused) → deactivated. For the `TimeSensor` node specifically it also computes and posts `fraction_changed`, `time`, `cycleTime`, and `elapsedTime` on every relevant tick edge.

## Key files

| File | Role |
|---|---|
| `runtime/events/X3DTimeDependentSystem.hpp` | Base `System` implementing the full X3D clock state machine (activation/pause/deactivation/elapsed-time) shared by every time-dependent node. Per-node state is held in the system, not on the node. |
| `runtime/events/TimeSensorSystem.hpp` | Concrete `X3DTimeDependentSystem` for `TimeSensor`. Guards `attach` to `TimeSensor*`, reads `enabled`/`loop`/`cycleInterval`, and implements `emitCycleOutputs` (→ `fraction_changed`, `time`) and `emitCycleTime` (→ `cycleTime`). |
| `runtime/events/X3DSystem.hpp` | `System` base class — defines `attach(node, ctx)` and the default no-op `update(now, ctx)`. `X3DTimeDependentSystem` inherits from `System`. |
| `runtime/events/tests/timesensor_test.cpp` | Lifecycle edge-case test suite (doctest case `timesensor_test` in the `x3d_events_tests` target). |
| `runtime/events/tests/timesensor_rtc_test.cpp` | Regression suite for TimeSensor clock and input edge cases in `x3d_events_tests`. |
| `runtime/events/tests/animation_test.cpp` | End-to-end animation chain test using `TimeSensorSystem` (doctest case `animation_test` in the `x3d_events_tests` target). |

## Interfaces and seams

### Exposed interface

`TimeSensorSystem` is a `System` subclass. A browser (or test harness) registers it once with the execution context and calls `attach` for every `TimeSensor` node:

```cpp
// Register the system (once per context)
auto sys = std::make_shared<x3d::runtime::TimeSensorSystem>();
ctx.addSystem(sys);

// Enroll each TimeSensor in the scene
sys->attach(timeSensorNode, ctx);

// Each tick the context calls update internally:
// sys->update(now, ctx);  // called by X3DExecutionContext::tick(now)
```

`X3DExecutionContext::tick(now)` calls `update(now, ctx)` on every registered system before draining the event cascade. The system posts all output events via `ctx.postEvent(node, "<fieldName>", value)`, so downstream ROUTEs fire in the same timestamp quantum (see [Event cascade](event-cascade.md) and [ROUTEs](routes.md)).

### Per-node state

`X3DTimeDependentSystem` holds per-node lifecycle state in a private `std::unordered_map<X3DTimeDependentNode *, State>`. The `State` struct tracks:

| Field | Meaning |
|---|---|
| `active` | Whether the node is currently active. |
| `paused` | Whether the node is paused (outputs frozen). |
| `timeBase` | Effective activation origin, shifted forward by any paused spans so that `elapsed = now - timeBase` excludes paused intervals. |
| `cycleBase` / `cycleInterval` / `lastCycleTick` | Cycle phase and last evaluated tick. Changing `cycleInterval` rebases cycle phase without changing elapsed time; an interval shorter than the elapsed cycle completes that cycle. |
| `pausedAt` | Tick when pausing took effect; the resume shift excludes the entire observed paused span. |
| `activeStartTime` | `startTime` snapshotted at activation; the lifecycle uses this frozen value while active (spec §8.2.4.3 requires `set_startTime` to be ignored mid-run). |
| `completed` / `completedStartTime` | Re-activation guard: a finished single-shot must not auto-restart unless a new `startTime` is set. |
| `lastCycleAnnounced` | Index of the last cycle whose `cycleTime` pulse was emitted; ensures exactly one pulse per boundary. |

State lives in the system, never on the node — the node and its reflection `FieldTable` remain the source of truth; outputs flow back onto the node's `outputOnly` fields through the cascade.

### Seam points

- `X3DExecutionContext::addSystem` / `tick` — The execution context drives the system on every tick. See [Execution context](execution-context.md).
- `emitCycleOutputs` (virtual hook) — Derived systems override this to emit their node-specific continuous outputs each active, unpaused tick. `TimeSensorSystem` emits `time` and `fraction_changed` here.
- `emitCycleTime` (virtual hook) — Derived systems override this to emit a cycle-start pulse. `TimeSensorSystem` emits `cycleTime` here.
- `readEnabled` / `readLoop` / `readCycleInterval` (virtual reads) — Overridable per node type. `MediaTimeSystem` (`runtime/events/MediaTimeSystem.hpp`, attached by `attachStandardRuntime`) uses them for `AudioClip` and `MovieTexture`: `enabled`, `loop`, and a cycle of one media pass, `duration_changed` divided by the pitch or speed captured at activation; active `set_pitch` and `set_speed` deliveries are ignored. An unknown duration (-1 before load) plays until `stopTime`. A consumer that opens a movie calls `reportMovieDuration(ctx, movie, seconds)` once its duration is known; this posts `duration_changed` through the event cascade and supplies the lifecycle clock.
- Timing input filter — The shared base registers a filter with the execution context when a node is attached. It rejects active `startTime` and `stopTime <= startTime` input events before the cascade writes or routes them (§8.2.4.3). This covers TimeSensor, AudioClip, MovieTexture, and SoundTimeSystem's v4 sound nodes; ordered stop-then-start restart remains accepted. `MediaTimeSystem` adds its own filter on the same hook for active `set_pitch` / `set_speed`.
- `ctx.postEvent` — All output writes go through `X3DExecutionContext::postEvent`; the cascade fans them out to connected ROUTEs in the same tick. See [ROUTEs](routes.md).
- Interpolator downstream — `fraction_changed` from `TimeSensorSystem` is the canonical clock signal that drives interpolators. See [Interpolators](system-interpolators.md).
- Sensor layer — `TimeSensorSystem` is one of the sensor-like systems registered through the `System` pattern. See [Sensors overview](sensors.md).

## Time origin (absolute epoch vs. application-relative)

X3D's `SFTime` is normatively defined as seconds since the Unix epoch (00:00:00
GMT, 1 Jan 1970), so a strict reading makes `startTime=0` mean *1970* and treats a
`loop=TRUE` `TimeSensor` as having run since the epoch. Castle Game Engine documents
why that is awkward in practice — huge time values lose precision in shaders, you
cannot pause, and `startTime=0` is not "now" — and adds a `NavigationInfo.timeOriginAtLoad`
extension so `startTime=0` means *application load*
(see [the Castle write-up](https://castle-engine.io/x3d_time_origin_considered_uncomfortable)).

This SDK does not bake a time origin in. `TimeSensorSystem` (and every other
time-dependent system) reads time only via the `now` value the embedder passes to
`X3DExecutionContext::tick(now)` — it never calls a wall clock itself. The *consumer*
chooses the origin:

- Application-relative (the default for our renderers). Both bundled consumers
  feed `now` starting at ≈0 at load — the OpenGL PoC feeds `glfwGetTime()`, the CPU
  rasterizer's `--animate` feeds `frame / fps`. With this feeding, `startTime=0`
  behaves as *at load* — i.e., x3d-cpp gets Castle's `timeOriginAtLoad` behaviour for
  free, which is why authored loop animations begin immediately and stay in
  float-friendly ranges. Pausing/scrubbing is just "stop advancing / rewind `now`".
- Absolute epoch (strict conformance). A consumer that needs literal spec
  semantics — e.g. a file authoring an absolute `startTime` as a real wall-clock
  instant — feeds Unix-epoch `now`. The SDK honours whatever it is given.

Strict-conformance nuance: because the origin is the consumer's choice, a file
that authors an absolute `startTime` will *phase differently* under application-relative
feeding than under epoch feeding. For loop animations with `startTime <= now` this only
shifts the phase, not whether they animate; it matters only for content keyed to an
absolute instant. This is a deliberate contract, not a bug — see finding `TIME-ORIGIN-1`.

## Related
- [Architecture](../../explanation/architecture.md)
