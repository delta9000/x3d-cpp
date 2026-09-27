# Shaders — conformance

_Generated. Levels 1 · 8 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| ComposedShader | 1 | ✓ | — | — | ROUTE-IO-ALIAS | X3DAppearanceChildNode, X3DProgrammableShaderObject, X3DShaderNode |
| FloatVertexAttribute | 1 | ✓ | — | — | ROUTE-IO-ALIAS, SHDR-1 | X3DGeometricPropertyNode, X3DVertexAttributeNode |
| Matrix3VertexAttribute | 1 | ✓ | — | — | ROUTE-IO-ALIAS, SHDR-1 | X3DGeometricPropertyNode, X3DVertexAttributeNode |
| Matrix4VertexAttribute | 1 | ✓ | — | — | ROUTE-IO-ALIAS, SHDR-1 | X3DGeometricPropertyNode, X3DVertexAttributeNode |
| PackagedShader | 1 | ✓ | — | — | ROUTE-IO-ALIAS | X3DAppearanceChildNode, X3DProgrammableShaderObject, X3DShaderNode, X3DUrlObject |
| ProgramShader | 1 | ✓ | — | — | ROUTE-IO-ALIAS | X3DAppearanceChildNode, X3DShaderNode |
| ShaderPart | 1 | ✓ | — | — | ROUTE-IO-ALIAS | X3DUrlObject |
| ShaderProgram | 1 | ✓ | — | — | ROUTE-IO-ALIAS | X3DProgrammableShaderObject, X3DUrlObject |

## Findings

- **SHDR-1** [minor/CLOSED] — §31.4.2: Custom vertex-attribute nodes silently dropped — 'attrib' children never read.
  - Fixed: MeshBuilder extracts FloatVertexAttribute (1-4 components), Matrix3VertexAttribute (9), and Matrix4VertexAttribute (16) into named MeshData vertex streams, expanded by coordIndex alongside positions. Covered by custom vertex attributes follow expanded coordinate vertices in mesh_builder_t3_test.cpp.

