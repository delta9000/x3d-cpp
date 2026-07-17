# Headless X3D → SVG projector (`x3d2svg`)

An out-of-SDK **consumer** of the x3d-cpp extraction seam — not part of the SDK.
It parses an X3D scene, resolves a camera, projects every extracted triangle
through a pinhole camera, flat-shades each facet with a headlight using the
material's composed colour, and paints them back-to-front (painter's algorithm)
into an SVG. With `--animate` it steps the simulation clock and emits one
SMIL-keyed `<g>` per frame, so an animated X3D world becomes a single
self-contained **animated SVG** — all headless, no GPU, no display.

## What it demonstrates

`x3d2svg` is the **smallest full renderer in the tree**, and it makes one point
sharply: **the public SDK façade alone is enough to write a real renderer.**

- It links **only `x3d_cpp::sdk`** and includes **only `"x3d/sdk.hpp"`** — where
  `cpu_raster` links the internal `x3d_cpp::x3d_cpp` target and reaches into
  `runtime/` headers, this tool touches nothing private. (The one documented
  exception: `x3d::core::SFVec3f`, the value type `Mat4::transformPoint` needs,
  which the façade does not re-export.)
- **Zero third-party dependencies** — no image library, no font backend, no GL.
- The entire SDK contact surface is a handful of calls: `parseFile` →
  `RuntimeSession::create` → `extractor().sceneWorldBounds()` / `camera()` /
  `item()` → `MaterialDesc::toRGBA()` / `Mat4::transformPoint()`, and the
  per-frame `tick()` / `delta()` loop. See `x3d2svg.hpp`.

## Build & run

```sh
mise run x3d2svg          # configure + build + ctest (build-x3d2svg/)
# or directly:
cmake -S . -B build-x3d2svg -G Ninja -DX3D_CPP_BUILD_X3D2SVG=ON
cmake --build build-x3d2svg
```

```sh
BIN=build-x3d2svg/examples/x3d2svg/x3d_x3d2svg
$BIN examples/x3d2svg/assets/smoke.x3d -o smoke.svg          # a still
$BIN scene.x3d -o anim.svg --animate 24 --fps 24             # an animated SVG
$BIN scene.x3d --fit -o fit.svg                              # force a view-all camera
$BIN scene.x3d --headless                                    # extract + report, write nothing
```

### Options

| Flag | Meaning |
|---|---|
| `-o FILE` | output SVG (default `out.svg`) |
| `-w W`, `-h H` | pixel dimensions (default 900×600) |
| `--animate N` | emit `N` frames as one animated (SMIL) SVG |
| `--fps F` | frames per second for `--animate` (default 30) |
| `--fit` | force a view-all camera, ignoring any bound `Viewpoint` |
| `--headless` | extract + print counts, write no file (the CI probe) |

## Camera resolution

By default the tool honours the scene's bound `Viewpoint`. When none is bound
(many authored scenes rely on the browser default), it synthesizes a **view-all
fit** from `extractor().sceneWorldBounds()`, framing the scene from a gentle 3/4
angle — the same fallback `cpu_raster` uses. As a safety net, if a bound
`Viewpoint` frames *nothing* (e.g. a geo-located tile the camera doesn't point
at), the tool automatically falls back to the fit so an arbitrary scene still
yields an image. `--fit` forces the fit unconditionally.

## Scope (honest boundaries)

- **Text renders nothing.** Glyph geometry requires a `FontMetrics` backend,
  which this tool deliberately does not wire (keeping the dependency set to just
  the SDK). A `Text`-only scene extracts zero items — by design.
- **Triangles crossing the near plane are culled, not clipped.** A whole
  triangle with any vertex behind the camera is dropped. For most scenes this is
  invisible; for a large ground plane viewed edge-on it can drop the plane.
- **No hidden-surface removal beyond a per-facet painter's sort.** Interpenetrating
  geometry can show sort artefacts; this is a technical-illustration renderer,
  not a z-buffer.
- **Raw coordinates only.** A `GeoElevationGrid` without a wired geo-projection
  is drawn in its raw lat/long/elevation space (degrees × metres), which can be
  a degenerate sliver — a faithful reflection of an unwired SDK seam, not a tool
  bug.

Validated headless against the Web3D example corpus (256 scenes: 0 crashes, and
the only empties are `Text`- or `Background`-only scenes with no mesh geometry).

## Layout

```
main.cpp            arg parsing + orchestration (parse → camera → frames → SVG)
x3d2svg.hpp         the reusable, unit-tested core: Vec3 math, Camera (bound or
                    view-all fit), triangle projection + flat shading, SVG emit
tests/project_test.cpp   projection unit tests (ctest x3d_x3d2svg_project)
assets/smoke.x3d    the gate scene (box + sphere + cone, one bound Viewpoint)
```
