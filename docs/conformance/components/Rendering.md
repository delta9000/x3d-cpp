# Rendering — conformance

_Generated. Levels 1,2,3,5 · 15 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| ClipPlane | 5 | ✓ | — | — | REQ-CLIP, ROUTE-IO-ALIAS | X3DChildNode |
| Color | 1 | ✓ | — | — | ROUTE-IO-ALIAS | X3DColorNode, X3DGeometricPropertyNode |
| ColorRGBA | 1 | ✓ | — | — | ROUTE-IO-ALIAS | X3DColorNode, X3DGeometricPropertyNode |
| Coordinate | 1 | ✓ | — | — | CONTAINERFIELD-FALSEPOS, ROUTE-IO-ALIAS | X3DCoordinateNode, X3DGeometricPropertyNode |
| CoordinateDouble | 1 | ✓ | — | — | GEO-2, ROUTE-IO-ALIAS | X3DCoordinateNode, X3DGeometricPropertyNode |
| IndexedLineSet | 1 | ✓ | ✓ | — | AUD-RND-1, AUD-RND-2, ROUTE-IO-ALIAS, SEAM-LINEPOINT | X3DGeometryNode |
| IndexedTriangleFanSet | 3 | ✓ | ✓ | — | EXT-002, ROUTE-IO-ALIAS | X3DComposedGeometryNode, X3DGeometryNode |
| IndexedTriangleSet | 3 | ✓ | ✓ | — | ROUTE-IO-ALIAS | X3DComposedGeometryNode, X3DGeometryNode |
| IndexedTriangleStripSet | 3 | ✓ | ✓ | — | EXT-002, ROUTE-IO-ALIAS | X3DComposedGeometryNode, X3DGeometryNode |
| LineSet | 1 | ✓ | ✓ | — | AUD-RND-1, AUD-RND-2, ROUTE-IO-ALIAS | X3DGeometryNode |
| Normal | 2 | ✓ | — | — | ROUTE-IO-ALIAS | X3DGeometricPropertyNode, X3DNormalNode |
| PointSet | 1 | ✓ | ✓ | — | AUD-RND-1, AUD-RND-2, ROUTE-IO-ALIAS, SEAM-LINEPOINT | X3DGeometryNode |
| TriangleFanSet | 3 | ✓ | ✓ | — | EXT-002, ROUTE-IO-ALIAS | X3DComposedGeometryNode, X3DGeometryNode |
| TriangleSet | 3 | ✓ | ✓ | — | ROUTE-IO-ALIAS | X3DComposedGeometryNode, X3DGeometryNode |
| TriangleStripSet | 3 | ✓ | ✓ | — | EXT-002, ROUTE-IO-ALIAS | X3DComposedGeometryNode, X3DGeometryNode |

## Findings

- **EXT-002** [major/CLOSED `fe4d730`] — §11.3.2, 11.4.13, 11.4.15: With colorPerVertex/normalPerVertex=FALSE, fan/strip sets index color/normal per TRIANGLE (faceNo++ per triangle) instead of per fan/strip — wrong colors/normals when a fan/strip has >1 triangle.
  - Increment faceNo per fan/strip primitive (per fanCount/stripCount entry, per -1 run for indexed), not per emitted triangle.
- **REQ-CLIP** [major/FIXED] — §11.4.1; F.5: ClipPlane equations and enabled state reached extraction and the CPU reference rasterizer, but the GL poc_renderer ignored planes.
  - Wired: SceneExtractor collects enabled ClipPlane nodes as scoped state (a plane affects the following siblings and their subtrees within its parent grouping node) and carries the plane on RenderItem::clipPlanes (ClipPlaneList, fixed capacity kMaxClipPlanes=6 per Annex F.5) resolved to WORLD space (n' = M^-T n through the ClipPlane's world frame; x3d::runtime::transformPlane in runtime/math/Mat4.hpp). The CPU reference rasterizer (examples/cpu_raster) discards fragments where a*x+b*y+c*z+d < 0 in eye space (SceneRender.hpp maps each world plane through the view matrix, Rasterizer.hpp tests it). Posted ClipPlane field changes and indexed ancestor-frame changes now conservatively replace the extraction baseline, refreshing per-item world planes; disabled and empty scopes retain invalidation dependencies. Tests: scene_extractor_state_delta_test (channel-respecting mirror vs fresh snapshot, shared placements, disabled/empty scopes and unrelated-TRS retention), scene_extractor_clip_test (scope/enabled/world-transform/re-snapshot) and cpuraster clip_plane_test (half-space discard, multi-plane, all-clip). Fixed for the GL poc_renderer: it maps each plane to eye space per draw, lit.vert/unlit.vert write gl_ClipDistance for the Phong, PBR and unlit programs (GL_CLIP_DISTANCE0..n-1 enabled only for those draws), and author programs receive the planes through the numClipPlanes/clipPlane vocabulary uniforms and clip themselves. Test: examples/poc_renderer/tests/clip_plane_gl_test.py (each built-in program cut at x = 0; a disabled plane draws the box whole) under Xvfb. Camera near/far clipping is a different operation.
- **CONTAINERFIELD-FALSEPOS** [minor/CLOSED] — §ISO 19776-1 (containerField): CONTAINERFIELD_MISMATCH warns whenever containerField != the child's own default, false-positiving on legal non-default overrides (e.g. <NurbsCurve><Coordinate containerField='controlPoint'/></NurbsCurve>).
  - Closed by resolving whether the explicit containerField names an SF/MFNode field on the PARENT that accepts the child's type (manifest acceptableNodeTypes) before warning; only a field that exists on neither side is flagged. Regression: tests/conformance/test_validate.py (test_legal_containerfield_override_not_flagged, test_containerfield_naming_no_parent_field_still_flagged).
- **AUD-RND-1** [minor/CLOSED] — §11.2.2.5: Authored normals on points/lines are dropped, so they can never be lit.
  - Line and point meshes retain authored normals, and both example renderers light them. Covered by line_and_point_normals_enable_lighting and linepoint_style_test.
- **AUD-RND-2** [minor/CLOSED] — §11.2.2.5: Unlit points/lines take the Material's diffuse colour instead of emissiveColor.
  - MaterialDesc exposes unlitGeometryRGBA, which both example renderers use for normal-less lines and points. Covered by unlit_line_set_material_uses_emissive_color.

