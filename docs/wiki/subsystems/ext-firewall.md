---
title: "Ext Firewall"
summary: The ext firewall isolates binary-mesh, STL, and ExternalGeometry code behind a quarantined directory, a one-way dependency, and an opt-in CMake flag — keeping the spec-correct core pristine.
tags: [subsystem, ext-firewall, binary, stl, external-geometry, externproto]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../decisions/0001-ext-firewall.md
  - extract.md
---

# Ext Firewall

## Purpose

The ext firewall contains every piece of code that is non-spec, binary-format, or third-party within a quarantined directory tree (`runtime/ext/`) that the spec-correct core must never include. The boundary problem it solves: the VR CAVE consumer needs typed binary geometry (`PackedMesh`) routed via an `ExternalGeometry` dialect node, but the generated bindings, golden layer, and conformance gate must remain ISO 19775-1-pure. The firewall makes that containment structural: a one-way dependency (ext may include core; core must never include ext), an opt-in CMake flag (`X3D_CPP_BUILD_EXT`, default `OFF`), and a resolver seam that the embedder opts into explicitly at parse time. X3D files that use `ExternalGeometry` declare it via a standard `<ExternProtoDeclare>` and degrade gracefully in parsers that do not install the ext resolver.

## Key files

| File / directory | Role |
|---|---|
| `runtime/ext/ExternalGeometry.hpp` | Hand-written `X3DNode` subclass — the `ExternalGeometry` dialect node; holds `url`, `bboxCenter`, `bboxSize`, `contentType`; `defaultContainerField()` is `"geometry"` |
| `runtime/ext/ExtResolver.hpp` | `install(base)` factory — wraps a `ProtoDeclarationResolver` to intercept `urn:x3d-cpp-gen:ext:ExternalGeometry` and delegate other URNs to `base`; caches the ProtoDeclaration per-process via `static` |
| `runtime/ext/ExternalGeometryResolver.hpp` | `makeExternalGeometryResolver(bytesResolver)` — the lazy materialization seam; produces a `std::function<PackedMesh(const X3DNode*, AssetResolver)>` for `MeshBuildOptions::externalGeometryResolver`; owns a url-keyed `PackedMesh` cache |
| `runtime/ext/codecs/StlReader.hpp` | Binary STL codec: `parseStlBinary(data, len)` and `parseStlBinary(vector)` — reads the 84 + N×50 byte layout, produces a non-indexed `PackedMesh` (Position + Normal, `Topology::Triangles`, `ccw=true`, `solid=true`) |
| `include/x3d/ext.hpp` | Opt-in embedder façade; re-exports `x3d::ext::ExternalGeometry`, `x3d::ext::install`, `x3d::ext::kExternalGeometryUrn`, `x3d::ext::makeExternalGeometryProto`, `x3d::ext::makeExternalGeometryResolver` |
| `runtime/ext/tests/` | Four ext-gated tests (see "How it is tested") |

## Interfaces and seams

### Exposed interface

The embedder surface is exported from `include/x3d/ext.hpp` under `namespace x3d::ext`:

```cpp
// 1. Parse-time: install the EXTERNPROTO interceptor.
//    Call once at single-threaded setup, before parseDocument/parseFile.
x3d::codec::ProtoDeclarationResolver
x3d::ext::install(x3d::codec::ProtoDeclarationResolver base =
                      x3d::codec::localFileProtoResolver);

// 2. Activation constant — the URN in <ExternProtoDeclare url='…'/>
inline constexpr const char* x3d::ext::kExternalGeometryUrn =
    "urn:x3d-cpp-gen:ext:ExternalGeometry";

// 3. Extract-time: build the lazy-materialization resolver.
//    Pass the returned function to MeshBuildOptions::externalGeometryResolver.
std::function<PackedMesh(const X3DNode*, AssetResolver)>
x3d::ext::makeExternalGeometryResolver(AssetResolver bytesResolver);

// 4. The node type itself (advanced use: build scenes programmatically).
class x3d::ext::ExternalGeometry : public X3DNode { … };
```

Canonical embedder usage (from the `include/x3d/ext.hpp` file header):

```cpp
#include "x3d/sdk.hpp"        // core surface — include this first
#include "x3d/ext.hpp"        // ext add-on (requires x3d_cpp::ext link)

auto resolver = x3d::ext::install();
auto doc = x3d::sdk::parseDocument(text, x3d::sdk::Encoding::XML, "", resolver);
```

### Seam points

- **`ProtoDeclarationResolver` seam** — `install(base)` wraps any existing `ProtoDeclarationResolver` (default: `x3d::codec::localFileProtoResolver`). The returned resolver intercepts the `kExternalGeometryUrn` and returns a cached `ProtoDeclaration` whose body is an `ExternalGeometry` node with four IS-wired fields (`url`, `bboxCenter`, `bboxSize`, `contentType`). All other URL lists are forwarded to `base`. No new parameters on `parseDocument` — the existing resolver argument is the exact opt-in gap.

- **`MeshBuildOptions::externalGeometryResolver` seam** — `makeExternalGeometryResolver(bytesResolver)` returns a `std::function<PackedMesh(const X3DNode*, AssetResolver)>`. The extractor calls this function when it encounters a geometry node that is not a known core type. The closure `dynamic_cast`s to `ExternalGeometry`, reads `url`, sniffs the extension (`.stl` → binary STL; else returns empty `PackedMesh`), fetches bytes via the embedder-supplied `AssetResolver` with `AssetKind::ExternalGeometry`, calls `parseStlBinary`, caches the result keyed by URL, and returns the `PackedMesh`. An empty `PackedMesh` (`vertex_count == 0`) signals Pending/skip to the extractor.

- **`fallbackNodeCreator()` hook** — `install()` also registers `x3d::runtime::fallbackNodeCreator()` exactly once (`std::once_flag`) so that `deepClone` (called during EXTERNPROTO expansion) can instantiate `ExternalGeometry` nodes by type name. This is a process-global, sticky, set-once hook — intended for single-threaded setup time.

- **`AssetResolver` / `AssetKind::ExternalGeometry`** — the ext layer uses the same `AssetResolver` abstraction (from `runtime/extract/AssetResolver.hpp`) used for textures, requesting `AssetKind::ExternalGeometry`. Pending vs. ready vs. failed is signalled by `AssetResult`.

### Data flow summary

```
X3D file
  <ExternProtoDeclare url='"urn:x3d-cpp-gen:ext:ExternalGeometry"'/>
  <Shape>
    <ProtoInstance name='ExternalGeometry'>
      <fieldValue name='url' value='"chair.stl"'/>
    </ProtoInstance>
  </Shape>
         │
         ▼  parseDocument( …, install() )
  EXTERNPROTO expansion (core machinery, no ext code)
         │  intercepts kExternalGeometryUrn → factory ProtoDeclaration
         │  IS-wires url → ExternalGeometry.url
         ▼
  ExternalGeometry node in scene graph
         │
         ▼  SceneExtractor + MeshBuildOptions::externalGeometryResolver
  makeExternalGeometryResolver(bytesResolver):
    dynamic_cast → ExternalGeometry
    getUrl() → "chair.stl"
    extension sniff → .stl
    bytesResolver("chair.stl", AssetKind::ExternalGeometry)
    parseStlBinary(bytes) → PackedMesh
    cache[url] = mesh
         ▼
  RenderItem { geometry_ext.kind = Packed, geometry_ext.packed = PackedMesh }
```

## How it is tested

All ext tests are compiled and registered only inside the `if(X3D_CPP_BUILD_EXT)` block in `CMakeLists.txt` and run only when `-DX3D_CPP_BUILD_EXT=ON` is set. They therefore do not appear in, and do not affect, the default `ctest` run.

- **`stl_reader_test`** (`runtime/ext/tests/stl_reader_test.cpp`) — synthesizes a two-triangle binary STL in-memory and asserts: `vertex_count == 6`, `index_count == 0`, Position and Normal attributes present with correct per-vertex values, AABB computed correctly (`min=(0,0,0)`, `max=(3,1,0)`), and three malformed-input cases (buffer too short, truncated triangle data, null pointer) all return empty `PackedMesh` without crashing.

- **`external_geometry_node_test`** (`runtime/ext/tests/external_geometry_node_test.cpp`) — unit tests the `ExternalGeometry` node class: default field values, typed accessors, `nodeTypeName() == "ExternalGeometry"`, `defaultContainerField() == "geometry"`, reflection `FieldTable` correctness (field names, types, access modes), get/set thunks, `accept(NodeVisitor&)`, and DEF/USE inheritance from `X3DNode`.

- **`external_geometry_roundtrip_test`** (`runtime/ext/tests/external_geometry_roundtrip_test.cpp`) — end-to-end parse + round-trip: (1) parses a doc with `<ExternProtoDeclare>` + `<ProtoInstance name='ExternalGeometry'>` using `install()`, confirms zero proto warnings and `url == ["chair.stl"]` on the expanded node; (2) verifies `expandedSources` maps the primary node back to the `ProtoInstance`; (3) re-serializes to XML and asserts `<ExternProtoDeclare>` + `<ProtoInstance>` round-trip with no raw `<ExternalGeometry>` element emitted; (4) verifies graceful degradation — parsing the same doc without `install()` records `ProtoWarning::Kind::UnresolvedExtern` and produces a clean (non-crashing) round-trip; (5) verifies URN delegation — non-matching URNs reach the stub base resolver, matching URNs are intercepted without forwarding.

- **`external_geometry_e2e_test`** (`runtime/ext/tests/external_geometry_e2e_test.cpp`) — Round 2 lazy materialization proof: builds a `Group → Shape → ExternalGeometry(url="m.stl")` scene programmatically, wires `makeExternalGeometryResolver` with a counting stub that returns a known two-triangle binary STL, runs `SceneExtractor::fullSnapshot()`, and asserts: (1) PACKED — one `RenderItem` with `geometry_ext.kind == Packed`, `vertex_count == 6`, and the known v0 position `(1,2,3)`; (2) CACHE — a second extract reuses the cached `PackedMesh` and the stub byte-provider was called exactly once; (3) PENDING — an unresolvable URL results in no `RenderItem` emitted, no crash.

## Related specs and ADRs

- [ADR-0001: External-Code Firewall](../decisions/0001-ext-firewall.md) — the rationale, four-layer firewall design, trade-offs, and CMake target structure
- [Architecture](../architecture.md)
- [Extract subsystem](extract.md) — the extraction pipeline that calls the `externalGeometryResolver` seam
- Dated design spec: `docs/superpowers/specs/2026-06-19-binary-geometry-extension-design.md` — consolidated design; cites ISO 19775-1 §4.4.5.1 and the EXTERNPROTO extension point
- Dated spec: `docs/superpowers/specs/2026-06-18-binary-mesh-texture-abstractions.md` — earlier abstraction design that preceded the firewall split
- Ingestion roadmap: the [GitHub Project](https://github.com/users/delta9000/projects/2) — deferred items including the portable `.x3d` fallback baker (second slot in the EXTERNPROTO url list) and future codecs (PLY, OBJ single-mesh, glTF per-mesh)
