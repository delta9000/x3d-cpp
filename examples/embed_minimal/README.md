# Minimal x3d-cpp Embedder

This is a deliberately small downstream-style project. It assumes `x3d-cpp` was
installed somewhere on `CMAKE_PREFIX_PATH`; it does not depend on the source tree.

```sh
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/path/to/x3d-cpp/install
cmake --build build
./build/x3d_embed_minimal
```

The example links only `x3d_cpp::sdk` and includes only `x3d/sdk.hpp`.
