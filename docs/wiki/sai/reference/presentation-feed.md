---
title: SAI native presentation feed
summary: Reference for make_presented_scene, immutable presentation frames and changes_between.
tags: [sai, reference, native, presentation]
updated: 2026-11-14
related:
  - ../how-to/present-frames.md
  - ../../decisions/0066-sai-presentation-materials-and-deltas.md
---

# SAI native presentation feed

Header: `<x3d/sai_presentation.hpp>`, namespace
`x3d::sai::experimental::native`. This is a native-only extension; the portable
contract and `make_service()` are unchanged. Decision record:
[ADR-0066](../../decisions/0066-sai-presentation-materials-and-deltas.md). Task
guide: [Present frames](../how-to/present-frames.md).

## Types

| Type | Members |
|---|---|
| `presented_scene` | `service`, `scene`, `feed` |
| `render_feed` | `snapshot(service)` |
| `presentation_frame` | `source` (opaque scene identity), `native_tick`, optional `host_time`, `items` |
| `presentation_item` | `placement_key`, `mesh_key`, `world_transform`, `mesh`, `material`, `material_revision` |
| `presentation_delta` | `added`, `removed`, `updated_transform`, `updated_geometry`, `updated_material`, `items` |

`make_presented_scene()` returns the ordinary service, one ordinary setup scene
and a feed bound to that exact scene and service. Other scenes created on that
service do not acquire a feed.

## `snapshot`

`feed.snapshot(service)` pulls the latest complete immutable frame.

| Condition | Result |
|---|---|
| Exact service, owner thread, active frontend state | `shared_ptr<const presentation_frame>` |
| Setup, wrong thread, retired, closed or foreign service | Error |
| Frontend fault after native execution completed | Error |

- No presentation callback runs in a native turn.
- The activation frame has no `host_time`. Later frames record the native tick
  and the supplied time.
- A rejected submission that never enters the backend creates no frame.

## Frame ownership

A frame owns its item vector, matrices, extracted `MaterialDesc` values and
shared immutable mesh data. It contains no native node pointers, storage IDs,
scene or session storage, or callback captures.

A host can retain and render an old frame after the service is destroyed. That
does not retain authority to operate on the expired scene handle.

## Keys

| Key | Identifies | Stability |
|---|---|---|
| `placement_key` | One placement path | Survives unchanged paths across extractor rebuilds. A removed path is forgotten; a later re-addition gets a new key |
| `mesh_key` | Currently live immutable mesh ownership | Shared placements share a key. Equivalent freshly rebuilt content may get another key |

Neither key space is a portable node handle. Numeric keys are meaningful only
within the frame's opaque scene identity. Identity exhaustion fails explicitly.

## `changes_between`

`changes_between(previous.get(), *current)` compares two retained frames from
the same scene. It needs no live service.

| Input | Behaviour |
|---|---|
| `previous == nullptr` | Initial upload: every current placement is `added` |
| Skipped ticks, equal host times | Supported |
| Foreign scene, decreasing native tick | Rejected |

| Bucket | Contents |
|---|---|
| `added`, `removed` | Placement keys |
| `updated_transform` | Exact matrix value changed |
| `updated_geometry` | Mesh identity changed |
| `updated_material` | `material_revision` advanced |
| `items` | Complete current payload per placement key, for the union of added and updated placements |

- Update buckets can overlap each other but exclude added and removed
  placements.
- Each changed key appears once in `items`, even when geometry and material
  both change. Unchanged and removed keys have no payload.
- Ordering within a bucket is unspecified.
- Payloads are owned values with shared immutable meshes, so the input frames
  can be released after comparison.
- This is a net frame comparison, not a log of intermediate edits.

## Materials

Material values come directly from extraction. The supported node slice has no
Appearance authoring, so each item carries the extractor's unlit-white
fallback.

`material_revision` is an opaque invalidation revision, not a content hash. It
advances on extractor material invalidation or placement rebuild, so
`updated_material` can conservatively request a refresh of an equal value.
Revisions survive unchanged turns and cannot be compared across scene
identities.

## Retention policy

The backend keeps one current frame and current identity maps. There is no
historical frame queue.

- A slow host skips intermediate frames and replaces its displayed scene from
  the next complete frame.
- Portable event and receipt queues keep their separate policies.
- Producer retention scales with the current scene. Consumer-retained frames
  and backend scene storage are outside the portable queue byte budgets.

## Limits

- The feed exposes geometry, materials and world transforms for the four-node
  slice. Camera, shading, rasterization and presentation belong to the host.
- A configuration preflight limits every registered node path, including
  detached nodes, to the native nesting bound (currently 1,000 nodes). This
  stops the extractor's silent depth truncation from being reported as a
  complete frame.
- If extraction exhausts its separate visit budget after native execution
  starts, capture fails, the activation faults and the feed rejects that
  candidate. Previously retained complete frames stay valid.
- The extractor distinguishes shared shapes under different Transform paths but
  collapses repeated identical root or path occurrences. The feed preserves
  that behaviour and does not claim occurrence-distinct rendering there.

## Evidence

`sai_provider_presentation` checks immutable frames, ownership and lifetime.
Its `materials_and_frame_deltas` and `keyed_delta_payloads` cases check the
delta contracts.
