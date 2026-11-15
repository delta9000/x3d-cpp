---
title: The tick model
summary: Why the runtime is single-threaded, host-driven and pull-based, and what that means for deltas.
tags: [explanation, execution, embedding]
updated: 2026-11-14
related:
  - architecture.md
  - ../reference/native-host-contract.md
---

# The tick model

The runtime never runs on its own. It has no thread, no timer and no event
loop. The host calls `tick(now)` when it wants the scene to advance and pulls
what changed afterwards. This page explains that shape; the exact rules are in
the [native host contract](../reference/native-host-contract.md), and the
layers are in [Architecture](architecture.md).

## One thread, one direction

A loaded document has one `X3DExecutionContext` and one `SceneExtractor`, and
one thread drives both. Each frame runs in the same order: the host pushes
input, ticks once, then takes one delta. `RuntimeSession` packages the three
objects, but the order is the same if you hold them yourself.

Keeping it synchronous makes a tick a closed unit. Inside it the runtime
advances the clock, updates every registered `System`, drains the ROUTE event
cascade until nothing more fires, then propagates dirty transforms and bounds.
When `tick` returns, the pull surface (world transforms, bounds, the view
matrix, the bound nodes, picking) is consistent with that one instant. A host
never sees half a cascade.

## Time belongs to the host

`now` is seconds on whatever monotonic clock the host chooses. The runtime
doesn't read a clock, which is what lets the same scene run in real time, in
fixed simulation substeps, or frame by frame in an offline renderer. Repeating
a `now` value is allowed.

## Deltas describe one tick

The dirty set that feeds extraction covers exactly one tick, and the next tick
clears it. A delta is therefore a description of the last tick, not a history.
Two consequences follow:

- A host that ticks without taking the delta has lost those changes and needs
  a fresh full snapshot.
- A second `delta()` with no tick in between is empty, because nothing can have
  changed. The guard keys on a tick counter, not on the clock, which is why
  repeated `now` values are safe.

The first `delta()` before any `fullSnapshot()` returns the snapshot, since
that is the baseline everything later is relative to.

## Callbacks run on your thread

Resource callbacks run synchronously inside the call that needed them, so slow
IO blocks the host's thread. The runtime doesn't start background work to hide
that. Where a result type allows it, a callback can answer `Pending` and the
extractor asks again on later ticks; the host still owns the loading, the
timeout and the cancellation. [Seams, ports and backends](seams.md) explains
why resource IO sits outside the core at all.
