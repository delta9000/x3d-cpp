# Compile Topology Follow-up Results

**Date:** 2026-07-14
**Branch:** `perf/compile-contract-factory-phase2`
**Baseline:** merged PR #54 (`0f439d6`)

## Method

Before and after builds used GCC 16.1.1, Ninja, the `ci` preset with Werror,
shared generated nodes, 16 build jobs, fresh `/tmp` build directories, and
`CCACHE_DISABLE=1`. Whole-target measurements came from `/usr/bin/time -v`.
Per-object durations came from Ninja's log and represent elapsed compiler time;
their sum is useful as cumulative/serial compiler work even though the build ran
in parallel.

The baseline source was the unchanged merged PR #54 worktree. The after source
was this branch. Documentation commits do not affect either generated graph.

## Header compile-contract graph

| Measurement | Before | After | Change |
|---|---:|---:|---:|
| Commands reachable from `x3d_compile_contracts` | 454 | 409 | -45 (-9.9%) |
| Generated-node object commands | 44 | 0 | -100% |
| Node-library command mentions | 3 | 0 | -100% |
| Cold wall time | 32.16 s | 18.94 s | -41.1% |
| Cold user CPU | 422.54 s | 247.08 s | -41.5% |
| Cold system CPU | 15.90 s | 11.21 s | -29.5% |
| Maximum RSS | 1,004,720 KiB | 591,236 KiB | -41.2% |

The 409 remaining commands are the legitimate all-headers compile/link plus one
translation unit per public header. Ninja's command graph contains neither
`x3d_cpp_nodes.dir` nor `libx3d_cpp_nodes`. Header diagnostics and coverage are
unchanged; the job simply no longer builds an implementation library it cannot
use.

The installed-consumer smoke moved to the behavior label. It still installs,
configures, builds, and runs the downstream embed example, but now executes in
the job that has already built the linked SDK runtime.

## Doctest entry point

| Measurement | Before | After |
|---|---:|---:|
| Default CI compile commands for `doctest_main.cpp` | 5 | 1 |
| Optional + default source-list occurrences | 8 | 1 |

All five default grouped suites link `libx3d_doctest_main.a`; their runtime tests
pass. Optional text, texture, and movie suites use the same library when their
backends are enabled. Sanitizer configurations instrument the one library once
through the existing directory-wide sanitizer compile options.

## Generated node factory

`X3DNodeFactory.cpp` remains in `unity_37` under the current stable source
ordering, so its before/after unity object is directly comparable.

| Measurement | Before | After | Change |
|---|---:|---:|---:|
| Factory unity compile | 17.867 s | 2.919 s | -83.7% |
| Factory unity preprocessed lines | 149,281 | 82,746 | -44.6% |
| Factory unity object | 10,116,312 B | 1,575,088 B | -84.4% |
| All node unity objects, cumulative compiler time | 203.762 s | 182.349 s | -10.5% |
| Longest node unity compile | 17.867 s | 6.538 s | -63.4% |

The longest object is no longer the factory batch. The new maximum is
`unity_8`; work is distributed among existing per-node sources instead of
concentrated in one translation unit that includes every concrete header.

The factory's public API and registry contents are unchanged. Shared and static
node-library configurations both build; SDK facade, parser behavior, and the
static installed-consumer smoke all pass. Static self-registration was avoided,
so archive dead-stripping cannot remove registry entries.

## Conclusion

This branch removes an accidental 41% cold header-job cost and the largest node
compile straggler without weakening a contract or changing a consumer API. The
factory redesign also reduces total generated-node compiler work by 10.5%, not
just critical-path latency.

The remaining large cost is the implementation-heavy public runtime headers.
That work stays in a separate architectural PR: a shared `x3d_cpp_runtime`,
migrated subsystem by subsystem with link-level coverage and measured stop
conditions. Mixing that ABI/ownership change into these graph and generator
fixes would make both review and performance attribution worse.
