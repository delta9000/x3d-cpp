---
title: Extraction Pipeline
summary: SceneExtractor → MeshBuilder → PackedMesh + RenderItem — the pull-based geometry extraction seam for renderers.
tags: [subsystem, extract, scene-extractor, mesh-builder, packed-mesh, render-item, nurbs]
updated: 2026-06-27
related:
  - ../architecture.md
  - ../subsystems/extract-textures.md
  - ../subsystems/extract-topology.md
  - ../decisions/0015-extraction-pull-per-path.md
  - ../decisions/0040-nurbs-tessellation-first-party.md
---

# Extraction Pipeline

The extraction pipeline converts a live X3D scene graph into renderer-consumable
descriptors without coupling the SDK to any graphics API. It owns the boundary
between the scene-graph runtime and every downstream renderer (PoC, CAVE consumer,
or any other consumer). No node is modified; every output lives in side tables keyed
on `const X3DNode*` or dense integer handles.

## Purpose

The pipeline answers one question per frame: "given the current scene graph state,
what needs to be drawn, and how has that changed since last frame?" It produces
flat, POD-only descriptors (`RenderItem`, `MeshData`, `MaterialDesc`, `LightDesc`,
`CameraDesc`, `BackgroundDesc`) that a renderer can consume without parsing X3D
nodes directly. The split between a full snapshot (frame 0 and scene-reload) and
an incremental `delta()` (every subsequent frame) keeps per-frame cost proportional
to what actually changed, not to scene size.

## Key files

| File | Role |
|---|---|
| `runtime/extract/SceneExtractor.hpp` | Top-level extractor: visibility-aware DFS, path interning, reverse-index maintenance, `fullSnapshot()` + `delta()` |
| `runtime/extract/MeshBuilder.hpp` | Geometry-node → `MeshData` in the node's local frame; handles all composed, lattice, analytic, line, and point types |
| `runtime/extract/PackedMesh.hpp` | Binary slab descriptor for embedder-supplied geometry (glTF-accessor-compatible layout) |
| `runtime/extract/RenderItem.hpp` | Pure-POD descriptor layer: `PathKey`, `RenderItemId`, `GeomId`, `MeshData`, `MaterialDesc`, `LightDesc`, `CameraDesc`, `BackgroundDesc`, `RenderDelta` |
| `runtime/extract/TextureExtract.hpp` | Texture/material extraction + resolver threading (see [Texture extraction](extract-textures.md)) |
| `runtime/extract/MaterialSystem.hpp` | Appearance → `MaterialDesc` mapping (see [Texture extraction](extract-textures.md)) |
| `runtime/extract/LightSystem.hpp` | World-resolved `LightDesc` collection (see [Texture extraction](extract-textures.md)) |
| `runtime/extract/Topology.hpp` | `Topology` enum: `Triangles`, `Lines`, `Points` (see [Topology](extract-topology.md)) |
| `runtime/extract/NurbsEval.hpp` | Node-free NURBS math unit (`x3d::runtime::extract::nurbs`): Cox–de Boor basis, rational (weighted) curve/surface eval, periodic/closed handling, analytic surface normals — plain arrays, no X3D-node dependency (see [NURBS](#nurbs)) |

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
RenderDelta dn = ex.delta(); // one call per tick; asserts one-delta-per-tick.

// Item access by dense handle.
const RenderItem &it = ex.item(id); // id in delta.added / updatedTransform / etc.
std::size_t n        = ex.itemCount();

// Scene-level read-outs (recomputed per call).
CameraDesc    cam  = ex.camera();
BackgroundDesc bg  = ex.background();
std::vector<LightDesc> lights = ex.lights(); // fresh collect; or:
const std::vector<LightDesc>& snapLights = ex.snapshotLights(); // from last fullSnapshot()

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

The walk is bounded by a per-snapshot **`WalkBudget`** (`runtime/RecursionLimits.hpp`,
default `kMaxGraphWalkVisits` = 1'000'000, overridable via
`MeshBuildOptions.maxWalkVisits`) shared across light collection and the geometry
walk. An acyclic graph that fans out to `2^depth` paths emits one RenderItem
**per path** (intentional for USE-instancing), so the walk cannot dedupe — the
budget caps the node-visits and `budgetExceeded()` flags a partial result. See
[ADR-0037](../decisions/0037-graph-walk-traversal-budget.md).

**`RenderDelta`** is the single authoritative change channel:

```cpp
struct RenderDelta {
  std::vector<RenderItemId> added;
  std::vector<RenderItemId> removed;
  std::vector<RenderItemId> updatedTransform;
  std::vector<RenderItemId> updatedGeometry;
  std::vector<RenderItemId> updatedMaterial;
  bool cameraChanged    = false;
  bool backgroundChanged = false;
  bool lightsChanged    = false;
};
```

**`RenderItem`** (stored inside `SceneExtractor`, accessed via `item(id)`) carries:

- `path` (`PathKey`) — full root-to-leaf node pointer chain; the per-path identity.
- `worldTransform` (`Mat4`) — re-accumulated fresh per path, never from `TransformSystem::world_`.
- `geometry` (`GeomId`) — `{node*, contentVersion}`; equal GeomIds share GPU geometry.
- `geometry_ext` (`Geometry`) — union of AoS `MeshData` (default) and `PackedMesh` (binary resolver path).
- `material` (`MaterialDesc`) — full Phong/Physical/Unlit descriptor with textures.
- `mesh` (`MeshData`) — local-frame triangles used by `sceneWorldBounds()`.
- `lights` — indices into `snapshotLights()` for lights whose scope covers this placement.
- `beyondVisibilityLimit` — hint: item origin is past `Viewpoint.farDistance` / `NavigationInfo.visibilityLimit`.
- `castShadow` — `X3DShapeNode.castShadow` (X3D default `true`); whether this shape occludes light. Carried, not interpreted — the shadow-visibility query (technique-defined per §17) is a consumer/seam concern (see [ADR-0028](../decisions/0028-shadow-visibility-seam.md)).

**`buildLocalMesh`** is the MeshBuilder entry point:

```cpp
// Returns local-frame MeshData; empty mesh = unsupported or legitimately empty.
// `recognized` out-param: false = unknown geometry type (coverage gap), not called
// for legitimately-empty recognized types.
MeshData buildLocalMesh(const X3DNode *geom,
                        const MeshBuildOptions &opt = {},
                        bool *recognized = nullptr);
```

Geometry types handled:

- **Composed/indexed sets (T1/T2):** `IndexedFaceSet`, `IndexedTriangleSet`, `TriangleSet`, `IndexedTriangleFanSet`, `IndexedTriangleStripSet`, `IndexedQuadSet`, `TriangleFanSet`, `TriangleStripSet`, `QuadSet`
- **Height-grid lattice (T2/B5):** `ElevationGrid`, `GeoElevationGrid` (geo-projection embedder seam)
- **Attribute resolution (T3):** authored `Normal`/`Color`/`ColorRGBA`/`TextureCoordinate` resolved per corner; flat normals generated when no `Normal` is authored; `creaseAngle` smooth-normal post-pass (B6)
- **Analytic primitives (T4):** `Box`, `Sphere`, `Cone`, `Cylinder` — parametric tessellation driven by `MeshBuildOptions` density knobs
- **Extrusion (B3):** SCP-frame sweep with `beginCap`/`endCap`, implicit TC3 texcoords
- **Line/point topology (B4):** `IndexedLineSet`, `LineSet`, `PointSet` — `MeshData.topology = Lines/Points`, always unlit, `solid=false`
- **NURBS (NRB-1):** `NurbsCurve` → `Topology::Lines`, `NurbsPatchSurface` → `Topology::Triangles` with analytic normals + implicit `(u,v)` texcoords (see [NURBS](#nurbs))
- **Text (T-TEXT):** delegated to `buildTextMesh` (see [Text extraction](extract-text.md)); sets `MeshData.isGlyphMesh = true`

### NURBS

`NurbsEval.hpp` is a first-party, node-free math unit (namespace
`x3d::runtime::extract::nurbs`) that evaluates NURBS curves and surfaces over plain
arrays — Cox–de Boor basis, rational (weighted) homogeneous accumulation, periodic
`closed`/`uClosed`/`vClosed` wrap, and analytic surface normals via the quotient rule
(`∂S/∂u = (A_u − w_u·S)/w`, `n = normalize(S_u × S_v)` — no `creaseAngle` post-pass).
Sampling spans the valid domain `[knot[order−1], knot[numCP]]`; authored knot vectors of
the wrong length default to clamped-uniform; weights of the wrong length default to all-1.
Two thin `MeshBuilder` arms read the X3D fields and call it:

- **`NurbsCurve`** → resolves the `controlPoint` child, reads `knot`/`weight`/`order`/
  `tessellation`/`closed`, calls `tessellateCurve`, and emits expanded line-pair
  positions as `Topology::Lines` (`solid=false`, unlit — the B4 convention).
- **`NurbsPatchSurface`** → resolves the control net + `u*/v*` fields, calls
  `tessellateSurface`, and emits two triangles per grid cell as `Topology::Triangles`
  with populated analytic normals and implicit normalized `(u,v)` texcoords.

Both flip to `true` in `recognizedGeometryType()` (kept in lockstep with the dispatch).
`GeometryBounds.hpp` gives them control-point convex-hull AABB bounds (a NURBS
curve/surface lies within the AABB of its control net). The still-deferred NURBS nodes
(`NurbsTrimmedSurface`/`NurbsSweptSurface`/`NurbsSwungSurface`, NRB-3) stay unrecognized
and continue to route through the `externalGeometryResolver` fallback. The "first-party,
not a seam" rationale and the deferral set are in [ADR-0040](../decisions/0040-nurbs-tessellation-first-party.md).

### Seam points

- **`MeshBuildOptions` (embedder-configured)** — holds tessellation density knobs (`sphereRings`, `sphereSegments`, `radialSlices`), an optional `GeoProjection` callback (B5 geodesy seam — SDK never calls geodesy itself), a `FontMetrics` callback (T-TEXT — SDK never opens fonts), and an optional `externalGeometryResolver` (`std::function<PackedMesh(const X3DNode*, AssetResolver)>`) for Phase 1 binary geometry. All default-constructible; existing callers are source-compatible when options are added.

- **`TextureResolver` (embedder-configured)** — supplied at `SceneExtractor` construction; the SDK never decodes image bytes. The resolver is called per `TextureRef` with `Source::Url`; its result is threaded onto `TextureRef::resolvedPixels`. Default is `makeNullTextureResolver()` (always `Failed`; PoC white-fallback). See [Texture extraction](extract-textures.md).

- **`externalGeometryResolver` / `PackedMesh`** — Phase 1 binary geometry path. When `buildLocalMesh` returns `recognized=false` and an `externalGeometryResolver` is wired, the extractor calls it with the unrecognized geometry node. A non-empty `PackedMesh` (glTF-accessor-compatible byte slabs, `attrib_mask` bitmask, `VertexBufferView` per attribute) triggers `emitPacked()`, producing a `RenderItem` with `geometry_ext.kind == Geometry::Kind::Packed`. An empty `PackedMesh` signals Pending (silent retry next tick). See [Ext firewall](ext-firewall.md).

- **`X3DExecutionContext` (runtime dependency)** — provides `dirtyTracker()` (the `DirtyTracker` read by `delta()`), `now()` (for the one-delta-per-tick assert), `boundViewpoint()`, `boundBackground()`, `boundNavigationInfo()`, `viewMatrix()`, and `cameraWorldPosition()`. See [Execution context](execution-context.md).

- **`DirtyTracker` (runtime dependency)** — `delta()` reads `changedNodes()` and `flags(n)` exactly once per tick. Dirty flags consumed: `DirtyLocalTransform | DirtyWorldTransform` → transform re-accumulation; `DirtyField` → geometry content re-extract or material re-read; `DirtyChildren` → subtree re-walk from the cached entry matrix. See [Dirty/bounds/transform](dirty-bounds-transform.md).

> **Per-`delta()` transform memoization.** The transform re-accumulation walks each
> dirty item's full root→leaf `PathKey`, but `TransformSystem::localMatrix` (five
> reflective field reads + a quaternion compose) and `isTransform` are memoized in
> a per-`delta()` node cache, and each item is re-accumulated only once even when
> several dirty ancestors flag it. A shared ancestor of *N* dirty items is therefore
> recomposed once, not *N* times — the re-accumulation stays O(distinct transforms),
> not O(items × depth). The memo is path-independent (a node's local matrix depends
> only on its own fields), so the per-path product — and thus DEF/USE instancing —
> stays exact. Regression: `runtime/extract/tests/scene_extractor_delta_perf_test.cpp`.

- **`TransformSystem::localMatrix` (runtime dependency)** — called per path-ancestor during transform re-accumulation in `reaccumulateWorld()` and during the DFS `walk()`. The extractor never reads `TransformSystem::world_` (the first-path-only table); every world matrix is re-accumulated path-by-path. See [Dirty/bounds/transform](dirty-bounds-transform.md).

### Threading contract

Single-threaded producer+consumer. The mutable interning caches (`items_`, `index_`, the three `DepMap` reverse indices, `entryMatrix_`) are not safe to share across threads. This is a stated seam invariant.

### One-delta-per-tick contract

`delta()` asserts `snapped_` (a prior `fullSnapshot()` must have run) and that `ctx_.now()` has advanced since the last call. Calling `delta()` twice in one tick, or before any `fullSnapshot()`, trips the assert. This is intentional: `tick()` clears the dirty set at tick end, so a stale second call would silently drop changes.

## How it is tested

MeshBuilder and SceneExtractor each have dedicated unit tests. All targets are registered in `CMakeLists.txt` and run under `ctest --preset dev`.

| ctest target | What it covers |
|---|---|
| `x3d_render_item` | `RenderItem.hpp` descriptor-layer compile gate (pure-POD) |
| `x3d_mesh_builder_t2` | Strips/fans/quads + `ElevationGrid` with flat normals |
| `x3d_mesh_builder_t3` | Normal/Color/ColorRGBA/TextureCoordinate attribute resolution, `normalPerVertex`/`colorPerVertex`, flat-normal generation |
| `x3d_mesh_builder_t4` | Analytic primitive parametric tessellation (Box/Sphere/Cone/Cylinder) |
| `x3d_mesh_builder_b3` | Extrusion SCP-frame sweep + caps |
| `x3d_mesh_builder_b4` | Line/point topology (`IndexedLineSet`, `LineSet`, `PointSet`) |
| `x3d_mesh_builder_b5` | `GeoElevationGrid` lattice emission + `GeoProjection` seam |
| `x3d_mesh_builder_b6` | `creaseAngle` smooth-normal post-pass |
| `x3d_mesh_builder_tc1` | Implicit bbox-projection texcoords (no authored `TextureCoordinate`) |
| `x3d_mesh_builder_tc2` | Implicit grid texcoords (`ElevationGrid`/`GeoElevationGrid`) |
| `x3d_mesh_builder_tc3` | Extrusion implicit texcoords (chord-length S, spine-length T, cap bbox) |
| `x3d_mesh_builder_tc4` | Analytic primitive default UV mapping (seam-shifted S + T) |
| `x3d_mesh_builder_txc1` | Seam-shifted longitudinal S for analytic primitives (TXC-1) |
| `x3d_scene_extractor` | SceneExtractor core (early vertical slice) |
| `x3d_scene_extractor_t7` | Full visibility-aware DFS: Switch/LOD special-cases, real material/lights |
| `x3d_scene_extractor_t8` | `delta()` incremental engine: transform/geometry/material change dispatch + subtree re-walk |
| `x3d_scene_extractor_b2` | `skippedGeometryCounts()` coverage signal for unrecognized geometry types |
| `x3d_scene_extractor_col2` | `Collision.proxy` excluded from render set (COL-2) |
| `x3d_scene_extractor_cad1` | `CADFace.shape` traversed only for Shape/LOD/Transform children (CAD-1, §32.4.2) |
| `x3d_scene_extractor_m25_5` | Per-item light scoping (global vs. scoped, `scopeRoot` path-ancestor test) |
| `x3d_scene_extractor_audit` | Extractor conformance audit |
| `x3d_render_feed_audit` | End-to-end render-feed audit |
| `x3d_packed_mesh` | `PackedMesh` descriptor: `set_attrib`, `has()`, `empty()`, `is_indexed()` |
| `x3d_render_item_geometry` | `Geometry` union: AoS vs Packed kind switching |
| `x3d_light_system` | `LightSystem::collect()` world-resolution + global/scoped flag |
| `x3d_material_system` | `MaterialSystem::materialOf()` Phong/Physical/Unlit dispatch |
| `x3d_texture_extract` | Texture extraction + resolver threading (see [Texture extraction](extract-textures.md)) |

The full conformance extraction oracle is exercised by `x3d_extract_oracle_test` (registered separately in `CMakeLists.txt`).

## Related specs and ADRs

- [ADR-0015: Extraction pull per path](../decisions/0015-extraction-pull-per-path.md) — the core design decision: pull-based dirty-set read, per-path identity, geometry/material node-keyed
- [ADR-0037: Graph-walk traversal budget](../decisions/0037-graph-walk-traversal-budget.md) — bounds an acyclic "doubling DAG" fan-out: a `WalkBudget` node-visit cap on the per-path walk + light collection, surfaced as `budgetExceeded()`
- [ADR-0001: Ext Firewall](../decisions/0001-ext-firewall.md) — keeps binary/external geometry behind the `externalGeometryResolver` seam, out of the spec-correct core
- [ADR-0040: NURBS tessellation first-party](../decisions/0040-nurbs-tessellation-first-party.md) — `NurbsEval.hpp` curve+patch math is first-party (I/O-free, spec-prescribed), not a swap-tested seam; the resolver stays the unrecognized-geometry fallback for the deferred NURBS nodes
- [Texture extraction](extract-textures.md) — `TextureExtract`, `MaterialSystem`, `LightSystem`, `TextureResolver`, `AssetResolver`
- [Text extraction](extract-text.md) — `buildTextMesh`, `FontMetrics`, glyph-quad layout
- [Topology classification](extract-topology.md) — `Topology` enum and consumer contract
- [Dirty/bounds/transform](dirty-bounds-transform.md) — `DirtyTracker`, `TransformSystem`, `BoundsSystem` (upstream inputs to `delta()`)
- [Execution context](execution-context.md) — `X3DExecutionContext` (camera/background/dirty pull surface)
- [Ext firewall](ext-firewall.md) — `PackedMesh` + `externalGeometryResolver` live here behind the firewall
- Spec: `docs/superpowers/specs/2026-06-14-m25-extraction-poc-renderer-design.md` — the M2.5 design that established `SceneExtractor`, `RenderDelta`, and the pull-per-path model
- Spec: `docs/superpowers/specs/2026-06-18-binary-mesh-texture-abstractions.md` — Phase 1 `PackedMesh`/`TextureDesc` binary geometry and texture abstractions
