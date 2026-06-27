---
title: View-Dependent System
summary: LOD level selection, ProximitySensor/VisibilitySensor/TransformSensor events, and Billboard view-facing rotation — the subsystem that couples the scene graph to the bound viewer each tick.
tags: [subsystem, view-dependent, lod, visibility, transform-sensor, billboard, sensors]
updated: 2026-06-22
related:
  - ../architecture.md
  - ../subsystems/system-viewpointbind.md
  - ../subsystems/dirty-bounds-transform.md
  - ../subsystems/extract.md
  - ../subsystems/execution-context.md
  - ../subsystems/sensors.md
---

# View-Dependent System

The View-Dependent System is the runtime subsystem that re-evaluates every scene-graph node whose correct behavior depends on the current viewer pose. It runs once per tick from the bound `Viewpoint` and is responsible for three distinct behaviors:

1. **LOD level selection** — computes eye-to-center distance in each LOD node's local frame, applies the §23.4.3 step function, clamps to the child count, and fires `level_changed` on transitions.
2. **Environmental sensor edge detection** — evaluates `ProximitySensor` (viewer-in-box, §22.4.1), `VisibilitySensor` (box-in-cone, §22.4.3), and `TransformSensor` (targetObject-AABB-in-sensor-box, §22.4.2); fires `isActive`/`enterTime`/`exitTime` on enter and exit edges and `position_changed`/`orientation_changed` while inside (Proximity + Transform). Change-gated (ENV-04: emit only on actual change) and disable-deactivates (ENV-07: a disabled active sensor fires exit). **Gap:** sensors reachable only through a non-selected `Switch` child or inactive `LOD` level are still evaluated — per ADR-0034 they should be treated as removed from the hierarchy (`SENSOR-SWITCH`, open).
3. **Billboard rotation math** — a standalone, header-only helper that computes the local rotation matrix that faces a Billboard node toward the viewer; consumed directly by `SceneExtractor` and `PickSystem` during their per-path walks.

The split between tick-time (LOD events, sensor edges) and render-time (Billboard rotation, LOD child selection for rendering) is intentional and documented: the `ViewDependentSystem` class owns the event side; the free function `billboardLocalMatrix` and the inline `lodSelectLevel` are used at extract/pick time. This avoids a dependency cycle between `SceneExtractor`, `PickSystem`, and the execution context.

## Key files

| File / directory | Role |
|---|---|
| `runtime/scene/ViewDependentSystem.hpp` | `ViewDependentSystem` class (a `System`): `attach`, `update`, sensor/LOD tracking, observer seams |
| `runtime/scene/Billboard.hpp` | `billboardLocalMatrix` free function (§23.4.1) + `viewdep::` math helpers (`sub`, `dot`, `cross`, `len`, `norm`); no `X3DExecutionContext` dependency |
| `runtime/events/X3DSceneBridge.hpp` | `attachViewDependent(Scene&, X3DExecutionContext&)` — production wiring: scene walk + per-node `attach` call |
| `runtime/scene/tests/view_dependent_test.cpp` | Full unit test suite (24 test functions, one `main`); doctest case `view_dependent_test` in the `x3d_geometry_scene` target |

## Interfaces and seams

### Exposed interface

`ViewDependentSystem` is a `System` (from `runtime/events/X3DSystem.hpp`) registered via `ctx.addSystem(...)`.

```cpp
// runtime/scene/ViewDependentSystem.hpp
class ViewDependentSystem : public System {
public:
  // Called once per node during scene setup. Registers LOD, ProximitySensor,
  // VisibilitySensor, and TransformSensor nodes; silently ignores everything else.
  void attach(X3DNode *node, X3DExecutionContext &ctx) override;

  // Called every frame (ctx.tick -> System::update). Reads cameraWorldPosition()
  // and viewMatrix() from ctx; posts events via ctx.postEvent().
  void update(double now, X3DExecutionContext &ctx) override;

  // Test/observer seam: fired when an LOD's announced level changes.
  void setLevelChangedHook(std::function<void(X3DNode *, int)> h);

  // Test/observer seam: fired on each sensor active/inactive edge.
  void setSensorHook(std::function<void(X3DNode *, bool, double)> h);

  // Consumer-supplied view volume. When valid and aspect > 1, VisibilitySensor
  // widens the effective cone half-angle by the supplied aspect factor.
  // Call each frame from the renderer's camera before ctx.tick().
  struct ViewVolume { bool valid = false; float aspect = 1.0f; };
  void setViewVolume(const ViewVolume &vv);
};
```

Billboard rotation is a free function — no class, no registration:

```cpp
// runtime/scene/Billboard.hpp
// §23.4.1: returns the local rotation matrix for a Billboard node.
// parentWorldM = world transform of the Billboard's parent frame.
// axisOfRotation == (0,0,0) selects viewer-alignment mode.
Mat4 billboardLocalMatrix(const Mat4 &parentWorldM,
                          const SFVec3f &cameraWorldPos,
                          const SFVec3f &viewerUpWorld,
                          const SFVec3f &axisOfRotation);
```

LOD level computation is also a free inline:

```cpp
// §23.4.3 step function L(d): returns the zero-based level index.
// Empty range -> 0 (browser choice: highest detail).
int lodSelectLevel(const X3DNode &lod, float distToCenter);
```

### Seam points

- **`X3DExecutionContext` (query side)** — `update` reads `ctx.cameraWorldPosition()`, `ctx.viewMatrix()`, `ctx.worldTransform(node)`, `ctx.cameraWorldUp()`, and `ctx.boundViewpoint()` to obtain viewer pose and per-node world transforms. All of these are provided by [Execution Context](execution-context.md).
- **`ctx.postEvent(node, field, value)`** — all sensor and `level_changed` events are posted through the cascade; the cascade propagates them along any routes that consume them. See [Event Cascade](event-cascade.md).
- **`attachViewDependent` (production wiring)** — lives in `runtime/events/X3DSceneBridge.hpp` rather than in `X3DSceneBridge.hpp` itself to avoid an include cycle (`X3DExecutionContext.hpp` → `PickSystem.hpp` → `Billboard.hpp`; pulling `ViewDependentSystem.hpp` into that chain would close the loop). The caller invokes `attachViewDependent(scene, ctx)` after `ctx.buildSceneGraph(scene)`.
- **`SceneExtractor` (Billboard + LOD render-time)** — `SceneExtractor` includes `Billboard.hpp` directly and calls `billboardLocalMatrix` during its per-path DFS walk (line ~497 of `runtime/extract/SceneExtractor.hpp`). It also calls `lodSelectLevel` during the LOD child-selection pass. These are render-time, per-path decisions; the `ViewDependentSystem` tick handles only the event side. See [Extract](extract.md).
- **`PickSystem` (Billboard render-time)** — `PickSystem` also includes `Billboard.hpp` and applies `billboardLocalMatrix` so ray-pick resolves against the view-rotated geometry, not the unrotated local frame.
- **`ViewVolume` frustum seam** — renderers call `vds->setViewVolume({true, aspect})` each frame to widen the `VisibilitySensor` cone test by the camera's aspect ratio. Full 6-plane frustum test is documented as deferred.
- **Observer hooks** (`setLevelChangedHook`, `setSensorHook`) — test/integration seam; not for production. Production consumers read sensor state by listening on the cascade or reading stored field values.

## How it is tested

- **`view_dependent_test`** (`ctest --preset dev -R x3d_geometry_scene`) — 24 unit tests in `runtime/scene/tests/view_dependent_test.cpp` covering:
  - `testBillboardAxis` / `testBillboardViewerAlign` — `billboardLocalMatrix` for axis-aligned and viewer-aligned modes; asserts the rotated local +Z points toward the viewer.
  - `testBillboardInExtractor` — Billboard rotation applied through `SceneExtractor::fullSnapshot`; the `RenderItem` world transform's +Z faces the viewer.
  - `testBillboardInPick` — Ray pick resolves against the view-rotated geometry (a ray that misses the un-rotated box hits the rotated one at the expected face).
  - `testLodSelect` — `lodSelectLevel` step function: level 0/1/2 at distances 3/7/20 with `range=[5,10]`; empty range → 0.
  - `testLodLevelChanged` — `ViewDependentSystem` fires `level_changed` on viewer move; hook captured.
  - `testLodLevelClamp` — LOD-1 §23.4.3: with 2 children and 3 range bins, far viewer (raw level 2) clamps to reported level 1.
  - `testProximitySensor` — enter/exit edge + correct timestamps.
  - `testProximityLoadTime` — initial enter fires at tick 0 when the viewer starts inside the box.
  - `testProximityChangeGate` — ENV-04: `position_changed` fires only on viewer movement, not every tick; a no-move tick leaves a downstream sentinel unchanged.
  - `testProximityReEnable` — ENV-07: disabling then re-enabling fires a fresh enter when the viewer is still inside.
  - `testProximityDisableFiresExit` — ENV-07: disabling an active sensor fires `isActive=FALSE`/`exitTime`, not silent deactivation.
  - `testVisibilitySensorCone` — box at origin visible from +Z looking down −Z; box moved behind the camera becomes invisible.
  - `testVisibilitySensorFrustumSeam` — off-axis box invisible under the bare cone, visible once `setViewVolume({true, 2.0f})` widens the effective half-angle.
  - `testVisibleFalseSkip` — Group with `visible=false` produces no `RenderItem` in the extractor.
  - `testVisibilityLimitTag` — shape beyond `NavigationInfo.visibilityLimit` is tagged `beyondVisibilityLimit` on its `RenderItem`.
  - `testCameraPose` — `ctx.cameraWorldPosition()` matches the bound Viewpoint's position.
  - `testProductionWiring` — `attachViewDependent` discovers and attaches to all `LOD` and `ProximitySensor` nodes recursively; both fire correct events after the first tick.
  - `testTransformSensorEnter` / `testTransformSensorExit` — §22.4.2: target AABB intersect sensor box fires `isActive` + `enterTime` on entry, `isActive=FALSE` + `exitTime` on exit (verified by translating the wrapping Transform between ticks; requires the new `worldTransformAny` so the target reachable via `targetObject` resolves through its Transform ancestor).
  - `testTransformSensorPositionChanged` — target at known offset emits `position_changed` in the sensor's local frame, relative to center.
  - `testTransformSensorChangeGate` — ENV-04 mirror: `position_changed` / `orientation_changed` fire only on actual change, not every tick.
  - `testTransformSensorDisableFiresExit` — ENV-07: disabling an active TransformSensor fires exit (mirror of Proximity/Visibility).
  - `testTransformSensorNullTarget` — null `targetObject` is inert: no enter event ever fires.

## Related specs and ADRs

- [Architecture](../architecture.md) — the behavior System set and the `ctx.tick()` contract the whole System family runs under.
- [Execution Context](execution-context.md) — provides the viewer-pose query surface (`cameraWorldPosition`, `viewMatrix`, `worldTransform`, `boundViewpoint`).
- [Dirty Bounds / Transform](dirty-bounds-transform.md) — `TransformSystem` + `BoundsSystem` run in the same tick; the View-Dependent System reads their output (`worldTransform(node)`) to compute sensor tests in local space.
- [Sensors](sensors.md) — cross-cutting sensor wiring view; `ViewDependentSystem` owns the environmental sensor family (Proximity + Visibility + LOD).
- [Extract](extract.md) — `SceneExtractor` is the other consumer of `Billboard.hpp` and `lodSelectLevel`; the per-path render-time selection complements this system's per-tick event side.
- [ViewpointBind System](system-viewpointbind.md) — manages the bound Viewpoint that this system reads; both systems depend on `ctx.cameraWorldPosition()` being in sync with the current bind stack.
- Spec: `docs/superpowers/specs/2026-06-07-architecture-validation-and-resequencing.md` — M2e milestone context in which the View-Dependent System was introduced.
