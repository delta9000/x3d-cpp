---
title: Extraction pipeline
summary: SceneExtractor → MeshBuilder → PackedMesh + RenderItem — the pull-based geometry extraction seam for renderers.
tags: [subsystem, extract, scene-extractor, mesh-builder, packed-mesh, render-item, nurbs]
updated: 2026-09-30
related:
  - ../../explanation/architecture.md
  - extract-textures.md
  - extract-topology.md
  - extract-deltas.md
  - extract-geometry.md
  - gltf-export.md
  - ../../explanation/decisions/0015-extraction-pull-per-path.md
  - ../../explanation/decisions/0040-nurbs-tessellation-first-party.md
---

# Extraction pipeline

- The extraction pipeline converts a live X3D scene graph into
  renderer-consumable descriptors without coupling the SDK to any graphics
  API.
- It owns the boundary between the scene-graph runtime and every downstream
  renderer (PoC, CAVE consumer, or any other consumer).
- Text layout output fields are updated during extraction; render data is
  returned through descriptors.
- Every output lives in side tables keyed on `const X3DNode*` or dense integer
  handles.

## Purpose

The pipeline answers one question per frame: "given the current scene graph state, what needs to be drawn, and how has that changed since last frame?" It produces flat, POD-only descriptors (`RenderItem`, `MeshData`, `MaterialDesc`, `LightDesc`, `CameraDesc`, `BackgroundDesc`) that a renderer can consume without parsing X3D nodes directly.

## Behavior

- The split between a full snapshot (frame 0 and scene-reload) and an
  incremental `delta()` keeps geometry, material, and TRS refreshes
  proportional to the affected items.
- Switch and LOD selection changes keep path IDs and mesh caches.
- Structural and scoped-descriptor changes take one bounded full-scene
  replacement walk per affected tick; correctness comes before preserving the
  old subtree optimization.
- In scenes with lights, each advancing `delta()` re-collects the light list
  that items index; `lightsChanged` is set only when the published list
  differs.

## Key files

| File | Role |
|---|---|
| `runtime/extract/SceneExtractor.hpp` | Top-level extractor: visibility-aware DFS, path interning, reverse-index maintenance, `fullSnapshot()` + `delta()` |
| `runtime/extract/MeshBuilder.hpp` | Geometry-node → `MeshData` in the node's local frame; handles all composed, lattice, analytic, line, and point types |
| `runtime/extract/GltfWriter.hpp`, `.cpp` | `writeGlb`: render items → binary glTF 2.0 (see [glTF export](gltf-export.md)) |
| `runtime/extract/PackedMesh.hpp` | Binary slab descriptor for embedder-supplied geometry (glTF-accessor-compatible layout) |
| `runtime/extract/RenderItem.hpp` | Pure-POD descriptor layer: `PathKey`, `RenderItemId`, `GeomId`, `MeshData`, `MaterialDesc`, `LightDesc`, `CameraDesc`, `BackgroundDesc`, `FogDesc`, `LocalFogDesc`, `RenderDelta` |
| `runtime/extract/TextureExtract.hpp` | Texture/material extraction + resolver threading (see [Texture extraction](extract-textures.md)) |
| `runtime/extract/MaterialSystem.hpp` | Appearance → `MaterialDesc` mapping (see [Texture extraction](extract-textures.md)) |
| `runtime/extract/LightSystem.hpp` | World-resolved `LightDesc` collection (see [Texture extraction](extract-textures.md)) |
| `runtime/extract/LocalFogSystem.hpp` | Enabled `LocalFogDesc` collection, each scoped to its enclosing grouping placement path (§24.4.3) |
| `runtime/extract/Topology.hpp` | `Topology` enum: `Triangles`, `Lines`, `Points` (see [Topology](extract-topology.md)) |
| `runtime/extract/NurbsEval.hpp` | Node-free NURBS math unit (`x3d::runtime::extract::nurbs`): Cox–de Boor basis, rational (weighted) curve/surface eval, periodic/closed handling, analytic surface normals — plain arrays, no X3D-node dependency (see [NURBS](extract-geometry.md#nurbs)) |

## Interfaces and seams

### Exposed interface

The primary consumer-facing types are in `runtime/extract/RenderItem.hpp`; the
extractor itself is `SceneExtractor` in `runtime/extract/SceneExtractor.hpp`.

```cpp
namespace x3d::runtime::extract {

// Construct once per scene.
SceneExtractor ex(ctx, scene, meshOptions, textureResolver);

// Frame 0 (or scene reload): full walk, every item in delta.added.
RenderDelta d0 = ex.fullSnapshot();

// Subsequent frames: incremental; reads ctx.dirtyTracker() exactly once.
RenderDelta dn = ex.delta(); // one call per tick; a second with no tick = empty.

// Item access by dense handle.
const RenderItem &it = ex.item(id); // id in delta.added / updatedTransform / etc.
std::size_t n        = ex.itemCount();

// Scene-level read-outs (recomputed per call).
CameraDesc    cam  = ex.camera();
BackgroundDesc bg  = ex.background(); // sky/ground gradient + six panorama face TextureRefs
                                      // (Background *Url / TextureBackground *Texture) + transparency.
FogDesc       fog  = ex.fog(); // bound Fog (§24.4.2); visibilityRange world-scaled, 0 = off.
int           lf   = it.localFog; // §24.4.3: nearest in-scope LocalFog index, or -1.
const std::vector<LocalFogDesc>& localFogs = ex.snapshotLocalFogs(); // resolve lf here.
std::vector<LightDesc> lights = ex.lights(); // fresh collect; or:
const std::vector<LightDesc>& snapLights = ex.snapshotLights(); // refreshed by snapshots, selection walks, and advancing delta() in lit scenes

// World-bounds (per-path-correct, not first-path-only).
Aabb bounds = ex.sceneWorldBounds();

// Coverage signal: geometry types the builder could not tessellate.
const std::map<std::string,int>& gaps = ex.skippedGeometryCounts();

// DoS signal (#21): true if this walk hit MeshBuildOptions.maxWalkVisits and
// stopped early — the RenderDelta is then a PARTIAL view of a pathologically
// wide (acyclic "doubling DAG") scene. See ADR-0037.
bool partial = ex.budgetExceeded();

} // namespace x3d::runtime::extract
```

- The walk is bounded by a per-snapshot `WalkBudget`
  (`runtime/RecursionLimits.hpp`, default `kMaxGraphWalkVisits` = 1'000'000,
  overridable via `MeshBuildOptions.maxWalkVisits`) shared across light
  collection and the geometry walk.
- An acyclic graph that fans out to `2^depth` paths emits one RenderItem per
  path (intentional for USE-instancing), so the walk cannot dedupe — the
  budget caps the node-visits and `budgetExceeded()` flags a partial result.
- See
  [ADR-0037](../../explanation/decisions/0037-graph-walk-traversal-budget.md).

`RenderDelta` is the single authoritative change channel:

```cpp
struct RenderDelta {
  std::vector<RenderItemId> added;
  std::vector<RenderItemId> removed;
  std::vector<RenderItemId> updatedTransform;
  std::vector<RenderItemId> updatedGeometry;
  std::vector<RenderItemId> updatedSkinPose;
  std::vector<RenderItemId> updatedMaterial;
  bool cameraChanged    = false;
  bool backgroundChanged = false;
  bool lightsChanged    = false;
  bool fogChanged       = false;
};
```

- `lightsChanged` is false when the world-resolved `LightDesc` list is
  unchanged.
- `LightSystem::collect` records its own dependencies (`LightDependencies`):
  every node on a path to a light, including disabled lights, and to each
  Switch/LOD it selects through, plus the current choice of each LOD the
  geometry walk does not already track.
- These cover the graph the light walk reads, which includes subtrees the
  geometry walk skips (`visible=FALSE`, `Collision.proxy`, a GeoLOD `rootNode`
  beside loaded children), so a light change there notifies too.
- The `lightsChanged` comparison runs only when one of those dependencies is
  dirty or a selection changed, so unrelated animation leaves it false.
- Full snapshots initialize the consumer with `lightsChanged=true`; structural
  replacement deltas compare with the previous published list.
- Camera, background, and fog bits are set on every delta.
- `snapshotLights()` is refreshed by full snapshots, selection walks, and
  every advancing `delta()` in a scene that has lights (or when a light node
  changed), and live items are re-tagged against it.
- `snapshotLocalFogs()` refreshes on full snapshots and selection walks.

### RenderItem fields

`RenderItem` (stored inside `SceneExtractor`, accessed via `item(id)`) carries:

- `path` (`PathKey`) — full root-to-leaf node pointer chain; the per-path identity.
- `worldTransform` (`Mat4`) — re-accumulated fresh per path, never from `TransformSystem::world_`.
- `geometry` (`GeomId`) — `{node*, contentVersion}`; equal GeomIds share identical GPU geometry. `contentVersion` is opaque: baked/deformed variants of the same node receive distinct values, as do revised payloads. It is not a field-write counter. Full baselines reset the namespace.
- `geometry_ext` (`Geometry`) — union of AoS `MeshData` (default) and `PackedMesh` (binary resolver path).
- `material` (`MaterialDesc`) — full Phong/Physical/Unlit descriptor with textures.
- `mesh` (`MeshRef` = `shared_ptr<const MeshData>`) — local-frame triangles, shared across every placement of one `GeomId` ([ADR-0045](../../explanation/decisions/0045-shared-mesh-instancing.md)), so host RAM is O(distinct content) rather than O(placements). Never null (a Packed item points at `emptyMeshRef()`), so `item.mesh->positions` needs no null check. `external_geom_seam_test.cpp` verifies this on emitted packed items and checks their empty AoS scene-bounds channel. Immutable by contract: a content change builds a new mesh and bumps `GeomId::contentVersion` rather than editing one a co-owner can see.
- `lights` — indices into `snapshotLights()` for lights whose scope covers this placement.
  When `lightsChanged` is set, re-read the cache and each placement's `lightsOf()`
  indices together.
- `LightSystem` collects only lights reached through the same selected `Switch` child or distance-selected `LOD` level as geometry. It resolves locations and directions per path and scales PointLight/SpotLight radius through ancestor transforms (§17.4.2–3).
- `LocalFogSystem` collects a bound-independent `LocalFog` for each enclosing grouping placement.
    - `LocalFogDesc::scopePath` contains the complete root-to-enclosing-group node-pointer chain; matching requires an exact prefix of the item path.
    - The longest matching prefix wins, so USE-shared enclosing groups keep their own world-scaled `visibilityRange`, including shared nested scopes.
    - An empty scope path is scene-wide root fog.
    - Disabled fogs are skipped, leaving the nearest outer enabled fog or global `Fog`; `RenderItem::localFog == -1` selects global `Fog`.
    - Existing equal-scope tie behavior is retained: the last collected scoped fog wins, while the first root fog supplies the scene-wide fallback.
    - The walk keeps its shared visit budget and depth cap and rejects containment back-edges without deduplicating separate USE paths.
    - This does not change light scoping.

- `clipPlanes` (`ClipPlaneList`) — the enabled `ClipPlane` nodes (§11.4.1) in scope for this placement, resolved to world space (a plane's half-space is invariant, so a consumer maps it into its own frame — e.g. eye space — as needed). A `ClipPlane` affects the *following siblings and their subtrees* within its parent grouping node, threaded down the walk as scoped state. Fixed capacity — `ClipPlaneList::kMaxClipPlanes = 6` (the Annex F.5 minimum); planes beyond the sixth are dropped. `enabled=false` planes are ignored.
- `beyondVisibilityLimit` — hint: item origin is past `Viewpoint.farDistance` / `NavigationInfo.visibilityLimit`.
- `castShadow` — `X3DShapeNode.castShadow` (X3D default `true`); whether this shape occludes light. The extractor carries this flag; the CPU consumer applies it in triangle shadow queries and the GL PoC in its per-light depth maps (`shaders/shadow.glsl`, `shadow_gl_test.py`). The shadow-visibility query (technique-defined per §17) is a consumer/seam concern (see [ADR-0028](../../explanation/decisions/0028-shadow-visibility-seam.md)).

- `LocalFogDesc::scopeRoot` remains the enclosing group pointer (or null for
  root fog), but it is informational and cannot identify a USE placement by
  itself.
- The appended `scopePath` field preserves earlier member order, ordinary
  field access, and four-field aggregate initializers; code doing its own
  scope resolution must use the complete path, or consume the extractor's
  `RenderItem::localFog` index.
- This is an experimental descriptor layout change, not binary compatibility:
  the descriptor now owns a vector, requires recompilation, and cannot be
  copied or serialized as raw bytes.
- Pointer paths borrow scene-node lifetime, just like `RenderItem::path`; they
  are not persistent cross-scene IDs.
- Storage is proportional to the collected scope-path lengths.

### Seam points

- `MeshBuildOptions` configures tessellation density, font metrics, and the optional external geometry resolver. Geospatial conversion uses the execution context's retained `GeoProjection`; the extractor has no separate projection override. See [geospatial ownership](geospatial.md#world-ownership-and-migration).

- `TextureResolver` (embedder-configured) — supplied at `SceneExtractor` construction; the SDK never decodes image bytes. The resolver is called per `TextureRef` with `Source::Url`; its result is threaded onto `TextureRef::resolvedPixels`. Default is `makeNullTextureResolver()` (always `Failed`; PoC white-fallback). See [Texture extraction](extract-textures.md).

- `externalGeometryResolver` / `PackedMesh` — Phase 1 binary geometry path.
  When `buildLocalMesh` returns `recognized=false` and an
  `externalGeometryResolver` is wired, the extractor calls it with the
  unrecognized geometry node. A non-empty `PackedMesh` (glTF-accessor-compatible
  byte slabs, `attrib_mask` bitmask, `VertexBufferView` per attribute) triggers
  `emitPacked()`, producing a `RenderItem` with
  `geometry_ext.kind == Geometry::Kind::Packed`. An empty `PackedMesh` omits the
  placement as Pending. `delta()` retries Pending placements on later ticks and
  refreshes packed payloads after geometry field edits; Ready payloads need no
  readiness polling. See [Ext firewall](ext-firewall.md).

- `X3DExecutionContext` (runtime dependency) — provides `dirtyTracker()` (the `DirtyTracker` read by `delta()`), `tickGeneration()` (the monotonic advance count the one-delta-per-tick guard keys on — deliberately not `now()`, which an embedder may pause or replay), `boundViewpoint()`, `boundBackground()`, `boundNavigationInfo()`, `viewMatrix()`, and `cameraWorldPosition()`. See [Execution context](execution-context.md).

- `DirtyTracker` (runtime dependency) — `delta()` consumes the current tick's `changedNodes()` and `flags(n)` across its invalidation and update passes. Dirty flags consumed: `DirtyLocalTransform | DirtyWorldTransform` → transform re-accumulation; `DirtyField` → geometry content re-extract or material re-read; `DirtyChildren` → one authoritative replacement snapshot before traversing stale cached paths. See [Dirty/bounds/transform](dirty-bounds-transform.md).

> Per-`delta()` transform memoization. The transform re-accumulation walks each
> dirty item's full root→leaf `PathKey`, but `TransformSystem::localMatrix` (five
> reflective field reads + a quaternion compose) and `isTransform` are memoized in
> a per-`delta()` node cache, and each item is re-accumulated only once even when
> several dirty ancestors flag it. A shared ancestor of *N* dirty items is therefore
> recomposed once, not *N* times — the re-accumulation stays O(distinct transforms),
> not O(items × depth). The memo is path-independent (a node's local matrix depends
> only on its own fields), so the per-path product — and thus DEF/USE instancing —
> stays exact. Regression: `runtime/extract/tests/scene_extractor_delta_perf_test.cpp`.

- `TransformSystem::localMatrix` (runtime dependency) — called per path-ancestor during transform re-accumulation in `reaccumulateWorld()` and during the DFS `walk()`. The extractor never reads `TransformSystem::world_` (the first-path-only table); every world matrix is re-accumulated path-by-path. See [Dirty/bounds/transform](dirty-bounds-transform.md).

### Threading contract

Single-threaded producer+consumer. The mutable interning caches (`items_`, `index_`, the three `DepMap` reverse indices, `entryMatrix_`) are not safe to share across threads. This is a stated seam invariant.

### One-delta-per-tick contract

`delta()` without a baseline returns `fullSnapshot()`. A second call without an
intervening tick returns an empty delta. The guard uses `tickGeneration()`, not
the supplied clock, so repeated timestamps remain valid. `tick()` clears the
dirty set at its start: consume each tick before advancing again.

## Split-out pages

| Page | Contents |
|---|---|
| [Extraction delta contracts](extract-deltas.md) | Update cost, content identity, view-dependent placements, structural and scoped replacement, visibility hints, geometry liveness |
| [Geometry extraction](extract-geometry.md) | `buildLocalMesh`, the geometry types handled, NURBS |
| [glTF export](gltf-export.md) | `writeGlb` and what it writes |

## Related

- [ADR-0015: Extraction pull per path](../../explanation/decisions/0015-extraction-pull-per-path.md)
- [ADR-0037: Graph-walk traversal budget](../../explanation/decisions/0037-graph-walk-traversal-budget.md)
- [ADR-0001: Ext Firewall](../../explanation/decisions/0001-ext-firewall.md)
- [ADR-0040: NURBS tessellation first-party](../../explanation/decisions/0040-nurbs-tessellation-first-party.md)
- [Extraction delta contracts](extract-deltas.md)
- [Geometry extraction](extract-geometry.md)
- [glTF export](gltf-export.md)
- [Texture extraction](extract-textures.md)
- [Text extraction](extract-text.md)
- [Topology classification](extract-topology.md)
- [Dirty/bounds/transform](dirty-bounds-transform.md)
- [Execution context](execution-context.md)
- [Ext firewall](ext-firewall.md)
- [Architecture](../../explanation/architecture.md)
