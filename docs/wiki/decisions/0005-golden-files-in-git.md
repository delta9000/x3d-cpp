---
title: "ADR-0005: Golden Files Committed to Git"
summary: Generated headers are committed as golden files in git, requiring codegen to be deterministic; drift is intentional and gated.
tags: [adr, golden-files, determinism, codegen, git]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/generated-bindings.md
  - ../subsystems/generator.md
---

# ADR-0005: Golden Files Committed to Git

## Status

Accepted — 2026-06-02 (binding decision); gate tooling landed throughout M1–M3.

## Context

The generator reads the X3D Unified Object Model (UOM) XML and emits a C++ binding layer — currently 343 `.hpp` + 340 `.cpp` files under `generated_cpp_bindings/`. Three forces made the handling of these files a deliberate decision rather than an implementation detail.

**Code review of generated code.** Generated output is not always obviously correct. Type mappings, accessor names, virtual inheritance, range-validation logic, and the reflection thunks are all in the emitted files. Having them in git means every generator change appears in a diff that can be reviewed, reverted, and bisected like any other code change. Without committed goldens, regressions in emitter logic are invisible until a downstream consumer breaks.

**The conformance claim rests on the generated layer.** The project's core value proposition is that node bindings are spec-correct by construction (from the ISO 19775-1 UOM). That claim is falsifiable only if the generated files are pinned and audited. A regenerate-on-demand workflow would leave the claim unverifiable in CI — any silent drift in the emitter or UOM parser would go undetected.

**Determinism is a gate, not an assumption.** Python dicts historically iterate in insertion order (CPython 3.7+), but the generator pulls node data from an XML file whose field sets are ordered by spec position, not by alphabetical or topological position. Early emitter code used `set` for include-list deduplication, which produces non-deterministic output across Python versions and runs. If the emitter is non-deterministic, byte-for-byte golden comparison fails on every CI run for a non-bug reason — so committing goldens forced the determinism fix.

**The C1 decl/def split (2026-06-16) expanded scope.** Before C1, only `*.hpp` headers were golden. After C1, each node also has a generated `*.cpp` holding the out-of-line reflection thunks and `fields()` FieldTable. The golden gate was extended to cover both extensions so that the 17× cold-build speedup (1296s → 76s on the C1 commit, ccache-off, -j4) did not introduce a category of unreviewed generated files.

## Decision

Generated C++ binding files (`generated_cpp_bindings/*.hpp` and `generated_cpp_bindings/*.cpp`) are committed to git as golden files. The codegen pipeline must be deterministic: no `set`-driven include ordering, all sorted iterators (`sorted(nodes)`, `sorted(enum_defs)`) wherever output order is derived from a collection. Drift is an intentional signal, not noise.

Two complementary gates enforce this:

1. **`scripts/check_golden.sh`** (runnable locally, invoked as `mise run golden`): regenerates the full tree into a temp directory and diffs every `*.hpp` + `*.cpp` against `generated_cpp_bindings/` in both directions (missing AND extra headers are failures). Exits non-zero on any drift and prints a human-readable report.

2. **`tests/test_golden_tree.py`** (pytest): the programmatic twin of the shell gate, run as part of `uv run pytest`. Asserts the regenerated tree is byte-for-byte identical to the committed golden, including both directions (golden minus produced = missing; produced minus golden = extra). Both gates require `clang-format` to be installed (the formatter is part of the determinism contract; the gate skips if absent rather than failing).

The update workflow is intentional and explicit: change a template or emitter, run `uv run x3d-cpp-gen --out generated_cpp_bindings`, review the diff, and commit both the generator change and the updated goldens together. The gate fails until that commit is made — preventing "generator changed but output not regenerated" from reaching CI.

## Consequences

**Positive:**

- Every generator change produces a reviewable diff in `generated_cpp_bindings/`. Regressions (wrong accessor names, missing fields, broken range checks, non-deterministic include order) are caught before merge, not after a downstream consumer breaks.
- The conformance claim ("spec-correct by construction") is continuously verifiable: the committed headers are the exact files the runtime compiles against, and `mise run golden` confirms they match the emitter at any point.
- Forced determinism improved emitter quality: the constraint eliminated `set`-driven include ordering (a latent non-determinism bug) and established `sorted()` as the canonical pattern throughout the emitter (`src/x3d_cpp_gen/emit/factory.py:58`, `src/x3d_cpp_gen/generator.py:213,229`).
- Extending the gate to `*.cpp` files (post-C1) at no extra process cost: the same diff mechanism covers both extensions, and the sha256 over the committed tree (`97ab4e2a4ffd6c3630a7a5121630eebee40ca0f2d8c0f37453584e56237d6727` at C1 landing, 2026-06-16) serves as a stable reference for the build-perf claim.
- Bisectability: `git log generated_cpp_bindings/` shows exactly when any node's API changed and what caused it, with the generator commit as the parent.

**Trade-offs / costs:**

- The committed tree is large (343 `.hpp` + 340 `.cpp`; 683 files total). Every generator change touches many of them simultaneously. PRs that modify the emitter will have large diffs that are visually noisy even when semantically uniform (e.g., adding a field to every node).
- `clang-format` must be present and must be the same version that produced the golden. A version mismatch causes formatting drift that triggers false gate failures. The gate skips (rather than hard-fails) when `clang-format` is absent, which means CI without the formatter installed does not catch formatting regressions — this is an accepted trade-off in favor of not blocking contributors who lack the formatter.
- Golden regeneration is manual (`uv run x3d-cpp-gen --out generated_cpp_bindings`). There is no auto-regenerate-and-commit hook in CI; that would blur the distinction between intentional and accidental drift, removing the review step. The `mise run golden` gate failing is the signal to regenerate — contributors must do so explicitly.

## Related

- [Architecture](../architecture.md)
- [Generated Bindings subsystem](../subsystems/generated-bindings.md) — the committed golden layer in detail; file layout and build integration
- [Generator subsystem](../subsystems/generator.md) — the emitter pipeline that must satisfy the determinism requirement
- `scripts/check_golden.sh` — the shell gate; the drift report format and exit codes
- `tests/test_golden_tree.py` — pytest twin of the shell gate
- `generated_cpp_bindings/` — the committed golden tree (343 `.hpp` + 340 `.cpp`)
- `src/x3d_cpp_gen/emit/factory.py` — uses `sorted()` for the factory registry include list; the canonical determinism pattern
- `docs/superpowers/specs/2026-06-16-c1-decl-def-split-design.md` — the C1 spec that extended golden coverage from `*.hpp` to `*.hpp`+`*.cpp`
- `docs/superpowers/plans/2026-06-16-c1-decl-def-split.md` — the C1 implementation plan
