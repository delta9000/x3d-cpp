---
title: Consume the SAI SDK from CMake
summary: Install or embed the SAI SDK and link the reference backend into an application.
tags: [sai, how-to, cmake]
updated: 2026-11-14
related:
  - ../reference/packages-and-targets.md
  - use-the-native-backend.md
---

# Consume the SAI SDK from CMake

This guide links the **reference** backend. For the native runtime backend see
[Build and link the native backend](use-the-native-backend.md).

You need a C++20 compiler and CMake 3.24 or newer.

## Option A: install, then `find_package`

1. Build and install from `sai/`:

    ```sh
    cd sai
    cmake -S . -B build-sdk -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release
    cmake --build build-sdk --parallel 3
    cmake --install build-sdk --prefix /path/to/sdk
    ```

2. Link it in your project:

    ```cmake
    find_package(x3d_sai CONFIG REQUIRED)
    add_executable(application main.cpp)
    target_link_libraries(application PRIVATE x3d::sai_reference_provider)
    ```

3. Configure your project with `-DCMAKE_PREFIX_PATH=/path/to/sdk`.

The installed prefix can be moved afterwards.

## Option B: embed the checkout

```cmake
add_subdirectory(path/to/x3d-cpp/sai sai EXCLUDE_FROM_ALL)
add_executable(application main.cpp)
target_link_libraries(application PRIVATE x3d::sai_reference_provider)
```

CMake then builds only the SDK targets your application selects. Tests and
examples are not added to an embedded build.

## Create the service

```cpp
#include <x3d/sai/experimental/reference_provider.hpp>

namespace sai = x3d::sai::experimental;

auto service = sai::make_reference_provider();
if (!service) { /* inspect service.error() */ }
```

`make_reference_provider` takes optional
[resource limits](../reference/resource-limits.md) and returns an owning
`provider::service`.

## Pick a different target

| You need | Link |
|---|---|
| The portable contract only (for example to write a backend) | `x3d::sai` |
| Snapshots, atomic editors and kernel sessions | `x3d::sai_reference_kernel` |
| The above plus the generated catalog and typed bindings | `x3d::sai_reference_metadata` |

The full list is in
[Packages, targets and headers](../reference/packages-and-targets.md).

## Keep application code backend-blind

Put your scene logic in functions that take `provider::service &` and choose
the backend only where you construct the service. Then the same code runs on
either backend. `sai/examples/provider_scene.cpp` is laid out this way.
