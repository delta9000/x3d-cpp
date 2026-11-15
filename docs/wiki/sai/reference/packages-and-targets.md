---
title: SAI packages, targets and headers
summary: Source layout, CMake packages and targets, public headers and build requirements of the SAI SDK and native provider.
tags: [sai, reference, cmake]
updated: 2026-11-14
related:
  - ../how-to/consume-the-sdk.md
---

# SAI packages, targets and headers

To put these to use see
[Consume the SDK from CMake](../how-to/consume-the-sdk.md).

## Requirements

- C++20 compiler
- CMake 3.24 or newer
- No Python and no generator at application build time

## Source layout

| Path | Role |
|---|---|
| `sai/include/x3d/sai/experimental/` | Public headers |
| `sai/src/` | Frontend (`provider.cpp`), reference backend, kernel, session and browser services |
| `sai/src/x3d_sai_gen/` | SAI emitters (uv package `x3d-sai-gen`) |
| `sai/generated_cpp_bindings/public/x3d/sai/` | Generated field keys and typed bindings (installed) |
| `sai/generated_cpp_bindings/private/x3d_sai/detail/` | Generated catalog implementation (not installed, not a consumer include root) |
| `sai/examples/` | `provider_scene.cpp` and four reference-kernel examples |
| `sai/tests/` | doctest suites registered as CTests, generator pytest |
| `sai/docs/conformance/` | Service and invariant registers and the generated baseline |
| `adapters/sai_provider/` | Native backend package |
| `examples/sai_host/` | Headless native host example |
| `tests/cmake/sai_provider/`, `tests/cmake/sai_coexistence/` | Integration harnesses |

## CMake packages

| Package | Provides | Built from |
|---|---|---|
| `x3d_sai` | The `x3d::` targets below | `sai/` |
| `x3d_cpp_sai` | `x3d_cpp::sai_provider` | `adapters/sai_provider/` |

Installed packages are relocatable; their CMake files contain no checkout
paths. An explicit package version request must match exactly.

## Targets

| Target | Role | Portable? |
|---|---|---|
| `x3d::sai` | Compiled provider-neutral frontend and generated field keys | Yes |
| `x3d::sai_provider` | Alias of `x3d::sai`, not another interface | Yes |
| `x3d::sai_reference_provider` | Reference backend behind the frontend | Yes |
| `x3d_cpp::sai_provider` | Native backend over a `RuntimeSession` | Yes |
| `x3d::sai_provider_fixture` | Shared behavioural test oracle for backends | Test only |
| `x3d::sai_reference_kernel` | Concrete reference kernel and `kernel.hpp` | Reference only |
| `x3d::sai_reference_metadata` | Kernel plus the generated UOM catalog, `metadata.hpp` and typed bindings | Reference only |

All targets propagate the C++20 requirement, include directories and platform
thread support. The SDK archives use position-independent code, so shared
libraries and renderer plugins can link them. The installed SDK includes the
pinned `tl::expected` header and its license.

## Public headers

Under `<x3d/sai/experimental/...>`:

| Header | Contents |
|---|---|
| `provider.hpp` | The portable contract: `provider::service`, handles, receipts, capabilities, `provider::backend` |
| `reference_provider.hpp` | `make_reference_provider(resource_limits = {})` |
| `types.hpp` | Value vocabulary, `result<T>`, `sai_error`, `error_code` |
| `schema.hpp` | `field_key` and the generated key and node descriptors |
| `user_data.hpp` | `user_data_value` |
| `value_constraints.hpp` | Shared value validation, including `valid_rotation_axis` |
| `matrix.hpp` | Header-only checked matrix algebra |
| `image.hpp` | Checked SFImage helpers: `image_from_pixels`, `image_pixels`, `with_sub_image` |
| `bindings/<Node>.hpp` | Generated typed keys per node, for example `bindings/Transform.hpp` |
| `kernel.hpp` | Reference kernel: contexts, snapshots, edits, events |
| `session.hpp` | Reference `external_session` |
| `browser_services.hpp` | Reference browser services |
| `metadata.hpp` | Generated semantic catalog bridge |
| `testing/provider_fixture.hpp` | Shared backend oracle |

Native backend headers:

| Header | Contents |
|---|---|
| `<x3d/sai_provider.hpp>` | `native::make_service`, `native::adopt_session`, `native::backend` |
| `<x3d/sai_presentation.hpp>` | `native::make_presented_scene`, `render_feed`, `changes_between` |

## Factories

| Factory | Returns |
|---|---|
| `x3d::sai::experimental::make_reference_provider(resource_limits = {})` | Owning `provider::service` on the reference backend |
| `x3d::sai::experimental::native::make_service(resource_limits = {})` | Owning `provider::service` on the native backend |
| `native::make_presented_scene(resource_limits = {})` | Service, one setup scene and its render feed |
| `native::adopt_session(std::move(session), resource_limits = {})` | Service, scene, activation and checked host access |

## Build options and environment

| Setting | Effect |
|---|---|
| `BUILD_TESTING=ON` (standalone `sai/` build) | Adds tests and example executables |
| `X3D_SAI_BUILD_DIR` | CMake/CTest directory for `sai-build` and `sai-ci`; default `build` (that is, `sai/build`) |
| `X3D_CPP_SHARED_NODES=ON|OFF` | Shared or static native node packages in the provider harnesses |
| `X3D_CPP_SOURCE_DIR`, `X3D_SAI_SOURCE_DIR` | Source-mode build of `examples/sai_host` |
| `CMAKE_BUILD_PARALLEL_LEVEL` | Compiler concurrency for `sai-hello` and the harness scripts |

`sai-build` uses three parallel jobs so the test and generated-catalog
translation units do not consume unbounded memory.

## Generator

`x3d-sai-gen` adds the SAI emitters on top of `x3d-cpp-gen` (a path
dependency) and reads the UOM from `src/x3d_cpp_gen/data/`, checked against the
SHA-256 pin in `sai/src/x3d_sai_gen/data/UOM-4.0.sha256`. Catalog provenance carries the X3D
specification version, a deterministic SHA-256 fingerprint of the resolved
semantic model, and the generator version.

| Task | Does |
|---|---|
| `mise run sai-gen` | Regenerate `sai/generated_cpp_bindings` |
| `mise run sai-gen-determinism` | Generate into a temporary directory and compare contents and file inventory; never rewrites the tree. `TMPDIR` selects the scratch location |
| `mise run sai-uom-pin` | Fail if the vendored UOM drifted from the recorded SHA-256 |
