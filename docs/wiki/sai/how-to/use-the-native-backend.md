---
title: Build and link the native SAI backend
summary: Install the SAI SDK, the runtime and the native provider into one prefix and link x3d_cpp::sai_provider.
tags: [sai, how-to, native, cmake]
updated: 2026-11-14
related:
  - ../reference/native-provider.md
  - consume-the-sdk.md
---

# Build and link the native SAI backend

The native backend runs the portable contract over a real x3d-cpp
`RuntimeSession`. (For the reference backend see
[Consume the SDK from CMake](consume-the-sdk.md).) It is three packages: the SAI SDK, the runtime and the
adapter. Build all three from the **same x3d-cpp revision** with the same
compiler; the contract has no stable ABI.

## Install into one prefix

From the x3d-cpp root:

```sh
sai_source="$PWD/sai"
sai_prefix="$PWD/install-sai"

# 1. SAI SDK
cmake -S "$sai_source" -B build/sai-package -G Ninja \
  -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX="$sai_prefix"
cmake --build build/sai-package --parallel 2
cmake --install build/sai-package

# 2. Runtime
cmake -S . -B build/native-package -G Ninja \
  -DX3D_CPP_BUILD_TESTS=OFF -DX3D_CPP_ENABLE_DEV_TOOLING=OFF \
  -DCMAKE_INSTALL_PREFIX="$sai_prefix"
cmake --build build/native-package --parallel 2
cmake --install build/native-package

# 3. Adapter
cmake -S adapters/sai_provider -B build/sai-adapter -G Ninja \
  -DCMAKE_PREFIX_PATH="$sai_prefix" -DCMAKE_INSTALL_PREFIX="$sai_prefix"
cmake --build build/sai-adapter --parallel 2
cmake --install build/sai-adapter
```

## Link your host

```cmake
find_package(x3d_cpp_sai CONFIG REQUIRED)
target_link_libraries(my_host PRIVATE x3d_cpp::sai_provider)
```

Configure with `-DCMAKE_PREFIX_PATH="$sai_prefix"`. The target supplies its own
dependencies.

## Create the service

```cpp
#include <x3d/sai_provider.hpp>

namespace sai = x3d::sai::experimental;

auto service = sai::native::make_service();
if (!service) { /* inspect service.error() */ }
```

From here on, use `provider::service` exactly as you would on the reference
backend.

## Build from source instead

To skip installation, add the three source trees in this order:

```cmake
add_subdirectory(path/to/x3d-cpp/sai x3d-sai EXCLUDE_FROM_ALL)
add_subdirectory(path/to/x3d-cpp x3d-cpp EXCLUDE_FROM_ALL)
add_subdirectory(path/to/x3d-cpp/adapters/sai_provider adapter EXCLUDE_FROM_ALL)
target_link_libraries(my_host PRIVATE x3d_cpp::sai_provider x3d_cpp::x3d_cpp)
```

`examples/sai_host/CMakeLists.txt` does this when you pass
`X3D_CPP_SOURCE_DIR` and `X3D_SAI_SOURCE_DIR`.

## Check what you can do

Query `service.supported()` and each field's `supported` flag before issuing
operations. The constructible slice and its limits are in
[Native provider](../reference/native-provider.md).

## Next steps

- [Run the host loop](run-the-host-loop.md)
- [Adopt a loaded session](adopt-a-session.md)
- [Present frames from the native feed](present-frames.md)
