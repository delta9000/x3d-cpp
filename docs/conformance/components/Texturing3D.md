# Texturing3D — conformance

_Generated. Levels 1,2 · 7 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| ComposedTexture3D | 1 | ✓ | — | — | ROUTE-IO-ALIAS, T3D-1, T3D-2 | X3DAppearanceChildNode, X3DTexture3DNode, X3DTextureNode |
| ImageTexture3D | 2 | ✓ | — | — | REQ-FTP, ROUTE-IO-ALIAS, T3D-1, T3D-2 | X3DAppearanceChildNode, X3DTexture3DNode, X3DTextureNode, X3DUrlObject |
| PixelTexture3D | 1 | ✓ | — | — | ROUTE-IO-ALIAS, T3D-1, T3D-2 | X3DAppearanceChildNode, X3DTexture3DNode, X3DTextureNode |
| TextureCoordinate3D | 1 | ✓ | — | — | ROUTE-IO-ALIAS, T3D-3 | X3DGeometricPropertyNode, X3DSingleTextureCoordinateNode, X3DTextureCoordinateNode |
| TextureCoordinate4D | 1 | ✓ | — | — | ROUTE-IO-ALIAS, T3D-3 | X3DGeometricPropertyNode, X3DSingleTextureCoordinateNode, X3DTextureCoordinateNode |
| TextureTransform3D | 1 | ✓ | — | — | ROUTE-IO-ALIAS | X3DAppearanceChildNode, X3DTextureTransformNode |
| TextureTransformMatrix3D | 1 | ✓ | — | — | ROUTE-IO-ALIAS | X3DAppearanceChildNode, X3DTextureTransformNode |

## Findings

- **T3D-1** [major/DEFERRED] — §33.4.1, 33.4.2, 33.4.3: 3D textures: PixelTexture3D + ComposedTexture3D now extract a Source::Tex3D descriptor; ImageTexture3D and 3D sampling still ignored (white fallback).
  - WIRED: MaterialSystem::refOf now dispatches the X3DTexture3DNode family into TextureRef::Source::Tex3D (appended last in the Source enum, no value shift). PixelTexture3D materialises its inline image MFInt32 into Texture3DDesc{width,height,depth,numComponents,texels} via texextract::pixelTexture3DDesc (MSB-first unpack, numComponents clamped [0,4], bounded by the token stream — same ownership rules as the 2D SFImage); ComposedTexture3D reuses the 2D refOf path per slice, carrying tex3dSlices (one ref per texture node, tex3d.depth = slice count). repeatR is surfaced on TextureRef::repeatR and ExtendedSamplerParams::repeatR/boundaryModeR. IGNORED: ImageTexture3D stays unhandled — the SDK ships no 3D image decoder (NRRD/DDS/PNG-stack), so it neither yields a descriptor nor a Url; and neither bundled renderer samples 3D textures — cpu_raster Texture::fromRef and poc_renderer resolveTexRef return the flat material color for Source::Tex3D (a sampler3D path remains future work). Width/height of a ComposedTexture3D resolve only once its consumer decodes the 2D slices. Tests: texture_extract_test "test3DTextures". (T3D-1 slice 2026-09-28)
- **T3D-2** [minor/FIXED] — §33.4.1: repeatR (R-axis wrap) is never read for any 3D texture node.
  - Fixed: ExtendedSamplerParams gained repeatR + boundaryModeR; extendedSamplerOf reads repeatR (legacy path) and TextureProperties.boundaryModeR. Tested in texture_extract_test.cpp (testExtendedSampler 3D cases). (sweep 2026-06-25, fixed same day)
- **T3D-3** [minor/FIXED] — §33.4.4: TextureCoordinate3D/4D point arrays are never extracted.
  - Fixed: MeshBuilder reads TextureCoordinate3D MFVec3f and TextureCoordinate4D MFVec4f point arrays. On the current 2D MeshData seam it uses the spec-permitted implementation-dependent 2D-texture fallback: (s,t) for 3D and homogeneous (s/w,t/w) for 4D. Covered by texture_extract_test.

