---
title: Extraction Pipeline
summary: SceneExtractor → MeshBuilder → PackedMesh + RenderItem — the pull-based geometry extraction seam for renderers.
tags: [subsystem, extract, scene-extractor, mesh-builder, packed-mesh, render-item, nurbs]
updated: 2026-09-30
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
or any other consumer). Text layout output fields are updated during extraction; render data is returned through descriptors. Every output lives in side tables keyed
on `const X3DNode*` or dense integer handles.

## Purpose

The pipeline answers one question per frame: "given the current scene graph state,
what needs to be drawn, and how has that changed since last frame?" It produces
flat, POD-only descriptors (`RenderItem`, `MeshData`, `MaterialDesc`, `LightDesc`,
`CameraDesc`, `BackgroundDesc`) that a renderer can consume without parsing X3D
nodes directly. The split between a full snapshot (frame 0 and scene-reload) and
an incremental `delta()` keeps geometry, material, and TRS refreshes
proportional to the affected items. Switch and LOD selection changes keep path
IDs and mesh caches. Structural and scoped-descriptor changes take one bounded
full-scene replacement walk per affected tick; correctness comes before
preserving the old subtree optimization. In scenes with lights, each advancing
`delta()` re-collects the light list that items index; `lightsChanged` is set
only when the published list differs.

## Key files

| File | Role |
|---|---|
| `runtime/extract/SceneExtractor.hpp` | Top-level extractor: visibility-aware DFS, path interning, reverse-index maintenance, `fullSnapshot()` + `delta()` |
| `runtime/extract/MeshBuilder.hpp` | Geometry-node → `MeshData` in the node's local frame; handles all composed, lattice, analytic, line, and point types |
| `runtime/extract/GltfWriter.hpp`, `.cpp` | `writeGlb`: render items → binary glTF 2.0 (see [glTF export](#gltf-export)) |
| `runtime/extract/PackedMesh.hpp` | Binary slab descriptor for embedder-supplied geometry (glTF-accessor-compatible layout) |
| `runtime/extract/RenderItem.hpp` | Pure-POD descriptor layer: `PathKey`, `RenderItemId`, `GeomId`, `MeshData`, `MaterialDesc`, `LightDesc`, `CameraDesc`, `BackgroundDesc`, `FogDesc`, `LocalFogDesc`, `RenderDelta` |
| `runtime/extract/TextureExtract.hpp` | Texture/material extraction + resolver threading (see [Texture extraction](extract-textures.md)) |
| `runtime/extract/MaterialSystem.hpp` | Appearance → `MaterialDesc` mapping (see [Texture extraction](extract-textures.md)) |
| `runtime/extract/LightSystem.hpp` | World-resolved `LightDesc` collection (see [Texture extraction](extract-textures.md)) |
| `runtime/extract/LocalFogSystem.hpp` | Enabled `LocalFogDesc` collection, each scoped to its enclosing grouping placement path (§24.4.3) |
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

The walk is bounded by a per-snapshot `WalkBudget` (`runtime/RecursionLimits.hpp`,
default `kMaxGraphWalkVisits` = 1'000'000, overridable via
`MeshBuildOptions.maxWalkVisits`) shared across light collection and the geometry
walk. An acyclic graph that fans out to `2^depth` paths emits one RenderItem
per path (intentional for USE-instancing), so the walk cannot dedupe — the
budget caps the node-visits and `budgetExceeded()` flags a partial result. See
[ADR-0037](../decisions/0037-graph-walk-traversal-budget.md).

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

`lightsChanged` is false when the world-resolved `LightDesc` list is unchanged.
`LightSystem::collect` records its own dependencies (`LightDependencies`): every
node on a path to a light, including disabled lights, and to each Switch/LOD it
selects through, plus the current choice of each LOD the geometry walk does not
already track. These cover the graph the light walk reads, which includes
subtrees the geometry walk skips (`visible=FALSE`, `Collision.proxy`, a GeoLOD
`rootNode` beside loaded children), so a light change there notifies too.
The `lightsChanged` comparison runs only when one of those dependencies is
dirty or a selection changed, so unrelated animation leaves it false. Full
snapshots initialize the consumer with `lightsChanged=true`; structural
replacement deltas compare with the previous published list. Camera, background,
and fog bits are set on every delta. `snapshotLights()` is refreshed by full
snapshots, selection walks, and every advancing `delta()` in a scene that has
lights (or when a light node changed), and live items are re-tagged against it.
`snapshotLocalFogs()` refreshes on full snapshots and selection walks.

### Scalar material update cost

A scalar appearance-subtree edit refreshes each affected placement once per tick,
even when several of its source nodes are dirty. It retains the reverse dependency
index: only SFNode/MFNode replacements can change those edges, and those already
use a structural baseline. This avoids rescanning a shared material's N-element
placement vector once for each of N placements (#144). The performance regression
uses a counted, non-rendered metadata child to prove scalar refresh does not walk
unchanged dependency subtrees; structural replacement and subsequent scalar writes
remain covered.

### Geometry content identity

A host cache keyed by `GeomId` can upload each immutable AoS payload once. The
extractor interns payload variants within each geometry owner: identical
TextureTransform parameters share their cached mesh and key; different baked UVs
or HAnimSegment deformation produce distinct content versions. The bake cache
includes the raw deformation payload as an input, so equal texture transforms do
not collapse different Segment positions (#137).

Geometry invalidation evicts that owner's current payload lookup, then assigns
fresh content identities. Versions are never reused inside a baseline, even when
only one Segment's placement changes. `fullSnapshot()` resets them under the
existing host replacement/eviction protocol. Exhausting the uint32 identity space
throws with a request to take a fresh full snapshot rather than aliasing an old
resource. Consumer-cache and shared-Segment regressions verify actual mesh bytes,
not just distinct CPU pointers.

### View-dependent placements

`delta()` checks recorded LOD selections in each placement's current world frame,
including selections whose branch emits no mesh. Switch and LOD selection
changes use a bounded active-scene walk retaining path IDs and immutable mesh
caches. The add/remove lists contain the active-set difference; unaffected
placements retain their IDs and mesh references. Dependencies are rebuilt for
active paths, so nested selections and DEF/USE placements remain independent.
Deselected geometry keeps its raw mesh cache as *dormant*, with its own source
index: a `DirtyField` on the geometry or a direct source node evicts it and
advances its content version, so later reactivation reads current fields. An
unedited level comes back with the same `GeomId` and mesh reference, so a viewer
sitting on an LOD range boundary does not rebuild or re-upload on each crossing.
HAnimSegment-displaced variants are still evicted on deselection. Scoped
light/fog index changes republish only records whose indices change; surviving
external packed items receive geometry updates because the resolver is called
again. The walk also re-derives every surviving record's material, textures,
shader and skin binding: a record whose payload differs (for example, a texture
that finished resolving) is listed in `updatedMaterial` (`updatedSkinPose` for
the skin binding). See
[ADR-0069](../decisions/0069-selection-deltas-and-cheap-ticks.md).
The node-level `level_changed` event alone cannot represent all USE placements.

Billboard descendants recompute their per-path transforms from the current
tracked eye/up and authored ancestor transforms. A camera-only change emits
`updatedTransform` only when the resulting matrix changes and retains immutable
mesh payloads. No-motion ticks do not add Billboard transform updates. Geometry
that becomes nonempty again also recomputes its current path frame before its
addition is published, including tracked motion while it was absent. Regression:
`runtime/extract/tests/scene_extractor_view_delta_test.cpp` (#136).

### Structural delta replacement contract

Any `DirtyChildren` (SFNode/MFNode writes, `visible` changes, and
`LayerSet.order` edits), a scalar `HAnimHumanoid` configuration edit, or a
scene topology revision replaces the extraction baseline once. Switch/LOD
selection uses `DirtyActiveChild` and the retained-path walk described under
[View-dependent placements](#view-dependent-placements); structural edits in
the same tick take precedence:
`removed` contains every previously live ID and `added` contains the complete
current snapshot. Apply removals before additions; dense IDs can occur in
both. Release their live content-cache entries before accepting new records;
actual GPU destruction remains fence-controlled by the consumer. IDs, borrowed
references and content-version counters must not be treated as persistent across
this boundary. The snapshot resets caches and content versions.

The full walk uses the existing traversal budget and may report a partial view
through `budgetExceeded()`. It intentionally costs O(visited scene paths) plus
mesh/material extraction, including unchanged placements, on structural ticks.
Scalar geometry/material/TRS ticks retain the incremental and shared-mesh paths
except for humanoid configuration and the descriptor dependencies described below.
This avoids stale dependencies after geometry replacement, forgotten reattachment
of a removed path, last-writer-only USE-group placement rebuilding, and walking
raw path pointers after a removed subtree has already been destroyed.

`fullSnapshot()` called directly is also authoritative replacement, but returns
only `added`; the caller clears its previous state. `delta()` twice without a
new tick is empty. Consumers must consume each tick or explicitly rebaseline;
there is no retained history of missed ticks.

### Scoped render-state replacement contract

`Shape.castShadow`, `ClipPlane` fields and `LocalFog` fields do not have a dedicated
per-item update bucket in `RenderDelta`. Changes to their cached dependencies take
the same remove-all-before-add-all replacement path. `visible` is active
traversal state and is classified as `DirtyChildren`, including an initially hidden
branch that has no emitted item or reverse item dependency yet.

The dependency sets include Shape records (also recognized-empty geometry), every
encountered ClipPlane and LocalFog before testing `enabled`, and all static
Transform ancestors of each scoped descriptor placement. Ancestor TRS changes
therefore refresh world-space planes and world-scaled fog ranges, including every
USE placement. Dependencies are rebuilt on each replacement. They are collected
by the existing clip/fog walks, not by an additional per-tick scene traversal.
For ClipPlanes below a Billboard, the extractor also retains the per-placement
scope frame (including disabled planes and scopes without items). Each tick
compares only those stored paths against the current tracked eye/up; a changed
frame replaces the baseline. Billboard scalar edits participate in the scoped
node dependencies too. This adds O(recorded view-dependent clip-scope paths ×
path depth) checking; stable views and scenes without those scopes do not gain
replacement walks. Unrelated transforms, including a Shape's frame below an
already established clip/fog scope, remain incremental and retain their mesh
allocations.

Dirty tracking is node-granular: any `DirtyField` on an indexed Shape or scoped
source/frame conservatively replaces the snapshot. A scoped frame's local/world
transform dirtiness does too, even for disabled descriptors or empty scopes.
This deliberately costs a bounded full walk and re-extraction of unchanged
content on those ticks; it is a correctness fallback, not an O(changed-items)
claim. Replacement refreshes `snapshotLocalFogs()` and item indices together and
sets `fogChanged`. Consumers must not interpret a plain `updatedTransform` or
`updatedMaterial` as permission to refresh unrelated clip or shadow descriptors.

`scene_extractor_state_delta_test.cpp` exercises posted events through `tick()`
and compares a channel-respecting consumer mirror against a separate fresh
extractor: visibility, shadow flags, clip edits and frame changes, local fog
color/type/range/enabled and scale, disabled/empty/hidden scopes, shared placements,
unrelated TRS, coalesced edits, and camera-only/shared-empty Billboard clip
scopes with independent expected world-plane values. Shared enclosing LocalFog
groups also have explicit expected per-placement ranges before and after ancestor
scale changes. This establishes delta/snapshot consistency and those snapshot
semantics. The OpenGL example clips with these planes (`clip_plane_gl_test.py`) and fogs each item with its tagged LocalFog (`local_fog_gl_test.py`).

### Optional visibility-limit hints

`RenderItem::beyondVisibilityLimit` is an origin-distance hint, for both AoS
and packed placements: distance from the current world origin to the tracked eye
is strictly greater than positive `Viewpoint.farDistance`, or, if that is not
positive, positive `NavigationInfo.visibilityLimit`. Nonpositive effective limits
leave the hint false; equality is false. This is neither a bounds test nor a
frustum/occlusion guarantee. Geometry is still emitted and hosts may ignore it.

The hint has no narrow `RenderDelta` channel. Before consuming a tick, the extractor
compares relevant live hints with their current values. An actual boolean change
uses the existing bounded remove-all-before-add-all replacement. A host must
apply removals before additions and read the complete newly added records; plain
transform/material/camera notifications do not grant unrelated descriptor updates.
This catches far/fallback changes (including binding switches and finite-to-unlimited
clearing), eye motion and ancestor TRS. Added/revived geometry gets its current hint
even when the inputs changed while that placement was empty. Packed emission now
uses this same rule instead of always initializing the hint to false.

Unlimited-to-unlimited ticks do no per-item hint checking. With a finite limit,
changed eye/up/limit inputs require O(live items) distance comparisons; static
placements reuse their stored world origins. Dirty-frame and Billboard placements
instead recompose current paths, sharing a local-matrix cache (O(affected path
lengths)). Stable inputs inspect only dirty-frame dependencies; Billboard scalar
edits also check the recorded Billboard placements. No hint change means no extra
replacement or mesh allocation, including ordinary tracking ticks. A crossing does
cost a bounded full scene walk and re-extraction of unchanged meshes, just like
other unsupported per-item descriptor updates; this is an explicit correctness
fallback, not a fine-grained state channel or a replacement on every head update.

`scene_extractor_visibility_delta_test.cpp` checks explicit expected hint values
and a channel-respecting mirror against independent snapshots, including shared
placements, disabled/fallback limits, view motion, bindings, TRS, empty activation
and packed geometry. Billboard transform/LOD refresh remains the separate
view-dependent extraction contract; hint checks use current Billboard frames.

### Incremental geometry ownership and liveness

A Coordinate, Normal, Color or other direct content child can feed several
geometry owners. Each dirty source resolves to all of those owners, and
multiple dirty sources coalesce into one cache invalidation and content-version
advance per owner per tick. Each owner's placements rebuild from that owner's
primitive type; placements with identical geometry and bake parameters continue
to share immutable mesh storage. Unrelated meshes are retained. Scoped
HAnimSegment displacers still affect only placements within their Segment, even
when the geometry owner is shared.

Recognized-empty AoS geometry keeps its source dependencies and placement paths.
Becoming nonempty emits `added`; becoming empty emits `removed`, and replaces the
stored mesh with the empty result rather than retaining stale drawable content.
An already allocated placement reuses its ID when it becomes nonempty again.
Initially empty placements allocate no ID until their first nonempty result.
Transform/material edits while dormant are reflected when a placement returns,
and update buckets never target removed or newly added placements. This path
visits only affected geometry owners/placements, not the entire scene.

`itemCount()` counts allocated dense slots, which may include dormant removed
items; consumers maintain the live set from `added` and `removed`. Empty/nonempty
transitions are content changes, so they do not reset the baseline. The structural
replacement contract above still applies when node-valued fields change.
These statements concern native AoS geometry; packed resolver retry/liveness is
a separate seam.

`RenderItem` (stored inside `SceneExtractor`, accessed via `item(id)`) carries:

- `path` (`PathKey`) — full root-to-leaf node pointer chain; the per-path identity.
- `worldTransform` (`Mat4`) — re-accumulated fresh per path, never from `TransformSystem::world_`.
- `geometry` (`GeomId`) — `{node*, contentVersion}`; equal GeomIds share identical GPU geometry. `contentVersion` is opaque: baked/deformed variants of the same node receive distinct values, as do revised payloads. It is not a field-write counter. Full baselines reset the namespace.
- `geometry_ext` (`Geometry`) — union of AoS `MeshData` (default) and `PackedMesh` (binary resolver path).
- `material` (`MaterialDesc`) — full Phong/Physical/Unlit descriptor with textures.
- `mesh` (`MeshRef` = `shared_ptr<const MeshData>`) — local-frame triangles, shared across every placement of one `GeomId` ([ADR-0045](../decisions/0045-shared-mesh-instancing.md)), so host RAM is O(distinct content) rather than O(placements). Never null (a Packed item points at `emptyMeshRef()`), so `item.mesh->positions` needs no null check. `external_geom_seam_test.cpp` verifies this on emitted packed items and checks their empty AoS scene-bounds channel. Immutable by contract: a content change builds a new mesh and bumps `GeomId::contentVersion` rather than editing one a co-owner can see.
- `lights` — indices into `snapshotLights()` for lights whose scope covers this placement.
  When `lightsChanged` is set, re-read the cache and each placement's `lightsOf()`
  indices together.
- `LightSystem` collects only lights reached through the same selected `Switch` child or distance-selected `LOD` level as geometry. It resolves locations and directions per path and scales PointLight/SpotLight radius through ancestor transforms (§17.4.2–3).
- `LocalFogSystem` collects a bound-independent `LocalFog` for each enclosing grouping placement. `LocalFogDesc::scopePath` contains the complete root-to-enclosing-group node-pointer chain; matching requires an exact prefix of the item path. The longest matching prefix wins, so USE-shared enclosing groups keep their own world-scaled `visibilityRange`, including shared nested scopes. An empty scope path is scene-wide root fog. Disabled fogs are skipped, leaving the nearest outer enabled fog or global `Fog`; `RenderItem::localFog == -1` selects global `Fog`. Existing equal-scope tie behavior is retained: the last collected scoped fog wins, while the first root fog supplies the scene-wide fallback. The walk keeps its shared visit budget and depth cap and rejects containment back-edges without deduplicating separate USE paths. This does not change light scoping.

- `clipPlanes` (`ClipPlaneList`) — the enabled `ClipPlane` nodes (§11.4.1) in scope for this placement, resolved to world space (a plane's half-space is invariant, so a consumer maps it into its own frame — e.g. eye space — as needed). A `ClipPlane` affects the *following siblings and their subtrees* within its parent grouping node, threaded down the walk as scoped state. Fixed capacity — `ClipPlaneList::kMaxClipPlanes = 6` (the Annex F.5 minimum); planes beyond the sixth are dropped. `enabled=false` planes are ignored.
- `beyondVisibilityLimit` — hint: item origin is past `Viewpoint.farDistance` / `NavigationInfo.visibilityLimit`.
- `castShadow` — `X3DShapeNode.castShadow` (X3D default `true`); whether this shape occludes light. The extractor carries this flag; the CPU consumer applies it in triangle shadow queries and the GL PoC in its per-light depth maps (`shaders/shadow.glsl`, `shadow_gl_test.py`). The shadow-visibility query (technique-defined per §17) is a consumer/seam concern (see [ADR-0028](../decisions/0028-shadow-visibility-seam.md)).

`LocalFogDesc::scopeRoot` remains the enclosing group pointer (or null for root
fog), but it is informational and cannot identify a USE placement by itself.
The appended `scopePath` field preserves earlier member order, ordinary field
access, and four-field aggregate initializers; code doing its own scope resolution
must use the complete path, or consume the extractor's `RenderItem::localFog` index.
This is an experimental descriptor layout change, not binary compatibility:
the descriptor now owns a vector, requires recompilation, and cannot be copied or
serialized as raw bytes. Pointer paths borrow scene-node lifetime, just like
`RenderItem::path`; they are not persistent cross-scene IDs. Storage is proportional
to the collected scope-path lengths.

`buildLocalMesh` is the MeshBuilder entry point:

```cpp
// Returns local-frame MeshData; empty mesh = unsupported or legitimately empty.
// `recognized` out-param: false = unknown geometry type (coverage gap), not called
// for legitimately-empty recognized types.
MeshData buildLocalMesh(const X3DNode *geom,
                        const MeshBuildOptions &opt = {},
                        bool *recognized = nullptr);
```

Geometry types handled:

- Composed/indexed sets (T1/T2): `IndexedFaceSet`, `IndexedTriangleSet`, `TriangleSet`, `IndexedTriangleFanSet`, `IndexedTriangleStripSet`, `IndexedQuadSet`, `TriangleFanSet`, `TriangleStripSet`, `QuadSet`
- Height-grid lattice (T2/B5): `ElevationGrid`, `GeoElevationGrid` (geo-projection embedder seam). The shared `emitHeightGrid` honours authored `Color`/`Normal` and `colorPerVertex`/`normalPerVertex` per §13.3.4 (per-vertex → lattice vertex `row*xDim+col`, per-quad → cell `row*(xDim-1)+col`); EXT-001
- Attribute resolution (T3): authored `Normal`/`Color`/`ColorRGBA`/`TextureCoordinate` resolved per corner; flat normals generated when no `Normal` is authored; `creaseAngle` smooth-normal post-pass
- Analytic primitives (T4): `Box`, `Sphere`, `Cone`, `Cylinder` — parametric tessellation driven by `MeshBuildOptions` density knobs
- Extrusion : SCP-frame sweep with `beginCap`/`endCap`, implicit TC3 texcoords. Caps use the IndexedFaceSet ear clipper when `convex=FALSE`. Underdetermined (2-distinct-point / straight) spines use the ADR-0031 local-axis rule (Z = normalize(modelZ − (modelZ·Y)Y), fallback modelX; X = Y×Z) and <2 distinct spine points render nothing (§13.3.5.4.5); EXTRUSION-SCP
- Line/point topology : `IndexedLineSet`, `LineSet`, `PointSet` — `MeshData.topology = Lines/Points`, `solid=false`. Authored `Normal` values follow expanded vertices and enable lighting (§11.2.2.5). Without normals, consumers use `MaterialDesc::unlitGeometryRGBA()` or vertex colors; the fallback uses emissiveColor.
- Fog depths (§24.4.3): a `FogCoordinate` in `fogCoord` (composed, line and point sets, `ElevationGrid`) fills `MeshData::fogDepths`, parallel to `positions`: each corner takes the depth of its Coordinate point (`sourceCoordIndex`) or grid vertex (`latticeIndex`), a short list repeats its last depth, and depths clamp to [0,1]. Both reference renderers blend a fragment by its interpolated depth toward the applicable fog colour instead of the distance-derived interpolant.
- Custom vertex attributes (§31.4.2): `MeshData::vertexAttributes` carries named, fixed-width `FloatVertexAttribute`, `Matrix3VertexAttribute`, and `Matrix4VertexAttribute` streams. Their vertex-major values follow emitted positions through composed-geometry expansion and indexed coordinate lookup.
- Geometry2D (§14): the eight XY-plane primitives — `Arc2D`/`Circle2D`/`Polyline2D` → `Lines`, `Polypoint2D` → `Points` (unlit, `solid=false`), and `ArcClose2D` (PIE/CHORD)/`Disk2D` (fan + annulus; `innerRadius==outerRadius` → a circle line)/`Rectangle2D`/`TriangleSet2D` → `Triangles` with +Z normals and per-node `solid`. Circular primitives use one chord per `2π/64` rad (64 chords per full circle); texture coordinates map the geometry's XY bounding box to `[0,1]²`
- NURBS: `NurbsCurve` → `Topology::Lines`; `NurbsPatchSurface`, `NurbsSweptSurface` and `NurbsSwungSurface` → `Topology::Triangles` with analytic normals + implicit texcoords (see [NURBS](#nurbs))
- Text: delegated to `buildTextMesh` (see [Text extraction](extract-text.md)); sets `MeshData.isGlyphMesh = true`

### NURBS

`NurbsEval.hpp` is a first-party, node-free math unit (namespace
`x3d::runtime::extract::nurbs`) that evaluates NURBS curves and surfaces over plain
arrays — Cox–de Boor basis, rational (weighted) homogeneous accumulation, periodic
`closed`/`uClosed`/`vClosed` wrap, and analytic surface normals via the quotient rule
(`∂S/∂u = (A_u − w_u·S)/w`, `n = normalize(S_u × S_v)` — no `creaseAngle` post-pass).
Sampling spans the valid domain `[knot[order−1], knot[numCP]]`; authored knot vectors of
the wrong length default to clamped-uniform; weights of the wrong length default to all-1.
Two thin `MeshBuilder` arms read the X3D fields and call it:

- `NurbsCurve` → resolves the `controlPoint` child, reads `knot`/`weight`/`order`/
  `tessellation`/`closed`, calls `tessellateCurve`, and emits expanded line-pair
  positions as `Topology::Lines` (`solid=false`, unlit — the B4 convention).
- `NurbsPatchSurface` → resolves the control net + `u*/v*` fields, calls
  `tessellateSurface`, and emits two triangles per grid cell as `Topology::Triangles`
  with populated analytic normals and implicit normalized `(u,v)` texcoords.
- `NurbsSweptSurface` / `NurbsSwungSurface`  → read a 2D control curve
  (`crossSectionCurve`/`profileCurve` and the swung `trajectoryCurve`) via
  `tessellateCurve2D` — the shared evaluator lifted to z=0, with `ContourPolyline2D`
  forced to its piecewise-linear order 2 — plus a 3D `trajectoryCurve` for the swept
  case. `sweptSurfaceGrid` carries the cross-section in the frame perpendicular to the
  trajectory tangent (a circle swept along a straight line is a cylinder);
  `swungSurfaceGrid` builds the classic swung surface `S(u,v) = (px·tx, px·ty, py)`
  (a line profile swung around a circle is a surface of revolution). Both emit the same
  `Topology::Triangles` form as the patch surface (central-difference per-vertex normals,
  unit-square texcoords) and honor `solid`/`ccw`/`tessellation`.

All five flip to `true` in `recognizedGeometryType()` (kept in lockstep with the
dispatch). `GeometryBounds.hpp` gives them conservative AABB bounds (control-point
convex hull for curve/patch; for swept the trajectory hull ⊕ the cross-section radius;
for swung `|px|·|t|` in x/y with the profile height as z). `NurbsTrimmedSurface`
remains unrecognized — it needs 2D contour-loop point-in-region clipping plus polygon
triangulation in the `(u,v)` domain, and no such helper exists — and continues to route
through the `externalGeometryResolver` fallback. The "first-party, not a seam" rationale
is in [ADR-0040](../decisions/0040-nurbs-tessellation-first-party.md).

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

## glTF export

`extract::writeGlb(extractor, snapshot.added, options, &report)` (also
`sdk::writeGlb`) serializes render items to a GLB. It reads only the render
feed, so it exports what a renderer would be given: the scene after PROTO and
Inline expansion, with Switch and LOD selections applied and every
MeshBuilder geometry tessellated. `x3d extract in.x3d -o out.glb` is the CLI
form.

| X3D side | glTF side |
|---|---|
| One `RenderItem` | One root node with the item's world `matrix` (the grouping hierarchy is flattened), named after the nearest DEF on its path |
| One `MeshData` per material | One mesh; DEF/USE placements share its accessors |
| positions, normals, `texcoords` and further `texcoordSets`, per-vertex colours | `POSITION` (with min/max), `NORMAL` (normalized), `TEXCOORD_n` with `t` flipped to glTF's top-left origin, `COLOR_0` (RGBA float) |
| Triangles, Lines, Points | modes 4, 1, 0; `ccw=FALSE` triangles are turned round |
| `PhysicalMaterial` | `pbrMetallicRoughness` directly |
| `Material` | Approximation: diffuse → base colour, metallic 0, roughness `(2 / (128·shininess + 2))^¼`. Specular colour and ambient intensity are lost |
| `UnlitMaterial`, no material, lines/points without normals | `KHR_materials_unlit` with the emissive colour |
| `transparency` | base colour alpha with `alphaMode` BLEND; `AlphaMode::Mask` → MASK with its cutoff |
| `solid=FALSE` or a two-sided material | `doubleSided` |
| Textures in the base colour/diffuse, normal, emissive, occlusion and metallic-roughness slots | The matching glTF texture property. On an unlit surface (an `Appearance` with a texture and no `Material`, or `UnlitMaterial.emissiveTexture`) the emissive texture is the `baseColorTexture`, since `KHR_materials_unlit` shows the base colour. `repeatS/T` → sampler wrap |
| Texture images | A PNG or JPEG `ImageTexture` is embedded when `GltfWriteOptions::readImage` returns its bytes (the CLI reads local files, resolved against the texture's source document), so the GLB does not depend on where it is written; otherwise it is an external `images[].uri`, the url as authored. A PixelTexture, or any other url texture the host's `TextureResolver` has decoded → an embedded PNG (stored uncompressed). Identical images are written once |
| DirectionalLight, PointLight, SpotLight (`extractor.lights()`) | `KHR_lights_punctual` lights, each on a node with the world location and a rotation aiming −Z along the world direction. `radius` → `range`; `beamWidth`/`cutOffAngle` → inner/outer cone, with inner pulled inside outer |
| Viewpoint, OrthoViewpoint | Cameras on nodes carrying the viewpoint's world pose, the bound one first; `description` (else DEF) → name; `nearDistance`/`farDistance` → `znear`/`zfar` |
| PositionInterpolator / OrientationInterpolator routed to a Transform's `translation`, `scale` or `rotation` | A glTF animation: the Transform is kept as a node (see below), the interpolator's `key`/`keyValue` become a `LINEAR` sampler (rotations as quaternions), and key times are `key × cycleInterval` of the TimeSensor routed to the interpolator's `set_fraction`. One animation per TimeSensor, named by its DEF |
| PositionInterpolator / OrientationInterpolator routed to a Viewpoint's or OrthoViewpoint's `position` / `orientation`, or a PositionInterpolator routed to a PointLight's or SpotLight's `location` | Channels in the same animation: the camera or light node carries the authored value as its `translation`/`rotation`, under a `matrix` node for the Transforms above it |

Approximations: X3D light `intensity` is a unitless factor and is written
unchanged as glTF intensity (lux or candela), so absolute brightness will not
match a physically based viewer; `ambientIntensity`, the `attenuation`
coefficients and light scoping (`global=FALSE`) have no glTF equivalent and
are dropped. X3D `fieldOfView` spans the smaller viewport dimension and is
written as `yfov`, which is the same angle for a landscape viewport. An
OrthoViewpoint's off-centre extent is centred. Colours are copied as authored
with no sRGB/linear conversion.

`writeGlb(items, extras, ...)` takes the lights and cameras explicitly;
`gltfExtrasFrom(extractor)` collects them, and the extractor overload does both.

Colour, metallic, roughness and alpha factors are clamped to [0,1], so an
out-of-range authored value still gives a valid file.

Not written, each counted in `GltfWriteReport`: packed (binary) geometry items;
MovieTexture, MultiTexture, cube and 3D textures, textures on generated texture
coordinates and undecoded textures with no PNG/JPEG url (`droppedTextures`);
specular, shininess and ambient textures and the lighting maps of an unlit
surface (`unmappedTextureSlots`); back materials; author shaders; GeoViewpoint.
A normal map is written without a `TANGENT` attribute, so viewers derive the
tangent space. Skinned H-Anim
items are written as static meshes. Backgrounds and fog are not exported.

**Animated Transforms.** A scene with no exportable animation is written flat,
as above. Each Transform that an animation targets becomes glTF nodes, once per
placement (a USE'd Transform under two parents gives two node chains and two
channels on one sampler). X3D composes `T·C·R·SR·S·SR⁻¹·C⁻¹` where glTF has
only `T·R·S`, so the Transform is written as `{translation T+C, rotation R,
scale S}` with a child `{translation −C}` when it has a `center`, or as three
nodes `{T+C, R}` → `{rotation SR, scale S}` → `{translation SR⁻¹(−C), rotation
SR⁻¹}` when it has a `scaleOrientation`. Translation keys have the centre
added. Plain Transforms above an animated one collapse into one `matrix` node;
shapes below hang from the animated node with the remainder of their world
matrix, so every shape's composed matrix at export time equals its
`RenderItem::worldTransform`. A repeated `key` (an X3D jump) is nudged forward
by one float step, because glTF sample times must strictly increase.

**Lights and cameras.** A light or viewpoint below an animated Transform is a
child of that Transform's node, so it moves with it; one whose own `location`,
`position` or `orientation` is animated keeps that value as its node's
`translation`/`rotation` under a `matrix` node for its ancestors. Everything
else is still a world-space root.

Animation limits: core glTF animates only a node's translation, rotation and
scale (and morph weights), so those are the channels written. Interpolators
that drive colours, scalars (transparency, intensity, field of view) or a
light's `direction` would need the `KHR_animation_pointer` extension, which
most viewers do not load, and CoordinateInterpolators would need morph
targets; they are not exported. Neither are chains through other nodes
(sequencers, Scripts, EaseInEaseOut, followers), `center` and
`scaleOrientation` animation, and H-Anim motion. Dropped interpolator routes
are counted in `skippedAnimationRoutes`, and a channel whose target was not
written (a Transform with nothing exported under it, a light that is off) in
`skippedAnimationChannels`. glTF has no loop flag, so `TimeSensor.loop` is
left to the viewer.

The JSON and the PNG encoder are written by hand; the tests
(`runtime/extract/tests/gltf_writer_test.cpp`) check the container and
selected values, and exports of the CLI fixtures and the lion-head models pass
the Khronos glTF validator with no errors.

## Related

- [ADR-0015: Extraction pull per path](../decisions/0015-extraction-pull-per-path.md)
- [ADR-0037: Graph-walk traversal budget](../decisions/0037-graph-walk-traversal-budget.md)
- [ADR-0001: Ext Firewall](../decisions/0001-ext-firewall.md)
- [ADR-0040: NURBS tessellation first-party](../decisions/0040-nurbs-tessellation-first-party.md)
- [Texture extraction](extract-textures.md)
- [Text extraction](extract-text.md)
- [Topology classification](extract-topology.md)
- [Dirty/bounds/transform](dirty-bounds-transform.md)
- [Execution context](execution-context.md)
- [Ext firewall](ext-firewall.md)
