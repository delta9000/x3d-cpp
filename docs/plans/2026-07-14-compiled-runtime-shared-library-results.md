# Compiled Runtime Shared Library Results

**Date:** 2026-07-14

**Branch:** `perf/runtime-shared-library`

**Baseline:** fetched `origin/main` at `4887c72`

**Compiler:** GCC 16.1.1, Ninja `-j16`, `CCACHE_DISABLE=1`

## Outcome

The branch replaces repeated header implementation work with three acyclic
implementation layers:

```text
x3d_cpp_headers
  <- x3d_cpp_nodes
  <- x3d_cpp_authoring_runtime
  <- x3d_cpp_runtime
```

All three are shared by default. `X3D_CPP_SHARED_NODES=OFF` switches the entire
implementation stack to static libraries for compatibility. Existing consumer
facades remain the entry points: authoring consumers link `x3d_cpp::authoring`,
full consumers link `x3d_cpp::x3d_cpp`, and SDK consumers link `x3d_cpp::sdk`.

The authoring runtime now owns field-value formatting and the four writers. The
full runtime owns the parse front door, concrete readers, node-building helpers,
and `MeshBuilder`. Templates and genuinely small inline helpers remain in public
headers.

## Clean behavior build

Both measurements used a fresh CI-preset build directory, the same compiler and
job count, the scoped `x3d_behavior_tests` target, and disabled ccache.

| Metric | `origin/main` | This branch | Change |
|---|---:|---:|---:|
| Wall time | 73.26 s | 52.33 s | -28.6% |
| User CPU | 982.81 s | 715.64 s | -27.2% |
| System CPU | 23.13 s | 18.71 s | -19.1% |
| Peak RSS | 1,114,596 KiB | 854,980 KiB | -23.3% |
| Build tree | 734 MiB | 513 MiB | -30.1% |

The acceptance threshold was a 10% affected-group compiler-time reduction.
Repeated codec, parser, and extraction test compiles fell from 371.130 seconds
to 191.182 seconds. Including the new authoring/full runtime implementation
objects once (25.424 seconds) gives 216.606 seconds, a net 41.6% reduction.

## Compiler work by group

Values are cumulative command durations for object entries in the fresh Ninja
log. The new implementation rows are the cost paid once instead of in each
consumer translation unit.

| Group | `origin/main` | This branch | Change |
|---|---:|---:|---:|
| Codec tests | 145.557 s | 59.580 s | -59.1% |
| Parser tests | 65.491 s | 21.170 s | -67.7% |
| Extraction tests | 160.082 s | 110.432 s | -31.0% |
| Event tests | 166.051 s | 138.410 s | -16.6% |
| Geometry/scene tests | 76.970 s | 62.181 s | -19.2% |
| Generated nodes | 197.193 s | 192.350 s | -2.5% |
| Other consumers | 323.554 s | 203.500 s | -37.1% |
| Authoring runtime (new) | — | 10.300 s | paid once |
| Full runtime (new) | — | 15.124 s | paid once |
| **All object compiles** | **1,134.898 s** | **813.047 s** | **-28.4%** |

## Public SDK probe

The direct syntax-only probe uses C++20 and the public `x3d/sdk.hpp` facade.

| Metric | `origin/main` | This branch | Change |
|---|---:|---:|---:|
| Preprocessed lines | 150,061 | 129,721 | -13.6% |
| Compiler user CPU | 2.97 s | 2.14 s | -27.9% |
| Peak RSS | 571 MiB | 517 MiB | -9.5% |

The remaining SDK weight is primarily header-only event, scene, and extraction
behavior. It is a follow-up opportunity, not required to justify this boundary.

## Artifact and disk size

Normal shared artifacts are small relative to the generated node library:

| Artifact | Size |
|---|---:|
| `libx3d_cpp_nodes.so.0.1.0` | 68.7 MiB |
| `libx3d_cpp_authoring_runtime.so.0.1.0` | 1.6 MiB |
| `libx3d_cpp_runtime.so.0.1.0` | 3.6 MiB |
| Complete normal behavior tree | 513 MiB |
| Complete ASan/UBSan behavior tree | 3.0 GiB |

The sanitizer tree was built in `/tmp`, used `-g1`, and contained only the
behavior aggregate. This keeps it far below the historical approximately
25 GiB failure mode. The all-static compatibility build occupied 3.3 GiB
because every test executable absorbs the static implementation archives; it
is deliberately not the normal or sanitizer CI configuration.

## Verification

The final branch was exercised through:

- the complete Python suite;
- normal shared behavior and compile-contract aggregates;
- all public headers compiled independently;
- a clean ASan/UBSan build with all 46 sanitizer behavior tests;
- an all-static build with all 47 behavior/install tests;
- installed authoring and SDK consumers;
- the Clang libFuzzer target with coverage and ASan/UBSan applied to the newly
  compiled implementation layers;
- CPU raster headless execution;
- the OpenGL renderer headless and real-GL screenshot paths;
- cgltf and Assimp asset-import paths plus their backend swap test;
- the authoring footprint gate (`.text=42,160`, baseline `18,418,227`).

No serialization, parsing, mesh-generation, or consumer behavior was changed.
The work changes compilation and linkage ownership only.
