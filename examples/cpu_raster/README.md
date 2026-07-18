# Headless CPU rasterizer (`x3d_cpu_raster`)

An **out-of-SDK consumer** of the `x3d_cpp` extraction seam — like
`examples/poc_renderer/`, but it draws on the **CPU** and writes an image
instead of opening an OpenGL window. No GPU, GLFW, glad, or OpenGL: both the
binary and its unit tests run on a bare CI runner with no display, which is the
point — it's a **golden-image / visual-regression harness** for the extraction
pipeline.

Image I/O uses single-header **stb** (vendored, not system libraries, so the
bare-runner property holds): output is **PNG** by default (vendored
`stb_image_write`, a consumer-side encoder; pass `-o foo.ppm` for the
dependency-free P6 writer), and PNG/JPEG texture *input* is decoded by
**explicitly opting into the SDK's `TextureResolver` decode-seam backend**
(`-DX3D_CPP_BUILD_STB=ON`, ADR-0024) — the example composes a first-party SDK
seam rather than vendoring its own decoder. `proc:`/`.ppm` stay built-in.

It is firewalled behind `X3D_CPP_BUILD_CPURASTER=OFF`, so the default
build/golden/ctest path never compiles any of it.

## What it covers

- **All three `MaterialModel`s** the extraction seam emits, as CPU ports of the
  PoC GLSL shaders:
  - **Phong** (`Material`) — Blinn-Phong + textures + screen-space-derivative
    normal mapping (`lit.frag`).
  - **Physical** (`PhysicalMaterial`) — metallic-roughness GGX BRDF, directional
    lights, no IBL (`pbr.frag`).
  - **Unlit** (`UnlitMaterial` / lines / points / normal-less) (`unlit.frag`).
- **GLSL emulation**, in two layers:
  1. A `glsl::` math layer (vec/mat/builtins/`sampler2D`) so the fixed-function
     shaders read as near-verbatim ports of the GL GLSL — same lighting math,
     same sRGB encode, same texture-slot semantics, including `dFdx`/`dFdy` via
     a real **2×2-quad** rasterizer (helper invocations, like a GPU).
  2. A **GLSL-subset interpreter** (`GlslInterpreter.hpp`) that *executes* author
     `ComposedShader` fragment source on the CPU — the path you reach with
     `--frag`. When the SDK wires `RenderItem::shaderProgram`, the same machinery
     binds it automatically.
- **Textures** via a `TextureResolver`: `ImageTexture url="proc:<name>"`
  synthesizes a test texture (`checker`, `uvgrid`, `gradient`, `brick`, `bars`)
  on the fly and `url="foo.ppm"` decodes a binary PPM with no decoder at all;
  PNG/JPEG decode through the opt-in stb seam (`-DX3D_CPP_BUILD_STB=ON`).
  `PixelTexture` (inline) works directly. `--gentex <dir>` dumps the procedural set as PPMs.
  (`uvgrid` is an orientation probe: red=U, green=V — a no-V-flip check you can
  read off the image.)
- **Text** rendered as readable letters via a built-in public-domain 8×8 glyph
  atlas (`BuiltinFont.hpp` + `font8x8.hpp`): the rasterizer supplies a real
  `FontMetrics` (with atlas UVs) to the extractor and alpha-tests the atlas in the
  glyph draw. Without a font atlas the SDK's metrics-only stub fills each glyph
  cell solid (bars, not letters) — this closes that gap.
- **`Background` sky/ground gradient**: the bound `Background`'s
  `skyColor`/`skyAngle` + `groundColor`/`groundAngle` ramps fill the frame as a
  per-pixel spherical gradient (a lone `skyColor` stays a flat clear).
- **Two-sided materials** (`Appearance.backMaterial`): back-facing fragments are
  shaded with the back material when present and model-compatible.
- **`TextureCoordinateGenerator` (Sphere)**: base-texture UVs are generated from
  the camera-space normal (sphere/reflection map) when the geometry binds one.
- **`Background` skybox**: the bound `Background`'s six panorama faces
  (`frontUrl`/`backUrl`/`leftUrl`/`rightUrl`/`topUrl`/`bottomUrl`) are resolved
  through the texture resolver and drawn as an infinite cube around the viewer,
  composited over the sky/ground gradient by texel alpha (§Background). A
  ready-made CC0 cubemap lives in `assets/skybox/` (`assets/skybox_smoke.x3d`).
- Per-path world transforms, perspective-correct interpolation, a z-buffer,
  back-face culling honoring `MeshData.ccw/solid`, near-plane clipping, the
  view-all fit camera, eye-space directional + positional (point/spot) lights
  with distance attenuation and spot-cone falloff + the `NavigationInfo`
  headlight fallback, and a back-to-front transparency pass — matching the PoC's
  conventions so a CPU frame lines up with the GL one.

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

Encode them to WebM + animated-GIF heroes (needs `ffmpeg` with `libvpx-vp9`):

```sh
mise run demos     # -> docs/videos/demos/*.webm + docs/images/gallery/anim-*.gif
```

Regression is byte-exact and needs no ffmpeg: `demo_animation_test` renders fixed
sample frames and compares them to committed golden PPMs
(`tests/golden/demos/`). Regenerate goldens after an intentional visual change
with `mise run demos-bless`.

## Layout

```
cpuraster/glsl.hpp            GLSL-emulation vec/mat/builtins/sampler
cpuraster/Framebuffer.hpp     RGBA8 + depth buffer + PPM writer
cpuraster/Texture.hpp         CPU sampler2D (bilinear, wrap, sRGB-on-sample)
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
