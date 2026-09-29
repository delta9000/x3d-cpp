# CubeMapTexturing — conformance

_Generated. Levels 1,2,3 · 3 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| ComposedCubeMapTexture | 1 | ✓ | — | — | CMT-1, REQ-CUBE, ROUTE-IO-ALIAS | X3DAppearanceChildNode, X3DEnvironmentTextureNode, X3DTextureNode |
| GeneratedCubeMapTexture | 3 | ✓ | — | — | REQ-CUBE, ROUTE-IO-ALIAS | X3DAppearanceChildNode, X3DEnvironmentTextureNode, X3DTextureNode |
| ImageCubeMapTexture | 2 | ✓ | — | — | REQ-CUBE, REQ-FTP, ROUTE-IO-ALIAS | X3DAppearanceChildNode, X3DEnvironmentTextureNode, X3DTextureNode, X3DUrlObject |

## Findings

- **REQ-CUBE** [major/OPEN] — §34.2; 34.4.1-34.4.3: Cube-map support ends at six-face descriptors; image and generated cube-map behavior and reference-consumer sampling are absent.
  - MaterialSystem::refOf special-cases ComposedCubeMapTexture only; ImageCubeMapTexture falls through to a 2D URL ref and GeneratedCubeMapTexture to an empty URL ref. Neither reference consumer samples cube refs. background_desc_test verifies six refs, not environment mapping. Acceptance: face orientation and reflection sampling, image cube loading, and generated update modes with the specified view and size.
- **CMT-1** [major/CLOSED] — §34.4.1: ComposedCubeMapTexture face textures silently dropped by MaterialSystem refOf().
  - Closed 2026-09-26: refOf maps ComposedCubeMapTexture to Source::Cube with cubeFaces in front/back/left/right/top/bottom order, and resolveTextureRefs resolves each face (background_desc_test). No bundled renderer samples cube maps yet; ImageCubeMapTexture stays a Url ref and GeneratedCubeMapTexture is not rendered.

