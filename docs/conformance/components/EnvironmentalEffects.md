# EnvironmentalEffects — conformance

_Generated. Levels 1,2,3,4 · 5 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Background | 1 | ✓ | — | ◑ | BIND-06, SEAM-BACKGROUND | X3DBackgroundNode, X3DBindableNode, X3DChildNode |
| Fog | 2 | ✓ | — | ✓ | BIND-06, ENV-10 | X3DBindableNode, X3DChildNode, X3DFogObject |
| FogCoordinate | 4 | ✓ | — | — | — | X3DGeometricPropertyNode |
| LocalFog | 4 | ✓ | — | — | — | X3DChildNode, X3DFogObject |
| TextureBackground | 3 | ✓ | — | ✗ | BIND-06, ENV-11, SEAM-BACKGROUND | X3DBackgroundNode, X3DBindableNode, X3DChildNode |

## Findings

- **ENV-11** [major/OPEN] — §24.4.5: TextureBackground texture faces never read — renders identically to a plain Background.
  - Only binding-stack recognition exists (BindingSystem.hpp:80-85); the six face-texture fields (back/front/left/right/top/bottomTexture) are read nowhere in runtime/. `behaves: inert` forces the matrix to ✗ for the node's own feature; full skybox texturing tracking blocked on cube/sky texture render support. (sweep 2026-06-25)
- **SEAM-BACKGROUND** [major/OPEN] — §24.4.2, 24.4.4: BackgroundDesc surfaces only sky/ground gradient — the six panorama *Url faces, TextureBackground textures, and Background.transparency never reach the seam; the gallery skybox only works because the example reads the *Url fields straight off the node.
  - background() SceneExtractor.hpp:351-363 + BackgroundDesc RenderItem.hpp:520-527 carry no panorama; examples/cpu_raster/main.cpp:204-209 bypasses the seam via node reflection, masking the gap. Fix: put the six *Url slots, transparency, and TextureBackground texture nodes on BackgroundDesc so the skybox is reproducible from the contract. (extraction-seam review.)
- **BIND-06** [major/CLOSED `95d1107`] — §7.2.2: Deleted bound node doesn't behave as set_bind FALSE (raw ptrs, no removeNode/detach).
  - Shared with a System detach() hook; CONF-VIEWNAV cluster.
- **ENV-10** [major/CLOSED] — §24.4.2: Fog is inert — bound and round-tripped, but no fog effect reaches the shader.
  - Closed: FogDesc (RenderItem.hpp) + SceneExtractor::fog() surface the bound Fog's color/fogType/visibilityRange (world-scaled per the node's local coordinate frame); the §17 Table 17.5 fogInterpolant applies as the final step in examples/cpu_raster/cpuraster/MaterialShader.hpp (all models incl. Unlit) and poc_renderer lit/pbr/unlit.frag + main.cpp uFogColor/uFogType/uFogVisibilityRange uniforms. visibilityRange 0 disables fog. Tests: scene_extractor_fog_test (extraction incl. transform scale) and cpuraster fog_shader_test (LINEAR/EXPONENTIAL at d=0, V/2, d>=V; range 0). (2026-06-25)

