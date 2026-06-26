# M1 Behavior Runtime — Implementation Spec

**Date:** 2026-06-03
**Status:** Ready to build (TDD, header-only, node-agnostic)
**Branch:** `modernize-x3d-spec`
**Builds on:** `2026-06-03-event-system-design.md` (cascade foundation, shipped),
`2026-06-03-week-plan.md` (Day-by-day plan). Implements its keystone tasks.

## Goal & invariants

Make a parsed `Scene` *tickable*: bridge its DEF-named ROUTEs onto the
pointer-based `EventGraph`, validate them at registration, generalize
`ActiveNode` into a `System` over node collections, and implement the
interpolator family + full TimeSensor / `X3DTimeDependentNode` lifecycle as
systems.

**Binding invariants (from decisions memory + design doc):**
- **Node-as-truth.** OOP node model + reflection `FieldTable` stays the source of
  truth. Systems are runtime-side projections *over* nodes; no entity store, no
  ECS, no per-node event bookkeeping in generated headers.
- **Systems-seam.** All new files are header-only under `runtime/events/`, in
  `namespace x3d::runtime`, driven purely by reflection + generated accessors.
  No codegen changes are required by this milestone (the inputOnly/outputOnly
  thunks already exist).
- **inputOnly = event API** (`onX` + `setOnXHandler`); **outputOnly** written via
  the reflection `set` thunk (cascade-internal). Range-checked setters throw
  `std::out_of_range`; the cascade lets it propagate.

---

## 1. Scene → EventGraph bridge + ROUTE validation

New header **`runtime/events/X3DSceneBridge.hpp`** — a free-function bridge, not a
class (mirrors the codec style: stateless, reflection-driven).

```cpp
namespace x3d::runtime {

struct RouteError {            // one rejected ROUTE + why
  std::size_t index;           // position in Scene.routes
  std::string reason;          // human-readable
};

struct BridgeResult {
  std::size_t routesAdded = 0;
  std::vector<RouteError> rejected;   // empty == all routes valid
};

// Resolve Scene.routes (DEF names) to FieldAddress endpoints and populate ctx.
// Calls scene.resolveRoutes() first. Validates each route (see below); valid
// routes are added via ctx.addRoute(), invalid ones collected in result.rejected
// and NOT added. Never throws on a bad route — returns diagnostics.
BridgeResult buildRoutes(Scene &scene, X3DExecutionContext &ctx);

} // namespace x3d::runtime
```

**Validation (per route, at registration — not at delivery):**
Look up `FieldInfo` for `fromField` on the resolved `from` node and `toField` on
the resolved `to` node, then check, in order:

1. **Unresolved endpoint** — `from`/`to` `weak_ptr` is expired (unknown,
   forward-ref, or IMPORTed DEF). → *skip silently*, not an error (matches the
   serialization tolerance; record as `routesAdded` unaffected). Optionally
   surface as a soft note; do **not** add to `rejected`.
2. **Unknown field** — no `FieldInfo` with that `x3dName` on the node. → reject.
3. **Direction** — source must be readable-as-event (`OutputOnly` or
   `InputOutput`); sink must be writable-as-event (`InputOnly` or
   `InputOutput`). Reject routing *from* `InputOnly`/`InitializeOnly` or *to*
   `OutputOnly`/`InitializeOnly`.
4. **Type compatibility** — `from.type == to.type` (`X3DFieldType`). X3D does no
   implicit field-type coercion across a ROUTE, so require an exact tag match.
   (Enum fields: both `SFEnum`/`MFEnum` *and* same underlying — defer strict
   enum-class identity; exact tag match is sufficient for M1.)

Endpoint addresses use the resolved node's raw pointer (`weak_ptr::lock().get()`)
+ `x3dName`. The bridge does **not** retain the `shared_ptr`; the `Scene` owns
node lifetime, the `X3DExecutionContext` only observes (consistent with
`FieldAddress` being a non-owning pointer).

**Convenience:** add `X3DExecutionContext::buildFrom(Scene&)` thin wrapper that
calls `buildRoutes` and returns the `BridgeResult`, so a browser drives a loaded
document in one call.

---

## 2. `System` abstraction (generalizing `ActiveNode`)

`ActiveNode::update(now, ctx)` is per-node and per-tick only. Generalize to a
**`System`** that owns a *collection* of attached nodes and supports both the
time-driven (per-tick) and event-driven (handler-registration) styles, enabling
data-oriented batching at the hot path (Day-2 decision).

New header **`runtime/events/X3DSystem.hpp`**:

```cpp
namespace x3d::runtime {

// A behavior family operating over a collection of nodes of one kind.
// Time-driven systems do work in update(); event-driven systems wire handlers
// in attach() and leave update() a no-op. A System is the unit a browser
// registers with the context.
class System {
public:
  virtual ~System() = default;

  // Called once when a node is enrolled in this system (after route build).
  // Event-driven systems register their inputOnly handler here.
  virtual void attach(X3DNode *node, X3DExecutionContext &ctx) = 0;

  // Called every tick for time-driven systems; default no-op for event-driven.
  virtual void update(double now, X3DExecutionContext &ctx) { (void)now; (void)ctx; }
};

} // namespace x3d::runtime
```

- `X3DExecutionContext` gains `addSystem(std::shared_ptr<System>)` and, in
  `tick()`, calls `update(now, *this)` on each system *before* draining the
  cascade (replacing/augmenting the existing `active_` loop). Keep
  `addActiveNode` as a deprecated thin adapter (`ActiveNode` → a one-node
  `System`) so existing `cascade_test`/`animation_test` keep passing
  (behavior-preserving port, per Day 2).
- **Batching seam (not required for M1 correctness):** a System may store its
  nodes contiguously and iterate them in `update()`; this is where future
  data-oriented optimization lands without touching the node model.

**Interpolation helper** — factor the shared `key`/`keyValue` lookup out of
`PositionInterpolatorBehavior` into **`runtime/events/Interpolation.hpp`**:

```cpp
// Returns (lo, hi, t) for a fraction over a monotonic key[]: indices bracketing
// `fraction` and the in-segment parameter t in [0,1]. Clamps at both ends.
// Empty/singleton keys handled by the caller. Reused by every interpolator.
struct KeySpan { std::size_t lo, hi; float t; bool clamped; };
KeySpan locateKeySpan(const MFFloat &key, float fraction);
```

Each interpolator supplies only its per-component `lerp(a, b, t)`; the span
lookup is shared.

---

## 3. Interpolator family (event-driven systems)

One header per interpolator under `runtime/events/`, each a `System` whose
`attach()` registers the node's `setOnSet_fractionHandler` to compute a value and
`ctx.postEvent(node, "value_changed", value)`. All reuse `locateKeySpan`. Port
`PositionInterpolatorBehavior` onto `System` as the template.

| Node | `keyValue` type | Output | Interpolation |
|---|---|---|---|
| `ScalarInterpolator` | `MFFloat` | `SFFloat` | linear |
| `PositionInterpolator` | `MFVec3f` | `SFVec3f` | linear per-component (exists) |
| `PositionInterpolator2D` | `MFVec2f` | `SFVec2f` | linear per-component |
| `ColorInterpolator` | `MFColor` | `SFColor` | **HSV** lerp (RGB→HSV, lerp, HSV→RGB; shortest hue arc) |
| `OrientationInterpolator` | `MFRotation` | `SFRotation` | **SLERP** of axis-angle as quaternions |
| `CoordinateInterpolator` | `MFVec3f` | `MFVec3f` | per-point linear; `keyValue` is N·numPoints flat |
| `CoordinateInterpolator2D` | `MFVec2f` | `MFVec2f` | per-point linear, flat |
| `NormalInterpolator` | `MFVec3f` | `MFVec3f` | per-point SLERP-on-sphere then renormalize |

**Math notes (spec-correctness):**
- **ColorInterpolator** interpolates in **HSV**, not RGB (X3D
  Interpolation 19775-1). Convert each keyValue RGB→HSV once; lerp H along the
  shorter arc; convert back. Provide `rgbToHsv`/`hsvToRgb` in
  `Interpolation.hpp`.
- **OrientationInterpolator** is **SLERP** between unit quaternions built from
  axis-angle, choosing the shorter path (negate one quat if `dot < 0`); fall
  back to NLERP when `sin(theta)≈0`. Result re-expressed as axis-angle
  `SFRotation`.
- **Coordinate/Normal** interpolators output `MFVec3f`: the flat `keyValue` is
  reshaped into `numKeys` rows of `numPoints`; each output point interpolates
  between the bracketing rows. Normals interpolate on the unit sphere
  (slerp-direction) and are renormalized.
- Degenerate inputs (empty/mismatched `key`/`keyValue`, single key) yield a safe
  constant (front value or zero), never throw.

Each interpolator gets a focused cascade test:
`tick`/`postEvent` a fraction at 0, 0.5, 1, and one off-key value; assert the
emitted `value_changed` reaches a sink. One multi-interpolator scene test
(Day 3) exercises several driven by one TimeSensor.

---

## 4. TimeSensor / `X3DTimeDependentNode` lifecycle

Put the reusable lifecycle in **`runtime/events/X3DTimeDependentSystem.hpp`** (a
base `System`); `TimeSensorSystem` derives from it (audio/movie inherit later).
Replaces the minimal `TimeSensorBehavior` (keep it as a thin compat shim or
delete after porting `animation_test`).

**State machine** (per node, held in the system keyed by node pointer — never on
the node):

- **Activation.** Become active when `enabled` and the clock crosses an effective
  start: `startTime <= now` and (`stopTime <= startTime` *or* `now < stopTime`).
  On activation emit `isActive = true`, set `cycleTime = now` (cycle start),
  record activation time.
- **Loop / cycle.** `frac = (now - startTime) / cycleInterval` (guard
  `cycleInterval <= 0` → treat as 1 for fraction, but a non-looping sensor with
  `cycleInterval <= 0` deactivates immediately after one tick). If `loop`: wrap
  to `[0,1)`, emit `cycleTime` at each wrap boundary. If not `loop`: clamp to
  `[0,1]` and **deactivate** when `now - startTime >= cycleInterval` (emit final
  `fraction_changed = 1`, then `isActive = false`).
- **stopTime gating.** If `0 < stopTime <= now` while active → deactivate
  (clamp/emit final fraction first).
- **pause / resume.** When `pauseTime` is reached (and `> resumeTime`): emit
  `isPaused = true`, freeze elapsed accumulation. On `resumeTime` (and active):
  emit `isPaused = false`, shift the time base so elapsed time excludes the
  paused span.
- **Per-tick outputs** while active and not paused: `time = now`,
  `fraction_changed = frac`, `elapsedTime = active+unpaused seconds`. All emitted
  via `ctx.postEvent(node, "<field>", value)` using the exact `x3dName`s
  (`fraction_changed`, `time`, `cycleTime`, `elapsedTime`, `isActive`,
  `isPaused`).
- **Change events.** Emit an output only when its value actually changes
  (`isActive`/`isPaused`/`cycleTime` are edge-triggered; `time`/`fraction`/
  `elapsedTime` continuous while running).

Reads happen through generated getters (`getEnabled`, `getLoop`, `getStartTime`,
`getStopTime`, `getCycleInterval`, `getPauseTime`, `getResumeTime`); all writes
go through the cascade so downstream ROUTEs fire in the same timestamp.

**Tests** (`runtime/events/tests/timesensor_test.cpp`): start gating;
single-shot completion + deactivation; looping wrap + `cycleTime` pulses;
`stopTime` early stop; pause/resume elapsed-time correctness; `enabled=false`
emits nothing.

---

## File summary

New, header-only, `namespace x3d::runtime`, under `runtime/events/`:

- `X3DSceneBridge.hpp` — `buildRoutes(Scene&, ctx)` + validation + `BridgeResult`.
- `X3DSystem.hpp` — `System` base; context `addSystem`/`tick` integration.
- `Interpolation.hpp` — `locateKeySpan`, `rgbToHsv`/`hsvToRgb`, quaternion SLERP.
- `ScalarInterpolatorSystem.hpp`, `ColorInterpolatorSystem.hpp`,
  `OrientationInterpolatorSystem.hpp`, `CoordinateInterpolatorSystem.hpp`
  (+ `2D`), `NormalInterpolatorSystem.hpp`, `PositionInterpolator2DSystem.hpp`;
  port `PositionInterpolatorBehavior` → `PositionInterpolatorSystem`.
- `X3DTimeDependentSystem.hpp`, `TimeSensorSystem.hpp`.

Tests under `runtime/events/tests/`: `scene_bridge_test.cpp`,
per-interpolator tests, `timesensor_test.cpp`, one multi-interpolator scene test.
Wire all into CTest; **no codegen / no golden regeneration** expected.

## Error handling (consistent with the foundation)

- Bad ROUTE (unknown field / wrong direction / type mismatch) → rejected at
  bridge time with a `RouteError`, never added, never a mid-cascade `any_cast`
  throw. Unresolved endpoints → skipped, not errors.
- Degenerate interpolator inputs → safe constant, no throw.
- Range-checked sink setters may `throw std::out_of_range` during the cascade;
  the engine lets it propagate (a bad event value is a program error).

## Out of scope (per roadmap)

Followers (Damper/Chaser), sequencers/triggers, sensors needing a
renderer/picking, Script/SAI, PROTO IS-event runtime mapping, media decode. Each
is a later sub-project layered on this systems-seam.
