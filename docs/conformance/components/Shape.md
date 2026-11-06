# Shape — conformance

_Generated. Levels 1,2,3,4,5 · 10 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| AcousticProperties | 5 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DAppearanceChildNode |
| Appearance | 1 | ✓ | — | — | MAT-001, MAT-006, MAT-009, MAT-010, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DAppearanceNode |
| FillProperties | 3 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, REQ-FILL, ROUTE-IO-ALIAS | X3DAppearanceChildNode |
| LineProperties | 2 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, SEAM-LINEPOINT | X3DAppearanceChildNode |
| Material | 1 | ✓ | — | — | AUD-RND-2, MAT-002, MAT-003, MAT-004, MAT-005, MAT-006, MAT-007, MAT-009, MAT-010, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DAppearanceChildNode, X3DMaterialNode, X3DOneSidedMaterialNode |
| PhysicalMaterial | 2 | ✓ | — | — | MAT-005, MAT-006, MAT-007, MAT-008, MAT-009, MAT-010, MAT-011, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DAppearanceChildNode, X3DMaterialNode, X3DOneSidedMaterialNode |
| PointProperties | 5 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, SEAM-LINEPOINT | X3DAppearanceChildNode |
| Shape | 1 | ✓ | — | — | EXTRACT-VISIBILITY-HINT-DELTA, MAT-001, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode, X3DShapeNode |
| TwoSidedMaterial | 4 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DAppearanceChildNode, X3DMaterialNode |
| UnlitMaterial | 1 | ✓ | — | — | MAT-005, MAT-007, MAT-009, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, UNLIT-EMISSIVE | X3DAppearanceChildNode, X3DMaterialNode, X3DOneSidedMaterialNode |

## Findings

- **MAT-001** [critical/CLOSED] — §12.4.2, 17.2.2.4: Appearance with material=NULL extracts a grey Phong material; spec mandates an UnlitMaterial white(1,1,1), lighting off, and the texture as emissive — a visibly wrong lit-grey surface.
  - materialOf() default branch must return Unlit emissive=(1,1,1) + texture in the Emissive slot, not Phong baseColor 0.8 grey.
- **MAT-002** [major/CLOSED] — §12.4.5, 17.2.2.5: Material.shininessTexture (alpha) has no descriptor slot — consumers can't modulate per-pixel shininess.
  - Add a Shininess TextureRef::Slot + appendSlot in the Phong texturesOf path.
- **MAT-004** [major/CLOSED] — §12.4.5: Material.occlusionStrength not read — authored non-default values dropped (occlusionTexture always applied at full strength).
- **MAT-011** [major/FIXED] — §12.4.6: Ambient occlusion derived from the metallic-roughness texture's R channel even when a separate occlusionTexture was bound (and applied even when none was).
  - material_shader_test.cpp: Occlusion samples the Occlusion slot rather than the metallic-roughness texture's red channel.
- **REQ-FILL** [major/FIXED] — §12.4.3: Appearance.fillProperties is not extracted; filled, hatched, hatchColor and hatchStyle have no rendering effect.
  - material_system_test.cpp, scene_extractor_snapshot_test.cpp: FillProperties reach material descriptors and live dependency updates.
- **PROTO-INSTANCE-IS-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.8: All four writers retain IS mappings on nested prototype instances.
- **PROTO-EMPTY-OVERRIDE-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.10: JSON and Classic writers retain explicit NULL and empty MFNode prototype overrides.
- **MAT-003** [minor/CLOSED] — §12.4.5, 17.2.2.5: Material.ambientTexture (RGB) has no descriptor slot — per-pixel ambient modulation unavailable.
- **MAT-005** [minor/CLOSED] — §12.3.4: normalScale (X3DOneSidedMaterialNode) not surfaced — normal-map intensity ignored.
- **MAT-006** [minor/CLOSED] — §12.2.3, 12.4.2: Appearance.backMaterial (two-sided, solid=FALSE) not extracted — no separate back-face MaterialDesc.
  - Closed — materialOf() now reads backMaterial via the same three-way dispatch as the front material and stores it as unique_ptr<MaterialDesc> on MaterialDesc; doubleSided=true set when backMaterial is present; backMaterialConstraintMet diagnostic added (050bc4b).
- **MAT-007** [minor/CLOSED] — §12.4.5, 17.2.3: xxxTextureMapping fields (X3D v4 per-texture UV-set selection) not surfaced — multi-UV authoring dropped.
  - Closed — TextureRef::texCoordMapping populated from the X3D v4 xxxTextureMapping field (fieldName + "Mapping" reflection read) in appendSlot(); empty string = UV set 0 (default).
- **MAT-010** [minor/CLOSED] — §12.2.3, 12.4.2: Appearance.backMaterial constraint checks material-model type only — texture-set match between front and back materials is not validated.
  - backMaterial textures are now populated from the BACK material node's own slots (matsys::materialSlotTextures, the same dispatch texturesOf uses for the front), and backMaterialConstraintMet compares both model type AND the (slot, channel) multisets (matsys::sameTextureSlotSet) per §12.2.3. Regression: material_system_test (front+back both diffuse → met; front textured/back not → not met). (2026-06-27)
- **SEAM-LINEPOINT** [minor/CLOSED] — §12.4.6 (LineProperties), 12.4.8 (PointProperties): Line width and point size are unrepresentable on the seam — MeshData/MaterialDesc carry no lineWidth/pointSize, so every IndexedLineSet draws at 1px regardless of LineProperties.linewidthScaleFactor.
  - linepoint_style_test.cpp: Line and point descriptors carry width, size, and attenuation to the CPU consumer.
- **EXTRACT-VISIBILITY-HINT-DELTA** [minor/FIXED]: Optional RenderItem origin-distance visibility hints stayed stale after view, limit and placement changes.
  - scene_extractor_visibility_delta_test.cpp: Changed per-placement visibility hints use remove-before-add replacement while unchanged hints preserve meshes.
- **MAT-008** [low/CLOSED] — §17.2.3: ORM channel-packing convention (R=Occlusion, G=Roughness, B=Metallic) not documented in the descriptor — consumers must guess.
  - Closed — TextureRef::Slot enum comment in RenderItem.hpp documents the ORM packing convention (glTF §3.9.4 / X3D §17); test pins single-slot MetallicRoughness emission.
- **MAT-009** [low/CLOSED] — §12.4.2: Gamma/sRGB output encoding stance not documented or enforced — PoC consumer rendered in linear space.
  - Closed — the stance is ADR-0027's (2026-09-26, superseding the first closure, which also encoded Phong output). PhysicalMaterial uses a linear workflow in both reference consumers (sRGB-decoded colour textures, linear factors, linearToSRGB() at output); Material (Phong) and UnlitMaterial stay in display space (no decode, no encode) so X3D 3.x content keeps its authored look. The SDK emits colours in authored space; gamma is consumer policy.
- **UNLIT-EMISSIVE** [low/CLOSED] — §12.4.10: ISO prose falsely calls emissiveColor inherited (base default 0 0 0); the base has no such field. UOM/engine correct - direct field, default 1 1 1.
  - shape.md:818 (note) is wrong vs shape.md:374 ("defined in implementing nodes"). UnlitMaterial declares emissiveColor directly, default 1 1 1; X3DOneSidedMaterialNode has no emissiveColor. x3d-cpp correct (UnlitMaterial.hpp:293/:44). Optional regression assert default == 1 1 1. 4.1 - confirmed; 4.1 UOM unchanged (direct field, 1 1 1; base still none).

