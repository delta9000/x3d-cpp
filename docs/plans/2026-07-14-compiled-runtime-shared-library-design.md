# Compiled Runtime Shared Library Design

**Date:** 2026-07-14
**Status:** Approved for autonomous implementation
**Base:** `origin/main` at merge commit `4887c72` (PR #55)

## Context

PRs #54 and #55 made generated node definitions shared, removed the node
library from compile-only contracts, reused one doctest entry point, and
distributed node-factory construction. The remaining dominant cost is the
hand-written runtime: approximately 29,800 lines of public runtime headers,
with parsing, codecs, extraction, scene behavior, and event systems still
defined inline.

The fetched baseline is clean (`345 passed, 1 skipped`). A cold, ccache-disabled
`x3d_behavior_tests` build on GCC 16.1.1 and Ninja took 73.26 seconds wall,
982.81 seconds user CPU, and 1,114,596 KiB peak RSS. Cumulative compiler time in
the grouped tests was:

| Group | Compiler time |
|---|---:|
| extraction | 160.082 s |
| events | 166.051 s |
| codecs | 145.557 s |
| parse | 65.491 s |
| geometry/scene | 76.970 s |

A syntax-only `x3d/sdk.hpp` probe expands to 150,061 lines, consumes 571 MiB,
and uses 2.97 seconds of compiler CPU. The public parse, codec, extraction, and
execution headers each independently expand beyond 107,000 lines because their
implementation dependencies overlap heavily.

## Alternatives considered

### One monolithic runtime DSO

Put writers, parsers, extraction, events, scene systems, and generated nodes in
one shared library. This produces the fewest artifacts, but destroys the slim
authoring boundary: a writer-only consumer would acquire parsing, simulation,
and extraction dependencies. It also makes the generated layer and
hand-written layer impossible to measure or replace independently.

### Public shared library per subsystem

Export separate codec, parse, extraction, scene, and event DSOs. This maximizes
incremental linking but exposes internal subsystem coupling as permanent package
surface. Consumers would need to understand a graph that should remain an SDK
implementation detail, and cross-subsystem moves would become ABI migrations.

### Acyclic implementation layers (selected)

Export three implementation layers while keeping the existing facades:

```text
x3d_cpp_headers
  <- x3d_cpp_nodes
  <- x3d_cpp_authoring_runtime
  <- x3d_cpp_runtime
```

`x3d_cpp_authoring_runtime` owns compiled field formatting and writers required
by the slim authoring facade. `x3d_cpp_runtime` owns parsing, extraction, scene,
and event implementations. Internally, its source list remains grouped by
subsystem so migration and performance attribution stay independent. Normal
consumers still link only `x3d_cpp::authoring`, `x3d_cpp::x3d_cpp`, or
`x3d_cpp::sdk`.

This is the selected approach because it keeps package ownership accurate
without turning implementation folders into public link APIs.

## Linkage and package topology

All three implementation layers are shared by default. The existing
`X3D_CPP_SHARED_NODES=OFF` compatibility mode switches nodes, authoring runtime,
and full runtime to static together. Mixed shared/static implementation layers
are intentionally not supported in this phase: they either duplicate generated
symbols across DSOs or require position-independent static archives that can be
absorbed by more than one shared consumer.

The shared targets receive the project `VERSION` and major `SOVERSION`, plus
Windows automatic symbol export consistent with `x3d_cpp_nodes`. All targets
are installed and exported as `x3d_cpp::nodes`,
`x3d_cpp::authoring_runtime`, and `x3d_cpp::runtime`.

Facade composition becomes:

- `x3d_cpp::authoring` -> `x3d_cpp::authoring_runtime`;
- `x3d_cpp::x3d_cpp` -> `x3d_cpp::runtime`;
- `x3d_cpp::sdk` -> `x3d_cpp::x3d_cpp`.

The implementation libraries expose include requirements transitively, so a
downstream consumer never needs a private source-tree path. The installed
consumer gate must exercise both authoring and SDK facades.

## Migration boundary

The branch must finish with real runtime code in the libraries; anchor-only
targets are allowed only during the first topology commit and are removed as
soon as the first implementation source lands.

The minimum meaningful migration is:

1. non-template `FieldValueIO` and the four public codec writers into the
   authoring runtime;
2. reader implementations, node-building helpers, and parse front-door/resolver
   loops into the full runtime;
3. non-template `MeshBuilder` algorithms and dispatch into the full runtime.

Further SceneExtractor, scene-system, and event-system waves are admitted only
when they remain reviewable and have independent link-level coverage. They are
not required merely to make the new target look larger. The branch stops once
the minimum migration is complete, correctness gates are green, and the
affected target groups improve by at least 10% cumulatively; remaining systems
then become a follow-up against a proven ABI boundary.

Templates, `constexpr` functions, tiny value accessors, abstract seams, and
generic callback adapters remain in headers. Public signatures and class data
layout remain unchanged in the first pass. PImpl is deferred unless measurement
shows a specific private type prevents a meaningful include reduction; it will
not be introduced speculatively.

## Dependency and include rules

Every compiled source is listed explicitly in CMake. Runtime sources may include
the concrete readers, generated node types, parser state, streams, algorithms,
and containers they implement. Public headers retain only dependencies required
to declare their public and private class layout.

Removing implementation-only includes is part of each migration wave, not a
global include purge. Any consumer that relied on an accidental transitive
include must add the declaring header explicitly. Header isolation remains the
oracle for self-contained public headers.

The full runtime may depend on the authoring runtime and nodes; the authoring
runtime must not depend on parsing, events, scene systems, or extraction. This
preserves the existing authoring footprint and prevents a dependency cycle.

## Correctness and ABI coverage

Tests will first pin the configured and installed target graph, shared/static
artifact forms, facade linkage, and absence of source-tree paths from exports.
Each implementation wave then gets a link-boundary test that calls the moved
API through a public facade. The test must demonstrate an unresolved reference
when the required compiled layer is deliberately omitted before the production
link is added.

Required gates are:

- complete pytest and generated golden-tree checks;
- behavior and compile-contract aggregates;
- header isolation after public include slimming;
- shared and all-static library configurations;
- installed SDK and authoring consumers;
- CPU raster, real GL PoC, and asset-import consumers;
- ASan/UBSan behavior suite;
- fuzz target for parse-front-door changes.

No serialization bytes, parse results, tessellation order, floating-point
operation order, event sequencing, or public source signature may change as a
side effect of compilation ownership.

## Performance acceptance

Before/after builds use the same compiler, Ninja job count, CI preset, and
`CCACHE_DISABLE=1` with fresh build directories. Record:

- total behavior target wall/user/system time and peak RSS;
- cumulative compiler time for codecs, parse, extraction, events, and
  geometry/scene groups;
- `x3d/sdk.hpp` preprocessing lines, syntax CPU time, and peak RSS;
- compiled runtime library sizes and complete build-tree disk usage;
- hosted normal and sanitizer CI wall time after publication.

The minimum acceptance threshold is a 10% cumulative compiler-time reduction
across the groups touched by the retained migration waves, without increasing
peak RSS or weakening any gate. A wave that does not materially reduce parsing
or repeated code generation is reverted rather than justified by architecture
alone.
