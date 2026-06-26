# M2b — Bounding Volumes

**Date:** 2026-06-13
**Branch:** `modernize-x3d-spec`
**Status:** design approved, pre-implementation
**Milestone:** M2b (second unit of the M2 scene-graph runtime). Roadmap:
`2026-06-07-architecture-validation-and-resequencing.md` §3 Step 2; builds on M2a
(`2026-06-13-m2a-dirty-tracking-world-transform-design.md`).

## Context

M2a delivered the dirty-tracking layer + world-transform side table. M2b adds
**axis-aligned bounding volumes**: a per-node local-frame AABB computed from
geometry and unioned bottom-up through the grouping hierarchy, with world-space
bounds as a lazy query composing M2a's `worldTransform()`. Bounds are the substrate
for M2d (picking/ray) and M2e (LOD/visibility/culling) and feed the M2.5 extraction
seam.

**Runtime-only, golden byte-identical.** Local AABBs live in a side table keyed by
`const X3DNode*` (same discipline as M2a's world transforms). No generated node is
touched; golden sha256 stays `7226b3a07e744b4aea1c3d9f34897384e5d8c085cb0c79743b8a23872c7183c0`.

**Identity consistency (M2C-1):** M2b inherits M2a's per-node identity — a USE'd node
has one local AABB; its world bounds differ across placements only via the (single)
world transform. This is the **same** USE-sharing approximation as M2a, **not
deepened**; the path-vs-node identity decision remains a pre-M2.5 spike (backlog
M2C-1). Documented here, no new debt.

## Verified surface

- `X3DBoundedObject` (47 node types) carries `bboxCenter` (SFVec3f, initializeOnly)
  and `bboxSize` (SFVec3f, default `-1 -1 -1` = "unspecified, compute it") +
  `bboxDisplay`.
- Primitives: `Box.getSize()` (SFVec3f), `Sphere.getRadius()`, `Cone.getBottomRadius()`
  +`getHeight()`, `Cylinder.getRadius()`+`getHeight()`.
- **Coord-based geometry share a `coord` SFNode field** → a `Coordinate`/`CoordinateDouble`
  with `getPoint()` (MFVec3f). One reflection path covers IndexedFaceSet, PointSet,
  IndexedLineSet, LineSet, TriangleSet/TriangleStripSet/TriangleFanSet,
  IndexedTriangleSet/…StripSet/…FanSet, QuadSet/IndexedQuadSet (~15 types).
- `ElevationGrid`: `getXDimension/getZDimension` (SFInt32), `getXSpacing/getZSpacing`
  (SFFloat), `getHeight()` (MFFloat).
- `Extrusion`: `getCrossSection()` (MFVec2f), `getSpine()` (MFVec3f), `getScale()`
  (MFVec2f), `getOrientation()` (MFRotation).
- NURBS surfaces: `getControlPoint()` (via `X3DNurbsSurfaceGeometryNode`).
- `Text`: `getString()` (MFString), `getLength()` (MFFloat), `getMaxExtent()` (SFFloat);
  `FontStyle.getSize()` (SFFloat), `getSpacing()` (SFFloat).
- `Shape.getGeometry()` (SFNode); reflection field name `geometry`.
- M2a provides `Mat4`, `worldTransform(node)`, `DirtyTracker`/`DirtyBounds` (already
  reserved as `1u<<4`).

## Design

### 1. `Aabb` — axis-aligned bounding box (`runtime/math/Aabb.hpp`)

```cpp
struct Aabb {
  SFVec3f min, max;
  bool empty = true;          // empty is the union identity
  void expand(const SFVec3f &p);           // grow to include p (clears empty)
  void unionWith(const Aabb &o);           // grow to include o (empty-aware)
  Aabb transformed(const Mat4 &m) const;   // transform 8 corners, re-fit AABB
  SFVec3f center() const;                  // (min+max)/2  (empty -> {0,0,0})
  SFVec3f size() const;                    // max-min      (empty -> {0,0,0})
  static Aabb fromCenterSize(const SFVec3f &c, const SFVec3f &s); // bbox* interop
};
```

`transformed` of an empty AABB is empty. Header-only, namespace `x3d::runtime`.

### 2. Geometry local bounds — `localGeometryBounds(const X3DNode*)` (`runtime/scene/GeometryBounds.hpp`)

Free function returning the geometry's AABB **in its own local frame**, dispatched by
`nodeTypeName()` + reflection field presence (all fields read via `node->fields()`):

- **Box** → `fromCenterSize({0,0,0}, size)`.
- **Sphere** → `[-r,+r]³`.
- **Cone** → x,z ∈ `[-bottomRadius, +bottomRadius]`, y ∈ `[-height/2, +height/2]`.
- **Cylinder** → x,z ∈ `[-radius,+radius]`, y ∈ `[-height/2,+height/2]`.
- **Has a `coord` SFNode field** (generic mesh path) → resolve to the `Coordinate`/
  `CoordinateDouble`, AABB over its `point` (MFVec3f). Empty if no/empty coord.
- **ElevationGrid** → x ∈ `[0,(xDimension-1)·xSpacing]`, z ∈ `[0,(zDimension-1)·zSpacing]`,
  y ∈ height range (min/max over `height`, or `[0,0]` if empty).
- **Extrusion** → for each spine point, place the (scaled, oriented) crossSection ring
  and expand; union over the spine. (If spine/crossSection empty → empty.)
- **Has a `controlPoint` field** (NURBS surfaces) → AABB over the control net
  (`controlPoint` is the bounding hull of the surface — exact containment). Reached via
  reflection (SFNode `Coordinate` or MFVec3f, whichever the field yields).
- **Text** → conservative estimate: width ≈ `max over strings(len·FontStyle.size·0.6)`
  capped by `maxExtent` (when `>0`) / per-string `length` (when set), height ≈
  `lineCount·FontStyle.size·spacing`, centered per justification-agnostic default;
  z = 0. **Documented approximation** (exact glyph bounds need a font engine — out of
  scope for a headless SDK; backlog M2B-1).
- **Geo\*** (GeoElevationGrid) → best-effort grid bound in the local frame from
  dimensions/spacing; **backlog M2B-2** for geo-accurate bounds (needs GEO projection).
- **Unknown / unsupported / no geometry** → empty `Aabb`.

### 3. `BoundsSystem` — local-bounds table + bottom-up propagation (`runtime/scene/BoundsSystem.hpp`)

- **Parent index over the full scene graph** (every node → its parent; bounds union
  flows through ALL grouping nodes, not just Transforms), built from one DFS walk over
  node-typed fields (guard `if (!f.get) continue;` — same InputOnly-MFNode trap M2a
  hit). Records children lists too.
- **Local-AABB side table**: `unordered_map<const X3DNode*, Aabb>` — each node's bounds
  in its own coordinate frame.
- **`buildBounds(scene, const TransformSystem&)`** (bottom-up post-order):
  - Author override first: a node whose `bboxSize` is author-specified — **every
    component ≥ 0** (the default `-1 -1 -1` means unspecified) — uses
    `localBounds = Aabb::fromCenterSize(bboxCenter, bboxSize)`, and its subtree is NOT
    unioned into it (the author bound is authoritative). Read `bboxSize`/`bboxCenter`
    via reflection (present only on X3DBoundedObject nodes).
  - Else a Shape → `localGeometryBounds(geometry)`.
  - Else a grouping/other node → union over children `c` of
    `c-is-a-Transform ? localBounds(c).transformed(localMatrix(c)) : localBounds(c)`
    (a child Transform's local matrix maps its frame into this node's frame; non-Transform
    children share this node's frame). `localMatrix` reuse: expose it from
    `TransformSystem` (make the M2a `localMatrix` helper accessible — a small public
    static `TransformSystem::localMatrix(node)` or a shared free function in Mat4/the
    transform header).
- **`propagate(DirtyTracker&, const TransformSystem&)`** (bottom-up, incremental): for
  each node in `dirty.changedNodes()` that is bounds-relevant (`DirtyBounds`, or a
  geometry/`coord` `DirtyField`, or `DirtyChildren`), recompute its local AABB and walk
  up the parent index re-unioning ancestors, **stopping when an ancestor's AABB is
  unchanged**; mark each recomputed node `DirtyBounds`. (World-transform-only changes do
  NOT trigger bounds propagation — world bounds are a lazy query, §below.)
- **Queries:** `const Aabb& localBounds(node)` (empty if unknown);
  `Aabb worldBounds(node, const TransformSystem& ts)` =
  `localBounds(node).transformed(ts.worldTransform(node))`.

### 4. `X3DExecutionContext` integration + pull API

- Own a `BoundsSystem bounds_`.
- `buildSceneGraph(scene)` (M2a) also calls `bounds_.buildBounds(scene, transforms_)`.
- The dirty classifier marks `DirtyBounds` when a delivered field is geometry/coord
  affecting (e.g. `size`, `radius`, `height`, `point`, `coord`, `controlPoint`,
  `crossSection`, `spine`, `bboxSize`) or `children` changes — conservatively, OR
  `DirtyBounds` alongside `DirtyField`/`DirtyChildren` on bounds-participating nodes.
- `tick`: …`cascade_.process()` → `transforms_.propagate(dirty_)` →
  **`bounds_.propagate(dirty_, transforms_)`** (transforms first — bounds may read world
  transforms; and the incremental bounds recompute reads current local matrices).
- Pull API: `localBounds(node)`, `worldBounds(node)` (composes `transforms_`), beside
  M2a's `worldTransform`/`dirtyTracker`.

### Scope boundaries

- **In:** `Aabb`; the full geometry dispatch (primitives, generic coord path, ElevationGrid,
  Extrusion, NURBS control-hull, Text-approx, Geo best-effort); full-graph parent index;
  bottom-up build with author-bbox override; incremental `propagate`; lazy `worldBounds`;
  tick + pull. Tests for each.
- **Out / backlog:** exact Text glyph bounds (M2B-1); geo-accurate Geo\* bounds (M2B-2);
  M2a's existing rows (M2C-1 path identity, M2C-2 structural re-index, etc.) unchanged —
  M2b inherits, does not deepen.

## File structure

| File | Responsibility |
|------|----------------|
| `runtime/math/Aabb.hpp` (new) | AABB type: expand/union/transformed/center/size |
| `runtime/math/tests/aabb_test.cpp` (new) | union, expand, transformed-by-rotation |
| `runtime/scene/GeometryBounds.hpp` (new) | `localGeometryBounds` type dispatch |
| `runtime/scene/tests/geometry_bounds_test.cpp` (new) | each geometry family |
| `runtime/scene/BoundsSystem.hpp` (new) | parent index, local-bounds table, build + incremental propagate, world-bounds query |
| `runtime/scene/tests/bounds_system_test.cpp` (new) | build, author override, incremental |
| `runtime/scene/TransformSystem.hpp` (modify) | expose `localMatrix(node)` (public static) for BoundsSystem reuse |
| `runtime/events/X3DExecutionContext.hpp` (modify) | own BoundsSystem; build + tick step; DirtyBounds classify; pull API |
| `runtime/events/tests/m2b_tick_test.cpp` (new) | end-to-end build + worldBounds via tick |
| `CMakeLists.txt` (modify) | register the 4 new test executables |

## Testing

1. **Aabb:** union of two boxes; `transformed` of a unit cube by a 45° Z-rotation has the
   expected enlarged AABB; empty-union identity.
2. **GeometryBounds:** Box/Sphere/Cone/Cylinder analytic; an IndexedFaceSet over known
   `coord` points → exact AABB; ElevationGrid dims; Extrusion (a simple square section on
   a straight spine) → expected box; NURBS control-hull; Text non-empty and shrinks to
   `maxExtent` when set.
3. **BoundsSystem build:** Transform>Shape>Box → group `localBounds` = box AABB ×
   Transform local matrix; `worldBounds` = × world transform (compose M2a).
4. **Author override:** a Group with explicit `bboxSize` returns exactly that box,
   ignoring a child whose geometry would be larger.
5. **Incremental:** change a leaf `Box.size`, mark dirty, `propagate` → the Box's Shape and
   its ancestor group bounds grow; an unrelated sibling subtree's bounds are unchanged
   (assert the sibling was not re-marked `DirtyBounds`).
6. **Tick end-to-end:** `buildSceneGraph` then `tick`; `worldBounds(root)` encloses the
   scene.

## Verification

- `mise run build` (pytest + ctest) green; **golden byte-identical**; existing
  M2a/event/animation tests unaffected.

## Risks / watch-items

- **`localMatrix` reuse:** M2b needs M2a's per-Transform local matrix. Expose it as a
  public `static Mat4 TransformSystem::localMatrix(const X3DNode*)` (it is already a
  private static) rather than duplicating the TRS read.
- **InputOnly MFNode getter trap:** the parent-index walk must guard `if (!f.get)
  continue;` (Transform's `addChildren`/`removeChildren` have null getters — the exact
  M2a Task-4 bug). Reuse that guard.
- **Author bbox sentinel:** treat `bboxSize == (-1,-1,-1)` (the default) as "unspecified";
  any other value (including a partial like `(-1,-1,5)`? per spec, a single -1 means
  unspecified) — use the spec rule: bbox is used only when ALL components are ≥ 0. Decide
  and pin in a test: **use the author bbox iff every component of `bboxSize` is ≥ 0.**
- **Empty/degenerate geometry:** empty coord/height/spine → empty AABB (must not crash or
  produce NaN); the union identity handles it.
- **Double-include / namespace:** all new types in `x3d::runtime`; `Aabb` in
  `runtime/math` beside `Mat4`.
