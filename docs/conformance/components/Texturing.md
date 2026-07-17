# Texturing — conformance

_Generated. Levels 1,2,3 · 11 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| ImageTexture | 1 | ✓ | — | — | — | X3DAppearanceChildNode, X3DSingleTextureNode, X3DTexture2DNode, X3DTextureNode, X3DUrlObject |
| MovieTexture | 3 | ✓ | — | ◑ | MULTI-INHERIT, TDN-5, VIS-MOVIE-DECODE | X3DAppearanceChildNode, X3DChildNode, X3DSingleTextureNode, X3DSoundNode, X3DSoundSourceNode, X3DTexture2DNode, X3DTextureNode, X3DTimeDependentNode, X3DUrlObject |
| MultiTexture | 2 | ✓ | — | — | — | X3DAppearanceChildNode, X3DTextureNode |
| MultiTextureCoordinate | 2 | ✓ | — | — | TXT-6 | X3DGeometricPropertyNode, X3DTextureCoordinateNode |
| MultiTextureTransform | 2 | ✓ | — | — | — | X3DAppearanceChildNode, X3DTextureTransformNode |
| PixelTexture | 1 | ✓ | — | — | ENC-VRML-SFIMAGE | X3DAppearanceChildNode, X3DSingleTextureNode, X3DTexture2DNode, X3DTextureNode |
| TextureCoordinate | 1 | ✓ | — | — | — | X3DGeometricPropertyNode, X3DSingleTextureCoordinateNode, X3DTextureCoordinateNode |
| TextureCoordinateGenerator | 2 | ✓ | — | — | TXF-2 | X3DGeometricPropertyNode, X3DSingleTextureCoordinateNode, X3DTextureCoordinateNode |
| TextureProperties | 2 | ✓ | — | — | — |  |
| TextureTransform | 1 | ✓ | — | — | TXF-1, TXF-3 | X3DAppearanceChildNode, X3DTextureTransformNode |
| X3DSingleTextureTransformNode | 1 | ✓ | — | — | — | X3DAppearanceChildNode, X3DTextureTransformNode |

## Findings

- **TDN-5** [major/DEFERRED] — §8.2.4.1, 16.4.2, 18.4.2: AudioClip/MovieTexture have no time-lifecycle System — startTime/loop/isActive inert.
  - Blocked on the media/duration_changed seam — audio sources beyond OscillatorSource are not built (SND-4); the time-dependent lifecycle for sound sources is also inert (SND-2).
- **TXF-2** [major/DEFERRED] — §18.4.8: TextureCoordinateGenerator UVs (SPHERE/CAMERASPACE*) are view-dependent per-vertex and must be computed at render time — the descriptor is surfaced but no UVs are produced.
  - By-design render-time seam; ensure MeshData supplies the world/camera-frame normals the consumer needs. Documented.
- **TXF-1** [minor/DEFERRED] — §18.4.10: TextureTransform matrix application order divergence (translate/center/rotate/scale/−center) — UVs transformed incorrectly for non-trivial transforms.
  - Verify TextureTransform2D against §18.4.10 right-to-left order T·C·R·S·(−C); fix + regression.
- **ENC-VRML-SFIMAGE** [major/FIXED `1e3c51d`] — §ISO 19776-2 5.3 (sfimageValue): ClassicVRML SFImage READER consumes only width and discards height/components/all pixel words — a 2x2 texture parses back as `2 0 0` (empty).
  - Real scenes NetworkedCamera.x3d, examples/.../textured_text.x3d: `image='2 2 3 0 65280 255 16711680'` survives XML->JSON direct, the .x3dv even WRITES it correctly, but reading that .x3dv back yields `[2,0,0]` — every inline PixelTexture destroyed the instant the scene passes through ClassicVRML. Fix: the ClassicVRML SFImage parser must consume width*height pixel words after the `w h comp` header. (round-trip sweep, newly surfaced.) CLOSE: NodeBuilder::collectFieldValue special-cases SFImage (3 header tokens then w*h pixel words; hostile headers capped at 2^20 per axis). Regression: codec_string_hardening_test (SFImage operator== across the .x3dv hop).
- **VIS-SPHERE-NOUV** [major/FIXED] — §13.3.6, 18.4.10: Sphere with an ImageTexture (and NO Material) rendered untextured (flat white) — Geometry/Sphere/texture.x3d. FIXED: the VTS decal now applies (upright, vertically centred per the scene description).
  - Mischaracterized originally: the sphere DOES carry texcoords (tessellateSphere emits them). The real cause is the no-Material case — an Appearance with an ImageTexture but no Material is unlit per spec (§12.2.5), so the extractor correctly surfaces MaterialModel::Unlit with the image on the EMISSIVE slot (MaterialSystem.hpp). The PoC's UNLIT shader had no texcoord/sampler at all, so the texture was never sampled (Box/texture.x3d has a <Material/> -> Phong path, which is why it worked). Fixed PoC-side: added aTexCoord + sampler2D to unlit.vert/unlit.frag and bound the {Emissive,BaseColor,Diffuse} slot in the unlit draw path. SDK extraction was already spec-correct.
- **VIS-MOVIE-DECODE** [major/FIXED] — §23.4.1: MovieTexture frames were never decoded — TextureRef::Source::Movie was surfaced but no consumer decode path existed, so movie-textured scenes rendered the white/last fallback (blank). FIXED for MPEG-1: the MovieDecoder seam + pl_mpeg Backend A play the video onto geometry (the whole NIST corpus is MPEG-1).
  - Implemented per ADR-0041: the MovieDecoder seam (runtime/extract/MovieDecoder.hpp) + Backend A pl_mpeg (runtime/io/plmpeg/, flag-gated x3d_plmpeg) wired into the PoC, which decodes the current media-time frame and uploads it as the texture each tick. The NIST .mpg files are RAW elementary video streams (no MPEG-PS system layer), so the backend drives pl_mpeg's low-level plm_video_t directly for those (plm_t + seek for program streams). Verified by the x3d_movie_tests semantics-contract test and by rendering Appearance/Appearance/movietexture.x3d (the VTS card plays on box/sphere/cone/cylinder). Theora since shipped as MovieDecoder Backend B (TheoraMovieDecoder, seam-status.md — the seam is STABLE); WebM remains a follow-up. MovieTexture time-lifecycle nuance tracked by TDN-5.
- **TXF-3** [minor/FIXED] — §18.4.10: MatrixTextureTransform + MultiTextureTransform not handled (only the single TextureTransform).
  - Fixed for the generated model's available nodes: MultiTextureTransform now yields one TextureTransform2DParams per child and SceneExtractor applies transforms to matching MeshData.texcoordSets; TextureTransformMatrix3D projects its 4x4 transform over (s,t,0,1) onto the current 2D seam. TextureMatrixTransform appears in profile prose, but the generated concrete node is TextureTransformMatrix3D. Covered by texture_extract_test.
- **MULTI-INHERIT** [minor/CLOSED] — §16.4.2, 18.4.2: MovieTexture declared under two abstract node types; engine handles it via ADR-0004 virtual mixins. AudioClip is the clean single-node pattern (named by association).
  - UOM nominates one primary Inheritance + AdditionalInheritance; bindings emit every base public virtual (MovieTexture.hpp:40-42), the shared X3DNode collapses, and reflection accessors are qualified by declaring ancestor (MovieTexture.cpp:19,89,106). containerField defaults deterministic - MovieTexture=texture, AudioClip=source. No engine impact (see ADR-0004). 4.1 - confirmed; 4.1 did NOT do the X3DSoundSourceObject interface recast (still multiple inheritance), so the virtual-mixin approach remains the durable answer.
- **TXT-6** [low/FIXED] — §18.4.4: MultiTextureCoordinate silently falls back to default UVs (no 'point' field).
  - Fixed: MeshBuilder now unwraps MultiTextureCoordinate.texCoord children, preserves all channels in MeshData.texcoordSets, mirrors the first usable channel to legacy MeshData.texcoords, and emits fixed-width fallback (0,0) corners for empty channels. Covered by texture_extract_test.

