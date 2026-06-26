---
title: "ADR-0018: CAVE Cross-Process RenderDelta Wire Contract"
summary: The VR CAVE target fixes a 1-master / N-wall-consumer topology with a serializable, schema-pinned RenderDelta wire format; master broadcasts the world clock so walls are inherently time-synced.
tags: [adr, cave, cross-process, render-delta, wire-format, vr]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/extract.md
  - ./0015-extraction-pull-per-path.md
  - ./0009-sim-snapshot-diff.md
---

# ADR-0018: CAVE Cross-Process RenderDelta Wire Contract

## Status

Accepted — 2026-06-20

## Context

The SDK's first concrete consumer is a VR CAVE installation where a single simulation
master advances the X3D scene and N wall processes each render an off-axis view of the same
world model. This topology reveals a gap in the existing extraction contract
([ADR-0015](0015-extraction-pull-per-path.md)): `SceneExtractor::delta()` returns a C++
`RenderDelta` struct in-process; for the CAVE, the same information must cross a process
boundary from the master to each wall.

Three forces motivate the design:

1. **Single source of truth is non-negotiable.** If each wall process ran its own
   `X3DExecutionContext::tick()`, floating-point divergence would desync wall views within
   seconds. The master must own the authoritative scene state and send changes outward.

2. **The dirty-set machinery is already per-tick.** `X3DExecutionContext::tick()` clears
   `dirty_` after each advance, giving one precisely-bounded delta per tick (the ADR-0015
   structural precondition). Serializing that delta is a serialization problem, not a
   scheduling problem.

3. **Transport and genlock are BYO.** The CAVE hardware already provides a genlocked display
   sync and a low-latency inter-process channel (shared memory or UDP LAN). The SDK must not
   impose a transport; it must define a schema that any transport can carry.

Additionally, each wall process needs to render its own off-axis frustum. The current SDK
exposes a bound `Viewpoint` (position + orientation in world space). A per-wall projection
requires adding a head-pose offset to derive the wall's eye position without forking the
authoritative Viewpoint or re-running scene behavior on the wall.

This ADR records the binding decisions that resolve these forces. The grounding source is
`docs/memory/x3d-cpp-gen-cave-consumer.md` and the headless/embedder design already
validated in `docs/superpowers/specs/2026-06-07-architecture-validation-and-resequencing.md`.

## Decision

We decided:

1. **Topology: 1 simulation master, N wall consumers.** The master process owns the
   `X3DExecutionContext`, runs `tick(now)`, and calls `SceneExtractor::fullSnapshot()` once
   (frame 0) then `SceneExtractor::delta()` once per tick. Wall processes are pure consumers:
   they receive the serialized delta, apply it to a mirror render state, and draw. No wall
   ever calls `tick()` or modifies the scene.

2. **Wire format: `{frame#, worldClock, RenderDelta delta}` with a pinned schema.** The
   master serializes the `RenderDelta` struct from ADR-0015 (`added`, `removed`,
   `updatedTransform`, `updatedGeometry`, `updatedMaterial`, `cameraChanged`, etc.) together
   with a monotonically increasing frame counter and the current `worldClock` value from
   `X3DExecutionContext`. The schema is pinned at the point where the SDK is integrated into
   the CAVE (not versioned dynamically at runtime); schema changes require a coordinated
   recompile of master and all wall consumers. Serialization format (e.g. flatbuffers, simple
   binary layout, or msgpack) is an embedder choice — the SDK defines the *field set*, not the
   encoding.

3. **Master broadcasts the world clock.** Including `worldClock` in every frame packet makes
   walls inherently time-synced with the master's simulation time. Walls do not need a
   separate NTP or PTP synchronization scheme to align their animation state; they simply apply
   the delta at the master's declared clock value. This is a free consequence of the
   pull-per-tick contract already established in ADR-0015.

4. **Per-wall head-pose offset without forking the Viewpoint.** The bound `Viewpoint` in the
   master's scene defines the canonical camera position (e.g., the user's nominal head
   position). Each wall applies a static per-wall offset (the geometric relationship between
   the wall and the CAVE center, plus a live head-tracking input seam) to derive its own
   eye-space matrix at render time. This offset is applied after the delta is received,
   entirely on the wall side. The master's `Viewpoint` is unchanged; the `cameraChanged` flag
   in the delta tells the wall when to recompute. The `ViewpointOffset` seam already exists
   in `runtime/events/ViewpointOffset.hpp` (introduced by ADR-0015's M2.5 work).

5. **The SDK is not the transport.** Shared memory, UDP, or a named pipe are all valid
   transports. The SDK produces a serializable in-memory `RenderDelta`; the CAVE integration
   layer wraps and sends it. This preserves the headless, embedder-agnostic posture.

## Consequences

**Positive:**

- The CAVE topology validates the headless design: the SDK runs as a library inside the
  master process with no knowledge of the walls, exactly as it would run inside any other
  embedder. No new coupling is introduced.
- The `{frame#, worldClock, RenderDelta}` wire format composes directly with the pull
  contract from ADR-0015. No new dirty-tracking machinery is needed; the extractor already
  maintains the delta.
- Wall processes are dead-simple: receive packet, apply delta to local render state, draw.
  No scene-graph traversal, no tick, no behavior systems. Off-axis projection is a matrix
  operation at render time.
- The master-broadcasts-clock convention makes wall timing robust to clock drift without
  requiring hardware PTP or NTP on the CAVE LAN.
- Component breadth (physics, sound, NURBS) is not needed for the CAVE forcing function and
  remains deferred, which is the correct priority order: a synced world model with geometry,
  material, and lighting is the load-bearing capability.

**Trade-offs / costs:**

- **Schema pinning requires coordinated redeployment.** If the `RenderDelta` struct changes
  (new fields, changed types), master and all wall consumers must be recompiled and
  redeployed together. The CAVE is a controlled environment (not a public protocol), so this
  is acceptable; it must be documented in the CAVE integration layer, not the SDK.
- **Serialization is out of scope for the SDK core.** The SDK does not ship a
  `serializeDelta()` function. The CAVE integration layer writes it. This is intentional
  (transport-agnostic posture) but means the contract exists in documentation and design,
  not in a shipmable codec. A future `RenderDeltaCodec` in `runtime/ext/` would be the right
  location if multiple embedders need one.
- **Head-tracking input seam is not yet formalized.** `ViewpointOffset.hpp` exists for the
  per-wall offset, but live head-tracking (streaming per-frame pose from a tracker device
  into the master's Viewpoint or into the wall's offset matrix) is not yet specified. This is
  a post-v1 input seam addition.
- **`updatedGeometry` binary data may be large for per-frame mesh animation.** The current
  `GeomId.contentVersion` bump on any content-field change will re-send the full mesh on
  animation (e.g., CoordinateInterpolator driving `coord`). The CAVE transport may need a
  geometry-delta sub-format for animated meshes; this is a follow-up optimization.

## Related

- [Architecture](../architecture.md)
- [Extract subsystem](../subsystems/extract.md) — `SceneExtractor`, `RenderItem`, `RenderDelta`
- [ADR-0015: Extraction as Pull over an Incremental Dirty Set](0015-extraction-pull-per-path.md) — the structural precondition (pull seam, per-tick dirty clear, `{frame#, worldClock}` embedding)
- Memory note: `docs/memory/x3d-cpp-gen-cave-consumer.md` — the user's CAVE topology description, the RenderDelta wire format requirement, and the head-pose offset approach
- Design synthesis: `docs/superpowers/specs/2026-06-18-future-proof-architecture-synthesis.md` §5 (headless posture) + §6 (caveats: sustainability / real consumer as forcing function)
- `runtime/events/ViewpointOffset.hpp` — the per-wall viewpoint offset seam
