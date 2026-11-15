---
title: SAI scene units and metadata
summary: Reference for the scene_units and scene_metadata capabilities of the provider contract.
tags: [sai, reference, provider, units]
updated: 2026-11-14
related:
  - ../how-to/declare-units-and-metadata.md
  - ../explanation/canonical-units.md
---

# SAI scene units and metadata

`scene_units` and `scene_metadata` are optional capabilities, implemented by
both bundled backends. Storage belongs to the backend. Task guide:
[Declare scene units and metadata](../how-to/declare-units-and-metadata.md).
Rationale: [Canonical units](../explanation/canonical-units.md).

## Units

| Operation | State | Behaviour |
|---|---|---|
| `units(context)` | Setup, active, faulted | Four effective declarations in angle, force, length, mass order |
| `declare_unit(scene, declaration)` | Setup, before the first successful node creation (detached nodes included) | Replaces that category's default |

Defaults for undeclared categories:

| Category | Name | Factor |
|---|---|---|
| angle | radian | 1 |
| force | newton | 1 |
| length | metre | 1 |
| mass | kilogram | 1 |

A declaration is accepted when:

- its category has not been declared yet (one per category)
- its name is nonempty and contains no ASCII whitespace
- its factor is finite and positive

A rejected call has no effect and leaves earlier declarations usable. Time is
always seconds, and derived dimensions are not separately declared.

Declarations describe document units. They do not rescale provider values,
which are always canonical SI.

## Metadata

Scene META is distinct from node `metadata` fields and from application
[user data](user-data.md).

| Operation | State | Behaviour |
|---|---|---|
| `metadata(scene)` | Setup, active, faulted | Owned key/value entries in first-insertion order |
| `metadata(scene, key)` | Setup, active, faulted | `optional<string>`; absent differs from present-and-empty |
| `set_metadata(scene, key, value)` | Setup only, immediate | Replace, remove or append |

`set_metadata` rules:

| Call | Effect |
|---|---|
| Existing key, new value | Replaces the value; position unchanged |
| `nullopt` | Removes the entry |
| Key removed earlier | Appends at the end |

Keys are case-sensitive, and an empty key is a valid key. Setup changes emit no
field events, open no client gate, activate no node and consume no logical
turn.

## Authority and lifetime

All operations use the checked scene and context authority and the owner-thread
rules. Wrong-thread, foreign, disposed, retired and closed authority is
rejected. Owning results survive later edits, retirement and service
destruction.

## Backend storage

| Backend | Units | Metadata |
|---|---|---|
| Reference | Kernel snapshots and isolated edit commits | Same |
| Native | `X3DDocument::head.units` and `Scene::sourceUnits`, updated together after allocation succeeds | Ordered entries in the document head, retained by `RuntimeSession::document()` after activation |

An adopted native scene refuses portable metadata projection with
`unsupported_operation`; see [Native provider](native-provider.md).

## Not covered

- The standard live, per-client buffered `setMetaData` service. Mutation here
  is immediate and setup-only.
- Importing duplicate document META names and optional XML META attributes.
- Nested execution contexts.
- Authored-space input, parsing, serialization and lossless codec round trips.

Insertion order is this SDK's policy, not a claimed ISO requirement.

## Standards mapping

- [Architecture 3.3, 7.2.5.5 UNIT](https://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/core.html#UNITStatement)
  and [4.3.6 standard units](https://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/concepts.html#StandardUnitsAndCoordinateSystem)
  define the four categories, their initial units, positive conversion factors
  and one declaration per category.
- Abstract SAI 6.4.5 getUnits and 6.5.2/6.5.3 getMetaData/setMetaData map to
  partial rows in the [service register](service-register.md). No row is
  promoted to implemented.
- The [4.0 draft service reference](https://web3d.org/specifications/X3Dv4Draft/ISO-IEC19775-2v4.0-CD/Part02/services.html#getUnits)
  corroborates effective defaults, single-value metadata replacement and NULL
  removal. It is supporting evidence, not a substituted baseline.

The owning C++ structs are this SDK's mapping and are not interface-compatible
with the published binding.

## Evidence

| Suite | Covers |
|---|---|
| Shared provider fixture (both backends) | Effective defaults, all four categories, invalid and duplicate declarations, insertion and removal order, authority checks, ownership past destruction, detached-node sealing, canonical setup and live interpolation under non-identity declarations |
| Native-only fixture | Source-unit declarations, normalized marks, generated Box defaults |
| `sai_provider_contract` probes | Absent capabilities, optional default methods, malformed backend declarations, reads after an injected fault |
