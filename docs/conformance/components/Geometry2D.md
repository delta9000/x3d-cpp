# Geometry2D — conformance

_Generated. Levels 1,2 · 8 nodes · profiles: Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Arc2D | 2 | ✓ | ✓ | — | G2D-1, G2D-2, ROUTE-IO-ALIAS, SEAM-2D-NURBS | X3DGeometryNode |
| ArcClose2D | 2 | ✓ | ✓ | — | G2D-1, G2D-2, ROUTE-IO-ALIAS | X3DGeometryNode |
| Circle2D | 2 | ✓ | ✓ | — | G2D-1, G2D-2, ROUTE-IO-ALIAS, SEAM-2D-NURBS | X3DGeometryNode |
| Disk2D | 2 | ✓ | ✓ | — | G2D-1, G2D-2, ROUTE-IO-ALIAS, SEAM-2D-NURBS | X3DGeometryNode |
| Polyline2D | 1 | ✓ | ✓ | — | G2D-1, G2D-2, ROUTE-IO-ALIAS | X3DGeometryNode |
| Polypoint2D | 1 | ✓ | ✓ | — | G2D-1, G2D-2, ROUTE-IO-ALIAS | X3DGeometryNode |
| Rectangle2D | 1 | ✓ | ✓ | — | G2D-1, G2D-2, ROUTE-IO-ALIAS, SEAM-2D-NURBS | X3DGeometryNode |
| TriangleSet2D | 1 | ✓ | ✓ | — | G2D-1, G2D-2, ROUTE-IO-ALIAS | X3DGeometryNode |

## Findings

- **G2D-1** [major/CLOSED] — §14.3: All 8 Geometry2D nodes absent from recognizedGeometryType() — extract silently drops them.
  - Closed: all 8 names added to recognizedGeometryType() and tessellated by a new mesh_detail::buildGeometry2D (MeshBuilder.cpp), dispatched before the coord guard since they carry no `coord`. Arc2D/Circle2D/Polyline2D -> Lines, Polypoint2D -> Points, ArcClose2D (PIE/CHORD)/Disk2D/Rectangle2D/TriangleSet2D -> Triangles with +Z normals. Circular primitives use 64 chords/full circle (2π/64 rad per chord). Tested in mesh_builder_geom2d_test.cpp. (sweep 2026-06-25, closed 2D-geometry pass)
- **SEAM-2D-NURBS** [major/CLOSED] — §14 (Geometry2D): The entire 2D-geometry component is unrecognized by the mesh builder and renders nothing — indistinguishable from a parser failure.
  - Closed: recognizedGeometryType now lists all eight 2D primitives and buildGeometry2D tessellates them, so SceneExtractor no longer drops Disk2D/Arc2D/ArcClose2D/Circle2D/Rectangle2D/Polyline2D/Polypoint2D/TriangleSet2D (no more ++skippedGeometry_ for them). Tessellation is cheap trig fans/lines in the XY plane. Tested in mesh_builder_geom2d_test.cpp. NURBS was already split out of this finding (NRB-1 fixed; NRB-3 tracks the rest). (extraction-seam review; closed 2D-geometry pass.)
- **G2D-2** [minor/CLOSED] — §14.3: Geometry2D nodes absent from localGeometryBounds() — bboxes always empty.
  - Closed: localGeometryBoundsImpl (GeometryBounds.hpp) gained an exact arm per 2D node — arc/circle bounds from the arc's cardinal extremes (0,±π/2,π) inside the swept range plus endpoints, PIE sectors expand the centre, Disk2D uses outerRadius, Rectangle2D size, Polyline2D/Polypoint2D/TriangleSet2D their point/vertex arrays. Tested (bounds assertions) in mesh_builder_geom2d_test.cpp. (sweep 2026-06-25, closed 2D-geometry pass)

