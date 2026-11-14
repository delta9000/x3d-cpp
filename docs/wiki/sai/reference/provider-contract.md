---
title: SAI provider contract
summary: Handles, operations, scene states, threading and teardown rules of provider::service.
tags: [sai, reference, provider]
updated: 2026-11-14
related:
  - submissions.md
  - resource-limits.md
---

# SAI provider contract

Header: `<x3d/sai/experimental/provider.hpp>`, namespace
`x3d::sai::experimental::provider`.

This is source-level experimental interoperability for the advertised subset.
It is not a stable binary ABI or complete ISO conformance. Rebuild frontend and
backend together.

## Handles

| Handle | Identifies | Notes |
|---|---|---|
| `scene` | One provider-owned scene | |
| `context` | A lexical context of a scene | One root context per scene today |
| `node` | A node in a scene | |
| `field` | A canonical field of a node | Carries its `field_info` descriptor |
| `activation` | One activation of a scene | Separate identity from the scene and from a logical turn |
| `client` | One connected client of an activation | Owns one update gate |
| `ticket` | An owning receipt for one request | Usable after the service is destroyed |
| `subscription` | A live observation | Move-only; cancels on destruction |

- Handles carry provider and scene identity. Retiring and later reusing an
  internal numeric ID cannot resurrect an old handle.
- Consumer-held identity tokens retain no backend scene storage.
- `dispose()` is local wrapper invalidation. Disposed, foreign, expired or
  empty handles do not grant authority.

## Node values

| Type | Meaning |
|---|---|
| `node_value` = `std::optional<node>` | SFNode payload |
| `node_values` = `std::vector<node_value>` | MFNode payload |

Only `nullopt` is X3D NULL. An engaged but empty, disposed, foreign or expired
node is an invalid reference; it never silently becomes NULL. Raw numeric node
IDs cannot be passed as payloads (they are excluded at compile time).

## Scene states

`activation_state`: `setup`, `active`, `faulted`, `retired`.

## Operations

| Operations | Allowed state and outcome | Origin |
|---|---|---|
| `create_scene`, `create_node`, `define_name`, `append_root`, `author`, `add_route` | Setup only; exact kind, access and authority checks | Bounded portable setup contract |
| `roots`, `named`, `named_nodes`, `names`, `type_name`, `fields`, `get_field`, `read` | Setup, active or faulted; inputOnly read denied | SAI-derived service semantics |
| `activate` | Ends setup for the scene's owned nodes; failure can leave it faulted | Binding lifecycle policy |
| `enqueue_route` | Active, optional `live_routes`; installs at its FIFO turn | Portable optional capability |
| `connect`, `begin_update`, `end_update`, `enqueue`, `cancel`, `disconnect` | Active activation; per-client gate and owning requests | SAI-derived buffering plus receipt policy |
| `pump` | Active only; finite nondecreasing host time; no hidden scheduler | Host extension |
| `observe`, `dispatch_notifications` | Deferred owning notifications; explicit cancellation and callback affinity | SAI-derived observation plus C++ scheduling policy |
| `user_data`, `set_user_data` | Immediate node or field metadata, independent of values and revisions | Owning C++ mapping |
| `units`, `declare_unit`, `metadata`, `set_metadata` | See [Scene units and metadata](scene-information.md) | Optional capabilities |
| `retire` | Setup, active or faulted; removes live registries and backend resources | Lifecycle policy |
| `ticket::status` | Thread-safe snapshot, including after service destruction | C++ receipt extension |
| `close` | Checked owner-thread operation; explicit failure before teardown if a turn is in flight | C++ owner contract |

What happens to queued work is in
[Submissions, turns and notifications](submissions.md); budgets are in
[Resource limits](resource-limits.md).

Typed overloads take a generated `field_key<Owner, T>` (for example
`bindings::Transform::translation`) and return or accept the matching value
type. Dynamic overloads take a `field` handle and a `payload` variant.

## Capabilities

`service.supported()` returns `capabilities`:

| Member | Meaning |
|---|---|
| `backend_name` | Backend identifier |
| `contract_version` | Currently 1 |
| `services` | `service_capability { id, version }` list; query with `supports(id, version = 1)` |
| `nodes` | Constructible node types with schema membership tags |
| `field_kinds` | Value kinds implemented across the declared node slice |

`service_id` values: `scene_authoring`, `host_turns`, `notifications`,
`node_values`, `indexed_writes`, `user_data`, `reference_snapshots`,
`reference_transactions`, `reference_checkpoints`, `scene_units`,
`scene_metadata`, `live_routes`.

Per-field `supported` flags stay authoritative. Exact generated-key owner
matching is a current restriction; complete interface inheritance projection is
not claimed. How to read these claims is covered in
[Scope and limits](../explanation/scope-and-limits.md).

## Errors

Semantic failures return `result<T>` with a `sai_error`. Its `code`,
`operation` and `message` are always set; `node`, `field`, `declaration` and
revision members are filled when they apply. Branch on `code`, not on message
wording. Ordinary allocation failure may throw `std::bad_alloc`.

Codes the frontend itself returns:

| Code | Typical cause |
|---|---|
| `unsupported_operation` | Capability not advertised, or opaque content |
| `unsupported_field_type` | Field kind outside the backend's value services |
| `access_denied` | Wrong access type for the operation or scene state |
| `type_mismatch` | Payload kind differs from the field kind |
| `invalid_value` | Value fails a validity rule such as rotation axes |
| `index_out_of_range` | Indexed write beyond the pending value |
| `invalid_route` | Bad ROUTE direction or kind, or a duplicate setup route |
| `invalid_context` | Handle from another service, or a root, value or ROUTE crossing lexical context |
| `invalid_name`, `unknown_field` | Bad DEF name; no such canonical field |
| `invalid_descriptor` | Backend returned malformed names, values or descriptors |
| `stale_handle` | Expired or retired authority |
| `disposed` | Explicitly disposed wrapper |
| `wrong_thread` | Call from a thread other than the owner |
| `resource_limit` | Admission budget exhausted |
| `operation_in_progress` | Reentrant call during a turn, or a recursive drain |
| `event_time_regression` | Decreasing host time passed to `pump` |
| `callback_failed` | A backend call or application callback threw |

A backend can add its own codes, for example `containment_cycle` for an
authored graph that would contain itself.

## Threading

- Normal service operations are owner-thread-affine.
- Retained thread-lifetime tokens stop a recycled OS thread ID from acquiring
  authority.
- Copying owning values or receipts across threads does not authorize
  concurrent scene mutation.
- `subscription::cancel` is thread-safe. A callback that is already executing
  is not interrupted.
- `close()` returns an error on the wrong thread or during a backend turn.
  Destruction requires the owner thread unless the service is already closed.

## Teardown

- The frontend owns the backend and the shutdown sequence. Backend event sinks
  retain only weak frontend state.
- Teardown revokes authority before releasing application captures.
- Application callbacks and capture destructors may reenter or destroy the
  service. Dispatch pins detached state and does not resume through a destroyed
  owner.
- Retained receipts, owning values and presentation frames outlive the service.
  They keep their results, not authority.

## Implementing a backend

`provider::backend` is the dispatch seam. Its IDs and values are backend
storage addresses, never application authority; the frontend alone wraps and
unwraps node values and controls admission, gates, receipts and events.

Optional virtuals default to `unsupported_operation` without effects, so adding
one keeps existing subclasses source compatible:

| Virtual | Capability |
|---|---|
| `do_named_nodes` | Named-node discovery |
| `do_units`, `do_declare_unit` | `scene_units` |
| `do_metadata`, `do_set_metadata` | `scene_metadata` |
| `do_add_live_route`, `do_check_live_route` | `live_routes` (the check defaults to admit) |

`backend::observed(address)` tells a backend whether a live observer watches a
field, so it can skip reading and recording writes nobody observes.

Run a new backend against the shared oracle in
`testing/provider_fixture.hpp`; see [Test suites and gates](test-suites.md).
