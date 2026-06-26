# `x3d sim` — headless behavior simulation/trace harness

**Date:** 2026-06-20
**Status:** Approved design; build via workflow.
**Goal:** A headless harness that drives the SDK's event/behavior runtime and traces what changes — the SDK's signature capability (no other X3D CLI runs the behavior graph headlessly), the biggest dogfooding gap (the event runtime is untouched by the CLI), and the universal test rig that grows automatically as the runtime grows.

## Why
`convert`/`validate`/`extract`/`canonicalize` exercise the parse + extract layers. `sim` lights up the **event/behavior runtime** — routes, sensors, interpolators, scripts, view-dependent — which is the SDK's differentiator. It makes X3D *behavior* testable in CI (regression-test an animation/script/sensor without a renderer), and it's the closest CLI analog to the CAVE master's tick loop. It is also the **harness for every future behavior subsystem** — notably the planned Jolt-backed physics seam (the physics nodes are modeled but not yet simulated; `sim` is what you'd build the integrator against).

## Architecture
Three units:
1. **"Wire full runtime" helper** (new, reusable — sim-local now, extractable to `x3d::sdk` later): given a parsed `Scene` + an `X3DExecutionContext`, attach **every** behavior system the runtime has — `TimeSensorSystem`, the interpolator registration (`InterpolatorSystem`/`SplineInterpolatorSystem` via `InterpolatorRegistration`), `EventUtilitySystem`, `ViewDependentSystem`, and the `ScriptSystem` (if a script engine backend is linked). Study `runtime/parse/tests/inline_routes_test.cpp` + `runtime/events/tests/*` for the exact per-system attach idiom (no single helper exists today — that's the gap this fills). Include a **physics-attach hook**: a clean, no-op-today slot where a future `RigidBodySystem` plugs in (so the Jolt-physics initiative wires in without touching sim). This is the CAVE-useful "run this scene's behavior" unit.
2. **Tick loop + pluggable drivers.** Tick `N` times at `1/fps` from `t0=0`. Drivers (v1):
   - **Time** (always): `ctx.tick(now)` for `now = 0, 1/fps, 2/fps, …` → runs interpolators, scripts, event-logic, the cascade. TimeSensors with `loop=true` animate.
   - **Viewer-path** (`--move 'x y z -> x y z over Ds'`): each tick, set the viewer position along the linear path through the execution-context viewer seam (`setHeadPose` / viewpoint offset → `cameraWorldPosition()`), which drives `ViewDependentSystem` → ProximitySensor/VisibilitySensor enter/exit, LOD switches, Billboard rotation.
3. **Field-change tracer.** Uses a **snapshot-diff** mechanism, not a cascade observer. Reason: `X3DExecutionContext` exposes exactly one per-field delivery observer slot (`cascade_.setFieldObserver`), already bound by `buildSceneGraph()` to `classifyDirty()` which feeds the `DirtyTracker` → transform/bounds propagation. Evicting or chaining that observer is not exposed and would break the dirty-driven world-transform/bounds update. So the tracer takes the documented fallback: each tick it snapshots every readable non-node field value (via `fields()` reflection), diffs against the previous snapshot, and emits `(node, field, newValue)` deltas. Sensor + interpolator outputs (`enterTime`, `isActive`, `position_changed`, `value_changed`) are `inputOutput`/`outputOnly` fields whose stored values the cascade writes, so they surface naturally — no special-casing.

## CLI
`x3d sim <in> [--fps F=30] [--ticks N | --duration D] [--move 'a -> b over Ds'] [--watch DEF.field …] [--json]`
- `--ticks N` or `--duration D` (⇒ `N = round(D·F)`); default `--fps 30 --ticks 60` (2 s).
- Default trace: broad — every changed `(node,field)=value` per tick. `--watch DEF.field` (repeatable) narrows to specific fields. `--json` emits a machine trace (the golden-regression form: an array of `{tick, t, changes:[{node,field,value}]}`).
- Exit: 0 ok, 2 parse/IO, 1 usage. Never crash on malformed input.

## Self-oracle (sim's "prove it")
No clean external oracle (Xj3D's SAI could in principle, but heavy) — so self-based, and strong:
1. **Determinism** — same scene + same drivers → byte-identical trace (a test runs sim twice, asserts equal). A behavior harness that isn't deterministic is useless.
2. **Golden traces** — commit the expected `--json` trace for fixture scenes; a regression test asserts the current trace matches; slot into the existing `mise run cli-gate` regression machinery.
3. **Behavioral correctness** — assert real computed values on crafted fixtures: a `TimeSensor(cycleInterval=1,loop) → PositionInterpolator(key 0 1, keyValue '0 0 0  10 0 0') → Transform` shows `translation.x ≈ 5` at `t≈0.5`; a scene with a `ProximitySensor` (box region) + a `--move` path through it fires `enterTime`/`exitTime` at the ticks where the viewer crosses the region; (if the script engine is linked) a `Script` fixture shows script-driven output.

## Scope (v1)
- **Drivers: time + viewer-path.** Documented growth seams, already-wired systems behind them: **input-injection** (`--key`/`--click` at ticks → TouchSensor/drag/KeySensor via `PointingSensorSystem`/`KeyDeviceSensorSystem`) — fast-follow; **physics** — the no-op attach hook is in v1; the Jolt-backed `RigidBodySystem` is the next initiative, and `sim` traces it the moment it lands.
- ScriptSystem attaches only if a script engine backend is available in the build; if not, scripts are inert and `sim` notes it (not a failure).
- Trace is field-changes; it does NOT (v1) render or extract geometry — that's `extract`'s job.

## Testing
- CLI (`tools/tests/x3d_cli_test.sh` + small fixtures): sim the animation fixture → trace shows correct animating `translation` values + deterministic + matches golden; the ProximitySensor + `--move` fixture → `enterTime`/`exitTime` appear at the right tick; `--watch` narrows; `--json` parses.
- Determinism unit test (two runs byte-identical).
- A C++ behavioral unit test asserting the interpolator value at `t≈0.5` and the proximity enter/exit tick (the runtime actually computing correctly).

## Success criteria
1. `x3d sim scene.x3d` drives the full behavior runtime and emits a deterministic field-change trace; `--watch`/`--json` work.
2. Behavioral correctness holds (interpolator value at t; proximity enter/exit on the viewer path).
3. Golden traces committed + regression-gated.
4. Gates: `mise run build` green, `mise run golden` zero-drift, generated/runtime-default untouched. The "wire full runtime" helper carries a clean physics-attach hook for the next initiative.
