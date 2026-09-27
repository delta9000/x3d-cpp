# EnvironmentalEffects — conformance

_Generated. Levels 1,2,3,4 · 5 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Background | 1 | ✓ | — | ✓ | AUD-BG-1, BIND-06, SEAM-BACKGROUND | X3DBackgroundNode, X3DBindableNode, X3DChildNode |
| Fog | 2 | ✓ | — | ✓ | BIND-06, ENV-10 | X3DBindableNode, X3DChildNode, X3DFogObject |
| FogCoordinate | 4 | ✓ | — | — | — | X3DGeometricPropertyNode |
| LocalFog | 4 | ✓ | — | — | — | X3DChildNode, X3DFogObject |
| TextureBackground | 3 | ✓ | — | ✓ | AUD-BG-1, AUD-BG-2, BIND-06, ENV-11, SEAM-BACKGROUND | X3DBackgroundNode, X3DBindableNode, X3DChildNode |

## Findings

- **BIND-06** [major/CLOSED `95d1107`] — §7.2.2: Deleted bound node doesn't behave as set_bind FALSE (raw ptrs, no removeNode/detach).
  - Shared with a System detach() hook; CONF-VIEWNAV cluster.
- **ENV-10** [major/CLOSED] — §24.4.2: Fog is inert — bound and round-tripped, but no fog effect reaches the shader.
  - Closed: FogDesc (RenderItem.hpp) + SceneExtractor::fog() surface the bound Fog's color/fogType/visibilityRange (world-scaled per the node's local coordinate frame); the §17 Table 17.5 fogInterpolant applies as the final step in examples/cpu_raster/cpuraster/MaterialShader.hpp (all models incl. Unlit) and poc_renderer lit/pbr/unlit.frag + main.cpp uFogColor/uFogType/uFogVisibilityRange uniforms. visibilityRange 0 disables fog. Tests: scene_extractor_fog_test (extraction incl. transform scale) and cpuraster fog_shader_test (LINEAR/EXPONENTIAL at d=0, V/2, d>=V; range 0). (2026-06-25)
- **ENV-11** [major/CLOSED] — §24.4.5: TextureBackground texture faces never read — renders identically to a plain Background.
  - Closed 2026-09-26: the six TextureBackground face textures reach BackgroundDesc via refOf (ImageTexture -> Url, PixelTexture -> Inline, MovieTexture -> Movie); cpu_raster renders them as the skybox (see SEAM-BACKGROUND).
- **SEAM-BACKGROUND** [major/CLOSED] — §24.4.2, 24.4.4: BackgroundDesc surfaces only sky/ground gradient — the six panorama *Url faces, TextureBackground textures, and Background.transparency never reach the seam; the gallery skybox only works because the example reads the *Url fields straight off the node.
  - Closed 2026-09-26: BackgroundDesc carries front/back/left/right/top/bottom TextureRefs (Background *Url lists as Source::Url, TextureBackground face nodes through refOf) plus transparency and hasPanorama(). cpu_raster builds its skybox from the descriptor and no longer reads the node (background_desc_test). poc_renderer still draws only the gradient.
- **AUD-BG-1** [minor/CLOSED] — §24.2.1: Decreasing skyAngle/groundAngle values are accepted silently.
  - Range validation emits BACKGROUND_ANGLE_ORDER warnings for decreasing skyAngle and groundAngle, preserving lenient reads (ADR-0003). Test: Background reports decreasing sky and ground angles.
- **AUD-BG-2** [minor/CLOSED] — §24.4.5: A MultiTexture panorama face is dropped from BackgroundDesc.
  - TextureRef now carries MultiTexture panorama stages and BackgroundDesc counts populated stages. Test: TextureBackground preserves MultiTexture panorama face. Completes ENV-11.

