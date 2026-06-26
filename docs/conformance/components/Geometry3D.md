# Geometry3D — conformance

_Generated. Levels 1,2,3,4 · 7 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Box | 1 | ✓ | ✓ | — | TXF-1 | X3DGeometryNode |
| Cone | 1 | ✓ | ✓ | — | TXF-1 | X3DGeometryNode |
| Cylinder | 1 | ✓ | ✓ | — | TXF-1 | X3DGeometryNode |
| ElevationGrid | 3 | ✓ | ✓ | — | EXT-001, EXT-003, TXF-2 | X3DGeometryNode |
| Extrusion | 4 | ✓ | ✓ | — | — | X3DGeometryNode |
| IndexedFaceSet | 2 | ✓ | ✓ | — | TXF-2 | X3DComposedGeometryNode, X3DGeometryNode |
| Sphere | 1 | ✓ | ✓ | — | TXF-1 | X3DGeometryNode |

## Findings

- **EXT-001** [major/OPEN] — §13.3.4, 25.3.2: ElevationGrid/GeoElevationGrid drop authored color + normal nodes (colorPerVertex/normalPerVertex ignored) — only auto flat normals, no vertex colors.
  - The grid handlers never call buildAttrs(); wire color/normal/colorPerVertex/normalPerVertex (per-vertex i+j*xDim, per-quad i+j*(xDim-1)).
- **EXT-003** [major/CLOSED] — §13.3.4: ElevationGrid/GeoElevationGrid cells were wound backwards — with the default ccw=TRUE the generated face normals pointed −Y (down), so the surface was lit from below and, under a consumer's default solid=TRUE back-face cull, the whole terrain vanished when viewed from above.
  - emitHeightGrid() (the shared height-grid emitter for both ElevationGrid and GeoElevationGrid) split each cell (v00,v10,v11)+(v00,v11,v01), which is CW from the +Y side → faceNormal −Y. Reversed to (v00,v11,v10)+(v00,v01,v11) so the surface is CCW/front-facing from above and normals point +Y per §13.3.4. latticeIndex/creaseAngle smoothing key on the source lattice id, so they are order-independent. Surfaced by the Kelp Forest RockFloor (an ElevationGrid) rendering as scattered fragments vs X_ITE. Regression: mesh_builder_elevationgrid_winding_test.cpp (flat grid, default ccw → every normal +Y). (2026-06-25)

