---
title: Followers System
summary: §39 Followers component runtime — Damper (IIR cascade) and Chaser (re-basing ramp) smoothing of field values, all 14 node types.
tags: [subsystem, followers, damper, chaser, animation]
updated: 2026-06-21
related:
  - ../architecture.md
  - system-interpolators.md
  - event-cascade.md
---

# Followers System

## Purpose

The Followers System animates scene-graph field values by *smoothing* a stream of `set_destination` events toward their target, rather than sampling a keyframe table the way the [Interpolator System](system-interpolators.md) does. It covers all fourteen nodes of ISO/IEC 19775-1 §39 (Followers component) — seven value types, each as a **Damper** (a critically-shaped IIR low-pass) and a **Chaser** (a fixed-duration re-basing ramp). A follower has no `key`/`keyValue`; it holds an output and chases a moving `set_destination`, emitting `value_changed` each tick while in transition and toggling `isActive` at the transition boundaries.

This subsystem mirrors the shape of the Interpolator System: a small arithmetic layer (`FollowerArith`) plus two generic `System`-derived templates wired into node handlers at scene-construction time by an `attachFollowers` scene-walk.

## Key files

| File | Role |
|---|---|
| `runtime/events/FollowerArith.hpp` | Per-type arithmetic the two systems are generic over: `FollowerArith<T>::lerp` / `dist` / `reshapeLike` for `float`, `SFVec2f`, `SFVec3f`, `SFColor`, and `SFRotation` (rotation `lerp` is `slerpRotation`; `dist` is the geodesic angle). `MFFollowerArith<Elem>` provides element-wise `lerp`/`dist` and the length-reconciling `reshapeLike` for the `MFVec3f`/`MFVec2f` (Coordinate/TexCoord) types. |
| `runtime/events/FollowerSystem.hpp` | Two generic `System` templates: `DamperSystem<NodeT,ValueT>` (§39.3.2 IIR damper cascade) and `ChaserSystem<NodeT,ValueT>` (§39.3.1 re-basing linear ramp). Each installs `set_destination` / `set_value` handlers on attach and drives outputs in `update(now, ctx)`. |
| `runtime/events/FollowerRegistration.hpp` | `makeFollowerSystems()` — the single source of truth for the 14-system list (7 types × Damper + Chaser). `attachFollowers(scene, ctx)` (defined in `runtime/events/X3DSceneBridge.hpp`) is the production caller that walks the scene and offers every node to each system's `attach`. |

## Interfaces and seams

### Exposed interface

The public entry point for embedders is `attachFollowers`, declared in `runtime/events/X3DSceneBridge.hpp` and called right after `attachInterpolators` in the production tick setup (`tools/x3d-cli/sim_runtime.hpp`):

```cpp
// Call after ctx.buildSceneGraph(scene) to wire all followers.
void attachFollowers(Scene &scene, X3DExecutionContext &ctx);
```

Both systems are subclasses of `System` from `runtime/events/X3DSystem.hpp` (the same base the Interpolator/EventUtility systems use). The 14 instances built by `makeFollowerSystems()`:

```cpp
// runtime/events/FollowerRegistration.hpp
DamperSystem<ScalarDamper, float>          ChaserSystem<ScalarChaser, float>
DamperSystem<PositionDamper, SFVec3f>      ChaserSystem<PositionChaser, SFVec3f>
DamperSystem<PositionDamper2D, SFVec2f>    ChaserSystem<PositionChaser2D, SFVec2f>
DamperSystem<ColorDamper, SFColor>         ChaserSystem<ColorChaser, SFColor>
DamperSystem<OrientationDamper, SFRotation> ChaserSystem<OrientationChaser, SFRotation>
DamperSystem<CoordinateDamper, MFVec3f>    ChaserSystem<CoordinateChaser, MFVec3f>
DamperSystem<TexCoordDamper2D, MFVec2f>    ChaserSystem<TexCoordChaser2D, MFVec2f>
```

### Seam points

- **`set_destination` / `set_value` handler registration** — `attach(node, ctx)` installs `setOnSet_destinationHandler` and `setOnSet_valueHandler` lambdas (matched by `dynamic_cast<NodeT*>`, so the scene-walk is safe to offer every node). `set_destination` updates the target and activates the transition; `set_value` snaps the output and stops it.
- **Dual write on output** — each emission both calls the node's own `emitValue_changed` / `emitIsActive` setter **and** `ctx.postEvent(node, "value_changed"|"isActive", …)` to seed the [event cascade](event-cascade.md), so downstream ROUTEs fire. This matches the Interpolator System's `postEvent` path.
- **`attachFollowers` scene-walk** — mirrors `attachInterpolators`; `makeFollowerSystems()` is the registry, `attachFollowers` in `X3DSceneBridge.hpp` is the production wiring.

### Spec-conformance annotations

- **Damper (§39.3.2)** — `DamperSystem` models a cascade of `order` first-order IIR lerp-filters, each driven with `α = exp(-dt/τ)`. `order==0` or `τ<=0` is a passthrough (immediate snap to destination, then `isActive=false`). Convergence is tested **before** the update pass (the end-of-transition snap fires before the next filter step) using `FollowerArith<ValueT>::dist` against the per-filter `tolerance` (a negative `tolerance` uses the `0.001` default per the spec). `set_value` resets all filter states and stops the transition.
- **Chaser (§39.3.1)** — `ChaserSystem` is a re-basing linear ramp: on each `set_destination` the start re-bases to the *current output* at event time and the destination becomes the new value, so `O(t) = lerp(start, dest, clamp((now − startTime) / duration, 0, 1))`. At `t = startTime + duration` the output equals the destination and the transition ends. `duration ≤ 0` snaps immediately.
- **Initial transition** — if `initialValue ≠ initialDestination` at attach time, the follower starts active and emits `isActive=TRUE` immediately (§39 requires `isActive` TRUE when a transition begins).
- **MF length reconciliation (FOL overlap fix)** — for the `MFVec3f`/`MFVec2f` (Coordinate/TexCoord) followers, `reshapeLike` broadcasts a length-1 seed up to the destination's length so the damper cascade / chaser ramp can converge element-wise rather than stalling on a length mismatch.
- **FOL-OVL (deferred)** — the Chaser ships a single-transition re-basing lerp (uniform across types, including `SFRotation` slerp). Literal §39 FIR-superposition fidelity for *overlapping* `set_destination` events within one `duration` is deferred; see `docs/conformance/components/Followers.md`.

## How it is tested

- `ctest --preset dev -R x3d_events_tests` (doctest case: `follower_conformance_test`) — behavioral-conformance tests (`runtime/events/tests/follower_conformance_test.cpp`). Covers `FollowerArith` ops (incl. `SFRotation` slerp + geodesic `dist`), `DamperSystem` IIR convergence and passthrough, `ChaserSystem` re-basing ramp and exact end-of-transition value, the initial-active `isActive` emission, MF element-wise reconciliation, and `attachFollowers` production wiring.

## Related specs and ADRs

- Spec: ISO/IEC 19775-1 §39 (Followers component) — normative source for the §39.3.1 chaser ramp and §39.3.2 damper IIR cascade.
- Conformance view: `docs/conformance/components/Followers.md` — `FOL-*` finding records (Followers runtime shipped; `FOL-OVL` overlapping-transition fidelity deferred).
- Conformance findings source of truth: `docs/conformance/findings.yaml`.
- [Interpolator System](system-interpolators.md) — the sibling animation subsystem this one mirrors (registry + `attach…` scene-walk + `postEvent`).
- [Event Cascade subsystem](event-cascade.md) — delivers the `value_changed` / `isActive` events the follower handlers post.
