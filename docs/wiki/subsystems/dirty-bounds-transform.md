---
title: "Dirty Tracking, Transforms, and Bounds"
summary: Dirty-flag propagation, world-transform accumulation, and bounding-volume computation across the scene graph.
tags: [subsystem, dirty-tracking, transforms, bounds, cycle-breaker]
updated: 2026-09-30
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
  DirtyChildren       = 1u << 2,  // node-reference or visibility changed
  DirtyField          = 1u << 3,  // any other field changed
  DirtyBounds         = 1u << 4,  // AABB needs recompute
  DirtyActiveChild    = 1u << 5,  // Switch/LOD selection; references stay valid
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
  Mat4 worldTransformUnder(const X3DNode *parent, const X3DNode *n) const; // Transform n's world through one parent edge (live); a DEF/USE node under two parents has a distinct world per parent
  void propagate(DirtyTracker &dirty);           // per-tick incremental pass (local transforms + DirtyChildren re-index)
  std::uint64_t revision() const;               // monotonic; bumps on any world-matrix or index change (cheap cache key)
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

- Side-table design. Nothing is stored on the node objects. `DirtyTracker`, `TransformSystem`, and `BoundsSystem` each hold their own `unordered_map<const X3DNode *, …>` keyed by pointer. This keeps the generated node bindings unchanged and means any consumer can hold a raw pointer and query the tables.
- Incremental propagation. `TransformSystem::propagate` visits two kinds of change. For a `DirtyLocalTransform` it finds the minimal set of subtree roots (dirtied, no dirtied ancestor) and re-accumulates world matrices down from each. For a `DirtyChildren` (any `SFNode`/`MFNode` field, including `Shape.geometry`, or `visible` changing active traversal) it re-walks only that node's changed direct children: new children (and their whole subtrees, across every DEF/USE edge) are indexed and their worlds computed, removed children are dropped when no other parent still reaches them, and unchanged children are left untouched — `reindexChildren` (`runtime/scene/TransformSystem.hpp`). The render-item extractor separately takes one replacement snapshot for structural dirtiness (see [Extraction](extract.md#structural-delta-replacement-contract)). Removing an edge to a still-shared subtree triggers one transform-index rebuild from live roots, so non-Transform descendants and flattened Transform edges also get current ancestor frames. Switch/LOD selection changes are marked `DirtyActiveChild`, which leaves the transform and bounds indexes in place. The transform index caches unchanged descendant local matrices, so moving an ancestor recomposes only authored local changes; the cache refreshes on `DirtyLocalTransform` or `DirtyField`, and a GeoLocation/GeoTransform `geoSystem` write is classified `DirtyLocalTransform` so the geo frame is recomposed in the same tick. `BoundsSystem::propagate` ignores world-only dirty nodes. A local TRS edit invalidates parent unions and keeps the edited Transform's own local box; geometry and field edits invalidate their node and all parents before a memoized post-order recomputation. This updates every USE placement and joins diamond-shaped paths only after their children are current. A changed local Transform matrix invalidates parent unions even when that Transform's own local AABB is unchanged. An unchanged computed bound does not bump the bounds revision.
- Topology and lifetime. Indexes keep weak node identities, so a field replacement may immediately destroy the old geometry. Transform removal uses cached classification rather than dereferencing removed nodes. Bounds compares the direct children of structurally dirty nodes and rebuilds its topology and local bounds from live roots only when child identities actually change. Ordinary geometry/TRS updates remain incremental; structural updates are linear in distinct nodes and edges, not root-to-leaf paths. Rebuild scene indexes explicitly when changing scene roots.
- First-path vs per-parent world. `world_`/`worldTransform` keep the documented per-node first-path approximation (a USE-shared Transform under two parents has one cached world). `worldTransformUnder(parent, n)` resolves the world through a specific parent edge live (`worldTransform(parent) * localMatrix(n)`), so a DEF/USE instance under two parents reads back both matrices. The extractor and pick still re-accumulate per PATH themselves.
- Revision counter. `TransformSystem::revision()` is a monotonic `std::uint64_t` bumped by `buildIndex`, by any transform re-accumulation, and by any structural re-walk; a no-op tick leaves it unchanged. It is the intended cache key for consumers that memoize transform-derived state (e.g. a pick index).
- World bounds are lazy. `BoundsSystem::worldBounds` composes `localBounds` + `TransformSystem::worldTransform` on the fly; no separate world-AABB table is maintained. (A separate `worldTransformAny` walks up the parent index to resolve a target through its nearest ancestor Transform — for callers that need the world frame of a non-Transform node, e.g. `TransformSensor.targetObject`.)
- USE/DEF sharing guard. Both `TransformSystem::walk` and `BoundsSystem::index` use a `walked_` / `indexed_` visited set so a USE-shared node's subtree is only recursed once (avoiding a multiplicative explosion on heavy USE/DEF scenes). `TransformSystem::walk` still records a `children_` edge for every parent that reaches a shared Transform, so `worldTransformUnder()` can resolve the per-parent world; `BoundsSystem::index` still records every parent edge per reference so the bounds union stays correct.
- Cycle guard inside `compute`. `BoundsSystem::compute` uses a `computing_` gray-set to stop a recursive re-entry on a back-edge that escaped `CycleBreaker` (e.g., a back-edge reached only through `index`'s all-edge recording). The contribution of such a re-entrant node is zero.

### Seam points

- Field writes — the execution context classifies cascade deliveries and direct `writeField` changes. Structural node references mark child/bounds dirtiness; transform fields mark local transform dirtiness. See `X3DExecutionContext::classifyDirty` for the classification.
- `ExecutionContext` / per-tick driver — After the event cascade settles, the per-tick driver calls `TransformSystem::propagate(dirty)` then `BoundsSystem::propagate(dirty, ts)`. The order is enforced: bounds need the already-updated world transforms. (`DirtyTracker::clear()` is not called here — `X3DExecutionContext::tick` drops the previous tick's changed-set at the *start* of the next tick, before re-evaluating.)
- `TransformSystem::localMatrix(node, projection)` and `isTransform` are shared by bounds, picking, and extraction. Transform-bearing types include Transform, HAnimHumanoid, HAnimJoint, HAnimSite, CADPart, GeoLocation, and GeoTransform. Billboard rotation belongs to view-dependent walkers.
- `GeometryBounds::localGeometryBounds` — `BoundsSystem::compute` calls the free function `localGeometryBounds(node, projection)` from `GeometryBounds.hpp` to obtain a geometry leaf's own AABB. New geometry types are added here without changing `BoundsSystem`.
- Text bounds / FontMetrics seam — `localGeometryBounds(node, projection, fontMetrics)` lays a `Text` node's glyphs out through the same §15 engine the renderer uses (`extract::computeTextLayout` + `extract::textLayoutExtent`) and returns the exact glyph extent, so the bound never under-bounds the rendered mesh. `BoundsSystem::setFontMetrics` injects the seam; with no seam set (the default — the SDK is IO-free) `Text` falls back to the conservative heuristic, byte-for-byte unchanged.
- Author `bboxSize` / `bboxCenter` override — If a node carries both `bboxSize` fields with all components ≥ 0, `BoundsSystem::authorBounds` uses that box and skips the child union for that node (children still get their own entries computed).
- `CycleBreaker` / scene build — `breakContainmentCycles(scene)` is called once inside `buildSceneGraph` (in `runtime/events/X3DExecutionContext.hpp`) before any system indexes the scene. It requires read+write field access (`f.get` and `f.set`) to re-seat severed fields to null/empty. See [ADR-0016](../decisions/0016-cycle-breaker.md).

## Related

- [ADR-0016: Containment Cycles Severed Once at Scene Build](../decisions/0016-cycle-breaker.md)
- [ADR-0011: ECS-Flavored, Not ECS](../decisions/0011-ecs-flavored-not-ecs.md)
- [ADR-0009: sim snapshot-diff](../decisions/0009-sim-snapshot-diff.md)
- [Scene Graph](../subsystems/scene-graph.md)
- [Math](../subsystems/math.md)
