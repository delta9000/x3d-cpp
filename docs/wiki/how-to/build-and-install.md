---
title: Build and install the SDK
summary: Build x3d-cpp from a checkout, install it to a prefix and link it from another CMake project.
tags: [how-to, build, cmake, embedding]
updated: 2026-11-15
related:
  - ../reference/build-and-tasks.md
  - ../reference/api-stability.md
---

# Build and install the SDK

You need a C++20 compiler, CMake 3.21 or later, and Ninja.

## Build and install

From the checkout:

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DX3D_CPP_BUILD_TESTS=OFF
cmake --build build --parallel 2
cmake --install build --prefix "$PWD/install"
```

- Use `RelWithDebInfo` when you need debug information.
- The node and runtime libraries are shared by default. Add
  `-DX3D_CPP_SHARED_NODES=OFF` for static libraries.
- The build also produces the `x3d` command at `build/x3d`.

## Link it from your project

In your application's `CMakeLists.txt`, after defining the `my_app` target:

```cmake
find_package(x3d_cpp CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE x3d_cpp::sdk)
```

Configure your project with the install prefix on `CMAKE_PREFIX_PATH`:

```sh
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/path/to/x3d-cpp/install
cmake --build build
```

In your C++ source, include the SDK header:

```cpp
#include "x3d/sdk.hpp"
namespace sdk = x3d::sdk;
```

## Choose a target

| You are writing | Link | Include |
|---|---|---|
| A host that loads, runs and extracts scenes | `x3d_cpp::sdk` | `x3d/sdk.hpp` |
| A tool that only builds and writes scenes | `x3d_cpp::authoring` | `x3d/authoring.hpp` |
| A host that also needs the opt-in extensions | `x3d_cpp::sdk` and `x3d_cpp::ext` | `x3d/ext.hpp` (build with `-DX3D_CPP_BUILD_EXT=ON`) |

Target compatibility is described in [API stability](../reference/api-stability.md).

## Turn on optional backends

Choose backends with the `X3D_CPP_BUILD_*` options in
`cmake/x3d/backends.cmake`. Most are off by default; the bundled Duktape script
backend is enabled by default in `cmake/x3d/script-backend.cmake`. Some backends
fetch dependencies at configure time. Use a separate build directory for each
backend combination.

See [Supply resources through the seams](supply-resources.md) for wiring
backends into a host, and [Build and mise tasks](../reference/build-and-tasks.md)
for presets, compiled libraries and tooling switches.

## Check the install

`examples/embed_minimal/` is a complete downstream project that uses only the
installed package. Run these commands from the SDK checkout after installing:

```sh
cmake -S examples/embed_minimal -B build-embed -G Ninja \
  -DCMAKE_PREFIX_PATH="$PWD/install"
cmake --build build-embed
cd build-embed && ./x3d_embed_authoring && ./x3d_embed_minimal
```

`scripts/verify_install_embed.sh` runs the same chain against an installed and
a relocated prefix.

## Next steps

- [Load, run and extract your first scene](../tutorials/first-scene.md)
- [Integrate a native host](integrate-a-native-host.md)
