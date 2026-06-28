# PoC OpenGL renderer (`x3d_poc_renderer`)

A small **out-of-SDK** OpenGL renderer that **consumes** the headless `x3d_cpp`
runtime SDK through the M2.5 extraction seam. The renderer is *not* part of the
SDK: it links only the `x3d_cpp::x3d_cpp` INTERFACE target and lives entirely
under `examples/poc_renderer/`. It is firewalled behind `X3D_CPP_BUILD_POC=OFF`,
so the normal `mise run build` / `mise run golden` / `ctest` path never
configures, compiles, or links any of it.

## What it does

- `parseFile(argv[1])` → `X3DDocument` → `Scene`
- `X3DExecutionContext::buildSceneGraph` + `buildFrom` (the `BridgeResult` is
  checked and logged; rejected ROUTEs are non-fatal — geometry still renders)
- `tick(0)`, then a GLFW Wayland window with a **GL core 3.3** context
  (forward-compatible, 24-bit depth) loaded via **glad**
- a `GL_KHR_debug` callback + a forced full-screen clear to the bound
  `Background` color (so a black screen is diagnosable)
- a render loop that draws each extracted `RenderItem` **lit** — Lambert +
  ambient, two-sided (`N = gl_FrontFacing ? N : -N`) — with `MaterialDesc`
  color (diffuse/emissive/transparency, per-vertex `Color` override) and
  per-draw back-face culling honoring `MeshData.ccw`/`solid`
  (`projection * view * model`, `view = ctx.viewMatrix()`, a PoC perspective
  built from `CameraDesc.fieldOfView`, near/far fit from
  `extractor.sceneWorldBounds()`). Lights are the extractor's world-resolved
  `LightDesc` directionals; when a scene authors none, the bound
  `NavigationInfo` headlight (default true) supplies a camera-space fallback.
- an interactive Dear ImGui diagnostics overlay for frame timing, render-item
  counts, texture-cache counts, runtime time, a wireframe toggle, and the ImGui
  demo window; **F1** toggles the panel. It is disabled for `--headless`, `--screenshot`, and `--animate`
  so CI captures stay deterministic.

With no argument it loads the bundled `assets/triangle.x3d` (first-light).

## Build

The PoC is OFF by default. Build it into a **separate** build dir so the dev
preset's `build/` (which `golden`/`ctest` depend on) stays pristine:

```sh
mise run poc
# → ./build-poc/examples/poc_renderer/x3d_poc_renderer
```

or by hand:

```sh
cmake -S . -B build-poc -G Ninja -DX3D_CPP_BUILD_POC=ON
cmake --build build-poc --target x3d_poc_renderer
./build-poc/examples/poc_renderer/x3d_poc_renderer            # bundled triangle
./build-poc/examples/poc_renderer/x3d_poc_renderer path/to/scene.x3d
```

### Headless probe (no display required)

`--headless` parses + extracts a scene, prints the `RenderItem` count and the
first item's vertex count, then exits `0` **without creating a GL context or a
window** (so it runs over SSH / in CI). This is the T10 build-side acceptance
check that the asset is drawable, independent of the on-screen visual verify:

```sh
./build-poc/examples/poc_renderer/x3d_poc_renderer --headless   # bundled triangle
# → render_items=1 first_item_vertices=3   (exit 0)
```

The same check runs in the default `mise run build` ctest path as
`x3d_poc_triangle_asset` (no GL, so it needs no display).

### Headless GL validation (Xvfb + software mesa)

The **real** GL pipeline is also exercised without a display via the `--screenshot`
path, which renders a few frames, reads the framebuffer back with `glReadPixels`,
writes a PPM, and exits. CI's `examples-gate` job and `mise run validate-examples`
run it under `xvfb-run` with mesa's software rasterizer (llvmpipe), mirroring the
sibling `../x3d-render` validation approach:

```sh
mise run validate-examples   # builds cpu_raster + poc, runs both probes + the GL smoke
# or directly:
xvfb-run -a env LIBGL_ALWAYS_SOFTWARE=1 GALLIUM_DRIVER=llvmpipe __GLX_VENDOR_LIBRARY_NAME=mesa \
  ./build-poc/examples/poc_renderer/x3d_poc_renderer --screenshot out.ppm scene.x3d
```

`__GLX_VENDOR_LIBRARY_NAME=mesa` forces the software vendor on boxes that also have
a GPU GLX driver (NVIDIA's libGL ignores `LIBGL_ALWAYS_SOFTWARE`); a GPU-less CI
runner lands on llvmpipe automatically.

GLFW (>=3.4) and Dear ImGui are fetched via `FetchContent` (GLFW native Wayland
on, X11 on as an XWayland safety net). The glad core-3.3 loader is committed
pre-generated at `third_party/glad/` (real `src/glad.c` + `include/glad/gl.h` +
`include/KHR/khrplatform.h`), so there is no generator step at build time.

## Arch / Wayland prerequisites

GLFW configures fine but **fails at build/run** with cryptic errors if the
native-Wayland dev libraries are missing. On Arch:

```sh
sudo pacman -S --needed \
    wayland wayland-protocols libxkbcommon \
    mesa libglvnd \
    extra-cmake-modules        # provides wayland-scanner glue for GLFW's build
```

On a proprietary NVIDIA setup (e.g. the RTX 4080 here) also install:

```sh
sudo pacman -S --needed libnvidia-egl-wayland   # EGL-on-Wayland platform glue
```

If native Wayland still misbehaves, the X11/XWayland path is compiled in as a
safety net; force it with `GDK_BACKEND=x11` / `SDL`-style env or by running
under XWayland. (GLFW picks Wayland automatically on a Wayland session.)

## Scope (this milestone)

- **M0 (T9):** window + context + camera + clear loop. *Done.*
- **M1 (T10):** draw one extracted mesh, unlit-flat, double-sided
  (`GL_CULL_FACE` OFF), via the minimal `SceneExtractor`. *Done.*
- **M2 (T11):** incremental `delta()`-driven updates + a USE'd shape rendered
  at both per-path transforms. *Done.*
- **M3 (T12):** per-pixel **lighting** (Lambert + ambient, two-sided),
  `MaterialDesc` color with per-vertex `Color` override, and per-draw
  back-face **culling** honoring `MeshData.ccw`/`solid`. The no-explicit-light
  fallback is the bound `NavigationInfo` headlight. *Done.*
  - Try it: `x3d_poc_renderer assets/lit_scene.x3d` — an explicit
    `DirectionalLight`, a solid culled IFS cube, a Phong `Box`, and a
    per-vertex-`Color` triangle.
- **M4 (T13):** textures via an asset resolver. *Deferred/optional.*

Build/link + the headless `--headless` and Xvfb `--screenshot` smokes are the CI
gate (`examples-gate`); the interactive GUI is still run by a human on a real
Wayland session for on-screen visual verification.
