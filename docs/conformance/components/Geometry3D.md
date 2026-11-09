# Geometry3D — conformance

_Generated. Levels 1,2,3,4 · 7 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Box | 1 | ✓ | ✓ | — | JSON-TYPED-FIELD-VALUES, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, REQ-UNIT, ROUTE-IO-ALIAS, TXF-1 | X3DGeometryNode |
| Cone | 1 | ✓ | ✓ | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, TXF-1 | X3DGeometryNode |
| Cylinder | 1 | ✓ | ✓ | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, TXF-1 | X3DGeometryNode |
| ElevationGrid | 3 | ✓ | ✓ | — | EXT-001, EXT-003, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, TXF-2, VIS-ELEV-SPACING-EMPTY | X3DGeometryNode |
| Extrusion | 4 | ✓ | ✓ | — | AUD-EXT-1, EXTRUSION-SCP, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, VIS-EXTRUSION-EMPTY | X3DGeometryNode |
| IndexedFaceSet | 2 | ✓ | ✓ | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, TXF-2 | X3DComposedGeometryNode, X3DGeometryNode |
| Sphere | 1 | ✓ | ✓ | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, TXF-1, VIS-SPHERE-NOUV | X3DGeometryNode |

## Findings

- **EXT-001** [major/CLOSED] — §13.3.4, 25.3.2: ElevationGrid/GeoElevationGrid drop authored color + normal nodes (colorPerVertex/normalPerVertex ignored) — only auto flat normals, no vertex colors.
  - mesh_builder_grid_attributes_test.cpp: Height grids apply authored per-vertex and per-cell colors and normals.
- **EXT-003** [major/CLOSED] — §13.3.4: ElevationGrid/GeoElevationGrid cells were wound backwards — with the default ccw=TRUE the generated face normals pointed −Y (down), so the surface was lit from below and, under a consumer's default solid=TRUE back-face cull, the whole terrain vanished when viewed from above.
  - mesh_builder_elevationgrid_winding_test.cpp: Default height-grid triangles face upward with positive-Y normals.
- **VIS-EXTRUSION-EMPTY** [major/FIXED] — §13.3.5: Extrusion with default/simple fields extracts NO geometry (render_items=0) — default_extrusion.x3d and test_simpleorient.x3d render blank. FIXED: now render (default_extrusion = unit box, test_simpleorient = oriented square swept along the spine).
  - mesh_builder_extrusion_test.cpp: Generated multi-element defaults preserve the complete Extrusion cross section and spine.
- **VIS-ELEV-SPACING-EMPTY** [major/FIXED] — §ISO/IEC 19775-1 13.3.4: ElevationGrid infers one omitted dimension from a divisible height count.
  - NIST test_xSpacing and test_zSpacing scenes omit one required dimension. MeshBuilder.cpp infers a dimension below two when the height count divides evenly by the other dimension, preserving these reference renders as a deliberate leniency. The omitted-dimension regression in mesh_builder_composed_geometry_test.cpp checks both axes with non-default spacing, triangle counts, and corner heights.
- **AUD-EXT-1** [major/CLOSED] — §13.3.5.6, 11.2.3: Caps are fanned from vertex 0 even with convex FALSE, so a concave cap covers its notch.
  - Extrusion caps with convex FALSE use the IndexedFaceSet ear clipper. Covered by extrusion_concave_cap_does_not_cover_notch.
- **JSON-TYPED-FIELD-VALUES** [major/FIXED] — §ISO/IEC 19776-5 5.1.3, 5.2, 5.6, 5.15, Annex A: JSON author fields, prototype defaults, and instance overrides retain native typed values.
  - The reader uses the declared field type to decode numbers, booleans, and arrays, and accepts legacy wire strings emitted by the existing writer. Test: reader_audit_test.cpp, JSON typed author and prototype field values are retained.
- **PROTO-INSTANCE-IS-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.8: All four writers retain IS mappings on nested prototype instances.
- **PROTO-EMPTY-OVERRIDE-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.10: JSON and Classic writers retain explicit NULL and empty MFNode prototype overrides.
- **EXTRUSION-SCP** [minor/CLOSED] — §13.3.5.4.5: 2-distinct-point Extrusion spine fixes the SCP plane normal but leaves in-plane X/Z free; single distinct point has no SCP. Browsers diverge (X3DOM Harrier wings reversed).
  - mesh_builder_extrusion_scp_test.cpp: Degenerate spine frames use projected model axes and coincident-only spines emit no surface.

