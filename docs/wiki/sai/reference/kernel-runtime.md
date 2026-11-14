---
title: SAI kernel runtime evaluation
summary: Reference for runtime_step, runtime processors, stateful runtime frames and checkpoints in the reference kernel.
tags: [sai, reference, kernel, events]
updated: 2026-11-14
related:
  - kernel.md
  - ../how-to/write-a-kernel-runtime-host.md
---

# SAI kernel runtime evaluation

A runtime host lets the browser owner compute native outputs while an event
batch is evaluated. This is a [reference kernel](kernel.md) extension. Task
guide: [Evaluate events with a kernel runtime host](../how-to/write-a-kernel-runtime-host.md).

## Installing a host

| Call | Host kind |
|---|---|
| `browser::set_runtime_processor(scene, callback)` | Stateless callback receiving a `runtime_step` |
| `browser::set_runtime_factory(scene, factory)` | Stateful frames with an owned checkpoint |

Both replace the same host evaluator. Passing an empty callback or factory
disables evaluation. Execution-context clients cannot construct a
`runtime_step` or obtain output authority through a write-intent flag.

## Frame sequence

For each `event_batch::commit()` with a host installed:

1. (Stateful only) the factory runs once with the immutable base snapshot and
   the frame time.
2. The host is called once with `step.input() == nullptr`. This preparation
   call can emit sensor outputs from `step.time()`, even for an empty batch.
3. The host is called for each external or routed input.
4. Native outputs go through ROUTEs. They do not recursively invoke the host as
   inputs.
5. (Stateful only) `finish()` runs once and its state becomes the checkpoint.
6. Field values, realization and the checkpoint publish in one revision.

All deliveries in the settled cascade share the batch timestamp. A successful
frame advances the time floor even if it produces no events. Later frames need
strictly increasing finite timestamps.

## `runtime_step`

| Member | Behaviour |
|---|---|
| `step.time()` | The frame time |
| `step.input()` | The current input, or `nullptr` in the preparation call; inputOnly payloads are in `input()->payload` |
| `step.read(field)` | Owning copy of the working value: prior processed deliveries and the current tentative inputOutput value |
| `step.emit(field, value)` | Queues an outputOnly or inputOutput event |
| `step.ignore_input()` | Rejects the incoming value without failing the frame |

- Committed snapshots still show the previous revision until publication.
- Typed and dynamic reads and outputs support explicit authored-unit
  conversion.
- Node-valued outputs take local handles or spans, preserve NULL entries and
  apply the usual scope and type checks.
- Queued outputs become working values when processed after the callback.
- No mutable storage and no snapshot view is exposed to the host.
- Copies of a step keep its input and time facts alive, but `read` and `emit`
  return `stale_revision` after the callback returns.
- Errors name `runtime_step.read` or `runtime_step.emit`, with the offending
  node identity and field name where applicable.

### Ignored and adjusted inputs

| Host action | Result |
|---|---|
| `ignore_input()` | The node is realized, but the payload is not retained, echoed or observed; `read()` sees the prior value |
| Emits an adjusted inputOutput value | That output replaces the automatic echo of the incoming value |

The ledger retains incoming events and marks ignored ones. Field observations
report emitted outputs.

### Output limits

Per-field and per-ROUTE limits allow one output per timestamp, including when
the graph feeds native output back into its input. Every attempted output is
validated before loop suppression, so a repeated field cannot hide a foreign or
malformed value.

## `event_result`

| Origin | Extra data |
|---|---|
| `external` | |
| `route` | `route_index` |
| `runtime` | `cause`: the ledger index of its input; preparation outputs have none |

## Atomicity

State changes and queued observations become visible only after the whole
cascade succeeds. A returned error, a host exception or an invalid output
aborts field storage, realization changes, queues, revisions and time
advancement.

## Reentrancy and locking

- Host callbacks run outside context locks. They can inspect committed
  snapshots and manage subscriptions.
- Concurrent or reentrant publication to that context, processor replacement
  and world replacement return `operation_in_progress`. A load completion
  rejected at this boundary keeps its ticket for retry.
- Retired callbacks are destroyed outside browser and context locks.
- `runtime_step::read()` and output-authority validation acquire no foreign
  context owners.

`commit()` reports processed completion synchronously. Asynchronous acceptance
and presentation scheduling are separate work.

## Stateful frames

A factory returns `result<runtime_frame>` wrapping an ordinary moveable object
with two public members:

```cpp
result<void> process(runtime_step &);
result<State> finish();
```

The `runtime_frame_adapter` concept checks this protocol. Neither the frame nor
its state needs to be copyable, and host code inherits no SDK base class.

| Rule | Behaviour |
|---|---|
| Checkpoint | `finish()`'s state is moved into an owned const checkpoint |
| Failure | A factory, processor or finalizer error aborts the frame, including routed changes, observation queues and time advancement |
| Resources | Released through ordinary destructors outside context locks, on success and failure |
| Revision | A stateful frame advances the revision even when its X3D fields stay equal |

### Reading checkpoints

`scene_snapshot::runtime_state<State>()` returns
`result<std::shared_ptr<const State>>`.

| Result | Meaning |
|---|---|
| Success, null pointer | No checkpoint |
| `type_mismatch` | A checkpoint of a different state type exists |
| Success, non-null | Pins the checkpoint; historical snapshots keep their earlier state |

A `runtime_state_change` alternative in `semantic_change` owns the before and
after checkpoints and exposes the same typed queries.

### Checkpoint lifetime

| Action | Effect on the checkpoint |
|---|---|
| Disable evaluation (empty factory or processor) | Last checkpoint left intact |
| Install a stateless processor | Cleared on its next successful frame |
| Authoring edit | Preserved; the host is not evaluated |

### Host obligations

- Stage private working resources and return an owned checkpoint.
- Preserve the checkpoint's immutability. Const access does not prevent
  mutation through aliases the host keeps.
- The kernel cannot undo external I/O or mutations made outside this protocol.

## Examples

| Example | Uses | Checks |
|---|---|---|
| `sai/examples/generated_runtime.cpp` | Generated PositionInterpolator and Transform fields | Reads key data, computes interpolation, emits `value_changed`, checks routed translation, settled observation and endpoint clamping |
| `sai/examples/generated_stateful_runtime.cpp` | Generated RigidBody and Transform bindings | Integrates `position` from `linearVelocity` through a ROUTE to translation; owns a move-only checkpoint; checks late finalizer failure, retry at the same timestamp, coherent observation and retained history |

Both link only the SAI project. They exercise the kernel's host callback
boundary. They are not an independent backend, and they do not implement the
whole Interpolation or RigidBodyPhysics component: the stateful example
ignores incoming position events and does not evaluate forces, torques, mass,
damping, gravity, orientation, geometry or collision.

Requirements follow the
[X3D event model](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/concepts.html#Eventmodel)
and
[PositionInterpolator](https://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/interp.html#PositionInterpolator).
