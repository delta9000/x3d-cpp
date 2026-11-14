---
title: SAI user data
summary: Reference for application user data attached to nodes and fields, in the provider contract and the reference kernel.
tags: [sai, reference, user-data]
updated: 2026-11-14
related:
  - ../how-to/attach-user-data.md
---

# SAI user data

User data is application-owned data associated with a node or a canonical
field. Task guide: [Attach user data](../how-to/attach-user-data.md).

## API

Header: `<x3d/sai/experimental/user_data.hpp>`.

| Surface | Read | Write |
|---|---|---|
| `provider::service` | `user_data(node)`, `user_data(field)` | `set_user_data(node, value)`, `set_user_data(field, value)` |
| Kernel `node`, `dynamic_field`, `typed_node<Tag>`, `field<T>` | `user_data()` | `set_user_data(value)` |

Both return `result<user_data_value>` and `result<void>`. The kernel's
`imported_node` and `dynamic_imported_field` views do not carry user data.

## `user_data_value`

| Operation | Behaviour |
|---|---|
| `user_data_value::make<T>(args...)` | Constructs and owns a `T` (move-only types included) with shared ownership and type erasure |
| Default-constructed value | Empty; passing it to `set_user_data` clears the association |
| `as<T>()` on an empty value | Succeeds with an empty `shared_ptr` |
| `as<T>()` with the exact stored type | `shared_ptr<const T>` |
| `as<T>()` with a different type | `type_mismatch` |

- A successful empty read is distinct from a failed service call.
- Copying a value or calling `as<T>()` retains ownership independently of later
  replacement, clearing, wrapper disposal and context lifetime.
- No borrowed `void*`, no payload copy constructor call and no unchecked
  downcast is exposed.
- Const access is shallow. Synchronizing and owning anything the payload points
  to is the application's job.

`T` must be a non-array, unqualified object type with a nonthrowing destructor.

## Identity

- An association belongs to a node identity, or to the pair of node identity
  and canonical field name. It is a sidecar on the context and is not part of
  immutable scene state.
- Separate contexts cannot collide, even when their local node IDs and
  generation numbers match.
- The `set_` and `_changed` aliases of an inputOutput field resolve to that
  field's canonical association.
- An unknown field, including an empty field name, returns `unknown_field`.
- Associations are independent of field access type: inputOnly, outputOnly,
  initializeOnly and inputOutput fields can all carry data.

## Update semantics

User-data changes are immediate, mutex-serialized metadata operations. They:

- do not join an authoring edit, update gate or event batch
- do not realize a node
- never modify field values, revision numbers, routes, notifications or event
  queues

In the kernel, a field obtained through an old snapshot reads the *current*
association; snapshot history does not freeze or roll back user data. An open
scene edit stays valid across user-data changes, and its later commit cannot
overwrite them.

## Wrapper and context lifetime

| Situation | Behaviour |
|---|---|
| One wrapper disposed | The association stays for copies and reacquired wrappers |
| Field acquired before its node wrapper was disposed | Still independently usable |
| Disposed wrapper, expired or retired context | Rejected |
| Node absent from the currently published state | `unknown_node`, even if historical snapshots keep its values |
| Unpublished authoring handle | Cannot create an association |

Wrappers carry weak context authority and do not prolong the context's
lifetime. A retained payload does not prolong it either, unless its own
contents hold an owning reference.

## Retention policy

Associations are kept until they are explicitly cleared or replaced, or the
owning context is destroyed. Node removal and context retirement do not release
them eagerly. Node IDs are not reused, so retention cannot leak an old
association into a new node.

This keeps data longer than the minimum the binding permits. It is not
automatic garbage collection.

!!! warning "Ownership cycles"
    Clear any association that holds an owning reference to its context,
    browser, provider or anything that indirectly owns the context, before
    those references become unreachable and before node removal or context
    retirement makes the live clear operation unavailable. There is no cycle
    collector. Prefer weak references when ownership is unnecessary.

## Locking and exceptions

- Reads retain shared ownership while holding the context mutex. Writers
  validate under that mutex and publish with nonthrowing swaps.
- Different wrappers may read and write concurrently. Mutating or discarding
  the *same wrapper object* concurrently with a call on it needs application
  synchronization.
- Payload construction happens before service entry.
- Replaced, cleared and rejected payloads are released only after internal
  locks are gone. Their destructors may reenter public services; a replacement
  is already visible when the old payload is released, so a destructor's
  reentrant write can supersede the outer write.
- Payload construction or allocation failure propagates as an ordinary C++
  exception. Map and key allocation precedes taking ownership, so a failed
  insertion leaves existing associations, scene state and events unchanged.
- A throwing `noexcept` destructor still terminates the process.

## Source and SDK choices

The published C++ binding's
[§4.3.3.1, Data storage](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19777-4v3.3-IS/Part04/concepts.html#DataStorage)
requires user-data associations with both node and field identity, observed
identically through different references, including after wrapper disposal and
reacquisition. It permits release once neither applications nor the scene graph
retain the node or field, without requiring immediate reclamation. The field
operations appear in
[Annex A.4.5, X3DField](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19777-4v3.3-IS/Part04/abstracts.html#X3DField)
as `setUserData(CObject)` and `getUserData()`.

The owning C++ types, error vocabulary, immediate-update policy and
conservative retention above are SDK choices. The node methods here are not
spellings published in Annex A. Register rows:
`SVC-NODE-USER-DATA-READ/WRITE` and `SVC-FIELD-USER-DATA-READ/WRITE`.

## Evidence

`sai/tests/user_data_services_test.cpp` (CTest `sai_user_data_services`) covers
owning and move-only payloads, exact-type errors, node and canonical field
identity, wrapper disposal and reacquisition, independent field lifetime, field
access modes, typed forwarding, foreign-context isolation, absent, disposed,
removed, retired and expired handles, history retention, destructor reentry on
replacement, clear and failure, allocation-failure atomicity, isolation from
scene, events and revisions, and concurrent readers and writers. The shared
provider fixture runs the provider-level cases against both backends.
