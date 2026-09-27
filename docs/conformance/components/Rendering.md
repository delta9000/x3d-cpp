# Rendering — conformance

_Generated. Levels 1,2,3,5 · 15 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| ClipPlane | 5 | ✓ | — | — | — | X3DChildNode |
| Color | 1 | ✓ | — | — | — | X3DColorNode, X3DGeometricPropertyNode |
| ColorRGBA | 1 | ✓ | — | — | — | X3DColorNode, X3DGeometricPropertyNode |
| Coordinate | 1 | ✓ | — | — | CONTAINERFIELD-FALSEPOS | X3DCoordinateNode, X3DGeometricPropertyNode |
| CoordinateDouble | 1 | ✓ | — | — | GEO-2 | X3DCoordinateNode, X3DGeometricPropertyNode |
| IndexedLineSet | 1 | ✓ | ✓ | — | AUD-RND-1, AUD-RND-2, SEAM-LINEPOINT | X3DGeometryNode |
| IndexedTriangleFanSet | 3 | ✓ | ✓ | — | EXT-002 | X3DComposedGeometryNode, X3DGeometryNode |
| IndexedTriangleSet | 3 | ✓ | ✓ | — | — | X3DComposedGeometryNode, X3DGeometryNode |
| IndexedTriangleStripSet | 3 | ✓ | ✓ | — | EXT-002 | X3DComposedGeometryNode, X3DGeometryNode |
| LineSet | 1 | ✓ | ✓ | — | AUD-RND-1, AUD-RND-2 | X3DGeometryNode |
| Normal | 2 | ✓ | — | — | — | X3DGeometricPropertyNode, X3DNormalNode |
| PointSet | 1 | ✓ | ✓ | — | AUD-RND-1, AUD-RND-2, SEAM-LINEPOINT | X3DGeometryNode |
| TriangleFanSet | 3 | ✓ | ✓ | — | EXT-002 | X3DComposedGeometryNode, X3DGeometryNode |
| TriangleSet | 3 | ✓ | ✓ | — | — | X3DComposedGeometryNode, X3DGeometryNode |
| TriangleStripSet | 3 | ✓ | ✓ | — | EXT-002 | X3DComposedGeometryNode, X3DGeometryNode |

## Findings

- **AUD-RND-1** [minor/OPEN] — §11.2.2.5: Authored normals on points/lines are dropped, so they can never be lit.
  - Spec: with a Normal node, 'points and lines shall be rendered using the same lighting equations'. Probe: audit_line_and_point_normals_enable_lighting.
- **AUD-RND-2** [minor/OPEN] — §11.2.2.5: Unlit points/lines take the Material's diffuse colour instead of emissiveColor.
  - Spec: 'geometry shall be rendered as unlit and only the emissiveColor is used.' Probe: audit_line_set_material_uses_emissive_color.
- **EXT-002** [major/CLOSED `fe4d730`] — §11.3.2, 11.4.13, 11.4.15: With colorPerVertex/normalPerVertex=FALSE, fan/strip sets index color/normal per TRIANGLE (faceNo++ per triangle) instead of per fan/strip — wrong colors/normals when a fan/strip has >1 triangle.
  - Increment faceNo per fan/strip primitive (per fanCount/stripCount entry, per -1 run for indexed), not per emitted triangle.
- **CONTAINERFIELD-FALSEPOS** [minor/CLOSED] — §ISO 19776-1 (containerField): CONTAINERFIELD_MISMATCH warns whenever containerField != the child's own default, false-positiving on legal non-default overrides (e.g. <NurbsCurve><Coordinate containerField='controlPoint'/></NurbsCurve>).
  - Closed by resolving whether the explicit containerField names an SF/MFNode field on the PARENT that accepts the child's type (manifest acceptableNodeTypes) before warning; only a field that exists on neither side is flagged. Regression: tests/conformance/test_validate.py (test_legal_containerfield_override_not_flagged, test_containerfield_naming_no_parent_field_still_flagged).

