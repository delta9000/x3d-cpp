---
title: Examples
summary: Every example program in the repository, what it shows, how it is built and where its page is.
tags: [reference, examples]
updated: 2026-11-14
related:
  - cpu-raster.md
  - poc-renderer.md
  - x3d2svg.md
  - asset-import.md
---

# Examples

All examples live under `examples/`. The renderers are consumers of the SDK,
not part of it.

## SDK façade programs

Headless programs that use only `x3d/sdk.hpp` and `x3d_cpp::sdk`. They build
and run as CTests when `X3D_CPP_BUILD_EXAMPLES=ON` (the default for a
top-level build).

| File | Target | Shows |
|---|---|---|
| `01_load_validate_convert.cpp` | `x3d_example_01_load_validate_convert` | Load a document in any encoding, report diagnostics, convert to XML, JSON and ClassicVRML |
| `02_extract_render_feed.cpp` | `x3d_example_02_extract_render_feed` | Wire a context, take a full snapshot, print items, camera, lights and a per-tick delta |
| `03_attach_behavior_tick.cpp` | `x3d_example_03_attach_behavior_tick` | A custom `System` that rotates a Transform through `ctx.writeField` |

```sh
cmake --preset dev -DX3D_CPP_BUILD_EXAMPLES=ON
cmake --build build -j4 --target x3d_example_02_extract_render_feed
./build/x3d_example_02_extract_render_feed
```

## Installed-package consumer

`examples/embed_minimal/` is a separate CMake project that finds an installed
x3d-cpp through `CMAKE_PREFIX_PATH`.

| Source | Target | Shows |
|---|---|---|
| `authoring.cpp` | `x3d_embed_authoring` | Builds a scene with `SceneBuilder` and writes `hello.x3d`; links only `x3d_cpp::authoring` |
| `main.cpp` | `x3d_embed_minimal` | `RuntimeSession`, `fullSnapshot()`, then tick and `delta()` over that scene |
| `native_host.cpp` | `x3d_embed_native_host` | The [native host contract](../native-host-contract.md) checks |
| `ext.cpp` | `x3d_embed_ext` | The opt-in extension layer; built with `-DX3D_EMBED_EXT=ON` |

`scripts/verify_install_embed.sh` installs the package and chains these
programs. See [Build and install the SDK](../../how-to/build-and-install.md#check-the-install).

## Out-of-SDK consumers

Each is off by default and builds into its own directory.

| Example | Build option | mise task | Page |
|---|---|---|---|
| CPU rasterizer and GLSL emulation | `X3D_CPP_BUILD_CPURASTER` | `mise run cpuraster` | [Headless CPU rasterizer](cpu-raster.md) |
| OpenGL renderer | `X3D_CPP_BUILD_POC` | `mise run poc` | [PoC OpenGL renderer](poc-renderer.md) |
| SVG projector (façade only, no dependencies) | `X3D_CPP_BUILD_X3D2SVG` | `mise run x3d2svg` | [Headless SVG projector](x3d2svg.md) |
| Foreign asset converter | `X3D_CPP_BUILD_ASSET_IMPORT` | `mise run asset-import` | [Asset import converter](asset-import.md) |
| Headless SAI host | Built from `examples/sai_host/` | `mise run sai-provider` | [Headless host example](../../sai/reference/host-example.md) |
