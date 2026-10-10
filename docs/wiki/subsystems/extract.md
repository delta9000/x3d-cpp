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
or any other consumer). No node is modified; every output lives in side tables keyed
on `const X3DNode*` or dense integer handles.

## Purpose

The pipeline answers one question per frame: "given the current scene graph state,
what needs to be drawn, and how has that changed since last frame?" It produces
flat, POD-only descriptors (`RenderItem`, `MeshData`, `MaterialDesc`, `LightDesc`,
`CameraDesc`, `BackgroundDesc`) that a renderer can consume without parsing X3D
nodes directly. The split between a full snapshot (frame 0 and scene-reload) and
an incremental `delta()` keeps scalar/TRS updates proportional to affected items.
Structural/active-child and scoped-descriptor changes take one bounded full-scene
replacement walk per affected tick; correctness comes before preserving the old subtree optimization.

## Key files

| File | Role |
|---|---|
| `runtime/extract/SceneExtractor.hpp` | Top-level extractor: visibility-aware DFS, path interning, reverse-index maintenance, `fullSnapshot()` + `delta()` |
| `runtime/extract/MeshBuilder.hpp` | Geometry-node → `MeshData` in the node's local frame; handles all composed, lattice, analytic, line, and point types |
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
including selections whose branch emits no mesh. A changed per-path selection
uses the structural replacement contract below. The node-level `level_changed`
event alone cannot represent all USE placements.

Billboard descendants recompute their per-path transforms from the current
tracked eye/up and authored ancestor transforms. A camera-only change emits
`updatedTransform` only when the resulting matrix changes and retains immutable
mesh payloads. No-motion ticks do not add Billboard transform updates. Geometry
that becomes nonempty again also recomputes its current path frame before its
addition is published, including tracked motion while it was absent. Regression:
`runtime/extract/tests/scene_extractor_view_delta_test.cpp` (#136).

### Structural delta replacement contract

Any `DirtyChildren` (including SFNode/MFNode writes, `visible` changes and
Switch/LOD active-child changes), or a scene topology revision, replaces the extraction baseline once:
`removed` contains every previously live ID and `added` contains the complete
current snapshot. **Apply removals before additions**; dense IDs can occur in
both. Release their live content-cache entries before accepting new records;
actual GPU destruction remains fence-controlled by the consumer. IDs, borrowed
references and content-version counters must not be treated as persistent across
this boundary. The snapshot resets caches and content versions.

The full walk uses the existing traversal budget and may report a partial view
through `budgetExceeded()`. It intentionally costs O(visited scene paths) plus
mesh/material extraction, including unchanged placements, on structural ticks.
Scalar geometry/material/TRS ticks retain the incremental and shared-mesh paths
except for the descriptor dependencies described below.
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
the same **remove-all-before-add-all replacement** path. `visible` is active
traversal state and is classified as `DirtyChildren`, including an initially hidden
branch that has no emitted item or reverse item dependency yet.

The dependency sets include Shape records (also recognized-empty geometry), every
encountered ClipPlane and LocalFog **before** testing `enabled`, and all static
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

`RenderItem::beyondVisibilityLimit` is an **origin-distance hint**, for both AoS
and packed placements: distance from the current world origin to the tracked eye
is strictly greater than positive `Viewpoint.farDistance`, or, if that is not
positive, positive `NavigationInfo.visibilityLimit`. Nonpositive effective limits
leave the hint false; equality is false. This is neither a bounds test nor a
frustum/occlusion guarantee. Geometry is still emitted and hosts may ignore it.

The hint has no narrow `RenderDelta` channel. Before consuming a tick, the extractor
compares relevant live hints with their current values. An actual boolean change
uses the existing bounded **remove-all-before-add-all replacement**. A host must
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
geometry owners. Each dirty source resolves to **all** of those owners, and
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

**`RenderItem`** (stored inside `SceneExtractor`, accessed via `item(id)`) carries:

- `path` (`PathKey`) — full root-to-leaf node pointer chain; the per-path identity.
- `worldTransform` (`Mat4`) — re-accumulated fresh per path, never from `TransformSystem::world_`.
- `geometry` (`GeomId`) — `{node*, contentVersion}`; equal GeomIds share identical GPU geometry. `contentVersion` is opaque: baked/deformed variants of the same node receive distinct values, as do revised payloads. It is not a field-write counter. Full baselines reset the namespace.
- `geometry_ext` (`Geometry`) — union of AoS `MeshData` (default) and `PackedMesh` (binary resolver path).
- `material` (`MaterialDesc`) — full Phong/Physical/Unlit descriptor with textures.
- `mesh` (`MeshRef` = `shared_ptr<const MeshData>`) — local-frame triangles, **shared** across every placement of one `GeomId` ([ADR-0045](../decisions/0045-shared-mesh-instancing.md)), so host RAM is O(distinct content) rather than O(placements). Never null (a Packed item points at `emptyMeshRef()`), so `item.mesh->positions` needs no null check. `external_geom_seam_test.cpp` verifies this on emitted packed items and checks their empty AoS scene-bounds channel. Immutable by contract: a content change builds a **new** mesh and bumps `GeomId::contentVersion` rather than editing one a co-owner can see.
- `lights` — indices into `snapshotLights()` for lights whose scope covers this placement.
- `LightSystem` collects only lights reached through the same selected `Switch` child or distance-selected `LOD` level as geometry. It resolves locations and directions per path and scales PointLight/SpotLight radius through ancestor transforms (§17.4.2–3).
- `LocalFogSystem` collects a bound-independent `LocalFog` for each enclosing grouping **placement**. `LocalFogDesc::scopePath` contains the complete root-to-enclosing-group node-pointer chain; matching requires an exact prefix of the item path. The longest matching prefix wins, so USE-shared enclosing groups keep their own world-scaled `visibilityRange`, including shared nested scopes. An empty scope path is scene-wide root fog. Disabled fogs are skipped, leaving the nearest outer enabled fog or global `Fog`; `RenderItem::localFog == -1` selects global `Fog`. Existing equal-scope tie behavior is retained: the last collected scoped fog wins, while the first root fog supplies the scene-wide fallback. The walk keeps its shared visit budget and depth cap and rejects containment back-edges without deduplicating separate USE paths. This does not change light scoping.

- `clipPlanes` (`ClipPlaneList`) — the enabled `ClipPlane` nodes (§11.4.1) in scope for this placement, resolved to **world space** (a plane's half-space is invariant, so a consumer maps it into its own frame — e.g. eye space — as needed). A `ClipPlane` affects the *following siblings and their subtrees* within its parent grouping node, threaded down the walk as scoped state. Fixed capacity — `ClipPlaneList::kMaxClipPlanes = 6` (the Annex F.5 minimum); planes beyond the sixth are dropped. `enabled=false` planes are ignored.
- `beyondVisibilityLimit` — hint: item origin is past `Viewpoint.farDistance` / `NavigationInfo.visibilityLimit`.
- `castShadow` — `X3DShapeNode.castShadow` (X3D default `true`); whether this shape occludes light. The extractor carries this flag; the CPU consumer applies it in triangle shadow queries and the GL PoC in its per-light depth maps (`shaders/shadow.glsl`, `shadow_gl_test.py`). The shadow-visibility query (technique-defined per §17) is a consumer/seam concern (see [ADR-0028](../decisions/0028-shadow-visibility-seam.md)).

`LocalFogDesc::scopeRoot` remains the enclosing group pointer (or null for root
fog), but it is informational and cannot identify a USE placement by itself.
The appended `scopePath` field preserves earlier member order, ordinary field
access, and four-field aggregate initializers; code doing its own scope resolution
must use the complete path, or consume the extractor's `RenderItem::localFog` index.
This is an **experimental descriptor layout change**, not binary compatibility:
the descriptor now owns a vector, requires recompilation, and cannot be copied or
serialized as raw bytes. Pointer paths borrow scene-node lifetime, just like
`RenderItem::path`; they are not persistent cross-scene IDs. Storage is proportional
to the collected scope-path lengths. No stable public SDK facade changed.

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
- **Height-grid lattice (T2/B5):** `ElevationGrid`, `GeoElevationGrid` (geo-projection embedder seam). The shared `emitHeightGrid` honours authored `Color`/`Normal` and `colorPerVertex`/`normalPerVertex` per §13.3.4 (per-vertex → lattice vertex `row*xDim+col`, per-quad → cell `row*(xDim-1)+col`); EXT-001
- **Attribute resolution (T3):** authored `Normal`/`Color`/`ColorRGBA`/`TextureCoordinate` resolved per corner; flat normals generated when no `Normal` is authored; `creaseAngle` smooth-normal post-pass (B6)
- **Analytic primitives (T4):** `Box`, `Sphere`, `Cone`, `Cylinder` — parametric tessellation driven by `MeshBuildOptions` density knobs
- **Extrusion (B3):** SCP-frame sweep with `beginCap`/`endCap`, implicit TC3 texcoords. Caps use the IndexedFaceSet ear clipper when `convex=FALSE`. Underdetermined (2-distinct-point / straight) spines use the ADR-0031 local-axis rule (Z = normalize(modelZ − (modelZ·Y)Y), fallback modelX; X = Y×Z) and <2 distinct spine points render nothing (§13.3.5.4.5); EXTRUSION-SCP
- **Line/point topology (B4):** `IndexedLineSet`, `LineSet`, `PointSet` — `MeshData.topology = Lines/Points`, `solid=false`. Authored `Normal` values follow expanded vertices and enable lighting (§11.2.2.5). Without normals, consumers use `MaterialDesc::unlitGeometryRGBA()` or vertex colors; the fallback uses emissiveColor.
- **Custom vertex attributes (§31.4.2):** `MeshData::vertexAttributes` carries named, fixed-width `FloatVertexAttribute`, `Matrix3VertexAttribute`, and `Matrix4VertexAttribute` streams. Their vertex-major values follow emitted positions through composed-geometry expansion and indexed coordinate lookup.
- **Geometry2D (§14):** the eight XY-plane primitives — `Arc2D`/`Circle2D`/`Polyline2D` → `Lines`, `Polypoint2D` → `Points` (unlit, `solid=false`), and `ArcClose2D` (PIE/CHORD)/`Disk2D` (fan + annulus; `innerRadius==outerRadius` → a circle line)/`Rectangle2D`/`TriangleSet2D` → `Triangles` with +Z normals and per-node `solid`. Circular primitives use one chord per `2π/64` rad (64 chords per full circle); texture coordinates map the geometry's XY bounding box to `[0,1]²`
- **NURBS (NRB-1, NRB-3):** `NurbsCurve` → `Topology::Lines`; `NurbsPatchSurface`, `NurbsSweptSurface` and `NurbsSwungSurface` → `Topology::Triangles` with analytic normals + implicit texcoords (see [NURBS](#nurbs))
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
- **`NurbsSweptSurface` / `NurbsSwungSurface`** (NRB-3) → read a 2D control curve
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
for swung `|px|·|t|` in x/y with the profile height as z). `NurbsTrimmedSurface` (NRB-3)
remains unrecognized — it needs 2D contour-loop point-in-region clipping plus polygon
triangulation in the `(u,v)` domain, and no such helper exists — and continues to route
through the `externalGeometryResolver` fallback. The "first-party, not a seam" rationale
is in [ADR-0040](../decisions/0040-nurbs-tessellation-first-party.md).

### Seam points

- **`MeshBuildOptions` (embedder-configured)** — holds tessellation density knobs (`sphereRings`, `sphereSegments`, `radialSlices`), an optional `GeoProjection` callback (B5 geodesy seam — SDK never calls geodesy itself), a `FontMetrics` callback (T-TEXT — SDK never opens fonts), and an optional `externalGeometryResolver` (`std::function<PackedMesh(const X3DNode*, AssetResolver)>`) for Phase 1 binary geometry. All default-constructible; existing callers are source-compatible when options are added.

- **`TextureResolver` (embedder-configured)** — supplied at `SceneExtractor` construction; the SDK never decodes image bytes. The resolver is called per `TextureRef` with `Source::Url`; its result is threaded onto `TextureRef::resolvedPixels`. Default is `makeNullTextureResolver()` (always `Failed`; PoC white-fallback). See [Texture extraction](extract-textures.md).

- **`externalGeometryResolver` / `PackedMesh`** — Phase 1 binary geometry path. When `buildLocalMesh` returns `recognized=false` and an `externalGeometryResolver` is wired, the extractor calls it with the unrecognized geometry node. A non-empty `PackedMesh` (glTF-accessor-compatible byte slabs, `attrib_mask` bitmask, `VertexBufferView` per attribute) triggers `emitPacked()`, producing a `RenderItem` with `geometry_ext.kind == Geometry::Kind::Packed`. An empty `PackedMesh` omits the placement as Pending. Once the host has geometry ready, take a fresh `fullSnapshot()` to retry the resolver. See [Ext firewall](ext-firewall.md).

- **`X3DExecutionContext` (runtime dependency)** — provides `dirtyTracker()` (the `DirtyTracker` read by `delta()`), `tickGeneration()` (the monotonic advance count the one-delta-per-tick guard keys on — deliberately not `now()`, which an embedder may pause or replay), `boundViewpoint()`, `boundBackground()`, `boundNavigationInfo()`, `viewMatrix()`, and `cameraWorldPosition()`. See [Execution context](execution-context.md).

- **`DirtyTracker` (runtime dependency)** — `delta()` consumes the current tick's `changedNodes()` and `flags(n)` across its invalidation and update passes. Dirty flags consumed: `DirtyLocalTransform | DirtyWorldTransform` → transform re-accumulation; `DirtyField` → geometry content re-extract or material re-read; `DirtyChildren` → one authoritative replacement snapshot before traversing stale cached paths. See [Dirty/bounds/transform](dirty-bounds-transform.md).

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

`delta()` without a baseline returns `fullSnapshot()`. A second call without an
intervening tick returns an empty delta. The guard uses `tickGeneration()`, not
the supplied clock, so repeated timestamps remain valid. `tick()` clears the
dirty set at its start: consume each tick before advancing again.

## How it is tested

MeshBuilder and SceneExtractor each have dedicated unit tests. All targets are registered under `cmake/x3d/` (most in the grouped `x3d_extract_tests` binary, `cmake/x3d/doctest-suites.cmake`) and run under `ctest --preset dev`.

| ctest target | What it covers |
|---|---|
| `x3d_render_item` | `RenderItem.hpp` descriptor-layer compile gate (pure-POD) |
| `x3d_mesh_builder_t2` | Strips/fans/quads + `ElevationGrid` with flat normals |
| `x3d_mesh_builder_t3` | Normal/Color/ColorRGBA/TextureCoordinate attribute resolution, `normalPerVertex`/`colorPerVertex`, flat-normal generation |
| `x3d_mesh_builder_t4` | Analytic primitive parametric tessellation (Box/Sphere/Cone/Cylinder) |
| `x3d_mesh_builder_geom2d` | §14 Geometry2D nodes: Arc2D/ArcClose2D/Circle2D/Disk2D/Polyline2D/Polypoint2D/Rectangle2D/TriangleSet2D extraction (topology, tessellation count, +Z normals, XY bounds) |
| `x3d_mesh_builder_b3` | Extrusion SCP-frame sweep + caps |
| `x3d_mesh_builder_extrusion_scp` | Extrusion degenerate-spine SCP axes + distinct-point cull (ADR-0031) |
| `x3d_mesh_builder_ext001` | ElevationGrid/GeoElevationGrid authored Color/Normal + colorPerVertex/normalPerVertex (§13.3.4) |
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
| `x3d_scene_extractor_t8` | `delta()` incremental scalar dispatch + structural replacement snapshot |
| `x3d_scene_extractor_b2` | `skippedGeometryCounts()` coverage signal for unrecognized geometry types |
| `x3d_scene_extractor_col2` | `Collision.proxy` excluded from render set (COL-2) |
| `x3d_scene_extractor_cad1` | `CADFace.shape` traversed only for Shape/LOD/Transform children (CAD-1, §32.4.2) |
| `x3d_scene_extractor_m25_5` | Per-item light scoping (global vs. scoped, `scopeRoot` path-ancestor test) |
| `x3d_scene_extractor_audit` | Extractor conformance audit |
| `x3d_render_feed_audit` | End-to-end render-feed audit |
| `x3d_packed_mesh` | `PackedMesh` descriptor: `set_attrib`, `has()`, `empty()`, `is_indexed()` |
| `x3d_render_item_geometry` | `Geometry` union: AoS vs Packed kind switching |
| `x3d_light_system` | `LightSystem::collect()` world-resolution + global/scoped flag |
| `x3d_scene_extractor_fog` | Bound global `Fog` `FogDesc` + `LocalFog` placement paths, nested/disabled/root scope, world-scale, and bounded collection (§24.4.3) |
| `x3d_material_system` | `MaterialSystem::materialOf()` Phong/Physical/Unlit dispatch |
| `x3d_texture_extract` | Texture extraction + resolver threading (see [Texture extraction](extract-textures.md)) |

The full conformance extraction oracle is exercised by `x3d_extract_oracle_test` (registered only with `-DX3D_CPP_BUILD_EXT=ON`, in `cmake/x3d/ext.cmake`).

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
