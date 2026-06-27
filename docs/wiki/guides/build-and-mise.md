---
title: Build and mise tasks
summary: The dev CMake preset, mise run build, the compile-job pool, ccache, and every mise task explained.
tags: [guide, build, mise, cmake, ninja, ccache]
updated: 2026-06-20
related:
  - gate-system.md
---

# Build and mise tasks

This guide covers the day-to-day C++ build workflow: the `dev` preset, `mise run build`,
the compile-job pool (and its OOM history), ccache, fast-linker auto-selection, and every
`mise run` task you will encounter.

---

## Prerequisites

- `mise` installed — it manages `uv` (Python toolchain) and all task definitions
- `cmake` 3.20+, `ninja`, `ccache` (optional but strongly recommended)
- `mold` or `lld` optional — auto-detected at configure time for faster linking

Run `mise install` once after checkout; it bootstraps `uv` per `mise.toml`'s `[tools]` section.
All `mise run` tasks shell out to `uv run` where needed, so no manual `venv` activation is
required.

---

## The `dev` preset

All C++ work uses the `dev` configure/build/test preset defined in `CMakePresets.json`:

| Property | Value |
|---|---|
| Generator | Ninja |
| Build directory | `build/` (relative to source root) |
| `CMAKE_EXPORT_COMPILE_COMMANDS` | `ON` (clangd, code-rag ingest) |
| `X3D_CPP_PER_HEADER_CHECKS` | `OFF` (see below) |
| Build type | None (unoptimized; the suite tests correctness + behavior, not performance) |

The preset is intentionally unoptimized — asserts are not disabled, NDEBUG is not set, and
the test suite does not depend on any optimization flag. That is correct for the project's
correctness-first posture.

---

## `mise run build` — the standard local workflow

```bash
mise run build
```

This single command runs three steps sequentially (from `mise.toml`):

```
cmake --preset dev
cmake --build --preset dev
ctest --preset dev -j "$(nproc)"
```

1. **Configure** — runs CMake with the `dev` preset (Ninja, `build/` dir, compile-commands on,
   per-header checks off). Idempotent; re-runs only detect cache changes.
2. **Build** — compiles everything the preset knows about. On a warm ccache this is seconds.
3. **Test** — runs all ctests in parallel across all logical cores. With `X3D_CPP_PER_HEADER_CHECKS`
   off, the dev preset runs the behavior/integration suite only (not the ~800 per-header
   isolation tests).

---

## The compile-job pool

`CMakeLists.txt` lines 53–58 set a Ninja job-pool named `x3d_compile` that caps concurrent
**compile** jobs (links and light TUs are uncapped):

```cmake
cmake_host_system_information(RESULT _x3d_logical_cores QUERY NUMBER_OF_LOGICAL_CORES)
set(X3D_CPP_COMPILE_JOBS "${_x3d_logical_cores}" CACHE STRING "Max concurrent compile jobs")
if(CMAKE_GENERATOR MATCHES "Ninja")
    set_property(GLOBAL PROPERTY JOB_POOLS x3d_compile=${X3D_CPP_COMPILE_JOBS})
    set(CMAKE_JOB_POOL_COMPILE x3d_compile)
endif()
```

**Default: all logical cores.** Post-C1 (the decl/def split — see below), each compile's
peak RSS is ~0.86 GB. On the 16-core/60 GB development machine the full parallel load is
roughly 4.9 GB of total compiler RSS — comfortable. You only need to lower the cap on a
low-RAM host.

**Historical note.** Before the C1 split (before commit `843a5bd`), the generated headers were
pure header-only and each TU instantiated the reflection `std::function` thunks inline. That
pushed per-compile peak RSS high enough to OOM-kill `cc1plus` above `-j4` on the same 60 GB
box. The cap was therefore hard-coded at 4. After C1 moved the heavy thunk instantiation into
the `x3d_cpp_nodes` static lib, the memory pressure dropped and the cap became the core count.

**Override:**

```bash
cmake --preset dev -DX3D_CPP_COMPILE_JOBS=4   # throttle on a low-RAM host
```

The cache var is sticky — an existing `build/` keeps its cached value. Reset it explicitly:

```bash
cmake --preset dev -U X3D_CPP_COMPILE_JOBS
```

**Do not retry PCH.** Before C1, a shared PCH was measured at ~0% improvement (1296 s → 1260 s)
because `clang -ftime-trace` showed ~70% of per-TU cost was template instantiation + codegen, not
parsing. PCH avoids re-parsing, not instantiation. The correct fix was C1 (done; do not revisit).

---

## ccache — warm vs. cold builds

`CMakeLists.txt` lines 17–22 auto-detect and wire ccache:

```cmake
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM AND NOT CMAKE_CXX_COMPILER_LAUNCHER)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "ccache launcher")
    set(CMAKE_C_COMPILER_LAUNCHER   "${CCACHE_PROGRAM}" CACHE STRING "ccache launcher")
endif()
```

It is a no-op when ccache is absent (CI runners without it still build cleanly).

**Cold vs. warm numbers (16-core, post-C1):**

| Build state | Approximate time |
|---|---|
| Cold, no ccache (pre-C1 baseline) | ~1296 s |
| Cold, no ccache (post-C1) | ~39 s |
| Warm ccache (any edit to a non-widely-included header) | seconds |

The C1 "decl/def split" is the most important build-time change in the project's history.
It moved the heavy `fields()` reflection tables and per-field `std::function` get/set thunks
out of every `<Node>.hpp` into separate `<Node>.cpp` files, compiled once into the CMake
STATIC lib `x3d_cpp_nodes` (`CMakeLists.txt` line 96). Consumers link against the lib rather
than re-instantiating the thunks per TU. The design rationale and measurements live in
`docs/superpowers/specs/2026-06-16-c1-decl-def-split-design.md`.

---

## Fast-linker auto-selection

`CMakeLists.txt` lines 30–38 prefer `mold` then `lld` over the system linker:

```cmake
find_program(X3D_MOLD mold)
find_program(X3D_LLD ld.lld)
if(X3D_MOLD)
    add_link_options("-fuse-ld=mold")
elseif(X3D_LLD)
    add_link_options("-fuse-ld=lld")
endif()
```

With ~70 test executables plus the all-headers aggregate TU, link time is non-trivial on the
default BFD/gold linker. `mold` in particular makes sequential links nearly instant. The
fallback to the system default is graceful — the build never breaks on a fresh checkout.

---

## Per-header isolation tests

`CMakeLists.txt` declares the `X3D_CPP_PER_HEADER_CHECKS` option:

```cmake
option(X3D_CPP_PER_HEADER_CHECKS "Compile each header in isolation as a ctest" ON)
```

These are ~800 ctests (one per generated header) that pin a break to a specific file. They
are the slow part of a cold ctest run. The `dev` preset overrides this to `OFF`
(`CMakePresets.json`), so `mise run build` runs the behavior/integration suite only.

**CI must enable them.** Any CI configuration that does not use the `dev` preset (or explicitly
sets `-DX3D_CPP_PER_HEADER_CHECKS=ON`) retains full per-header break-pinning coverage.

To run them locally:

```bash
cmake --preset dev -DX3D_CPP_PER_HEADER_CHECKS=ON
cmake --build --preset dev
ctest --preset dev -j "$(nproc)"
```

---

## All mise tasks

### Core iteration tasks

| Task | What it does |
|---|---|
| `mise run build` | Configure + build + ctest (behavior suite). The standard local workflow. |
| `mise run gen` | Re-run the Python generator: `uv run x3d-cpp-gen --out generated_cpp_bindings`. Use this after changing a template or emitter. |
| `mise run golden` | Golden-drift gate: regenerates to a temp dir and diffs `*.hpp`/`*.cpp` against the committed tree. Fails on any drift. |
| `mise run test` | Run the Python test suite (`uv run pytest`). Depends on `sync`. |
| `mise run sync` | `uv sync` — installs/refreshes the project env including the dev group. |

### CI and gate tasks

| Task | What it does |
|---|---|
| `mise run ci` | Full pipeline: `test` + `golden` + `conformance-gate` + `build` + `cli-gate-regression`. Run before pushing. |
| `mise run conformance` | Regenerate the conformance view (`docs/conformance/`) from `docs/conformance/findings.yaml` in place. |
| `mise run conformance-gate` | Conformance gate: validate `findings.yaml` schema + check that the generated view matches the committed files. |
| `mise run cli-gate-regression` | CLI regression gate: runs `x3d_cli_gate` + `x3d_canon_gate` in `--gate` mode. Depends on `build`. |
| `mise run cli-gate` | Informative (non-failing) differential gate: produces `tools/x3d-cli/goldens/divergence-report.md`. |
| `mise run canon-gate` | Informative tiered canonicalize gate (T1 idempotence + T2 tolerant-diff + T3 byte-exact rate). |
| `mise run cli-gate-baseline` | Refresh the committed baseline TSVs after accepting new intentional divergences. |

### Corpus tasks

| Task | What it does |
|---|---|
| `mise run corpus-fetch` | Fetch the X3D test corpus the differential gates need — the curated subset (`tools/x3d-cli/goldens/subset.txt`) **plus its transitive `Inline`/`EXTERNPROTO` dependencies** — from `www.web3d.org` into `.x3d-corpus/` (gitignored). ~9 MB, seconds. The gate/corpus tasks default to this location. |
| `mise run corpus [/path]` | Corpus smoke: recursively sweep a corpus dir (parse → extract → tick), print categorized stats. Builds `x3d_corpus_sweep` first. Sweeps *whatever is in the dir*: defaults to `$X3D_CORPUS_DIR`, else `.x3d-corpus/` (the ~300-file `corpus-fetch` subset — a smoke, not full coverage). Point it at a full archive checkout for exhaustive coverage. |

### Documentation tasks

| Task | What it does |
|---|---|
| `mise run docs` | Serve the wiki locally with live-reload at `http://127.0.0.1:8000`. |
| `mise run docs-build` | Build the wiki with `--strict` (broken links, nav orphans fail the build). The docs gate. |

### RAG / ingestion tasks

| Task | What it does |
|---|---|
| `mise run code-rag <args>` | Semantic search over the C++ runtime. E.g. `mise run code-rag query "how are bounds computed"`. |
| `mise run code-ingest` | (Re)build the `x3d-cpp-code` Qdrant collection from the runtime source. |

### Fixture generation (one-time / after corpus expansion)

| Task | What it does |
|---|---|
| `mise run cli-golden-gen` | Run X3DJSAIL `-validate` over the curated corpus subset to generate `tools/x3d-cli/goldens/`. Requires JDK + corpus. |
| `mise run canon-golden-gen` | Generate X3DJSAIL `-canonical` reference fixtures for the tiered canon gate. Requires JDK 25 + corpus. |

### PoC renderer (optional, off by default)

| Task | What it does |
|---|---|
| `mise run poc` | Configure + build the out-of-SDK OpenGL PoC renderer into `build-poc/`. Separated from `build/` to avoid perturbing the dev preset's ccache/ctest. |

The PoC renderer requires Wayland prereqs; see `examples/poc_renderer/README.md`. Run the binary
manually (`./build-poc/examples/poc_renderer/x3d_poc_renderer`); it needs a display and is not
a ctest.

---

## Common gotchas

**Stale compile-job cap.** If you override `X3D_CPP_COMPILE_JOBS` and then re-run
`cmake --preset dev` without `-U X3D_CPP_COMPILE_JOBS`, the old cached value persists. Always
reset explicitly when you want the default to re-apply.

**`build/` cache after a preset change.** CMake cache vars are sticky. When switching
between configurations (e.g. toggling `X3D_CPP_PER_HEADER_CHECKS`), prefer:

```bash
rm -rf build/ && cmake --preset dev [-Dvars...]
```

**Missing corpus.** `mise run cli-gate-regression` and `mise run corpus` default to
`$X3D_CORPUS_DIR`. Pass an explicit path as the first
argument when the corpus lives elsewhere:

```bash
mise run cli-gate-regression /path/to/archive
mise run corpus /path/to/archive
```

The gate harnesses exit 1 immediately on a missing directory rather than reporting a false-green
(zero checks = vacuous pass is explicitly rejected).

**PoC and the dev build cache.** `mise run poc` builds into `build-poc/`, not `build/`. This
is intentional: the PoC pulls in GLFW and glad, which would pollute the compile-command export
and the ccache hit rate for the core build if they shared a directory.

---

## See also

- [Gate System](gate-system.md) — the four CI gates (golden / conformance / CLI regression / docs) in detail
