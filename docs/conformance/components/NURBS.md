# NURBS — conformance

_Generated. Levels 1,2,3,4 · 13 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Contour2D | 4 | ✓ | — | — | ROUTE-IO-ALIAS |  |
| ContourPolyline2D | 3 | ✓ | — | — | ROUTE-IO-ALIAS | X3DNurbsControlCurveNode |
| NurbsCurve | 1 | ✓ | ✓ | — | CONTAINERFIELD-FALSEPOS, NRB-1, NRB-4, ROUTE-IO-ALIAS | X3DGeometryNode, X3DParametricGeometryNode |
| NurbsCurve2D | 3 | ✓ | — | — | ROUTE-IO-ALIAS | X3DNurbsControlCurveNode |
| NurbsOrientationInterpolator | 1 | ✓ | — | — | NRB-2, ROUTE-IO-ALIAS | X3DChildNode |
| NurbsPatchSurface | 1 | ✓ | ✓ | — | NRB-1, NRB-4, ROUTE-IO-ALIAS | X3DGeometryNode, X3DNurbsSurfaceGeometryNode, X3DParametricGeometryNode |
| NurbsPositionInterpolator | 1 | ✓ | — | — | NRB-2, ROUTE-IO-ALIAS | X3DChildNode |
| NurbsSet | 2 | ✓ | — | — | ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode |
| NurbsSurfaceInterpolator | 1 | ✓ | — | — | NRB-2, ROUTE-IO-ALIAS | X3DChildNode |
| NurbsSweptSurface | 3 | ✓ | ✗ | — | NRB-3, ROUTE-IO-ALIAS | X3DGeometryNode, X3DParametricGeometryNode |
| NurbsSwungSurface | 3 | ✓ | ✗ | — | NRB-3, ROUTE-IO-ALIAS | X3DGeometryNode, X3DParametricGeometryNode |
| NurbsTextureCoordinate | 1 | ✓ | — | — | ROUTE-IO-ALIAS |  |
| NurbsTrimmedSurface | 4 | ✓ | ✗ | — | NRB-3, ROUTE-IO-ALIAS | X3DGeometryNode, X3DNurbsSurfaceGeometryNode, X3DParametricGeometryNode |

## Findings

- **NRB-3** [major/DEFERRED] — §27.4: Trimmed/swept/swung NURBS surfaces still absent from recognizedGeometryType() — extract inert.
  - The curve + patch slice shipped first-party (NRB-1, ADR-0040). NurbsTrimmedSurface/NurbsSweptSurface/NurbsSwungSurface remain unrecognized: trimmed surfaces need 2D contour-loop clipping and swept/swung need profile-sweep eval — both follow-up cards. They continue to route through MeshBuildOptions::externalGeometryResolver (the unrecognized-geometry fallback) and are the canonical unrecognized specimen in external_geom_seam_test.cpp + scene_extractor_audit_test.cpp. Whichever profile-curve/cross-section evaluator these three nodes end up sharing with NurbsCurve/NurbsPatchSurface will also need to settle the controlPoint weight-convention question tracked in NRB-4 (unweighted-input vs. premultiplied-input) — worth resolving once, not per-node. (NURBS curve+patch wave 2026-06-27)
- **NRB-1** [major/FIXED `5ddbe5d`] — §27.4: NurbsCurve → Lines and NurbsPatchSurface → Triangles now tessellate in extract.
  - Resolved for curve + patch by the first-party NURBS math unit runtime/extract/NurbsEval.hpp (Cox–de Boor + rational eval + analytic normals + periodic closed/uClosed/vClosed handling), wired into two MeshBuilder arms; recognizedGeometryType() now returns true for both. Clamped-uniform default knots, control-point convex-hull bounds. The still-deferred trimmed/swept/swung surfaces move to NRB-3. See ADR-0040. (NURBS curve+patch wave 2026-06-27)
- **NRB-2** [major/CLOSED] — §27.4.5: NURBS interpolator set_fraction handlers are dead — value_changed never emitted.
  - Registered event systems for all three NURBS interpolators reuse runtime/extract/NurbsEval.hpp for rational curve and surface evaluation; tangent orientation follows increasing knot direction. Covered by interpolator_conformance_test (linear curve, rational quarter circle/tangent, bilinear surface, degenerate input).
- **NRB-4** [minor/FIXED] — §27.3: NURBS controlPoint weight convention now defaults to premultiplied (matching the whole shipping ecosystem), with an opt-in Euclidean/textbook mode.
  - Resolved by option (b): runtime/extract/NurbsEval.hpp gained a NurbsWeightMode { Premultiplied, Euclidean } on CurveDef/SurfaceDef, threaded through evalSurface/tessellateCurve, and surfaced as MeshBuildOptions::nurbsWeightMode. The DEFAULT is Premultiplied — controlPoint is used verbatim as the homogeneous xyz — because the entire shipping X3D ecosystem requires it: primary-source list evidence (x3d-public 2020-05, 'Nurbs control points, preweighted or not?') has Andreas Plesch ('all freeWrl, view3dscene and InstantPlayer expect premultiplied control points') and Michalis Kamburelis ('all X3D browsers require points to be premultiplied by the given weight'), the convention was set by the blaxxun/bitmanagement browser ~20 years ago, White Dune stores premultiplied handles, and X3DOM deliberately switched to it (Plesch, 2020-05-28); the X3D 4.0 keep-compat decision preserved it. The literal textbook/spec-formula reading (§27, multiply weight*controlPoint inside the evaluator) remains available via NurbsWeightMode::Euclidean. X3D §27 is genuinely silent on the convention, so both are spec-permitted; the default is chosen for interop faithfulness. weight==1 content (the common case + every existing golden, incl. the unit-weight Bezier teapot) is bit-identical under both modes — verified by nurbs_curve_unit_weights_mode_agnostic. Coverage: nurbs_curve_exact_unit_circle (Euclidean corners) + nurbs_curve_premultiplied_unit_circle (premultiplied corners, default mode) both trace the exact unit circle. NRB-3's deferred trimmed/swept/swung surfaces should reuse s.weightMode when they land.
- **CONTAINERFIELD-FALSEPOS** [minor/CLOSED] — §ISO 19776-1 (containerField): CONTAINERFIELD_MISMATCH warns whenever containerField != the child's own default, false-positiving on legal non-default overrides (e.g. <NurbsCurve><Coordinate containerField='controlPoint'/></NurbsCurve>).
  - Closed by resolving whether the explicit containerField names an SF/MFNode field on the PARENT that accepts the child's type (manifest acceptableNodeTypes) before warning; only a field that exists on neither side is flagged. Regression: tests/conformance/test_validate.py (test_legal_containerfield_override_not_flagged, test_containerfield_naming_no_parent_field_still_flagged).

