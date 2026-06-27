---
title: Scene Graph
summary: Runtime core scene graph — DEF/USE sharing, document/scene model, and the structural systems that traverse it (transform, bounds, binding, pick, dirty-tracking, cycle safety, view-dependent).
tags: [subsystem, scene-graph, runtime, def-use]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/dirty-bounds-transform.md
  - ../subsystems/execution-context.md
  - ../subsystems/extract.md
  - ../subsystems/system-viewdependent.md
  - ../subsystems/system-viewpointbind.md
  - ../subsystems/system-pointing.md
---

# Scene Graph

## Purpose

The scene graph subsystem owns the in-memory representation of a parsed X3D world and every read-only structural operation performed over it before or during a tick. It covers three concerns that are inseparable in practice:

1. **Document and scene model** — `X3DDocument` wraps an entire `<X3D>` element: header metadata, profile/version, the root `Scene`, and per-parse diagnostics (range warnings, proto warnings, inline warnings). `Scene` holds the root-node vector, the DEF symbol table, ROUTEs, PROTO/ExternPROTO declarations, and IMPORT/EXPORT statements. DEF/USE semantics are identity-sharing: a USE resolves to the exact same `std::shared_ptr<X3DNode>` registered by the prior DEF — no cloning, no indirection object.

2. **Structural side-systems** — a family of stateless-per-node systems (`TransformSystem`, `BoundsSystem`, `BindingSystem`, `PickSystem`, `DirtyTracker`, `CycleBreaker`, `ViewDependentSystem`, `Billboard`) that traverse the graph via its SFNode/MFNode reflection fields without touching node internals. All state lives in side tables keyed by `const X3DNode*`.

3. **Cycle safety** — malformed input can produce containment cycles (a node reachable from its own ancestor, e.g. `<X DEF='a' USE='a'/>`). `CycleBreaker` severs back-edges once at parse time, converting the graph to a DAG that every recursive walker can traverse without a visited-set.

The subsystem does **not** own the event cascade, route propagation, or per-tick behavioral semantics — those live in [Execution Context](../subsystems/execution-context.md).

## Key files

| File / directory | Role |
|---|---|
| `runtime/X3DScene.hpp` | `x3d::runtime::Scene` — root nodes, DEF table (`defs`), routes, proto/extern-proto declarations, unexpanded `ProtoInstance` list, `resolvedProtoRoutes`, `expandedInlines`, `resolvedInlineRoutes`, `protoRedirects`, `expandedSources`, IMPORT/EXPORT. Methods: `define()`, `resolve()`, `addRootNode()`, `findProto()`, `resolveRoutes()`. |
| `runtime/X3DDocument.hpp` | `x3d::runtime::X3DDocument` — top-level `<X3D>` object: `version`, `profile` (`Profile` enum), `head` (`Head`), `scene` (`Scene`), `rangeWarnings`, `protoWarnings`, `inlineWarnings`. Also defines `Scene::addRootNode()` (needs complete `X3DNode`). |
| `runtime/X3DRuntime.hpp` | Umbrella include: pulls in `X3DDocument`, `X3DHeader`, `X3DImportExport`, `X3DProto`, `X3DRangeValidate`, `X3DRoute`, `X3DScene` in one include. |
| `runtime/scene/DirtyTracker.hpp` | `DirtyTracker` — per-node dirty-category bitset + changed-node list. Categories: `DirtyLocalTransform`, `DirtyWorldTransform`, `DirtyChildren`, `DirtyField`, `DirtyBounds`. Side table; nothing stored on the node. |
| `runtime/scene/TransformSystem.hpp` | `TransformSystem` — transform-hierarchy index + world-transform side table + incremental propagation. Covers `Transform`, `HAnimHumanoid`, `HAnimJoint`, `CADPart`. Public: `buildIndex(Scene)`, `worldTransform(node)` (Transform only), `worldTransformAny(node)` (any node — walks UP via parent index to the nearest ancestor Transform, computed live), `propagate(DirtyTracker)`, `localMatrix(node)` (static), `isTransform(node)` (static). |
| `runtime/scene/BoundsSystem.hpp` | `BoundsSystem` — local-frame AABB per node + bottom-up propagation. `buildBounds(Scene, TransformSystem)`, `localBounds(node)`, `worldBounds(node, TransformSystem)`, `propagate(DirtyTracker, TransformSystem)`. Uses `GeometryBounds.hpp` for leaf geometry. |
| `runtime/scene/GeometryBounds.hpp` | `localGeometryBounds(X3DNode*)` — type-dispatched local AABB for geometry nodes (Box, Sphere, Cone, Cylinder, mesh via `coord`/`controlPoint`, ElevationGrid, Extrusion, Text). Also the reflection helpers `geombounds::getField`, `getNode`, `hasField` used across the scene layer. |
| `runtime/scene/CycleBreaker.hpp` | `breakContainmentCycles(Scene&)` — DFS severs SFNode/MFNode back-edges; returns the count of severed edges (0 on valid content). Called once in `buildSceneGraph`. |
| `runtime/scene/BindingStack.hpp` | `BindingStack` — one category's bind/unbind stack following ISO/IEC 19775-1 §7.2.2. Methods: `bind`, `unbind`, `pushDefault`, `top`. Decoupled from context via an `Emit` callback. |
| `runtime/scene/BindingSystem.hpp` | `BindingSystem` — enrols bindable nodes from a `Scene`, wires each node's `set_bind` handler to drive its stack, default-binds the first of each category. Uses a `Poster` + `Clock` callback to avoid an include cycle with `X3DExecutionContext`. |
| `runtime/scene/PickSystem.hpp` | `PickSystem` — on-demand ray pick: broad phase via `BoundsSystem` world AABBs, narrow phase analytic (Sphere, Cone, Cylinder) or triangle-mesh (IFS/ITS/TriangleSet/strip/fan sets via `buildLocalMesh`), AABB proxy fallback. Emits `PickResult` with `path` (root→hit chain), world-space `normal`, and surface `texCoord`. |
| `runtime/scene/Billboard.hpp` | `billboardLocalMatrix()` — pure math helper for §23.4.1 view-facing rotation; standalone to avoid include cycles between `ViewDependentSystem` and `PickSystem`. |
| `runtime/scene/ViewDependentSystem.hpp` | `ViewDependentSystem` — per-tick LOD `level_changed` tracking, `ProximitySensor`/`VisibilitySensor`/`TransformSensor` enter/exit (the latter resolves the target's world frame via `worldTransformAny` so the target reachable through both `targetObject` and a Transform ancestor picks the Transform path). Render-time LOD child selection and Billboard rotation live in the per-path walkers (`SceneExtractor`, `PickSystem`) not here. |
| `runtime/scene/` | Directory for all scene-layer headers; corresponding tests in `runtime/scene/tests/`. |

## Interfaces and seams

### Exposed interface

The core types are in `namespace x3d::runtime`. The umbrella include is:

```cpp
#include "X3DRuntime.hpp"   // Scene, X3DDocument, Route, PROTO, etc.
```

The document model is a value-semantic aggregate — no virtual dispatch, no hidden state:

```cpp
x3d::runtime::X3DDocument doc;
// After parsing:
x3d::runtime::Scene &sc = doc.scene;
// Resolve a USE to its shared node:
auto node = sc.resolve("MyDEFName");   // nullptr if unknown
// Iterate root nodes:
for (auto &root : sc.rootNodes) { /* ... */ }
```

The structural systems are instantiated as side-tables by the owner (`X3DExecutionContext`) and queried by consumers:

```cpp
x3d::runtime::TransformSystem ts;
ts.buildIndex(scene);
Mat4 world = ts.worldTransform(someNode);

x3d::runtime::BoundsSystem bs;
bs.buildBounds(scene, ts);
Aabb local = bs.localBounds(someNode);
Aabb worldAabb = bs.worldBounds(someNode, ts);

x3d::runtime::PickSystem ps;
ps.build(scene);
auto result = ps.pickClosest(ray, bs, cameraPos, cameraUp);

x3d::runtime::DirtyTracker dt;
dt.markDirty(node, DirtyLocalTransform);
ts.propagate(dt);
bs.propagate(dt, ts);
```

Cycle-breaking runs once after parse:

```cpp
int severed = x3d::runtime::breakContainmentCycles(scene);
```

### Seam points

- **DEF/USE identity seam** — `Scene::define(name, shared_ptr<X3DNode>)` and `Scene::resolve(name)` are the contract that readers write and the codec writers read. A writer emits `DEF=` on the first encounter and `USE=` on subsequent shared-ptr occurrences; the reader rebuilds the shared identity by calling `resolve()` at USE sites.

- **`X3DExecutionContext` owner** — the execution context (see [Execution Context](../subsystems/execution-context.md)) owns all the side-table systems (`TransformSystem`, `BoundsSystem`, `BindingSystem`, `DirtyTracker`, `PickSystem`) and calls `buildIndex` / `buildBounds` / `enrollScene` after parse and `propagate` after each tick.

- **`BindingSystem` Poster + Clock** — `BindingSystem::enrollScene(scene, Poster, Clock)` accepts callbacks so the system can post `isBound` / `bindTime` events into the cascade without an include dependency on `X3DExecutionContext`. The execution context supplies `[this](node, field, val){ postEvent(node, field, val); }` as the poster.

- **`ViewDependentSystem` consumer** — `ViewDependentSystem::update(now, ctx)` is called by the context each tick; it reads `ctx.cameraWorldPosition()`, `ctx.viewMatrix()`, `ctx.worldTransform(node)` and `ctx.boundViewpoint()` — all provided by `X3DExecutionContext`. A consumer can supply an exact view volume via `setViewVolume(ViewVolume)` for VisibilitySensor widening.

- **`PickSystem` Billboard hook** — `PickSystem::pickClosest(ray, bounds, cameraPos, cameraUp)` accepts live camera pose so Billboard nodes orient their geometry during the pick traversal, matching `SceneExtractor`'s per-path walk.

- **Reflection traversal** — every walker in this layer traverses the graph through `n->fields()` and casts `SFNode` / `MFNode` fields, so it works uniformly over both the generated bindings and any dynamically-fielded `Script` node that exposes node-type fields via `effectiveFields()`.

- **`CycleBreaker` → `BoundsSystem`** — `BoundsSystem` adds a secondary defense-in-depth cycle guard (`computing_` set) for containment cycles that reach bounds computation, but `breakContainmentCycles` is the primary fix applied once at `buildSceneGraph`.

## How it is tested

Unit tests live in `runtime/scene/tests/`, all compiled into the `x3d_geometry_scene` ctest executable:

- `ctest --preset dev -R x3d_geometry_scene` (doctest case: `dirty_tracker_test`) — `DirtyTracker` mark/clear/flag semantics.
- `ctest --preset dev -R x3d_geometry_scene` (doctest case: `transform_system_test`) — `TransformSystem` world-transform index and incremental propagation via a synthetic scene.
- `ctest --preset dev -R x3d_geometry_scene` (doctest case: `transform_system_hanim_cadpart_test`) — `HAnimHumanoid`, `HAnimJoint`, `CADPart` TRS frames.
- `ctest --preset dev -R x3d_geometry_scene` (doctest case: `bounds_system_test`) — `BoundsSystem` leaf geometry, shape hierarchy, post-order build.
- `ctest --preset dev -R x3d_geometry_scene` (doctest case: `bounds_shared_subgraph_test`) — DEF/USE shared subgraph: a USE-shared node must not produce multiplicative recompute; bounds are path-independent.
- `ctest --preset dev -R x3d_geometry_scene` (doctest case: `bounds_cycle_test`) — defense-in-depth cycle guard inside `BoundsSystem::compute`.
- `ctest --preset dev -R x3d_geometry_scene` (doctest case: `cycle_breaker_test`) — `breakContainmentCycles` severs back-edges; well-formed scenes return 0.
- `ctest --preset dev -R x3d_geometry_scene` (doctest case: `geometry_bounds_test`) — `localGeometryBounds` type dispatch for each primitive and mesh variant.
- `ctest --preset dev -R x3d_geometry_scene` (doctest case: `pick_system_test`) — `PickSystem` analytic narrow phase (Sphere/Cone/Cylinder), mesh narrow phase, AABB fallback, Billboard-pick pose.
- `ctest --preset dev -R x3d_geometry_scene` (doctest case: `binding_stack_test`) — `BindingStack` push/pop/bind/unbind protocol.
- `ctest --preset dev -R x3d_geometry_scene` (doctest case: `binding_system_test`) — `BindingSystem` scene enrolment + default-bind.
- `ctest --preset dev -R x3d_geometry_scene` (doctest case: `bind_time_conformance_test`) — `bindTime` fires on both bind and unbind transitions (ISO/IEC 19775-1 §23.3.1).
- `ctest --preset dev -R x3d_geometry_scene` (doctest case: `binding_stack_audit_test`) — extended audit of bind/unbind ordering edge cases.
- `ctest --preset dev -R x3d_geometry_scene` (doctest case: `interface_registry_test`) — `X3DInterfaceRegistry` (used by `BindingSystem::category`).
- `ctest --preset dev -R x3d_geometry_scene` (doctest case: `view_dependent_test`) — `ViewDependentSystem` LOD `level_changed`, `ProximitySensor`, `VisibilitySensor`, `TransformSensor` enter/exit (24 tests).

The scene model is also exercised end-to-end by the corpus sweep (`mise run corpus`, `x3d_corpus_smoke`) — 17,719 files, 0 crashes — which first surfaced the containment-cycle stack overflow that motivated `CycleBreaker`.

## Related specs and ADRs

- [Architecture](../architecture.md) — layer map showing where the scene graph sits in the full system.
- [Dirty, Bounds, Transform](../subsystems/dirty-bounds-transform.md) — dedicated page for `DirtyTracker` + `TransformSystem` + `BoundsSystem` as a composed incremental system.
- [Execution Context](../subsystems/execution-context.md) — owns and drives the scene-layer side-systems each tick.
- [Extract](../subsystems/extract.md) — `SceneExtractor` traverses the scene graph via the same SFNode/MFNode reflection fields; reads `BoundsSystem` world AABBs and `TransformSystem` world matrices.
- [View-Dependent System](../subsystems/system-viewdependent.md) — `ViewDependentSystem` detail.
- [Viewpoint Binding](../subsystems/system-viewpointbind.md) — `BindingSystem` + `BindingStack` in their behavioral context.
- [Pointing-Device Sensors](../subsystems/system-pointing.md) — `PickSystem` used by `PointingSensorSystem` to resolve hit nodes.
- Decision: [ADR-0016: Cycle Breaker](../decisions/0016-cycle-breaker.md) — records why cycles are severed at source rather than guarded in each walker.
- Spec: `docs/superpowers/specs/2026-06-20-project-wiki-design.md` — wiki design that this page is authored under.
