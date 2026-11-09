# Texturing — conformance

_Generated. Levels 1,2,3 · 10 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| ImageTexture | 1 | ✓ | — | — | AUD-TEX-1, DELTA-PENDING-RESOURCES, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, REQ-FTP, ROUTE-IO-ALIAS, TXF-4 | X3DAppearanceChildNode, X3DSingleTextureNode, X3DTexture2DNode, X3DTextureNode, X3DUrlObject |
| MovieTexture | 3 | ✓ | — | ? | AUD-MEDIA-1, AUD-MEDIA-3, AUD-MEDIA-4, AUD-TEX-1, AUD-TIME-3, META-EXTENDED-ATTRIBUTES-WRITE, MULTI-INHERIT, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, REQ-FTP, ROUTE-IO-ALIAS, TDN-5, VIS-MOVIE-DECODE | X3DAppearanceChildNode, X3DChildNode, X3DSingleTextureNode, X3DSoundNode, X3DSoundSourceNode, X3DTexture2DNode, X3DTextureNode, X3DTimeDependentNode, X3DUrlObject |
| MultiTexture | 2 | ✓ | — | — | AUD-BG-2, AUD-TEX-2, DELTA-PENDING-RESOURCES, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, REQ-MULTITEXTURE, REQ-MULTITEXTURE-2, ROUTE-IO-ALIAS | X3DAppearanceChildNode, X3DTextureNode |
| MultiTextureCoordinate | 2 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, REQ-MULTITEXTURE, REQ-MULTITEXTURE-2, ROUTE-IO-ALIAS, TXT-6 | X3DGeometricPropertyNode, X3DTextureCoordinateNode |
| MultiTextureTransform | 2 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, REQ-MULTITEXTURE, ROUTE-IO-ALIAS | X3DAppearanceChildNode, X3DTextureTransformNode |
| PixelTexture | 1 | ✓ | — | — | CLASSIC-SFIMAGE-LEADING-ZERO, ENC-VRML-SFIMAGE, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, TXF-4 | X3DAppearanceChildNode, X3DSingleTextureNode, X3DTexture2DNode, X3DTextureNode |
| TextureCoordinate | 1 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DGeometricPropertyNode, X3DSingleTextureCoordinateNode, X3DTextureCoordinateNode |
| TextureCoordinateGenerator | 2 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, TXF-2, TXF-EYE-COORDINATES | X3DGeometricPropertyNode, X3DSingleTextureCoordinateNode, X3DTextureCoordinateNode |
| TextureProperties | 2 | ✓ | — | — | AUD-TEX-3, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, TXF-4 |  |
| TextureTransform | 1 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, TXF-1, TXF-3 | X3DAppearanceChildNode, X3DTextureTransformNode |

## Findings

- **TDN-5** [major/CLOSED] — §8.2.4.1, 16.4.2, 18.4.2: AudioClip/MovieTexture have no time-lifecycle System — startTime/loop/isActive inert.
  - MediaTimeSystem (attached by attachStandardRuntime) runs the shared X3DTimeDependentSystem lifecycle for AudioClip, MovieTexture and BufferAudioSource: enabled, loop, and a cycle of duration_changed / pitch (AudioClip), / speed (MovieTexture), or buffer seconds over playbackRate x detune (BufferAudioSource); an unknown duration plays until stopTime (media_time_test, sound_immersive_test). Decoding and playback remain separate (SND-4).
- **TXF-2** [major/FIXED] — §18.4.8: Both reference hosts implement all eleven TextureCoordinateGenerator modes and transform the generated coordinates.
  - texcoord_gen_test.cpp, texgen_local_gl_test.py: The CPU and OpenGL reference shaders evaluate the eleven coordinate-generator modes per texture.
- **ENC-VRML-SFIMAGE** [major/FIXED `1e3c51d`] — §ISO 19776-2 5.3 (sfimageValue): ClassicVRML SFImage READER consumes only width and discards height/components/all pixel words — a 2x2 texture parses back as `2 0 0` (empty).
  - codec_string_hardening_test.cpp: ClassicVRML image reads preserve dimensions and packed pixel values.
- **VIS-SPHERE-NOUV** [major/FIXED] — §13.3.6, 18.4.10: Sphere with an ImageTexture (and NO Material) rendered untextured (flat white) — Geometry/Sphere/texture.x3d. FIXED: the VTS decal now applies (upright, vertically centred per the scene description).
  - mesh_builder_default_texcoords_test.cpp: Sphere tessellation supplies texture coordinates.
- **VIS-MOVIE-DECODE** [major/FIXED] — §23.4.1: MovieTexture frames were never decoded — TextureRef::Source::Movie was surfaced but no consumer decode path existed, so movie-textured scenes rendered the white/last fallback (blank). FIXED for MPEG-1: the MovieDecoder seam + pl_mpeg Backend A play the video onto geometry (the whole NIST corpus is MPEG-1).
  - movie_decoder_tests.cpp: pl_mpeg implements the frame lifecycle contract used by the OpenGL MovieTexture consumer.
- **AUD-TEX-2** [major/CLOSED] — §18.4.3: MultiTexture mode/source/function/color/alpha never reach the descriptor, so stages cannot be blended as authored.
  - TextureRef now retains ordered MultiTexture stages with mode, source, function, color and alpha, including defaults; resolver enrichment traverses stages. Test: MultiTexture stages preserve blend controls.
- **AUD-MEDIA-1** [major/CLOSED] — §18.4.2: set_speed changes an active movie's cycle; it must be ignored while playing.
  - Active MovieTexture speed deliveries are ignored and MediaTimeSystem holds the activation rate for cycle timing. Test: MovieTexture ignores speed changes while active. Completes TDN-5.
- **REQ-MULTITEXTURE** [major/FIXED] — §18.2.4; 18.4.3-18.4.5: Both reference hosts combine MultiTexture stages with their modes, source/function controls and independent stage coordinates.
  - multi_texture_test.cpp, interchange_test.cpp: The CPU reference host combines stages with separate RGB/alpha operators and source/function controls.
- **CLASSIC-SFIMAGE-LEADING-ZERO** [major/FIXED] — §ISO/IEC 19776-2 5.7, A.4: Classic image collection decodes dimensions with the shared SFInt32 parser so leading zeros remain decimal.
  - Pixel words retain hexadecimal support, and the VRML97 reader shares the collector. Test: reader_audit_test.cpp, Classic SFImage leading zero dimensions collect decimal pixel counts.
- **PROTO-INSTANCE-IS-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.8: All four writers retain IS mappings on nested prototype instances.
- **PROTO-EMPTY-OVERRIDE-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.10: JSON and Classic writers retain explicit NULL and empty MFNode prototype overrides.
- **DELTA-PENDING-RESOURCES** [major/FIXED] — §ISO/IEC 19775-1 15.4.2, 18.4.6: Pending textures and glyphs retry on later ticks and publish incremental completion.
- **TXF-4** [minor/FIXED] — §18.4.9: CPU reference sampler honored only repeat/clamp — MIRRORED_REPEAT and CLAMP_TO_BOUNDARY were ignored and the magnification filter was not applied.
  - texture_render_test.cpp, interchange_test.cpp: The CPU sampler applies extracted boundary, filter, mipmap, and anisotropy settings.
- **TXF-1** [minor/CLOSED] — §18.4.10: TextureTransform matrix application order divergence (translate/center/rotate/scale/−center) — UVs transformed incorrectly for non-trivial transforms.
  - texture_extract_test.cpp: Texture transforms use the matrix order C·S·R·C⁻¹·T.
- **TXF-EYE-COORDINATES** [minor/FIXED] — §18.4.8: CPU COORD-EYE fell back to SPHERE and CAMERASPACEREFLECTIONVECTOR used the reversed eye vector.
  - COORD-EYE now shares the raw camera-space position path; reflection uses E=normalize(-posEye), the position-to-eye direction required by Table 18.6. Direct signed/unbounded UV assertions and Phong/PBR ramp sampling cover both changes, with existing mode controls retained in x3d_cpuraster_texcoord_gen_test. Scope is CPU Phong/PBR only; remaining modes and the PoC are tracked by TXF-2. Issue #140. (2026-09-30)
- **TXF-3** [minor/FIXED] — §18.4.10: MatrixTextureTransform + MultiTextureTransform not handled (only the single TextureTransform).
  - Fixed for the generated model's available nodes: MultiTextureTransform now yields one TextureTransform2DParams per child and SceneExtractor applies transforms to matching MeshData.texcoordSets; TextureTransformMatrix3D projects its 4x4 transform over (s,t,0,1) onto the current 2D seam. TextureMatrixTransform appears in profile prose, but the generated concrete node is TextureTransformMatrix3D. Covered by texture_extract_test.
- **MULTI-INHERIT** [minor/CLOSED] — §16.4.2, 18.4.2: MovieTexture declared under two abstract node types; engine handles it via ADR-0004 virtual mixins. AudioClip is the clean single-node pattern (named by association).
  - test_emission.py: Virtual inheritance shares the node base and qualified field accessors resolve declaring ancestors.
- **AUD-TEX-1** [minor/CLOSED] — §9.3.2, 18.4.1: load FALSE on a texture still fetches its url.
  - Texture refs omit URL candidates while load is FALSE, so the consumer resolver cannot fetch them. Test: ImageTexture load FALSE suppresses URL fetch.
- **AUD-MEDIA-3** [minor/CLOSED] — §18.4.2: MovieTexture never sends duration_changed: the SDK opens no media (ADR-0041) and the MovieDecoder seam carries no duration.
  - The additive reportMovieDuration(context, movie, seconds) helper posts duration_changed through the cascade after consumer loading, feeding routes and MediaTimeSystem. Test: consumer reported MovieTexture duration drives lifecycle.
- **REQ-MULTITEXTURE-2** [minor/FIXED] — §18.4.3; 18.4.4: The OpenGL PoC now combines eight MultiTexture stages over eight UV sets, and every stage and material texture slot samples its own UV set or coordinate generator, as cpu_raster does.
  - multitexture_gl_test.py: The OpenGL host combines per-stage coordinates and texture operators within its stage capacity.
- **TXT-6** [low/FIXED] — §18.4.4: MultiTextureCoordinate silently falls back to default UVs (no 'point' field).
  - Fixed: MeshBuilder now unwraps MultiTextureCoordinate.texCoord children, preserves all channels in MeshData.texcoordSets, mirrors the first usable channel to legacy MeshData.texcoords, and emits fixed-width fallback (0,0) corners for empty channels. Covered by texture_extract_test.
- **AUD-TEX-3** [low/CLOSED] — §18.4.9: borderColor (deprecated in v4.0) is not extracted.
  - TextureProperties.borderColor now reaches ExtendedSamplerParams and the CPU reference sampler. Test: texture_extract_test custom borderColor assertions.

