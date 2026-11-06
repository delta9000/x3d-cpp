# Geometry2D — conformance

_Generated. Levels 1,2 · 8 nodes · profiles: Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Arc2D | 2 | ✓ | ✓ | — | G2D-1, G2D-2, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, SEAM-2D-NURBS | X3DGeometryNode |
| ArcClose2D | 2 | ✓ | ✓ | — | G2D-1, G2D-2, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DGeometryNode |
| Circle2D | 2 | ✓ | ✓ | — | G2D-1, G2D-2, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, SEAM-2D-NURBS | X3DGeometryNode |
| Disk2D | 2 | ✓ | ✓ | — | G2D-1, G2D-2, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, SEAM-2D-NURBS | X3DGeometryNode |
| Polyline2D | 1 | ✓ | ✓ | — | G2D-1, G2D-2, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DGeometryNode |
| Polypoint2D | 1 | ✓ | ✓ | — | G2D-1, G2D-2, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DGeometryNode |
| Rectangle2D | 1 | ✓ | ✓ | — | G2D-1, G2D-2, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, SEAM-2D-NURBS | X3DGeometryNode |
| TriangleSet2D | 1 | ✓ | ✓ | — | G2D-1, G2D-2, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DGeometryNode |

## Findings

- **G2D-1** [major/CLOSED] — §14.3: All 8 Geometry2D nodes absent from recognizedGeometryType() — extract silently drops them.
  - mesh_builder_geom2d_test.cpp: All eight Geometry2D types emit triangle, line, or point meshes.
- **SEAM-2D-NURBS** [major/CLOSED] — §14 (Geometry2D): The entire 2D-geometry component is unrecognized by the mesh builder and renders nothing — indistinguishable from a parser failure.
  - mesh_builder_geom2d_test.cpp: The eight Geometry2D nodes are recognized before coordinate-based geometry dispatch.
- **PROTO-INSTANCE-IS-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.8: All four writers retain IS mappings on nested prototype instances.
- **PROTO-EMPTY-OVERRIDE-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.10: JSON and Classic writers retain explicit NULL and empty MFNode prototype overrides.
- **G2D-2** [minor/CLOSED] — §14.3: Geometry2D nodes absent from localGeometryBounds() — bboxes always empty.
  - Closed: localGeometryBoundsImpl (GeometryBounds.hpp) gained an exact arm per 2D node — arc/circle bounds from the arc's cardinal extremes (0,±π/2,π) inside the swept range plus endpoints, PIE sectors expand the centre, Disk2D uses outerRadius, Rectangle2D size, Polyline2D/Polypoint2D/TriangleSet2D their point/vertex arrays. Tested (bounds assertions) in mesh_builder_geom2d_test.cpp. (sweep 2026-06-25, closed 2D-geometry pass)

