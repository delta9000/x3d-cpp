---
title: SAI submissions, turns and notifications
summary: Update gates, request states, host turns, failure effects and notification delivery in the provider contract.
tags: [sai, reference, provider, events]
updated: 2026-11-14
related:
  - provider-contract.md
  - ../how-to/run-the-host-loop.md
---

# SAI submissions, turns and notifications

This page defines what happens between `enqueue` and a delivered callback. For
the task-oriented version see [Run the host loop](../how-to/run-the-host-loop.md).
Handles, operations and scene states are in [Provider contract](provider-contract.md).

## Request states

`request_state`, read through `ticket::status()`:

| State | Meaning |
|---|---|
| `accepted` | The call passed admission and is held, possibly behind an open gate |
| `submitted` | Its gate was released; it is in the activation's FIFO |
| `processed` | The released work was evaluated, including documented coalescing |
| `cancelled` | Cancelled by the client, by retirement or by close |
| `failed` | Rejected at release or failed in execution; see `error` and `effects` |

`request_status` also carries `turn`, `effects` (`failure_effect`), an optional
`error` and the shared `submission` ID.

- Accepted, submitted and processed are different states. Processed does not
  mean every intermediate value appeared as a separately visible event.
- Dropping a ticket does not cancel accepted work.
- Terminal payloads leave service queues. Consumers choose how long to keep
  their receipt metadata.
- A submission ID is not a sortable FIFO sequence.

## Update gates

Sources: abstract SAI
[4.8.4.3](https://www.web3d.org/documents/specifications/19775-2/V3.3/Part02/concepts.html#ExtUpdatingSceneGraph),
[4.8.4.5](https://www.web3d.org/documents/specifications/19775-2/V3.3/Part02/concepts.html#SynchronizingMultipleApplications)
and [6.3.19](https://www.web3d.org/documents/specifications/19775-2/V3.3/Part02/servRef.html#UpdateControl).

| Rule | Behaviour |
|---|---|
| One gate per client | Gates do not nest. A repeated `begin_update` and an unmatched `end_update` are successful no-ops |
| Release | `end_update` releases one submission. Without an open gate each call is its own submission |
| Ordering | Release time sets FIFO order across clients of the same activation. The host chooses which independent activation to pump |
| Receipts | Each call keeps its own ticket and correlation ID; released calls share one outcome |
| Cancellation | Cancelling any call in a released submission cancels the whole submission |
| Intrinsic errors | A rejected call does not poison earlier accepted work |

## How writes combine inside a gate

| Write | Policy |
|---|---|
| Whole-field write to a stateful (inputOutput) field | Last accepted value wins |
| Whole write to an inputOnly field | Every write is an ordered occurrence, including repeated identical values. It is not treated as retained state |
| MF indexed operation | Aggregates into the pending field candidate in call order; the last accepted write to an index wins |
| MF indexed operation on an inputOnly field | Alters the most recent pending owning occurrence; rejected if there is no such base |
| Coupled constraints | Checked on the final released candidate and again immediately before execution. A held gate may repair an intermediate inconsistency |

The mixed whole-inputOnly and indexed-MF rule is an SDK decision. The published
[4.3.5.5.3 indexed aggregation rule](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19777-4v3.3-IS/Part04/concepts.html#Set1ValueExternalInteractions)
does not settle mixed whole-value occurrences, so this is recorded as our
choice and not as an inherited requirement. Browser, load, topology and dispose
ordering are outside this field-submission slice.

`write_mode` values: `replace`, `set_element`, `append`, `insert`, `remove`,
`clear`.

## Turns and time

- `pump(activation, time)` evaluates one FIFO submission, or performs an idle
  turn when the queue is empty.
- Each admitted pump has a distinct logical turn.
- Host time must be finite (`invalid_value`) and nondecreasing
  (`event_time_regression`). Equal times are valid.
- A release-time rejection consumes no turn. An execution preflight attempt
  consumes a turn even when it makes no backend mutation.

This is a host integration policy, not a promise of reference revision parity.

`turn_report` fields: `turn`, `time`, optional `request`, `state`
(`activation_state`), `effects` and an optional `error`.

## Evaluation order within a turn

- All stateful candidate inputs are applied before cascade evaluation.
- inputOnly occurrences keep their accepted order.
- Live ROUTE requests install before evaluation; see [Live ROUTEs](live-routes.md).

This does not promise a distinct output for each input. The architectural
[output and ROUTE limits](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/concepts.html#ExecutionModel)
and loop breakers stay in force for a cascade. No epsilon timestamps or
per-input guard resets are introduced. Tests compare required causality and
coherence, not an arbitrary fan-in winner.

## Failures

| Failure | Submission | Activation | Later work |
|---|---|---|---|
| Rejected at release | Fails; no turn consumed | Active | Unaffected |
| Preflight failure with known no mutation | Fails | Active | Still processable |
| Backend failure with partial or unknown effects | Fails with `partial_or_unknown` | Faulted | Cancelled |
| Application callback throws | Unaffected | Active | Unaffected |

- A successful outer `result` from `pump` can contain a faulted turn report.
  Always inspect `report.error`, `report.state` and `report.effects`.
- A faulted activation cannot be pumped again (`access_denied`). Retire it.
- Native rollback and automatic replay are never implied.
- A callback failure does not undo native work.

## Notifications

- `observe(activation, field, callback)` registers a callback and returns a
  `subscription`.
- Callbacks run only inside `dispatch_notifications()`, on the calling thread.
- A `notification` carries `source`, an owning `value`, `activation_epoch`,
  `turn` and `time`.

| Situation | Behaviour |
|---|---|
| Later turn fails | Notifications already completed survive; the failed turn's notices are discarded |
| Subscription cancelled or scene retired | Pending delivery is revoked |
| Callback creates new notices | They wait for a later drain |
| Recursive drain | Rejected with `operation_in_progress` |

Queue budgets are in [Resource limits](resource-limits.md).
