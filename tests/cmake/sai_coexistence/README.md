# SAI SDK / runtime coexistence check

Builds the runtime and the SAI SDK (`sai/`) as two separate packages and links
them into one process, in every header and link order, from source and from a
relocated installed prefix.

Requirements: CMake 3.24+, a C++20 compiler, and Ninja (or set
`CMAKE_GENERATOR`). From the x3d-cpp root:

```bash
bash scripts/verify_sai_coexistence.sh            # SAI source defaults to sai/
X3D_CPP_SHARED_NODES=OFF bash scripts/verify_sai_coexistence.sh   # static runtime symbols
```

Or `mise run sai-coexistence`. Run it when you change generated header
ownership, exported include paths or the SAI/runtime boundary.

To configure the source consumer directly:

```bash
cmake -S tests/cmake/sai_coexistence -B build/coexistence -G Ninja \
  -DX3D_SAI_SOURCE_DIR="$PWD/sai"
cmake --build build/coexistence --target sai_coexistence_all --parallel 2
ctest --test-dir build/coexistence --output-on-failure -R '^sai_coexistence_'
```

What is checked, and the script's options, are in
[Test suites and gates](../../../docs/wiki/sai/reference/test-suites.md#coexistence-check).
