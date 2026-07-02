# Conversion showcase — OBJ · glTF · USD → X3D

One cube, authored in three source formats, run through `x3d_asset_import` to show
that a single converter + material pipeline handles all three backends and maps each
to the right X3D 4.0 material node.

![showcase](showcase.png)

| Source | Backend | Material | X3D node |
|---|---|---|---|
| `cube.obj` + `cube.mtl` | assimp (`-DX3D_CPP_BUILD_ASSIMP=ON`) | copper `Kd`/`Ks` | `Material` (Phong) |
| `cube.gltf` (self-contained, embedded buffer) | assimp | gold metallic-roughness | `PhysicalMaterial` |
| `cube.usda` | tinyusdz (`-DX3D_CPP_BUILD_USD=ON`) | teal UsdPreviewSurface | `PhysicalMaterial` |

OBJ carries no PBR, so it imports as a Phong `Material`; glTF and USD both carry
metallic-roughness and map to `PhysicalMaterial`.

## License

All assets here are **authored in-repo** by [`gen.py`](gen.py) — no third-party
models — so the showcase is free of asset-licensing constraints (see the
[asset-licensing policy](../../README.md#example-models--licensing)).

## Regenerate + convert + render

```sh
python3 gen.py            # (re)writes cube.obj/.mtl/.gltf/.usda

BIN=build-asset-import/examples/asset_import/x3d_asset_import   # built with ASSIMP + USD ON
for fmt in obj gltf usda; do
  "$BIN" examples/asset_import/assets/showcase/cube.$fmt -o /tmp/cube_$fmt.x3d --stats
done

# visualize (headless CPU rasterizer)
build-cpuraster/examples/cpu_raster/x3d_cpu_raster /tmp/cube_gltf.x3d -o /tmp/cube.ppm
```

The montage above was composited from three such renders under one three-point rig.
