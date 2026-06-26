# Reference Browser Consumer — Interaction Increment (design)

**Status:** Accepted — 2026-06-21
**Scope:** First increment of evolving the existing PoC OpenGL renderer into the project's
reference browser consumer (the development north star until CAVE access).

## Context

The SDK's strategic forcing function is the VR CAVE consumer (ADR-0018): a 1-master /
N-wall topology where the master owns the `X3DExecutionContext` and walls render a pulled
`RenderDelta`. Until CAVE hardware access exists, we need a **single-process reference
consumer** that exercises the live runtime so every new feature has a place to light up.

A feature-complete OpenGL consumer **already exists**: `examples/poc_renderer/` (built via
`mise run poc`, `X3D_CPP_BUILD_POC=ON`). Verified against code, it already provides:

- **GLFW 3.4 + glad (core 3.3), Wayland-native** with an X11 fallback (FetchContent for
  GLFW; glad sources committed under `third_party/glad/`).
- The full runtime drive loop: `parseFile` → `ctx.buildSceneGraph` → `ctx.buildFrom`
  (routes) → `attachFullRuntime` (interpolators, followers, time, view-dependent,
  viewpoint-bind, key-device, optional physics/script) → `ctx.tick(now)` →
  `SceneExtractor.fullSnapshot()` then `.delta()` per frame.
- Rendering through M4+: lit (Lambert+ambient) / unlit programs, `MaterialDesc`
  (Phong/emissive/specular, per-vertex color), world-resolved `LightDesc`, camera from the
  bound Viewpoint, back-face culling honoring `ccw`/`solid`, line/point topology (B4),
  transparency back-to-front (B7), textures via a URL resolver + stb_image and inline
  PixelTexture (B8/M4), and Text glyph meshes.
- A delta-driven GPU mesh cache (refcounted by geometry identity; USE'd shapes instanced)
  and a `--headless` acceptance probe.

So the consumer is in real terms already a browser **renderer** — it just carries the name
"PoC", lives behind a flag, and is **passive**.

### The gap this increment closes

The PoC feeds **zero input**: no glfw key/mouse callbacks, no `setPointer`/`setKey`. And
`attachFullRuntime` **deliberately omits** both interaction systems:

- `NavigationSystem` — omitted because the *sim* drives the viewer directly via
  `ctx.setHeadPose()`; attaching it there would double-drive the camera.
- `PointingSensorSystem` — "deferred to the input-injection mode".

Both systems are **built and tested** in the SDK core. The only missing pieces are: (1) a
consumer-side input path that translates devices into the existing SDK input seam, (2)
attaching those two systems for the interactive (non-sim) consumer, and (3) a small
nav-vs-sensor pointer arbitration referee.

## Decision

**Approach A — the consumer owns *input*; the SDK systems own *behavior*.** The consumer
translates glfw devices into `setPointer*`/`setKey`, unprojecting the cursor into a
world-space `Ray`. The SDK's `PickSystem` does scene picking, `PointingSensorSystem` fires
sensors through the event cascade, and `NavigationSystem` moves the bound Viewpoint. The
consumer feeds raw input and renders the pulled result.

Rejected alternatives: **B** (consumer-side picking + its own camera controller) and **C**
(hybrid) both duplicate the SDK's `PickSystem`/`NavigationSystem`, diverge from the CAVE
master-owns-truth model, and — fatally for a north star — would mean the reference consumer
does **not** exercise the runtime being built. Approach A makes the consumer a faithful,
minus-transport preview of the eventual CAVE wall: feed input → SDK owns truth → pull render
state.

**This increment is interaction only.** We stay in place in `examples/poc_renderer/`; all
existing rendering is untouched. Renaming/promoting to a maintained "reference consumer"
directory is a *separate, later* increment (YAGNI here).

## Architecture & components

Four units, each with one responsibility:

1. **`InputBridge` (new, consumer-side: `examples/poc_renderer/input.{hpp,cpp}`).** Pure
   device→seam translation; no scene logic. Owns:
   - glfw key callback → `ctx.setKey(x3dCode, down)` via a GLFW→X3D keycode map; character/
     action/modifier keys additionally forward to the KeyDeviceSensor push methods so
     KeySensor/StringSensor scenes work.
   - glfw mouse-button callback → `ctx.setPointerButton(down)`.
   - cursor-position + window focus/enter → `ctx.setPointerPresent(present)` and
     `ctx.setPointer(ray)`, where `ray = unproject(cursorXY, lastView, lastProj, fbSize)`.
   - It never picks, navigates, or mutates the scene.

2. **Interactive wiring helper `attachInteractive(scene, ctx)`.** After the existing
   `attachFullRuntime`, attaches `PointingSensorSystem` then `NavigationSystem` (order is
   load-bearing — see Arbitration). Named + testable rather than loose in `main`.

3. **Arbitration flag (small SDK-core addition).** A per-tick "pointer consumed by a
   sensor" signal so navigation yields the pointer to an active sensor grab.

4. **`main.cpp` loop edit (minimal).** Register glfw callbacks (wired to `InputBridge`),
   feed input before `ctx.tick(now)`, keep reading `extractor.camera()` for the view matrix
   (now moved by `NavigationSystem`), stash it as `lastView/lastProj`. Delta apply + draw
   unchanged.

The split: the SDK owns picking, sensor behavior, and camera motion; the consumer owns only
device→seam translation and rendering — the clean Approach-A boundary, identical in shape to
a CAVE wall minus the network.

## Per-frame data flow

```
poll glfw events → callbacks into InputBridge:
    key down/up   → ctx.setKey(x3dCode, down)            (+ KeyDeviceSensor pushes)
    mouse button  → ctx.setPointerButton(down)
    cursor/focus  → ctx.setPointerPresent(p);
                    ctx.setPointer(unproject(cursorXY, lastView, lastProj, fbSize))
ctx.tick(now)     → PointingSensorSystem + NavigationSystem consume PointerState/KeyState;
                    sensors fire through the cascade; NavigationSystem moves bound Viewpoint
extractor.delta() → (unchanged) apply added/removed/updated* to GPU cache
cam = extractor.camera() → view reflects navigation; stash lastView/lastProj
draw              → (unchanged)
```

**The ray uses last frame's camera.** Unprojection needs a view/projection, but the camera
may move this tick; we unproject against the previous frame's view/proj — exactly the image
the user was looking at when they clicked. Standard one-frame convention, correct for
picking.

## Input mapping

- **Pointer:** both systems read the same `PointerState`. The consumer feeds world ray +
  button + present each frame; `NavigationSystem` derives its own drag deltas from ray
  motion (it already tracks `lastPx_/lastPy_`); `PointingSensorSystem` calls `ctx.pick(ray)`.
  No screen-delta math in the consumer.
- **Keyboard:** GLFW→X3D keycode table for nav keys → `setKey`; character/action/modifier
  keys also forwarded to KeyDeviceSensor pushes (same callback).
- **Navigation mode:** honors the bound `NavigationInfo.type` (default **EXAMINE** when
  absent — orbit-drag + dolly, a good "poke at a scene" default). A dev affordance: a key
  cycles the *effective* mode (EXAMINE↔FLY↔LOOKAT) at runtime so WASD-fly is always reachable
  regardless of how the scene was authored. This needs a `NavigationSystem::setForcedMode`
  override.
- **Deferred (not this increment):** scroll-wheel dolly (no scroll seam today), gamepad,
  touch.

## Arbitration contract

Spec basis: a pointing-device sensor grab takes *exclusive* ownership of all pointer motion
until button-up (ISO/IEC 19775-1 §20.2.1). So while a sensor is grabbed, navigation must not
also consume the pointer drag. Mechanism — **system order + a per-tick flag**, inside the
SDK so every consumer is correct:

1. `attachInteractive` adds `PointingSensorSystem` before `NavigationSystem` (systems run in
   add order).
2. `PointingSensorSystem::update` sets `ctx`'s `pointerConsumedBySensor` flag when a grab is
   active or starting this tick.
3. `NavigationSystem::update` skips **pointer-drag** rotation when the flag is set; keyboard
   navigation is unaffected (flying while holding a drag-sensor is acceptable).
4. The flag resets at tick start, like other per-frame input state.

Button-down *on a sensor* → grab, nav yields; button-down in empty space → nav rotates.
Spec-faithful, with no consumer-side picking.

## SDK additions (exact surface)

All additive; all unit-tested:

- `X3DExecutionContext`: `pointerConsumedBySensor()` getter + setter; reset at tick start.
- `PointingSensorSystem::update`: set the flag on active/starting grab.
- `NavigationSystem::update`: honor the flag (skip pointer-drag when set).
- `NavigationSystem::setForcedMode(std::optional<Mode>)`: override `NavigationInfo.type` for
  the dev mode-cycle key (`std::nullopt` = scene-driven).

This is the entire core change — three small touches plus one override. Everything else is
consumer-side.

## Error handling / edge cases

- Cursor leaves the window → `setPointerPresent(false)`: picking and nav-drag stop; an
  in-progress grab continues to follow until button-up (existing `PointingSensorSystem`
  behavior — faithful to §20.2.1).
- No bound Viewpoint → `camera()` identity fallback (existing). No geometry under the
  pointer → pick miss → navigation gets the drag.
- `unproject()` is a **pure free function** (camera + cursor + framebuffer size → `Ray`),
  not buried in a glfw callback, so it is unit-testable and the headless harness bypasses
  glfw entirely.

## Testing

- **Unit:** `unproject()` (known camera+cursor → expected ray); GLFW→X3D keycode map; the
  arbitration flag (active grab ⇒ `NavigationSystem` skips pointer-drag); `setForcedMode`.
- **Acceptance gate (new ctest — the anti-rot gate), no GL:** a headless interaction harness
  reusing the `--headless` probe pattern. Load a fixture scene with a `TouchSensor` ROUTEd
  to an observable target; inject synthetic input via `ctx.setPointer(rayOverSensor)` +
  `setPointerButton(down→up)` across ticks; assert the sensor fired (`touchTime`/`isActive`
  through the cascade). Plus a navigation assertion: feed WASD, assert
  `camera().viewMatrix` changed. Deterministic; proves interaction works and cannot silently
  rot.

## Scope boundaries (deferred)

- **Promotion/rename** of `examples/poc_renderer/` to a maintained reference-consumer
  directory, with docs and ergonomic `mise run browse <scene>` — a later increment.
- **Scroll/gamepad/touch** input.
- **CAVE delta serialization** (ADR-0018) — explicitly post-CAVE-access.
- Rendering-fidelity gaps (Background sky/ground, fog, shadows) — a separate increment.

## References

- ADR-0018 (`docs/wiki/decisions/0018-cave-cross-process-delta-contract.md`) — the CAVE
  topology this consumer previews minus transport.
- ADR-0015 (`docs/wiki/decisions/0015-extraction-pull-per-path.md`) — the pull seam.
- `docs/wiki/subsystems/extract.md` — `SceneExtractor`, `RenderItem`, `RenderDelta`.
- `docs/wiki/subsystems/system-pointing.md` — `PointingSensorSystem`, `PointerState`,
  `PickSystem`, grab lifecycle.
- `docs/wiki/subsystems/system-navigation.md` — `NavigationSystem` modes + pointer/key
  consumption.
- `tools/x3d-cli/sim_runtime.hpp` — `attachFullRuntime`; the wiring `attachInteractive`
  composes with.
- `examples/poc_renderer/main.cpp` — the existing renderer + `--headless` probe.
- ISO/IEC 19775-1 §20.2.1 / §20.2.3 — pointing-device sensor activation, grab exclusivity,
  nearest-geometry / lowest-enabled-sensor resolution.
