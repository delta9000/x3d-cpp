---
title: Texture, Material, and Light Extraction
summary: Texture descriptor extraction, material system, light extraction, and the asset-resolver seam used by rendering consumers.
tags: [subsystem, extract, textures, materials, lights, asset-resolver]
updated: 2026-11-05
related:
  - ../architecture.md
  - extract.md
  - ext-firewall.md
---

# Texture, Material, and Light Extraction

## Purpose

This subsystem converts the scene graph's Appearance, material, and light nodes into flat, renderer-ready descriptors that a consumer can read without touching generated bindings or spec internals. It owns three distinct extraction concerns:

- Material: `materialOf(appearance)` in `MaterialSystem.hpp` dispatches on the material node type (Material/PhysicalMaterial/UnlitMaterial) and produces a tagged `MaterialDesc` with texture slots populated. Texture slot precedence (material-borne slots win over legacy `Appearance.texture`) is enforced here, not in the caller.
- Texture: `MaterialSystem` fills sampler state with `extendedSamplerOf()`.
  `TextureExtract.hpp` adds §18.4.8 procedural coordinate generation descriptors
  (`texCoordGenOf()`), bakes §18.4.10 UV transforms across coordinate sets
  (`applyTextureTransformsToMesh()`), and resolves decoded pixels through the
  consumer's `TextureResolver` callback.
- Lights: `LightSystem::collect(scene)` walks the scene graph, accumulates the world transform fresh along every path (never reads the shared `world_` table, which is first-path-only), and returns a `std::vector<LightDesc>` with all active lights world-resolved. `LightSystem::collectProjectors` runs the same walk for TextureProjector/TextureProjectorParallel and returns `ProjectorDesc` values (`SceneExtractor::projectors()` resolves their textures).

The subsystem is intentionally IO-free: it never loads image bytes, opens files, or calls network APIs. (The SDK as a whole does open local files on the parse path — `parseFile` — but nothing in texture extraction does.) `AssetResolver.hpp` defines the seam type (`std::function`) that a consumer plugs in to supply bytes; `TextureResolver.hpp` defines the narrower, decoded-pixel variant used by `TextureExtract`.

## Key files

| File | Role |
|---|---|
| `runtime/extract/MaterialSystem.hpp` | `materialOf()` / `texturesOf()` — Appearance → `MaterialDesc` + `TextureRef` vector; enforces material-slot precedence (D6); builds `SamplerParams` and `ExtendedSamplerParams`. |
| `runtime/extract/TextureExtract.hpp` | `textureTransformParamsListOf()`, `applyTextureTransformsToMesh()`, `extendedSamplerOf()`, `texCoordGenOf()`, `resolveTextureRefs()`, `enrichTextureRefs()` — enriches `TextureRef` vectors with samplers, UV gen, baked transforms, and resolved pixels. |
| `runtime/extract/TextureTransform2D.hpp` | Pure-math §18.4.10 UV transform: `TextureTransform2DParams`, `applyTextureTransform()`, `isIdentityTextureTransform()`, `makeTextureTransform3x3()`. Also hosts `BoundaryMode`, `MagFilter`, `MinFilter`, `ExtendedSamplerParams`, `TexCoordGenMode`, `TexCoordGenDesc`. No node or scene-graph includes. |
| `runtime/extract/TextureResolver.hpp` | Seam type: `TextureResolver` (`std::function<TexturePixelResult(const std::string&)>`), `TexturePixelResult` / `TexturePixels` / `TextureResolveStatus`, `makeNullTextureResolver()`. |
| `runtime/extract/LightSystem.hpp` | `LightSystem::collect(scene)` — scene walk → `std::vector<LightDesc>`, world-resolved; per-path transform re-accumulation to handle USE'd lights under multiple transforms. |
| `runtime/extract/AssetResolver.hpp` | Generic IO seam: `AssetResolver` (`std::function<AssetResult(url, AssetKind)>`), `AssetResult`, `AssetStatus`, `AssetKind`. Two invocation contracts: render-time (Pending allowed) vs parse-time/SYNC (Pending is incoherent). |
| `runtime/events/GeneratedCubeMapSystem.hpp` | §34.4.2 runtime half of GeneratedCubeMapTexture: resets `update` NEXT_FRAME_ONLY to NONE at the start of the tick after the frame that drew it, as an event ([ADR-0060](../decisions/0060-generated-cube-maps-local-frame.md)). Attached by `attachStandardRuntime`. |
| `runtime/events/TextureProjectorSystem.hpp` | §42 `aspectRatio` output: emits width/height for a PixelTexture projector texture on load and when the texture or its image changes ([ADR-0061](../decisions/0061-texture-projectors-as-lights.md)). Attached by `attachStandardRuntime`. |
| `runtime/extract/RenderItem.hpp` | Descriptor contract shared by the whole extraction layer: `TextureRef`, `MaterialDesc`, `LightDesc`, `MeshData`, `SamplerParams`. Includes `TextureTransform2D.hpp`. |

## Interfaces and seams

### Exposed interface

Material

```cpp
// namespace x3d::runtime::extract
MaterialDesc materialOf(const X3DNode *appearance);
std::vector<TextureRef> texturesOf(const X3DNode *appearance);
```

`materialOf()` is the primary entry point. It returns a `MaterialDesc` whose `model` field is one of `MaterialModel::{Phong, Physical, Unlit}`. `textures` on the returned descriptor carries `TextureRef` entries with slots (`TextureRef::Slot::{BaseColor, Diffuse, Emissive, Normal, Occlusion, MetallicRoughness, Specular, Shininess, Ambient}`), sources (`TextureRef::Source::{Url, Inline, Movie, Multi, Cube, Buffer, Tex3D}`), and fully-populated sampler state. `Multi` refs retain ordered stages and their §18.4.3 mode, source, function, color, and alpha controls; material slots still expose stages as separate refs. A panorama face can carry the `Multi` ref intact. `load=FALSE` on a URL texture yields no URL candidates for a resolver.

3D textures (§33, `X3DTexture3DNode` family; T3D-1) surface as `Source::Tex3D` (appended last in the enum so no existing value shifts). `PixelTexture3D` carries an inline `Texture3DDesc{width, height, depth, numComponents, texels}` decoded from its `image` MFInt32 by `texextract::pixelTexture3DDesc` (MSB-first component unpack, `numComponents` clamped to [0,4], pixel loop bounded by the token stream — the same ownership/immutability rules as the 2D `SFImage` for `PixelTexture`). `ComposedTexture3D` carries `tex3dSlices`, one 2D `TextureRef` per `texture` child (reusing the 2D `refOf` path), with `tex3d.depth` = slice count; its width/height resolve only once the consumer decodes those slices. `repeatR` is surfaced on `TextureRef::repeatR` and `ExtendedSamplerParams::{repeatR, boundaryModeR}`. `ImageTexture3D` is not handled — the SDK ships no 3D image decoder (NRRD/DDS/etc.), so it yields neither a descriptor nor a url and falls back to white. Neither bundled renderer samples 3D textures: `cpu_raster` `Texture::fromRef` and `poc_renderer` `resolveTexRef` return the flat material color for `Source::Tex3D` (no `sampler3D` path yet).

Null/default rules:
- `appearance == nullptr` → Unlit white (debug fallback, not spec-compliant).
- Appearance present, `material == nullptr` → Unlit white + any `Appearance.texture` in the Emissive slot (§12.2.5 rule 4;  correction).
- `Material` → Phong. `PhysicalMaterial` → Physical. `UnlitMaterial` → Unlit.

Texture enrichment

```cpp
std::vector<TextureTransform2DParams>
textureTransformParamsListOf(const X3DNode *appearance);
std::vector<TextureTransform2DParams>
textureTransformsForGeometry(const X3DNode *appearance, const X3DNode *geom);
void applyTextureTransformsToMesh(
    MeshData &mesh, const std::vector<TextureTransform2DParams> &params);
ExtendedSamplerParams extendedSamplerOf(const std::shared_ptr<X3DNode> &texNode);
TexCoordGenDesc texCoordGenOf(const X3DNode *geom, bool *has, int channel = 0);
void resolveTextureRefs(std::vector<TextureRef> &refs, const TextureResolver &resolver);
void enrichTextureRefs(std::vector<TextureRef> &refs,
                       const X3DNode *geom,
                       const X3DNode *appearance = nullptr);
```

`applyTextureTransformsToMesh()` bakes §18.4.10 UV transforms into
`MeshData::texcoordSets` and keeps `texcoords` as the set-zero mirror. A single
transform applies to every set; a list selects transforms by set index. Empty
lists leave the mesh unchanged. Identity transforms preserve UV values. The
application order follows the normative matrix `Tc' = C·S·R·C⁻¹·T·Tc`: translate,
pivot to the center, rotate, scale, then un-pivot. Rotation precedes scaling,
following §18.4.10's application list; the contradicting prose sentence is
treated as editorial. `makeTextureTransform3x3()` produces the equivalent 3×3
matrix for shader upload.

`resolveTextureRefs()` applies the MFString fallback order (first non-failed URL wins) and threads decoded pixels from the `TextureResolver` callback onto `TextureRef::resolvedPixels`. It recurses into `Source::Multi` stages. `Source::Inline` (PixelTexture) and `Source::Movie` refs are skipped.

`texCoordGenOf()` selects a `TextureCoordinateGenerator` from the requested geometry channel. `enrichTextureRefs()` resolves named `texCoordMapping` labels against `MultiTextureCoordinate` children and carries the generator and its matching transform on each `TextureRef`. Authored mesh UVs are transformed once during extraction; generated UVs receive `generatedTransform` after generation in the consumer. A single TextureTransform applies to all channels; named MultiTextureTransform mappings align with the corresponding coordinate mappings. Live transform and nested coordinate edits are registered in the geometry/material dependency indices: baked UVs and generated-coordinate descriptors update through RenderDelta. RuntimeSession regressions exercise both event paths.

The CPU reference host implements all eleven Table 18.6 modes for unlit, Phong and PBR: SPHERE, CAMERASPACENORMAL, CAMERASPACEPOSITION, CAMERASPACEREFLECTIONVECTOR, SPHERE-LOCAL, COORD, COORD-EYE, NOISE, NOISE-EYE, SPHERE-REFLECT and SPHERE-REFLECT-LOCAL. Local modes use interpolated pre-transform positions/normals. Noise uses deterministic three-dimensional Perlin noise with authored scale and translation; refraction uses the authored index ratio and eye position, with zero on total internal reflection. Those are explicit implementation choices for the underspecified noise/refraction prose. The OpenGL PoC evaluates all eleven modes per fragment (`examples/poc_renderer/shaders/texgen.glsl`, the same noise and refraction choices) and applies the generated-coordinate transform; `texgen_gl_test.py` covers the camera-space modes and the issue #140 reflection vector, `texgen_local_gl_test.py` the local, noise and refraction modes ( fixed). Like the CPU, it gives each texture slot and MultiTexture stage its own generator or UV set.

Cube-map environment textures (§34,). A `Source::Cube` ref (ComposedCubeMapTexture) in the base-colour slot or a MultiTexture stage is sampled by direction in both hosts: the generator's full (s, t, r) vector, or with no generator the camera-space reflection vector, with eye-space directions carried into the geometry's local frame so the cube turns with the geometry ([ADR-0060](../decisions/0060-generated-cube-maps-local-frame.md)). The faces follow Figure 34.1 and the Background panorama convention (cpu_raster `cubeFaceUv`; GL `sampleCubeFaces` over a six-layer texture array). An ImageCubeMapTexture is a `Source::Cube` ref with a `url` and no `cubeFaces`; `resolveTextureRefs()` resolves it as one image, which the `TextureResolver` decodes to six layers (`TexturePixels::layers`, [ADR-0059](../decisions/0059-cube-map-images-through-texture-decode.md); DDS through the bundled decode backends, see [Texture Decode Seam](system-texture-decode.md)). A GeneratedCubeMapTexture is a `Source::Cube` ref carrying `generatedCube` (node, `update`, `size`): the consumer renders the six faces from the local origin of the first Shape using it, along its local axes, without the Shapes that use it, and keeps them while `update` is NONE. `GeneratedCubeMapSystem` resets NEXT_FRAME_ONLY to NONE at the start of the following tick (cpu_raster `GeneratedCubeCache`; the GL PoC keeps a six-layer render target per node).  lists the other limits.

Texture projectors (§42,). `SceneExtractor::projectors()` returns one world-resolved `ProjectorDesc` per active projector: colour, intensity, ambientIntensity, location, direction, up vector, view and projection matrices, near/far and parallel extents scaled by the local frame, `aspectRatio` from the resolved image, the texture as a resolved `TextureRef`, and `global`/`scopeRoot` (`projectorApplies(desc, item)`). `ProjectorDesc::project(world, s, t)` returns the texture coordinates of a point inside the volume. Both hosts light the scene through a projector as an unattenuated light (positional for TextureProjector, directional for TextureProjectorParallel) filtered by the projected texel's rgb x alpha and zero outside the volume ([ADR-0061](../decisions/0061-texture-projectors-as-lights.md)): cpu_raster through `EyeLight::projected`, the GL PoC through `shaders/projector.glsl` (up to three projectors on units 13-15). See the findings register for remaining projector gaps.

`extendedSamplerOf()` resolves the full §18.4.9 sampler state onto `TextureRef::extSampler`: boundary modes S/T/R (`REPEAT`, `CLAMP`, `CLAMP_TO_EDGE`, `CLAMP_TO_BOUNDARY`, `MIRRORED_REPEAT`), magnification/minification filters, `generateMipMaps`, anisotropy, and `borderColor`. When a `TextureProperties` node is present it governs and the texture node's `repeatS`/`repeatT` are ignored (§18.4.9); otherwise the boundary modes derive from `repeatS`/`repeatT` (`Repeat` / `ClampToEdge` per §18.2.3). The CPU reference sampler (`examples/cpu_raster/cpuraster/Texture.hpp`) consumes this state and implements REPEAT, CLAMP, CLAMP_TO_EDGE, CLAMP_TO_BOUNDARY (using the extracted border color, default `0,0,0,0`) and MIRRORED_REPEAT, plus magnification nearest-vs-bilinear. Each mode clamps to its own §18.4.9 Table 18.7 range: CLAMP to `[0,1]`, CLAMP_TO_EDGE to `[1/(2N), 1-1/(2N)]`, CLAMP_TO_BOUNDARY to `[-1/(2N), 1+1/(2N)]`, and MIRRORED_REPEAT is mirrored then clamped as CLAMP_TO_EDGE. Under bilinear filtering CLAMP and CLAMP_TO_BOUNDARY let the outermost taps read the border color, so CLAMP differs from CLAMP_TO_EDGE near a texture edge. When `generateMipMaps` is true, the CPU builds box-filtered mip levels and consumes the minification filter using per-channel fragment derivatives. Nearest/bilinear and nearest/trilinear mip selection are supported. Without generated levels, mip filters fall back to their corresponding base-level filter. Anisotropy is clamped to the supported maximum of 16.

MultiTexture stage combining (§18.4.3,). `MaterialSystem::appendSlot` retains stage controls and channel identity. The CPU combines every Table 18.3 mode, including BLEND*, DOTPRODUCT3 and MODULATEALPHA/MODULATEINV operations, with separate RGB/alpha mode tokens. `source` replaces Arg2 with DIFFUSE, SPECULAR or FACTOR, and `function` transforms the operation result. Each stage and each ordinary material texture slot samples its own mapped/generated coordinates and derivatives. Unlit Appearance textures are consumed through the Emissive slot. The OpenGL PoC runs the same combiner in GLSL (`examples/poc_renderer/shaders/multitexture.glsl`) for up to eight stages over UV sets 0-7, with a coordinate mapping (UV set or generator) per stage and per material slot in `shaders/texgen.glsl`. It drops to four stages when the context has fewer than 20 fragment texture image units, and binds one cube map per material. `multitexture_gl_test.py` covers it.

The published table is inconsistent for REPLACE and SELECTARG2. The CPU takes REPLACE as the stage texture (Arg1) and SELECTARG2 as the previous result (Arg2), following their described operation meanings. The reference-host tests use hand-computed expected values for these choices.

Lights

```cpp
// class LightSystem
std::vector<LightDesc> collect(const Scene &scene);
```

Returns all active (`on == true`) lights. The CPU consumes eight authored lights in addition to the optional headlight, and applies per-light triangle occlusion when `shadows` is true; `castShadow`, visibility and `shadowIntensity` affect that query. World direction and location are resolved into the returned `LightDesc` fields. The `global` flag and `scopeRoot` pointer are carried verbatim from the authored node — not promoted or interpreted.

Asset resolver (bytes seam)

```cpp
// Type alias
using AssetResolver = std::function<AssetResult(const std::string &url, AssetKind kind)>;
```

One type, two invocation contracts (documented in `AssetResolver.hpp`):
- Contract A (render-time, texture/movie): `Status::Pending` is legal; consumer retries next frame.
- Contract B (parse-time, Inline/EXTERNPROTO): must return `Ready` or `Failed` synchronously; `Pending` is incoherent.

Texture resolver (decoded-pixel seam)

```cpp
using TextureResolver = std::function<TexturePixelResult(const std::string& url)>;
TextureResolver makeNullTextureResolver(); // always returns makeFailed()
```

RGBA8 pixels, bottom-left origin (GL convention). The SDK never calls this resolver; the consumer calls `resolveTextureRefs()` explicitly at render time.

`TextureRef::Source::Buffer` remains reserved for existing consumer switches.
It has no payload fields or extraction path. All source enumerator values stay
unchanged. Decoded image data crosses the `TextureResolver` seam as
`TexturePixelResult`.

### Movie frame decoding

`MovieDecoder` is a consumer-owned callback. The SDK surfaces a MovieTexture
URL and lifecycle state; the host supplies the requested media time and uploads
the returned frame. `x3d_plmpeg` provides MPEG-1 decoding when
`X3D_CPP_BUILD_PLMPEG` is enabled, and the PoC renderer uses this path. The host selects a decoder for its media format.

### Seam points

- `TextureResolver` seam — the consumer (or an adapter around stb_image/libpng) supplies a `TextureResolver` and calls `resolveTextureRefs()` after `materialOf()`; the SDK provides the stub `makeNullTextureResolver()` (always-failed, white-fallback rendering).
- `AssetResolver` seam — one callback for both render-time URL bytes and parse-time Inline/EXTERNPROTO bytes. `AssetKind` lets one resolver branch by intent and pick the appropriate async/sync policy.
- `geombounds` reflection helpers — all field reads in `MaterialSystem`, `TextureExtract`, and `LightSystem` go through `geombounds::getField` / `geombounds::getNode` / `geombounds::hasField` so no generated node type is directly named; the subsystem stays valid across regeneration.
- `SceneExtractor` integration — `SceneExtractor.hpp` includes `MaterialSystem.hpp` and `LightSystem.hpp`. It calls `materialOf(appearance)` for each Shape's Appearance, calls `LightSystem::collect(scene)` on full snapshots and deltas with active or changed lights, and calls `TextureExtract` helpers (via `textureTransformsForGeometry` + `applyTextureTransformsToMesh()` + `enrichTextureRefs()`) on extracted mesh data. `SceneExtractor` is the only caller in the production path; the free functions are also exercised directly by tests.

## Related

- [ADR-0015: Extraction Pull Per Path](../decisions/0015-extraction-pull-per-path.md)
