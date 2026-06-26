# Followers Component Runtime (Chasers + Dampers) — Design

**Date:** 2026-06-21
**Status:** Approved (brainstorm) → ready for implementation plan
**Closes:** the Followers component behavioral gap — 14 currently-inert nodes
(ColorChaser/Damper, CoordinateChaser/Damper, OrientationChaser/Damper,
PositionChaser/Damper, PositionChaser2D/Damper2D, ScalarChaser/Damper,
TexCoordChaser2D/Damper2D) get a runtime. Closes the audited FOL-* findings.

## 1. Problem

The 14 Followers nodes parse and extract but have **no System wired**, so they are
behaviorally inert: routing `set_destination` into a follower produces no
`value_changed` events. Followers smoothly animate a value toward a destination over
time — the standard way to create eased transitions (camera moves, UI, object motion)
without authoring interpolator keyframes. This adds the runtime that makes them behave.

## 2. Feasibility (de-risked by a throwaway math spike)

A dependency-free C++ spike validated both algorithm cores against the spec's stated
properties:

- **Damper** (order 1, τ=0.3, step 0→1): output = **0.6321 at t=τ** — exactly the spec's
  `1 − 1/e` (63.2%, §39.3.2) — then asymptotes to 1. Order 3 is visibly smoother/slower
  (correct higher-order shape).
- **Chaser** (D=1, step 0→1 at t=0): reaches the destination **exactly at t=1.0** and
  holds. Overlap case (→1 at t=0, →0.5 at t=0.5) superposes the two ramps and lands at
  0.5 by t=1.5 (= T_last + D).

So **Damper = a cascade of IIR low-pass filters** (`α = e^(−dt/τ)`); **Chaser = an FIR
superposition of linear ramps**. The spike was discarded.

## 3. Spec grounding (ISO/IEC 19775-1 §39 Followers)

- **§39.3.2 X3DDamperNode:** IIR. Internally `order` (`[0..5]`, default 3) cascaded
  first-order filters; the input of filter 1 is `set_destination`, each subsequent
  filter's input is the previous filter's output, the last filter's output is
  `value_changed`. `tau` (`[0,∞)`, default 0.3) is the time constant: a filter reaches
  63% (`1 − 1/e`) of an input step in `tau` seconds. `order=0` **or** `tau=0` →
  destination forwarded directly. `tolerance` (`−1 or [0,∞)`, default −1): when `−1` the
  browser picks a tolerance (use **0.001**); the transition ends and `isActive` goes
  FALSE only when **all** internal filters are within `tolerance` of their input. The
  end-of-transition test runs **before** computing the new output; if within tolerance,
  the destination is assigned exactly.
- **§39.3.1 X3DChaserNode:** FIR. Each `set_destination` event creates a transition from
  the previous destination to the new one; all transitions sum to form `value_changed`
  (Equations 1–3). The transition completes `duration` (`[0,∞)`, default 1) seconds after
  the last `set_destination`. Events older than `duration` can be discarded without
  changing the output (bounded buffer).
- **Both base types (X3DFollowerNode):** `set_value` stops any transition in progress and
  jumps the output directly to the given value. `initialValue` seeds the output;
  `initialDestination` seeds the destination (an initial transition runs at load only if
  they differ).
- **Per-type value:** the node's value type is `SFFloat` (Scalar), `SFVec3f` (Position),
  `SFVec2f` (Position2D), `SFColor` (Color), `SFRotation` (Orientation), `MFVec3f`
  (Coordinate), `MFVec2f` (TexCoord2D). The chaser prose notes all algorithm variables
  are floating-point **except** the value-typed `d_n`, `A_n(t)`, `O(t)`.

## 4. Architecture

Mirrors the proven `InterpolatorSystem` pattern: templated `System` subclasses
constructed with per-type ops, aggregated by a `makeFollowerSystems()` factory in a new
`FollowerRegistration.hpp`, wired by `X3DSceneBridge` alongside the interpolator systems.

Unlike interpolators (stateless, purely event-driven on `set_fraction`), followers are
**stateful + time-driven**. The `System` base already provides both hooks:
`attach(node, ctx)` and `update(double now, ctx)`.

### 4a. Per-type ops trait — `FollowerArith<T>` (`runtime/events/FollowerArith.hpp`)

A small trait providing exactly what the two algorithms need, specialized per value type:

- `static T lerp(const T& a, const T& b, float t)` — the damper filter step
  (`out = lerp(in, prev, α)`) and the chaser ramp.
- `static float dist(const T& a, const T& b)` — magnitude of the difference, for the
  damper tolerance test.

Specializations:
- **SFFloat / SFVec2f / SFVec3f / SFColor** — component-wise linear lerp; `dist` =
  Euclidean (per-component max for the tolerance comparison is acceptable; use the
  max-abs-component to match "all filters within tolerance").
- **SFRotation** — `lerp` = `slerpRotation(a, b, t)` (reuse `Interpolation.hpp`); `dist`
  = the rotation angle between `a` and `b` (via the existing `Quat` helpers).
- **MFVec3f / MFVec2f** — element-wise lerp over the array (handle differing lengths by
  operating up to the destination's length; output adopts the destination length);
  `dist` = max element distance.

### 4b. Damper system — `DamperSystem<NodeT, ValueT>` (`runtime/events/FollowerSystem.hpp`)

State per attached node: `std::vector<ValueT> filters` (size `order+1`), `ValueT
destination`, `bool active`, `double lastTick`. On `update(now)`:
```
dt = now - lastTick; lastTick = now
if (order == 0 || tau == 0) { out = destination; emit; active = (dist(out,prevOut) > tol); return }
// end-of-transition test FIRST (§39.3.2): if every filter within tol of its input, snap + deactivate
α = exp(-dt / tau)
filters[0] = destination
for i in 1..order: filters[i] = lerp(filters[i-1], filters[i], α)   // in_i = current prev-filter output
out = filters[order]
emitValue_changed(out); ctx.postEvent(node, "value_changed", out)
```
`set_destination` handler: set `destination`, set `active=true`, emit `isActive` TRUE on
the rising edge. `set_value` handler: set every filter to the value, `active=false`,
emit the value once + `isActive` FALSE (jump). Seed `filters` from `initialValue` and
`destination` from `initialDestination` at attach; if they differ, start active.

### 4c. Chaser system — `ChaserSystem<NodeT, ValueT>` (`runtime/events/FollowerSystem.hpp`)

State: the output base + a bounded buffer of `(T_n, deltaToward)` transitions within
`duration`, `destination`, `active`, `lastTick`. Output = the FIR superposition
(spike form): `O(t) = base + Σ rampScale_n · (toward latest)`, where each event's ramp
goes 0→1 over `duration`. For vector-space types this is the linear-ramp sum; for
**SFRotation**, compose the per-event rotational deltas via `slerpRotation` (chain the
buffered destinations by their ramp fractions) — the one non-uniform path. Transition
ends (and `isActive` FALSE) once `now ≥ lastEventTime + duration`. `set_value` jumps
(clear buffer, base = value). `duration = 0` → forward `destination` directly.

### 4d. Registration + wiring (`runtime/events/FollowerRegistration.hpp`)

```cpp
inline std::vector<std::shared_ptr<System>> makeFollowerSystems() {
  std::vector<std::shared_ptr<System>> s;
  s.push_back(std::make_shared<DamperSystem<PositionDamper, SFVec3f>>());
  s.push_back(std::make_shared<ChaserSystem<PositionChaser, SFVec3f>>());
  // ... all 14 (7 types × 2 families); MF types use the Multi variants
  return s;
}
```
`X3DSceneBridge` calls `makeFollowerSystems()` and registers them next to
`makeInterpolatorSystems()` (add-only; no behavior change to existing systems).

## 5. Testing (ungated — pure runtime, no engine)

New `runtime/events/tests/follower_conformance_test.cpp` (registered in CMake), driving
attached nodes via their `set_destination`/`set_value` handlers + stepped `update(now)`:

1. **Damper step response:** order-1, τ=0.3, step 0→1 → output ≈ 0.632 at t=τ (±tol),
   monotonic, asymptotes toward 1.
2. **Damper order/tau passthrough:** `order=0` and `τ=0` forward the destination
   immediately.
3. **Damper tolerance / isActive:** with a finite tolerance the node emits `isActive`
   FALSE and snaps to destination once within tolerance; `set_value` jumps + deactivates.
4. **Chaser reaches at duration:** D=1, step 0→1 → output reaches 1.0 (±ε) exactly at
   t=1.0 and holds; before that it's strictly between.
5. **Chaser overlap + set_value:** overlapping destinations superpose (spike values);
   `set_value` jumps and clears the transition.
6. **Per-type coverage:** SFVec3f, SFRotation (slerp reaches the target orientation, stays
   unit), SFColor, SFVec2f, MFVec3f + MFVec2f (element-wise; per-element convergence —
   closes FOL-9), SFFloat.
7. **Determinism:** identical `(initial, destination, dt-sequence)` → value-identical
   output across two runs.
8. **Wiring:** a follower attached through the registration emits `value_changed` over
   `update` ticks after a `set_destination` (was inert before).

**CLI golden:** `sim-damper.x3d` — a `PositionDamper` whose `value_changed` routes to a
`Transform.translation`; `set_destination` set once; `x3d sim … --watch X.translation
--json` golden captured after the conformance test verifies the ease. Demonstrates the
through-pipeline behavior.

**Regression:** full suite stays green; existing goldens byte-identical (followers were
inert, so no existing trace changes); `mise run conformance-gate` passes after regen.

## 6. Files touched

- `runtime/events/FollowerArith.hpp` — per-type `lerp`/`dist` trait + specializations.
- `runtime/events/FollowerSystem.hpp` — `DamperSystem`/`ChaserSystem` (+ Multi variants).
- `runtime/events/FollowerRegistration.hpp` — `makeFollowerSystems()`.
- `runtime/events/X3DSceneBridge.hpp` (or wherever `makeInterpolatorSystems()` is wired) —
  register the follower systems.
- `runtime/events/tests/follower_conformance_test.cpp` + `CMakeLists.txt` registration.
- `tools/x3d-cli/fixtures/sim-damper.x3d`, `tools/x3d-cli/goldens/sim-damper.trace.json`,
  `tools/tests/x3d_cli_test.sh` — CLI golden.
- `docs/conformance/findings.yaml` (FOL-* → fixed) + regen (`mise run conformance`);
  `docs/superpowers/BACKLOG.md` — record the closure.

## 7. Scope guardrails (YAGNI)

- All 14 follower nodes, both families; pure value-follower behavior only.
- Reuse existing `slerpRotation`/`Quat` helpers — do not reimplement rotation math.
- Tolerance default for `−1` is **0.001** (spec-permitted simple choice).
- No `MovieTexture`/audio-driven sources, no rendering, no closed-loop control.

## 8. Out of scope (deferred)

Time-lifecycle/media sources (TDN-5, CONF-SOUND); any follower fields beyond the §39
base + per-type value (there are none beyond metadata); GPU/consumer-side smoothing.
