---
title: View-Dependent System
summary: LOD level selection, ProximitySensor/VisibilitySensor/TransformSensor events, and Billboard view-facing rotation — the subsystem that couples the scene graph to the bound viewer each tick.
tags: [subsystem, view-dependent, lod, visibility, transform-sensor, billboard, sensors]
updated: 2026-06-25
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

1. LOD level selection — computes eye-to-center distance in each LOD node's local frame, applies the §23.4.3 step function, clamps to the child count, and fires `level_changed` on transitions.
2. Environmental sensor edge detection — evaluates `ProximitySensor` (viewer-in-box, §22.4.1), `VisibilitySensor` (box-vs-six-plane view frustum, §22.4.3), and `TransformSensor` (targetObject-AABB-in-sensor-box, §22.4.2; the box is in the sensor's local frame, so ancestor Transforms move, scale and rotate it). Proximity and Visibility sensors evaluate the union of their active DEF/USE paths. A cached visit-once pass identifies subtrees with sensors; the active per-path walk skips other subtrees and uses ADR-0037's `WalkBudget` with a 100,000-visit per-update cap, exposing truncation through `budgetExceeded()`. The cache refreshes after scene or child-structure changes. Pose outputs from ProximitySensor include the exit instant; `centerOfRotation_changed` requires LOOKAT in the bound NavigationInfo. Disabling an active Proximity or Visibility sensor emits `isActive=FALSE` without `exitTime`. Sensors reachable only through a non-selected `Switch` child or inactive `LOD` level are treated as removed from the hierarchy (ADR-0034): `update` computes active-path reachability from the scene roots each tick and deactivates any attached sensor it does not reach. ProximitySensor `enterTime`/`exitTime` are the interpolated boundary-crossing times along the viewer's straight-line motion between ticks ; VisibilitySensor uses the tick `now`.
3. Billboard rotation math — a standalone, header-only helper that computes the local rotation matrix that faces a Billboard node toward the viewer; consumed directly by `SceneExtractor` and `PickSystem` during their per-path walks.

The split between tick-time (LOD events, sensor edges) and render-time (Billboard rotation, LOD child selection for rendering) is intentional and documented: the `ViewDependentSystem` class owns the event side; the free function `billboardLocalMatrix` and the inline `lodSelectLevel` are used at extract/pick time. This avoids a dependency cycle between `SceneExtractor`, `PickSystem`, and the execution context.

Proximity sensor orientation uses `rotationFromMatrix` to convert the viewer's
forward/up basis in sensor coordinates. X- and Z-axis half-turns preserve that
basis. A zero forward vector returns the identity orientation.

The viewer pose comes from the bound Viewpoint, navigation, and relative head
tracking, or from the host's persistent `setViewerWorldPose(cameraToWorld)`
override. With no bound Viewpoint, head tracking composes at the world origin,
so LOD, Billboards, and sensors follow a tracked viewer in scenes that author
no Viewpoint; with identity tracking the viewer stays at the origin. All of
these feed the same camera queries. Set the pose before ticking. See
[ADR-0065](../decisions/0065-absolute-viewer-world-pose.md) and
[ADR-0073](../decisions/0073-implicit-default-viewer.md).

## Key files

| File / directory | Role |
|---|---|
| `runtime/scene/ViewDependentSystem.hpp` | `ViewDependentSystem` class (a `System`): `attach`, `update`, sensor/LOD tracking, observer seams |
| `runtime/scene/Billboard.hpp` | `billboardLocalMatrix` free function (§23.4.1), using `runtime/math/Vec3.hpp`; no `X3DExecutionContext` dependency |
| `runtime/events/X3DSceneBridge.hpp` | `attachViewDependent(Scene&, X3DExecutionContext&)` — production wiring: scene walk + per-node `attach` call |
| `runtime/scene/tests/view_dependent_test.cpp` | Full unit test suite (31 test functions, one `main`); doctest case `view_dependent_test` in the `x3d_geometry_scene` target |

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

  // Consumer-supplied view volume. VisibilitySensor tests the sensor box
  // against the six view-frustum planes of the bound viewpoint; `aspect` (the
  // consumer camera's width/height ratio, default 1.0) sets how much the
  // horizontal half-angle exceeds the vertical one.
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

- `X3DExecutionContext` (query side) — `update` reads `ctx.cameraWorldPosition()`, `ctx.viewMatrix()`, `ctx.worldTransform(node)`, `ctx.cameraWorldUp()`, and `ctx.boundViewpoint()` to obtain viewer pose and per-node world transforms. All of these are provided by [Execution Context](execution-context.md).
- `ctx.postEvent(node, field, value)` — all sensor and `level_changed` events are posted through the cascade; the cascade propagates them along any routes that consume them. See [Event Cascade](event-cascade.md).
- `attachViewDependent` (production wiring) — is declared in `runtime/events/X3DSceneBridge.hpp` and defined in `X3DSceneBridge.cpp`, so `ViewDependentSystem.hpp` stays out of the header chain (`X3DExecutionContext.hpp` → `PickSystem.hpp` → `Billboard.hpp`) and cannot close an include cycle. The caller invokes `attachViewDependent(scene, ctx)` after `ctx.buildSceneGraph(scene)`.
- `SceneExtractor` (Billboard + LOD render-time) — `SceneExtractor` includes `Billboard.hpp` directly and calls `billboardLocalMatrix` during its per-path DFS walk (line ~497 of `runtime/extract/SceneExtractor.hpp`). It also calls `lodSelectLevel` during the LOD child-selection pass. These are render-time, per-path decisions; the `ViewDependentSystem` tick handles only the event side. See [Extract](extract.md).
- `PickSystem` (Billboard render-time) — `PickSystem` also includes `Billboard.hpp` and applies `billboardLocalMatrix` so ray-pick resolves against the view-rotated geometry, not the unrotated local frame.
- `ViewVolume` frustum seam — renderers call `vds->setViewVolume({true, aspect})` each frame to supply the camera aspect ratio; `VisibilitySensor` builds the six view-frustum planes from `boundViewpoint().fieldOfView` (the smaller of the horizontal/vertical angles, §23.4.6) and tests the sensor's world-space box against them.
- Active-path reachability — `update` walks `ctx.sceneRoots()` honouring `Switch.whichChoice` and the selected `LOD` level (ADR-0034) and deactivates any attached sensor outside the active hierarchy.
- Observer hooks (`setLevelChangedHook`, `setSensorHook`) — test/integration seam; not for production. Production consumers read sensor state by listening on the cascade or reading stored field values.

## Related

- [Architecture](../architecture.md)
- [Execution Context](execution-context.md)
- [Dirty Bounds / Transform](dirty-bounds-transform.md)
- [Sensors](sensors.md)
- [Extract](extract.md)
- [ViewpointBind System](system-viewpointbind.md)
