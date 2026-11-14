---
title: Reference kernel design
summary: The design choices behind the SAI reference kernel and what each one buys.
tags: [sai, explanation, kernel]
updated: 2026-11-14
related:
  - ../reference/kernel.md
---

# Reference kernel design

The reference kernel captures the irreducible semantics of a modern C++ SAI as
executable choices. It is the storage behind the reference backend and is also
usable directly through `kernel.hpp`. This page explains the choices; the exact
behaviour is in [Reference kernel](../reference/kernel.md).

## Firewalled and dependency-light

The kernel lives in `x3d::sai::experimental`. It uses the C++ standard runtime
and platform thread support and does not modify or depend on x3d-cpp's runtime.
`result<T>` is a public alias of the pinned single-header `tl::expected` 1.3.1
(CC0-1.0), which supplies `and_then`, `transform`, `transform_error` and
`or_else` without a configure-time dependency or a homegrown expected type.

Generated runtime node objects never take part in SAI identity. The optional
`x3d::sai_reference_metadata` bridge copies node and field facts from an
instance-free catalog generated from the UOM.

## Identity is not containment

A `node_id` is semantic identity. An `occurrence` is an ordered containment
edge with its own parent, field, index and path. Keeping them apart is what
lets a shared (DEF/USE) node be one node with several placements, instead of
several nodes that happen to look alike.

## Immutable snapshots, atomic edits

An `execution_context` owns one generation. A `scene_snapshot` is immutable. A
`scene_edit` stages an atomic revision and returns an ordered `change_set`.

- Readers never see a half-applied edit.
- A failed or conflicting edit publishes nothing.
- History stays inspectable: old snapshots remain historical truth after later
  edits and after the world is replaced.

The context reserves node and declaration IDs when they are created. An
abandoned edit can leave gaps but cannot lend its IDs to a later object.

## One descriptor for typed and dynamic access

One `field_descriptor` governs both dynamic and typed field handles. Typed
access is a checked view over the same descriptor, so it cannot grant anything
dynamic access would refuse. Access type is enforced as a lifecycle rule, and
`writable()` is intent-specific: setup authoring, retained writes, event input
and host-only runtime output are different questions.

## Owning values and context-bearing handles

- MF fields are edited through revision-bound range editors, never borrowed
  mutable containers.
- Node-valued writes need context-bearing `node` handles. A bare ID is
  inspection data and cannot alias an object in another ownership domain.
- Field storage equality is the `same_representation` relation: IEEE payload
  bits survive, signed zeros differ and identical NaN payloads match. Ordinary
  `operator==` stays numerical.

## Explicit removal, explicit boundaries

Node removal is a staged, non-cascading mutation. Roots, node-valued fields,
names, exports, routes and committed imports keep their targets until the
caller removes or rewrites those references. Nothing disappears as a side
effect.

Execution-context boundaries have explicit ordered EXPORT/IMPORT apertures.
Imported nodes and fields are distinct inspection-only proxy types. A parent
snapshot captures immutable source revisions without claiming a globally atomic
multi-context snapshot.

## Observation is queued and drained

Observation is explicit, queued and drained outside internal locks. Callbacks
run on the thread that drains, so choosing that thread sets affinity. This is
the same reason the portable contract has an explicit
`dispatch_notifications`: application code never runs inside kernel locks, so
it can reenter the API safely.

## Events are batches with a causal trace

External event intent is a timestamped, move-only batch. ROUTE propagation
records causal predecessors and breaks loops per route. Fan-in stays
inspectable as a deterministic trace plus a structured portability issue, so
queue order is never silently blessed as semantics.

## Generations guard against stale completions

World replacement and asynchronous completion are generation and request
aware. A stale handle or a stale completion cannot mutate a replacement world.

## Capabilities fail closed

The kernel advertises authoring and inspection. It does not advertise live
execution, loading or rendering just because their publication-safety
protocols can be exercised. `capabilities().live` stays false. See
[Scope and limits](scope-and-limits.md).

## How to judge the API

No API here should be promoted merely because it compiles. The invariant tests
and composed examples are the review surface: the experimental vocabulary is
revised until those user stories stay clear and the remaining planned
invariants can be expressed without privileged access.
