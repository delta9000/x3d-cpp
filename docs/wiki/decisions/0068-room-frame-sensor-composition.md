---
title: "ADR-0068: Room anchoring through sensor composition"
summary: A room-frame sensor drives a standard Transform through ROUTEs; RoomGroup is an ext PROTO, with no native transform semantic or render-item flag.
tags: [adr, ext-firewall, cave, sensors, transforms, externproto]
updated: 2026-10-18
related:
  - 0001-ext-firewall.md
  - ../subsystems/ext-firewall.md
---

# ADR-0063: Room anchoring through sensor composition

## Status

Accepted for the opt-in extension layer. Implementation and tests are included
in this proposal; build and test execution are deferred to the maintainer.

## Problem

A CAVE host maps world content into physical room coordinates using its
navigation transform. Some authored content must stay fixed in that room while
the user navigates. Rendering, picking, bounds, lights, and spatial sensors must
agree on where that content lives.

Standard X3D already supplies transforms and ROUTEs. Its ProximitySensor reports
the effective viewer within a region, but does not expose a calibrated physical
room frame or arbitrary tracked-device state. ScreenGroup changes pixel scale;
Layout and Layering do not identify the physical room frame.

## Decision

Add native TrackedDeviceSensor and RoomFrameSensor nodes only in `runtime/ext`.
Resolve their versioned `urn:x3d-cpp-gen:ext:...:1` identifiers through the
existing EXTERNPROTO resolver and declaration-local clone factories.

RoomFrameSensor publishes translation, rotation, and scale of
`worldFromRoom = inverse(roomFromWorld)`. RoomGroup is a PROTO whose primary
node is a standard Transform. An auxiliary RoomFrameSensor and three internal
ROUTEs drive that Transform. The normal event cascade settles these writes in
the same tick as the submitted frame, before extraction and picking.

RoomGroup requires a transform-free path to the world root. Identity Transform
ancestors count as violations because later animation changes their effect.
The extension system exposes warnings for invalid paths, including shared
placements. Children can contain ordinary room-local transforms. RoomGroup
does not cancel arbitrary ancestors or establish a new absolute frame rule.

All extracted transforms remain world transforms. For a room child C, the host
renders `roomFromWorld * worldFromRoom * C`. No RenderItem flag or special
extraction, bounds, lighting, or picking behavior is introduced.

Room transforms must be finite invertible positive uniform-scale similarities.
Invalid input deactivates the room sensor and retains the last valid pose.
Device input is sampled state; loss releases held controls, and recovery emits
complete state. The host owns device freshness and input arbitration.

Keep NavigationInfo mapping in the host. A new viewpoint node and an absolute
viewer-pose API are outside this proposal.

## Alternatives

A native absolute RoomGroup would need consistent frame-boundary behavior in
every spatial traversal, including DEF/USE paths and incremental extraction.
That cost is unnecessary for root-level room content.

A render-item room flag would let a renderer skip navigation, but would leave
picking and spatial behavior in a different frame unless those systems gained
parallel special cases. Separate Layer navigation and binding stacks would add
unrelated machinery without defining physical room coordinates.

## Consequences

The extension remains opt-in and one-way: ext includes core, never the reverse.
The generated node bindings and core scalar types stay unchanged. Standard
PROTO fallbacks retain visible children without pretending physical anchoring
or tracking is available.

Mat4 and extracted world transforms use 32-bit floats. Cancelling large world
translations can produce visible jitter; this decision does not solve floating
origin precision. Moving room frames also update world transforms for all room
placements. Hosts must use one coherent navigation snapshot for submission,
picking rays, and rendering, and handle invalid room frames explicitly.
