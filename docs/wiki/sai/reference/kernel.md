---
title: SAI reference kernel
summary: Reference for the kernel's contexts, snapshots, edits, node lifecycle, declarations, observation and publication guarantees.
tags: [sai, reference, kernel]
updated: 2026-11-14
related:
  - ../explanation/kernel-design.md
  - ../how-to/author-with-the-kernel.md
  - kernel-runtime.md
---

# SAI reference kernel

Header: `<x3d/sai/experimental/kernel.hpp>`. Targets:
`x3d::sai_reference_kernel`, or `x3d::sai_reference_metadata` for the generated
catalog and typed bindings.

Everything on this page is a reference-only extension. It is not part of the
portable [provider contract](provider-contract.md) and is not required of
another backend. The reasons behind the model are in
[Reference kernel design](../explanation/kernel-design.md); tasks are in
[Author and inspect with the reference kernel](../how-to/author-with-the-kernel.md).

## Core types

| Type | Role |
|---|---|
| `execution_context` | Owns one generation of a scene |
| `scene_snapshot` | Immutable view of one revision |
| `scene_edit` | Stages one atomic revision; `commit()` returns an ordered `change_set` |
| `node_id` | Semantic identity |
| `occurrence` | Ordered containment edge with its own parent, field, index and path |
| `node`, `typed_node<Tag>` | Context-bearing handles |
| `dynamic_field`, `field<T>` | Field handles over one `field_descriptor` |
| `type_registry` | The configured node-type vocabulary |
| `event_batch` | Timestamped, move-only external event intent |
| `subscription` | Move-only RAII observation; cancellation and destruction invoke no callbacks |

## Vocabulary discovery

`registry.node_types()` and `snapshot.node_types()` return owning
`node_type_descriptor` values sorted by name.

- The list includes abstract interfaces (`type.abstract`).
- Each descriptor has its ordered fields, exact value kinds, defaults, access,
  accepted node types, units, component facts and interface ancestry.
- Editing or retaining the returned collection cannot change the registry or
  scene.
- The vocabulary stays with its execution-context generation and is
  inspectable through historical snapshots after world replacement.

Local declarations have a separate declaration discovery API.

## Typed and dynamic access

| Operation | Behaviour |
|---|---|
| `snapshot.as<bindings::Transform>(node)` | Checked specialization; verifies context, node type and model provenance |
| Abstract view | A Transform can be viewed as `typed_node<bindings::X3DGroupingNode>` |
| `field.dynamic()` | The same handle for runtime-typed algorithms; grants no extra access |
| `readable()` | Predicts snapshot inspection |
| `writable(intent)` | Distinguishes setup authoring, retained writes, event input and host-only runtime output |

Typed fields support `readable`, `writable`, `add_route`, `observe`, reads,
writes and range edits through the same descriptor checks as dynamic handles.

- Generated tags and registries carry the same model fingerprint.
- Typed creation fails before producing a handle when provenance differs.
- A successful public mutation of a generated registry revokes that
  provenance, so mixed handwritten and generated descriptors cannot acquire
  generated typed handles.
- Typed reads report a structured `type_mismatch` and never expose variant
  access exceptions.

## Reads

| Call | Returns |
|---|---|
| `snapshot.read(field)` | Owning value; node fields return IDs as inspection data |
| `snapshot.read(field, value_space::authored)` | Owning value converted to authored units |
| `snapshot.read_node(field)`, `read_nodes(field)` | Usable handles, with every MFNode NULL position preserved |
| `snapshot.read_view(field)` | `shared_ptr<const T>` (typed) or `shared_ptr<const value>` (dynamic) |

`read_view` pins immutable canonical storage without copying its payload.

- It stays valid after the snapshot, context or browser is gone.
- Later revisions never change its contents.
- An imported view pins the exact source revision and enforces the same
  aperture and access checks as owning reads.
- A view carries inspection data, not mutation authority.
- A view retains its entire source revision, so release it when finished.

## Node handles

| Member | Meaning |
|---|---|
| `node{}` | X3D NULL; carries no context authority |
| `is_null()` | Tests absence |
| `disposed()` | Reports local wrapper disposal |
| `dispose()` | `noexcept`, idempotent, callback-free, identity-preserving; no scene or copy-wide effect |

A disposed wrapper cannot author values. For SFNode fields,
`edit.set(field, std::nullopt)` and `events.send(field, std::nullopt)` assign
NULL, as do `node{}` handles.

## Edits

- An edit stages changes against one base revision and publishes them
  atomically with `commit()`.
- MF fields are edited through revision-bound typed or dynamic range editors.
  Indexed changes stay ordered inside one edit. MF editors check access again
  at every mutation.
- MFNode mutations accept only context-bearing handles and validate the
  complete replacement range before staging it.
- Roots, ordered scoped names and unique routes are inspected and edited
  through the same transaction. IDs found through inspection resolve back to
  handles; mutation tokens need no private keys.

### `change_set`

For ordinary state changes, `before` and `after` are transition facts. Three
kinds are discriminated differently, so consumers must switch on `kind`:

| Kind | Payload |
|---|---|
| `unit_declared` | `field` is the category, `before` the declared name, `after` the canonical conversion factor, `index` the declaration order |
| `node_realized` | Empty field name, `before=false`, `after=true` |
| `metadata_change` | Key plus optional `before`/`after` strings |

### Node removal

Removal is an explicit, non-cascading staged mutation.

- Roots, node-valued fields, names, exports, routes and committed imports keep
  their targets until the caller removes or rewrites those references.
- Field observers and undrained event notifications keep targets at the live
  context boundary.
- Old snapshots remain historical truth. Current handles become stale after
  publication.

## Setup and realization

Follows the setup/realized distinction in
[SAI 4.4.8](https://www.web3d.org/documents/specifications/19775-2/V3.3/Part02/concepts.html).

| Event | Effect |
|---|---|
| Node created | Starts in setup |
| Unattached node committed | Stays in setup; initializeOnly fields can be configured in a later edit |
| `edit.realize(node)` | Explicitly ends setup; idempotent |
| Assigned to a node-valued field, or added as a child or root | The referenced node graph is realized in the same edit |
| First event to a node | Realizes its target and node payloads with the event's retained effects |
| Reference removed | Never reopens setup |

- The *containing* node stays in setup until it is itself referenced, rooted or
  realized.
- NULL slots and shared references produce no duplicate transitions.
- `snapshot.is_realized(node)` inspects the state at that revision.
- Failed or conflicting edits publish neither phase nor field changes.
- Name and declaration metadata changes leave realization unchanged.

Snapshots can inspect retained initializeOnly facts after realization. Live SAI
field access must enforce its lifecycle table at the live service boundary;
inspection does not stand in for that boundary.

## Scene metadata

| Call | Behaviour |
|---|---|
| `edit.set_metadata(key, value)` | Replace or append; position kept on replacement |
| `edit.set_metadata(key, std::nullopt)` | Remove |
| `snapshot.metadata(key)` | Owning `optional<string>`; absent differs from present-and-empty |
| `snapshot.metadata()` | Immutable `metadata_entry` values in insertion order; keep the snapshot alive while using the range |

Keys are case-sensitive. Setting an identical value or removing an absent key
adds no change. Metadata publishes together with graph changes, emits no field
events and realizes no node. Historical snapshots keep their metadata. The
zero-or-one value and NULL-removal rules follow the
[SAI scene services](https://web3d.org/specifications/X3Dv4Draft/ISO-IEC19775-2v4.0-CD/Part02/servRef.html#getMetaData).

## Declarations (PROTO and EXTERNPROTO)

These belong to the reference declaration API. The portable provider contract
has no PROTO service.

| Rule | Detail |
|---|---|
| Node defaults | `edit.set_declaration_default(proto, field, node_or_null)` and a `std::span<const std::optional<node>>` overload |
| Ownership | The complete referenced node graph becomes declaration-owned; defaults and body roots share that scope |
| Replacing a default | Previously claimed nodes stay owned until the declaration is removed |
| Bare IDs | Inspection data only; cannot author non-null defaults |
| `metadata` field | Interfaces cannot redeclare the inherited field |
| Annotations | Each interface field owns `appinfo` and `documentation` strings, kept in snapshots and change records |
| EXTERNPROTO | No authored defaults; by-name kind and access fields must be compatible with the resolved local declaration; omitted node-type and unit constraints are unconstrained |
| Resolution | `edit.resolve_external_declaration(external, local)` with declaration handles. `update_declaration` may edit metadata or reset load state but cannot assign a resolved target from a bare ID |
| Templates | PROTO template nodes cannot receive live events or appear in node-valued event payloads. Template claims cannot take nodes retained by observers, undrained events or committed imports |

Local and external declarations share one ID sequence.

## EXPORT and IMPORT

Execution-context boundaries have explicit ordered apertures. Imported nodes
and fields are distinct inspection-only proxy types. A parent snapshot captures
immutable source revisions and does not claim a globally atomic multi-context
snapshot.

## Observation

| Rule | Behaviour |
|---|---|
| Delivery | Queued, then drained explicitly outside internal locks |
| Thread | Callbacks run on the active drain caller's thread |
| Recursive or concurrent drain | `already_draining`, without consuming notifications |
| Late producer | A publish after the drain's final empty check needs a later drain |
| Cancellation | Suppresses callbacks not yet acquired by the drain; an acquired callback may finish |
| Captures | Retired outside context and browser locks, including on registration failure and world replacement |

`observe`, `set_runtime_processor` and `set_runtime_factory` accept move-only
callables directly. Copyable callables and the `std::function` aliases remain
valid.

## Events

- `inputOnly` delivery is transient. `inputOutput` delivery publishes retained
  state once.
- ROUTE propagation records causal predecessors and breaks loops per route.
- Fan-in is reported as a deterministic trace plus a structured portability
  issue.
- Runtime evaluation is described in [Kernel runtime evaluation](kernel-runtime.md).

## Publication guarantees

Commits and event cascades validate node references and the declared
containment graph before publication, following
[X3D 4.3.3 and 4.3.5](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/concepts.html#Scenegraphhierarchy).

| Case | Outcome |
|---|---|
| Containment cycle | Rejects the whole commit |
| Shared nodes, NULL slots, non-containment reference cycles | Valid |
| Node-valued events and routed deliveries | Same accepted-node-type and declaration-scope checks as authoring |
| `std::bad_alloc` during publication | Propagates; visible revision, field values, queued observations, event time and import retention are unchanged |

Commit prepares its result, notifications, candidate state and import retention
before changing any published state.

### `validate()`

`scene_edit::validate() const` preflights the same checks as `commit()`:
previous mutation errors, consumed edits, declaration compatibility, node
references and containment, current revision and lifecycle, live retention,
and import source exports.

- It does not consume the edit, repair its graph, realize nodes or publish
  anything.
- Success reserves nothing. An intervening edit, retirement, observation,
  import or runtime evaluation can change whether the later commit is admitted.
- Containment diagnostics identify the node and field whose edge closes a
  cycle. Traversal is by identity and declared field order, so that endpoint is
  deterministic.

### Recovering from failure

| Failure | Recovery |
|---|---|
| Failed `validate()` | The staged edit is intact |
| Commit-time graph error | Correct it with further edits and validate again |
| A mutation that poisoned the edit | Start a fresh edit |
| Failed event batch | Discard it and start a fresh one |

Publication atomicity does not cover arbitrary mutations of a thread-confined
staged edit or external host side effects.

## World replacement and loading

- World replacement and asynchronous completion are generation and request
  aware.
- Completion validates its ticket and installs the world atomically with
  respect to replacement and other completions.
- Success consumes the ticket, even for the current scene.
- A rejected replacement leaves the ticket retryable or cancellable while its
  original epoch remains current.
- The load API models only these cancellation and stale-completion rules.

## Capabilities

The kernel advertises authoring and inspection. `capabilities().live` is false.
See [Scope and limits](../explanation/scope-and-limits.md).

## Examples and tests

| File | Shows |
|---|---|
| `sai/examples/generated_author_inspect.cpp` | Primary authoring story with generated descriptors, typed keys and one commit |
| `sai/examples/author_inspect.cpp` | Handwritten dynamic fixture for event, ROUTE, shared-node and import behaviour |
| `sai/examples/generated_runtime.cpp`, `generated_stateful_runtime.cpp` | Runtime hosts |

The focused doctest suite is registered as stable `sai_*` CTests so each
invariant can be cited by the [service register](service-register.md).
