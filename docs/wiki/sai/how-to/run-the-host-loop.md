---
title: Run the SAI host loop
summary: Queue input, group it behind an update gate, pump turns, check outcomes, drain notifications and shut down.
tags: [sai, how-to, provider, events]
updated: 2026-11-14
related:
  - ../reference/submissions.md
  - ../reference/resource-limits.md
---

# Run the SAI host loop

The service never runs on its own. Your host decides when input is evaluated
and when callbacks run. This guide assumes an activated scene:

```cpp
auto active = service.activate(scene);   // result<activation>
auto client = service.connect(*active);  // result<client>
```

Snippets below omit error checks on results that are not the point of the step.
Exact rules are in [Submissions, turns and notifications](../reference/submissions.md).

## Queue one input

```cpp
auto receipt = service.enqueue(
    *client, interpolator, bindings::PositionInterpolator::set_fraction, 0.5f);
```

A successful `enqueue` means *accepted*, not applied. Keep the ticket if you
want to know what happened.

## Group several writes into one submission

```cpp
service.begin_update(*client);
auto first  = service.enqueue(*client, mover, bindings::Transform::translation,
                              sai::vec3f{1, 0, 0});
auto second = service.enqueue(*client, mover, bindings::Transform::translation,
                              sai::vec3f{2, 0, 0});
service.end_update(*client);
```

Both calls are evaluated in the same turn and share one outcome. For this
inputOutput field the last accepted value wins.

## Pump a turn

```cpp
auto report = service.pump(*active, {now});
if (!report) {
  // The pump itself was refused, for example resource_limit or wrong_thread.
} else if (report->error) {
  // The turn ran and failed. Check report->state and report->effects.
}
```

Each call evaluates at most one submission. Call it again to process the next
one. `now` must be finite and must not decrease; equal times are fine. A pump
with nothing queued still advances behaviour.

## Check the outcome

Check all three, because a successful outer result can carry a failed turn:

| Check | Tells you |
|---|---|
| `report->error` | Whether this turn failed |
| `report->state` | `active`, or `faulted` after partial or unknown effects |
| `receipt->status()` | The request's `state`, `turn`, `effects` and `error` |

```cpp
auto status = receipt->status();
if (status && status->state == provider::request_state::processed) { /* done */ }
```

## Deliver callbacks

```cpp
auto subscription = service.observe(*active, translation,
    [&](const provider::notification &notice) {
      latest = std::get<sai::vec3f>(notice.value);
    });

// ... after one or more pumps:
service.dispatch_notifications();
```

No callback runs inside `pump`. Callbacks run on the thread that calls
`dispatch_notifications`. Keep the `subscription` alive for as long as you want
callbacks; destroying it cancels.

## Handle backpressure

If the notification queue is full, the next observed `pump` is refused with
`resource_limit` before anything executes, and the receipt stays `submitted`.

1. Call `dispatch_notifications()`.
2. Pump again with the same host time. The same work is still at the head of
   the queue.

Raise or lower budgets through
[resource limits](../reference/resource-limits.md) when you construct the
service.

## Decide whether to retry

| Outcome | Do |
|---|---|
| Refused before execution (`resource_limit`, release-time rejection) | Fix the cause and retry |
| Submission failed with `failure_effect::none` | The activation is healthy; carry on with later work |
| `partial_or_unknown`, activation `faulted` | Do not replay. Retire the scene |

## Cancel work

```cpp
service.cancel(*client, *receipt);
```

A held call can be cancelled individually. Cancelling a call from a released
gate cancels its whole submission.

## Shut down

```cpp
subscription->cancel();
service.retire(scene);
service.close();
```

Run mutation and shutdown on the service's owner thread. `close()` returns an
error on the wrong thread or during a turn. Receipts you kept still report
their final state after the service is gone.
