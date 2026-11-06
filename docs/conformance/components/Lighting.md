# Lighting — conformance

_Generated. Levels 1,2 · 3 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| DirectionalLight | 1 | ✓ | — | — | AUD-LGT-2, DELTA-LIGHT-CACHE, LGT-1, LGT-2, LGT-3, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, REQ-SHADOW, ROUTE-IO-ALIAS | X3DChildNode, X3DLightNode |
| PointLight | 2 | ✓ | — | — | AUD-LGT-1, AUD-LGT-2, DELTA-LIGHT-CACHE, LGT-1, LGT-2, LGT-3, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, REQ-SHADOW, ROUTE-IO-ALIAS | X3DChildNode, X3DLightNode |
| SpotLight | 2 | ✓ | — | — | AUD-LGT-1, AUD-LGT-2, DELTA-LIGHT-CACHE, LGT-1, LGT-2, LGT-3, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, REQ-SHADOW, ROUTE-IO-ALIAS | X3DChildNode, X3DLightNode |

## Findings

- **LGT-2** [major/FIXED] — §17.2.2.4: Per-light ambientIntensity (LightSystem.hpp extracts it) was read by NO consumer.
  - light_ambient_test.cpp: The CPU reference shader accumulates attenuated ambient contributions per light.
- **LGT-3** [major/FIXED] — §23.4.4: NavigationInfo.headlight TRUE (default) must turn on a headlight regardless of scene lights; both consumers only added it when the scene had no lights.
  - interchange_test.cpp: NavigationInfo.headlight controls the synthetic headlight independently of authored lights.
- **AUD-LGT-1** [major/CLOSED] — §17.4.2, 17.4.3: Light radius ignores ancestor scale.
  - LightSystem scales radius through ancestor transforms. Covered by point_light_radius_scales_with_parent_transform and spot_light_radius_scales_with_parent_transform.
- **AUD-LGT-2** [major/CLOSED] — §10.4.3: Lights in an unchosen Switch branch (or inactive LOD level) still illuminate.
  - LightSystem and SceneExtractor share Switch and LOD branch selection. Covered by light_under_inactive_switch_does_not_illuminate and light_under_unselected_lod_level_does_not_illuminate.
- **REQ-SHADOW** [major/FIXED] — §17.2.2.8; 17.3.1: Both reference hosts apply per-light occlusion by castShadow shapes and shadowIntensity.
  - shadow_gl_test.py: The OpenGL host uses per-light shadow depth maps.
- **PROTO-INSTANCE-IS-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.8: All four writers retain IS mappings on nested prototype instances.
- **PROTO-EMPTY-OVERRIDE-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.10: JSON and Classic writers retain explicit NULL and empty MFNode prototype overrides.
- **LGT-1** [minor/FIXED] — §17.3.1: shadows / shadowIntensity now extracted onto LightDesc.
  - light_system_test.cpp, shadow_gl_test.py: Shadow settings reach light descriptors and the reference host shadow paths.
- **DELTA-LIGHT-CACHE** [minor/FIXED] — §ISO/IEC 19775-1 17.3.1: Light-change deltas refresh cached descriptors and per-placement scope indices.
- **ENVLIGHT-ORPHAN** [low/CLOSED] — §17.5 Table 17.6: EnvironmentLight orphaned in 4.0 Table 17.6 only; removed from the 4.0 body and X3DUOM. Engine correctly omits it for 4.0.
  - test_committed_manifests.py: Expected node availability is derived from the versioned UOM rather than the prose-table orphan.

