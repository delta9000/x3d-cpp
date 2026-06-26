# NURBS — conformance

_Generated. Levels 1,2,3,4 · 13 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Contour2D | 4 | ✓ | — | — | — |  |
| ContourPolyline2D | 3 | ✓ | — | — | — | X3DNurbsControlCurveNode |
| NurbsCurve | 1 | ✓ | ✗ | — | CONTAINERFIELD-FALSEPOS, NRB-1, SEAM-2D-NURBS | X3DGeometryNode, X3DParametricGeometryNode |
| NurbsCurve2D | 3 | ✓ | — | — | — | X3DNurbsControlCurveNode |
| NurbsOrientationInterpolator | 1 | ✓ | — | — | NRB-2 | X3DChildNode |
| NurbsPatchSurface | 1 | ✓ | ✗ | — | NRB-1, SEAM-2D-NURBS | X3DGeometryNode, X3DNurbsSurfaceGeometryNode, X3DParametricGeometryNode |
| NurbsPositionInterpolator | 1 | ✓ | — | — | NRB-2 | X3DChildNode |
| NurbsSet | 2 | ✓ | — | — | — | X3DBoundedObject, X3DChildNode |
| NurbsSurfaceInterpolator | 1 | ✓ | — | — | NRB-2 | X3DChildNode |
| NurbsSweptSurface | 3 | ✓ | ✗ | — | NRB-1 | X3DGeometryNode, X3DParametricGeometryNode |
| NurbsSwungSurface | 3 | ✓ | ✗ | — | NRB-1 | X3DGeometryNode, X3DParametricGeometryNode |
| NurbsTextureCoordinate | 1 | ✓ | — | — | — |  |
| NurbsTrimmedSurface | 4 | ✓ | ✗ | — | NRB-1 | X3DGeometryNode, X3DNurbsSurfaceGeometryNode, X3DParametricGeometryNode |

## Findings

- **NRB-1** [major/DEFERRED] — §27.4: NURBS geometry nodes absent from recognizedGeometryType() — extract inert.
  - runtime/extract/MeshBuilder.hpp:1239-1252 omits all NURBS surfaces; external_geom_seam_test.cpp confirms NurbsPatchSurface → recognized=false. Blocked on a NURBS tessellation subsystem. (sweep 2026-06-25)
- **NRB-2** [major/DEFERRED] — §27.4.5: NURBS interpolator set_fraction handlers are dead — value_changed never emitted.
  - NurbsOrientationInterpolator.hpp:158-161 onSet_fraction() is a no-op unless a handler is set; InterpolatorRegistration.hpp:35-61 registers 13 systems but none of the three NURBS interpolators. Events to set_fraction are dropped. Blocked on the NURBS evaluation subsystem (NRB-1). (sweep 2026-06-25)
- **CONTAINERFIELD-FALSEPOS** [minor/OPEN] — §ISO 19776-1 (containerField): CONTAINERFIELD_MISMATCH warns whenever containerField != the child's own default, false-positiving on legal non-default overrides (e.g. <NurbsCurve><Coordinate containerField='controlPoint'/></NurbsCurve>).
  - validate.py:76 compares only against the child default; the adjacent comment promises a parent-field check that is unimplemented. Fix: resolve whether containerField names a valid SF/MFNode field on the PARENT that accepts this node type before warning; add a NURBS controlPoint regression. (validation review.)

