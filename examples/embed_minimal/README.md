# Minimal x3d-cpp Embedder

This is a deliberately small downstream-style project. It assumes `x3d-cpp` was
installed somewhere on `CMAKE_PREFIX_PATH`; it does not depend on the source tree.

The two programs are the main README's Quickstart §2 snippets, kept compilable
so the README cannot rot:

- **`authoring.cpp` → `x3d_embed_authoring`** — the hello world: build a scene
  graph in C++ (red `Sphere` under a `Shape`/`Appearance`/`Material`) and write
  it to `hello.x3d`. Links only the slim authoring surface
  (`x3d/authoring.hpp` + `x3d_cpp::authoring` — no runtime, no extraction),
  proving that target stays sufficient for authoring-only tools.
- **`main.cpp` → `x3d_embed_minimal`** — the render consumer: `RuntimeSession`
  over a parsed scene (defaults to `hello.x3d`, or pass a path), upload the
  `fullSnapshot()` (mesh/material/texture access), then tick + `delta()`.
  Links `x3d_cpp::sdk`, includes `x3d/sdk.hpp`.

```sh
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/path/to/x3d-cpp/install
cmake --build build
./build/x3d_embed_authoring          # writes hello.x3d
./build/x3d_embed_minimal            # parses it back, extracts the mesh
```

CI chains exactly this pair against a throwaway install prefix on every
C++-touching PR (`scripts/verify_install_embed.sh`, the
`x3d_install_embed_smoke` ctest): author → serialize → parse → extract, with
the exit code asserting real vertices came out.
