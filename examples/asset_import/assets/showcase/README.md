# Conversion showcase — OBJ · glTF · USD → X3D

One (2,3) torus knot (~6.7k verts / 13.4k tris, smooth-shaded), authored in three
source formats, run through `x3d_asset_import` to show that a single converter +
material pipeline handles all three backends and maps each to the right X3D 4.0
material node.

![showcase](showcase.png)

| Source | Backend | Material | X3D node |
|---|---|---|---|
| `knot.obj` + `knot.mtl` | assimp (`-DX3D_CPP_BUILD_ASSIMP=ON`) | copper `Kd`/`Ks` | `Material` (Phong) |
| `knot.gltf` (self-contained, embedded buffer) | assimp | gold metallic-roughness | `PhysicalMaterial` |
| `knot.usda` | tinyusdz (`-DX3D_CPP_BUILD_USD=ON`) | teal UsdPreviewSurface | `PhysicalMaterial` |

OBJ carries no PBR, so it imports as a Phong `Material`; glTF and USD both carry
metallic-roughness and map to `PhysicalMaterial`.

## License

The mesh is generated procedurally by [`gen.py`](gen.py) (a parametric torus knot
with rotation-minimizing frames) — no third-party models — so the showcase is free
of asset-licensing constraints (see the
[asset-licensing policy](../../README.md#example-models--licensing)). The generated
`knot.*` assets are `.gitignore`d (regenerable; not vendored, to keep the repo lean).

## Regenerate + convert + render

```sh
cd examples/asset_import/assets/showcase && python3 gen.py   # writes knot.obj/.mtl/.gltf/.usda
cd -

BIN=build-asset-import/examples/asset_import/x3d_asset_import   # built with ASSIMP + USD ON
for fmt in obj gltf usda; do
  "$BIN" examples/asset_import/assets/showcase/knot.$fmt -o /tmp/knot_$fmt.x3d --stats
done

# visualize (headless CPU rasterizer)
build-cpuraster/examples/cpu_raster/x3d_cpu_raster /tmp/knot_gltf.x3d -o /tmp/knot.ppm
```

The montage above was composited from three such renders under one three-point rig.
