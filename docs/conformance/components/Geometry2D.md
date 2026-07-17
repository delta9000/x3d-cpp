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
- **SEAM-2D-NURBS** [major/OPEN] — §14 (Geometry2D): The entire 2D-geometry component is unrecognized by the mesh builder and renders nothing — indistinguishable from a parser failure.
  - recognizedGeometryType (MeshBuilder.cpp:938-953) lists none of Disk2D/Arc2D/ArcClose2D/Circle2D/ Rectangle2D/Polyline2D/Polypoint2D/TriangleSet2D; each falls to ++skippedGeometry_ (SceneExtractor.hpp). view3dscene/X_ITE tessellate these. Fix: tessellate the 2D primitives (cheap trig fans). (extraction-seam review.) NURBS is no longer part of this finding: NurbsCurve/NurbsPatchSurface now tessellate (NRB-1, fixed) and the remaining trimmed/swept/swung surfaces are tracked by NRB-3.
- **G2D-2** [minor/OPEN] — §14.3: Geometry2D nodes absent from localGeometryBounds() — bboxes always empty.
  - runtime/scene/GeometryBounds.hpp:52-140 handles Box/Sphere/Cone/Cylinder/grids/Extrusion/Text + generic coord nodes, but no Geometry2D node; the 2D nodes carry no 'coord' field so the generic fallback (line 74) also misses them → wrong culling/picking. Fix alongside G2D-1. (sweep 2026-06-25)

