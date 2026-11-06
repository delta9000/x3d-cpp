---
title: "Ext Firewall"
summary: The ext firewall isolates binary-mesh, STL, and ExternalGeometry code behind a quarantined directory, a one-way dependency, and an opt-in CMake flag — keeping the core pristine.
tags: [subsystem, ext-firewall, binary, stl, external-geometry, externproto]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../decisions/0001-ext-firewall.md
  - extract.md
---

# Ext Firewall

## Purpose

The extension firewall isolates opt-in foreign geometry under `runtime/ext/`. Core headers do not include that directory. Optional resource backends live separately under `runtime/io/` and other subsystem directories. The boundary problem it solves: the VR CAVE consumer needs typed binary geometry (`PackedMesh`) routed via an `ExternalGeometry` dialect node, but the generated bindings, golden layer, and conformance gate must remain ISO 19775-1-pure. The firewall makes that containment structural: a one-way dependency (ext may include core; core must never include ext), an opt-in CMake flag (`X3D_CPP_BUILD_EXT`, default `OFF`), and a resolver seam that the embedder opts into explicitly at parse time. X3D files that use `ExternalGeometry` declare it via a standard `<ExternProtoDeclare>` and degrade gracefully in parsers that do not install the ext resolver.

## Key files

| File / directory | Role |
|---|---|
| `runtime/ext/ExternalGeometry.hpp` | Hand-written `X3DNode` subclass — the `ExternalGeometry` dialect node; holds `url`, `bboxCenter`, `bboxSize`, `contentType`; appends inherited descriptors from `X3DNode::fields()`; `defaultContainerField()` is `"geometry"` |
| `runtime/ext/ExtResolver.hpp` | `install(base)` factory — wraps a `ProtoDeclarationResolver` to intercept `urn:x3d-cpp-gen:ext:ExternalGeometry` and delegate other URNs to `base`; derives the PROTO interface and IS connections from the body node's `initializeOnly` field descriptors |
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

- `ProtoDeclarationResolver` seam — `install(base)` wraps any existing `ProtoDeclarationResolver` (default: `x3d::codec::localFileProtoResolver`). The returned resolver intercepts the `kExternalGeometryUrn` and returns a cached `ProtoDeclaration` whose body is an `ExternalGeometry` node with four IS-wired fields (`url`, `bboxCenter`, `bboxSize`, `contentType`). All other URL lists are forwarded to `base`. No new parameters on `parseDocument` — the existing resolver argument is the exact opt-in gap.

- `MeshBuildOptions::externalGeometryResolver` seam — `makeExternalGeometryResolver(bytesResolver)` returns a `std::function<PackedMesh(const X3DNode*, AssetResolver)>`. The extractor calls this function when it encounters a geometry node that is not a known core type. The closure `dynamic_cast`s to `ExternalGeometry`, reads `url`, sniffs the extension (`.stl` → binary STL; else returns empty `PackedMesh`), fetches bytes via the embedder-supplied `AssetResolver` with `AssetKind::ExternalGeometry`, calls `parseStlBinary`, caches the result keyed by URL, and returns the `PackedMesh`. An empty `PackedMesh` (`vertex_count == 0`) signals Pending/skip to the extractor.

- `fallbackNodeCreator()` hook — `install()` also registers `x3d::runtime::fallbackNodeCreator()` exactly once (`std::once_flag`) so that `deepClone` (called during EXTERNPROTO expansion) can instantiate `ExternalGeometry` nodes by type name. This is a process-global, sticky, set-once hook — intended for single-threaded setup time.

- `AssetResolver` / `AssetKind::ExternalGeometry` — the ext layer uses the same `AssetResolver` abstraction (from `runtime/extract/AssetResolver.hpp`) used for textures, requesting `AssetKind::ExternalGeometry`. Pending vs. ready vs. failed is signalled by `AssetResult`.

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

## Related

- [ADR-0001: External-Code Firewall](../decisions/0001-ext-firewall.md)
- [Architecture](../architecture.md)
- [Extract subsystem](extract.md)
- [GitHub Project](https://github.com/users/delta9000/projects/2)
