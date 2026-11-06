# Shaders — conformance

_Generated. Levels 1 · 8 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| ComposedShader | 1 | ✓ | — | — | DELTA-DIRECTOUTPUT-AUTHOR, DELTA-SHADER-AUTHOR-NODE, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, REQ-SHADER, REQ-SHADER-2, REQ-SHADER-3, ROUTE-IO-ALIAS, SHADER-AUTHOR-FIELD-CODECS, SHADER-AUTHOR-FIELD-CODECS-READ | X3DAppearanceChildNode, X3DProgrammableShaderObject, X3DShaderNode |
| FloatVertexAttribute | 1 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, SHDR-1 | X3DGeometricPropertyNode, X3DVertexAttributeNode |
| Matrix3VertexAttribute | 1 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, SHDR-1 | X3DGeometricPropertyNode, X3DVertexAttributeNode |
| Matrix4VertexAttribute | 1 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, SHDR-1 | X3DGeometricPropertyNode, X3DVertexAttributeNode |
| PackagedShader | 1 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, REQ-FTP, REQ-SHADER, ROUTE-IO-ALIAS, SHADER-AUTHOR-FIELD-CODECS, SHADER-AUTHOR-FIELD-CODECS-READ | X3DAppearanceChildNode, X3DProgrammableShaderObject, X3DShaderNode, X3DUrlObject |
| ProgramShader | 1 | ✓ | — | — | DELTA-DIRECTOUTPUT-AUTHOR, DELTA-SHADER-AUTHOR-NODE, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, REQ-SHADER, REQ-SHADER-2, REQ-SHADER-3, ROUTE-IO-ALIAS | X3DAppearanceChildNode, X3DShaderNode |
| ShaderPart | 1 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, REQ-FTP, REQ-SHADER, ROUTE-IO-ALIAS | X3DUrlObject |
| ShaderProgram | 1 | ✓ | — | — | DELTA-DIRECTOUTPUT-AUTHOR, DELTA-SHADER-AUTHOR-NODE, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, REQ-FTP, REQ-SHADER, REQ-SHADER-2, REQ-SHADER-3, ROUTE-IO-ALIAS, SHADER-AUTHOR-FIELD-CODECS, SHADER-AUTHOR-FIELD-CODECS-READ | X3DProgrammableShaderObject, X3DUrlObject |

## Findings

- **REQ-SHADER-3** [minor/OPEN] — §31.2.2; 31.2.4: Geometry and tessellation stages are not linked, cube and 3D textures do not reach sampler uniforms, and node-valued author fields are read only from XML.
  - Geometry and tessellation stages are assembled into ShaderProgramDesc, but the GL PoC links only VERTEX+FRAGMENT and the CPU host interprets only the FRAGMENT stage (its rasterizer supplies the varyings). Texture fields are bound as sampler2D only: cube-map and 3D texture nodes reach ShaderFieldBinding::textures but neither host binds samplerCube or sampler3D. SFImage, MFImage and MFString fields have no GLSL form. Codecs: Classic and JSON readers capture programmable-shader declarations and node-valued author defaults; the XML writer re-emits author fields only for Script (ComposedShader/ShaderProgram fields are dropped on write). Acceptance: geometry/tessellation stages linked where the host supports them, samplerCube/sampler3D bindings, and node-valued author fields in every encoding.
- **DELTA-SHADER-AUTHOR-NODE** [major/FIXED] — §ISO/IEC 19775-1 §31.2.4: Replacing an SFNode or MFNode shader author field rebuilds extraction dependencies on its new subtree.
  - Structural classification consults the effective static and author field table. Tests: shader_selection_test.cpp replacement author texture remains a live shader dependency, for both field types.
- **DELTA-DIRECTOUTPUT-AUTHOR** [major/FIXED] — §ISO/IEC 19775-1 §29.2.6, §31.2.4: Script directOutput writes to shader author fields notify dirty tracking and update extracted uniforms.
  - SaiContext uses the dirty-aware execution-context write for static and author fields, retaining direct value mutation semantics. Type-mismatched author writes leave values and dirtiness unchanged. Tests: shader_selection_test.cpp directOutput author uniform writes reach the render delta.
- **REQ-SHADER** [major/FIXED] — §31.2.2.3; 31.2.4; 31.3.2; 31.4: Scene-authored programs never populated RenderItem.shaderProgram or drove shader selection outputs.
  - shader_selection_test.cpp, author_shader_gl_test.py: GLSL shader selection and host validation produce the selected stage sources.
- **SHADER-AUTHOR-FIELD-CODECS-READ** [major/FIXED] — §ISO/IEC 19776-2 A.3; ISO/IEC 19776-5 Annex A: Classic and JSON readers capture programmable-shader author field declarations and defaults.
  - Reader-side closure of SHADER-AUTHOR-FIELD-CODECS; writer behavior is tracked separately. Test: reader_audit_test.cpp, Classic and JSON capture programmable shader author fields.
- **PROTO-INSTANCE-IS-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.8: All four writers retain IS mappings on nested prototype instances.
- **SHADER-AUTHOR-FIELD-CODECS** [major/FIXED] — §ISO/IEC 19775-1, 31; ISO/IEC 19776-2, A.3: XML, canonical XML, and JSON writers retain programmable shader author declarations.
  - Writer half; Classic already emits declarations. JSON and Classic reader checks remain WARN pending the separate reader fixes.
- **PROTO-EMPTY-OVERRIDE-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.10: JSON and Classic writers retain explicit NULL and empty MFNode prototype overrides.
- **SHDR-1** [minor/CLOSED] — §31.4.2: Custom vertex-attribute nodes silently dropped — 'attrib' children never read.
  - Fixed: MeshBuilder extracts FloatVertexAttribute (1-4 components), Matrix3VertexAttribute (9), and Matrix4VertexAttribute (16) into named MeshData vertex streams, expanded by coordIndex alongside positions. Covered by custom vertex attributes follow expanded coordinate vertices in mesh_builder_attributes_test.cpp.
- **REQ-SHADER-2** [minor/FIXED] — §31.2.4; 31.4: Texture (SFNode/MFNode) and array (MF) author-field uniforms reach the program descriptor and both reference hosts bind them.
  - shader_selection_test.cpp, author_shader_test.cpp: MF author fields and texture-valued node fields become shader bindings.

