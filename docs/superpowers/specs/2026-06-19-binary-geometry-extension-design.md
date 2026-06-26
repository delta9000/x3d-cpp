# Binary Geometry Extension — Consolidated Design

**Date:** 2026-06-19
**Status:** Design proposal for ratification. **Supersedes** the extract-only `2026-06-18-binary-mesh-texture-abstractions.md` (its `PackedMesh`/`TextureDesc` are folded in *with corrections*); incorporates the IFS/texture audit, the EXTERNPROTO-native-backing spike, the X3DOM/InstantReality prior-art reference (`2026-06-19-binary-geometry-prior-art-reference.md`), and two read-only integration surveys (extract-side + scene-graph-side).
**Grounding memory:** `x3d-cpp-gen-ingestion-roadmap`, `x3d-binary-geometry-prior-art`.

> **REVISION 2026-06-20b — scene-vs-geometry routing (`Inline` vs `ExternalGeometry`).** The two external-reference nodes split by what the source IS, not by encoding:
> - **A *scene* source** (glTF, OBJ+MTL, anything with hierarchy / materials / multiple meshes) routes through **`Inline`** → it BECOMES an X3D **subtree** (Transform/Shape/Appearance/PhysicalMaterial + geometry leaves). This reuses the SHIPPED Spec-1 Inline expansion (resolver returns a `Scene`; for glTF the resolver is a glTF→`Scene` converter — DEF-isolation, route hoisting, reference-preserving round-trip all free). **Spec-SANCTIONED** (§9.4.2 lists `model/gltf-bin` as an Inline content type) — X3DOM does exactly this; for X3D 4.0 it's not abuse. The loaded glTF "becomes X3D" in memory.
> - **A *single-mesh* source** (binary/ASCII STL, PLY) routes through **`ExternalGeometry`** → a single `PackedMesh` geometry leaf (no subtree; there's nothing to become).
> They COMPOSE: a glTF-via-`Inline` subtree's Shapes contain `ExternalGeometry`/`PackedMesh` mesh leaves → X3D identity for structure + binary fidelity for meshes (the X3DOM `BufferGeometry` both-worlds result, cleaner). So glTF support = (glTF→Scene Inline resolver) + (per-mesh ExternalGeometry leaves), NOT an ExternalGeometry format-fork. `ExternalGeometry`'s format-fork is single-mesh formats only. **Implication: glTF is well-positioned — the composition layer (Inline) is done; only the glTF→Scene converter remains.**
>
> **REVISION 2026-06-20 — node is `ExternalGeometry`, NOT `BinaryGeometry`.** The lazy decision makes the node a pure *reference* (url + bbox); "binary" is an *encoding*, not a node property, and external sources are often text (OBJ, ASCII STL/PLY) or mixed (glTF). So: node = **`ExternalGeometry { url MFString, bboxCenter, bboxSize, [contentType hint] }`** — NO binary-layout/accessor fields on the node (those were an eager-model artifact; they live in `PackedMesh`, produced by the resolver). The **"binary-or-not" fork lives entirely in the resolver/codec layer** (sniff format → dispatch to STL-binary/STL-ascii/OBJ/PLY/glTF codecs, each → `PackedMesh`). Renames throughout this doc: `BinaryGeometry`/`BinaryGeometryProxy` → **`ExternalGeometry`**; `binaryGeomResolver` → **`externalGeometryResolver`**; `AssetKind::BinaryGeometry` → **`AssetKind::ExternalGeometry`**; URN `urn:x3d-cpp-gen:ext:ExternalGeometry`. "Binary" survives ONLY in format-specific codec names (`parseStlBinary`). Symmetry with `Inline`: two external-reference nodes by output type — `Inline`→`Scene`, `ExternalGeometry`→`PackedMesh`. Mirrors the prior-art's own arc (BinaryGeometry→ExternalGeometry/SRC→glTF). The §A node field table below is superseded by this banner (url+bbox only). The already-merged P1 seam/kind keep their old names until the next phase renames them.

## Goal

Bring high-fidelity binary geometry + textures (glTF, binary STL/PLY/OBJ, embedded images) into the headless renderer-agnostic X3D runtime **without precision loss and without polluting the spec-correct core** — as a *runtime-side extension*, clearly firewalled, opt-in, that still gives imported assets full X3D node identity (DEF/USE, `Inline`-compose, Transform, serialize) and reference-preserving round-trip.

## Locked decisions

1. **Custom node on the runtime side, clearly separate** (user). Hard firewall: own tree `runtime/ext/`, namespace `x3d::runtime::ext`, hand-written (NOT `generated_cpp_bindings/`, NOT golden-gated), one-way dep `ext→core`, opt-in default-off.
2. **Spec-conformant dialect via EXTERNPROTO + native resolver** (spike-validated). The ext node is declared `<ExternProtoDeclare>`; its implementation is the "implementation-dependent mechanism" of ISO §4.4.5.1 — a native resolver injected through the existing seam. Files stay valid X3D; degrade gracefully (read-and-ignore) elsewhere; EXTERNPROTO url-list carries native-first + portable-`.x3d`-fallback.
3. **LAZY materialization** (user). The ext node is a lightweight **proxy** (url + bbox + glTF-accessor refs); it does NOT expand to geometry at parse. `SceneExtractor` materializes it at extract time via a cached binary-geometry resolver (mirrors the `AssetResolver` render-time/Pending contract). Mirrors X3DOM `ExternalGeometry`/`POPGeometry`.
4. **Mirror glTF's accessor model** (prior-art consensus: BinaryGeometry→SRC→glTF→X3DOM `BufferGeometry`/`BufferView`/`BufferAccessor`). No bespoke schema. **There is no "BinBuffer" primitive** — just owned bytes + typed views.

## Architecture — two sides of one membrane

### A. Scene-graph side (`runtime/ext/`)

**`BinaryGeometryProxy`** — a hand-written `X3DNode` subclass (NOT generated). `nodeTypeName() == "BinaryGeometry"`, `defaultContainerField() == "geometry"`. Static hand-written `FieldTable` (no codegen, no `DynamicFieldStore`) mirroring glTF accessors:

| Field | X3D type | Role |
|---|---|---|
| `url` | MFString | binary blob url(s) — native URN first, portable `.x3d` fallback |
| `bboxSize`/`bboxCenter` | SFVec3f | pre-computed bounds (so `BoundsSystem` needs no geometry) |
| `bufferByteLength` | SFInt32 | blob length |
| `indexByteOffset`/`indexCount`/`indexType` | SFInt32/…/SFString | index accessor (Uint16/Uint32) |
| `coordByteOffset`/`coordType` | SFInt32/SFString | position accessor (Float32/…) |
| `normalByteOffset`/`texCoordByteOffset` | SFInt32 | optional accessors (−1 = absent) |
| `byteStride` | SFInt32 | interleaved stride (0 = packed) |
| `primType` | SFString | TRIANGLES/LINES/POINTS |

**Activation — `x3d::runtime::ext::install`** (in `runtime/ext/ExtResolver.hpp`):
```cpp
// returns a ProtoDeclarationResolver that intercepts urn:x3d-cpp-gen:ext:*
// and returns the ext factory's ProtoDeclaration; delegates everything else.
ProtoDeclarationResolver install(ProtoDeclarationResolver base = localFileProtoResolver);
```
Embedder opt-in (zero impact if absent):
```cpp
auto r = x3d::runtime::ext::install();
auto doc = x3d::sdk::parseDocument(text, Encoding::Unknown, base, r);
```
No new params on `parseDocument`/`parseFile` — the existing `ProtoDeclarationResolver` seam (which already skips `urn:` in its default) is the exact opt-in gap. No globals, no static registration. The default file-resolver path is untouched.

**Lazy-proxy expansion — "Option A" (reuses everything, zero core changes).** The ext resolver returns a `ProtoDeclaration` whose `body.nodes[0]` is a `BinaryGeometryProxy` with IS-wired interface fields. `expandInstance` deep-clones + splices it normally → the proxy sits in the graph at the right slot (Transform/bounds/CAVE-serialization see a real node), and `scene.expandedSources[proxy] = inst` is recorded. **No changes to `expandInstance`/`expandScene`/any writer.**

**Round-trip** falls out of the existing AUD-B redirect: `writeNodeElement` sees `expandedSources[proxy]` → re-emits `writeProtoInstanceElement(inst)` → file round-trips as `<ExternProtoDeclare>` + `<ProtoInstance name="BinaryGeometry">`, never the proxy's own fields, never the binary payload. The `<ExternProtoDeclare>` survives expansion in `scene.externProtoDeclarations`. (The recently-merged nested-ProtoInstance writer fix also covers the un-resolved/degraded case.)

### B. Extract side (`runtime/extract/`)

> **Naming trap (from survey):** the per-item record named `RenderItem` lives in `SceneExtractor.hpp:94`; `RenderItem.hpp` is the POD-types leaf (`MeshData`, `TextureRef`, …). The `Geometry` union goes on the `SceneExtractor.hpp` record.

- **`Geometry` discriminated union** (additive) — `RenderItem.hpp` after `MeshData`:
  ```cpp
  struct Geometry { enum class Kind { AoS, Packed }; Kind kind = Kind::AoS;
                    MeshData aos; PackedMesh packed;
                    bool is_packed() const { return kind == Kind::Packed; } };
  ```
  Existing AoS path is the default for every standard node — no consumer breaks.
- **`TextureRef::Source::Buffer`** (the GLB-embedded-image hole) — new enum value + `std::vector<uint8_t> bufferBytes` + `std::string mimeHint` + `std::optional<TextureDesc> resolved_desc`. Produced in `MaterialSystem::refOf()` (the `PixelTexture`/`MovieTexture`/`ImageTexture` dispatch, ~line 110) for an ext binary-image node; bytes are inline on the node, so no resolver call at `refOf()` time.
- **Lazy materialization hook** — a generic `std::function<PackedMesh(const X3DNode*, AssetResolver)> binaryGeomResolver` field on `MeshBuildOptions` (mirrors the existing `geoProjection`/`fontMetrics` seams; deals only in core types `PackedMesh`+`X3DNode*` → **no ext-type leak, firewall intact**; null by default). In `SceneExtractor::walk()` (~line 521), when `buildLocalMesh` returns `recognized==false` on a `BinaryGeometry` proxy and the resolver is set: call it → empty `PackedMesh` ⇒ Pending (skip this tick, retry next), non-empty ⇒ new `emitPacked()` stores `Geometry{Packed}`. **Cache is owned by the embedder's lambda** (keyed on url/hash) — no cache state in the extractor, same as textures.
- **`AssetResolver`** — add `AssetKind::BinaryGeometry` (contract A, Pending-able). Reuse the one resolver callback; the `binaryGeomResolver` is the higher-level (bytes→`PackedMesh`) layer that calls it.

### C. The data structures — `PackedMesh` + `TextureDesc` (corrected)

Adopt the 2026-06-18 structs with these **mandatory corrections** (from the extract survey, mirroring glTF):
1. **BLOCKER:** `ComponentType::HalfFloat = 5125` **collides** with glTF `UnsignedInt = 5125`. Fix: HalfFloat → `5131` (KHR_mesh_quantization) or drop for v1. Resolve before freezing the header.
2. Add `uint32_t byte_stride = 0` to `VertexBufferView` (0 = tightly packed) — glTF `bufferView.byteStride`, needed for zero-copy interleaved DCC output.
3. Add `uint32_t byte_offset = 0` to `VertexBufferView` — glTF `accessor.byteOffset`, needed to sub-slice a shared mmap'd `.glb` blob.
4. Model the **owned slab**: `std::vector<std::vector<uint8_t>> attribute_data` on `PackedMesh`; `VertexBufferView::data` points into it (the spec said "owned" but modeled non-owning).
5. Move `Topology` to its own `runtime/extract/Topology.hpp` so `PackedMesh.hpp` stays a leaf with no `RenderItem.hpp` dep.

## Firewall — 4-layer enforcement

1. **Code+namespace:** `runtime/ext/` + `x3d::runtime::ext`, one-way dep (core never `#include`s ext — trivially verified: today no `runtime/ext/` exists). Hand-written, generator untouched.
2. **Opt-in default-off:** vanilla `parseDocument`/`parseFile` = 100% as today; ext fires only when the embedder calls `ext::install(...)`. No static init in any core TU.
3. **Visible in file:** always `<ExternProtoDeclare url='"urn:x3d-cpp-gen:ext:BinaryGeometry" "BinaryGeometry.x3d"'/>`; readers without the ext module skip the URN, try the `.x3d` fallback, else record `ProtoWarning::UnresolvedExtern` — no crash, no loss.
4. **Quarantine from gates:** new CMake option `X3D_CPP_BUILD_EXT=ON` (default **OFF**) + target `x3d_cpp_ext`; ext tests live in `runtime/ext/tests/`, NOT golden/conformance-gated; the standard suite + `mise run conformance-gate` never link or see ext. Public API: separate add-on header `include/x3d/ext.hpp` (core `include/x3d/sdk.hpp` never includes it; `x3d::sdk` gains no ext `using`s).

## Integration plug-points (implementation map)

| Concern | File:line | Change shape |
|---|---|---|
| Resolver opt-in gap | `runtime/parse/X3DParse.hpp` (parseDocument sig; `localFileProtoResolver` skips `urn:`) | wrap via `ext::install` — no core edit |
| Proxy expansion | `runtime/X3DProtoExpand.hpp` `expandInstance`/`expandScene` | none (reused as-is) |
| Round-trip redirect | `runtime/codecs/XmlWriter.hpp` `writeNodeElement` `expandedSources` check | none (reused as-is) |
| Geometry union | `runtime/extract/RenderItem.hpp` after `MeshData`; record at `SceneExtractor.hpp:94` | additive struct + `emitPacked()` |
| Encoded-texture home | `runtime/extract/RenderItem.hpp` `TextureRef::Source`; `MaterialSystem.hpp` `refOf()` ~110 | additive enum + fields + branch |
| Lazy geom hook | `runtime/extract/MeshBuilder.hpp` `MeshBuildOptions`; `SceneExtractor.hpp` `walk()` ~521 | additive `std::function` + dispatch arm |
| Asset kind | `runtime/extract/AssetResolver.hpp` `AssetKind` ~54 | one enum value |

## Phased implementation

**Phase 1 — extract abstractions (additive, decision-free, no ext yet).** New `PackedMesh.hpp` (with the 5 corrections) + `TextureDesc.hpp` + `Topology.hpp`; `Geometry` union + `Source::Buffer` in `RenderItem.hpp`; `AssetKind::BinaryGeometry`; `binaryGeomResolver` seam on `MeshBuildOptions`; `emitPacked()` + the `walk()` dispatch arm. Zero change to any existing AoS/text path; golden stays byte-identical. **2 new files + ~4 small edits.**

**Phase 2 — the ext module + lazy proxy.** `runtime/ext/` tree: `BinaryGeometryProxy` node, the `ProtoDeclaration` factory, `ext::install`, `include/x3d/ext.hpp`, `X3D_CPP_BUILD_EXT` + `x3d_cpp_ext` target + `runtime/ext/tests/`. Proves: parse `<ExternProtoDeclare>`+`<ProtoInstance>` with `install`ed resolver → proxy in graph → round-trips as the reference. (No binary parsing yet.)

**Phase 3 — first binary codec (de-risk the materialization).** A binary STL reader (~80 LOC) → `PackedMesh`; wire the `binaryGeomResolver` lambda (with cache) so a proxy materializes → `emitPacked` → a renderable `RenderItem`. End-to-end lazy proof on the simplest format.

**Phase 4 — glTF.** GLB container + accessors → `PackedMesh` (zero-copy via byte_offset/stride); embedded images → `Source::Buffer`/`TextureDesc`; PBR → `PhysicalMaterial`. Build the runtime scene graph for animated/skinned; extract directly to `PackedMesh`.

## Judgment calls made (user delegated "the rest")

- URN namespace: **`urn:x3d-cpp-gen:ext:`** (defensible; no claim on an unregistered `x3d-native`).
- Extract hook on `MeshBuildOptions` (generic, core-typed) rather than a separate `ExtMeshBuildOptions` — keeps the firewall (no ext type in the signature) while following the established seam pattern.
- `ext::install` is **single-node now**; evolve to a URN→factory registry when a 2nd ext node lands (YAGNI).
- Proxy reflection: **hand-written static `FieldTable`** (fixed interface; cleaner than `DynamicFieldStore`).

## Open questions for ratification

1. **PackedMesh `byte_stride`/`HalfFloat`** — confirm the corrections (esp. the 5125 collision fix) before Phase 1 freezes the header.
2. **First codec for Phase 3** — binary STL (simplest, de-risks the lazy loop) vs jump to a glTF subset (higher value, more surface). Recommend STL first.
3. **Portable `.x3d` fallback baking** — should the ext module also emit a text-geometry fallback at asset-prep (the EXTERNPROTO url-list's 2nd slot)? Reserve the slot; defer the baker until an interchange consumer asks.
4. **`BoundsSystem` leniency** — confirm it falls through to `bboxSize`/`bboxCenter` on an unrecognized proxy geometry (the proxy carries both); a conformance-correctness item, not a blocker.
