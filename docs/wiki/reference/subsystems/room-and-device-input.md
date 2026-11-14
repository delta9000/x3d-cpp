---
title: "Room and device input"
summary: "The opt-in physical-room and tracked-device sensor nodes: interfaces, coordinate and lifecycle contract, host submission, RoomGroup placement and the portable fallback."
tags: [subsystem, ext, sensors, tracking]
updated: 2026-11-14
related:
  - ext-firewall.md
  - ../../explanation/decisions/0068-room-frame-sensor-composition.md
---

# Room and device input

These nodes are part of the opt-in [extension layer](ext-firewall.md). Room
anchoring is decided in
[ADR-0068](../../explanation/decisions/0068-room-frame-sensor-composition.md).

The CAVE extensions expose host snapshots as ordinary X3D events. They do not
own hardware, navigation, camera poses, wall projections, or input capture.
NavigationInfo remains a host mapping. There is no new viewpoint node or
RenderItem coordinate-space flag. See [ADR-0068](../../explanation/decisions/0068-room-frame-sensor-composition.md).

## Node interfaces

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

## Coordinate and lifecycle contract

- Room coordinates use metres, floor-centre origin, +Y up, and front wall -Z.
  The host calibrates device axes; the wand points along its local -Z axis.
- The submitted matrix maps world points into the room frame, and device poses
  map device-local coordinates into room coordinates.
- A device sensor's `position_changed`/`orientation_changed` describe
  device-to-world in scene-root coordinates; the `room*` outputs describe
  device-to-room.
- Sensor placement in the scene does not change either output frame; unlike
  ProximitySensor, the world outputs are not local to the sensor's parent.
- Inside a RoomGroup, route `roomPosition_changed` and
  `roomOrientation_changed`: RoomGroup's local frame is the room frame, so
  routing `position_changed` into a Transform inside a RoomGroup applies the
  room transform twice.
- Device sensors report pose without a scale field.

Native extension values and RoomGroup contents use the runtime's normalized
metre-based coordinates. Convert host tracking millimetres before submission.
Apply authored unit conversions to assets before composing physical room
content; the system does not infer units or calibration from device names.

- Instantiate one TrackedDeviceSensor per logical device.
- The host supplies `"HEAD"`, `"WAND"`, or additional application-specific
  names.
- Missing names stay inactive; the first matching entry wins if the host
  submits duplicate names.
- HEAD always reports empty buttons and a zero joystick.
- For controllers, button index 0 is the primary trigger; document further
  indices in the host.
- Joystick axes are +X right and +Y forward, clamped to [-1, 1].
- Both sensors require `enabled=TRUE` and a valid room frame.
- Device sensors also require a matching valid device pose and finite joystick
  values.
- On activation or recovery, sensors publish their complete current state,
  even if pose values match those retained before loss.
- Later ticks publish changes only.
- Tracking loss or disabling sets `isActive=FALSE`, releases each previously
  exposed button, and zeros the joystick.
- Pose values remain readable but emit no new events while inactive.
- Losing a device does not deactivate RoomFrameSensor.
- RoomFrameSensor reports translation, rotation, and uniform scale of
  `inverse(roomFromWorld)`.
- The matrix must be finite, affine, invertible, and have positive uniform
  scale.
- Shear, reflection, non-uniform scale, perspective, or a non-finite inverse
  makes it inactive.
- Validation uses relative 1e-3 tolerances for equal column lengths and
  orthogonality, and 1e-6 for affine bottom-row checks.
- Within tolerance the rotation is re-orthonormalized and the scale is the
  mean column length, so float drift in a host's accumulated navigation matrix
  does not freeze the room; device poses and RoomGroup placement both use that
  cleaned similarity.
- An inactive RoomFrameSensor retains its last transform, so RoomGroup retains
  its last placement.
- The host must stop or hide physical-room rendering when its room frame is
  unavailable.

Buttons are sampled state, not queued transitions. A press and release between
submissions can disappear; there is no lossless click stream.

## Host setup and per-frame submission

Install the parse resolver before loading a scene, build its ROUTEs and scene
graph as usual, and install the input system once per execution context:

```cpp
#include <x3d/sdk.hpp>
#include <x3d/ext.hpp>

// Pass x3d::ext::install(yourProtoResolver) to the scene's parse call.
// After the session has built the graph and ROUTEs:
auto caveInput = x3d::ext::installCaveInput(session.scene(), session.context());
```

- The returned system enrolls native sensors, including auxiliary PROTO peers.
- The System attach/detach lifecycle enrolls or removes later Inline content.
- Install it after the session's standard systems (RuntimeSession and
  `attachStandardRuntime` add `InlineRuntimeSystem` there), as the snippet
  does.
- Systems update in installation order, so a CAVE system installed before the
  Inline loader sees sensors from a freshly loaded Inline or GeoLOD tile only
  on the next tick.
- The scene must outlive the system.
- Calls run on the context's owner thread, with no concurrent submission or
  teardown.

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

- `toX3dMatrix` and the pose conversions above belong to the host.
- Mat4 uses column-major storage, `m[column*4 + row]`.
- Use the logical head pose used for the cameras, including any deliberate
  desktop simulation.
- Device validity, tracking freshness, and calibration remain host
  responsibilities.
- The latest submission persists until replaced, so submit invalid devices
  explicitly on loss or app deactivation.
- Missing room support is the default inactive state; a desktop host can
  explicitly supply a synthetic floor frame.
- The system queues outputs with `postOutputEvent`, without first mutating
  their backing fields.
- X3DExecutionContext drains the cascade and re-evaluates systems within the
  same tick, so RoomGroup's three ROUTEs update its Transform before
  extraction or picking after that tick.
- Submit once per application frame, using the same navigation snapshot for
  input, world-space rays, and rendering; do not submit once per wall, eye, or
  GPU.
- The runtime's one-output-production-per-field-per-timestamp rule applies:
  feedback that changes an already-produced output in that timestamp is not a
  second production.

## RoomGroup placement and extraction

- Place RoomGroup at the world root or beneath structural groups that
  introduce no transform.
- Put room-local transforms inside RoomGroup.
- Transform-bearing ancestors count even when their current matrix is
  identity.
- Billboard, ScreenGroup, and Layout ancestors also violate this contract.
- A nested RoomGroup has a transforming ancestor and is invalid.
- Do not place RoomGroup inside a LayerSet layer: each Layer binds its own
  Viewpoint and NavigationInfo, but the room frame tracks root-scene
  navigation, so such placements warn too.
- Ancestry follows only fields that place children in the rendered graph:
  `children`, `LOD.level` and `Switch.choice` (X3D 3.x names),
  `GeoLOD.rootNode`, `LayerSet.layers`, `HAnimHumanoid.skeleton`/`skin`, and
  `CADFace.shape`.
- Reference and non-rendered slots, such as `TransformSensor.targetObject`,
  `Collision.proxy`, metadata, and geometry/appearance fields, do not make a
  RoomGroup transformed.
- RoomGroups are recognized by their private auxiliary sensor type, not by its
  `RoomGroupFrame` DEF, so author content reusing that name cannot create a
  false anchor.
- `CaveInputSystem::warnings()` reports invalid placements and rejected input
  after a tick.
- Placement warnings contain the affected primary node and a message.
- Input warnings identify the enabled device sensor and its device name; a
  null node identifies a rejected room frame.
- Reasons distinguish shear, non-uniform scale, non-finite or non-affine
  matrices, singularity, reflection, invalid host flags, unknown devices,
  invalid poses, and non-finite joysticks.
- Disabled sensors do not produce device warnings.
- Room input is diagnosed once per snapshot evaluation when at least one CAVE
  sensor is enrolled, even when no room sensor is enabled.
- A shared group warns if any path has a transforming ancestor.
- Placement diagnostics are cached across updates and cascade passes; TRS
  animation, viewer motion, and LOD selection do not rescan authored ancestry.
- Structural SFNode/MFNode writes advance `sceneTopologyRevision()`.
- Attach/detach, Inline/GeoLOD splices, and `buildRoutes()` also invalidate
  the placement cache.
- After direct root or private ROUTE collection edits, call
  `context.markSceneTopologyChanged()` (or rebuild the ROUTEs).
- Recalculation runs on the next system update, including another pass within
  the same tick.
- With no enrolled CAVE sensors, updates return immediately; with no RoomGroup
  anchors, no ancestry walk runs.
- Input validity is evaluated on each required update and composed with cached
  placement warnings.
- Detach drops cached warning pointers before removed nodes can be released.
- Copy messages before the next update; node pointers are borrowed and must
  not survive scene topology edits.
- The system diagnoses these placements; it does not cancel their ancestors.

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

## Portable fallback

- Copy `runtime/ext/protos/cave-fallbacks.x3d` beside your scene (or use the
  installed copy) and include the matching fragment URL after the URN.
- Without the ext resolver, the standard file resolver loads that PROTO.
- Sensor fallbacks use standard Group bodies with exposed interface fields and
  produce no tracking events.
- RoomGroup's fallback is an identity Transform that preserves its children.
- Physical anchoring is unavailable in that mode; mouse-to-wand emulation is
  not implicit.

The fallback file is an X3D 4.0 document: RoomGroup's fallback connects
`visible` and `bboxDisplay`, which are X3D 4 Transform fields. X3D 3.x browsers
may reject the file or that PROTO; scenes that must degrade in 3.x browsers
need their own fallback without those two connections.
