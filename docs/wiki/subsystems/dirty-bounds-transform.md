---
title: "Dirty Tracking, Transforms, and Bounds"
summary: Dirty-flag propagation, world-transform accumulation, and bounding-volume computation across the scene graph.
tags: [subsystem, dirty-tracking, transforms, bounds, cycle-breaker]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/scene-graph.md
  - ../subsystems/math.md
  - ../decisions/0016-cycle-breaker.md
---

# Dirty Tracking, Transforms, and Bounds

## Purpose

This subsystem records which nodes changed during a tick, propagates world-space transforms down the transform hierarchy incrementally, and maintains local-frame axis-aligned bounding boxes (AABBs) for every scene-graph node. It owns the contract that renderers and pick-systems can read the current world matrix or AABB of any node via a side table without touching the node object itself. Malformed scenes containing containment cycles are sanitised once at scene-build time by `CycleBreaker`, converting the graph to a DAG so every downstream recursive walker is safe.

## Key files

| File | Role |
|---|---|
| `runtime/scene/DirtyTracker.hpp` | Per-node dirty-category bit flags + ordered changed-node list; the central tap the event cascade and field-write seam write into each tick |
| `runtime/scene/TransformSystem.hpp` | Transform-hierarchy index, world-matrix side table, and incremental `propagate()` pass driven by `DirtyLocalTransform` flags |
| `runtime/scene/BoundsSystem.hpp` | Local-frame AABB side table, post-order build, and incremental bottom-up `propagate()` pass driven by any dirty flag |
| `runtime/scene/GeometryBounds.hpp` | Type-dispatched local AABB computation for geometry leaves (Box, Sphere, Cone, Cylinder, IFS-via-coord, ElevationGrid, Extrusion, Text, …) |
| `runtime/scene/CycleBreaker.hpp` | One-shot DFS that severs containment back-edges so the graph is a DAG before any walker runs |

## Interfaces and seams

### Exposed interface

```cpp
namespace x3d::runtime {

// --- DirtyTracker ---
enum DirtyFlags : unsigned {
  DirtyNone           = 0,
  DirtyLocalTransform = 1u << 0,  // a Transform's TRS field changed
  DirtyWorldTransform = 1u << 1,  // world matrix recomputed this tick
  DirtyChildren       = 1u << 2,  // children/addChildren/removeChildren changed
  DirtyField          = 1u << 3,  // any other field changed
  DirtyBounds         = 1u << 4,  // AABB needs recompute
};

class DirtyTracker {
public:
  void markDirty(const X3DNode *n, unsigned flags);
  unsigned flags(const X3DNode *n) const;
  const std::vector<const X3DNode *> &changedNodes() const;
  void clear();
};

// --- TransformSystem ---
class TransformSystem {
public:
  void buildIndex(const Scene &scene);          // call once at scene load
  Mat4 worldTransform(const X3DNode *n) const;  // Transform node only; identity if not indexed
  Mat4 worldTransformAny(const X3DNode *n) const; // any node: Transform = own world; non-Transform = nearest ancestor Transform's world (walked UP via the parent index, computed live each call so mid-tick `setF` on translation is reflected immediately — unlike the cached `worldTransform`, which only refreshes via `propagate()` at tick end)
  void propagate(DirtyTracker &dirty);           // per-tick incremental pass
  static Mat4 localMatrix(const X3DNode *n);    // public: used by BoundsSystem, PickSystem
  static bool isTransform(const X3DNode *n);    // shared predicate
};

// --- BoundsSystem ---
class BoundsSystem {
public:
  void buildBounds(const Scene &scene, const TransformSystem &ts); // call once at scene load
  const Aabb &localBounds(const X3DNode *n) const;
  Aabb worldBounds(const X3DNode *n, const TransformSystem &ts) const; // lazy: localBounds + worldTransform
  void propagate(DirtyTracker &dirty, const TransformSystem &ts);       // per-tick incremental pass
};

// --- CycleBreaker ---
// Returns the number of back-edges severed (0 for any well-formed scene).
int breakContainmentCycles(Scene &scene);

} // namespace x3d::runtime
```

Key properties:

- **Side-table design.** Nothing is stored on the node objects. `DirtyTracker`, `TransformSystem`, and `BoundsSystem` each hold their own `unordered_map<const X3DNode *, …>` keyed by pointer. This keeps the generated node bindings unchanged and means any consumer can hold a raw pointer and query the tables.
- **Incremental propagation.** `TransformSystem::propagate` only visits subtrees whose root `Transform` node is flagged `DirtyLocalTransform` and has no dirtied ancestor (i.e., it finds the minimal set of subtree roots and recurses down from each). `BoundsSystem::propagate` walks upward from every dirty node via `recomputeUp`, stopping when the recomputed AABB matches the stored value.
- **World bounds are lazy.** `BoundsSystem::worldBounds` composes `localBounds` + `TransformSystem::worldTransform` on the fly; no separate world-AABB table is maintained. (A separate `worldTransformAny` walks up the parent index to resolve a target through its nearest ancestor Transform — for callers that need the world frame of a non-Transform node, e.g. `TransformSensor.targetObject`.)
- **USE/DEF sharing guard.** Both `TransformSystem::walk` and `BoundsSystem::index` use a `walked_` / `indexed_` visited set so a USE-shared node's subtree is only recursed once (avoiding a multiplicative explosion on heavy USE/DEF scenes). `BoundsSystem::index` still records every parent edge per reference so the bounds union stays correct.
- **Cycle guard inside `compute`.** `BoundsSystem::compute` uses a `computing_` gray-set to stop a recursive re-entry on a back-edge that escaped `CycleBreaker` (e.g., a back-edge reached only through `index`'s all-edge recording). The contribution of such a re-entrant node is zero.

### Seam points

- **Field-write seam / event cascade** — When the event cascade applies a route-delivered value to a node field, it calls `DirtyTracker::markDirty` with appropriate flags (`DirtyLocalTransform` for a Transform TRS field, `DirtyField` for everything else). The dirty tracker is the single shared observer slot per tick (see `docs/superpowers/specs/2026-06-20-x3d-sim-design.md` ADR-0009).
- **`ExecutionContext` / per-tick driver** — After the event cascade settles, the per-tick driver calls `TransformSystem::propagate(dirty)` then `BoundsSystem::propagate(dirty, ts)`. The order is enforced: bounds need the already-updated world transforms. (`DirtyTracker::clear()` is not called here — `X3DExecutionContext::tick` drops the previous tick's changed-set at the *start* of the next tick, before re-evaluating.)
- **`TransformSystem::localMatrix` / `isTransform`** — `BoundsSystem` and `PickSystem` import `TransformSystem::localMatrix` and `TransformSystem::isTransform` directly (both are `static` public members) so there is a single canonical definition of the four transform-bearing node types: `Transform`, `HAnimHumanoid`, `HAnimJoint`, `CADPart`. `Billboard` is excluded from the static set because its effective transform depends on the active viewpoint (deferred to ViewDependentSystem).
- **`GeometryBounds::localGeometryBounds`** — `BoundsSystem::compute` and `BoundsSystem::recomputeLocal` call the free function `localGeometryBounds(const X3DNode *)` from `GeometryBounds.hpp` to obtain a geometry leaf's own AABB. New geometry types are added here without changing `BoundsSystem`.
- **Author `bboxSize` / `bboxCenter` override** — If a node carries both `bboxSize` fields with all components ≥ 0, `BoundsSystem::authorBounds` uses that box and skips the child union for that node (children still get their own entries computed).
- **`CycleBreaker` / scene build** — `breakContainmentCycles(scene)` is called once inside `buildSceneGraph` (in `runtime/X3DRuntime.hpp` or equivalent) before any system indexes the scene. It requires read+write field access (`f.get` and `f.set`) to re-seat severed fields to null/empty. See [ADR-0016](../decisions/0016-cycle-breaker.md).

## How it is tested

All of these are doctest cases compiled into the `x3d_geometry_scene` ctest executable.

| doctest case | Covers |
|---|---|
| `dirty_tracker_test` | `DirtyTracker` flag OR-ing, first-transition-only list insertion, `clear()` |
| `transform_system_test` | `buildIndex` + `worldTransform` for nested `Transform` chains; `propagate` incremental pass |
| `transform_system_hanim_cadpart_test` | `HAnimHumanoid`, `HAnimJoint`, `CADPart` treated as transform-bearing nodes |
| `geometry_bounds_test` | `localGeometryBounds` for each supported primitive and mesh type |
| `bounds_system_test` | `buildBounds` post-order computation, author-bbox override, `propagate` bottom-up update |
| `bounds_shared_subgraph_test` | USE/DEF sharing: a node reached by multiple paths gets one local AABB entry without multiplicative recompute; 30-second timeout |
| `bounds_cycle_test` | Containment back-edge in `BoundsSystem::compute` is caught by the gray-set guard and does not overflow the stack; 30-second timeout |
| `cycle_breaker_test` | `breakContainmentCycles` severs SFNode and MFNode back-edges; no effect on well-formed scenes; 30-second timeout |

Run the full group:

```
ctest --preset dev -R x3d_geometry_scene
```

There are no golden files for this subsystem — correctness is checked by assertion in each test binary.

## Related specs and ADRs

- [ADR-0016: Containment Cycles Severed Once at Scene Build](../decisions/0016-cycle-breaker.md)
- [ADR-0011: ECS-Flavored, Not ECS](../decisions/0011-ecs-flavored-not-ecs.md) — side-table design rationale (systems own data, not nodes)
- [ADR-0009: sim snapshot-diff](../decisions/0009-sim-snapshot-diff.md) — the single cascade observer slot is reserved for dirty-tracking; `x3d sim` uses snapshot-diff instead
- [Scene Graph](../subsystems/scene-graph.md) — the `Scene` and `X3DNode` types the side tables are keyed on
- [Math](../subsystems/math.md) — `Mat4` and `Aabb` types used throughout
- Spec: `docs/superpowers/specs/2026-06-20-x3d-sim-design.md` — observer-slot / dirty-tracker contract
- Source: `runtime/scene/DirtyTracker.hpp`
- Source: `runtime/scene/TransformSystem.hpp`
- Source: `runtime/scene/BoundsSystem.hpp`
- Source: `runtime/scene/GeometryBounds.hpp`
- Source: `runtime/scene/CycleBreaker.hpp`
