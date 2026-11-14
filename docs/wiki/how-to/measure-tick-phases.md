---
title: Measure tick phases
summary: Get per-phase tick timings from the profile tool or from your own host.
tags: [how-to, execution, performance]
updated: 2026-11-14
related:
  - ../reference/native-host-contract.md
---

# Measure tick phases

## With the profile tool

Build with `X3D_CPP_BUILD_EXAMPLES=ON` and run `x3d_runtime_tick_profile`
(`tools/runtime_tick_profile.cpp`) with a scene and a positive tick count:

```sh
./build/x3d_runtime_tick_profile /path/to/scene.x3d 91
```

The tool parses the scene, creates a standard `RuntimeSession`, takes a full
snapshot and advances from zero at 30 Hz with a fixed authored viewer and no
pointer.

| Output | Contents |
|---|---|
| First lines | Parse, session and snapshot milliseconds |
| CSV rows, one per tick | Systems and cascade, post-cascade hooks, transforms, bounds, total tick, delta |
| Same rows | Placement and update counts, `lightsChanged` |

It fails if the extraction walk budget is exhausted.

## In your own host

Pass a `TickTimings` to the context tick:

```cpp
sdk::X3DExecutionContext::TickTimings timings;
session->context().tick(now, &timings);
```

It receives the same per-phase milliseconds. Plain `tick(now)` reads no
clocks, so leave the argument out in production paths you don't measure.

## Reading the numbers

Compare only runs built with identical settings. The timings are measurements
of one machine, not a real-time guarantee. What each phase does is in
[The tick model](../explanation/tick-model.md); the rules a host follows are in
the [native host contract](../reference/native-host-contract.md).
