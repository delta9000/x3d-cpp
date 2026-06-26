# Rendering — conformance

_Generated. Levels 1,2,3,5 · 15 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| ClipPlane | 5 | ✓ | — | — | — | X3DChildNode |
| Color | 1 | ✓ | — | — | — | X3DColorNode, X3DGeometricPropertyNode |
| ColorRGBA | 1 | ✓ | — | — | — | X3DColorNode, X3DGeometricPropertyNode |
| Coordinate | 1 | ✓ | — | — | — | X3DCoordinateNode, X3DGeometricPropertyNode |
| CoordinateDouble | 1 | ✓ | — | — | GEO-2 | X3DCoordinateNode, X3DGeometricPropertyNode |
| IndexedLineSet | 1 | ✓ | ✓ | — | — | X3DGeometryNode |
| IndexedTriangleFanSet | 3 | ✓ | ✓ | — | EXT-002 | X3DComposedGeometryNode, X3DGeometryNode |
| IndexedTriangleSet | 3 | ✓ | ✓ | — | — | X3DComposedGeometryNode, X3DGeometryNode |
| IndexedTriangleStripSet | 3 | ✓ | ✓ | — | EXT-002 | X3DComposedGeometryNode, X3DGeometryNode |
| LineSet | 1 | ✓ | ✓ | — | — | X3DGeometryNode |
| Normal | 2 | ✓ | — | — | — | X3DGeometricPropertyNode, X3DNormalNode |
| PointSet | 1 | ✓ | ✓ | — | — | X3DGeometryNode |
| TriangleFanSet | 3 | ✓ | ✓ | — | EXT-002 | X3DComposedGeometryNode, X3DGeometryNode |
| TriangleSet | 3 | ✓ | ✓ | — | — | X3DComposedGeometryNode, X3DGeometryNode |
| TriangleStripSet | 3 | ✓ | ✓ | — | EXT-002 | X3DComposedGeometryNode, X3DGeometryNode |

## Findings

- **EXT-002** [major/CLOSED `fe4d730`] — §11.3.2, 11.4.13, 11.4.15: With colorPerVertex/normalPerVertex=FALSE, fan/strip sets index color/normal per TRIANGLE (faceNo++ per triangle) instead of per fan/strip — wrong colors/normals when a fan/strip has >1 triangle.
  - Increment faceNo per fan/strip primitive (per fanCount/stripCount entry, per -1 run for indexed), not per emitted triangle.

