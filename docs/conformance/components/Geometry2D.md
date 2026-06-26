# Geometry2D — conformance

_Generated. Levels 1,2 · 8 nodes · profiles: Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Arc2D | 2 | ✓ | ✗ | — | G2D-1, G2D-2, SEAM-2D-NURBS | X3DGeometryNode |
| ArcClose2D | 2 | ✓ | ✗ | — | G2D-1, G2D-2 | X3DGeometryNode |
| Circle2D | 2 | ✓ | ✗ | — | G2D-1, G2D-2, SEAM-2D-NURBS | X3DGeometryNode |
| Disk2D | 2 | ✓ | ✗ | — | G2D-1, G2D-2, SEAM-2D-NURBS | X3DGeometryNode |
| Polyline2D | 1 | ✓ | ✗ | — | G2D-1, G2D-2 | X3DGeometryNode |
| Polypoint2D | 1 | ✓ | ✗ | — | G2D-1, G2D-2 | X3DGeometryNode |
| Rectangle2D | 1 | ✓ | ✗ | — | G2D-1, G2D-2, SEAM-2D-NURBS | X3DGeometryNode |
| TriangleSet2D | 1 | ✓ | ✗ | — | G2D-1, G2D-2 | X3DGeometryNode |

## Findings

- **G2D-1** [major/OPEN] — §14.3: All 8 Geometry2D nodes absent from recognizedGeometryType() — extract silently drops them.
  - runtime/extract/MeshBuilder.hpp:1239-1252 lists no Geometry2D node; buildLocalMesh returns recognized=false → empty MeshData. No tessellation logic for any 2D primitive exists. Implementable now against the existing MeshBuilder. (sweep 2026-06-25)
- **SEAM-2D-NURBS** [major/OPEN] — §14 (Geometry2D), 27 (NURBS): The entire 2D-geometry component and all NURBS nodes are unrecognized by the mesh builder and render nothing — indistinguishable from a parser failure.
  - recognizedGeometryType MeshBuilder.hpp:1242-1256 lists none of Disk2D/Arc2D/ArcClose2D/Circle2D/ Rectangle2D/Polyline2D/Polypoint2D/TriangleSet2D or NurbsCurve/NurbsPatchSurface/NurbsTrimmedSurface; each falls to ++skippedGeometry_ (SceneExtractor.hpp:558). view3dscene/X_ITE tessellate these. Fix: tessellate the 2D primitives (cheap trig fans) now; flag NURBS as a known gap. (extraction-seam review.)
- **G2D-2** [minor/OPEN] — §14.3: Geometry2D nodes absent from localGeometryBounds() — bboxes always empty.
  - runtime/scene/GeometryBounds.hpp:52-140 handles Box/Sphere/Cone/Cylinder/grids/Extrusion/Text + generic coord nodes, but no Geometry2D node; the 2D nodes carry no 'coord' field so the generic fallback (line 74) also misses them → wrong culling/picking. Fix alongside G2D-1. (sweep 2026-06-25)

