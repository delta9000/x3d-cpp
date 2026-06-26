---
title: Interpolator System
summary: Linear and spline interpolator systems for animating field values driven by TimeSensor fractions.
tags: [subsystem, interpolators, animation, spline, linear]
updated: 2026-06-20
related:
  - ../architecture.md
  - system-time.md
  - event-cascade.md
---

# Interpolator System

## Purpose

The Interpolator System animates scene-graph field values by responding to `set_fraction` events (typically routed from a TimeSensor) and emitting `value_changed` outputs through the event cascade. It covers all thirteen nodes in ISO/IEC 19775-1 §19 (Interpolation component): eight linear interpolators, three Hermite spline interpolators, one Squad orientation interpolator, and one EaseInEaseOut fraction modifier. The subsystem owns the math layer (key-span lookup, per-type lerp functions, Hermite/Squad/ease algorithms) and the `System`-derived behavior wiring that installs those math functions into node handlers at scene construction time.

## Key files

| File | Role |
|---|---|
| `runtime/events/Interpolation.hpp` | Shared linear-interpolation math: `KeySpan`/`locateKeySpan`, scalar/vector lerps (`lerpf`, `lerpVec2`, `lerpVec3`), HSV color conversion (`rgbToHsv`/`hsvToRgb`/`lerpColorHsv`), quaternion SLERP (`Quat`, `slerp`, `slerpRotation`, `slerpNormal`), and the generic dispatch templates `interpolateValue` and `interpolateMulti`. |
| `runtime/events/SplineInterpolation.hpp` | Non-linear math: §19.2.4 Hermite spline (`hermiteSpline<T>`), §19.4.13 Squad quaternion interpolation (`squadOrientation`), §19.4.4 ease-in/ease-out fraction modifier (`easeInEaseOut`), and quaternion algebra helpers (`quatMul`, `quatConj`, `quatLog`, `quatExp`, `squadIntermediate`). |
| `runtime/events/InterpolatorSystem.hpp` | Two generic `System` subclasses: `InterpolatorSystem<NodeT,ValueT>` (single-value family) and `MultiInterpolatorSystem<NodeT,ElemT>` (multi-value / flat keyValue family). Both register a `set_fraction` handler that calls `interpolateValue` or `interpolateMulti` and posts `value_changed` via `X3DExecutionContext::postEvent`. |
| `runtime/events/SplineInterpolatorSystem.hpp` | Three concrete `System` subclasses for the non-linear family: `SplineInterpolatorSystem<NodeT,ValueT>` (Hermite spline), `SquadOrientationInterpolatorSystem`, and `EaseInEaseOutSystem` (emits `modifiedFraction_changed`). |
| `runtime/events/InterpolatorRegistration.hpp` | `makeInterpolatorSystems()` — the single source of truth for the complete 13-system list. `registerInterpolatorSystems(ctx)` add-registers them. `attachInterpolators(scene, ctx)` (the production caller, defined in `runtime/events/X3DSceneBridge.hpp`) walks the scene and calls each system's `attach` on every node. |

## Interfaces and seams

### Exposed interface

The public entry point for embedders is `attachInterpolators`, declared in `runtime/events/X3DSceneBridge.hpp`:

```cpp
// Call after ctx.buildSceneGraph(scene) to wire all interpolators.
void attachInterpolators(Scene &scene, X3DExecutionContext &ctx);
```

Each system type is a subclass of `System` from `runtime/events/X3DSystem.hpp`:

```cpp
class System {
public:
  virtual ~System() = default;
  virtual void attach(X3DNode *node, X3DExecutionContext &ctx) = 0;
  virtual void update(double now, X3DExecutionContext &ctx) {}  // default no-op
};
```

The two generic templates cover the full linear family:

```cpp
// Single-value: keyValue is std::vector<ValueT>, output is ValueT.
template <typename NodeT, typename ValueT>
class InterpolatorSystem : public System {
public:
  using LerpFn = std::function<ValueT(const ValueT &, const ValueT &, float)>;
  explicit InterpolatorSystem(LerpFn lerp);
  void attach(X3DNode *node, X3DExecutionContext &ctx) override;
};

// Multi-value: keyValue is flat std::vector<ElemT>, reshaped by numKeys.
template <typename NodeT, typename ElemT>
class MultiInterpolatorSystem : public System {
public:
  using LerpFn = std::function<ElemT(const ElemT &, const ElemT &, float)>;
  explicit MultiInterpolatorSystem(LerpFn lerp);
  void attach(X3DNode *node, X3DExecutionContext &ctx) override;
};
```

The math layer is header-only and callable directly for unit testing:

```cpp
// Key-span lookup (Interpolation.hpp).
KeySpan locateKeySpan(const MFFloat &key, float fraction);

// Generic dispatch (Interpolation.hpp).
template <typename T, typename Lerp>
T interpolateValue(const MFFloat &key, const std::vector<T> &keyValue,
                   float fraction, Lerp lerp);

template <typename T, typename Lerp>
std::vector<T> interpolateMulti(const MFFloat &key,
                                const std::vector<T> &keyValue,
                                float fraction, Lerp lerp);

// Hermite spline (SplineInterpolation.hpp).
template <typename T>
T hermiteSpline(const MFFloat &key, const std::vector<T> &keyValue,
                const std::vector<T> &keyVelocity, bool closed,
                bool normalizeVelocity, float fraction);

// Squad (SplineInterpolation.hpp).
SFRotation squadOrientation(const MFFloat &key,
                            const std::vector<SFRotation> &keyValue,
                            float fraction);

// EaseInEaseOut (SplineInterpolation.hpp).
float easeInEaseOut(const MFFloat &key, const std::vector<SFVec2f> &eieo,
                    float fraction);
```

### Seam points

- **`set_fraction` handler registration** — `attach(node, ctx)` installs a `setOnSet_fractionHandler` lambda on the matched node. The lambda reads the node's `key`/`keyValue` fields, calls the math function, then calls `ctx.postEvent(node, "value_changed", std::any(...))`. The [Event Cascade subsystem](event-cascade.md) delivers that event downstream through any ROUTEs.

- **`X3DExecutionContext` event queue** — the subsystem depends on `X3DExecutionContext::postEvent` and `ctx.process()` for event delivery. These are declared in `runtime/events/X3DExecutionContext.hpp`.

- **[TimeSensor System](system-time.md) as upstream source** — in production, a TimeSensor emits `fraction_changed` (an `SFFloat`), which is ROUTEd to an interpolator's `set_fraction`. The Interpolator System does not depend on TimeSensor directly; the ROUTE graph wires them.

- **`attachInterpolators` scene-walk** — `InterpolatorRegistration.hpp` provides `makeInterpolatorSystems()`. The production caller `attachInterpolators` in `runtime/events/X3DSceneBridge.hpp` offers every node in the scene to every system's `attach`; each system filters by `dynamic_cast<NodeT*>`, so the walk is safe to offer all nodes.

### Spec-conformance annotations

- **INTERP-02** (§19.3.1): an interpolator with an empty `key` field must emit no `value_changed` event. All systems carry a live empty-key guard (`if (interp->getKey().empty()) return;`) checked at event time, so a later non-empty `key` re-enables emission without re-attaching.

- **INTERP-01** (§19.2.4, §19.4.10-13): the Hermite Spline, Squad, and EaseInEaseOut nodes previously had no System; `set_fraction` fired a no-op generated default. `SplineInterpolatorSystem`, `SquadOrientationInterpolatorSystem`, and `EaseInEaseOutSystem` close this gap.

- **PIV-1**: `registerInterpolatorSystems` (add-only) was never wired to attach nodes; `attachInterpolators` (scene-walk + add) is the correct production call.

- **ColorInterpolator** interpolates in HSV, not RGB, lerping hue along the shorter arc (Foley & van Dam conversion). `lerpColorHsv` in `Interpolation.hpp`.

- **OrientationInterpolator** uses quaternion SLERP choosing the shorter path (negate quaternion when dot product < 0); falls back to NLERP when sin(ω) ≈ 0. Implemented as `slerpRotation` in `Interpolation.hpp`.

- **NormalInterpolator** uses spherical SLERP (`slerpNormal`), renormalizing the result.

- **Hermite spline** (`SplineInterpolation.hpp`): implements the §19.2.4 algorithm — Hermite basis (h00, h01, h10, h11), per-segment F+/F- non-uniform-interval tangent scaling, optional author `keyVelocity` (size 2 = endpoints only, size N = per-key), `normalizeVelocity` chord-length scaling, closed-loop key/value wrap, and open-curve zero-tangent endpoint overrides.

- **SquadOrientationInterpolator**: Shoemake Squad in quaternion space (§19.4.13). N=2 reduces to plain SLERP, matching `OrientationInterpolator`.

- **EaseInEaseOut** (§19.4.4): the ten-step algorithm; the eased local fraction is remapped to the global key domain so the output is directly usable as a downstream interpolator's `set_fraction`.

## How it is tested

- `ctest --preset dev -R x3d_event_interpolators` — per-type linear interpolator cascade tests (`runtime/events/tests/interpolator_test.cpp`). Covers all eight linear types: Scalar, Position, Position2D, Color (HSV midpoint), Orientation (SLERP angle), Coordinate (multi-point), CoordinateInterpolator2D, and Normal (unit-sphere SLERP). Each drives fractions at 0, 0.5, 1, and one off-key value; a final scene test fans one fraction to three interpolators simultaneously.

- `ctest --preset dev -R x3d_interpolator_conformance` — behavioral conformance tests (`runtime/events/tests/interpolator_conformance_test.cpp`). Closes INTERP-02 (empty-key guard: sentinel value held after `set_fraction`; re-enabled after non-empty key assigned), INTERP-01 (Hermite scalar 2-key and 3-key exact values; author `keyVelocity` endpoint form; SplinePositionInterpolator component values; Squad N=2 reduces to SLERP; EaseInEaseOut three piecewise regions; S>1 rescaling path), and PIV-1 (`attachInterpolators` wires ScalarInterpolator and SplinePositionInterpolator via scene-walk, no manual per-node attach). All expected values are hand-computed from the normative §19.2.4 Hermite basis and §19.4.4 algorithm.

## Related specs and ADRs

- Spec: ISO/IEC 19775-1 §19 (Interpolation component) — normative source for `locateKeySpan`, Hermite basis, ColorInterpolator HSV, OrientationInterpolator SLERP, Squad, and EaseInEaseOut algorithms.
- Conformance view: `docs/conformance/components/Interpolation.md` — INTERP-01, INTERP-02, PIV-1 finding records (all CLOSED at commit `07c31ca`).
- Conformance findings source of truth: `docs/conformance/findings.yaml`.
- [Event Cascade subsystem](event-cascade.md) — delivers `value_changed` events posted by the interpolator handlers.
- [TimeSensor System](system-time.md) — produces `fraction_changed` that ROUTEs into `set_fraction`.
