# CubeMapTexturing — conformance

_Generated. Levels 1,2,3 · 3 nodes · profiles: Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| ComposedCubeMapTexture | 1 | ✓ | — | — | CMT-1 | X3DAppearanceChildNode, X3DEnvironmentTextureNode, X3DTextureNode |
| GeneratedCubeMapTexture | 3 | ✓ | — | — | — | X3DAppearanceChildNode, X3DEnvironmentTextureNode, X3DTextureNode |
| ImageCubeMapTexture | 2 | ✓ | — | — | — | X3DAppearanceChildNode, X3DEnvironmentTextureNode, X3DTextureNode, X3DUrlObject |

## Findings

- **CMT-1** [major/CLOSED] — §34.4.1: ComposedCubeMapTexture face textures silently dropped by MaterialSystem refOf().
  - Closed 2026-09-26: refOf maps ComposedCubeMapTexture to Source::Cube with cubeFaces in front/back/left/right/top/bottom order, and resolveTextureRefs resolves each face (background_desc_test). No bundled renderer samples cube maps yet; ImageCubeMapTexture stays a Url ref and GeneratedCubeMapTexture is not rendered.

