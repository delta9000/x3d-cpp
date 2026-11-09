# EnvironmentalEffects — conformance

_Generated. Levels 1,2,3,4 · 5 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Background | 1 | ✓ | — | ? | AUD-BG-1, BIND-06, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, SEAM-BACKGROUND | X3DBackgroundNode, X3DBindableNode, X3DChildNode |
| Fog | 2 | ✓ | — | ? | BIND-06, ENV-10, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DBindableNode, X3DChildNode, X3DFogObject |
| FogCoordinate | 4 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, REQ-LOCALFOG, ROUTE-IO-ALIAS | X3DGeometricPropertyNode |
| LocalFog | 4 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, REQ-LOCALFOG, ROUTE-IO-ALIAS | X3DChildNode, X3DFogObject |
| TextureBackground | 3 | ✓ | — | ? | AUD-BG-1, AUD-BG-2, BIND-06, ENV-11, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, SEAM-BACKGROUND | X3DBackgroundNode, X3DBindableNode, X3DChildNode |

## Findings

- **BIND-06** [major/CLOSED] — §7.2.2: Deleted bound node doesn't behave as set_bind FALSE (raw ptrs, no removeNode/detach).
  - Shared with a System detach() hook; CONF-VIEWNAV cluster.
- **ENV-10** [major/CLOSED] — §24.4.2: Fog is inert — bound and round-tripped, but no fog effect reaches the shader.
  - scene_extractor_fog_test.cpp, fog_shader_test.cpp: Extracted global fog scales through its world frame and blends in the reference shader.
- **ENV-11** [major/CLOSED] — §24.4.5: TextureBackground texture faces never read — renders identically to a plain Background.
  - Closed 2026-09-26: the six TextureBackground face textures reach BackgroundDesc via refOf (ImageTexture -> Url, PixelTexture -> Inline, MovieTexture -> Movie); cpu_raster renders them as the skybox (see SEAM-BACKGROUND).
- **SEAM-BACKGROUND** [major/CLOSED] — §24.4.2, 24.4.4: BackgroundDesc surfaces only sky/ground gradient — the six panorama *Url faces, TextureBackground textures, and Background.transparency never reach the seam; the gallery skybox only works because the example reads the *Url fields straight off the node.
  - Closed 2026-09-26: BackgroundDesc carries front/back/left/right/top/bottom TextureRefs (Background *Url lists as Source::Url, TextureBackground face nodes through refOf) plus transparency and hasPanorama(). cpu_raster builds its skybox from the descriptor and no longer reads the node (background_desc_test). poc_renderer still draws only the gradient.
- **REQ-LOCALFOG** [major/FIXED] — §24.2.2; 24.4.3; 24.4.4: Global Fog, LocalFog scope and authored FogCoordinate depths work end to end in both reference hosts.
  - scene_extractor_fog_test.cpp, local_fog_gl_test.py: LocalFog follows enclosing placement scope and both reference renderer paths.
- **PROTO-INSTANCE-IS-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.8: All four writers retain IS mappings on nested prototype instances.
- **PROTO-EMPTY-OVERRIDE-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.10: JSON and Classic writers retain explicit NULL and empty MFNode prototype overrides.
- **AUD-BG-1** [minor/CLOSED] — §24.2.1: Decreasing skyAngle/groundAngle values are accepted silently.
  - Range validation emits BACKGROUND_ANGLE_ORDER warnings for decreasing skyAngle and groundAngle, preserving lenient reads (ADR-0003). Test: Background reports decreasing sky and ground angles.
- **AUD-BG-2** [minor/CLOSED] — §24.4.5: A MultiTexture panorama face is dropped from BackgroundDesc.
  - TextureRef now carries MultiTexture panorama stages and BackgroundDesc counts populated stages. Test: TextureBackground preserves MultiTexture panorama face. Completes ENV-11.

