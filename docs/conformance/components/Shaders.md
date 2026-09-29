# Shaders — conformance

_Generated. Levels 1 · 8 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| ComposedShader | 1 | ✓ | — | — | REQ-SHADER, ROUTE-IO-ALIAS | X3DAppearanceChildNode, X3DProgrammableShaderObject, X3DShaderNode |
| FloatVertexAttribute | 1 | ✓ | — | — | ROUTE-IO-ALIAS, SHDR-1 | X3DGeometricPropertyNode, X3DVertexAttributeNode |
| Matrix3VertexAttribute | 1 | ✓ | — | — | ROUTE-IO-ALIAS, SHDR-1 | X3DGeometricPropertyNode, X3DVertexAttributeNode |
| Matrix4VertexAttribute | 1 | ✓ | — | — | ROUTE-IO-ALIAS, SHDR-1 | X3DGeometricPropertyNode, X3DVertexAttributeNode |
| PackagedShader | 1 | ✓ | — | — | REQ-FTP, REQ-SHADER, ROUTE-IO-ALIAS | X3DAppearanceChildNode, X3DProgrammableShaderObject, X3DShaderNode, X3DUrlObject |
| ProgramShader | 1 | ✓ | — | — | REQ-SHADER, ROUTE-IO-ALIAS | X3DAppearanceChildNode, X3DShaderNode |
| ShaderPart | 1 | ✓ | — | — | REQ-FTP, REQ-SHADER, ROUTE-IO-ALIAS | X3DUrlObject |
| ShaderProgram | 1 | ✓ | — | — | REQ-FTP, REQ-SHADER, ROUTE-IO-ALIAS | X3DProgrammableShaderObject, X3DUrlObject |

## Findings

- **REQ-SHADER** [major/OPEN] — §31.2.2.3; 31.2.4; 31.3.2; 31.4: Shader parsing and descriptors exist, but scene-authored programs never populate RenderItem.shaderProgram or drive shader selection outputs.
  - SceneExtractor declares shaderProgram but never assigns it. The PoC's author branch requires that optional value, so authored programs cannot reach its compiler through the SDK feed. Acceptance: one supported language (e.g. GLSL), ordered shader selection, compilation validity/isSelected feedback, activate handling, live uniforms and URL/source changes. Unsupported shader languages may remain inert as the specification permits; implementing every language is not required.
- **SHDR-1** [minor/CLOSED] — §31.4.2: Custom vertex-attribute nodes silently dropped — 'attrib' children never read.
  - Fixed: MeshBuilder extracts FloatVertexAttribute (1-4 components), Matrix3VertexAttribute (9), and Matrix4VertexAttribute (16) into named MeshData vertex streams, expanded by coordIndex alongside positions. Covered by custom vertex attributes follow expanded coordinate vertices in mesh_builder_t3_test.cpp.

