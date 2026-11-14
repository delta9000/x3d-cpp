---
title: Run a SAI scene on the native runtime
summary: Tutorial that builds the headless host example, replays a recording through the native backend and changes the input.
tags: [sai, tutorial, native]
updated: 2026-11-14
related:
  - first-scene.md
  - ../reference/host-example.md
---

# Run a SAI scene on the native runtime

In [Animate your first scene](first-scene.md) the reference backend evaluated
your scene. In this tutorial the real x3d-cpp runtime does it. You will build a
small headless host, replay a recording through it, and watch the rendered
frames respond when you change the input.

You need a C++20 compiler, CMake 3.24 or newer, and Ninja. Run everything from
the x3d-cpp checkout root. The first build compiles the runtime and takes a few
minutes.

## 1. Build the host

The host is `examples/sai_host`. Point it at the checkout so it builds the SAI
SDK, the runtime and the native adapter from source:

```sh
cmake -S examples/sai_host -B build-sai-host -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DX3D_CPP_SOURCE_DIR="$PWD" \
  -DX3D_SAI_SOURCE_DIR="$PWD/sai"
cmake --build build-sai-host --target sai_cpu_host
```

## 2. Replay the supplied recording

```sh
./build-sai-host/sai_cpu_host examples/sai_host/recorded-workload.txt
```

You should see output of this shape (your checksums can differ by platform):

```text
backpressure: drain=2 retry=same-submission
frame tick=3 time=0.1 placements=2 covered=479 checksum=9045358025970874193
frame tick=6 time=0.3 placements=1 covered=389 checksum=12771844129953350325
frame tick=9 time=0.5 placements=2 covered=478 checksum=11097066408944049082
frame tick=12 time=0.7 placements=2 covered=478 checksum=1512864877897629302
PASS turns=12 presentations=4 receipts=13 callbacks=7 backpressure=1 retained-frame=unchanged
```

The host built a scene with two boxes and an interpolator, ran twelve turns,
and rasterized four of them on the CPU. Nothing was drawn on screen; each
`frame` line summarizes an in-memory image.

## 3. Read the recording

Open `examples/sai_host/recorded-workload.txt`:

```text
0.00 fraction 0.125
0.00 fraction 0.25
0.10 group 0.35 0.50
0.20 swap large
0.20 fraction 0.75
0.30 remove right
0.40 add right
0.40 swap small
0.50 fraction 1.00
0.60 fraction 0.50
0.60 group 0.25 0.00
0.70 idle
```

Each line is one host turn: a time, then an operation. Match them to the
output:

- Twelve lines gave `turns=12`.
- The host presents every third tick, which is why you saw ticks 3, 6, 9
  and 12.
- At tick 6 the line `0.30 remove right` has just run, so the frame has
  `placements=1`. By tick 9 `add right` has put it back.

## 4. Change an input

Copy the recording and change the fraction sent on the ninth line, the turn that
becomes tick 9:

```sh
cp examples/sai_host/recorded-workload.txt my-workload.txt
sed -i 's/^0.50 fraction 1.00/0.50 fraction 0.80/' my-workload.txt
./build-sai-host/sai_cpu_host my-workload.txt
```

Compare the `tick=9` line with the earlier run. Its checksum is different and
the others are the same. Your new fraction went in through
`service.enqueue`, the native interpolator turned it into a position, a ROUTE
carried that to the left box's `Transform`, and the presentation feed handed
the moved box to the rasterizer.

## 5. Send something invalid

```sh
sed -i 's/^0.50 fraction 0.80/0.50 fraction 1.50/' my-workload.txt
./build-sai-host/sai_cpu_host my-workload.txt
```

```text
sai_cpu_host: workload line 11: fraction must be finite and in [0,1]
```

The host rejects the file before it touches the scene.

## 6. Find the contract in the host

Open `examples/sai_host/main.cpp` and look for these calls. You met most of
them in the first tutorial:

| Call | What it does here |
|---|---|
| `native::make_presented_scene(limits)` | Creates the native service, a scene and its presentation feed |
| `service.activate`, `service.connect` | Start the scene and attach the host as a client |
| `service.begin_update` / `end_update` | Send the two fractions of a `group` line as one submission |
| `service.pump` | Runs one native tick per recording line |
| `service.dispatch_notifications` | Delivers the queued callbacks |
| `presented.feed.snapshot(service)` | Pulls the immutable frame to rasterize |

The first line of output, `backpressure: drain=2 retry=same-submission`, comes
from a deliberately tiny notification budget. The host's second pump is
refused because two notifications are waiting, so it drains them and pumps the
same work again.

## What you did

You ran the same provider contract as the first tutorial on the native
runtime, drove it from a host loop you could read, and saw input flow through
interpolation, routing and extraction into rendered frames.

## Where next

- [Build and link the native backend](../how-to/use-the-native-backend.md) in
  your own project
- [Present frames from the native feed](../how-to/present-frames.md)
- [Adopt a loaded session](../how-to/adopt-a-session.md) to drive a scene you
  loaded from a file
- [Headless host example](../reference/host-example.md) for the recording
  format and everything the run checks
