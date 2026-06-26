# CPU-raster interpolator demos → WebM (design)

**Date:** 2026-06-25
**Status:** Approved (brainstorming) — pending implementation plan
**Scope:** A multi-frame ("animate") render mode for the headless CPU rasterizer
that steps simulation time, plus three interpolator demo scenes rendered to
committed short WebMs and byte-exact golden frames. Physics demos are explicitly
**deferred to a follow-up PR**.

## Motivation

The runtime's time-based animation is fully wired (`X3DExecutionContext::tick`
drives `TimeSensorSystem` + the interpolator systems through the ROUTE/event
cascade), but the `cpu_raster` example only ever calls `tick(0.0)` and renders a
single still frame. Nothing in the repo demonstrates animation over time, and
there is no moving-picture artifact in the docs. This adds a small,
out-of-SDK consumer-side pipeline that turns the existing per-tick machinery into
short showcase videos and, in the same render path, deterministic regression
goldens.

This serves two audiences at once (the agreed "Both"):
- **Showcase** — short WebMs embedded in the docs, like the existing hero gallery.
- **Visual regression** — golden frames sampled at fixed `t`, byte-compared in CI.

## Grounding (what already exists)

- `X3DExecutionContext::tick(double now)` (`runtime/events/X3DExecutionContext.hpp`)
  advances time, runs all attached systems, drains the cascade to quiescence, and
  propagates local→world transforms and bounds.
- `TimeSensorSystem` (`runtime/events/TimeSensorSystem.hpp`) emits
  `fraction_changed`; the generic `InterpolatorSystem<T>` consumes `set_fraction`
  and emits `value_changed`. SLERP for orientation and HSV-arc lerp for color are
  spec-correct (`runtime/events/Interpolation.hpp`).
- `cr::renderScene(ctx, extractor, opt)` (`examples/cpu_raster/cpuraster/SceneRender.hpp`)
  renders one frame from the current context + extractor snapshot. It is already
  reusable per-frame; nothing about it changes here.
- `SceneExtractor::fullSnapshot()` / `delta()` (`runtime/extract/SceneExtractor.hpp`)
  re-read the scene. The current `main.cpp` calls `fullSnapshot()` once.
- ffmpeg is already an established external dev tool in the repo
  (`examples/cpu_raster/assets/skybox/make_cubemap.sh`). No video library is linked.
- `Framebuffer` writes PNG (vendored stb) and dependency-free P6 PPM
  (`examples/cpu_raster/cpuraster/Framebuffer.hpp`).

## Architecture & data flow

```
scene.x3d
  → codec::parseFile → X3DExecutionContext::buildSceneGraph + buildFrom
  → attach behavior systems: TimeSensorSystem + interpolator systems   (NEW for cpu_raster)
  → for frame in 0 .. N-1 where N = round(duration * fps):
        t = frame / fps
        ctx.tick(t)                     # interpolators fire, ROUTEs mutate node fields
        extractor.fullSnapshot()        # re-extract so transform AND material changes land
        fb = renderScene(ctx, extractor, opt)   # unchanged existing call
        fb.writePNG(frames-dir/frame_%04d.png)
  → ffmpeg muxes frames → docs/videos/demos/<scene>.webm   # showcase (mise task)
  → frames at fixed t == committed PPM goldens             # regression (no ffmpeg)
```

The only new C++ is (a) attaching the TimeSensor + interpolator systems in the
example, and (b) the frame loop. Per-frame `fullSnapshot()` is chosen for
correctness simplicity: position/orientation land via re-propagated world
transforms and color lands via re-read material, with no delta bookkeeping. The
demo scenes are tiny, so the cost is irrelevant. (`delta()` is noted as a future
optimization, not implemented here.)

## Components

### 1. `--animate` mode in `examples/cpu_raster/main.cpp`

New CLI flags (existing flags unchanged):

| Flag | Meaning | Default |
|---|---|---|
| `--animate` | enable frame-stepping render mode | off |
| `--fps <n>` | frames per second | 30 |
| `--duration <s>` | seconds of wall-clock animation to render | 4.0 |
| `--frames-dir <dir>` | output dir for `frame_%04d.png` | required with `--animate` |

Behavior: parse + `buildSceneGraph` + `buildFrom`; attach `TimeSensorSystem` and
the interpolator systems (reuse the runtime's existing attach helpers — e.g.
`attachInterpolators` and `TimeSensorSystem::attach` per node, mirroring
`runtime/events/tests/interpolator_conformance_test.cpp` and `sim_runtime.hpp`);
then run the loop above. Validation: `fps > 0`, `duration > 0`, frames-dir
writable; clear error otherwise. Non-`--animate` behaviour is byte-for-byte
unchanged.

### 2. Demo scenes — `examples/cpu_raster/assets/demos/`

Each is `loop=true`, `cycleInterval ≈ 4s`, lit, over a gradient `Background`.

- `anim_position.x3d` — `TimeSensor → PositionInterpolator → Transform.translation`;
  a shape orbiting / bouncing along a closed path (so the loop is seamless).
- `anim_orientation.x3d` — `TimeSensor → OrientationInterpolator → Transform.rotation`;
  a tumbling textured `Box` (asymmetric so rotation reads clearly), exercising SLERP.
- `anim_color.x3d` — `TimeSensor → ColorInterpolator → Material.diffuseColor`;
  a `Sphere` cycling hue around the wheel and back (seamless loop), exercising the
  HSV-arc path.

Keyframes are authored so `key[0]` and `key[last]` coincide → the WebM loops
cleanly.

### 3. `mise run demos` task (mise.toml)

1. Ensure `build-cpuraster/` is built (or depend on the `cpuraster` task).
2. For each demo scene: render frames into a temp dir via
   `x3d_cpu_raster --animate --fps 30 --duration 4 -w 640 -H 360 --frames-dir <tmp> <scene>`.
3. Encode WebM:
   `ffmpeg -y -framerate 30 -i frame_%04d.png -c:v libvpx-vp9 -pix_fmt yuv420p -b:v 0 -crf 32 docs/videos/demos/<scene>.webm`.
4. Refresh golden PPMs for the fixed sample frames (copy/convert from the same
   render).

The task checks for ffmpeg up front and errors clearly if missing (deps note like
`make_cubemap.sh`). This task is **manual / docs-regen only** — not part of
`mise run ci`.

### 4. Golden-frame regression — `examples/cpu_raster/tests/demo_animation_test.cpp`

For each demo scene, render the frames at a few fixed sample times (e.g.
`t ∈ {0.0, 1.333, 2.667}`), and byte-compare the resulting `Framebuffer` RGBA
against committed **PPM** goldens in
`examples/cpu_raster/tests/golden/demos/<scene>_t<NN>.ppm`. PPM (P6) is chosen
over PNG for goldens to avoid any encoder ambiguity — exact byte comparison of
deterministic CPU output. A bonus smoke assertion: `--animate` produces exactly
`N` frames and they are non-empty. The test is registered in the existing
`x3d_cpuraster` ctest suite and needs **no ffmpeg**.

## Artifacts & placement

| Artifact | Path | Committed? | Notes |
|---|---|---|---|
| WebMs | `docs/videos/demos/{position,orientation,color}.webm` | yes | 640×360, 4s, 30fps, VP9 CRF~32; target a few hundred KB each |
| Goldens | `examples/cpu_raster/tests/golden/demos/<scene>_t<NN>.ppm` | yes | ~3 frames/scene, byte-exact |
| Demo scenes | `examples/cpu_raster/assets/demos/*.x3d` | yes | source of truth |

Actual WebM sizes will be reported during implementation; resolution/CRF can be
dialed down if they are too heavy (the repo already weighs the cubemap tradeoff).

## Docs (part of the diff)

- `examples/cpu_raster/README.md` — new "Animation demos" section: embed the three
  WebMs, document `--animate`, and give the `mise run demos` regen recipe.
- Root `README.md` — a link from the gallery area to the animation demos.
- Run `mise run docs-drift working` on the change and update whatever it flags
  (e.g. a wiki note); ensure `mise run docs-build` (strict) still passes if any
  wiki page is added.

## Testing

- **Golden** — `demo_animation_test` byte-compares fixed-`t` frames to committed
  PPMs (deterministic).
- **Smoke** — `--animate` emits exactly `N` non-empty frames.
- Both ride `mise run cpuraster` (ctest filter `x3d_cpuraster`), behind
  `X3D_CPP_BUILD_CPURASTER`.

## Error handling

- `--animate` validates `fps`/`duration`/frames-dir; clear messages.
- `mise run demos` pre-checks ffmpeg; clear "install ffmpeg" error otherwise.

## Out of scope (deferred)

- **Physics demos** — Jolt is non-deterministic across hosts (microarchitecture +
  job-thread count), so it cannot share the byte-exact golden approach. A
  follow-up PR adds a showcase-only physics WebM with a loose smoke check.
- **CoordinateInterpolator** (per-frame geometry morph) — not in this set.
- A combined "move+spin+color" scene — possible later; YAGNI for now.
- `delta()`-based incremental re-extract — full snapshot per frame is enough here.

## Definition of Done

- `--animate` mode renders N frames; non-animate path unchanged.
- Three demo scenes + three committed WebMs (≤ target size) + golden PPMs.
- `demo_animation_test` passes in the `cpuraster` suite without ffmpeg.
- `mise run demos` regenerates WebMs + goldens reproducibly.
- Docs updated (cpu_raster README + root README link); `docs-drift` reviewed,
  `docs-build` strict passes.
