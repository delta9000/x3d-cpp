# Shape — conformance

_Generated. Levels 1,2,3,4,5 · 10 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| AcousticProperties | 5 | ✓ | — | — | — | X3DAppearanceChildNode |
| Appearance | 1 | ✓ | — | — | MAT-001, MAT-006, MAT-009, MAT-010 | X3DAppearanceNode |
| FillProperties | 3 | ✓ | — | — | — | X3DAppearanceChildNode |
| LineProperties | 2 | ✓ | — | — | SEAM-LINEPOINT | X3DAppearanceChildNode |
| Material | 1 | ✓ | — | — | MAT-002, MAT-003, MAT-004, MAT-005, MAT-006, MAT-007, MAT-009, MAT-010 | X3DAppearanceChildNode, X3DMaterialNode, X3DOneSidedMaterialNode |
| PhysicalMaterial | 2 | ✓ | — | — | MAT-005, MAT-006, MAT-007, MAT-008, MAT-009, MAT-010, MAT-011 | X3DAppearanceChildNode, X3DMaterialNode, X3DOneSidedMaterialNode |
| PointProperties | 5 | ✓ | — | — | SEAM-LINEPOINT | X3DAppearanceChildNode |
| Shape | 1 | ✓ | — | — | MAT-001 | X3DBoundedObject, X3DChildNode, X3DShapeNode |
| TwoSidedMaterial | 4 | ✓ | — | — | — | X3DAppearanceChildNode, X3DMaterialNode |
| UnlitMaterial | 1 | ✓ | — | — | MAT-005, MAT-007, MAT-009, UNLIT-EMISSIVE | X3DAppearanceChildNode, X3DMaterialNode, X3DOneSidedMaterialNode |

## Findings

- **MAT-010** [minor/OPEN] — §12.2.3, 12.4.2: Appearance.backMaterial constraint checks material-model type only — texture-set match between front and back materials is not validated.
  - backMaterialConstraintMet (materialOf(), see MAT-006) validates only that backMaterial.model == front.model. A prior design discussion for this exact front/back constraint scoped it to cover both the material-model type AND a matching texture set between front and back; only the model-type half is implemented. The back MaterialDesc is currently constructed inline with its textures vector left unpopulated (Appearance-level textures are front-only in the current extraction path), so a texture-set comparison is not yet meaningful to add without first populating back-material textures — tracked as the deferred follow-on already named in ADR-0021.
- **SEAM-LINEPOINT** [minor/OPEN] — §12.4.6 (LineProperties), 12.4.8 (PointProperties): Line width and point size are unrepresentable on the seam — MeshData/MaterialDesc carry no lineWidth/pointSize, so every IndexedLineSet draws at 1px regardless of LineProperties.linewidthScaleFactor.
  - Line/point funnel MeshBuilder.hpp:1690-1766 sets topology Lines/Points and per-vertex color but the descriptors have no width/size carrier. Fix: add lineWidth/pointSize (or a small LineProperties descriptor) so the GL glLineWidth/gl_PointSize path is feedable. (extraction-seam review.)
- **MAT-001** [critical/CLOSED `c86b731`] — §12.4.2, 17.2.2.4: Appearance with material=NULL extracts a grey Phong material; spec mandates an UnlitMaterial white(1,1,1), lighting off, and the texture as emissive — a visibly wrong lit-grey surface.
  - materialOf() default branch must return Unlit emissive=(1,1,1) + texture in the Emissive slot, not Phong baseColor 0.8 grey.
- **MAT-002** [major/CLOSED `c86b731`] — §12.4.5, 17.2.2.5: Material.shininessTexture (alpha) has no descriptor slot — consumers can't modulate per-pixel shininess.
  - Add a Shininess TextureRef::Slot + appendSlot in the Phong texturesOf path.
- **MAT-004** [major/CLOSED `c86b731`] — §12.4.5: Material.occlusionStrength not read — authored non-default values dropped (occlusionTexture always applied at full strength).
- **MAT-011** [major/FIXED] — §12.4.6: Ambient occlusion derived from the metallic-roughness texture's R channel even when a separate occlusionTexture was bound (and applied even when none was).
  - Per §12.4.6 occlusion comes from occlusionTexture (red channel) scaled by occlusionStrength; metallicRoughnessTexture carries roughness in G and metallic in B. Fixed in the CPU reference evaluator (examples/cpu_raster/cpuraster/MaterialShader.hpp) and the PoC GLSL (examples/poc_renderer/shaders/pbr.frag): AO now reads the Occlusion slot only; the MR texture's R is no longer an AO source (the extractor emits no ORM-packed marker). Regression: x3d_cpuraster_material_shader_test.
- **MAT-003** [minor/CLOSED `c86b731`] — §12.4.5, 17.2.2.5: Material.ambientTexture (RGB) has no descriptor slot — per-pixel ambient modulation unavailable.
- **MAT-005** [minor/CLOSED `c86b731`] — §12.3.4: normalScale (X3DOneSidedMaterialNode) not surfaced — normal-map intensity ignored.
- **MAT-006** [minor/CLOSED `4c47b58`] — §12.2.3, 12.4.2: Appearance.backMaterial (two-sided, solid=FALSE) not extracted — no separate back-face MaterialDesc.
  - Closed — materialOf() now reads backMaterial via the same three-way dispatch as the front material and stores it as unique_ptr<MaterialDesc> on MaterialDesc; doubleSided=true set when backMaterial is present; backMaterialConstraintMet diagnostic added (050bc4b).
- **MAT-007** [minor/CLOSED `f3ee804`] — §12.4.5, 17.2.3: xxxTextureMapping fields (X3D v4 per-texture UV-set selection) not surfaced — multi-UV authoring dropped.
  - Closed — TextureRef::texCoordMapping populated from the X3D v4 xxxTextureMapping field (fieldName + "Mapping" reflection read) in appendSlot(); empty string = UV set 0 (default).
- **MAT-008** [low/CLOSED `1cc7748`] — §17.2.3: ORM channel-packing convention (R=Occlusion, G=Roughness, B=Metallic) not documented in the descriptor — consumers must guess.
  - Closed — TextureRef::Slot enum comment in RenderItem.hpp documents the ORM packing convention (glTF §3.9.4 / X3D §17); test pins single-slot MetallicRoughness emission.
- **MAT-009** [low/CLOSED `954ad98`] — §12.4.2: Gamma/sRGB output encoding stance not documented or enforced — PoC consumer rendered in linear space.
  - Closed — the stance is ADR-0027's (2026-09-26, superseding the first closure, which also encoded Phong output). PhysicalMaterial uses a linear workflow in both reference consumers (sRGB-decoded colour textures, linear factors, linearToSRGB() at output); Material (Phong) and UnlitMaterial stay in display space (no decode, no encode) so X3D 3.x content keeps its authored look. The SDK emits colours in authored space; gamma is consumer policy.
- **UNLIT-EMISSIVE** [low/CLOSED] — §12.4.10: ISO prose falsely calls emissiveColor inherited (base default 0 0 0); the base has no such field. UOM/engine correct - direct field, default 1 1 1.
  - shape.md:818 (note) is wrong vs shape.md:374 ("defined in implementing nodes"). UnlitMaterial declares emissiveColor directly, default 1 1 1; X3DOneSidedMaterialNode has no emissiveColor. x3d-cpp correct (UnlitMaterial.hpp:293/:44). Optional regression assert default == 1 1 1. 4.1 - confirmed; 4.1 UOM unchanged (direct field, 1 1 1; base still none).

