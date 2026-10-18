---
title: "Ext Firewall"
summary: Opt-in EXTERNPROTO extensions provide external geometry and physical room/device input without adding nonstandard nodes or transforms to core.
tags: [subsystem, ext-firewall, binary, stl, external-geometry, externproto, cave]
updated: 2026-10-18
related:
  - ../architecture.md
  - ../decisions/0001-ext-firewall.md
  - extract.md
  - ../decisions/0068-room-frame-sensor-composition.md
---

# Ext Firewall

## Purpose

The ext firewall contains every piece of code that is non-spec, binary-format, or third-party within a quarantined directory tree (`runtime/ext/`) that the spec-correct core must never include. The boundary problem it solves: the VR CAVE consumer needs typed binary geometry (`PackedMesh`) routed via an `ExternalGeometry` dialect node, but the generated bindings, golden layer, and conformance gate must remain ISO 19775-1-pure. The firewall makes that containment structural: a one-way dependency (ext may include core; core must never include ext), an opt-in CMake flag (`X3D_CPP_BUILD_EXT`, default `OFF`), and a resolver seam that the embedder opts into explicitly at parse time. X3D files that use `ExternalGeometry` declare it via a standard `<ExternProtoDeclare>` and degrade gracefully in parsers that do not install the ext resolver.

## Key files

| File / directory | Role |
|---|---|
| `runtime/ext/ExternalGeometry.hpp` | Hand-written `X3DNode` subclass — the `ExternalGeometry` dialect node; holds `url`, `bboxCenter`, `bboxSize`, `contentType`; `defaultContainerField()` is `"geometry"` |
| `runtime/ext/ExtResolver.hpp` | `install(base)` wraps a `ProtoDeclarationResolver`; declarations and clone factories belong to that resolver, with no process-global registration |
| `runtime/ext/CaveSensors.hpp` | Native `TrackedDeviceSensor` and `RoomFrameSensor`, with reflected fields and ordinary output events |
| `runtime/ext/CaveProtos.hpp` | Sensor declarations and the composed `RoomGroup` declaration: Transform, auxiliary RoomFrameSensor, and three ROUTEs |
| `runtime/ext/CaveInputSystem.hpp` | Owner-thread frame submission, sensor behavior, and readable RoomGroup placement warnings |
| `runtime/ext/protos/cave-fallbacks.x3d` | Portable inactive sensor PROTOs and identity RoomGroup fallback; distribute this file with your scene |
| `runtime/ext/ExternalGeometryResolver.hpp` | `makeExternalGeometryResolver(bytesResolver)` — the lazy materialization seam; produces a `std::function<PackedMesh(const X3DNode*, AssetResolver)>` for `MeshBuildOptions::externalGeometryResolver`; owns a url-keyed `PackedMesh` cache |
| `runtime/ext/codecs/StlReader.hpp` | Binary STL codec: `parseStlBinary(data, len)` and `parseStlBinary(vector)` — reads the 84 + N×50 byte layout, produces a non-indexed `PackedMesh` (Position + Normal, `Topology::Triangles`, `ccw=true`, `solid=true`) |
| `include/x3d/ext.hpp` | Opt-in embedder façade; re-exports `x3d::ext::ExternalGeometry`, `x3d::ext::install`, `x3d::ext::kExternalGeometryUrn`, `x3d::ext::makeExternalGeometryProto`, `x3d::ext::makeExternalGeometryResolver` |
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

With `X3D_CPP_BUILD_EXT=ON`, installation exports `x3d_cpp::ext` alongside
`x3d_cpp::sdk`. Link both targets and include `<x3d/ext.hpp>`; no source-checkout
include directories are needed. Headers install under `include/x3d_cpp/ext`,
and the portable fallback installs at `share/x3d_cpp/ext/cave-fallbacks.x3d`
under the default GNU install directories. Copy that file beside authored
scenes when distributing them. With the option off, neither ext headers nor
the ext target are installed.

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

- **`ProtoDeclarationResolver` seam** — `install(base)` wraps any existing `ProtoDeclarationResolver` (default: `x3d::codec::localFileProtoResolver`). The returned resolver intercepts the `kExternalGeometryUrn` and returns a cached `ProtoDeclaration` whose body is an `ExternalGeometry` node with four IS-wired fields (`url`, `bboxCenter`, `bboxSize`, `contentType`). All other URL lists are forwarded to `base`. No new parameters on `parseDocument` — the existing resolver argument is the exact opt-in gap.

- **`MeshBuildOptions::externalGeometryResolver` seam** — `makeExternalGeometryResolver(bytesResolver)` returns a `std::function<PackedMesh(const X3DNode*, AssetResolver)>`. The extractor calls this function when it encounters a geometry node that is not a known core type. The closure `dynamic_cast`s to `ExternalGeometry`, reads `url`, sniffs the extension (`.stl` → binary STL; else returns empty `PackedMesh`), fetches bytes via the embedder-supplied `AssetResolver` with `AssetKind::ExternalGeometry`, calls `parseStlBinary`, caches the result keyed by URL, and returns the `PackedMesh`. An empty `PackedMesh` (`vertex_count == 0`) signals Pending/skip to the extractor.

- **Declaration-local clone factory** — each native declaration supplies
  `ProtoDeclaration::createNode`. Expansion passes it through `CloneContext`;
  opting one resolver into extensions never changes another world's cloning.

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

## Physical room and device input

The CAVE extensions expose host snapshots as ordinary X3D events. They do not
own hardware, navigation, camera poses, wall projections, or input capture.
NavigationInfo remains a host mapping. There is no new viewpoint node or
RenderItem coordinate-space flag. See [ADR-0068](../decisions/0068-room-frame-sensor-composition.md).

### Node interfaces

Use these versioned identifiers in an EXTERNPROTO URL list, followed by a
portable fallback URL:

| Node | URN | Fallback URL relative to the scene |
|---|---|---|
| TrackedDeviceSensor | `urn:x3d-cpp-gen:ext:TrackedDeviceSensor:1` | `cave-fallbacks.x3d#TrackedDeviceSensor` |
| RoomFrameSensor | `urn:x3d-cpp-gen:ext:RoomFrameSensor:1` | `cave-fallbacks.x3d#RoomFrameSensor` |
| RoomGroup | `urn:x3d-cpp-gen:ext:RoomGroup:1` | `cave-fallbacks.x3d#RoomGroup` |

Both sensors have the following fields. OutputOnly entries list initial
readable native values, not authored defaults.

| Field | Type | Access | Default or initial value |
|---|---|---|---|
| metadata | SFNode | inputOutput | NULL |
| description | SFString | inputOutput | `""` |
| enabled | SFBool | inputOutput | TRUE |
| isActive | SFBool | outputOnly | FALSE |
| position_changed | SFVec3f | outputOnly | `0 0 0` |
| orientation_changed | SFRotation | outputOnly | `0 1 0 0` |

TrackedDeviceSensor adds these fields:

| Field | Type | Access | Default or initial value |
|---|---|---|---|
| device | SFString | initializeOnly | `"HEAD"` |
| roomPosition_changed | SFVec3f | outputOnly | `0 0 0` |
| roomOrientation_changed | SFRotation | outputOnly | `0 1 0 0` |
| buttons_changed | MFBool | outputOnly | `[]` |
| joystick_changed | SFVec2f | outputOnly | `0 0` |

RoomFrameSensor adds `scale_changed`, an outputOnly SFVec3f initially `1 1 1`.

RoomGroup exposes standard grouping fields through its Transform primary:

| Field | Type | Access | Default |
|---|---|---|---|
| metadata | SFNode | inputOutput | NULL |
| children | MFNode | inputOutput | `[]` |
| addChildren | MFNode | inputOnly | — |
| removeChildren | MFNode | inputOnly | — |
| visible | SFBool | inputOutput | TRUE |
| bboxDisplay | SFBool | inputOutput | FALSE |
| bboxCenter | SFVec3f | initializeOnly | `0 0 0` |
| bboxSize | SFVec3f | initializeOnly | `-1 -1 -1` |

### Coordinate and lifecycle contract

Room coordinates use metres, floor-centre origin, +Y up, and front wall -Z.
The host calibrates device axes; the wand points along its local -Z axis.
The submitted matrix maps world points into this room frame. Device poses
map device-local coordinates into room coordinates. The ordinary device
position/orientation outputs describe device-to-world, while the room outputs
describe device-to-room. Sensor placement in the scene does not change either
output frame. Device sensors report pose without a scale field.

Native extension values and RoomGroup contents use the runtime's normalized
metre-based coordinates. Convert host tracking millimetres before submission.
Apply authored unit conversions to assets before composing physical room
content; the system does not infer units or calibration from device names.

Instantiate one TrackedDeviceSensor per logical device. The host supplies
`"HEAD"`, `"WAND"`, or additional application-specific names. Missing names stay
inactive; the first matching entry wins if the host submits duplicate names.
HEAD always reports empty buttons and a zero joystick. For controllers, button
index 0 is the primary trigger; document further indices in the host. Joystick
axes are +X right and +Y forward, clamped to [-1, 1].

Both sensors require `enabled=TRUE` and a valid room frame. Device sensors also
require a matching valid device pose and finite joystick values. On activation
or recovery, sensors publish their complete current state, even if pose values
match those retained before loss. Later ticks publish changes only. Tracking
loss or disabling sets `isActive=FALSE`, releases each previously exposed
button, and zeros the joystick. Pose values remain readable but emit no new
events while inactive. Losing a device does not deactivate RoomFrameSensor.

RoomFrameSensor reports translation, rotation, and uniform scale of
`inverse(roomFromWorld)`. The matrix must be finite, affine, invertible, and
have positive uniform scale. Shear, reflection, non-uniform scale, perspective,
or a non-finite inverse makes it inactive. Validation uses relative 1e-5
tolerances for equal column lengths and orthogonality, and 1e-6 for affine
bottom-row checks. An inactive RoomFrameSensor retains its last transform;
RoomGroup consequently retains its last placement. The host must stop or hide
physical-room rendering when its room frame is unavailable.

Buttons are sampled state, not queued transitions. A press and release between
submissions can disappear. No lossless click stream is promised in v1.

### Host setup and per-frame submission

Install the parse resolver before loading a scene, build its ROUTEs and scene
graph as usual, and install the input system once per execution context:

```cpp
#include <x3d/sdk.hpp>
#include <x3d/ext.hpp>

// Pass x3d::ext::install(yourProtoResolver) to the scene's parse call.
// After the session has built the graph and ROUTEs:
auto caveInput = x3d::ext::installCaveInput(session.scene(), session.context());
```

The returned system enrolls native sensors, including auxiliary PROTO peers.
The existing System attach/detach lifecycle enrolls or removes later Inline
content. The scene must outlive the system. Calls run on the context's owner
thread, with no concurrent submission or teardown.

For each application frame, construct one complete snapshot and submit it
before the session's tick:

```cpp
x3d::ext::RoomFrame input;
input.valid = true; // Your calibration/navigation validity policy.
input.roomFromWorld = toX3dMatrix(frame.roomFromWorld);
input.devices = {
    {"HEAD", headValid, {headPosition, headOrientation}, {}, {0, 0}},
    {"WAND", wandValid, {wandPosition, wandOrientation}, buttons, joystick}
};
caveInput->submit(std::move(input));
session.tick(frame.time);
auto delta = session.delta();
for (const auto &warning : caveInput->warnings()) {
    report(warning.message);
}
```

`toX3dMatrix` and the pose conversions above belong to the host. Mat4 uses
column-major storage, `m[column*4 + row]`. Use the logical head pose used for
the cameras, including any deliberate desktop simulation. Device validity,
tracking freshness, and calibration remain host responsibilities. The latest
submission persists until replaced, so submit invalid devices explicitly on
loss or app deactivation. Missing room support is the default inactive state;
a desktop host can explicitly supply a synthetic floor frame.

The system queues outputs with `postOutputEvent`, without first mutating their
backing fields. X3DExecutionContext drains the cascade and re-evaluates systems
within the same tick. RoomGroup's three ROUTEs therefore update its Transform
before extraction or picking after that tick. Submit once per application
frame, using the same navigation snapshot for input, world-space rays, and
rendering; do not submit once per wall, eye, or GPU. Follow the runtime's
one-output-production-per-field-per-timestamp rule: feedback that changes an
already-produced output in that timestamp is not a second production.

### RoomGroup placement and extraction

Place RoomGroup at the world root or beneath structural groups that introduce
no transform. Put room-local transforms inside RoomGroup. Transform-bearing
ancestors count even when their current matrix is identity. Billboard,
ScreenGroup, and Layout ancestors also violate this contract. A nested
RoomGroup has a transforming ancestor and is invalid.

`CaveInputSystem::warnings()` reports invalid placements and rejected input
after a tick. Placement warnings contain the affected primary node and a
message. Input warnings identify the enabled device sensor and its device name;
a null node identifies a rejected room frame. Reasons distinguish shear,
non-uniform scale, non-finite or non-affine matrices, singularity, reflection,
invalid host flags, unknown devices, invalid poses, and non-finite joysticks.
Disabled sensors do not produce device warnings. Room input is diagnosed once
per snapshot evaluation when at least one CAVE sensor is enrolled, even when no
room sensor is enabled. A shared group warns if any path has a transforming
ancestor. Placement diagnostics are cached across updates and cascade passes;
TRS animation, viewer motion, and LOD selection do not rescan authored ancestry.
Structural SFNode/MFNode writes advance `sceneTopologyRevision()` automatically.
Attach/detach, Inline/GeoLOD splices, and `buildRoutes()` also invalidate the
placement cache. After direct root or private ROUTE collection edits, call
`context.markSceneTopologyChanged()` (or rebuild the ROUTEs). Recalculation runs
on the next system update, including another pass within the same tick. With
no enrolled CAVE sensors, updates return immediately; with no RoomGroup anchors,
no ancestry walk runs. Input validity is evaluated on each required update and
composed with cached placement warnings. Detach drops cached warning pointers
before removed nodes can be released. Copy messages before the next update;
node pointers are borrowed and must not survive scene topology edits. The
system diagnoses these placements; it does not cancel their ancestors.

For a child transform C, extraction emits the ordinary world transform:

```text
worldTransform = inverse(roomFromWorld) * C
roomFromWorld * worldTransform = C
```

Room motion changes existing placements through `updatedTransform`, preserving
mesh and placement identity. Picking, bounds, lighting, and spatial sensors
continue consuming standard world transforms. There is no overlay behavior or
special depth rule.

Mat4 and RenderItem.worldTransform store **32-bit floats**. Room content far
from the world origin can jitter because the host cancels two large float
transforms. Same-tick cancellation holds within float round-off, not exact
arithmetic. This extension does not change core scalar types or add origin
rebasing.

### Portable fallback

Copy `runtime/ext/protos/cave-fallbacks.x3d` beside your scene and include the
matching fragment URL after the URN. Without the ext resolver, the standard
file resolver loads that PROTO. Sensor fallbacks use standard Group bodies
with exposed interface fields and produce no tracking events. RoomGroup's
fallback is an identity Transform that preserves its children. Physical
anchoring is unavailable in that mode; mouse-to-wand emulation is not implicit.

## How it is tested

All ext tests are compiled and registered only inside the `if(X3D_CPP_BUILD_EXT)` block in `cmake/x3d/ext.cmake` and run only when `-DX3D_CPP_BUILD_EXT=ON` is set. They therefore do not appear in, and do not affect, the default `ctest` run.

The `x3d_ext_cave_tests` executable contains these independently registered
CTest cases. Their source is `runtime/ext/tests/cave_extensions_test.cpp`.
These describe assertions, not a claim that the cases have been executed.

| CTest name | Assertions |
|---|---|
| `x3d_ext_cave_proto_routes` | Native output passes through an exposed EXTERNPROTO and renamed enclosing PROTO output to a standard Transform; room-frame output routes too; native instances survive XML round-trip. |
| `x3d_ext_cave_room_same_tick` | Moving translation, rotation, and uniform scale cancel after exactly one tick per submission, in both the first snapshot and later transform deltas; a world ray derived from the same room snapshot hits the fixed room child. |
| `x3d_ext_cave_tracking_lifecycle` | Loss releases buttons and zeros the joystick while preserving pose; recovery republishes state through a ROUTE; disabled and unknown devices remain inactive; disabled room sensors, re-enabling, and detach/attach work. |
| `x3d_ext_cave_invalid_room` | Shear, non-uniform or zero scale, reflection, perspective, NaN, infinity, and host-invalid frames deactivate the sensor; valid frames recover, and positive uniform scale produces reciprocal scale. |
| `x3d_ext_cave_room_placement` | Structural groups are accepted; identity Transform and Billboard ancestors warn; enclosing PROTOs, child edits, and shared placement paths remain diagnosable. |
| `x3d_ext_cave_recursive_resolver` | Native sensor declarations resolve inside an external PROTO loaded through an Inline; only document URLs reach the asset backend. |
| `x3d_ext_cave_placement_cache` | Counts ancestry visits across repeated updates and TRS animation; checks child replacement, reparenting, SFNode edits, same-tick cascade edits, route replacement, detach, and device recovery with cached placement warnings. |
| `x3d_ext_cave_placement_idle` | An idle tick without CAVE sensors and an update without RoomGroup anchors perform zero ancestry visits. |
| `x3d_ext_cave_placement_inline` | Deferred Inline/enclosing-PROTO RoomGroups enroll, unload drops borrowed warnings and releases nodes, and reload diagnoses the new placement. |
| `x3d_ext_cave_placement_geolod` | GeoLOD child-tile arrival enrolls RoomGroups; retirement releases old anchors and warnings; returning to the tile rebuilds warnings. |
| `x3d_ext_cave_input_diagnostics` | Rejection messages identify room or device input, distinguish transform/sample failures, and clear on recovery. |
| `x3d_ext_cave_fallback` | The normal file resolver reads fallback fragment URLs without ext interception; no native sensors appear, output ROUTEs remain legal, children extract at identity room placement, and fallback XML round-trip loads. |

`x3d_install_embed_smoke` also builds an ext consumer when the producing build
has ext enabled. It resolves a native declaration and reads the installed
fallback before and after relocating the prefix. These additions are unrun.

- **`stl_reader_test`** (`runtime/ext/tests/stl_reader_test.cpp`) — synthesizes a two-triangle binary STL in-memory and asserts: `vertex_count == 6`, `index_count == 0`, Position and Normal attributes present with correct per-vertex values, AABB computed correctly (`min=(0,0,0)`, `max=(3,1,0)`), and three malformed-input cases (buffer too short, truncated triangle data, null pointer) all return empty `PackedMesh` without crashing.

- **`external_geometry_node_test`** (`runtime/ext/tests/external_geometry_node_test.cpp`) — unit tests the `ExternalGeometry` node class: default field values, typed accessors, `nodeTypeName() == "ExternalGeometry"`, `defaultContainerField() == "geometry"`, reflection `FieldTable` correctness (field names, types, access modes), get/set thunks, `accept(NodeVisitor&)`, and DEF/USE inheritance from `X3DNode`.

- **`external_geometry_roundtrip_test`** (`runtime/ext/tests/external_geometry_roundtrip_test.cpp`) — end-to-end parse + round-trip: (1) parses a doc with `<ExternProtoDeclare>` + `<ProtoInstance name='ExternalGeometry'>` using `install()`, confirms zero proto warnings and `url == ["chair.stl"]` on the expanded node; (2) verifies `expandedSources` maps the primary node back to the `ProtoInstance`; (3) re-serializes to XML and asserts `<ExternProtoDeclare>` + `<ProtoInstance>` round-trip with no raw `<ExternalGeometry>` element emitted; (4) verifies graceful degradation — parsing the same doc without `install()` records `ProtoWarning::Kind::UnresolvedExtern` and produces a clean (non-crashing) round-trip; (5) verifies URN delegation — non-matching URNs reach the stub base resolver, matching URNs are intercepted without forwarding.

- **`external_geometry_e2e_test`** (`runtime/ext/tests/external_geometry_e2e_test.cpp`) — Round 2 lazy materialization proof: builds a `Group → Shape → ExternalGeometry(url="m.stl")` scene programmatically, wires `makeExternalGeometryResolver` with a counting stub that returns a known two-triangle binary STL, runs `SceneExtractor::fullSnapshot()`, and asserts: (1) PACKED — one `RenderItem` with `geometry_ext.kind == Packed`, `vertex_count == 6`, and the known v0 position `(1,2,3)`; (2) CACHE — a second extract reuses the cached `PackedMesh` and the stub byte-provider was called exactly once; (3) PENDING — an unresolvable URL results in no `RenderItem` emitted, no crash.

- Two further ext-gated tests round out the suite: **`x3d_stl_write_test`** (binary-STL write path) and **`x3d_extract_oracle_test`** (differential extraction oracle), both in `runtime/ext/tests/`.

## Related specs and ADRs

- [ADR-0001: External-Code Firewall](../decisions/0001-ext-firewall.md) — the rationale, four-layer firewall design, trade-offs, and CMake target structure
- [Architecture](../architecture.md)
- [Extract subsystem](extract.md) — the extraction pipeline that calls the `externalGeometryResolver` seam
- Dated design spec: `docs/superpowers/specs/2026-06-19-binary-geometry-extension-design.md` — consolidated design; cites ISO 19775-1 §4.4.5.1 and the EXTERNPROTO extension point
- Dated spec: `docs/superpowers/specs/2026-06-18-binary-mesh-texture-abstractions.md` — earlier abstraction design that preceded the firewall split
- Ingestion roadmap: the [GitHub Project](https://github.com/users/delta9000/projects/2) — deferred items including the portable `.x3d` fallback baker (second slot in the EXTERNPROTO url list) and future codecs (PLY, OBJ single-mesh, glTF per-mesh)
