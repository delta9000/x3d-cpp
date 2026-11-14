---
title: SAI resource limits
summary: The admission budgets of provider::service, what they count and what they leave out.
tags: [sai, reference, provider]
updated: 2026-11-14
related:
  - submissions.md
---

# SAI resource limits

`resource_limits` is constructor configuration, passed to a factory such as
`make_reference_provider(limits)` or `native::make_service(limits)`.

## Defaults

| Member | Default | Bounds |
|---|---|---|
| `pending_requests` | 1,024 | Requests held or queued, including held gates and route calls |
| `payload_bytes` | 16 MiB | Accounted owning request payloads |
| `live_registrations` | 1,024 | Live observer registrations |
| `queued_notifications` | 4,096 | Notifications awaiting a drain |
| `notification_bytes` | 16 MiB | Accounted stored notification payloads |

A zero limit admits no work. How requests move through gates and turns is in
[Submissions, turns and notifications](submissions.md).

## What is counted

| Item | Accounting |
|---|---|
| Field write | One `provider::write` object, the canonical field-name bytes and the logical dynamic payload bytes |
| Zero-length payload | Still consumes its write and field-name storage |
| Route request | One `backend::admitted_route` object plus both canonical field-name byte counts |
| Notification | The stored notification object, canonical field bytes and logical dynamic value bytes |
| Held calls | Keep their accounting until processing or cancellation |

Arithmetic is checked for overflow.

## What is not counted

These are admission budgets, not total process-memory limits. Outside them:

- allocator overhead and spare capacity
- backend resources and topology storage
- application callback captures
- values, receipts and presentation frames retained by the consumer

Stored shared payload accounting is explicit. It does not claim the same
physical bytes were allocated once per reference.

## Behaviour at the limit

| Situation | Outcome |
|---|---|
| Predictable full queue | The call is rejected with `resource_limit` before backend entry; earlier accepted work is kept |
| Notification queue full before an observed turn | `pump` is rejected with `resource_limit` before backend entry; the receipt stays `submitted` and the same work can be pumped again after a drain |
| Overflow discovered during execution | Reported as `partial_or_unknown`; the activation faults. Events are never silently lost |

## Reclamation and cost

- Cancelling a request releases its capacity.
- Cancelled registrations are reclaimed on owner-thread cleanup.
- Retired scene records and terminal request history are not kept in immortal
  indexes.
- FIFO selection is constant-time in historical request count. Group cleanup is
  proportional to the group's per-call receipt count.
