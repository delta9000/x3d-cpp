# Geometry3D — conformance

_Generated. Levels 1,2,3,4 · 7 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Box | 1 | ✓ | ✓ | — | ROUTE-IO-ALIAS, TXF-1 | X3DGeometryNode |
| Cone | 1 | ✓ | ✓ | — | ROUTE-IO-ALIAS, TXF-1 | X3DGeometryNode |
| Cylinder | 1 | ✓ | ✓ | — | ROUTE-IO-ALIAS, TXF-1 | X3DGeometryNode |
| ElevationGrid | 3 | ✓ | ✓ | — | EXT-001, EXT-003, ROUTE-IO-ALIAS, TXF-2, VIS-ELEV-SPACING-EMPTY | X3DGeometryNode |
| Extrusion | 4 | ✓ | ✓ | — | AUD-EXT-1, EXTRUSION-SCP, ROUTE-IO-ALIAS, VIS-EXTRUSION-EMPTY | X3DGeometryNode |
| IndexedFaceSet | 2 | ✓ | ✓ | — | ROUTE-IO-ALIAS, TXF-2 | X3DComposedGeometryNode, X3DGeometryNode |
| Sphere | 1 | ✓ | ✓ | — | ROUTE-IO-ALIAS, TXF-1, VIS-SPHERE-NOUV | X3DGeometryNode |

## Findings

- **EXT-001** [major/CLOSED] — §13.3.4, 25.3.2: ElevationGrid/GeoElevationGrid drop authored color + normal nodes (colorPerVertex/normalPerVertex ignored) — only auto flat normals, no vertex colors.
  - emitHeightGrid (MeshBuilder.hpp, shared by both grids) now resolves authored Color/Normal via buildAttrs() and honors colorPerVertex/normalPerVertex per §13.3.4: per-vertex indexes the LATTICE vertex (row*xDim+col), per-quad indexes the CELL (row*(xDim-1)+col). An authored Normal also suppresses the generated-flat/crease path. Regression: mesh_builder_ext001_test.cpp (per-vertex and per-quad colour/normal, ElevationGrid + GeoElevationGrid). (2026-06-27)
- **EXT-003** [major/CLOSED] — §13.3.4: ElevationGrid/GeoElevationGrid cells were wound backwards — with the default ccw=TRUE the generated face normals pointed −Y (down), so the surface was lit from below and, under a consumer's default solid=TRUE back-face cull, the whole terrain vanished when viewed from above.
  - emitHeightGrid() (the shared height-grid emitter for both ElevationGrid and GeoElevationGrid) split each cell (v00,v10,v11)+(v00,v11,v01), which is CW from the +Y side → faceNormal −Y. Reversed to (v00,v11,v10)+(v00,v01,v11) so the surface is CCW/front-facing from above and normals point +Y per §13.3.4. latticeIndex/creaseAngle smoothing key on the source lattice id, so they are order-independent. Surfaced by the Kelp Forest RockFloor (an ElevationGrid) rendering as scattered fragments vs X_ITE. Regression: mesh_builder_elevationgrid_winding_test.cpp (flat grid, default ccw → every normal +Y). (2026-06-25)
- **VIS-EXTRUSION-EMPTY** [major/FIXED] — §13.3.5: Extrusion with default/simple fields extracts NO geometry (render_items=0) — default_extrusion.x3d and test_simpleorient.x3d render blank. FIXED: now render (default_extrusion = unit box, test_simpleorient = oriented square swept along the spine).
  - Root cause was the GENERATOR, not the extractor: the MF-struct default emitter (src/x3d_cpp_gen/emit/defaults.py) collapsed any multi-element array default to its FIRST element, so the generated Extrusion binding defaulted crossSection to a single point {1,1} and spine to {0,0,0} (instead of the 5-point unit square and the 2-point segment). An UNauthored crossSection/spine therefore arrived at tessellateExtrusion as size 1 and was culled by the ns>=2/nc>=2 guard. Fixed by chunking the flat scalar list into element-sized groups (covered by tests/test_default_expr.py) and regenerating Extrusion.hpp; the extractor's .empty() spec-default fallback stays as a safety net, and an AUTHORED 1-point spine still culls (the documented "< 2 spine points => empty" contract, mesh_builder_b3_test.cpp).
- **VIS-ELEV-SPACING-EMPTY** [major/FIXED] — §13.3.4: ElevationGrid with non-default xSpacing/zSpacing extracts NO geometry (render_items=0) — test_xSpacing.x3d, test_zSpacing.x3d render blank, while test_color (default spacing) extracts fine. FIXED: both now render 3 grids each.
  - Not a spacing bug: test_{x,z}Spacing author only ONE dimension (xDimension=12, height=24 values) and OMIT the other, leaving zDimension at its 0 default so the xd>=2 && zd>=2 guard in the ElevationGrid dispatch (MeshBuilder.hpp) zeroed the output. Per spec both dimensions are required, but the NIST reference renders expect inference. Fixed by deriving the missing dimension from height.size() when it divides evenly by the authored dimension (conservative — only fires when a dimension is < 2 and the height array is an exact multiple of the other). Deliberate, documented leniency.
- **AUD-EXT-1** [major/CLOSED] — §13.3.5.6, 11.2.3: Caps are fanned from vertex 0 even with convex FALSE, so a concave cap covers its notch.
  - Extrusion caps with convex FALSE use the IndexedFaceSet ear clipper. Covered by extrusion_concave_cap_does_not_cover_notch.
- **EXTRUSION-SCP** [minor/CLOSED] — §13.3.5.4.5: 2-distinct-point Extrusion spine fixes the SCP plane normal but leaves in-plane X/Z free; single distinct point has no SCP. Browsers diverge (X3DOM Harrier wings reversed).
  - Implemented per ADR-0031 in tessellateExtrusion() (MeshBuilder.cpp): with Y = unit spine tangent, the underdetermined Z = normalize(modelZ - (modelZ·Y)Y) (fallback modelX when Y ∥ modelZ), X = normalize(Y×Z), re-derive Z = X×Y (replacing the arbitrary ref×Y fallback); and a cull when fewer than 2 DISTINCT (coincident-collapsed) spine points (§13.3.5.4.5 sentence 2). Regression: mesh_builder_extrusion_scp_test.cpp (coincident 1-point cull; spine-along-+X in-plane axes; Y∥modelZ fallback; well-defined bent spine untouched). (2026-06-27)

