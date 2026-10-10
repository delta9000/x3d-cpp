# Shaders — conformance

_Generated. Levels 1 · 8 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| ComposedShader | 1 | ✓ | — | — | REQ-SHADER, REQ-SHADER-2, ROUTE-IO-ALIAS | X3DAppearanceChildNode, X3DProgrammableShaderObject, X3DShaderNode |
| FloatVertexAttribute | 1 | ✓ | — | — | ROUTE-IO-ALIAS, SHDR-1 | X3DGeometricPropertyNode, X3DVertexAttributeNode |
| Matrix3VertexAttribute | 1 | ✓ | — | — | ROUTE-IO-ALIAS, SHDR-1 | X3DGeometricPropertyNode, X3DVertexAttributeNode |
| Matrix4VertexAttribute | 1 | ✓ | — | — | ROUTE-IO-ALIAS, SHDR-1 | X3DGeometricPropertyNode, X3DVertexAttributeNode |
| PackagedShader | 1 | ✓ | — | — | REQ-FTP, REQ-SHADER, ROUTE-IO-ALIAS | X3DAppearanceChildNode, X3DProgrammableShaderObject, X3DShaderNode, X3DUrlObject |
| ProgramShader | 1 | ✓ | — | — | REQ-SHADER, REQ-SHADER-2, ROUTE-IO-ALIAS | X3DAppearanceChildNode, X3DShaderNode |
| ShaderPart | 1 | ✓ | — | — | REQ-FTP, REQ-SHADER, ROUTE-IO-ALIAS | X3DUrlObject |
| ShaderProgram | 1 | ✓ | — | — | REQ-FTP, REQ-SHADER, REQ-SHADER-2, ROUTE-IO-ALIAS | X3DProgrammableShaderObject, X3DUrlObject |

## Findings

- **REQ-SHADER-2** [minor/OPEN] — §31.2.4; 31.4: Texture (SFNode) and array (MF) author-field uniforms reach the program descriptor without a value.
  - ShaderFieldBinding carries an X3DFieldValue, which has no texture or array alternative, so an SFNode field naming an X3DTextureNode, or an MFFloat/MFVec3f uniform array, is listed with an empty value and neither reference host binds it. Also ignored: geometry and tessellation stages are assembled but the GL PoC links only VERTEX+FRAGMENT, and the CPU host interprets only the FRAGMENT stage (its rasterizer supplies the varyings). Acceptance: texture uniforms resolve to TextureRefs a host can bind to sampler uniforms, and MF uniforms reach array uniforms, both with live updates.
- **REQ-SHADER** [major/FIXED] — §31.2.2.3; 31.2.4; 31.3.2; 31.4: Scene-authored programs never populated RenderItem.shaderProgram or drove shader selection outputs.
  - Fixed: runtime/extract/ShaderExtract.hpp::selectShader walks Appearance.shaders in order and selects the first ComposedShader or ProgramShader whose language is GLSL and whose program is valid; other node types and languages stay inert, which §31.2.2.3 permits (PackagedShader is not supported). Part sources come from the inline CDATA (sourceCode, now also captured for ShaderProgram), then the first url that resolves: data: urls decode in the SDK, other urls go to ShaderOptions::resolver (AssetKind::Shader), and a Pending url defers validity. Validity is ShaderOptions::validator (a host compile check), defaulting to a structural VERTEX+FRAGMENT check. SceneExtractor sets RenderItem::shaderProgram (stages, author fields with current values, language) and re-selects on appearance-subtree DirtyField, so uniform events, url/sourceCode edits and activate reach the incremental delta as updatedMaterial. ShaderSystem (attached by attachStandardRuntime and RuntimeSession) runs the same selection and emits isValid (first evaluation, changes, every activate TRUE) and isSelected (on selection changes); activate TRUE re-resolves urls and re-validates. Reference hosts: cpu_raster runs the selected fragment stage in its GLSL interpreter with interpreterShaderValidator; the GL PoC validates by compiling in its context and uploads every SF author-field type. Tests: shader_selection_test (11 cases), cpuraster author_shader_test and poc author_shader_gl_test (pixel output, uniform event, fall-through and material fallback). Remaining gaps are REQ-SHADER-2.
- **SHDR-1** [minor/CLOSED] — §31.4.2: Custom vertex-attribute nodes silently dropped — 'attrib' children never read.
  - Fixed: MeshBuilder extracts FloatVertexAttribute (1-4 components), Matrix3VertexAttribute (9), and Matrix4VertexAttribute (16) into named MeshData vertex streams, expanded by coordIndex alongside positions. Covered by custom vertex attributes follow expanded coordinate vertices in mesh_builder_t3_test.cpp.

