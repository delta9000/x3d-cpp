# Skybox cubemap test assets

Six-face cube panoramas for exercising the X3D `Background` skybox path in the
headless CPU rasterizer (see `docs/superpowers/plans/2026-06-24-skybox-background.md`).
Faces are **PNG**, decoded via the rasterizer's opt-in stb decode seam
(`-DX3D_CPP_BUILD_STB=ON`).

## suburban_garden/

| File | Cube direction (from origin) | yaw / pitch |
|------|------------------------------|-------------|
| `front.png`  | −Z | 0,   0   |
| `back.png`   | +Z | 180, 0   |
| `right.png`  | +X | 90,  0   |
| `left.png`   | −X | −90, 0   |
| `top.png`    | +Y | 0,   90  |
| `bottom.png` | −Y | 0,   −90 |

1024×1024 each (from the 4k source HDRI). The four side faces tile into a seamless 360° panorama; `top` is
sky (with the sun as a clipped highlight) and `bottom` is the lawn.

Use from a scene:

```xml
<Background frontUrl='"skybox/suburban_garden/front.png"'
            backUrl='"skybox/suburban_garden/back.png"'
            rightUrl='"skybox/suburban_garden/right.png"'
            leftUrl='"skybox/suburban_garden/left.png"'
            topUrl='"skybox/suburban_garden/top.png"'
            bottomUrl='"skybox/suburban_garden/bottom.png"'/>
```

(URLs resolve relative to the scene file via the example's `TextureResolver`.)

## Source & license

- **suburban_garden** by Greg Zaal / Poly Haven — <https://polyhaven.com/a/suburban_garden>
- License: **CC0** (public domain) — no attribution required; credited here as courtesy.

## Regenerating / adding another HDRI

```sh
# defaults: suburban_garden @ 4k -> 1024px faces
./make_cubemap.sh
# or: <slug> <source-res> <face-size>
./make_cubemap.sh kloofendal_48d_partly_cloudy 4k 512
```

`make_cubemap.sh` downloads the equirectangular HDRI from the Poly Haven API and
converts it. **Why two tools:** ImageMagick has no equirectangular→cube-face
projection, so ffmpeg's `v360` filter does the geometric reprojection and
ImageMagick does the HDR→LDR tone shaping (`-gamma 1.8 -sigmoidal-contrast
3x42%`) and PPM packaging. Deps: `curl`, `ffmpeg` (with `v360`), ImageMagick.
