# Texturing3D — conformance

_Generated. Levels 1,2 · 7 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| ComposedTexture3D | 1 | ✓ | — | — | T3D-1, T3D-2 | X3DAppearanceChildNode, X3DTexture3DNode, X3DTextureNode |
| ImageTexture3D | 2 | ✓ | — | — | T3D-1, T3D-2 | X3DAppearanceChildNode, X3DTexture3DNode, X3DTextureNode, X3DUrlObject |
| PixelTexture3D | 1 | ✓ | — | — | T3D-2 | X3DAppearanceChildNode, X3DTexture3DNode, X3DTextureNode |
| TextureCoordinate3D | 1 | ✓ | — | — | T3D-3 | X3DGeometricPropertyNode, X3DSingleTextureCoordinateNode, X3DTextureCoordinateNode |
| TextureCoordinate4D | 1 | ✓ | — | — | T3D-3 | X3DGeometricPropertyNode, X3DSingleTextureCoordinateNode, X3DTextureCoordinateNode |
| TextureTransform3D | 1 | ✓ | — | — | — | X3DAppearanceChildNode, X3DTextureTransformNode |
| TextureTransformMatrix3D | 1 | ✓ | — | — | — | X3DAppearanceChildNode, X3DTextureTransformNode |

## Findings

- **T3D-1** [major/DEFERRED] — §33.4.1, 33.4.2: 3D textures have zero extraction — yield no TextureRef (white fallback).
  - runtime/ references X3DTexture3DNode subtypes only in generated bindings; MaterialSystem/SceneExtractor texture dispatch does not recognise them, so Appearance.texture with a 3D texture renders white with no error. Blocked on 3D-texture sampling support in the render seam. (sweep 2026-06-25)
- **T3D-3** [minor/OPEN] — §33.4.4: TextureCoordinate3D/4D point arrays are never extracted.
  - texCoordGenOf() (runtime/extract/TextureExtract.hpp:227-238) dispatches only TextureCoordinateGenerator; nothing reads MFVec3f/MFVec4f point data from 3D/4D coord nodes, so geometry falls through to default §13 UV projection. (sweep 2026-06-25)
- **T3D-2** [minor/FIXED] — §33.4.1: repeatR (R-axis wrap) is never read for any 3D texture node.
  - Fixed: ExtendedSamplerParams gained repeatR + boundaryModeR; extendedSamplerOf reads repeatR (legacy path) and TextureProperties.boundaryModeR. Tested in texture_extract_test.cpp (testExtendedSampler 3D cases). (sweep 2026-06-25, fixed same day)

