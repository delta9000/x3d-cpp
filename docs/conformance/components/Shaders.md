# Shaders — conformance

_Generated. Levels 1 · 8 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| ComposedShader | 1 | ✓ | — | — | — | X3DAppearanceChildNode, X3DProgrammableShaderObject, X3DShaderNode |
| FloatVertexAttribute | 1 | ✓ | — | — | SHDR-1 | X3DGeometricPropertyNode, X3DVertexAttributeNode |
| Matrix3VertexAttribute | 1 | ✓ | — | — | SHDR-1 | X3DGeometricPropertyNode, X3DVertexAttributeNode |
| Matrix4VertexAttribute | 1 | ✓ | — | — | SHDR-1 | X3DGeometricPropertyNode, X3DVertexAttributeNode |
| PackagedShader | 1 | ✓ | — | — | — | X3DAppearanceChildNode, X3DProgrammableShaderObject, X3DShaderNode, X3DUrlObject |
| ProgramShader | 1 | ✓ | — | — | — | X3DAppearanceChildNode, X3DShaderNode |
| ShaderPart | 1 | ✓ | — | — | — | X3DUrlObject |
| ShaderProgram | 1 | ✓ | — | — | — | X3DProgrammableShaderObject, X3DUrlObject |

## Findings

- **SHDR-1** [minor/OPEN] — §31.4.2: Custom vertex-attribute nodes silently dropped — 'attrib' children never read.
  - runtime/extract/MeshBuilder.hpp:514-549 (buildAttrs) reads normal/color/texCoord but never the 'attrib' containerField, so all three X3DVertexAttributeNode types are unreachable from extraction. Not in shaders.md. (sweep 2026-06-25)

