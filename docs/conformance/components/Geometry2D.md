# Geometry2D — conformance

_Generated. Levels 1,2 · 8 nodes · profiles: Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Arc2D | 2 | ✓ | ✗ | — | G2D-1, G2D-2 | X3DGeometryNode |
| ArcClose2D | 2 | ✓ | ✗ | — | G2D-1, G2D-2 | X3DGeometryNode |
| Circle2D | 2 | ✓ | ✗ | — | G2D-1, G2D-2 | X3DGeometryNode |
| Disk2D | 2 | ✓ | ✗ | — | G2D-1, G2D-2 | X3DGeometryNode |
| Polyline2D | 1 | ✓ | ✗ | — | G2D-1, G2D-2 | X3DGeometryNode |
| Polypoint2D | 1 | ✓ | ✗ | — | G2D-1, G2D-2 | X3DGeometryNode |
| Rectangle2D | 1 | ✓ | ✗ | — | G2D-1, G2D-2 | X3DGeometryNode |
| TriangleSet2D | 1 | ✓ | ✗ | — | G2D-1, G2D-2 | X3DGeometryNode |

## Findings

- **G2D-1** [major/OPEN] — §14.3: All 8 Geometry2D nodes absent from recognizedGeometryType() — extract silently drops them.
  - runtime/extract/MeshBuilder.hpp:1239-1252 lists no Geometry2D node; buildLocalMesh returns recognized=false → empty MeshData. No tessellation logic for any 2D primitive exists. Implementable now against the existing MeshBuilder. (sweep 2026-06-25)
- **G2D-2** [minor/OPEN] — §14.3: Geometry2D nodes absent from localGeometryBounds() — bboxes always empty.
  - runtime/scene/GeometryBounds.hpp:52-140 handles Box/Sphere/Cone/Cylinder/grids/Extrusion/Text + generic coord nodes, but no Geometry2D node; the 2D nodes carry no 'coord' field so the generic fallback (line 74) also misses them → wrong culling/picking. Fix alongside G2D-1. (sweep 2026-06-25)

