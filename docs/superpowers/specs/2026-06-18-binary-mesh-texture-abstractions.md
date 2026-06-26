# Binary Mesh + Texture Abstractions — Unlocking glTF, Binary STL/OBJ/PLY, and Modern GPU Paths

**Date:** 2026-06-18
**Status:** Design proposal. Adopts the pivot identified during the X_ITE comparison: the load-bearing abstractions are binary mesh + texture, not a per-format parser layer.

## Why This Pivot

The prior design (`2026-06-16-v1-closure-roadmap-design.md`) lists OBJ/STL/PLY/glTF codecs as parallel v1.5/v2 candidates. This is the wrong frame. **The codecs are leaf consumers; the abstractions they need are the actual work.** glTF in particular is not "another codec" — it requires:

1. **Binary vertex attribute streams** with typed `Accessor`s (vec3 FLOAT, vec2 UNSIGNED_SHORT, etc.) — currently `MeshData` only stores `vector<SFVec3f>`, forcing repack from text-typed X3D values.
2. **Binary texture data** with format metadata (PNG, JPEG, **Basis Universal / KTX2**, etc.) — currently `TextureRef::resolvedPixels` is RGBA8 SFImage only, no mipmaps, no cube/3D, no compressed formats.
3. **A loader pipeline that doesn't go through the live scene graph** — glTF's binary buffers should populate vertex data directly, not be serialized into X3D `Coordinate` nodes first.

The same abstractions unlock: binary STL, binary PLY, OBJ with binary MTL, `.x3db` (X3D binary), direct GPU uploads for Vulkan/Metal/WebGPU renderers, Basis Universal textures for memory-bound mobile/web embedders.

This doc specifies the **PackedMesh** and **TextureDesc** abstractions as first-class siblings of `MeshData` and `TextureRef`, the migration path, and the codecs they enable.

## Current State (the gap)

`runtime/extract/RenderItem.hpp:151-182` (`MeshData`):
```cpp
struct MeshData {
  std::vector<SFVec3f> positions;
  std::vector<std::uint32_t> indices;
  std::vector<SFVec3f> normals;
  std::vector<SFVec2f> texcoords;
  std::vector<SFColorRGBA> colors;
  // ... AoS only. No typed accessors, no component type, no byte stride.
};
```

`runtime/extract/RenderItem.hpp:196-254` (`TextureRef`):
```cpp
struct TextureRef {
  // ...
  SFImage inlinePixels;   // RGBA8 ONLY. {int w, int h, int ch, vector<byte> data}
  TexturePixelResult resolvedPixels; // RGBA8 ONLY. Consumer-side decode result.
  // ... no format enum, no mip chain, no cube/3D, no compressed formats.
};
```

`runtime/codecs/` — contains X3D-native codecs only (`XmlReader`, `JsonWriter`, `VrmlWriter`, `XmlLite`). No binary mesh codec. No image decoder.

`runtime/script/vendor/` — Duktape only. No image decoder.

`examples/poc_renderer/third_party/` — `stb_image.h` vendored **only for the PoC**, not in the SDK.

**The PoC renderer uploads `MeshData` to OpenGL by mapping each `SFVec3f` to a `glVertexAttribPointer`. This works for one renderer; it doesn't work for any GPU API that wants typed binary layouts.**

## The Abstractions

### `PackedMesh` — binary, typed, glTF-compatible

Located at `runtime/extract/PackedMesh.hpp`. Header-only, no X3D dependency, leaf of the dep graph alongside `RenderItem.hpp`.

```cpp
namespace x3d::runtime::extract {

// Mirrors glTF componentType (TINYGLTF/ComponentType).
enum class ComponentType : std::uint16_t {
  Byte           = 5120,
  UByte          = 5121,
  Short          = 5122,
  UShort         = 5123,
  HalfFloat      = 5125, // 16-bit float (IEEE 754 binary16)
  Float          = 5126,
  UnsignedInt    = 5125, // not glTF; reserved for index buffers > 65535
};

// One vertex attribute stream. data is owned by PackedMesh; non-owning view.
struct VertexBufferView {
  const void*   data = nullptr;       // contiguous bytes; no copy
  std::size_t   byte_length = 0;
  std::uint32_t component_type = 5126; // ComponentType as int (matches glTF)
  std::uint8_t  components_per_vertex = 0;  // 1, 2, 3, 4
  bool          normalized = false;        // for integer types (Byte/UByte → float)
  std::uint32_t vertex_count = 0;          // = byte_length / (stride of one vertex)
};

// Which attributes are present (bit-set, mirrors glTF semantics).
enum class VertexAttribute : std::uint8_t {
  Position    = 1 << 0,
  Normal      = 1 << 1,
  Tangent     = 1 << 2,
  TexCoord0   = 1 << 3,
  TexCoord1   = 1 << 4,
  Color       = 1 << 5,
  Joints      = 1 << 6,   // bone indices  (uvec4) — skinned meshes
  Weights     = 1 << 7,   // bone weights (vec4)  — skinned meshes
};
using VertexAttributeMask = std::uint32_t;

struct PackedMesh {
  // Per-attribute separate buffers (NOT interleaved). Mirrors glTF's
  // multi-BufferView model; consumers can re-pack to interleaved if desired.
  // Indexed by attribute bit position; absent attributes are null/zero.
  std::array<VertexBufferView, 8> attribute_views{};

  // Index buffer. Always 32-bit; glTF allows 16-bit but we widen at load.
  // Empty for non-indexed meshes.
  VertexBufferView index_view{};

  // Metadata.
  VertexAttributeMask attributes = 0;     // which attribute_views are valid
  std::uint32_t vertex_count = 0;
  std::uint32_t index_count = 0;          // 0 when non-indexed
  Topology topology = Topology::Triangles;

  // Per-mesh bounds (LOCAL frame, never world-baked — matches MeshData contract).
  Aabb bounds{};

  bool ccw = true;
  bool solid = true;

  // Optional: skinning data for skinned meshes (glTF skinned primitives).
  // When non-null, the consumer applies per-vertex joint transforms before
  // rasterization. Absent for static meshes; no impact on existing paths.
  const void* inverse_bind_matrices = nullptr;
  std::size_t inverse_bind_matrices_bytes = 0;
  std::uint32_t joint_count = 0;

  // Bounding/quantization metadata (descriptor-only; not exercised by PoC).
  // Useful for normal/tangent quantization (octahedral, 8-bit normal, etc.)
  // and for low-precision mobile pipelines. Mirrors glTF KHR_mesh_quantization.
  bool quantized = false;

  // Convenience predicates.
  bool has(VertexAttribute a) const { return attributes & static_cast<std::uint32_t>(a); }
  bool is_indexed() const { return index_count > 0; }
  bool is_skinning() const { return has(VertexAttribute::Joints) && has(VertexAttribute::Weights); }
};

} // namespace x3d::runtime::extract
```

**Why this shape**:
- Mirrors glTF's `Accessor`+`BufferView` model directly, so a glTF parser fills it with zero conversion.
- Separate (non-interleaved) attribute buffers — matches the GPU layout Vulkan/Metal/WebGPU want (each binding = one buffer); consumers can interleave if they prefer.
- Typed `ComponentType` (Byte/UByte/Short/UShort/HalfFloat/Float) — glTF-compatible and lets binary formats keep their native precision (no precision loss from text-to-float32).
- `joints`/`weights` + `inverse_bind_matrices` — covers glTF-skinned meshes in one struct.
- `quantized` flag — covers glTF KHR_mesh_quantization without breaking the typed view.

### `TextureDesc` — typed pixel format + GPU layout

Located at `runtime/extract/TextureDesc.hpp`. Header-only.

```cpp
namespace x3d::runtime::extract {

enum class PixelFormat : std::uint32_t {
  // Uncompressed
  R8              = 1,
  RG8             = 2,
  RGB8            = 3,
  RGBA8           = 4,
  R16F            = 5,
  RG16F           = 6,
  RGB16F          = 7,
  RGBA16F         = 8,
  R32F            = 9,
  RG32F           = 10,
  RGB32F          = 11,
  RGBA32F         = 12,
  // BCn (desktop compressed)
  BC1_RGB         = 20,   // also BC1A RGBA
  BC3_RGBA        = 21,
  BC4_R           = 22,
  BC5_RG          = 23,
  BC6H_RGB        = 24,
  BC7_RGBA        = 25,
  // ETC2 (mobile compressed)
  ETC2_RGB8       = 30,
  ETC2_RGBA8      = 31,
  // ASTC (mobile compressed, mobile + Metal)
  ASTC_4x4        = 40,
  ASTC_8x8        = 41,
  // Reserved for KTX2/Basis Universal (transcoded at load).
  BasisUniversal  = 100,
};

enum class TextureType : std::uint8_t {
  Tex2D,
  TexCube,
  Tex2DArray,
  Tex3D,
};

enum class ColorSpace : std::uint8_t {
  Linear,
  sRGB,
};

struct MipLevelDesc {
  std::uint32_t width = 0;       // 1 for Tex3D depth
  std::uint32_t height = 0;
  std::uint32_t depth = 1;       // Tex3D only
  std::uint32_t layer_count = 1; // TexCube=6, Tex2DArray=N, Tex2D=1
  std::size_t   byte_offset = 0; // into TextureDesc::data
  std::size_t   byte_length = 0;
};

struct TextureDesc {
  TextureType   type = TextureType::Tex2D;
  PixelFormat   format = PixelFormat::RGBA8;
  ColorSpace    color_space = ColorSpace::sRGB;

  std::uint32_t width = 0;
  std::uint32_t height = 0;
  std::uint32_t depth = 1;       // Tex3D only
  std::uint32_t array_layers = 1; // TexCube=6, Tex2DArray=N

  std::vector<MipLevelDesc> mips; // empty => no mip chain
  std::vector<std::uint8_t> data;  // all mips × all layers, tightly packed

  // Sampler (separate from data; matches TextureProperties in X3D).
  enum class Wrap { Repeat, ClampToEdge, MirroredRepeat, ClampToBorder };
  enum class Filter { Nearest, Linear, NearestMipmapNearest, LinearMipmapNearest,
                      NearestMipmapLinear, LinearMipmapLinear };
  Wrap   wrap_s = Wrap::Repeat;
  Wrap   wrap_t = Wrap::Repeat;
  Wrap   wrap_r = Wrap::Repeat;
  Filter min_filter = Filter::LinearMipmapLinear;
  Filter mag_filter = Filter::Linear;
  float  max_anisotropy = 1.0f;

  // KHR_texture_transform (descriptor-only; not exercised by PoC).
  // Offsets/rotation/scale that compose into the texture matrix in shaders.
  bool   has_transform = false;
  float  transform_offset[2] = {0, 0};
  float  transform_rotation = 0;
  float  transform_scale[2] = {1, 1};

  // Convenience predicates.
  bool is_compressed() const;
  bool is_cube() const { return type == TextureType::TexCube; }
  bool is_array() const { return type == TextureType::Tex2DArray; }
  bool is_3d() const { return type == TextureType::Tex3D; }
  std::size_t total_bytes() const { return data.size(); }
};

} // namespace x3d::runtime::extract
```

**Why this shape**:
- `PixelFormat` enum + `TextureType` + `ColorSpace` — covers all formats glTF supports plus the compressed families a modern renderer wants.
- `MipLevelDesc` per-mip — glTF image data can be multi-mip directly, and Basis Universal transcode output is a mip chain.
- `data` tightly packed, `byte_offset` per mip — matches KTX2 layout, Vulkan `VkBufferImageCopy`, D3D12 `D3D12_SUBRESOURCE_FOOTPRINT`, Metal `MTLTexture.replace(region:layer:mip:)`.
- Sampler state bundled — matches `TextureProperties` semantics in X3D.
- `KHR_texture_transform` field — covers glTF's per-texture UV transform without losing alignment with X3D's `TextureTransform`.

### The union in `RenderItem`

Extend `RenderItem` (or `Geometry` in `RenderItem`) so a render item carries either AoS or Packed geometry:

```cpp
struct Geometry {
  enum class Kind { AoS, Packed, Null };
  Kind kind = Kind::Null;
  // Exactly one of these is populated per item.
  MeshData    aos;    // when kind == AoS (existing path)
  PackedMesh  packed; // when kind == Packed (new path)

  bool is_null() const { return kind == Kind::Null; }
  bool is_aos()  const { return kind == Kind::AoS;  }
  bool is_packed() const { return kind == Kind::Packed; }
};
```

Same for textures: a `TextureRef` gets an optional `TextureDesc resolved_desc` field — when populated by a binary codec, the renderer uses it directly; when null, fall back to `resolvedPixels` (RGBA8 path).

The renderer pattern-matches: `if (geom.is_packed()) upload_as_typed_layout(...); else upload_aos(...)`.

## Migration: Two Paths Coexist

**Path A (existing): runtime → MeshData / TextureRef**
- `MeshBuilder` walks the live scene graph, produces `MeshData` (AoS).
- `TextureExtract` produces `TextureRef` with `resolvedPixels` (RGBA8 SFImage) via `TextureResolver` callback.
- Renderer uploads to GPU. This is the existing path; nothing changes.

**Path B (new): binary codec → PackedMesh / TextureDesc**
- A binary parser (STL/OBJ/PLY/glTF) reads bytes, produces `PackedMesh` directly.
- An image decoder (PNG/JPEG/Basis/KTX2) reads pixels, produces `TextureDesc` directly.
- Renderer uploads to GPU. This is the new path.

**Both paths flow into the same `RenderItem` (with `Geometry::kind` discriminating) and the same `TextureRef` (with `resolved_desc` optionally populated).** A renderer that handles both paths has one upload code path per geometry kind; a renderer that only handles one path can ignore the other (`Geometry::is_packed()` check).

This is a **strict additive upgrade**. No existing code changes; new codecs opt into the new path; existing text-X3D parsing continues to work.

## Codecs Enabled (in priority order)

### v1.5 — geometry-only, easy
- **Binary STL**: ~80 LOC parser. Read header (80 bytes), u32 triangle count, then (12 floats) × N. Fill `PackedMesh` with `Position` attribute (vec3 FLOAT) and `index_view` (0..N-1 trivially). No textures.
- **Binary PLY** (little-endian): ~150 LOC parser. Read header, find `vertex` element + `face` element, identify format (`float`, `uchar`, `ushort`). Fill `PackedMesh` with whatever attributes are present. No textures.
- **OBJ** (text): ~300 LOC parser. Wavefront OBJ is text but the geometry can flow into `PackedMesh` as FLOAT positions + per-face indices. Materials deferred to a separate MTL loader.

### v2 — image decoders
- **PNG**: stb_image (already vendored in PoC). Promote to SDK. Decode to `TextureDesc` (RGBA8, sRGB).
- **JPEG**: stb_image. Same.
- **WebP**: libwebp (or stb_image for read-only). Same.
- **KTX2 / Basis Universal**: optional behind CMake flag. Decode to `TextureDesc` with BCn/ETC2/ASTC format.

### v2 — glTF (the big one)
- glTF JSON header: read scene/mesh/node/material/texture/image/buffer/accessor/bufferView descriptors.
- glTF binary `.glb` container: parse JSON chunk + BIN chunk.
- glTF binary buffers: read into memory, view via `BufferView` offset/length, fill `PackedMesh` attribute buffers directly via `VertexBufferView`.
- glTF images: route PNG/JPEG/KTX2 through the image decoders above → `TextureDesc`.
- glTF materials: map to `MaterialDesc` (Physical model — `baseColorTexture`, `metallicRoughnessTexture`, `normalTexture`, `occlusionTexture`, `emissiveTexture`).
- glTF nodes/scenes: build the runtime scene graph from glTF nodes, OR construct `RenderItem`s directly bypassing the live scene graph.

The last point is the architectural choice. Two options:

**Option α: glTF → runtime scene graph → existing extract path.**
Pros: reuses every existing system (proto, ROUTE, scripts). Cons: loses the binary precision of glTF buffers (forces float32 conversion at parse), forces a runtime representation for things that don't need one.

**Option β: glTF → PackedMesh + TextureDesc directly, into RenderItems.**
Pros: zero precision loss, zero runtime overhead for geometry that doesn't change. Cons: glTF scenes without animation don't need the runtime; glTF scenes with animation/USE need both layers.

**Recommended: hybrid (option γ).** Build the runtime scene graph from glTF (for animation/USE/skinning), but at render time extract directly to `PackedMesh` (skipping `MeshData`'s AoS roundtrip). Skinning is solved by carrying `joints`/`weights`/`inverse_bind_matrices` through to the renderer; animation by carrying per-joint `Mat4` skinning matrices into a per-frame uniform/buffer.

This is what the existing extract path needs to grow into anyway — the runtime already produces `RenderItem`s; glTF extraction should just populate the `PackedMesh` slot instead of the `MeshData` slot.

## Render-Side Upload Mapping

For each (renderer, format) pair, the consumer maps `PackedMesh`/`TextureDesc` to its GPU representation:

| Consumer | PackedMesh upload | TextureDesc upload |
|---|---|---|
| OpenGL PoC | Map each `VertexBufferView` to a separate `GL_ARRAY_BUFFER` (or interleave) | `glTexImage2D` per mip; `GL_COMPRESSED_*` for compressed formats |
| Vulkan | One `VkBuffer` per attribute, `VkVertexInputBindingDescription` for layout | `VkImage` with `VkBufferImageCopy` from `MipLevelDesc::byte_offset` |
| Metal | `MTLBuffer` per attribute + `MTLVertexDescriptor` | `MTLTexture.replace(region:layer:mip:)` from `MipLevelDesc` |
| WebGPU | `GPUBuffer` per attribute + `GPUVertexBufferLayout` | `GPUTexture` + `GPUImageCopyTexture` |
| Godot (via `RenderingDevice`) | `RD::vertex_buffer_create` per attribute | `RD::texture_create` + `texture_update` |

**The same `PackedMesh` and `TextureDesc` flow unchanged into every consumer.** This is the point.

## File Layout (proposed)

New files (all header-only, leaf-of-dep-graph):

```
runtime/extract/PackedMesh.hpp       # VertexBufferView, PackedMesh, ComponentType, VertexAttribute
runtime/extract/TextureDesc.hpp      # TextureDesc, PixelFormat, TextureType, MipLevelDesc
runtime/extract/image/               # optional image decoders, behind CMake flag
  ImageDecoder.hpp                   # abstract interface
  StbImageDecoder.hpp                # PNG/JPEG/WebP via stb_image
  Ktx2Decoder.hpp                    # KTX2 + Basis Universal
  BasisDecoder.hpp                   # standalone Basis (no KTX2 container)
```

New codecs (in `runtime/codecs/`):

```
runtime/codecs/StlReader.hpp         # binary + ASCII STL → PackedMesh
runtime/codecs/PlyReader.hpp         # binary + ASCII PLY → PackedMesh
runtime/codecs/ObjReader.hpp         # OBJ → PackedMesh + future MTL
runtime/codecs/GltfReader.hpp        # glTF JSON + .glb binary → scene graph + PackedMesh + TextureDesc
```

New test files (in `runtime/extract/tests/`):

```
packed_mesh_test.cpp                 # round-trip via PackedMesh; glTF sample loader
texture_desc_test.cpp                # round-trip via TextureDesc; mip layout math
stl_reader_test.cpp                  # synthetic binary STL → PackedMesh
gltf_reader_test.cpp                 # (deferred to v2) simple glTF sample
```

## Adoption Phases

### Phase 1: Abstractions (1 week)
1. Write `runtime/extract/PackedMesh.hpp`.
2. Write `runtime/extract/TextureDesc.hpp`.
3. Extend `RenderItem.hpp` with `Geometry` discriminated union + `TextureRef::resolved_desc`.
4. Update `SceneExtractor` to construct `PackedMesh` from `MeshBuilder` when the underlying node is X3D-generated (i.e. always — `MeshData` becomes a back-compat view over `PackedMesh` with AoS re-materialization).
5. Tests: `packed_mesh_test.cpp`, `texture_desc_test.cpp`.

**Outcome**: existing renderers see no change. New renderers can opt into the binary path.

### Phase 2: Binary geometry codecs (1-2 weeks)
1. Vendor `stb_image` into `runtime/extract/image/` (optional behind CMake flag; can be skipped for minimal builds).
2. Write `StlReader.hpp`, `PlyReader.hpp`, `ObjReader.hpp` — produce `PackedMesh`.
3. Add to `x3d::sdk::parse` overload set: `parseStl(bytes)`, `parsePly(bytes)`, `parseObj(bytes)`.
4. Examples: `examples/04_load_binary_mesh.cpp` — load STL, render to PoC renderer via the new path.

**Outcome**: embedders can load "any 3D file." The "headless X3D runtime for embedded consumption" job description expands to "loads OBJ/STL/PLY too."

### Phase 3: Image decoders + KTX2 (1-2 weeks, optional behind flag)
1. Vendor KTX2 / Basis Universal (behind `X3D_ENABLE_KTX2=ON`).
2. Write `StbImageDecoder.hpp`, `Ktx2Decoder.hpp`.
3. `TextureResolver` extended to accept a `TextureDesc` directly (the codec side).
4. Update `TextureExtract.hpp` to populate `resolved_desc` from the decoded result.

**Outcome**: textures flow through the binary path. Renderers with mip/compressed support can consume directly.

### Phase 4: glTF (2-3 weeks)
1. Write `GltfReader.hpp` — JSON + .glb + buffer parsing.
2. Build runtime scene graph for animated/skinned glTF; construct `PackedMesh` directly at extract time.
3. Map glTF materials to `MaterialDesc` (Physical model).
4. Map glTF textures to `TextureRef::resolved_desc` (via Phase 3 decoders).
5. Conformance test against the glTF sample suite.

**Outcome**: glTF is a first-class codec. Embedders get "loads X3D, OBJ, STL, PLY, glTF, KTX2" — covers ~95% of in-the-wild 3D content.

## Why This Is Bigger Than Per-Codec Work

The codecs themselves (STL parser, PLY parser, glTF parser) are mechanical. The abstractions (`PackedMesh`, `TextureDesc`) are the architectural upgrade that:

1. **Eliminates AoS repack** at GPU upload — direct typed-layout upload for Vulkan/Metal/WebGPU.
2. **Eliminates RGBA8-only textures** — compressed formats (BCn/ETC2/ASTC) become first-class.
3. **Eliminates precision loss** in glTF → runtime conversion (typed ComponentType preserved).
4. **Eliminates per-format data layout** — every binary format shares `PackedMesh`/`TextureDesc` as the lingua franca.
5. **Enables direct GPU upload from binary codecs** without going through the live scene graph — for static glTF scenes, no runtime is needed at all (the codec + extractor alone produce renderable items).

This is the load-bearing layer underneath every binary codec. Build it once, and OBJ/STL/PLY/glTF become trivial. Skip it, and every binary codec is a separate repack.

## Open Questions

1. **Should `MeshData` be deleted post-migration, or kept as a back-compat view?** Recommended: keep `MeshData` as a `PackedMesh`-derived AoS view (rebuild on demand) so existing PoC renderers compile unchanged. Phase out only after the renderer migration is complete.

2. **Should image decoders be mandatory in the SDK or optional?** Recommended: optional behind `X3D_ENABLE_IMAGE_DECODERS=ON`. Embedders that want zero deps keep Duktape-only; embedders that want binary textures opt in.

3. **glTF conformance or just "load a representative sample"?** Recommended: representative sample for v2; full glTF CTS as a v3 effort. glTF CTS is a serious commitment (~weeks).

4. **Basis Universal / KTX2 dependency?** Recommended: defer to Phase 3; if the cost is too high, ship without compressed textures for v2 and let embedders wire their own decoder.

## Decision Required

Adopt this design as the v1.5 / v2 architectural pivot for binary codecs?

If yes, the next step is implementing Phase 1 (`PackedMesh.hpp` + `TextureDesc.hpp` + `Geometry` discriminated union in `RenderItem.hpp`). This is a 1-week effort, additive, no breaking changes.
