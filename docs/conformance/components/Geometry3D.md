# Geometry3D — conformance

_Generated. Levels 1,2,3,4 · 7 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Box | 1 | ✓ | ✓ | — | TXF-1 | X3DGeometryNode |
| Cone | 1 | ✓ | ✓ | — | TXF-1 | X3DGeometryNode |
| Cylinder | 1 | ✓ | ✓ | — | TXF-1 | X3DGeometryNode |
| ElevationGrid | 3 | ✓ | ✓ | — | EXT-001, EXT-003, TXF-2 | X3DGeometryNode |
| Extrusion | 4 | ✓ | ✓ | — | EXTRUSION-SCP | X3DGeometryNode |
| IndexedFaceSet | 2 | ✓ | ✓ | — | TXF-2 | X3DComposedGeometryNode, X3DGeometryNode |
| Sphere | 1 | ✓ | ✓ | — | TXF-1 | X3DGeometryNode |

## Findings

- **EXT-001** [major/OPEN] — §13.3.4, 25.3.2: ElevationGrid/GeoElevationGrid drop authored color + normal nodes (colorPerVertex/normalPerVertex ignored) — only auto flat normals, no vertex colors.
  - The grid handlers never call buildAttrs(); wire color/normal/colorPerVertex/normalPerVertex (per-vertex i+j*xDim, per-quad i+j*(xDim-1)).
- **EXTRUSION-SCP** [minor/OPEN] — §13.3.5.4.5: 2-distinct-point Extrusion spine fixes the SCP plane normal but leaves in-plane X/Z free; single distinct point has no SCP. Browsers diverge (X3DOM Harrier wings reversed).
  - Policy (ADR-0031) - with Y = unit spine tangent, set SCP Z = normalize(modelZ - (modelZ.Y)Y) (fallback modelX when Y is parallel to modelZ), X = normalize(YxZ), re-derive Z = XxY; cull when < 2 DISTINCT (coincident-collapsed) spine points. Site runtime/extract/MeshBuilder.hpp tessellateExtrusion() (ref-x-Y branch ~:1043-1049; count guard :951). 4.1 - partial; 4.1-CD added coincident/distinct-spine-point sections but the 2-point in-plane axes remain open (a Frenet-frame idea was floated, not adopted) — engine rule is a candidate ahead of spec.
- **EXT-003** [major/CLOSED] — §13.3.4: ElevationGrid/GeoElevationGrid cells were wound backwards — with the default ccw=TRUE the generated face normals pointed −Y (down), so the surface was lit from below and, under a consumer's default solid=TRUE back-face cull, the whole terrain vanished when viewed from above.
  - emitHeightGrid() (the shared height-grid emitter for both ElevationGrid and GeoElevationGrid) split each cell (v00,v10,v11)+(v00,v11,v01), which is CW from the +Y side → faceNormal −Y. Reversed to (v00,v11,v10)+(v00,v01,v11) so the surface is CCW/front-facing from above and normals point +Y per §13.3.4. latticeIndex/creaseAngle smoothing key on the source lattice id, so they are order-independent. Surfaced by the Kelp Forest RockFloor (an ElevationGrid) rendering as scattered fragments vs X_ITE. Regression: mesh_builder_elevationgrid_winding_test.cpp (flat grid, default ccw → every normal +Y). (2026-06-25)

