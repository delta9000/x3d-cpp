---
title: SAI scope and limits
summary: What the SAI SDK covers today, what it leaves out on purpose, and how to read its capability and register claims.
tags: [sai, explanation, scope]
updated: 2026-11-14
related:
  - ../reference/service-register.md
  - ../reference/native-provider.md
---

# SAI scope and limits

## How to read a capability

Backend service, node and field support describes executable behaviour.

- `service.supported()` lists services, constructible node types and field
  kinds for that backend.
- Each field descriptor's `supported` flag is authoritative for that field.
- A node's `required_component` and `minimum_declared_level` are schema
  membership facts. They do not claim the backend provides the complete
  component at that level, or any complete X3D profile.
- Unadvertised capabilities and default backend methods fail closed with
  `unsupported_operation`.

## What both backends construct today

Transform, Shape, Box and PositionInterpolator, with owner-bearing node values,
ROUTEs, scene authoring, host turns, notifications, indexed writes, user data,
scene units, scene metadata and live ROUTE addition.

The native backend can also adopt a loaded session and expose its nodes for
discovery and a bounded set of field kinds. See
[Native provider](../reference/native-provider.md).

## What the portable contract does not provide

- PROTO, IMPORT, Inline and Script services
- geospatial, URL loading and codec services
- browser services (these exist only on the reference kernel)
- ROUTE removal
- nested execution contexts
- a stable binary ABI

Native runtime support for a feature does not expose it through SAI. PROTO
instances, Inline contexts, IMPORT endpoints and Script fields keep executing
and rendering in an adopted scene, but they are opaque to field discovery.

## What the reference kernel leaves out

The kernel is a semantic reference model, not a complete ISO/IEC 19775-2 or
19777-4 implementation. It omits:

- parsing and serialization
- profiles and document-codec unit integration
- PROTO/EXTERNPROTO expansion and PROTO/IS apertures
- cross-context field writes and ROUTEs
- Inline loading and URL transport
- Script integration
- complete browser adapters and presentation scheduling
- scene-change provenance
- an ABI-stable boundary

Three points need care:

- **EXPORT/IMPORT** gives snapshot inspection, not multi-context live
  publication.
- **The load API** models only cancellation and stale-completion publication
  rules.
- **Live lifecycle** is partial. Browser-driven initialization, Script phases,
  live read restrictions and observation-triggered realization need a host
  boundary. Atomic `scene_edit` is an authoring extension; it does not
  implement SAI session update buffering or accepted/published/processed
  completion for a live host.

## Profile and component discovery

No profile list or component-support catalog is derived from node descriptors.
The abstract services (19775-2 §§6.3.6–9) require real support and at least one
supported profile and component. Returning an empty collection as "complete
discovery" would not meet that, and returning a familiar profile name without
implementing it would be a false claim.

A host-advertised catalog could own component and profile declarations later.
It would have to come from a host that implements and can substantiate what it
advertises, because descriptor presence alone cannot prove runtime support.

## What "partial" means in the register

The [service register](../reference/service-register.md) marks every service
as partial or planned. Partial means working, tested behaviour whose surface
or semantics are not yet the whole ISO service. Row counts measure inventory,
not a conformance percentage, and a green register check establishes
consistency, not conformance.

## Open audits

These are known gaps in the generated field model, tracked in
[Generated field projection](../reference/generated-fields.md):

- coupled numeric constraints such as sentinel tuples
- string vocabulary validation
- reference versus containment classification of node fields
- per-field unit categories, which the 4.0 UOM does not formally annotate
