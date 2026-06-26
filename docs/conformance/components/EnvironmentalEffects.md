# EnvironmentalEffects — conformance

_Generated. Levels 1,2,3,4 · 5 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Background | 1 | ✓ | — | ◑ | BIND-06, SEAM-BACKGROUND | X3DBackgroundNode, X3DBindableNode, X3DChildNode |
| Fog | 2 | ✓ | — | ◑ | BIND-06, ENV-10 | X3DBindableNode, X3DChildNode, X3DFogObject |
| FogCoordinate | 4 | ✓ | — | — | — | X3DGeometricPropertyNode |
| LocalFog | 4 | ✓ | — | — | — | X3DChildNode, X3DFogObject |
| TextureBackground | 3 | ✓ | — | ◑ | BIND-06, ENV-11, SEAM-BACKGROUND | X3DBackgroundNode, X3DBindableNode, X3DChildNode |

## Findings

- **ENV-10** [major/OPEN] — §24.4.2: Fog is inert — matrix marks 'Behaves ✓' but no fog effect is rendered.
  - boundFog() declared at runtime/events/X3DExecutionContext.hpp:266 has zero production callers; ShaderUniformVocabulary.hpp:167-175 references a 'FogDesc' struct that does not exist; SceneExtractor has no fog readout. The EnvironmentalEffects.md matrix Behaves:✓ is false — correct the status and wire fog colour/type/range into the shader uniform path. (sweep 2026-06-25)
- **ENV-11** [major/OPEN] — §24.4.5: TextureBackground texture faces never read — matrix 'Behaves ✓' is false.
  - Only binding-stack recognition exists (BindingSystem.hpp:80-85); the six face-texture fields (back/front/left/right/top/bottomTexture) are read nowhere in runtime/. Behaves identically to a plain Background. Correct the EnvironmentalEffects.md status; full skybox texturing tracking blocked on cube/sky texture render support. (sweep 2026-06-25)
- **SEAM-BACKGROUND** [major/OPEN] — §24.4.2, 24.4.4: BackgroundDesc surfaces only sky/ground gradient — the six panorama *Url faces, TextureBackground textures, and Background.transparency never reach the seam; the gallery skybox only works because the example reads the *Url fields straight off the node.
  - background() SceneExtractor.hpp:351-363 + BackgroundDesc RenderItem.hpp:520-527 carry no panorama; examples/cpu_raster/main.cpp:204-209 bypasses the seam via node reflection, masking the gap. Fix: put the six *Url slots, transparency, and TextureBackground texture nodes on BackgroundDesc so the skybox is reproducible from the contract. (extraction-seam review.)
- **BIND-06** [major/CLOSED `95d1107`] — §7.2.2: Deleted bound node doesn't behave as set_bind FALSE (raw ptrs, no removeNode/detach).
  - Shared with a System detach() hook; CONF-VIEWNAV cluster.

