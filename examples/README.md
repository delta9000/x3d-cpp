# x3d::sdk examples

Headless, std-only programs that use only the public `x3d::sdk` façade
(`#include "x3d/sdk.hpp"`, link `x3d_cpp::sdk`). They build + run as ctests when
configured with `-DX3D_CPP_BUILD_EXAMPLES=ON` (the default when this is the
top-level project).

| File | Shows |
|---|---|
| `01_load_validate_convert.cpp` | Load a document (any encoding, or a built-in inline scene), report conformance diagnostics, convert to X3D-XML / X3D-JSON / ClassicVRML. |
| `02_extract_render_feed.cpp` | Wire an execution context, take a full snapshot, and print the render items (mesh topology, counts, material), the camera, the lights, and a per-tick delta — the renderer-feed path, headless. |
| `03_attach_behavior_tick.cpp` | Subclass `x3d::sdk::System`, register it with `ctx.addSystem`, and drive ticks; the system rotates a DEF'd Transform via `ctx.writeField`. |

Run after building:

```sh
cmake --preset dev -DX3D_CPP_BUILD_EXAMPLES=ON
cmake --build build -j4 --target \
    x3d_example_01_load_validate_convert \
    x3d_example_02_extract_render_feed \
    x3d_example_03_attach_behavior_tick
./build/x3d_example_01_load_validate_convert [optional/path/to/scene]
./build/x3d_example_02_extract_render_feed
./build/x3d_example_03_attach_behavior_tick
```

`poc_renderer/` is a separate full OpenGL consumer (off by default,
`-DX3D_CPP_BUILD_POC=ON`), kept as an out-of-SDK integration proof.

`cpu_raster/` is a **dependency-free headless CPU rasterizer** (off by default,
`-DX3D_CPP_BUILD_CPURASTER=ON`, or `mise run cpuraster`). It consumes the same
extraction seam, supports all three material models, emulates the GLSL pipeline
(including a GLSL-subset interpreter for author `ComposedShader` source), and
writes a PPM — a GPU-free golden-image harness. See `cpu_raster/README.md`.
