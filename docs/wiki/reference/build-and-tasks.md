---
title: Build and mise tasks
summary: Build configuration, compiled libraries, and developer tooling.
tags: [guide, build]
updated: 2026-11-05
related: []
---

# Build and mise tasks

Run `mise tasks` for the task catalog maintained in `mise.toml`.
Use `mise run build` for local C++ iteration and `mise run build-ci` for the
behavior and compile-contract suites. See [Gate system](gates.md) for
verification boundaries.

## Prerequisites

The C++ build needs CMake 3.21 or later, Ninja, and a C++20 compiler.
`mise install` installs the tools pinned in `mise.toml`. Tasks use `uv run`
for Python commands, so you don't need to activate a virtual environment.

## Build presets

`CMakePresets.json` defines separate build directories:

| Preset | Directory | Configuration |
|---|---|---|
| `dev` | `build/` | Unoptimized; per-header isolation disabled |
| `ci` | `build-ci/` | Warnings as errors; per-header isolation enabled by default |
| `san` | `build-san/` | Debug; AddressSanitizer and UndefinedBehaviorSanitizer; behavior suite |
| `fuzz` | `build-fuzz/` | Clang; libFuzzer, AddressSanitizer, and UndefinedBehaviorSanitizer |

For a focused run after configuring and building:

```sh
ctest --preset dev -R x3d_extract_tests --output-on-failure
```

CTest selects executable-level tests. To select a doctest case, run its grouped
binary with `--test-case`. Case names are not separate CTest registrations.

## Compiled libraries

`cmake/x3d/libraries.cmake` defines the implementation layers:

| Target | Contents |
|---|---|
| `x3d_cpp::nodes` | Generated node definitions and reflection |
| `x3d_cpp::authoring_runtime` | Field formatting and codec writers |
| `x3d_cpp::runtime` | Parsers and mesh construction, linked to the authoring layer |
| `x3d_cpp::authoring` | Interface target for authoring consumers |
| `x3d_cpp::sdk` | Interface target for the full SDK |

The implementation libraries are shared by default. Configure with
`-DX3D_CPP_SHARED_NODES=OFF` to build all three as static libraries.
The runtime combines compiled sources and headers; it is not header-only.
Optional backend libraries are configured in `cmake/x3d/backends.cmake` and
`script-backend.cmake`. See the [seam matrix](seam-matrix.md).

## Developer tooling

`cmake/x3d/dev-tooling.cmake` enables ccache when found and prefers mold, then
lld, when available. It applies this tooling by default only in a top-level
build. An `add_subdirectory` consumer can opt in with
`X3D_CPP_ENABLE_DEV_TOOLING`.

The Ninja compile pool defaults to the logical core count. Lower the cached
limit on memory-constrained hosts:

```sh
cmake --preset dev -DX3D_CPP_COMPILE_JOBS=3
cmake --build --preset dev -j3
```

Reset the cached limit with `cmake --preset dev -U X3D_CPP_COMPILE_JOBS`.
Use separate build directories for different backend or instrumentation choices.

## Header checks and warnings

`cmake/x3d/tests.cmake` builds one isolation translation unit per public header
when `X3D_CPP_PER_HEADER_CHECKS` is enabled. These are compile contracts, not
hundreds of individual CTest executions. The `build-ci` task builds
`x3d_behavior_tests` and `x3d_compile_contracts` separately, then runs their
corresponding CTest labels. `cmake/x3d/test-suites.cmake` owns these lists.

`cmake/x3d/compiler-flags.cmake` defines the warning and instrumentation flags.
The `ci` preset enables `X3D_CPP_WERROR`; the `dev` preset leaves it disabled.

<a id="sanitizer-and-fuzz-gates-bld-2"></a>
## Sanitizer and fuzz gates

Use `mise run build-san` for the instrumented behavior suite. The `san` preset
uses Debug. `mise run build-fuzz` builds the parser harness; run it directly:

```sh
./build-fuzz/x3d_parse_fuzz -max_total_time=60
./build-fuzz/x3d_parse_fuzz corpus/
```

The compiled node and runtime libraries receive `fuzzer-no-link` coverage and
ASan/UBSan instrumentation. The harness supplies the libFuzzer driver. Instrumenting
only the harness would leave compiled parser code without coverage feedback.
A bounded run is a smoke check, not exhaustive input coverage.

## Optional dependency manifest

`vcpkg.json` supplies the optional curl port. You can also use system libcurl.
To configure through vcpkg:

```sh
cmake -S . -B build-curl -G Ninja -DX3D_CPP_BUILD_CURL=ON \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
```
