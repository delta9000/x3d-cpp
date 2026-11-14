---
title: "Ext Firewall"
summary: The ext firewall isolates binary-mesh, STL, ExternalGeometry, and physical room/device input code behind a quarantined directory, a one-way dependency, and an opt-in CMake flag, so core stays free of nonstandard nodes and transforms.
tags: [subsystem, ext-firewall, binary, stl, external-geometry, externproto, cave]
updated: 2026-11-05
related:
  - ../../explanation/architecture.md
  - ../../explanation/decisions/0001-ext-firewall.md
  - extract.md
  - ../../explanation/decisions/0068-room-frame-sensor-composition.md
---

# Ext Firewall

## Purpose

- The extension firewall isolates opt-in foreign geometry under
  `runtime/ext/`.
- Core headers do not include that directory.
- Optional resource backends live separately under `runtime/io/` and other
  subsystem directories.
- The boundary problem it solves: the VR CAVE consumer needs typed binary
  geometry (`PackedMesh`) routed via an `ExternalGeometry` dialect node, but
  the generated bindings, golden layer, and conformance gate must remain ISO
  19775-1-pure.
- The firewall makes that containment structural: a one-way dependency (ext
  may include core; core must never include ext), an opt-in CMake flag
  (`X3D_CPP_BUILD_EXT`, default `OFF`), and a resolver seam that the embedder
  opts into explicitly at parse time.
- X3D files that use `ExternalGeometry` declare it via a standard
  `<ExternProtoDeclare>` and degrade gracefully in parsers that do not install
  the ext resolver.
- The same layer provides nonstandard physical room and tracked-device sensors
  for CAVE hosts (see [Physical room and device
  input](#physical-room-and-device-input)); they also enter through
  EXTERNPROTO URNs with portable fallbacks.

## Key files

| File / directory | Role |
|---|---|
| `runtime/ext/ExternalGeometry.hpp` | Hand-written `X3DNode` subclass — the `ExternalGeometry` dialect node; holds `url`, `bboxCenter`, `bboxSize`, `contentType`; appends inherited descriptors from `X3DNode::fields()`; `defaultContainerField()` is `"geometry"` |
| `runtime/ext/ExtResolver.hpp` | `install(base)` factory — wraps a `ProtoDeclarationResolver` to intercept the ExternalGeometry and versioned CAVE URNs and delegate other URLs to `base`. Each returned resolver owns fresh declarations and their clone factories; there is no process-global registration. The ExternalGeometry interface and IS connections come from the body node's `initializeOnly` field descriptors |
| `runtime/ext/CaveSensors.hpp` | Native `TrackedDeviceSensor` and `RoomFrameSensor`, with reflected fields and ordinary output events |
| `runtime/ext/CaveProtos.hpp` | Sensor declarations and the composed `RoomGroup` declaration: Transform, auxiliary RoomFrameSensor, and three ROUTEs |
| `runtime/ext/CaveInputSystem.hpp` | Owner-thread frame submission, sensor behavior, and RoomGroup placement and input warnings |
| `runtime/ext/protos/cave-fallbacks.x3d` | Portable inactive sensor PROTOs and identity RoomGroup fallback; distribute this file with your scene |
| `runtime/ext/ExternalGeometryResolver.hpp` | `makeExternalGeometryResolver(bytesResolver)` — the lazy materialization seam; produces a `std::function<PackedMesh(const X3DNode*, AssetResolver)>` for `MeshBuildOptions::externalGeometryResolver`; owns a url-keyed `PackedMesh` cache |
| `runtime/ext/codecs/StlReader.hpp` | Binary STL codec: `parseStlBinary(data, len)` and `parseStlBinary(vector)` — reads the 84 + N×50 byte layout, produces a non-indexed `PackedMesh` (Position + Normal, `Topology::Triangles`, `ccw=true`, `solid=true`) |
| `include/x3d/ext.hpp` | Opt-in embedder façade; re-exports `x3d::ext::ExternalGeometry`, `x3d::ext::install`, `x3d::ext::kExternalGeometryUrn`, `x3d::ext::makeExternalGeometryProto`, `x3d::ext::makeExternalGeometryResolver`, and the CAVE surface (`TrackedDeviceSensor`, `RoomFrameSensor`, `RoomFrame`, `DevicePose`, `DeviceSample`, `RoomGroupWarning`, `CaveInputSystem`, `installCaveInput`, the three CAVE URN constants and declaration factories) |
| `runtime/ext/tests/` | Ext-gated geometry and CAVE tests (see "How it is tested") |

## Interfaces and seams

### Exposed interface

The embedder surface is exported from `include/x3d/ext.hpp` under `namespace x3d::ext`:

```cpp
// 1. Parse-time: install the EXTERNPROTO interceptor.
//    Create per-loader/world at setup, before parseDocument/parseFile.
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

### Installed extension layer

- With `X3D_CPP_BUILD_EXT=ON`, installation exports `x3d_cpp::ext` alongside
  `x3d_cpp::sdk`.
- Link both targets and include `<x3d/ext.hpp>`; no source-checkout include
  directories are needed.
- Headers install under `include/x3d_cpp/ext`, and the portable fallback
  installs at `share/x3d_cpp/ext/cave-fallbacks.x3d` under the default GNU
  install directories.
- Copy that file beside authored scenes when distributing them.
- With the option off, neither ext headers nor the ext target are installed.

For a confined recursive loader, decorate the factory callback:

```cpp
#include <AssetDocumentResolvers.hpp>
auto loaders = x3d::codec::assetResolversFrom(
    assets, x3d::codec::Encoding::Unknown,
    [](auto base) { return x3d::ext::install(std::move(base)); });
auto doc = x3d::sdk::parseDocument(text, encoding, baseDirectory,
                                 loaders.proto, loaders.inlineScene);
```

The decorator applies inside Inline and external PROTO documents as well as
at the top level. It delegates unrecognized URLs to the same asset policy.

### Seam points

- `ProtoDeclarationResolver` seam — `install(base)` wraps any existing `ProtoDeclarationResolver` (default: `x3d::codec::localFileProtoResolver`). The returned resolver intercepts `kExternalGeometryUrn` and returns its resolver-local `ProtoDeclaration` whose body is an `ExternalGeometry` node with four IS-wired fields (`url`, `bboxCenter`, `bboxSize`, `contentType`); it intercepts the three versioned CAVE URNs the same way. All other URL lists are forwarded to `base`. No new parameters on `parseDocument` — the existing resolver argument is the exact opt-in gap.

- `MeshBuildOptions::externalGeometryResolver` seam — `makeExternalGeometryResolver(bytesResolver)` returns a `std::function<PackedMesh(const X3DNode*, AssetResolver)>`. The extractor calls this function when it encounters a geometry node that is not a known core type. The closure `dynamic_cast`s to `ExternalGeometry`, reads `url`, sniffs the extension (`.stl` → binary STL; else returns empty `PackedMesh`), fetches bytes via the embedder-supplied `AssetResolver` with `AssetKind::ExternalGeometry`, calls `parseStlBinary`, caches the result keyed by URL, and returns the `PackedMesh`. An empty `PackedMesh` (`vertex_count == 0`) signals Pending/skip to the extractor.

- Declaration-local clone factory — each native declaration supplies `ProtoDeclaration::createNode`, and expansion passes it through `CloneContext` so `deepClone` can instantiate the native node by type name. Opting one resolver into extensions never changes another world's cloning.

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

## Physical room and device input

The nonstandard physical-room and tracked-device sensors that live in this
layer are documented in [Room and device input](room-and-device-input.md).

## How it is tested

- Ext tests are registered only inside the `if(X3D_CPP_BUILD_EXT)` block in
  `cmake/x3d/ext.cmake`, so they are absent from the default `ctest` run.
- The `ext` preset (`build-ext/`, warnings as errors) enables the option in
  its own tree: `mise run build-ext` builds the `x3d_ext_tests` aggregate and
  runs `ctest --preset ext`.
- `mise run ci` depends on it, and the GitHub `ext` job runs the same steps.

- `x3d_ext_stl_reader` (`runtime/ext/tests/stl_reader_test.cpp`) — binary STL layout, attributes, AABB, and malformed-input handling.
- `x3d_ext_external_geometry_node` (`runtime/ext/tests/external_geometry_node_test.cpp`) — `ExternalGeometry` defaults, accessors, reflection table, and DEF/USE inheritance.
- `x3d_ext_external_geometry_roundtrip` (`runtime/ext/tests/external_geometry_roundtrip_test.cpp`) — EXTERNPROTO expansion with `install()`, `expandedSources`, XML round-trip without a raw `<ExternalGeometry>` element, graceful degradation without `install()`, and URN delegation.
- `x3d_ext_external_geometry_e2e` (`runtime/ext/tests/external_geometry_e2e_test.cpp`) — lazy materialization to a packed `RenderItem`, URL cache reuse, and pending URLs.
- `x3d_stl_write_test` and `x3d_extract_oracle_test` (`tools/x3d-cli/`) — binary STL writing and the differential extraction oracle.
- `x3d_ext_cave_tests` (`runtime/ext/tests/cave_extensions_test.cpp`) registers one CTest case per behavior:

| CTest name | Assertions |
|---|---|
| `x3d_ext_cave_proto_routes` | Native output passes through an exposed EXTERNPROTO and renamed enclosing PROTO output to a standard Transform; room-frame output routes too; native instances survive XML round-trip. |
| `x3d_ext_cave_room_same_tick` | Moving translation, rotation, and uniform scale cancel after one tick per submission, in the first snapshot and in later transform deltas; a world ray from the same room snapshot hits the fixed room child. |
| `x3d_ext_cave_tracking_lifecycle` | Loss releases buttons and zeros the joystick while preserving pose; recovery republishes state through a ROUTE; disabled and unknown devices stay inactive; disabled room sensors, re-enabling, and detach/attach work. |
| `x3d_ext_cave_invalid_room` | Shear, non-uniform or zero scale, reflection, perspective, NaN, infinity, and host-invalid frames deactivate the sensor; valid frames recover, and positive uniform scale produces reciprocal scale. |
| `x3d_ext_cave_room_placement` | Structural groups are accepted; identity Transform and Billboard ancestors warn; enclosing PROTOs, child edits, and shared placement paths remain diagnosable. |
| `x3d_ext_cave_room_rotation_order` | With the room rotated about X and the device about Y, world orientation and position match `worldFromRoom * roomFromDevice`. |
| `x3d_ext_cave_room_drift` | A float-accumulated navigation matrix with skew and column-length error beyond 1e-5 stays active and places room children within float tolerance; 5e-3 shear and 0.5% non-uniform scale are rejected. |
| `x3d_ext_cave_placement_fields` | `TransformSensor.targetObject` references, a transformed `Collision.proxy`, and an author sensor reusing the `RoomGroupFrame` DEF do not warn; a `CADFace.shape` Transform and a LayerSet layer do. |
| `x3d_ext_cave_placement_inline_same_tick` | With the Inline loader installed before CAVE input, a sensor in a freshly loaded Inline activates in its load tick. |
| `x3d_ext_cave_recursive_resolver` | Native sensor declarations resolve inside an external PROTO loaded through an Inline; only document URLs reach the asset backend. |
| `x3d_ext_cave_placement_cache` | Ancestry visits across repeated updates and TRS animation; child replacement, reparenting, SFNode `Collision.proxy` edits, same-tick cascade edits, route replacement, detach, and device recovery with cached placement warnings. |
| `x3d_ext_cave_placement_idle` | An idle tick without CAVE sensors and an update without RoomGroup anchors perform zero ancestry visits. |
| `x3d_ext_cave_placement_inline` | Deferred Inline and enclosing-PROTO RoomGroups enroll, unload drops borrowed warnings and releases nodes, and reload diagnoses the new placement. |
| `x3d_ext_cave_placement_geolod` | GeoLOD child-tile arrival enrolls RoomGroups; retirement releases old anchors and warnings; returning to the tile rebuilds warnings. |
| `x3d_ext_cave_input_diagnostics` | Rejection messages identify room or device input, distinguish transform and sample failures, and clear on recovery. |
| `x3d_ext_cave_fallback` | The file resolver reads fallback fragment URLs without ext interception; no native sensors appear, output ROUTEs remain legal, children extract at identity room placement, and the fallback XML round-trips. |

`x3d_install_embed_smoke` also builds an ext consumer when the producing build
has ext enabled. It resolves a native declaration and reads the installed
fallback before and after relocating the prefix. It is outside the `ext`
preset's test filter, so it runs in a full ext-enabled build
(`ctest --test-dir build-ext`) rather than in the ext gate.

## Related

- [ADR-0001: External-Code Firewall](../../explanation/decisions/0001-ext-firewall.md)
- [ADR-0068: Room-frame sensor composition](../../explanation/decisions/0068-room-frame-sensor-composition.md)
- [Architecture](../../explanation/architecture.md)
- [Extract subsystem](extract.md)
- [GitHub Project](https://github.com/users/delta9000/projects/2)
