---
title: Scene graph
summary: Runtime core scene graph — DEF/USE sharing, document/scene model, and the structural systems that traverse it (transform, bounds, binding, pick, dirty-tracking, cycle safety, view-dependent).
tags: [subsystem, scene-graph, runtime, def-use]
updated: 2026-09-29
related:
  - ../../explanation/architecture.md
  - dirty-bounds-transform.md
  - execution-context.md
  - extract.md
  - system-viewdependent.md
  - system-viewpointbind.md
  - system-pointing.md
---

# Scene graph

## Purpose

The scene graph subsystem owns the parsed world, its structural indices and cycle handling.

## Behavior

1. Document and scene model — `X3DDocument` wraps an entire `<X3D>` element: header metadata, profile/version, the root `Scene`, and per-parse diagnostics (range warnings, proto warnings, inline warnings). `Scene` holds the root-node vector, the DEF symbol table, ROUTEs, PROTO/ExternPROTO declarations, and IMPORT/EXPORT statements. DEF/USE semantics are identity-sharing: a USE resolves to the exact same `std::shared_ptr<X3DNode>` registered by the prior DEF — no cloning, no indirection object.

2. Structural side-systems — a family of stateless-per-node systems (`TransformSystem`, `BoundsSystem`, `BindingSystem`, `PickSystem`, `DirtyTracker`, `CycleBreaker`, `ViewDependentSystem`, `Billboard`) that traverse the graph via its SFNode/MFNode reflection fields without touching node internals. All state lives in side tables keyed by `const X3DNode*`.

3. Cycle safety — malformed input can produce containment cycles (a node reachable from its own ancestor, e.g. `<X DEF='a' USE='a'/>`). `CycleBreaker` severs back-edges once at parse time, severing detected back-edges. Walkers retain their own visited sets and traversal budgets.

The subsystem does not own the event cascade, route propagation, or per-tick behavioral semantics — those live in [Execution context](execution-context.md).

## Key files

| File / directory | Role |
|---|---|
| `runtime/X3DScene.hpp` | `x3d::runtime::Scene` — root nodes, DEF table (`defs`), routes, proto/extern-proto declarations, unexpanded `ProtoInstance` list, `resolvedProtoRoutes`, `expandedInlines`, `resolvedInlineRoutes`, `protoRedirects`, `expandedSources`, IMPORT/EXPORT, source UNIT declarations, parse-time `authoredScalarFields`, per-field `normalizedUnitFields`, and weak-node-keyed `unitFieldSources` for PROTO and IS provenance. Methods: `define()`, `resolve()`, `addRootNode()`, `findProto()`, `declareProto()`, `declareExternProto()`, `resolveRoutes()`, `instanceAtPlacement()`. |
| `runtime/X3DAuthoredScalarFields.hpp` | Weak node-identity keys retain which scalar fields were explicitly assigned during parsing and which have been normalized, without keeping nodes alive. Authored marks distinguish omission from an explicit value equal to a generated default; normalized marks prevent repeated conversion across context setup and topology refresh. Neither is a log of later runtime writes. |
| `runtime/UnitConversion.hpp` | Explicit node/field dimension map and `normalizeRuntimeUnits(Scene&)`: applies source UNIT factors to authored known dimensional fields at runtime entry, uses per-field source provenance for expanded PROTO/IS targets, and leaves built-in defaults unchanged. |
| `runtime/X3DDocument.hpp` | `x3d::runtime::X3DDocument` — top-level `<X3D>` object: `version`, `profile` (`Profile` enum), `head` (`Head`), `scene` (`Scene`), `rangeWarnings`, `protoWarnings`, `inlineWarnings`. Also defines `Scene::addRootNode()` (needs complete `X3DNode`). |
| `runtime/X3DRuntime.hpp` | Umbrella include: pulls in `X3DDocument`, `X3DHeader`, `X3DImportExport`, `X3DProto`, `X3DRangeValidate`, `X3DRoute`, `X3DScene` in one include. |
| `runtime/scene/DirtyTracker.hpp` | `DirtyTracker` — per-node dirty-category bitset + changed-node list. Categories: `DirtyLocalTransform`, `DirtyWorldTransform`, `DirtyChildren`, `DirtyField`, `DirtyBounds`. Side table; nothing stored on the node. |
| `runtime/scene/TransformSystem.hpp` | `TransformSystem` — transform-hierarchy index + world-transform side table + incremental propagation. Covers `Transform`, `HAnimHumanoid`, `HAnimJoint`, `CADPart`. Public: `buildIndex(Scene)`, `worldTransform(node)` (Transform only), `worldTransformAny(node)` (any node — walks UP via parent index to the nearest ancestor Transform, computed live), `worldTransformUnder(parent, node)` (one parent edge; a DEF/USE node has a world per parent), `propagate(DirtyTracker)` (local transforms + `DirtyChildren` structural re-index), `revision()` (monotonic cache key), `localMatrix(node)` (static), `isTransform(node)` (static). |
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
for (auto &root : sc.rootNodes) { /*... */ }
```

- Parsed prototype instances occupy authored template positions in `rootNodes`
  and ordinary node fields until expansion replaces them in place.
- The structural `protoInstances` records own their pre-expansion source
  values; weak `placementTemplate` links identify their slots.
- Root and child vectors remain the authority for position, including repeated
  USE occurrences and caller reordering/removal.
- An unresolved instance retains an inert template in its position so
  serialization preserves its order.

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

- DEF/USE identity seam — `Scene::define(name, shared_ptr<X3DNode>)` and `Scene::resolve(name)` are the contract that readers write and the codec writers read. A writer emits `DEF=` on the first encounter and `USE=` on subsequent shared-ptr occurrences; the reader rebuilds the shared identity by calling `resolve()` at USE sites.

- `X3DExecutionContext` owner — the execution context (see [Execution context](execution-context.md)) owns all the side-table systems (`TransformSystem`, `BoundsSystem`, `BindingSystem`, `DirtyTracker`, `PickSystem`) and calls `buildIndex` / `buildBounds` / `enrollScene` after parse and `propagate` after each tick.

- `BindingSystem` Poster + Clock — `BindingSystem::enrollScene(scene, Poster, Clock)` accepts callbacks so the system can post `isBound` / `bindTime` events into the cascade without an include dependency on `X3DExecutionContext`. The execution context supplies `[this](node, field, val){ postEvent(node, field, val); }` as the poster.

- `ViewDependentSystem` consumer — `ViewDependentSystem::update(now, ctx)` is called by the context each tick; it reads `ctx.cameraWorldPosition()`, `ctx.viewMatrix()`, `ctx.worldTransform(node)` and `ctx.boundViewpoint()` — all provided by `X3DExecutionContext`. A consumer can supply an exact view volume via `setViewVolume(ViewVolume)` for VisibilitySensor widening.

- `PickSystem` Billboard hook — `PickSystem::pickClosest(ray, bounds, cameraPos, cameraUp)` accepts live camera pose so Billboard nodes orient their geometry during the pick traversal, matching `SceneExtractor`'s per-path walk.

- Reflection traversal — every walker in this layer traverses the graph through `n->fields()` and casts `SFNode` / `MFNode` fields, Generated fields are traversed directly; author fields require explicit store-aware traversal and are not implicitly added to `fields`.

- `CycleBreaker` → `BoundsSystem` — `BoundsSystem` adds a secondary defense-in-depth cycle guard (`computing_` set) for containment cycles that reach bounds computation, but `breakContainmentCycles` is the primary fix applied once at `buildSceneGraph`.

## Related

- [Architecture](../../explanation/architecture.md)
- [Dirty, Bounds, Transform](dirty-bounds-transform.md)
- [Execution context](execution-context.md)
- [Extract](extract.md)
- [View-dependent behavior](system-viewdependent.md)
- [Viewpoint Binding](system-viewpointbind.md)
- [Pointing-Device Sensors](system-pointing.md)
- [ADR-0016: Cycle Breaker](../../explanation/decisions/0016-cycle-breaker.md)
