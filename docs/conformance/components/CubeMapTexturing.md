# CubeMapTexturing — conformance

_Generated. Levels 1,2,3 · 3 nodes · profiles: Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| ComposedCubeMapTexture | 1 | ✓ | — | — | CMT-1 | X3DAppearanceChildNode, X3DEnvironmentTextureNode, X3DTextureNode |
| GeneratedCubeMapTexture | 3 | ✓ | — | — | — | X3DAppearanceChildNode, X3DEnvironmentTextureNode, X3DTextureNode |
| ImageCubeMapTexture | 2 | ✓ | — | — | — | X3DAppearanceChildNode, X3DEnvironmentTextureNode, X3DTextureNode, X3DUrlObject |

## Findings

- **CMT-1** [major/OPEN] — §34.4.1: ComposedCubeMapTexture face textures silently dropped by MaterialSystem refOf().
  - runtime/extract/MaterialSystem.hpp:98-125 dispatches PixelTexture/MovieTexture then falls through to a 'url' branch; ComposedCubeMapTexture has no url field (faces are SFNode children), so it yields an empty TextureRef. No runtime code reads the six face-texture SFNodes. At minimum refOf() should recognise X3DEnvironmentTextureNode rather than silently mis-bucketing it. (sweep 2026-06-25)

