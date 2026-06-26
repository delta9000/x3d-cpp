---
title: Texture, Material, and Light Extraction
summary: Texture descriptor extraction, material system, light extraction, and the asset-resolver seam used by rendering consumers.
tags: [subsystem, extract, textures, materials, lights, asset-resolver]
updated: 2026-06-20
related:
  - ../architecture.md
  - extract.md
  - ext-firewall.md
---

# Texture, Material, and Light Extraction

## Purpose

This subsystem converts the scene graph's Appearance, material, and light nodes into flat, renderer-ready descriptors that a consumer can read without touching generated bindings or spec internals. It owns three distinct extraction concerns:

- **Material**: `materialOf(appearance)` in `MaterialSystem.hpp` dispatches on the material node type (Material/PhysicalMaterial/UnlitMaterial) and produces a tagged `MaterialDesc` with texture slots populated. Texture slot precedence (material-borne slots win over legacy `Appearance.texture`) is enforced here, not in the caller.
- **Texture**: `TextureExtract.hpp` enriches texture descriptors produced by `MaterialSystem` with the full §18.4.9 sampler state (`extendedSamplerOf`), §18.4.8 procedural coordinate generation descriptors (`texCoordGenOf`), baked §18.4.10 UV transforms (`applyTextureTransformToMesh`), and resolved decoded pixels from the consumer's `TextureResolver` callback.
- **Lights**: `LightSystem::collect(scene)` walks the scene graph, accumulates the world transform fresh along every path (never reads the shared `world_` table, which is first-path-only), and returns a `std::vector<LightDesc>` with all active lights world-resolved.

The subsystem is intentionally IO-free: the SDK never loads image bytes, opens files, or calls network APIs. `AssetResolver.hpp` defines the seam type (`std::function`) that a consumer plugs in to supply bytes; `TextureResolver.hpp` defines the narrower, decoded-pixel variant used by `TextureExtract`.

## Key files

| File | Role |
|---|---|
| `runtime/extract/MaterialSystem.hpp` | `materialOf()` / `texturesOf()` — Appearance → `MaterialDesc` + `TextureRef` vector; enforces material-slot precedence (D6); builds `SamplerParams` and `ExtendedSamplerParams`. |
| `runtime/extract/TextureExtract.hpp` | `textureTransformParamsOf()`, `applyTextureTransformToMesh()`, `extendedSamplerOf()`, `texCoordGenOf()`, `resolveTextureRefs()`, `enrichTextureRefs()` — enriches `TextureRef` vectors with samplers, UV gen, baked transforms, and resolved pixels. |
| `runtime/extract/TextureTransform2D.hpp` | Pure-math §18.4.10 UV transform: `TextureTransform2DParams`, `applyTextureTransform()`, `isIdentityTextureTransform()`, `makeTextureTransform3x3()`. Also hosts `BoundaryMode`, `MagFilter`, `MinFilter`, `ExtendedSamplerParams`, `TexCoordGenMode`, `TexCoordGenDesc`. No node or scene-graph includes. |
| `runtime/extract/TextureResolver.hpp` | Seam type: `TextureResolver` (`std::function<TexturePixelResult(const std::string&)>`), `TexturePixelResult` / `TexturePixels` / `TextureResolveStatus`, `makeNullTextureResolver()`. |
| `runtime/extract/TextureDesc.hpp` | Binary texture descriptor for embedder-supplied compressed/decoded textures: `TextureDesc`, `PixelFormat`, `TextureType`, `ColorSpace`, `WrapMode`, `FilterMode`, `MipLevelDesc`, `KhrTextureTransform`. |
| `runtime/extract/LightSystem.hpp` | `LightSystem::collect(scene)` — scene walk → `std::vector<LightDesc>`, world-resolved; per-path transform re-accumulation to handle USE'd lights under multiple transforms. |
| `runtime/extract/AssetResolver.hpp` | Generic IO seam: `AssetResolver` (`std::function<AssetResult(url, AssetKind)>`), `AssetResult`, `AssetStatus`, `AssetKind`. Two invocation contracts: render-time (Pending allowed) vs parse-time/SYNC (Pending is incoherent). |
| `runtime/extract/RenderItem.hpp` | Descriptor contract shared by the whole extraction layer: `TextureRef`, `MaterialDesc`, `LightDesc`, `MeshData`, `SamplerParams`. Includes `TextureDesc.hpp` and `TextureTransform2D.hpp`. |

## Interfaces and seams

### Exposed interface

**Material**

```cpp
// namespace x3d::runtime::extract
MaterialDesc materialOf(const X3DNode *appearance);
std::vector<TextureRef> texturesOf(const X3DNode *appearance);
```

`materialOf` is the primary entry point. It returns a `MaterialDesc` whose `model` field is one of `MaterialModel::{Phong, Physical, Unlit}`. `textures` on the returned descriptor carries `TextureRef` entries with slots (`TextureRef::Slot::{BaseColor, Diffuse, Emissive, Normal, Occlusion, MetallicRoughness, Specular, Shininess, Ambient}`), sources (`TextureRef::Source::{Url, Inline, Movie, Buffer}`), and fully-populated sampler state.

Null/default rules:
- `appearance == nullptr` → Unlit white (debug fallback, not spec-compliant).
- Appearance present, `material == nullptr` → Unlit white + any `Appearance.texture` in the Emissive slot (§12.2.5 rule 4; MAT-001 correction).
- `Material` → Phong. `PhysicalMaterial` → Physical. `UnlitMaterial` → Unlit.

**Texture enrichment**

```cpp
TextureTransform2DParams textureTransformParamsOf(const X3DNode *appearance);
void applyTextureTransformToMesh(MeshData &mesh, const TextureTransform2DParams &p);
ExtendedSamplerParams extendedSamplerOf(const std::shared_ptr<X3DNode> &texNode);
TexCoordGenDesc texCoordGenOf(const X3DNode *geom, bool *has);
void resolveTextureRefs(std::vector<TextureRef> &refs, const TextureResolver &resolver);
void enrichTextureRefs(std::vector<TextureRef> &refs,
                       const std::vector<std::shared_ptr<X3DNode>> &texNodes,
                       const X3DNode *geom);
```

`applyTextureTransformToMesh` bakes the §18.4.10 UV transform into `MeshData::texcoords` in place; identity transforms early-exit with no allocation, keeping untransformed mesh bytes unchanged.

`resolveTextureRefs` applies the MFString fallback order (first non-failed URL wins) and threads decoded pixels from the `TextureResolver` callback onto `TextureRef::resolvedPixels`. `Source::Inline` (PixelTexture) and `Source::Movie` refs are skipped.

**Lights**

```cpp
// class LightSystem
std::vector<LightDesc> collect(const Scene &scene);
```

Returns all active (`on == true`) lights. World direction and location are resolved into the returned `LightDesc` fields. The `global` flag and `scopeRoot` pointer are carried verbatim from the authored node — not promoted or interpreted.

**Asset resolver (bytes seam)**

```cpp
// Type alias
using AssetResolver = std::function<AssetResult(const std::string &url, AssetKind kind)>;
```

One type, two invocation contracts (documented in `AssetResolver.hpp`):
- Contract A (render-time, texture/movie): `Status::Pending` is legal; consumer retries next frame.
- Contract B (parse-time, Inline/EXTERNPROTO): must return `Ready` or `Failed` synchronously; `Pending` is incoherent.

**Texture resolver (decoded-pixel seam)**

```cpp
using TextureResolver = std::function<TexturePixelResult(const std::string& url)>;
TextureResolver makeNullTextureResolver(); // always returns makeFailed()
```

RGBA8 pixels, bottom-left origin (GL convention). The SDK never calls this resolver; the consumer calls `resolveTextureRefs` explicitly at render time.

**Binary texture descriptor**

`TextureDesc` in `TextureDesc.hpp` is an embedder-filled struct for compressed or pre-decoded textures arriving via `TextureRef::Source::Buffer`. `PixelFormat` covers uncompressed (R8 through RGBA16F), block-compressed (BC1–BC7), mobile (ETC2, ASTC), and the `BasisUniversal = 255` supercompressed sentinel. The SDK does not fill or decode `TextureDesc`; the consumer fills `TextureRef::resolved_desc` once it has decoded `TextureRef::bufferBytes`.

### Seam points

- **`TextureResolver` seam** — the consumer (or an adapter around stb_image/libpng) supplies a `TextureResolver` and calls `resolveTextureRefs` after `materialOf`; the SDK provides the stub `makeNullTextureResolver()` (always-failed, white-fallback rendering).
- **`AssetResolver` seam** — one callback for both render-time URL bytes and parse-time Inline/EXTERNPROTO bytes. `AssetKind` lets one resolver branch by intent and pick the appropriate async/sync policy.
- **`geombounds` reflection helpers** — all field reads in `MaterialSystem`, `TextureExtract`, and `LightSystem` go through `geombounds::getField` / `geombounds::getNode` / `geombounds::hasField` so no generated node type is directly named; the subsystem stays valid across regeneration.
- **`SceneExtractor` integration** — `SceneExtractor.hpp` includes `MaterialSystem.hpp` and `LightSystem.hpp`. It calls `materialOf(appearance)` for each Shape's Appearance, calls `LightSystem::collect(scene)` once per snapshot, and calls `TextureExtract` helpers (via `textureTransformParamsOf` + `applyTextureTransformToMesh` + `enrichTextureRefs`) on extracted mesh data. `SceneExtractor` is the only caller in the production path; the free functions are also exercised directly by tests.

## How it is tested

- `ctest --preset dev -R x3d_extract_tests` (doctest case: `material_system_test`) — unit tests for `materialOf()` and `texturesOf()`: all three material model branches, null-appearance and null-material cases, PixelTexture inline, MultiTexture channel expansion, and material-slot precedence over legacy `Appearance.texture` (D6). Source: `runtime/extract/tests/material_system_test.cpp`.
- `ctest --preset dev -R x3d_extract_tests` (doctest case: `texture_extract_test`) — T-TEX v1 end-to-end coverage: authored `TextureCoordinate` wins over §13 default projection; `TextureTransform` center/rotation/scale/translation bake (§18.4.10); `extendedSamplerOf` with and without `TextureProperties` (§18.4.9); `TextureCoordinateGenerator` mode/parameter descriptor (§18.4.8); `resolveTextureRefs` MFString fallback order + Inline skip. Source: `runtime/extract/tests/texture_extract_test.cpp`.
- `ctest --preset dev -R x3d_extract_tests` (doctest case: `light_system_test`) — `LightSystem::collect` unit tests: DirectionalLight/PointLight/SpotLight field reads; world-frame direction and location; `global` flag default differences per light type; `on==false` skip; `scopeRoot` assignment. Source: `runtime/extract/tests/light_system_test.cpp`.
- `ctest --preset dev -R x3d_extract_tests` (doctest case: `texture_desc_test`) — `TextureDesc` struct, `PixelFormat` compressed/uncompressed predicate, `MipLevelDesc` accessors. Source: `runtime/extract/tests/texture_desc_test.cpp`.
- `ctest --preset dev -R x3d_extract_oracle_test` — STL round-trip oracle using `SceneExtractor` (exercises the `materialOf` integration path indirectly via extraction). Requires `X3D_CPP_BUILD_EXT`. Source: `tools/x3d-cli/extract_oracle_test.cpp`.

## Related specs and ADRs

- [ADR-0015: Extraction Pull Per Path](../decisions/0015-extraction-pull-per-path.md)
- Spec (v1-closure extraction design): `docs/superpowers/specs/2026-06-14-m25-extraction-poc-renderer-design.md`
- Spec (T-TEX closure): `docs/superpowers/specs/2026-06-16-v1-closure-design.md`
- Spec (binary mesh/texture abstractions): `docs/superpowers/specs/2026-06-18-binary-mesh-texture-abstractions.md`
- Conformance findings MAT-001 through MAT-005 (all closed): `docs/conformance/findings.yaml`
- Conformance finding TXF-1 (deferred — spec-ambiguous UV transform order): `docs/conformance/findings.yaml`
- Conformance finding MAT-006 (closed — `Appearance.backMaterial` two-sided extraction now reads via the same three-way dispatch as the front material): `docs/conformance/findings.yaml`
