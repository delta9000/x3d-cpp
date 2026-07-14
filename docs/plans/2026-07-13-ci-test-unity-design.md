# CI test unity batching design

## Context

Phase-one CI scoping reduced the normal pull-request gate to the targets that
own behavioral CTests. A cold local build of that aggregate still produced 230
compile commands and a 3.4 GB build tree. Test execution itself took only 0.14
seconds for all 45 CTests, so the remaining cost is compilation rather than
runtime coverage.

The cold Debug ASan/UBSan behavior build was more severe: approximately 24 GB.
Most of that live-tree footprint is repeated instrumented code and unrestricted
debug information, not data needed to execute the sanitizer gate.

Five doctest executables account for roughly 148 of those compile commands:
the geometry/scene, codecs, parse, extract, and events suites. Each source file
reparses the same large public runtime headers even though the files are linked
into only five binaries.

## Decision

Enable CMake unity builds only for those five grouped test executables. Limit
unity batches to eight sources and request a unique identifier for each source
included in a generated unity translation unit. Preserve every source file,
executable, CTest registration, label, and assertion.

Keep the sanitizer preset at `Debug`, but request minimal level-one debug
information for project targets. This retains symbolized source locations and
function names in sanitizer reports without emitting the full variable and
type records that CI never inspects.

The generated node library keeps its existing unity configuration. Production
libraries, small one-source test executables, header compile contracts, fuzz
targets, examples, and opt-in backend tests are unchanged.

## Why this approach

Unity batching removes redundant parsing while retaining the full behavioral
gate. It is safer than selecting behavior tests from a hand-maintained header
dependency map, and it provides an immediate result before the deeper Phase 2
work moves hand-written runtime implementations out of public headers.

Changed-subsystem-only behavior jobs remain a possible later optimization, but
they are not needed until measured cold builds show that compiling the complete
batched behavior suite is still too expensive.

## Validation

Shape tests will require all five grouped suites to opt into unity builds with
the agreed batch bound and unique source identifier. A clean CI-preset build
must then:

- build `x3d_behavior_tests` successfully with warnings as errors;
- pass all `behavior`-labeled CTests;
- materially reduce compile-command count from the 230-command baseline;
- reduce the cold build tree from the 3.4 GB baseline, or document why object
  layout prevents a disk reduction despite fewer compiler invocations.
- pass the same 45 behavior tests under ASan/UBSan; and
- materially reduce the cold sanitizer tree from its approximately 24 GB
  baseline, with a 10 GB maximum target for the live build tree.

If unity compilation exposes an actual symbol collision or order dependency,
fix the test source isolation when straightforward. Otherwise exclude only the
affected source from unity and document the exception adjacent to the target.

Phase 2 remains the structural solution for production and downstream compile
cost: it will migrate measured non-template implementations from public headers
into the compiled runtime topology.
