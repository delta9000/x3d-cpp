---
title: Geometry extraction
summary: buildLocalMesh, the geometry node types MeshBuilder turns into MeshData, and first-party NURBS tessellation.
tags: [subsystem, extract, mesh-builder, nurbs]
updated: 2026-11-14
related:
  - extract.md
  - extract-topology.md
  - ../../explanation/decisions/0040-nurbs-tessellation-first-party.md
---

# Geometry extraction

`runtime/extract/MeshBuilder.hpp` turns one geometry node into local-frame
`MeshData`. The extractor that calls it is in [Extraction pipeline](extract.md);
topology rules are in [Topology classification](extract-topology.md).

## Entry point

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

## NURBS

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
- `NurbsSweptSurface` / `NurbsSwungSurface`  → read a 2D control curve (`crossSectionCurve`/`profileCurve` and the swung `trajectoryCurve`) via `tessellateCurve2D` — the shared evaluator lifted to z=0, with `ContourPolyline2D` forced to its piecewise-linear order 2 — plus a 3D `trajectoryCurve` for the swept case.
    - `sweptSurfaceGrid` carries the cross-section in the frame perpendicular to the trajectory tangent (a circle swept along a straight line is a cylinder); `swungSurfaceGrid` builds the classic swung surface `S(u,v) = (px·tx, px·ty, py)` (a line profile swung around a circle is a surface of revolution).
    - Both emit the same `Topology::Triangles` form as the patch surface (central-difference per-vertex normals, unit-square texcoords) and honor `solid`/`ccw`/`tessellation`.

- All five flip to `true` in `recognizedGeometryType()` (kept in lockstep with
  the dispatch).
- `GeometryBounds.hpp` gives them conservative AABB bounds (control-point
  convex hull for curve/patch; for swept the trajectory hull ⊕ the
  cross-section radius; for swung `|px|·|t|` in x/y with the profile height as
  z).
- `NurbsTrimmedSurface` remains unrecognized — it needs 2D contour-loop
  point-in-region clipping plus polygon triangulation in the `(u,v)` domain,
  and no such helper exists — and continues to route through the
  `externalGeometryResolver` fallback.
- The "first-party, not a seam" rationale is in
  [ADR-0040](../../explanation/decisions/0040-nurbs-tessellation-first-party.md).
