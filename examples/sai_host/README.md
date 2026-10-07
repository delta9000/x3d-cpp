# Ordinary native SAI CPU host

This standalone consumer uses the ordinary provider service for scene setup,
routes, host turns, receipts, callbacks and teardown. The native presentation
feed supplies immutable mesh placements to a small in-memory CPU presenter.
It is a bounded local integration example, not a CAVEOS implementation or a
general renderer. It has no network access or background runtime loop.

The fixture is a finite local recording with an explicit host loop. It is
synthetic integration evidence, not a claim to reproduce a real CAVEOS workload.
The host creates two `Transform` placements of a shared `Shape`/`Box`, a second
pre-authored `Box`, and a `PositionInterpolator` routed to the left placement.

Installed build (the directory may be copied anywhere):

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/relocated/prefix
cmake --build build --target sai_cpu_host --parallel 1
./build/sai_cpu_host ./recorded-workload.txt
ctest --test-dir build --output-on-failure
```

For an explicit source build, pass both `-DX3D_CPP_SOURCE_DIR=/path/to/x3d-cpp`
and `-DX3D_SAI_SOURCE_DIR=/path/to/x3d-sai` instead of `CMAKE_PREFIX_PATH`.
No reference-provider target or testing header is used.

## What the run checks

- Setup, initialize-only geometry authoring, ROUTE installation, activation,
  input admission, update gates, portable reads and close use `provider::service`.
- Equal host times still produce distinct native ticks. Two calls behind a gate
  produce one submitted batch and two owning receipts; the final stateful
  translation wins while input-only fraction occurrences are admitted in order.
  Grouped input-only occurrences do not imply last-input-wins output: the host
  checks that the admitted native interpolation output and routed destination
  agree, allowing the runtime's timestamp-level event coalescing.
- Callbacks run only when the host explicitly calls `dispatch_notifications()`.
  The first single-fraction turn fills a two-notice budget using a translation
  listener and a diagnostic listener. The next turn is rejected before native
  execution with `resource_limit`; its receipt stays submitted. The host drains
  notifications, cancels the diagnostic listener and retries that same work and
  host time. Failures after native execution are reported, not blindly retried.
- Runtime work occurs on every successful pump; the CPU presenter samples tick
  zero for its baseline and then only ticks 3, 6, 9 and 12. A read of the feed at
  the rejected pump also checks that backpressure did not advance native state.
  Notification delivery is independent of presentation sampling. The example
  measures callback counts and does not require one output per input occurrence.
- The immutable frame has per-placement keys, per-mesh keys, local meshes and
  world transforms. Shared placements must have distinct placement keys and the
  same mesh key/allocation. A geometry swap changes mesh identity and bounds.
  Unchanged paths retain placement keys; a removed and re-added path gets a new
  key. Numeric keys are meaningful only within the frame's opaque scene source.
- A small CPU triangle rasterizer applies the frame's world matrices and a fixed
  host camera to a 96-by-72 color/depth buffer. It checks indices, vertex bounds,
  covered pixels, deterministic checksums, and visibly changed sampled frames.
  Both rendering and validation are active in Release builds.
- Close cancels a pending receipt; a completed receipt survives destruction.
  The baseline frame and its shared mesh still render to exactly the same pixels
  after geometry swaps, removal/re-addition, close and service destruction.

The presenter intentionally handles only bounded indexed triangle meshes. It
does not implement lighting, textures, transparency, clipping, cameras, input
devices or a general X3D renderer. It exports no files; image data stays in memory.

## Local recording format

Each non-comment line begins with a finite nonnegative host time in seconds,
followed by one operation. Times must be nondecreasing; equal times are valid.

```text
0.00 fraction 0.125    # send an input-only fraction in [0,1]
0.00 fraction 0.25
0.10 group 0.35 0.50  # two admitted calls behind one update gate
0.20 swap large        # choose the other already-authored Box
0.20 fraction 0.75
0.30 remove right
0.40 add right
0.40 swap small
0.50 fraction 1.00
0.60 fraction 0.50
0.60 group 0.25 0.00
0.70 idle              # pump behavior without a queued request
```

Only an explicitly supplied regular local file is read. Input is limited to
64 KiB, 256 bytes per line and 120 turns. The replay protocol requires a multiple
of three turns and starts with two distinct single-fraction operations to
demonstrate notification backpressure. A successful recording must cover equal
times, a group, swaps in both directions, removal and re-addition, and sampled
large geometry and removal. The supplied fixture provides this coverage.

Bad numbers, out-of-range fractions, unknown operations, trailing tokens and
oversized inputs exit nonzero with an ordinary error message. CTest runs both
the complete recording and ten malformed-input checks. A typical successful
run prints each presented tick's placement count, covered pixels and checksum,
then `PASS` with measured turn, receipt, callback and backpressure counts.
