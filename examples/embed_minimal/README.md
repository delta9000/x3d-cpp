# Minimal x3d-cpp embedder

A small downstream project. It needs x3d-cpp installed on `CMAKE_PREFIX_PATH`
and does not depend on the source tree.

```sh
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/path/to/x3d-cpp/install
cmake --build build
./build/x3d_embed_authoring          # writes hello.x3d
./build/x3d_embed_minimal            # parses it back and extracts the mesh
```

What each program shows is in
[docs/wiki/reference/examples/index.md](../../docs/wiki/reference/examples/index.md).
The rules `native_host.cpp` checks are in
[docs/wiki/reference/native-host-contract.md](../../docs/wiki/reference/native-host-contract.md).
