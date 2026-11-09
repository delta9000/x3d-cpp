# NURBS — conformance

_Generated. Levels 1,2,3,4 · 13 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Contour2D | 4 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS |  |
| ContourPolyline2D | 3 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DNurbsControlCurveNode |
| NurbsCurve | 1 | ✓ | ✓ | — | CONTAINERFIELD-FALSEPOS, DELTA-SWEPT-TRAJECTORY, META-EXTENDED-ATTRIBUTES-WRITE, NRB-1, NRB-4, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DGeometryNode, X3DParametricGeometryNode |
| NurbsCurve2D | 3 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DNurbsControlCurveNode |
| NurbsOrientationInterpolator | 1 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, NRB-2, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode |
| NurbsPatchSurface | 1 | ✓ | ✓ | — | META-EXTENDED-ATTRIBUTES-WRITE, NRB-1, NRB-4, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DGeometryNode, X3DNurbsSurfaceGeometryNode, X3DParametricGeometryNode |
| NurbsPositionInterpolator | 1 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, NRB-2, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode |
| NurbsSet | 2 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode |
| NurbsSurfaceInterpolator | 1 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, NRB-2, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DChildNode |
| NurbsSweptSurface | 3 | ✓ | ✓ | — | DELTA-SWEPT-TRAJECTORY, META-EXTENDED-ATTRIBUTES-WRITE, NRB-3, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DGeometryNode, X3DParametricGeometryNode |
| NurbsSwungSurface | 3 | ✓ | ✓ | — | META-EXTENDED-ATTRIBUTES-WRITE, NRB-3, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DGeometryNode, X3DParametricGeometryNode |
| NurbsTextureCoordinate | 1 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS |  |
| NurbsTrimmedSurface | 4 | ✓ | ✗ | — | DELTA-PACKED-PAYLOAD, META-EXTENDED-ATTRIBUTES-WRITE, NRB-3, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DGeometryNode, X3DNurbsSurfaceGeometryNode, X3DParametricGeometryNode |

## Findings

- **NRB-3** [major/DEFERRED] — §27.4: NurbsSweptSurface/NurbsSwungSurface now tessellate in extract; NurbsTrimmedSurface stays absent from recognizedGeometryType().
  - Swept + swung DONE (2026-07): both are recognized and tessellate into the shared patch-surface mesh form (Triangles, per-vertex normals, unit-square texcoords) via NurbsEval.hpp helpers — tessellateCurve2D (an X3DNurbsControlCurveNode lifted to the shared evaluator), sweptSurfaceGrid (cross-section carried in the Frenet-ish frame of the trajectory tangent: a circle swept along a straight line is a cylinder) and swungSurfaceGrid (the classic swung S(u,v)=(px·tx, px·ty, py): a line profile swung around a circle is a surface of revolution). They honor solid/ccw/tessellation like the patch surface, read the profile/cross-section via the control-curve fields (order/knot/weight/closed; ContourPolyline2D forced piecewise-linear order 2), reuse s.weightMode (NRB-4), and gain conservative GeometryBounds arms (trajectory control hull inflated by the cross-section radius; swung by |px|·|t|, z from the profile height). Covered by nurbs_swept_circle_along_line_is_cylinder + nurbs_swung_line_profile_around_circle_revolves + nurbs_swept_swung_recognized_oracle (mesh_builder_nurbs_swept_test.cpp). NurbsTrimmedSurface NOT DONE: it needs 2D contour-loop point-in-region clipping + polygon triangulation in the (u,v) domain, and the codebase has no such helper (MeshBuilder's only triangulators are lattice/strip/fan grid emitters). It remains unrecognized, routes through MeshBuildOptions::externalGeometryResolver, and is the canonical unrecognized specimen in external_geom_seam_test.cpp + scene_extractor_audit_test.cpp. Implementing it is a separate card (add a 2D-constrained-triangulation subsystem first). (NURBS curve+patch wave 2026-06-27; swept/swung 2026-07)
- **NRB-1** [major/FIXED `5ddbe5d`] — §27.4: NurbsCurve → Lines and NurbsPatchSurface → Triangles now tessellate in extract.
  - Resolved for curve + patch by the first-party NURBS math unit runtime/extract/NurbsEval.hpp (Cox–de Boor + rational eval + analytic normals + periodic closed/uClosed/vClosed handling), wired into two MeshBuilder arms; recognizedGeometryType() now returns true for both. Clamped-uniform default knots, control-point convex-hull bounds. The still-deferred trimmed/swept/swung surfaces move to NRB-3. See ADR-0040. (NURBS curve+patch wave 2026-06-27)
- **NRB-2** [major/CLOSED] — §27.4.5: NURBS interpolator set_fraction handlers are dead — value_changed never emitted.
  - Registered event systems for all three NURBS interpolators reuse runtime/extract/NurbsEval.hpp for rational curve and surface evaluation; tangent orientation follows increasing knot direction. Covered by interpolator_conformance_test (linear curve, rational quarter circle/tangent, bilinear surface, degenerate input).
- **PROTO-INSTANCE-IS-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.8: All four writers retain IS mappings on nested prototype instances.
- **PROTO-EMPTY-OVERRIDE-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.10: JSON and Classic writers retain explicit NULL and empty MFNode prototype overrides.
- **DELTA-SWEPT-TRAJECTORY** [major/FIXED] — §ISO/IEC 19775-1 27.4: Swept-surface deltas include the trajectory control-point dependency.
- **DELTA-PACKED-PAYLOAD** [major/FIXED] — §ISO/IEC 19775-1 4.4.8, 27.4: Packed geometry deltas refresh resolver payloads and retry Pending placements.
- **NRB-4** [minor/FIXED] — §27.3: NURBS controlPoint weight convention now defaults to premultiplied (matching the whole shipping ecosystem), with an opt-in Euclidean/textbook mode.
  - nurbs_eval_test.cpp: NURBS evaluation supports explicit Premultiplied and Euclidean weight modes, defaulting to Premultiplied.
- **CONTAINERFIELD-FALSEPOS** [minor/CLOSED] — §ISO 19776-1 (containerField): CONTAINERFIELD_MISMATCH warns whenever containerField != the child's own default, false-positiving on legal non-default overrides (e.g. <NurbsCurve><Coordinate containerField='controlPoint'/></NurbsCurve>).
  - Closed by resolving whether the explicit containerField names an SF/MFNode field on the PARENT that accepts the child's type (manifest acceptableNodeTypes) before warning; only a field that exists on neither side is flagged. Regression: tests/conformance/test_validate.py (test_legal_containerfield_override_not_flagged, test_containerfield_naming_no_parent_field_still_flagged).

