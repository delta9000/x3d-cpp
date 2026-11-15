---
title: SAI live ROUTEs
summary: Reference for the optional live_routes capability and enqueue_route.
tags: [sai, reference, provider, routes]
updated: 2026-11-14
related:
  - ../how-to/add-a-live-route.md
  - ../../explanation/decisions/0075-sai-live-route-addition.md
---

# SAI live ROUTEs

`service_id::live_routes` is an optional version-1 capability, implemented by
both bundled backends. The decision record is
[ADR-0075](../../explanation/decisions/0075-sai-live-route-addition.md). For the task see
[Add a ROUTE to a live scene](../how-to/add-a-live-route.md).

## Operations

| Operation | State | Duplicate route |
|---|---|---|
| `add_route(context, from, to)` | Setup only | Refused with `invalid_route` |
| `enqueue_route(client, from, to)` | Active, needs `live_routes` | Successful no-op |

`enqueue_route` returns an owning ticket that uses the same FIFO, update gate,
cancellation, budgets and retirement rules as field writes. A backend without
the capability rejects admission with `unsupported_operation`. ROUTE removal is
outside this capability.

## Admission checks

At `enqueue_route` the frontend validates:

- the client and both canonical fields
- service and scene authority, and lexical context
- exact portable kind
- direction: the source is outputOnly or inputOutput; the destination is
  inputOnly or inputOutput
- that both field services are supported

It then calls the backend's `do_check_live_route(from, to)`, which defaults to
admit. A backend uses it for restrictions the portable descriptors cannot
express. Installation at pump can still refuse, for example when the graph
changed after admission. Those refusals appear in the turn report and receipt,
not in the enqueue result.

## Installation order

| Case | Behaviour |
|---|---|
| No gate open | The request is one FIFO submission. The route installs before that turn's backend evaluation |
| Earlier submissions | Run without the route |
| Stored output | Not replayed on installation |
| Empty write vector | The route turn still performs the ordinary backend tick |
| Inside a released gate | All route requests install in their relative order before all retained writes and input occurrences in that submission, even writes enqueued before a route |
| Receipts in a gate | Share the submission and its outcome; cancelling one cancels the whole submission |

## Duplicates

An exact duplicate (same source node and field, same sink node and field) is
redundant and succeeds without effect (ISO/IEC 19775-1, 4.4.8.2).

- When a gate is released, the frontend keeps only the first copy of each route
  held in it. The duplicate's ticket shares the submission outcome.
- Deduplicating at release, not at enqueue, keeps cancellation of a held copy
  exact.
- A backend's `do_add_live_route` must return success without effect when the
  route is already installed.

## Failure effects

| Failure | Result |
|---|---|
| Refusal before any effect | Existing topology and the activation stay intact |
| A later installation or evaluation fails after a route in the same submission was installed | `partial_or_unknown`; the activation faults and later requests are cancelled |

No rollback of earlier installed routes is implied.

## Budgets

Each route call counts against `pending_requests` and `payload_bytes`; see
[Resource limits](resource-limits.md).

## Backend behaviour

| Backend | Installation | Extra refusals |
|---|---|---|
| Reference | Through its scene-edit transaction | None beyond the portable checks |
| Native | Updates executable state and the retained native document | See [Native provider](native-provider.md#live-route-restrictions) |

## Evidence

| Test | Covers |
|---|---|
| `sai_provider_contract` | FIFO ordering, gates, cancellation, authority, kind and direction, budgets, defaults, partial installation, redundant duplicates, admission pre-checks |
| `sai_provider_reference` | Next-turn delivery; duplicate no-ops alone and in a gate |
| `sai_provider_adopt_session` (`live_routes_on_adopted_session`) | Native: earlier versus later FIFO events, same-submission installation, deferred observation, admission-time refusals, duplicates, document and executable agreement |
