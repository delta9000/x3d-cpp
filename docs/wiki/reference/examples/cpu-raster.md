---
title: Headless CPU rasterizer
summary: What x3d_cpu_raster covers, how to build and run it, its options and author-shader conventions.
tags: [reference, examples, rendering]
updated: 2026-11-14
related:
  - index.md
---

# Headless CPU rasterizer

Source: `examples/cpu_raster/`. An out-of-SDK consumer of the `x3d_cpp` extraction seam — like
`examples/poc_renderer/`, but it draws on the CPU and writes an image
instead of opening an OpenGL window. No GPU, GLFW, glad, or OpenGL: both the
binary and its unit tests run on a bare CI runner with no display, It supplies image regression checks for the extraction pipeline.

Image I/O uses single-header stb (vendored, not system libraries, so the
bare-runner property holds): output is PNG by default (vendored
`stb_image_write`, a consumer-side encoder; pass `-o foo.ppm` for the
dependency-free P6 writer), and PPM/PNG/JPEG/DDS texture *input* is decoded by
explicitly opting into the SDK's `TextureResolver` decode-seam backend
(`-DX3D_CPP_BUILD_STB=ON`, ADR-0024) — the example composes a first-party SDK
seam rather than vendoring its own decoder. `proc:` textures stay built-in.

It is firewalled behind `X3D_CPP_BUILD_CPURASTER=OFF`, so the default
build/golden/ctest path never compiles any of it.

## What it covers

| Area | Behavior |
|---|---|
| Material models | All three `MaterialModel`s the extraction seam emits, as CPU ports of the PoC GLSL shaders: Phong (`Material`; Blinn-Phong, textures, screen-space-derivative normal mapping; `lit.frag`), Physical (`PhysicalMaterial`; metallic-roughness GGX BRDF, directional lights, no IBL; `pbr.frag`) and Unlit (`UnlitMaterial`, lines, points, normal-less geometry; `unlit.frag`) |
| `FillProperties` | Independent filling and hatching, hatch color, required styles 1 to 6 and fallback to style 1. Polygon holes leave depth untouched; lines and points are unaffected. Hatches use an eight-pixel window-space grid, preserve material alpha and coverage, and receive fog |
| GLSL math layer | `glsl::` vec, mat, builtins and `sampler2D`, so the fixed-function shaders read as near-verbatim ports of the GL GLSL: same lighting math, sRGB encode and texture-slot semantics, with `dFdx`/`dFdy` from a 2×2-quad rasterizer with helper invocations |
| GLSL interpreter | `GlslInterpreter.hpp` executes author `ComposedShader`/`ProgramShader` fragment source. Each item's selected program (`RenderItem::shaderProgram`) runs automatically with its author `<field>`s as uniforms (`AuthorShader.hpp`). A program the interpreter cannot compile is invalid, so the SDK falls through to the next shader or the material. `--frag` forces one shader onto every item |
| Textures | Through a `TextureResolver`. `ImageTexture url="proc:<name>"` synthesizes a test texture (`checker`, `uvgrid`, `gradient`, `brick`, `bars`). PPM, PNG, JPEG and DDS files decode through the SDK stb seam (`-DX3D_CPP_BUILD_STB=ON`) with bottom-left-origin pixels. `PixelTexture` works directly. `--gentex <dir>` dumps the procedural set as PPMs. `uvgrid` is an orientation probe: red is U, green is V |
| `TextureCoordinateGenerator` | The reference shaders evaluate the eleven modes and apply each stage's texture transform; see [texture extraction](../subsystems/extract-textures.md) |
| Text | Readable letters from a built-in public-domain 8×8 glyph atlas (`BuiltinFont.hpp`, `font8x8.hpp`): the rasterizer supplies a `FontMetrics` with atlas UVs and alpha-tests the atlas in the glyph draw. Without an atlas the SDK's metrics-only stub fills each glyph cell solid |
| `Background` gradient | The bound `Background`'s `skyColor`/`skyAngle` and `groundColor`/`groundAngle` ramps fill the frame as a per-pixel spherical gradient; a lone `skyColor` stays a flat clear |
| `Background` skybox | The six panorama faces are resolved through the texture resolver and drawn as an infinite cube around the viewer, composited over the gradient by texel alpha. A CC0 cubemap is in `assets/skybox/` (`assets/skybox_smoke.x3d`) |
| Two-sided materials | `Appearance.backMaterial`: back-facing fragments use the back material when present and model-compatible |
| Rasterization | Per-path world transforms, perspective-correct interpolation, a z-buffer, back-face culling honoring `MeshData.ccw`/`solid`, near-plane clipping, the view-all fit camera and a back-to-front transparency pass |
| Lights | Eye-space directional and positional (point, spot) lights with distance attenuation and spot-cone falloff, plus the `NavigationInfo` headlight (§23.4.4: on whenever `headlight` is TRUE, independent of scene lights) |

The conventions match the PoC renderer's, so a CPU frame lines up with the GL
one.

## Build & run

```sh
mise run cpuraster          # configure + build + ctest (build-cpuraster/)
# or manually (X3D_CPP_BUILD_STB=ON wires the SDK PNG/JPEG decode seam):
cmake -S . -B build-cpuraster -G Ninja -DX3D_CPP_BUILD_CPURASTER=ON -DX3D_CPP_BUILD_STB=ON
cmake --build build-cpuraster

# render the bundled smoke scene to a PNG (default; -o out.ppm for P6)
./build-cpuraster/examples/cpu_raster/x3d_cpu_raster -o out.png -w 800 -H 600

# render the skybox demo (Background panorama cube)
./build-cpuraster/examples/cpu_raster/x3d_cpu_raster \
    examples/cpu_raster/assets/skybox_smoke.x3d -o skybox.png

# run an author GLSL fragment shader through the interpreter
./build-cpuraster/examples/cpu_raster/x3d_cpu_raster \
    --frag examples/cpu_raster/shaders/author_lambert.frag -o author.png

# headless self-check (counts only, no image) — CI-friendly
./build-cpuraster/examples/cpu_raster/x3d_cpu_raster --headless
```

### CLI

| Flag | Meaning |
|---|---|
| `[scene.x3d]` | scene to render (default `assets/raster_smoke.x3d`) |
| `-o, --out <file.png>` | output image (default `cpu_raster.png`; a `.ppm` extension writes PPM) |
| `-w, --width` / `-H, --height` | image size (default 800×600) |
| `--frag <file.glsl>` | run an author fragment shader via the interpreter |
| `--gentex <dir>` | write the procedural test textures as PPM files and exit |
| `--animate` | render a frame per timestep instead of one still (drives the demo WebMs) |
| `--fps <n>` | frames per second in `--animate` (default 30) |
| `--duration <s>` | seconds of animation to render in `--animate` (default 4.0) |
| `--frames-dir <dir>` | output dir for `frame_%04d.ppm` (required with `--animate`) |
| `--headless` | extract + self-check only; print counts, write no image |

## Author-shader conventions

Author fragment shaders read the same `in` varyings the PoC's `lit.vert` emits —
`vPosEye`, `vNormalEye`, `vColor`, `vTexCoord` (+ `gl_FrontFacing`) — and write
`FragColor`. The interpreter seeds a generous uniform set from the
`MaterialDesc` + eye-space lights (`uDiffuse`, `uBaseColor`, `uEmissive`,
`uNumLights`, `uLightDirEye[]`, `uLightColor[]`, `uShininess`, `uMetallic`,
`uRoughness`, `uTime`, …) and binds the base-color sampler as `uTexture`. See
`shaders/author_lambert.frag` for a worked example.

Runtime `GlslError` failures keep the material fallback, log once per draw, and
increment the thread-local `interpreterRuntimeErrors` count per failed fragment.
Other exceptions propagate to the host.

`dFdx`/`dFdy` are exact for the two varyings the rasterizer precomputes
derivatives for (`vPosEye`, `vTexCoord`); any other argument yields zero.

## Animation demos

The `--animate` mode steps simulation time (`X3DExecutionContext::tick`) over a
range and writes one PPM frame per step, so the runtime's TimeSensor →
interpolator → ROUTE machinery produces a moving picture. The demo scenes live
in `assets/demos/` (all `loop="true"`):

- `anim_position.x3d` — a textured sphere orbiting via `PositionInterpolator`.
- `anim_orientation.x3d` — a uvgrid sphere spinning via `OrientationInterpolator` (SLERP).
- `anim_color.x3d` — a sphere cycling hue via `ColorInterpolator` (HSV arc).
- `anim_dvd.x3d` — the X3D "DVD logo": a `Text` node driven by a `PositionInterpolator`
  with true edge reflections, tuned to graze a corner without hitting it (8 s loop).

Encode them to WebM and animated GIF (needs `ffmpeg` with `libvpx-vp9`):

```sh
mise run demos     # -> build-cpuraster/demos/*.webm + anim-*.gif (not committed)
```

Regression is byte-exact and needs no ffmpeg: `demo_animation_test` renders fixed
sample frames and compares them to committed golden PPMs
(`tests/golden/demos/`). Regenerate goldens after an intentional visual change
with `mise run demos-bless`.

### Kelp Forest flythrough

`assets/kelp_flythrough/flythrough_kelp.x3d` flies a bound `Viewpoint` through
the third-party NPS/MOVES Kelp Forest Exhibit, which is fetched on demand and
not committed. Provenance and the required credit are in
`examples/cpu_raster/assets/kelp_flythrough/README.md`. The committed outputs are
the README hero `docs/images/gallery/anim-kelp.gif` and
`docs/videos/demos/kelp_flythrough.webm`.

```sh
# 1. Build the rasterizer (once):
mise run cpuraster

# 2. Fetch the exhibit into ./exhibit/ (needs wget):
examples/cpu_raster/assets/kelp_flythrough/fetch_kelp.sh

# 3a. One-shot, everything wired (fetch-if-needed + render + encode; needs ffmpeg):
mise run kelp-flythrough        # -> kelp_flythrough.webm

# 3b. …or drive the binary yourself — a still:
build-cpuraster/examples/cpu_raster/x3d_cpu_raster \
    examples/cpu_raster/assets/kelp_flythrough/flythrough_kelp.x3d -o kelp.png -w 800 -H 450

# …or the animated flythrough (PPM frames -> your encoder):
build-cpuraster/examples/cpu_raster/x3d_cpu_raster \
    examples/cpu_raster/assets/kelp_flythrough/flythrough_kelp.x3d \
    --animate --fps 30 --duration 12 -w 800 -H 450 --frames-dir /tmp/kelp
```

`--animate` runs the standard behavior runtime (TimeSensor + interpolators), so
the kelp, fish, and pump mechanism move; a plain still freezes them in their
authored rest pose. Note the exhibit's `ElevationGrid` rock floor renders solid
thanks to the §13.3.4 surface-winding fix (conformance EXT-003).

## Layout

```
cpuraster/glsl.hpp            GLSL-emulation vec/mat/builtins/sampler
cpuraster/Framebuffer.hpp     RGBA8 + depth buffer + PPM writer
cpuraster/Texture.hpp         CPU sampler2D (§18.4.9 wrap/filter, sRGB-on-sample)
cpuraster/Rasterizer.hpp      2x2-quad scan-convert, clip, z-buffer, cull
cpuraster/MaterialShader.hpp  Phong/PBR/Unlit CPU ports
cpuraster/GlslInterpreter.hpp GLSL-subset interpreter (author ComposedShader)
cpuraster/BuiltinFont.hpp     glyph atlas + FontMetrics so Text renders as letters
cpuraster/font8x8.hpp         vendored public-domain 8x8 font (CC0/Public Domain)
cpuraster/ProceduralTexture.hpp generated test textures + PPM IO + TextureResolver
cpuraster/SceneRender.hpp     extractor items -> framebuffer (framing/lights/text)
main.cpp                      CLI driver
assets/raster_smoke.x3d       all-three-models smoke scene
assets/text_smoke.x3d         Text node for the glyph regression test
assets/textured_scene.x3d     primitives carrying proc: ImageTextures
shaders/author_lambert.frag   sample author shader for --frag
tests/                        ctest unit + end-to-end render tests
```

## Related

- [Examples](index.md)
