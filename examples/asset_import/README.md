# asset_import — external assets → X3D 4.0 converter

`x3d_asset_import` is an out-of-SDK example that converts external 3D assets into
conformant X3D 4.0, built on the slim `x3d_cpp::authoring` target (no parser /
runtime / scripting / physics — see [ADR-0042](../../docs/wiki/decisions/0042-authoring-target.md)).
It is also the reference consumer for the USD material port and the portable-GLSL
material seam ([ADR-0043](../../docs/wiki/decisions/0043-usd-material-portable-glsl-seam.md)).

Pipeline: **backend → `ImportScene` IR → texture plan → `emit()` → X3D document → writer**.
The `ImportSource` seam keeps every third-party type behind a backend-agnostic POD IR
([`import_source.hpp`](import_source.hpp)); the [subsystem page](../../docs/wiki/subsystems/asset-import.md)
has the full data-flow and X3D mapping table.

## Backends

The backend is chosen from the input argument:

| Input | Backend | Build flag | Formats |
|---|---|---|---|
| `fixture:<name>` (e.g. `fixture:cube`) | `FixtureSource` | always on | synthetic scenes (dependency-free; CI + tests) |
| `*.usd` `*.usda` `*.usdc` `*.usdz` | `UsdSource` (tinyusdz) | `-DX3D_CPP_BUILD_USD=ON` | USD / USDZ, incl. composed references/payloads/subLayers and UsdPreviewSurface materials |
| anything else (e.g. `*.obj`, `*.gltf`, `*.fbx`) | `AssimpSource` | `-DX3D_CPP_BUILD_ASSIMP=ON` | 40+ formats via Assimp |

Both real backends are **OFF by default**; the default build ships only the
fixture backend, so it stays dependency-clean.

## Build

Fixture-only (what `mise run asset-import` does — configure + build + ctest):

```sh
mise run asset-import
# equivalently:
cmake -S . -B build-asset-import -G Ninja -DX3D_CPP_BUILD_ASSET_IMPORT=ON -DX3D_CPP_BUILD_STB=ON
cmake --build build-asset-import
ctest --test-dir build-asset-import -R x3d_assetimport --output-on-failure
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
it has **no** node for `ior` / `clearcoat` / specular-workflow / `opacityMode`.

`--emit-glsl` carries that extra fidelity: it writes a self-contained
[`usd_preview_surface.frag`](../cpu_raster/shaders/usd_preview_surface.frag) per
material with the UsdPreviewSurface parameters baked in as `const`. The same
canonical shader runs byte-identically in the CPU rasterizer's GLSL interpreter
(`x3d_cpu_raster --frag`) and on desktop OpenGL (`x3d_poc_renderer --pbr-shader`).
See [ADR-0043](../../docs/wiki/decisions/0043-usd-material-portable-glsl-seam.md).

## Showcase

[`assets/showcase/`](assets/showcase/) converts one cube authored in **OBJ**, **glTF**,
and **USD** through the three backends, showing the format-agnostic pipeline and the
material-model mapping (OBJ → Phong `Material`; glTF & USD → `PhysicalMaterial`):

![OBJ / glTF / USD conversion showcase](assets/showcase/showcase.png)

All showcase assets are authored in-repo (license-clean). See its
[README](assets/showcase/README.md) for the regenerate/convert/render recipe.

## Example models & licensing

**Only bundle assets whose license permits redistribution in an MIT project
(CC0, or CC-BY *with* attribution). Do not vendor NonCommercial (NC),
NoDerivatives (ND), or share-alike-incompatible assets** — the SDK is MIT and is
used commercially, so an NC/ND asset would restrict downstream users. Third-party
build/runtime dependencies are attributed in the root [`NOTICE`](../../NOTICE).

- Committed textured-PBR demo: [`../cpu_raster/assets/models/lion_head/`](../cpu_raster/assets/models/lion_head/)
  (**CC0**, with a regeneration recipe).
- The Khronos/USD-WG **DamagedHelmet** used while developing the USD material path
  is **CC BY-NC** ("Battle Damaged Sci-fi Helmet - PBR" by theblueturtle_) — a
  handy local test asset, but **not vendored here**: the NonCommercial clause is
  incompatible with an MIT repo. Convert it locally from a copy you fetch yourself;
  do not commit the `.usdz`, the converted `.x3d`, its textures, or renders of it.
- For a committed USD demo, convert a CC0 / permissive USDZ.
