# x3d-cpp SAI SDK

A modern C++20 Scene Access Interface for X3D. Applications program against one
contract, `provider::service` in `<x3d/sai/experimental/provider.hpp>`, and pick
a backend at construction:

- the **reference backend** in this directory (`x3d::sai_reference_provider`)
- the **native backend** in [`adapters/sai_provider`](../adapters/sai_provider/)
  (`x3d_cpp::sai_provider`), which drives a real x3d-cpp `RuntimeSession`

The namespace is `experimental`: the API can change between 0.x releases and
makes no binary ABI promise. Each backend advertises what it supports, so check
`service.supported()` rather than assuming the whole ISO SAI.

## Try it

From the x3d-cpp checkout:

```sh
mise run sai-hello
```

This builds and runs [`examples/provider_scene.cpp`](examples/provider_scene.cpp)
and prints `reference-kernel: translation = 5 0 0`.

## Use it

Requires C++20 and CMake 3.24 or newer.

```sh
cmake -S . -B build-sdk -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-sdk --parallel 3
cmake --install build-sdk --prefix /path/to/sdk
```

```cmake
find_package(x3d_sai CONFIG REQUIRED)
target_link_libraries(application PRIVATE x3d::sai_reference_provider)
```

## Documentation

The documentation lives in the x3d-cpp wiki under
[`docs/wiki/sai/`](../docs/wiki/sai/index.md) (`mise run docs` to browse it).

| If you want to | Start with |
|---|---|
| Learn the API | [Animate your first scene](../docs/wiki/sai/tutorials/first-scene.md) |
| Link the SDK into a project | [Consume the SDK from CMake](../docs/wiki/sai/how-to/consume-the-sdk.md) |
| Drive a scene from a host | [Run the host loop](../docs/wiki/sai/how-to/run-the-host-loop.md) |
| Look up exact behaviour | [Provider contract](../docs/wiki/sai/reference/provider-contract.md) |
| See targets and headers | [Packages, targets and headers](../docs/wiki/sai/reference/packages-and-targets.md) |
| Understand the design | [Architecture](../docs/wiki/sai/explanation/architecture.md) |
| Know what is not covered | [Scope and limits](../docs/wiki/sai/explanation/scope-and-limits.md) |

## Relationship to the ISO C++ binding

This SDK is another C++20 mapping of the abstract SAI services, shared to help
the X3D ecosystem; it does not replace ISO/IEC 19777-4. See
[Relationship to the ISO C++ binding](../docs/wiki/sai/explanation/iso-binding.md).

## Development

`sai/` is its own CMake project and uv package; the ordinary x3d-cpp runtime
build does not add it. From the x3d-cpp checkout:

| Task | Does |
|---|---|
| `mise run sai-ci` | UOM pin, generator determinism, registers, pytest, build, CTest (part of `mise run ci`) |
| `mise run sai-provider` | Native backend against this directory (part of `mise run ci-all`) |
| `mise run sai-baseline` | Regenerate [`docs/conformance/SAI-BASELINE.md`](docs/conformance/SAI-BASELINE.md) |

See [Verify a change](../docs/wiki/sai/how-to/verify-a-change.md).
