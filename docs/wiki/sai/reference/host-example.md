---
title: SAI headless host example
summary: Reference for examples/sai_host, covering what its run checks, the recording format and input limits.
tags: [sai, reference, native, example]
updated: 2026-11-14
related:
  - ../tutorials/native-host.md
---

# SAI headless host example

`examples/sai_host` is a standalone consumer of the native backend. It uses the
ordinary provider service for setup, routes, host turns, receipts, callbacks
and teardown, and the [presentation feed](presentation-feed.md) for a small
in-memory CPU presenter. The walkthrough is
[Run a scene on the native runtime](../tutorials/native-host.md).

It is a bounded local integration example with synthetic input. It has no
network access and no background runtime loop, and it uses no
reference-provider target or testing header.

## Scene

- two `Transform` placements of a shared `Shape`/`Box`
- a second pre-authored `Box`
- a `PositionInterpolator` routed to the left placement

## Files

| File | Role |
|---|---|
| `main.cpp` | Host loop |
| `workload.hpp` | Recording parser |
| `cpu_presenter.hpp` | CPU triangle rasterizer |
| `recorded-workload.txt` | The supplied recording |
| `parser-rejections.cmake` | Malformed-input CTest cases |

## What the run checks

| Area | Check |
|---|---|
| Contract | Setup, initializeOnly geometry authoring, ROUTE installation, activation, input admission, update gates, portable reads and close all go through `provider::service` |
| Time | Equal host times still produce distinct native ticks |
| Gates | Two calls behind a gate produce one submitted batch and two owning receipts. The final stateful translation wins; inputOnly fraction occurrences are admitted in order |
| Coalescing | Grouped inputOnly occurrences do not imply last-input-wins output. The host checks that the native interpolation output and routed destination agree, allowing timestamp-level event coalescing |
| Callbacks | Run only when the host calls `dispatch_notifications()` |
| Backpressure | The first turn fills a two-notice budget. The next turn is rejected before native execution with `resource_limit` and its receipt stays `submitted`. The host drains, cancels the diagnostic listener and retries the same work at the same host time |
| Sampling | Runtime work happens on every successful pump. The presenter samples tick zero and then ticks 3, 6, 9 and 12. A feed read at the rejected pump confirms backpressure did not advance native state |
| Frames | Per-placement keys, per-mesh keys, local meshes and world transforms. Shared placements have distinct placement keys and the same mesh key. A geometry swap changes mesh identity and bounds. A removed and re-added path gets a new key |
| Rasterizer | Applies world matrices and a fixed host camera to a 96×72 colour and depth buffer; checks indices, vertex bounds, covered pixels, deterministic checksums and visibly changed sampled frames |
| Shutdown | Close cancels a pending receipt; a completed receipt survives destruction. The baseline frame still renders to the same pixels after geometry swaps, removal and re-addition, close and service destruction |

Rendering and validation are both active in Release builds. Failures after
native execution are reported, not blindly retried.

## Presenter limits

The presenter handles only bounded indexed triangle meshes. It has no lighting,
textures, transparency, clipping, cameras or input devices, and it exports no
files; image data stays in memory.

## Recording format

Each non-comment line starts with a finite nonnegative host time in seconds,
followed by one operation. Times must be nondecreasing; equal times are valid.

```text
0.00 fraction 0.125    # send an input-only fraction in [0,1]
0.00 fraction 0.25
0.10 group 0.35 0.50   # two admitted calls behind one update gate
0.20 swap large        # choose the other already-authored Box
0.20 fraction 0.75
0.30 remove right
0.40 add right
0.40 swap small
0.50 fraction 1.00
0.60 fraction 0.50
0.60 group 0.25 0.00
0.70 idle              # pump behaviour without a queued request
```

| Operation | Arguments |
|---|---|
| `fraction` | One value in [0, 1] |
| `group` | Two fractions, sent behind one update gate |
| `swap` | `large` or `small` |
| `remove`, `add` | `right` |
| `idle` | None |

## Input limits and protocol

| Limit | Value |
|---|---|
| Input source | One explicitly supplied regular local file |
| File size | 64 KiB |
| Line length | 256 bytes |
| Turns | 120, and a multiple of three |

A recording must start with two distinct single-fraction operations (to show
notification backpressure) and must cover equal times, a group, swaps in both
directions, removal and re-addition, and sampled large geometry and removal.

Bad numbers, out-of-range fractions, unknown operations, trailing tokens and
oversized inputs exit nonzero with an ordinary error message.

## Output and tests

A successful run prints each presented tick's placement count, covered pixels
and checksum, then `PASS` with measured turn, receipt, callback and
backpressure counts. CTest runs the complete recording and ten malformed-input
checks.
