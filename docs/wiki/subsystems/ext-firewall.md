---
title: "Ext Firewall"
summary: The ext firewall isolates binary-mesh, STL, ExternalGeometry, and physical room/device input code behind a quarantined directory, a one-way dependency, and an opt-in CMake flag, so core stays free of nonstandard nodes and transforms.
tags: [subsystem, ext-firewall, binary, stl, external-geometry, externproto, cave]
updated: 2026-11-05
related:
  - ../architecture.md
  - ../decisions/0001-ext-firewall.md
  - extract.md
  - ../decisions/0068-room-frame-sensor-composition.md
---

# Ext Firewall

## Purpose

The extension firewall isolates opt-in foreign geometry under `runtime/ext/`. Core headers do not include that directory. Optional resource backends live separately under `runtime/io/` and other subsystem directories. The boundary problem it solves: the VR CAVE consumer needs typed binary geometry (`PackedMesh`) routed via an `ExternalGeometry` dialect node, but the generated bindings, golden layer, and conformance gate must remain ISO 19775-1-pure. The firewall makes that containment structural: a one-way dependency (ext may include core; core must never include ext), an opt-in CMake flag (`X3D_CPP_BUILD_EXT`, default `OFF`), and a resolver seam that the embedder opts into explicitly at parse time. X3D files that use `ExternalGeometry` declare it via a standard `<ExternProtoDeclare>` and degrade gracefully in parsers that do not install the ext resolver. The same layer provides nonstandard physical room and tracked-device sensors for CAVE hosts (see [Physical room and device input](#physical-room-and-device-input)); they also enter through EXTERNPROTO URNs with portable fallbacks.

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
The submitted matrix maps world points into the room frame, and device poses
map device-local coordinates into room coordinates. A device sensor's
`position_changed`/`orientation_changed` describe device-to-world in
scene-root coordinates; the `room*` outputs describe device-to-room. Sensor
placement in the scene does not change either output frame; unlike
ProximitySensor, the world outputs are not local to the sensor's parent.
Inside a RoomGroup, route `roomPosition_changed` and `roomOrientation_changed`:
RoomGroup's local frame is the room frame, so routing `position_changed` into a
Transform inside a RoomGroup applies the room transform twice. Device sensors
report pose without a scale field.

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
or a non-finite inverse makes it inactive. Validation uses relative 1e-3
tolerances for equal column lengths and orthogonality, and 1e-6 for affine
bottom-row checks. Within tolerance the rotation is re-orthonormalized and the
scale is the mean column length, so float drift in a host's accumulated
navigation matrix does not freeze the room; device poses and RoomGroup
placement both use that cleaned similarity. An inactive RoomFrameSensor retains
its last transform, so RoomGroup retains its last placement. The host must stop
or hide physical-room rendering when its room frame is unavailable.

Buttons are sampled state, not queued transitions. A press and release between
submissions can disappear; there is no lossless click stream.

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
The System attach/detach lifecycle enrolls or removes later Inline content.
Install it after the session's standard systems (RuntimeSession and
`attachStandardRuntime` add `InlineRuntimeSystem` there), as the snippet does.
Systems update in installation order, so a CAVE system installed before the
Inline loader sees sensors from a freshly loaded Inline or GeoLOD tile only on
the next tick. The scene must outlive the system. Calls run on the context's
owner thread, with no concurrent submission or teardown.

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
within the same tick, so RoomGroup's three ROUTEs update its Transform before
extraction or picking after that tick. Submit once per application frame, using
the same navigation snapshot for input, world-space rays, and rendering; do not
submit once per wall, eye, or GPU. The runtime's
one-output-production-per-field-per-timestamp rule applies: feedback that
changes an already-produced output in that timestamp is not a second
production.

### RoomGroup placement and extraction

Place RoomGroup at the world root or beneath structural groups that introduce
no transform. Put room-local transforms inside RoomGroup. Transform-bearing
ancestors count even when their current matrix is identity. Billboard,
ScreenGroup, and Layout ancestors also violate this contract. A nested
RoomGroup has a transforming ancestor and is invalid. Do not place RoomGroup
inside a LayerSet layer: each Layer binds its own Viewpoint and NavigationInfo,
but the room frame tracks root-scene navigation, so such placements warn too.

Ancestry follows only fields that place children in the rendered graph:
`children`, `LOD.level` and `Switch.choice` (X3D 3.x names), `GeoLOD.rootNode`,
`LayerSet.layers`, `HAnimHumanoid.skeleton`/`skin`, and `CADFace.shape`.
Reference and non-rendered slots, such as `TransformSensor.targetObject`,
`Collision.proxy`, metadata, and geometry/appearance fields, do not make a
RoomGroup transformed. RoomGroups are recognized by their private auxiliary
sensor type, not by its `RoomGroupFrame` DEF, so author content reusing that
name cannot create a false anchor.

`CaveInputSystem::warnings()` reports invalid placements and rejected input
after a tick. Placement warnings contain the affected primary node and a
message. Input warnings identify the enabled device sensor and its device name;
a null node identifies a rejected room frame. Reasons distinguish shear,
non-uniform scale, non-finite or non-affine matrices, singularity, reflection,
invalid host flags, unknown devices, invalid poses, and non-finite joysticks.
Disabled sensors do not produce device warnings. Room input is diagnosed once
per snapshot evaluation when at least one CAVE sensor is enrolled, even when no
room sensor is enabled. A shared group warns if any path has a transforming
ancestor.

Placement diagnostics are cached across updates and cascade passes; TRS
animation, viewer motion, and LOD selection do not rescan authored ancestry.
Structural SFNode/MFNode writes advance `sceneTopologyRevision()`.
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
consume standard world transforms. There is no overlay behavior or special
depth rule.

Mat4 and `RenderItem::worldTransform` store 32-bit floats. Room content far
from the world origin can jitter because the host cancels two large float
transforms; same-tick cancellation holds within float round-off. This extension
does not change core scalar types or add origin rebasing.

### Portable fallback

Copy `runtime/ext/protos/cave-fallbacks.x3d` beside your scene (or use the
installed copy) and include the matching fragment URL after the URN. Without
the ext resolver, the standard file resolver loads that PROTO. Sensor fallbacks
use standard Group bodies with exposed interface fields and produce no tracking
events. RoomGroup's fallback is an identity Transform that preserves its
children. Physical anchoring is unavailable in that mode; mouse-to-wand
emulation is not implicit.

The fallback file is an X3D 4.0 document: RoomGroup's fallback connects
`visible` and `bboxDisplay`, which are X3D 4 Transform fields. X3D 3.x browsers
may reject the file or that PROTO; scenes that must degrade in 3.x browsers
need their own fallback without those two connections.

## How it is tested

Ext tests are registered only inside the `if(X3D_CPP_BUILD_EXT)` block in
`cmake/x3d/ext.cmake`, so they are absent from the default `ctest` run. The
`ext` preset (`build-ext/`, warnings as errors) enables the option in its own
tree: `mise run build-ext` builds the `x3d_ext_tests` aggregate and runs
`ctest --preset ext`. `mise run ci` depends on it, and the GitHub `ext` job runs
the same steps.

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

- [ADR-0001: External-Code Firewall](../decisions/0001-ext-firewall.md)
- [ADR-0068: Room-frame sensor composition](../decisions/0068-room-frame-sensor-composition.md)
- [Architecture](../architecture.md)
- [Extract subsystem](extract.md)
- [GitHub Project](https://github.com/users/delta9000/projects/2)
