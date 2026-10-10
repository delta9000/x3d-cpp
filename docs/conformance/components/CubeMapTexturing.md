# CubeMapTexturing — conformance

_Generated. Levels 1,2,3 · 3 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| ComposedCubeMapTexture | 1 | ✓ | — | — | CMT-1, REQ-CUBE, ROUTE-IO-ALIAS | X3DAppearanceChildNode, X3DEnvironmentTextureNode, X3DTextureNode |
| GeneratedCubeMapTexture | 3 | ✓ | — | — | REQ-CUBE, ROUTE-IO-ALIAS | X3DAppearanceChildNode, X3DEnvironmentTextureNode, X3DTextureNode |
| ImageCubeMapTexture | 2 | ✓ | — | — | REQ-CUBE, REQ-FTP, ROUTE-IO-ALIAS | X3DAppearanceChildNode, X3DEnvironmentTextureNode, X3DTextureNode, X3DUrlObject |

## Findings

- **REQ-CUBE** [major/OPEN] — §34.2; 34.4.1-34.4.3: Both reference hosts sample ComposedCubeMapTexture as an environment map; image and generated cube maps are still absent.
  - MaterialSystem::refOf special-cases ComposedCubeMapTexture only; ImageCubeMapTexture falls through to a 2D URL ref and GeneratedCubeMapTexture to an empty URL ref, so neither renders. ComposedCubeMapTexture (2026-10-10): cpu_raster (Texture::sampleCube, MaterialShader environmentDirection) and the OpenGL PoC (shaders/multitexture.glsl sampleCubeFaces, a six-layer texture array on unit 12) sample a cube ref in the base-colour slot or as a MultiTexture stage. The lookup direction is the full (s, t, r) vector of the geometry's TextureCoordinateGenerator (all Table 18.6 modes; NOISE adds a third noise channel), or with no generator the camera-space reflection vector; §34.2.2 names CAMERASPACEREFLECTIONVECTOR as the usual generator but does not set a default, so that default is an implementation choice. Faces follow Figure 34.1 with the Background panorama convention (front -Z, back +Z, left -X, right +X, top +Y, bottom -Y, each upright seen from the centre). Tests: cube_map_test (cpu_raster) and cube_map_gl_test.py (validate-examples). Not wired: TextureCoordinate3D/4D explicit (s, t, r) coordinates, TextureTransform3D/Matrix3D on the direction, cube maps in slots other than base colour, and in the GL PoC more than one cube map per material (later cube stages reuse the first) and faces of unequal size (resampled to the largest). Acceptance for the rest: image cube loading, and generated update modes with the specified view and size.
- **CMT-1** [major/CLOSED] — §34.4.1: ComposedCubeMapTexture face textures silently dropped by MaterialSystem refOf().
  - Closed 2026-09-26: refOf maps ComposedCubeMapTexture to Source::Cube with cubeFaces in front/back/left/right/top/bottom order, and resolveTextureRefs resolves each face (background_desc_test). No bundled renderer samples cube maps yet; ImageCubeMapTexture stays a Url ref and GeneratedCubeMapTexture is not rendered.

