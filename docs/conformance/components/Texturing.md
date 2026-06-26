# Texturing — conformance

_Generated. Levels 1,2,3 · 11 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| ImageTexture | 1 | ✓ | — | — | — | X3DAppearanceChildNode, X3DSingleTextureNode, X3DTexture2DNode, X3DTextureNode, X3DUrlObject |
| MovieTexture | 3 | ✓ | — | ◑ | MULTI-INHERIT, TDN-5 | X3DAppearanceChildNode, X3DChildNode, X3DSingleTextureNode, X3DSoundNode, X3DSoundSourceNode, X3DTexture2DNode, X3DTextureNode, X3DTimeDependentNode, X3DUrlObject |
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
- **ENC-VRML-SFIMAGE** [major/OPEN] — §ISO 19776-2 5.3 (sfimageValue): ClassicVRML SFImage READER consumes only width and discards height/components/all pixel words — a 2x2 texture parses back as `2 0 0` (empty).
  - Real scenes NetworkedCamera.x3d, examples/.../textured_text.x3d: `image='2 2 3 0 65280 255 16711680'` survives XML->JSON direct, the .x3dv even WRITES it correctly, but reading that .x3dv back yields `[2,0,0]` — every inline PixelTexture destroyed the instant the scene passes through ClassicVRML. Fix: the ClassicVRML SFImage parser must consume width*height pixel words after the `w h comp` header. (round-trip sweep, newly surfaced.)
- **TXF-1** [minor/DEFERRED] — §18.4.10: TextureTransform matrix application order divergence (translate/center/rotate/scale/−center) — UVs transformed incorrectly for non-trivial transforms.
  - Verify TextureTransform2D against §18.4.10 right-to-left order T·C·R·S·(−C); fix + regression.
- **TXF-3** [minor/DEFERRED] — §18.4.10: MatrixTextureTransform + MultiTextureTransform not handled (only the single TextureTransform).
  - MultiTexture family deferred (T-TEX-D1); MatrixTextureTransform is a small add when needed.
- **TXT-6** [low/OPEN] — §18.4.4: MultiTextureCoordinate silently falls back to default UVs (no 'point' field).
  - runtime/extract/MeshBuilder.hpp:537-538 reads getField<MFVec2f>(*tc,"point",{}); MultiTextureCoordinate has no 'point' (only MFNode 'texCoord'), so multi-UV extraction is lost to generateDefaultTexCoords(). Related to TXF-3 (MultiTextureTransform). Same getField mismatch class. (sweep 2026-06-25)
- **MULTI-INHERIT** [minor/CLOSED] — §16.4.2, 18.4.2: MovieTexture declared under two abstract node types; engine handles it via ADR-0004 virtual mixins. AudioClip is the clean single-node pattern (named by association).
  - UOM nominates one primary Inheritance + AdditionalInheritance; bindings emit every base public virtual (MovieTexture.hpp:40-42), the shared X3DNode collapses, and reflection accessors are qualified by declaring ancestor (MovieTexture.cpp:19,89,106). containerField defaults deterministic - MovieTexture=texture, AudioClip=source. No engine impact (see ADR-0004). 4.1 - confirmed; 4.1 did NOT do the X3DSoundSourceObject interface recast (still multiple inheritance), so the virtual-mixin approach remains the durable answer.

