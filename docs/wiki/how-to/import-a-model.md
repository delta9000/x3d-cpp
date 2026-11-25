---
title: Import a model
summary: Convert a glTF, OBJ, FBX or USD model to X3D with `x3d import` or importFile().
tags: [how-to, import, gltf, usd, assimp, cli]
updated: 2026-11-25
related:
  - ../reference/subsystems/asset-import.md
  - ../reference/subsystems/gltf-export.md
---

# Import a model

What each format carries into X3D, and what is approximated, is in the
[mesh import reference](../reference/subsystems/asset-import.md).

## 1. Build the backends you need

glTF is always built. Turn on the others when you configure:

```sh
# Assimp needs a system package, e.g. `apt-get install libassimp-dev`.
cmake --preset dev -DX3D_CPP_BUILD_STB=ON -DX3D_CPP_BUILD_ASSIMP=ON -DX3D_CPP_BUILD_USD=ON
cmake --build --preset dev --target x3d_cli
```

The `import` preset (`mise run build-import`) does the same in `build-import/`
and runs the import tests. `X3D_CPP_BUILD_STB` lets the importer re-encode
images that are not PNG or JPEG. `x3d import --list-backends` shows what the
binary has.

## 2. Convert from the command line

```sh
x3d import model.glb -o out/model.x3d --verify --stats
```

- Textures go to `out/assets/`, once each, named by content hash. Pass
  `--assets-dir` to put them elsewhere, or `--no-textures` to skip them.
- The encoding follows the output extension: `.x3d`, `.x3dv` or `.json`.
- The profile is the smallest that fits. Pass `--profile full` to keep Full.
- `--verify` re-reads the output and runs the `validate` checks; exit code 3
  means it found something.
- A model with animations gets one looping TimeSensor per animation. Only the
  first starts enabled; pass `--autoplay all` or `--autoplay none` to change
  that.

For a USD material whose `ior`, clearcoat or specular workflow matters, add
`--emit-glsl out/shaders` to write one fragment shader per material.

## 3. Check the round trip

`x3d extract` writes glTF, so a scene can go out and back:

```sh
x3d extract scene.x3d -o scene.glb
x3d import scene.glb -o back/scene.x3d --verify
```

## 4. Import from code

```cpp
#include "x3d/sdk.hpp"
namespace sdk = x3d::sdk;

sdk::ImportOptions opts;
opts.outputPath = "out/model.x3d";   // texture urls are relative to this
sdk::ImportResult r = sdk::importFile("model.glb", opts);
for (const auto &w : r.report.warnings) std::cerr << w << "\n";
for (const auto &w : r.report.emit.warnings) std::cerr << w << "\n";
std::ofstream("out/model.x3d") << sdk::XmlWriter{}.writeDocument(r.document);
```

`importFile` throws `std::runtime_error` when no backend reads the file or the
backend fails. To add the optional backends, link `x3d_import_assimp` or
`x3d_import_usd` from the source tree and pass a registry:

```cpp
#include "AssimpSource.hpp"
#include "UsdSource.hpp"

auto reg = sdk::importer::builtinBackends();
sdk::importer::addAssimpBackend(reg);
sdk::importer::addUsdBackend(reg);
sdk::ImportResult r = sdk::importFile("model.usdz", opts, reg);
```

Set `opts.decoder = x3d::runtime::io::stb::makeStbTextureResolver();` (from
`StbTextureResolver.hpp`, `x3d_stb`) to have images that are not PNG or JPEG
written as PNG.
