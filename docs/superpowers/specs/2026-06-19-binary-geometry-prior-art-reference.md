# Binary Geometry Prior Art — X3DOM & InstantReality Reference

**Date:** 2026-06-19
**Status:** Reference. Grounds the binary-geometry extension design (see `2026-06-18-binary-mesh-texture-abstractions.md`) and the runtime-side custom-node decision (memory: `x3d-cpp-gen-ingestion-roadmap`).
**Method:** Two parallel web-research passes against X3DOM GitHub source (read directly), `doc.x3dom.org`, `doc.instantreality.org`, and the Fraunhofer IGD papers. Field tables verified against source where noted. Unverified items flagged inline.

## TL;DR — three findings that change our assumptions

1. **There is no `BinBuffer` node/type/blob primitive.** The "BinBuffer" recollection is informal: it's the **`#byteOffset+byteStride` URL-fragment convention** that lets several attribute fields view into one shared interleaved `.bin` file (e.g. `coord='mesh.bin#0+24'`, `normal='mesh.bin#8+24'`). Binary data is always carried by **plain URLs in `SFString` fields**; all layout metadata lives in the *node's fields*, and the `.bin` is a raw, headerless, flat typed array. No named container, no blob field type, no ownership-transfer mechanism. → Our "BinBuffer as an ownership primitive" framing was inventing something the prior art doesn't have; the prior art's ownership model is just "the browser fetches a URL and caches it."

2. **The whole field converged on glTF's accessor model.** Lineage: `BinaryGeometry` (2012, one file per attribute) → **SRC** (2014, single self-describing JSON-header + binary container) → **glTF 2.0** → current X3DOM **`BufferGeometry` + `BufferView` + `BufferAccessor`** (a direct mirror of glTF buffer/bufferView/accessor). X3DOM **removed** `ExternalGeometry`, `ImageGeometry`, SRC, and glTF-1.0 in **v1.8.2**; `BinaryGeometry`/`POPGeometry` survive but are legacy. → Strongly validates the "mirror glTF accessors" hedge: `PackedMesh` (our extract-side spec) and X3DOM's newest node are *the same model*. Don't design a bespoke buffer schema — adopt glTF's.

3. **Both browsers shipped these as native, non-standardized nodes** (not EXTERNPROTO, no `COMPONENT` declaration) — X3DOM's own AOPT docs call BinaryGeometry "a non-standardized, optimized node," and scenes using them are browser-specific. → Our decision (EXTERNPROTO-with-native-backing, opt-in, quarantined) makes a different trade-off on the conformance axis: it keeps the same binary fidelity while preserving spec-conformance + graceful degradation + round-trip.

## The trajectory (the strategic finding)

| Era | Mechanism | Shape | Status in X3DOM master |
|---|---|---|---|
| 2012 | `BinaryGeometry` | N URLs (one flat `.bin` per attribute) + per-attribute type fields | Present (legacy) |
| 2012 | `ImageGeometry` | attributes as pixels in `ImageTexture` nodes (progressive via image rows) | **Removed 1.8.2** |
| 2013 | `POPGeometry` | single interleaved buffer, progressive POP-buffer streaming w/ per-level chunks | Present (experimental) |
| 2014 | `ExternalGeometry` + **SRC** | one self-describing container (JSON header + binary payload) | **Removed 1.8.2** |
| 2018+ | glTF 2.0 via `Inline`/`ExternalShape` | standard glTF | glTF path kept; ExternalShape removed 1.8.2 |
| now | `BufferGeometry`/`BufferView`/`BufferAccessor` | glTF buffer/bufferView/accessor schema as X3D nodes | **Current** |

The arc is unambiguous: from ad-hoc per-attribute binary → self-describing container → **glTF as the lingua franca**. The newest X3DOM node literally re-encodes glTF's accessor tree as X3D nodes. Our `PackedMesh` (`ComponentType`/`VertexBufferView` mirroring glTF) is already on this trajectory.

## Node reference

### `X3DBinaryContainerGeometryNode` (base for BinaryGeometry / POPGeometry / ImageGeometry)
| Field | Type | Default | Meaning |
|---|---|---|---|
| `position` | SFVec3f | `0 0 0` | BBox centre; also the decode-origin for quantized (Int16) attributes |
| `size` | SFVec3f | `1 1 1` | BBox extents; with `position` defines the AABB + dequant transform |
| `vertexCount` | MFInt32 | `[0]` | vertex count per sub-mesh / draw call (one per `primType`) |
| `primType` | MFString | `["TRIANGLES"]` | per-sub-mesh primitive type: `POINTS`/`LINES`/`LINELOOP`/`LINESTRIP`/`TRIANGLES`/`TRIANGLESTRIP`/`TRIANGLEFAN` |

(Source: X3DOM `X3DBinaryContainerGeometryNode.js`, read directly.)

### `BinaryGeometry` (X3DOM + InstantReality — agree on the core)
Per-attribute **URL** fields (`SFString`, default `""`): `index`, `coord`, `normal`, `texCoord`, `color`, `tangent`(NYI/TODO), `binormal`(NYI/TODO).
Paired **type** fields (`SFString`): `indexType`(`"Uint16"`), `coordType`(`"Float32"`), `normalType`, `texCoordType`, `colorType`, `tangentType`, `binormalType`. Allowed: `Int8 Uint8 Int16 Uint16 Int32 Uint32 Float32 Float64`.
Encoding flags: `normalAsSphericalCoordinates`(SFBool,false — 2-component θ,φ normals), `rgbaColors`(SFBool,false), `numTexCoordComponents`(SFInt32,2), `normalPerVertex`(SFBool,true), `idsPerVertex`(SFBool,false — IDs packed in texcoords), `compressed`(SFBool,false — GZip transport).
Inherited geometry: `solid`,`ccw`,`lit`,`useGeoCache`,`metadata`.
InstantReality-only extras: `resolution`(SFFloat,1), `multiResolutionLevel`(SFInt32,1), `normalUpdateMode`(none/fast/`nice`), `cacheMode`(`auto`/dlist/vbo/off), `optimizationMode`(MFString,auto), `invalidateVolume`(SFBool).
Index type default `Uint16` (WebGL1 native); `Uint32` via `OES_element_index_uint`; AOPT `-a` splits meshes to dodge the 65535 limit. Three quantization schemes, combinable: compact Int16 (`-c`, uses position/size BBox to dequant), spherical normals (`-p`), GZip (`compressed`). ~6 bytes/triangle in the most compact combo.

### `BufferGeometry` + `BufferView` + `BufferAccessor` (X3DOM, current, glTF-aligned)
`BufferGeometry`: `buffer`(SFString URL to one binary blob), `draco`(SFBool — KHR_draco_mesh_compression), `views`(MFNode `BufferView`), `accessors`(MFNode `BufferAccessor`), + `position`/`size`/`primType`/`vertexCount`/`ccw`/`lit`/`solid`/`useGeoCache`.
`BufferView`: `byteOffset`, `byteLength`, `byteStride`.
`BufferAccessor`: `bufferType`, `view`, `byteOffset`, `componentType`(GL numeric constant), `count`, `components`(elements per value).
→ This is glTF's `buffer`/`bufferView`/`accessor` re-expressed as X3D nodes. **This is the shape to mirror.**

### `ExternalGeometry` (removed from X3DOM master 1.8.2; live in InstantReality + X3DOM stable/1.7.x)
Single field that matters: `url`(MFString — references a whole geometry container, e.g. SRC; multiple = fallback list; `#mesh0` fragment selects a sub-mesh). InstantReality adds `resolution`/`multiResolutionLevel`/`normalUpdateMode`/`cacheMode`/`optimizationMode`/`sources`(MFNode). **Lazy/on-demand load.** The high-level "load this whole geometry file" mechanism (vs BinaryGeometry's low-level explicit attribute arrays).

### `ImageGeometry` (removed X3DOM 1.8.2; InstantReality live)
Attribute fields are **MFNode/SFNode `X3DTextureNode` slots** (`index`,`coord`,`normal`,`texCoord`,`color`) — geometry packed as pixels in textures, decoded on the GPU, **progressive via image-row streaming**. `implicitMeshSize`(SFVec2f,256×256), `implicitMeshMode`(`3DSequence`), `numColorComponents`, `numTexCoordComponents`.

### `POPGeometry` (X3DOM, experimental, live)
**True progressive streaming.** Single interleaved buffer; child `PopGeometryLevel` nodes each carry `src`(URL chunk), `numIndices`, `vertexDataBufferOffset`. Renderer pre-allocates the GPU buffer (`vertexBufferSize`), appends chunks as they arrive, renders increasing precision with **no CPU decode** and **no full-download gate**. Screen-space error (`precisionFactor`, `min/maxPrecisionLevel`) drives LOD. Interleaved layout via `attributeStride` + per-attribute `*Offset`/`*Precision` (default precisions: pos=2B/Uint16, normal=1B/Uint8, texcoord=2B, color=1B).

## The binary-buffer reference mechanism (what "BinBuffer" actually is)
- **Separate mode:** one flat headerless `.bin` per attribute; type/stride/offset all in node fields.
- **Interleaved mode ("the BinBuffer behavior"):** one `.bin`, each attribute URL carries a `#<byteOffset>+<byteStride>` fragment → fed straight to `gl.vertexAttribPointer(size,type,normalized,stride,offset)`. Parsed via `lastIndexOf('#')`/`lastIndexOf('+')`.
- Raw little-endian (implied; never formally specified — X3DOM issue #325 confirms the format was deliberately undocumented). No magic, no schema in the file.
- The newer `BufferGeometry` replaces the fragment hack with explicit `BufferView`/`BufferAccessor` child nodes — i.e. glTF's structured version of the same idea.

## Loading / streaming models (for our eager-vs-lazy decision)
| Node | Model | Streaming | Gate |
|---|---|---|---|
| BinaryGeometry | **eager** (all attribute URLs fire on first render) | none | render waits for ALL downloads (`internalDownloadCount→0`) |
| ExternalGeometry/SRC | **lazy/on-demand**, single container request | partial progressive (SRC chunks) | header-then-payload, zero-copy upload |
| ImageGeometry | progressive via image rows | yes (image streaming) | per-texture |
| POPGeometry | **fully progressive** | yes (per-level chunks, GPU buffer pre-alloc) | none — draws increasing precision live |

Lesson for us: **eager is the simple/bounded choice (what our spike did); SRC's single-self-describing-container + POP's progressive model are the references when CAVE scale forces lazy.** The proxy-node-materialized-at-extract design maps onto ExternalGeometry's lazy `url` model; LOD/streaming maps onto POPGeometry's per-level chunks.

## Extension / conformance framing (how our decision differs)
- X3DOM & InstantReality: **native browser nodes**, registered in the `Geometry3D` component bucket, **no EXTERNPROTO, no COMPONENT declaration**. Explicitly "non-standardized"/"experimental"/"Avalon standard." Scenes using them are browser-specific and not portable to base X3D.
- **Our decision differs deliberately:** custom node behind an **EXTERNPROTO interface + native resolver backing** (spike-validated), `runtime/ext/` + `x3d::runtime::ext`, opt-in, quarantined from golden/conformance. This keeps spec-conformance, graceful degradation (read-and-ignore), and reference-preserving round-trip. We pay a small indirection (node is a ProtoInstance) so the file stays valid, portable-degradable X3D.

## Implications for our design
1. **Adopt glTF's accessor model, not a bespoke schema.** `PackedMesh` already mirrors glTF; the X3D-node face should mirror `BufferGeometry`/`BufferView`/`BufferAccessor` (≈ glTF buffer/bufferView/accessor). This keeps us on the standard's trajectory and makes a future official X3D binary node a migration.
2. **Drop "BinBuffer as a new primitive."** The ownership we need is just: a fetched/owned byte blob + typed views into it (offset/stride/componentType). `PackedMesh.VertexBufferView` already IS the view; its owner is a plain `std::vector<uint8_t>` (or the resolver's cache). No new node/field type required — the encoded-texture gap is closed by a `TextureRef::Source::Buffer` carrying owned encoded bytes + mime hint, not by a "BinBuffer node."
3. **Eager-vs-lazy:** prior art shows both are legitimate; the field's *direction* is lazy/progressive (SRC, POP). For the CAVE, lean lazy: an `ExternalGeometry`-style proxy (url + bbox + accessor refs) materialized at extract, with POP-style per-level chunks as the LOD story when needed.
4. **Our EXTERNPROTO framing is a deliberate delta** from the prior art — keep it; it's what lets imported binary content stay valid X3D.

## Sources
**X3DOM source (read directly):** `BinaryGeometry.js`, `X3DBinaryContainerGeometryNode.js`, `BufferGeometry.js`, `PopGeometry.js`, `BinaryContainerSetup.js`, `glTF/glTFLoader.js` (SRC structure), `CHANGELOG.md` — github.com/x3dom/x3dom (master + stable/1.7.x).
**Docs:** `doc.x3dom.org/author/Geometry3D/{BinaryGeometry,BufferGeometry,PopGeometry}.html`; `doc.instantreality.org/documentation/nodetype/{BinaryGeometry,ExternalGeometry,ImageGeometry}/`.
**Papers:** Behr et al., "Using Images and Explicit Binary Container…", Web3D 2012 (doi 10.1145/2338714.2338717); Limper et al., "SRC — A Streamable Format…", Web3D 2014 (doi 10.1145/2628588.2628589, pdf max-limper.de/publications/SRC/SRC.pdf); Limper et al., "The POP Buffer", CGF/Pacific Graphics 2013 (doi 10.1111/cgf.12227); Jung et al., "Fast and Efficient Vertex Data Representations for the Web", GRAPP 2013.

## Unverified / flags
- `tangent`/`binormal` fields are declared-but-inert (`// TODO` in source).
- Exhaustive `primType` enum confirmed only partially in InstantReality docs (X3DOM source has the full list).
- `.bin` endianness never formally specified (little-endian implied; X3DOM issue #325 confirms format was undocumented).
- SRC JSON header full schema and Behr-2012 full text not read directly (paywalled / not fetched); dequant formula `R = S·D + O` cited from secondary summaries.
- X3DOM `BufferAccessor`/`BufferView` field tables from source read; defaults not all captured.
