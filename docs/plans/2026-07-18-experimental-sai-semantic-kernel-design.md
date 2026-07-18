# Experimental C++ SAI Semantic Kernel Design

**Status:** Approved experimental proposal; no compatibility promise.

## Objective

Build a compiling, behavior-backed proposal for the irreducible semantics of a
modern C++ X3D SAI. The proposal must make identity, occurrence, field, edit,
error, lifetime, observation, concurrency, and asynchronous-generation choices
concrete enough to falsify with demanding user stories.

This is not an adapter over the current runtime. It is a firewalled reference
kernel for fast iteration. Runtime changes and adapters will be evaluated later,
one requirement at a time.

## Boundary

The proposal lives under `experimental/sai/` in namespace
`x3d::sai::experimental`. It does not include or modify the current `Scene`,
`X3DExecutionContext`, Script `SaiContext`, generated nodes, installed SDK
facades, or runtime libraries. It builds only when project tests are enabled.

The kernel is a small compiled library with public experimental headers. User
stories include only those headers. Private storage and algorithms remain
replaceable.

## Semantic objects

- `browser` owns the current execution context and world epoch.
- `execution_context` owns a sequence of immutable committed scene states.
- `node` and field handles identify semantic objects by context, generation,
  and stable ID; they never expose storage addresses.
- `node_id` denotes shared semantic node identity.
- `occurrence` denotes one containment edge and carries parent occurrence,
  container field, index, and path. Nodes have no canonical parent.
- `scene_snapshot` holds one immutable state and revision.
- `scene_edit` stages mutations against one base revision.

Replacing or disposing a context invalidates its handles predictably. IDs are
never reinterpreted as objects from a later generation.

## Descriptors and fields

`type_registry`, `node_type_descriptor`, and `field_descriptor` form the sole
source of field truth. A descriptor defines name, value kind, access type,
default value, and whether a node-valued field is a containment edge.

Dynamic discovery yields `dynamic_field`. `dynamic_field::as<T>()` yields a
typed `field<T>` view of the same field identity after checking its descriptor.
Both paths read and write through the same state, validation, lifecycle, edit,
and change machinery. Typed access adds compile-time value shape; it does not
add capabilities unavailable to dynamic access.

The proposal begins with a deliberately small value vocabulary: Boolean,
integer, number, string, `vec3f`, node ID, and ordered node-ID range. The
descriptor/value-kind boundary permits later generated X3D field coverage
without changing handle or transaction semantics.

Node IDs in snapshot values are inspection facts, not authoring authority.
Node-valued writes accept context-bearing `node` handles or borrowed spans of
handles, validate the complete value before staging, and store IDs only after
ownership validation. A snapshot can resolve an enumerated ID to a handle and
return an owning copy of its ordered type/field descriptor.

## Mutation and publication

All scene mutation occurs through `scene_edit`. It supports node creation,
field writes, root insertion/removal, scoped naming, and route insertion/removal.
An edit works on private staged state and records operations in authored order.

The first invalid operation poisons the edit. `commit()` then returns that
structured error and publishes nothing, preventing ignored intermediate errors
from producing partial edits. Commit also rejects an obsolete base revision.

A successful non-empty commit publishes one immutable state, advances exactly
one revision, and returns one ordered `change_set` containing before/after facts.
An empty commit is successful but does not advance the revision or notify
observers. Validation rejects dangling references, wrong value kinds, illegal
field access, invalid or duplicate routes, duplicate names, and containment
cycles. Scoped-name enumeration preserves definition order.

## Errors

Ordinary failure is `result<T>` carrying `sai_error`. The error contains a stable
`error_code`, operation, context generation, base/current revision, optional
node identity, optional field name, and explanatory text. Lookup absence, stale
handles, type mismatch, access violation, conflict, cancellation, and stale
completion are values. Exceptions are reserved for allocation failure and
violations of the C++ object contract such as calling `value()` on an error.

## Observation and concurrency

Committed states are immutable snapshots suitable for concurrent reading.
Mutation is single-writer and optimistic: edits may be staged concurrently, but
only an edit based on the current revision can commit.

`subscription` is move-only RAII. Commit enqueues change sets but invokes no
user callback. `execution_context::drain()` is the explicit single-consumer
dispatch point. Cancellation is immediate; destruction is callback-free and
`noexcept`. A callback may commit a new edit, whose notification is appended
after the current batch. The kernel creates no threads.

## Browser and asynchronous generation

The browser exposes capabilities, creates detached contexts, installs a context
as the current world, and begins explicit load requests. A load ticket records
request identity and browser epoch. Completion after cancellation or any world
replacement returns a stale/cancelled result and cannot replace the current
world or invoke a callback.

## Acceptance stories

The proposal is acceptable only when executable C++ stories demonstrate:

1. One shared node reached through multiple distinct occurrences.
2. Dynamic discovery and typed access observing identical state and errors.
3. Atomic multi-operation commit and total non-mutation after failure.
4. Ordered, complete change sets and stable machine-readable diagnostics.
5. Immutable snapshots plus deterministic stale-edit conflict.
6. Safe stale handles after world replacement.
7. RAII observation, cancellation, explicit dispatch, and reentrant commit.
8. Generation-safe asynchronous completion.
9. Scoped names and routes coordinated with the same commit boundary.
10. A composed authoring-and-inspection program using the whole surface.

No existing-runtime adapter is part of this slice.
