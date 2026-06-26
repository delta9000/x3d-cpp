---
title: "ADR-0015: Extraction as Pull over an Incremental Dirty Set"
summary: The extraction seam is a PULL of an incrementally-maintained dirty set; render-item identity is per-PATH with geometry/material node-keyed lookup.
tags: [adr, extraction, pull, dirty-set, per-path, render-item]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/extract.md
  - ../subsystems/dirty-bounds-transform.md
---

# ADR-0015: Extraction as Pull over an Incremental Dirty Set

## Status

Accepted — 2026-06-20

## Context

The M2.5 milestone introduced `runtime/extract/SceneExtractor.hpp` as the bridge between the X3D scene graph and renderer consumers. Two foundational questions had to be answered before any code was written, because they determine the shape of every API in the layer.

**Question 1: push or pull at the seam?**

A push seam would have `X3DExecutionContext` call the renderer after each tick, delivering changes directly. A pull seam reverses the dependency: the renderer calls `SceneExtractor` after each tick and receives a delta snapshot.

The push option was rejected because the SDK's design goal is a headless, renderer-agnostic runtime — renderers are consumers of the SDK, not collaborators wired into it. A push seam inverts that relationship and couples the execution context to a specific consumer interface. Push also conflicts with multi-consumer scenarios (the VR CAVE target has N wall processes consuming the same tick) where each consumer pulls on its own schedule.

The dirty set that drives a pull delta already exists: `X3DExecutionContext::tick()` accumulates `DirtyTracker dirty_` during the cascade, propagates it through `TransformSystem` and `BoundsSystem`, and — critically — **clears it at the end of every tick** (verified: `dirty_.clear()` at line 206 of `runtime/events/X3DExecutionContext.hpp`). This clear means the dirty set is exactly the diff of one advance. A pull extractor that reads `ctx.dirtyTracker()` immediately after each `tick()` gets a precise incremental change log for free; one that batches multiple ticks between reads would silently drop intermediate changes. The one-delta-per-tick constraint is therefore a structural property of the existing implementation, not an artificial restriction.

**Question 2: what is a RenderItem's identity?**

X3D allows a single `DEF`-named geometry node to appear under multiple parent `Transform` nodes via `USE` references. The scene graph is a DAG, not a tree. A node-keyed identity scheme — one RenderItem per geometry node — would assign one world transform to a node that legitimately has N placements, each with a distinct transform. This was already a known bug in `TransformSystem`: `world_` is keyed by `const X3DNode*`, so the second USE placement overwrites the first (the M2C-1 finding, verified at `runtime/scene/TransformSystem.hpp` lines 101–108, where a comment now states "world_ stays the documented per-node (first-path) approximation; the extractor re-accumulates per-path itself").

The correct identity for a renderable is the full root-to-leaf **path** through the DAG: a `PathKey = std::vector<const X3DNode*>`. One DEF'd `Shape` USE'd under two `Transform` nodes yields two `RenderItem`s sharing the same geometry and material but with distinct `PathKey`s and distinct world transforms. `RenderItemId` is a dense `uint32_t` handle into an interning table keyed by full-vector equality; the hash is only a hashmap bucket key, never identity (avoiding the hash-collision-aliasing flaw).

Geometry and material are the exception: they are **node-keyed**, not path-keyed, because they are legitimately frame-invariant and USE-shared. A `GeomId = { const X3DNode* node; uint32_t contentVersion; }` lets the same mesh upload once and instance across all placements. `MaterialDesc` is computed per appearance node and shared by value.

**Why the extractor must never read `ctx.worldTransform()`**

`TransformSystem::worldTransform(n)` returns the first-path value for node `n`. Reading it for a RenderItem matrix would produce the wrong transform for any USE-shared subtree. Instead, `SceneExtractor` accumulates `worldM` fresh down each path during its DFS — exactly the idiom already proven by `PickSystem::worldOfRec` — and caches each interior path-prefix node's accumulated entry matrix in `entryMatrix_`. This cache is what lets incremental `DirtyChildren` subtree re-walks resume in O(subtree) rather than re-walking from the root.

**The three reverse indices**

A single reverse index from path ancestors to RenderItemIds would miss material changes. `classifyDirty` marks a `Material` or `Appearance` field change only on the owning node, which sits below the `Shape` in the graph and is neither a path ancestor nor the geometry leaf. To route material changes to their affected RenderItems, `SceneExtractor` maintains three separate reverse maps populated during the DFS walk:

- `transformDeps_` — every `Transform` path-ancestor node → its dependent `RenderItemId`s
- `geomDeps_` — the geometry node → its dependent `RenderItemId`s
- `materialDeps_` — every appearance-subtree node (the `Appearance`, `Material`, each texture and `TextureProperties` reachable from the `Shape`) → its dependent `RenderItemId`s

Without the three-way split, a material field change that fires `DirtyField` on the material node would find no entry in a transform-only reverse index and silently produce no `updatedMaterial` output.

**`updatedGeometry` is gated on content fields, not `DirtyBounds`**

`BoundsSystem::recomputeUp` marks `DirtyBounds` on nodes during pure ancestor-scale animation. `classifyDirty` also sets `DirtyBounds` on a Transform's `scale` field write. Neither implies that the mesh geometry changed. Gating `updatedGeometry` on the coarse `DirtyBounds` bit would cause every scale-animated frame to re-extract and re-upload unchanged meshes. The extractor instead bumps `GeomId.contentVersion` only when a `DirtyField` whose field name is in the geometry-content subset (`coord`, `coordIndex`, `point`, `height`, `radius`, `size`, `index`, …) is observed on a geometry node.

## Decision

We decided that:

1. **The extraction seam is PULL.** The renderer calls `SceneExtractor::fullSnapshot()` once (frame 0) and `SceneExtractor::delta()` once per subsequent `tick()`, receiving a `RenderDelta` that carries `added`, `removed`, `updatedTransform`, `updatedGeometry`, and `updatedMaterial` item lists plus `cameraChanged`, `backgroundChanged`, and `lightsChanged` bits. The SDK never calls the renderer. The one-delta-per-tick constraint is asserted at the API level: calling `delta()` twice without an intervening `tick()` is a contract violation, not a recoverable state.

2. **RenderItem identity is per-PATH.** A `RenderItemId` indexes into a path-interning table keyed by full `PathKey` vector equality. The extractor accumulates world transforms fresh down each path — never reading `TransformSystem::world_` / `ctx.worldTransform()` for item matrices. Three reverse indices (`transformDeps_`, `geomDeps_`, `materialDeps_`) + an interior-node entry-matrix cache (`entryMatrix_`) make incremental updates O(path) or O(subtree) per changed item.

3. **Geometry and material are node-keyed.** `GeomId` is `{ const X3DNode* node; uint32_t contentVersion }` — equal across USE placements so the renderer uploads once and instances N times. `MaterialDesc` is shared by value. `updatedGeometry` fires only on a content-field `DirtyField`, never on the coarse `DirtyBounds` bit.

4. **`X3DExecutionContext` is unchanged.** The extractor is a free `const&` consumer over the existing public surface (`dirtyTracker()`, `tick()`, `boundViewpoint()`, etc.). No new member is added to the execution context. The SDK/golden layer is byte-untouched.

## Consequences

**Positive:**

- The SDK remains headless and renderer-agnostic. Any consumer — the VR CAVE, an off-screen raycaster, a test harness — pulls deltas on its own schedule without the SDK knowing the consumer exists.
- USE-shared geometry and material are uploaded once and instanced across all placements at the renderer, with zero per-frame re-extraction for unchanged content.
- Incremental updates are structurally O(path) for transform changes and O(subtree) for topology changes; unchanged items are not touched.
- The `fullSnapshot()` / `delta()` asymmetry gives renderers a single upload code path: frame-0 `fullSnapshot()` returns every item as `added`, so the renderer's "new item" handler covers both first-frame initialization and scene-graph additions identically.
- The per-path world-transform accumulation closes M2C-1 (the first-path overwrite in `TransformSystem`) without modifying `TransformSystem` itself.
- `X3DExecutionContext` remains byte-identical; the golden gate is unaffected by the extraction layer.

**Trade-offs / costs:**

- **One delta per tick is an asserted contract, not an enforced one.** `X3DExecutionContext::tick()` clears `dirty_` unconditionally at line 206, so a consumer that advances the simulation N times before calling `delta()` silently loses the first N−1 ticks' changes. The assert fires on the second `delta()` call at the same clock value, which catches the fixed-timestep case but not the "advance then skip delta" case. A future accumulating-pending-change variant would be needed for fixed-timestep physics consumers.
- **Path interning has a per-node-on-path cost.** Each `PathKey` is a `std::vector<const X3DNode*>` of length equal to the depth of the path. Deep HAnim humanoid scenes (500–1369 joint nodes) can produce wide, deep PathKey sets. HAnim is deferred from the first PoC scope for exactly this reason; the per-path design is correct but the practical cost for deep humanoids has not been measured.
- **`entryMatrix_` must be rebuilt on `DirtyChildren`.** If a topology change (node added/removed from the graph) occurs in the same tick as a transform-only update, the entry matrix for the modified subtree must be refreshed before the transform re-accumulation runs, or a stale entry matrix is used. The `DirtyChildren` subtree re-walk in `delta()` rebuilds the cache for the affected prefix; tests must cover the DirtyChildren-then-transform ordering in the same frame.
- **`ctx.viewMatrix()` retains first-path semantics.** The bound `Viewpoint` is almost never USE-shared, so this is documented and acceptable, but a consumer should not rely on `viewMatrix()` being per-path for exotic scenes that USE the Viewpoint under multiple frames.

## Related

- [Architecture](../architecture.md)
- [Extract subsystem](../subsystems/extract.md) — `runtime/extract/SceneExtractor.hpp`, `MeshBuilder.hpp`, `RenderItem.hpp`, `PackedMesh.hpp`; the full layer-A implementation
- [Dirty bounds and transform](../subsystems/dirty-bounds-transform.md) — `runtime/scene/DirtyTracker.hpp`, `TransformSystem.hpp`, `BoundsSystem.hpp`; the dirty-set machinery whose one-tick clear is the structural precondition for the pull contract
- Design spec: `docs/superpowers/specs/2026-06-14-m25-extraction-poc-renderer-design.md` — the authoritative M2.5 design; the "Decisions resolved" section names each sub-decision and cites the code-verified constraints
- `runtime/extract/SceneExtractor.hpp` — the implementation; the file-top comment block restates the per-path identity and one-delta-per-tick contract as load-bearing inline documentation
- `runtime/events/X3DExecutionContext.hpp` — `tick()` at line 206 where `dirty_.clear()` establishes the one-tick dirty window; `dirtyTracker()` accessor; `classifyDirty` binding at line 86
- `runtime/scene/TransformSystem.hpp` — `world_` (the first-path-only table); lines 101–108 comment documents the documented approximation and defers to the extractor for per-path correctness
- `runtime/scene/PickSystem.hpp` — `worldOfRec` is the prior art proving that fresh per-path accumulation is feasible and already correct in the codebase
