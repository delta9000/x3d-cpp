---
title: "ADR-0006: Generated Layer as Compiled Static Library"
summary: The generated bindings are compiled as a static library (x3d_cpp_nodes), yielding ~17x cold-build speedup; PCH was not pursued.
tags: [adr, static-lib, build, compile-time]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/generated-bindings.md
---

# ADR-0006: Generated Layer as Compiled Static Library

## Status

Accepted — 2026-06-16

## Context

Before this change (C1), every generated `<Node>.hpp` was fully inline: the class, its member declarations, and the heavy `fields()` reflection body — a function-local-static `FieldTable` that constructs up to four `std::function` thunks per field (typed get, data set, enum string-get, enum string-set). Because the library was a CMake `INTERFACE` target with no compiled artifact, every translation unit that included a node header re-parsed, re-instantiated, and re-codegen'd all of it.

A `clang -ftime-trace` profile of one representative factory-pulling TU (`runtime/parse/tests/json_reader_test.cpp`, 47.8 s cold) quantified where the time went:

| Bucket | Share |
|---|---|
| `InstantiateFunction` | 41 % |
| `PerformPendingInstantiations` | 36 % |
| Backend (codegen, even at `-O0`) | 28 % |
| `Source` (raw parse) | 26 % |

Template instantiation of the `std::function` thunks — not parsing — was the dominant cost, and it was repeated in approximately 48 factory-pulling TUs. This is precisely why a shared PCH measured no improvement (1296 s → 1260 s, ~0%): a PCH avoids re-parsing but not instantiation or codegen, which together account for ~70 % of the cost.

A compounding effect: `X3DNodeFactory.hpp` `#include`d 265 of the 342 node headers (~178 k LOC) so it could construct nodes by name. Any TU that pulled the factory paid that parse cost in full, independently. The per-TU memory of these inline-heavy compiles forced the Ninja compile-job pool to `-j4` on a 16-core / 60 GB host (OOM-kill above that level), throttling parallelism on top of the already-high per-TU wall time.

The full problem statement and profiling data are in `docs/superpowers/specs/2026-06-16-c1-decl-def-split-design.md`.

## Decision

We split each generated node into a header + a translation unit and compiled all generated `.cpp` files once into a CMake `STATIC` library named `x3d_cpp_nodes`.

Concretely:

- `<Node>.hpp` retains the class definition, member declarations (brace-initialized to spec defaults), inline trivial accessors (`getX`/`setX`/`setXUnchecked`), small statics, and **declarations** (no bodies) of the heavy virtuals: `nodeTypeName()`, `defaultContainerField()`, `fields()`, `accept()`, `validateRanges()`, and the protected range-checker statics.
- `<Node>.cpp` holds the **definitions** of those heavy members — principally the `FieldTable` construction with its four `std::function` thunks per field, `validateRanges()`, range checkers, and `accept()`. The function-local-static `FieldTable` and its thunks are therefore instantiated and codegen'd exactly once.
- `X3DNodeFactory` was split in parallel (the C2 fold): the header holds only the `create(const std::string&)` entry point; all 265 node `#include`s and the name-to-constructor table moved into a generated `X3DNodeFactory.cpp`, also compiled into the lib. This removes the 265-header parse from every factory-pulling consumer TU.
- The existing `x3d_cpp` CMake `INTERFACE` target gained a link dependency on `x3d_cpp_nodes`, so all in-tree and `find_package` consumers link the lib and stop re-instantiating the definitions automatically.
- The golden gate was extended to cover `*.cpp` alongside `*.hpp`; the hash changed once, by design.

The five landed commits are `843a5bd` (gate prep), `38d043b` (factory split), `0a50257` (CMake static lib), `a753c15` (per-node codegen split), and `40662cf` (golden + A/B record).

PCH was explicitly rejected as a remediation path: the profile proves it attacks the wrong bottleneck.

## Consequences

**Positive:**

- Cold-build time dropped from 1296 s to 76 s (~17×) at `-j4` with ccache disabled — measured A/B on the same machine. A subsequent `-j` raise (commit `719a6dd`) brought it further to 39 s (~33× cumulative), unlocked because per-compile peak RSS fell from OOM territory to ~0.86 GB (measured), removing the memory pressure that had forced the pool to 4.
- Consumer TUs are lighter: they include the lean header, not the thunk-instantiation cost. This makes adding new test executables and runtime subsystems cheap.
- The Ninja compile-job pool (`X3D_CPP_COMPILE_JOBS`, `CMakeLists.txt:54`) now defaults to the logical core count rather than a hard cap; it is a documented tunable (`-DX3D_CPP_COMPILE_JOBS=N`) for low-RAM hosts.
- The SDK ships a concrete artifact: `libx3d_cpp_nodes.a` + headers, installed via `install(TARGETS x3d_cpp x3d_cpp_nodes x3d_cpp_sdk …)`. `find_package(x3d_cpp)` consumers receive the link dependency transitively.
- The `dev` preset sets `X3D_CPP_PER_HEADER_CHECKS=OFF`, reducing local ctest from ~459 to 66 (aggregate header check + behavior tests), while CI keeps the full per-header isolation coverage.

**Trade-offs / costs:**

- Consumers that previously used a hand-rolled `-I`-only include path (not `find_package`, not `target_link_libraries(... x3d_cpp::x3d_cpp)`) must now also link `x3d_cpp_nodes`. The CMake target carries this transitively, so all in-tree and standard downstream consumers are unaffected. A `--header-only` codegen escape hatch is reserved but not built (YAGNI).
- The golden hash changed once across all generated files (the bodies moved). The regen-and-commit procedure absorbs this, but reviewers must understand the hash is intentionally new, not a correctness regression.
- Static-init / ODR: moving the function-local-static `FieldTable` into a `.cpp` is ODR-safe (one definition, one TU). No cross-TU static-init-order dependency is introduced; the table is still lazily built on first `fields()` call. The thunks in `.cpp` retain access to the full class via the node header they include.
- The existing CI pipeline (`.github/workflows/ci.yml`) used raw `cmake` + Make (serial); switching CI to Ninja and wiring `X3D_CPP_PER_HEADER_CHECKS=ON` is a documented follow-up, not part of this change.

## Related

- [Architecture](../architecture.md)
- [Generated Bindings subsystem](../subsystems/generated-bindings.md)
- Design spec: `docs/superpowers/specs/2026-06-16-c1-decl-def-split-design.md`
- Implementation plan: `docs/superpowers/plans/2026-06-16-c1-decl-def-split.md`
- Live CMake wiring: `CMakeLists.txt` lines 26–101 (linker selection, job pool, `x3d_cpp_nodes` target)
