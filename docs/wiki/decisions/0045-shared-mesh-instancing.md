---
title: "ADR-0045: Shared Mesh Storage — Make GeomId's Instancing Contract Structural"
summary: RenderItem::mesh becomes a shared, immutable MeshRef (shared_ptr<const MeshData>) backed by a content-keyed build cache in SceneExtractor, so N placements of one DEF'd geometry cost one tessellation and one allocation instead of N. Measured on 200 USEs of one 19,602-triangle IndexedFaceSet - 702 MiB to 3.3 MiB RSS, 1085 ms to 8.7 ms.
tags: [adr, extract, performance, instancing, memory, render-feed]
updated: 2026-07-15
related:
  - ../subsystems/scene-extraction.md
  - 0018-cave-cross-process-delta-contract.md
  - 0037-graph-walk-budget.md
---

# ADR-0045: Shared Mesh Storage — Make GeomId's Instancing Contract Structural

## Status

Accepted

## Context

`RenderItem.hpp` has always documented an instancing contract:

> positions/normals are LOCAL-frame — NEVER world-baked. The per-path world
> transform lives on the RenderItem side (worldTransform), so **one mesh is
> uploaded once and instanced across placements**.

`GeomId` (`{node, contentVersion}`) is the content-identity key that makes this
usable: the PoC renderer keys an `unordered_map<GeomId, GpuMesh>` on it, so N
placements sharing a `GeomId` upload **one** GPU buffer and instance N times.
That half worked, and still works.

The SDK side did not honour its own key. `RenderItem` held `MeshData` **by
value**, and `buildLocalMesh()` was called from inside `walk()` — once per
*placement*, not once per *geometry*. So the extractor:

1. re-tessellated identical content N times, then
2. retained N full copies of the resulting vertex/index/normal/texcoord vectors
   in host RAM,

in order to hand the consumer N descriptors that the consumer would immediately
de-duplicate using the very key the SDK had already computed.

Measured on a synthetic DEF/USE scene — one `IndexedFaceSet` (100×100 lattice,
19,602 triangles) `USE`'d under 200 `Transform`s:

| | Before |
|---|---|
| `buildLocalMesh` calls | 200 |
| Distinct `MeshData` allocations | 200 |
| Resident mesh bytes | 493.52 MiB |
| `fullSnapshot()` wall | 1084.98 ms |
| RSS delta | 702.43 MiB |

Per-instance cost was 5.42 ms against a 5.56 ms single build (0.98×) — proving
re-tessellation, not merely a copy (a 2.47 MiB memcpy would be ~0.1 ms).

The multiplier tracks *placement count*, so it is invisible in a teapot demo and
severe in exactly the scenes this SDK is pitched at: large DEF/USE worlds, CAD
assemblies, vegetation, CAVE/distributed scenes. This was reported by an external
reviewer as the most important technical risk in the pre-0.1 tree, and the
measurement confirmed it understated the problem (rebuild, not just duplicate).

## Decision

**`RenderItem::mesh` becomes `MeshRef` = `std::shared_ptr<const MeshData>`**, and
`SceneExtractor` gains a content-keyed build cache that is the single call site
of `buildLocalMesh()`.

Three properties are load-bearing:

1. **Shared** — one allocation per distinct content, co-owned by every placement.
   Host RAM becomes O(distinct content), matching what `GeomId` already claimed.
2. **Immutable** (`shared_ptr<const MeshData>`) — a mesh is never edited in place,
   because a co-owning placement would silently see the edit. The `delta()`
   geometry-change path therefore builds a **new** `MeshData` and bumps
   `GeomId::contentVersion`, which is precisely the signal a consumer's
   `GeomId`-keyed GPU cache already uses to orphan the stale buffer. The existing
   consumer contract needed no change to stay correct.
3. **Never null** — a Packed-geometry item (`emitPacked`) and a default-constructed
   `RenderItem` both point at a shared empty mesh (`emptyMeshRef()`), so
   `item.mesh->positions` is always safe without a null check.

### The cache key is (geometry, TextureTransform params) — not GeomId alone

The obvious key is the geometry node. That is wrong, and the subtlety is why this
is an ADR rather than a patch: `emit()` **bakes** the Appearance's
`TextureTransform` into `mesh.texcoords` (§18.4.10). The mesh is therefore a pure
function of *(geometry content, TextureTransform params)*, not of geometry alone —
one geometry `USE`'d under two different `TextureTransform`s legitimately yields
two different meshes.

So there are two caches:

* `rawMeshCache_` — keyed by geometry node; the pre-bake build.
* `bakedMeshCache_` — keyed by *(geometry node, serialized params)*; only ever
  populated when a `TextureTransform` is actually authored. The common
  untransformed case returns the raw mesh directly and allocates nothing extra,
  because `applyTextureTransformsToMesh()` is a documented no-op on an empty
  params list.

Params are serialized field-by-field, **not** by hashing the struct's bytes:
`TextureTransform2DParams` has padding after `hasMatrix`, and hashing
indeterminate padding would make cache hits nondeterministic.

### Lifetime

* `fullSnapshot()` **clears** both caches. A full walk is authoritative — it must
  re-read current field state, which is what makes a rebuild trustworthy. Within
  one walk the caches then collapse N placements onto one build.
* `delta()` **evicts** a geometry's entries (raw + every baked variant) when that
  geometry's content actually changes, *before* any arm can serve a stale entry,
  then rebuilds once through the same cache. The N dependents re-share one new
  allocation rather than taking N copies of it.

## Consequences

### Measured

Same scene, after:

| | Before | After |
|---|---|---|
| `buildLocalMesh` calls | 200 | **1** |
| Distinct `MeshData` allocations | 200 | **1** |
| Resident mesh bytes | 493.52 MiB | **2.47 MiB** |
| `fullSnapshot()` wall | 1084.98 ms | **8.71 ms** |
| RSS delta | 702.43 MiB | **3.26 MiB** |

~200× memory and ~125× wall on the instanced path; unchanged for a scene with no
instancing (one placement still costs one build).

### Source break (accepted)

`item.mesh.positions` becomes `item.mesh->positions`. This is a compile-time break
at every consumer read site — mechanical, loud, and impossible to get silently
wrong. At 0.1.0 with no tag, no ABI checker, and `CONTRIBUTING.md` already
advising exact pinning, a clean break is preferable to a compatibility shim that
would have to keep a by-value copy alive to be honest.

### Why not resource handles + resource tables?

The reviewer's suggested end-state was `MeshHandle`/`MaterialHandle` on
`RenderItem` plus separate `delta.addedMeshes`/`updatedMeshes`/`removedMeshes`
tables. That is a strictly larger redesign of the delta contract, and it is not
needed to remove the blowup: the consumer must still read the mesh **at least
once per `GeomId`** in order to upload it, and a shared immutable handle already
delivers that with O(distinct content) storage. `MeshRef` is also a strict
subset of that design — a later resource-table channel can be layered on without
re-litigating ownership, because ownership is now shared rather than exclusive.

Deferred to the Project board rather than done here: `MaterialDesc` is still held
by value per placement. It is orders of magnitude smaller than `MeshData` (no
vertex vectors), and its texture-pixel payload — the part that *is* large — is
addressed separately by the URL-keyed resolve memo and shared
`TexturePixelResult::pixels` (same wave, see `TextureResolver.hpp`).

### Regression guard

`runtime/extract/tests/scene_extractor_instancing_test.cpp` pins both halves —
build-once (`buildLocalMeshCallCount()` stays O(distinct geometry)) and share-once
(one allocation across all placements) — plus a **negative control** asserting
distinct geometry nodes do *not* share, so the cache cannot be "fixed" into a
global one-mesh-for-everything bug, and a delta-path case asserting a content
change rebuilds once, re-shares, and hands back a *new* pointer.

`buildLocalMeshCallCount()` follows the existing
`TransformSystem::localMatrixCallCount()` precedent (ADR-adjacent instrumentation
that lets a test assert an algorithmic property rather than a wall-clock time).
