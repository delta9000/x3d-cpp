---
title: Asset import converter
summary: How to build and run x3d_asset_import, its options, the portable GLSL output and asset licensing.
tags: [reference, examples, asset-import]
updated: 2026-11-14
related:
  - index.md
---

# Asset import converter

Source: `examples/asset_import/`. `x3d_asset_import` converts foreign assets into X3D 4.0 documents and writes
texture assets alongside the output. The conversion model is described in the
[subsystem reference](../subsystems/asset-import.md).

## Backends

See [backend selection](../subsystems/asset-import.md#backend-selection)
for available formats, priorities, and build options. `--backend <name>` forces
an implementation.

## Build

The default configuration includes fixture and cgltf backends. The mise task
also runs the converter's CTests, whose names start with `x3d_assetimport`:

```sh
mise run asset-import
# equivalently:
cmake -S . -B build-asset-import -G Ninja -DX3D_CPP_BUILD_ASSET_IMPORT=ON -DX3D_CPP_BUILD_STB=ON
cmake --build build-asset-import
```

Add a real backend by turning on its flag (and reconfiguring):

```sh
# USD / USDZ (fetches + patches tinyusdz via FetchContent)
cmake -S . -B build-asset-import -G Ninja \
    -DX3D_CPP_BUILD_ASSET_IMPORT=ON -DX3D_CPP_BUILD_STB=ON -DX3D_CPP_BUILD_USD=ON
cmake --build build-asset-import

# Assimp (OBJ/glTF/FBX/…)
cmake -S . -B build-asset-import -G Ninja \
    -DX3D_CPP_BUILD_ASSET_IMPORT=ON -DX3D_CPP_BUILD_STB=ON -DX3D_CPP_BUILD_ASSIMP=ON
cmake --build build-asset-import
```

`-DX3D_CPP_BUILD_STB=ON` wires the SDK's `stb` texture-decode seam so the texture
pipeline can re-encode non-web-safe images to PNG. The binary lands at
`build-asset-import/examples/asset_import/x3d_asset_import`.

## CLI

```
x3d_asset_import <input> [options]

  -o <path>                 output file (default: stdout)
  --format xml|json|vrml|canonical
  --assets-dir <dir>        where extracted/copied textures are written (creates <dir>/assets/)
  --no-textures             skip the texture pipeline
  --recompress              re-encode web-safe textures instead of passing them through
  --profile auto|interchange|immersive|full
  --emit-glsl <dir>         write one portable UsdPreviewSurface .frag per material (see below)
  --verify                  re-parse the emitted X3D with the full SDK and diff node/route counts
  --stats                   print import statistics
```

Examples:

```sh
BIN=build-asset-import/examples/asset_import/x3d_asset_import

# synthetic cube, verified round-trip
$BIN fixture:cube -o /tmp/cube.x3d --stats --verify

# a USDZ with textures extracted next to the output
$BIN model.usdz -o /tmp/model.x3d --assets-dir /tmp/out --stats

# also emit portable per-material shaders
$BIN model.usdz -o /tmp/model.x3d --assets-dir /tmp/out --emit-glsl /tmp/shaders
```

## Materials & the portable-GLSL seam

USD materials port through `UsdPreviewSurface` (the one shading model tinyusdz
resolves; MaterialX / arbitrary graphs are flatten-or-skip). The X3D
`PhysicalMaterial` node maps the glTF-metallic-roughness subset
(baseColor / metallic / roughness / emissive / occlusion / normal + textures);
it has no node for `ior` / `clearcoat` / specular-workflow / `opacityMode`.

- `--emit-glsl` carries that extra fidelity: it writes a self-contained
  `examples/cpu_raster/shaders/usd_preview_surface.frag` per material with the
  UsdPreviewSurface parameters baked in as `const`.
- The same canonical shader runs byte-identically in the CPU rasterizer's GLSL
  interpreter (`x3d_cpu_raster --frag`) and on desktop OpenGL
  (`x3d_poc_renderer --pbr-shader`).
- See
  [ADR-0043](../../explanation/decisions/0043-usd-material-portable-glsl-seam.md).

## Showcase

`examples/asset_import/assets/showcase/` converts one procedural torus knot authored in
OBJ, glTF, and USD through the three backends, showing the format-agnostic
pipeline and the material-model mapping (OBJ → Phong `Material`; glTF & USD →
`PhysicalMaterial`):

The rendered comparison is `examples/asset_import/assets/showcase/showcase.png`.

All showcase assets are authored in-repo (license-clean). See its
`examples/asset_import/assets/showcase/README.md` for the regenerate/convert/render recipe.

## Example models & licensing

This repo is MIT, so keep every committed asset MIT-clean to redistribute:
CC0, or CC-BY with attribution. Avoid NonCommercial, NoDerivatives, or otherwise
MIT-incompatible licenses. Third-party build/runtime dependencies are attributed in
the root `NOTICE`.

- Committed textured-PBR demo: `examples/cpu_raster/assets/models/lion_head/`
  (CC0, with a regeneration recipe).
- The conversion showcase (`examples/asset_import/assets/showcase/`) meshes are generated in-repo, so they
  carry no external license.
- To try USD/USDZ on a downloaded model, convert it from a local copy; only commit
  an asset once you've confirmed its license is CC0 or CC-BY.

## Related

- [Examples](index.md)
