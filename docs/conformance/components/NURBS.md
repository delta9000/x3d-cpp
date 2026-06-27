# NURBS — conformance

_Generated. Levels 1,2,3,4 · 13 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Contour2D | 4 | ✓ | — | — | — |  |
| ContourPolyline2D | 3 | ✓ | — | — | — | X3DNurbsControlCurveNode |
| NurbsCurve | 1 | ✓ | ✓ | — | CONTAINERFIELD-FALSEPOS, NRB-1, SEAM-2D-NURBS | X3DGeometryNode, X3DParametricGeometryNode |
| NurbsCurve2D | 3 | ✓ | — | — | — | X3DNurbsControlCurveNode |
| NurbsOrientationInterpolator | 1 | ✓ | — | — | NRB-2 | X3DChildNode |
| NurbsPatchSurface | 1 | ✓ | ✓ | — | NRB-1, SEAM-2D-NURBS | X3DGeometryNode, X3DNurbsSurfaceGeometryNode, X3DParametricGeometryNode |
| NurbsPositionInterpolator | 1 | ✓ | — | — | NRB-2 | X3DChildNode |
| NurbsSet | 2 | ✓ | — | — | — | X3DBoundedObject, X3DChildNode |
| NurbsSurfaceInterpolator | 1 | ✓ | — | — | NRB-2 | X3DChildNode |
| NurbsSweptSurface | 3 | ✓ | ✗ | — | NRB-3 | X3DGeometryNode, X3DParametricGeometryNode |
| NurbsSwungSurface | 3 | ✓ | ✗ | — | NRB-3 | X3DGeometryNode, X3DParametricGeometryNode |
| NurbsTextureCoordinate | 1 | ✓ | — | — | — |  |
| NurbsTrimmedSurface | 4 | ✓ | ✗ | — | NRB-3 | X3DGeometryNode, X3DNurbsSurfaceGeometryNode, X3DParametricGeometryNode |

## Findings

- **NRB-3** [major/DEFERRED] — §27.4: Trimmed/swept/swung NURBS surfaces still absent from recognizedGeometryType() — extract inert.
  - The curve + patch slice shipped first-party (NRB-1, ADR-0040). NurbsTrimmedSurface/NurbsSweptSurface/NurbsSwungSurface remain unrecognized: trimmed surfaces need 2D contour-loop clipping and swept/swung need profile-sweep eval — both follow-up cards. They continue to route through MeshBuildOptions::externalGeometryResolver (the unrecognized-geometry fallback) and are the canonical unrecognized specimen in external_geom_seam_test.cpp + scene_extractor_audit_test.cpp. (NURBS curve+patch wave 2026-06-27)
- **NRB-2** [major/DEFERRED] — §27.4.5: NURBS interpolator set_fraction handlers are dead — value_changed never emitted.
  - NurbsOrientationInterpolator.hpp:158-161 onSet_fraction() is a no-op unless a handler is set; InterpolatorRegistration.hpp:35-61 registers 13 systems but none of the three NURBS interpolators. Events to set_fraction are dropped. Blocked on the NURBS evaluation subsystem (NRB-1). (sweep 2026-06-25)
- **CONTAINERFIELD-FALSEPOS** [minor/OPEN] — §ISO 19776-1 (containerField): CONTAINERFIELD_MISMATCH warns whenever containerField != the child's own default, false-positiving on legal non-default overrides (e.g. <NurbsCurve><Coordinate containerField='controlPoint'/></NurbsCurve>).
  - validate.py:76 compares only against the child default; the adjacent comment promises a parent-field check that is unimplemented. Fix: resolve whether containerField names a valid SF/MFNode field on the PARENT that accepts this node type before warning; add a NURBS controlPoint regression. (validation review.)
- **NRB-1** [major/FIXED `5ddbe5d`] — §27.4: NurbsCurve → Lines and NurbsPatchSurface → Triangles now tessellate in extract.
  - Resolved for curve + patch by the first-party NURBS math unit runtime/extract/NurbsEval.hpp (Cox–de Boor + rational eval + analytic normals + periodic closed/uClosed/vClosed handling), wired into two MeshBuilder arms; recognizedGeometryType() now returns true for both. Clamped-uniform default knots, control-point convex-hull bounds. The still-deferred trimmed/swept/swung surfaces move to NRB-3. See ADR-0040. (NURBS curve+patch wave 2026-06-27)

