---
title: Time System
summary: TimeSensor system and the time-dependent node base that drives all time-indexed behaviors.
tags: [subsystem, time, timesensor, time-dependent]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/execution-context.md
  - ../subsystems/system-interpolators.md
  - ../subsystems/routes.md
  - ../subsystems/sensors.md
---

# Time System

## Purpose

The Time System advances time-dependent nodes one tick at a time and emits their spec-mandated output events into the cascade. It owns the X3D clock state machine for every node that carries `startTime`/`stopTime`/`pauseTime`/`resumeTime` fields (`X3DTimeDependentNode`), driving the full lifecycle: idle → active → (paused) → deactivated. For the `TimeSensor` node specifically it also computes and posts `fraction_changed`, `time`, `cycleTime`, and `elapsedTime` on every relevant tick edge. `TimeSensorBehavior` is an earlier, minimal active-node seed that drives only `fraction_changed` over a single cycle; it is retained for the `animation_test` but superseded by `TimeSensorSystem` for production use.

## Key files

| File | Role |
|---|---|
| `runtime/events/X3DTimeDependentSystem.hpp` | Base `System` implementing the full X3D clock state machine (activation/pause/deactivation/elapsed-time) shared by every time-dependent node. Per-node state is held in the system, not on the node. |
| `runtime/events/TimeSensorSystem.hpp` | Concrete `X3DTimeDependentSystem` for `TimeSensor`. Guards `attach` to `TimeSensor*`, reads `enabled`/`loop`/`cycleInterval`, and implements `emitCycleOutputs` (→ `fraction_changed`, `time`) and `emitCycleTime` (→ `cycleTime`). |
| `runtime/events/TimeSensorBehavior.hpp` | Earlier, minimal `ActiveNode` seed: drives `fraction_changed` only, over one cycle (or looped), with no `isActive`/pause/`cycleTime`/`elapsedTime` support. Used by `animation_test`; superseded by `TimeSensorSystem`. |
| `runtime/events/X3DSystem.hpp` | `System` base class — defines `attach(node, ctx)` and the default no-op `update(now, ctx)`. `X3DTimeDependentSystem` inherits from `System`. |
| `runtime/events/tests/timesensor_test.cpp` | Lifecycle edge-case test suite (doctest case `timesensor_test` in the `x3d_events_tests` target). |
| `runtime/events/tests/timesensor_rtc_test.cpp` | Regression suite for four spec-conformance bugs RTC-1..RTC-4 (doctest case `timesensor_rtc_test` in the `x3d_events_tests` target). |
| `runtime/events/tests/animation_test.cpp` | End-to-end animation chain test using `TimeSensorBehavior` (doctest case `animation_test` in the `x3d_events_tests` target). |

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

`X3DExecutionContext::tick(now)` calls `update(now, ctx)` on every registered system before draining the event cascade. The system posts all output events via `ctx.postEvent(node, "<fieldName>", value)`, so downstream ROUTEs fire in the same timestamp quantum (see [Event Cascade](../subsystems/event-cascade.md) and [Routes](../subsystems/routes.md)).

### Per-node state

`X3DTimeDependentSystem` holds per-node lifecycle state in a private `std::unordered_map<X3DTimeDependentNode *, State>`. The `State` struct tracks:

| Field | Meaning |
|---|---|
| `active` | Whether the node is currently active. |
| `paused` | Whether the node is paused (outputs frozen). |
| `timeBase` | Effective activation origin, shifted forward by any paused spans so that `elapsed = now - timeBase` excludes paused intervals. |
| `activeStartTime` | `startTime` snapshotted at activation; the lifecycle uses this frozen value while active (spec §8.2.4.3 requires `set_startTime` to be ignored mid-run). |
| `completed` / `completedStartTime` | Re-activation guard: a finished single-shot must not auto-restart unless a new `startTime` is set. |
| `lastCycleAnnounced` | Index of the last cycle whose `cycleTime` pulse was emitted; ensures exactly one pulse per boundary. |

State lives in the system, never on the node — the node and its reflection `FieldTable` remain the source of truth; outputs flow back onto the node's `outputOnly` fields through the cascade.

### Seam points

- **`X3DExecutionContext::addSystem` / `tick`** — The execution context drives the system on every tick. See [Execution Context](../subsystems/execution-context.md).
- **`emitCycleOutputs` (virtual hook)** — Derived systems override this to emit their node-specific continuous outputs each active, unpaused tick. `TimeSensorSystem` emits `time` and `fraction_changed` here.
- **`emitCycleTime` (virtual hook)** — Derived systems override this to emit a cycle-start pulse. `TimeSensorSystem` emits `cycleTime` here.
- **`readEnabled` / `readLoop` / `readCycleInterval` (virtual reads)** — Overridable so future time-dependent systems (`AudioClip`, `MovieTexture`) can spell these fields differently or supply defaults.
- **`ctx.postEvent`** — All output writes go through `X3DExecutionContext::postEvent`; the cascade fans them out to connected ROUTEs in the same tick. See [Routes](../subsystems/routes.md).
- **Interpolator downstream** — `fraction_changed` from `TimeSensorSystem` is the canonical clock signal that drives interpolators. See [Interpolator System](../subsystems/system-interpolators.md).
- **Sensor layer** — `TimeSensorSystem` is one of the sensor-like systems registered through the `System`/`X3DActiveNode` pattern. See [Sensors](../subsystems/sensors.md).

## Time origin (absolute epoch vs. application-relative)

X3D's `SFTime` is normatively defined as **seconds since the Unix epoch** (00:00:00
GMT, 1 Jan 1970), so a strict reading makes `startTime=0` mean *1970* and treats a
`loop=TRUE` `TimeSensor` as having run since the epoch. Castle Game Engine documents
why that is awkward in practice — huge time values lose precision in shaders, you
cannot pause, and `startTime=0` is not "now" — and adds a `NavigationInfo.timeOriginAtLoad`
extension so `startTime=0` means *application load*
(see [the Castle write-up](https://castle-engine.io/x3d_time_origin_considered_uncomfortable)).

**This SDK does not bake a time origin in.** `TimeSensorSystem` (and every other
time-dependent system) reads time only via the `now` value the embedder passes to
`X3DExecutionContext::tick(now)` — it never calls a wall clock itself. The *consumer*
chooses the origin:

- **Application-relative (the default for our renderers).** Both bundled consumers
  feed `now` starting at ≈0 at load — the OpenGL PoC feeds `glfwGetTime()`, the CPU
  rasterizer's `--animate` feeds `frame / fps`. With this feeding, `startTime=0`
  behaves as *at load* — i.e., x3d-cpp gets Castle's `timeOriginAtLoad` behaviour for
  free, which is why authored loop animations begin immediately and stay in
  float-friendly ranges. Pausing/scrubbing is just "stop advancing / rewind `now`".
- **Absolute epoch (strict conformance).** A consumer that needs literal spec
  semantics — e.g. a file authoring an absolute `startTime` as a real wall-clock
  instant — feeds Unix-epoch `now`. The SDK honours whatever it is given.

**Strict-conformance nuance:** because the origin is the consumer's choice, a file
that authors an absolute `startTime` will *phase differently* under application-relative
feeding than under epoch feeding. For loop animations with `startTime <= now` this only
shifts the phase, not whether they animate; it matters only for content keyed to an
absolute instant. This is a deliberate contract, not a bug — see finding `TIME-ORIGIN-1`.

## How it is tested

- `ctest --preset dev -R x3d_events_tests` (doctest case: `timesensor_test`) — `runtime/events/tests/timesensor_test.cpp`. Full lifecycle edge cases: `enabled=false`, start gating, single-shot completion, looping wrap + `cycleTime` pulses, `stopTime` early deactivation, pause/resume (`elapsedTime` excludes the paused span), `cycleInterval <= 0` guard, `set_startTime` ignored while active (TDN-4), `loop=FALSE` finishes the current cycle not immediately (TDN-3/TDN-7), strict `resumeTime > pauseTime` requirement (TDN-6), `pauseTime_changed`/`resumeTime_changed` ROUTE fire (TDN-1/TDN-2), and re-activation guard (no auto-restart after completion).
- `ctest --preset dev -R x3d_events_tests` (doctest case: `timesensor_rtc_test`) — `runtime/events/tests/timesensor_rtc_test.cpp`. Four spec-conformance regression fixes:
  - **RTC-1**: `cycleInterval <= 0` with `loop=TRUE` must not crash (divide-by-zero guard: `guardCycle` returns `1.0` for `<= 0`).
  - **RTC-2**: A looping sensor emits `fraction_changed == 1.0` at each exact cycle-boundary tick (spec §8.4.1 boundary rule: `if (f==0 && now>startTime) fraction_changed=1`).
  - **RTC-3**: `cycleTime` value equals `startTime + cycleIndex * cycleInterval` (the cycle-start time), not the tick's `now`.
  - **RTC-4**: `set_enabled FALSE` while active emits the final `time`/`fraction_changed`/`elapsedTime` outputs **before** `isActive=FALSE`.
- `ctest --preset dev -R x3d_events_tests` (doctest case: `animation_test`) — `runtime/events/tests/animation_test.cpp`. End-to-end chain test: `TimeSensorBehavior` → `PositionInterpolator` → `Transform.translation`, confirming the full clock → cascade → interpolator → field-write pipeline.

## Related specs and ADRs

- Spec §8.2.4 (X3D 4.0 ISO normative prose): `X3DTimeDependentNode` clock semantics — activation/deactivation gating, `set_startTime` ignored while active, `loop=FALSE` finishes the current cycle, pause/resume elapsed-time contract.
- Spec §8.4.1 (X3D 4.0): `TimeSensor` field semantics — `fraction_changed` boundary rule, `cycleTime` definition, `elapsedTime` contract.
- Spec §8.2.4.3: re-activation guard — a completed node must not auto-restart until a new `startTime` is received.
- Spec §8.2.4.4: pause/resume edge events — `pauseTime_changed` and `resumeTime_changed` output events at the transition edges.
