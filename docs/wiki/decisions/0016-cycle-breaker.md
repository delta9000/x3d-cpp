---
title: "ADR-0016: Containment Cycles Severed Once at Scene Build"
summary: Containment cycles in the scene graph are severed once in buildSceneGraph via breakContainmentCycles, converting the graph to a DAG before any recursive walker touches it.
tags: [adr, cycle-breaker, containment-cycle, dag, stack-overflow, bounds, scene-graph]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/dirty-bounds-transform.md
  - ../subsystems/scene-graph.md
---

# ADR-0016: Containment Cycles Severed Once at Scene Build

## Status

Accepted — 2026-06-17

## Context

X3D's USE mechanism resolves a name to an existing node reference at parse time. Malformed input can make a node its own ancestor: the canonical case is `<TouchSensor DEF='a' USE='a'/>` (a node USEing its own DEF name) — which resolves in memory to a genuine pointer cycle A → A. A less trivial form is a descendant USEing an enclosing DEF, producing A → B → A.

The runtime has many recursive scene-graph walkers:

- `BindingSystem::walk` — enrolls binding-stack nodes
- `PickSystem::worldOfRec` — computes world-space pick transforms
- `SceneExtractor::walk` — traverses the scene for geometry extraction
- `BoundsSystem::index` + `BoundsSystem::compute` — builds the bounds side-table and propagates AABBs bottom-up

None of these can use a naive `visited` set to detect back-edges without wrongly pruning **legitimate** USE-sharing — the same node reached by two distinct, non-ancestor paths is a valid DAG and must be traversed along each path that reaches it. A cycle check per-walker would require each walker to independently maintain an ancestor-path stack, duplicating fragile logic across every future walker.

This was a latent defect until the v1.0 confidence step widened the corpus smoke test to the full conformance archive (17,719 files). One malformed file — the deliberately-adversarial `TestSchematronDiagnostics.x3d` — triggered a stack overflow (SEGV) via every recursive walker. The fix had to be both correct (preserve DAG sharing) and applied exactly once rather than scattered across walkers.

`BoundsSystem` adds a second, independent defense: its `compute()` is post-order memoized (a node's `local_` entry is set only after all children finish), so a cycle's in-progress node is never memoized when re-entered — without a guard, `compute()` recurses the back-edge unboundedly even after `CycleBreaker` runs. The `computing_` in-progress set in `BoundsSystem::compute` (`runtime/scene/BoundsSystem.hpp`) closes this gap: on re-entry of an in-progress node, it returns an empty AABB immediately. This defense-in-depth guard is independent of `CycleBreaker` and handles any residual back-edge the scene graph might acquire after build time.

## Decision

We decided to run a single DFS sanitizer, `breakContainmentCycles(scene)` (`runtime/scene/CycleBreaker.hpp`), as the **first** step of `X3DExecutionContext::buildSceneGraph()` (`runtime/events/X3DExecutionContext.hpp`, line 77), before any walker (transform index, bounds build, binding enrollment, pick build) touches the scene. The sanitizer performs a standard gray/black DFS over all SFNode and MFNode fields: any child that is already on the current DFS path (on-stack = gray = an ancestor) is a containment back-edge and is severed — SFNode fields are nulled, MFNode entries are dropped. Legitimate USE-sharing (a node reachable by multiple distinct non-ancestor paths) is never severed. The function returns the number of edges removed (0 for any well-formed scene, making it a true no-op on valid content).

`BoundsSystem::compute` additionally guards its own in-progress traversal with a `computing_` set to catch any cycle that might exist in its `children_` side-table (which records all parent edges including back-edges accumulated before `breakContainmentCycles` ran, or edge sets acquired later).

## Consequences

**Positive:**

- Every recursive walker in the runtime traverses a guaranteed DAG. No walker needs its own cycle-detection logic; the invariant is established once at the site that owns scene-graph construction.
- The fix is a no-op on all well-formed scenes: `breakContainmentCycles` returns 0 and the only cost is one DFS pass over the scene, performed once at build time.
- Legitimate USE-sharing (diamond-shaped DAGs) is fully preserved. The gray/black DFS distinguishes "on the current ancestor path" from "already fully visited" so cross-edges and forward-edges are never cut.
- The function is idempotent: calling it twice on a DAG severs nothing on the second pass.
- The behavior is regression-pinned by two dedicated tests: `runtime/scene/tests/cycle_breaker_test.cpp` (direct contract: self-cycle, 2-cycle, DAG preservation) and `runtime/scene/tests/bounds_cycle_test.cpp` (end-to-end: cycle that previously caused a SEGV in `BoundsSystem::buildBounds` now terminates with finite bounds).

**Trade-offs / costs:**

- One extra DFS at scene-build time. For well-formed scenes the cost is a single O(N) traversal with no allocations beyond two `unordered_set<const X3DNode*>` whose population is bounded by the number of distinct nodes in the scene.
- The sanitizer operates by **mutation** (severing edges in-place). If a future caller needs to preserve the original pointer graph (e.g. to re-serialize malformed input faithfully), it must snapshot the graph before calling `buildSceneGraph`. Current writers read from the parsed `Scene` before `buildSceneGraph` is called, so there is no conflict today.
- `BoundsSystem`'s defense-in-depth guard (the `computing_` set) means `BoundsSystem::compute` silently contributes zero geometry for the back-reference node. This is the correct behavior (the back-reference adds no geometry not already accounted for by the legitimate forward path), but it is not surfaced as a warning. A future diagnostic pass could report severed edges if needed.

## Related

- [Architecture](../architecture.md)
- [Dirty Bounds and Transform Subsystem](../subsystems/dirty-bounds-transform.md)
- [Scene Graph Subsystem](../subsystems/scene-graph.md)
- Primary implementation: `runtime/scene/CycleBreaker.hpp`
- Call site: `runtime/events/X3DExecutionContext.hpp` (`buildSceneGraph`, line 77)
- Defense-in-depth guard: `runtime/scene/BoundsSystem.hpp` (`compute`, `computing_` set)
- Regression tests: `runtime/scene/tests/cycle_breaker_test.cpp`, `runtime/scene/tests/bounds_cycle_test.cpp`
- Discovery context: v1.0 confidence step (`ec9bc83`/`a67bf6a`), corpus sweep over 17,719 files
