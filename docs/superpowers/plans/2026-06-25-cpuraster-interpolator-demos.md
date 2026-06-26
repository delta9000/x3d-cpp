# CPU-raster Interpolator Demos → WebM Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a multi-frame `--animate` render mode to the headless CPU rasterizer plus three interpolator demo scenes (Position / Orientation / Color), rendered to committed short WebMs and byte-exact golden frames.

**Architecture:** `--animate` steps `X3DExecutionContext::tick(t)` over a time range, re-extracts, and writes a numbered PPM frame per step via the existing `renderScene`. A `mise run demos` task encodes those frames to VP9 WebM with `ffmpeg` (external dev tool, as in `make_cubemap.sh`). A ctest renders the same demo scenes at fixed sample times and byte-compares to committed golden PPMs — needing no ffmpeg, so CI stays bare-runner.

**Tech Stack:** C++20, CMake/Ninja, the `x3d_cpp` runtime + extraction seam, the out-of-SDK `examples/cpu_raster/` consumer, `ffmpeg` (dev-only), `mise` tasks.

## Global Constraints

- Out-of-SDK consumer code only; everything lives under `examples/cpu_raster/` (+ committed artifacts under `docs/`). No runtime/SDK changes.
- No new linked dependency: ffmpeg stays an **external CLI** invoked from a mise task; never linked into the binary. The bare-runner property (no GPU/display/system libs) must hold for the binary and its ctests.
- Frame + golden output uses **PPM (P6)** via `Framebuffer::writePPM` — dependency-free and byte-deterministic. WebMs use VP9 (`libvpx-vp9`).
- The non-`--animate` code path must remain byte-for-byte unchanged.
- Behavior wiring uses the runtime's existing `x3d::runtime::attachStandardRuntime(scene, ctx)` (declared in `runtime/events/X3DSceneBridge.hpp`, already on the example's include path) — attaches TimeSensor + all interpolators, no Script/Physics.
- Demo scenes: `loop="true"`, `cycleInterval="4"`; first and last keyframes coincide so the loop is seamless.
- ROUTE input fields use the `set_` alias form (`set_translation`/`set_rotation`/`set_diffuseColor`); the runtime strips the prefix (`X3DEventGraph.hpp:29`).
- Golden/regression renders: **256×144**. WebM renders: **640×360, 30fps, 4s** (120 frames).
- Golden sample frame indices: **30 and 90** (t = 1.0s, 3.0s at 30fps).

---

## File Structure

- `examples/cpu_raster/assets/demos/anim_position.x3d` — orbiting textured sphere (PositionInterpolator).
- `examples/cpu_raster/assets/demos/anim_orientation.x3d` — spinning checker box (OrientationInterpolator/SLERP).
- `examples/cpu_raster/assets/demos/anim_color.x3d` — hue-cycling sphere (ColorInterpolator/HSV).
- `examples/cpu_raster/tests/demo_animation_test.cpp` — animates assertion + golden-frame byte compare (+ bless mode).
- `examples/cpu_raster/tests/golden/demos/<scene>_f00NN.ppm` — committed golden frames.
- `examples/cpu_raster/main.cpp` — add `--animate` mode (modify).
- `examples/cpu_raster/CMakeLists.txt` — register the new test + add `X3D_CPURASTER_TEST_DIR` define (modify).
- `mise.toml` — add `demos` (encode WebMs) and `demos-bless` (regen goldens) tasks (modify).
- `docs/videos/demos/{position,orientation,color}.webm` — committed showcase WebMs.
- `examples/cpu_raster/README.md`, `README.md` — docs (modify).

---

## Task 1: Demo scenes + "it animates" test

Establishes the three scenes and proves they animate (TimeSensor → interpolator → field ROUTE actually moves pixels), with the test wired into ctest.

**Files:**
- Create: `examples/cpu_raster/assets/demos/anim_position.x3d`
- Create: `examples/cpu_raster/assets/demos/anim_orientation.x3d`
- Create: `examples/cpu_raster/assets/demos/anim_color.x3d`
- Create: `examples/cpu_raster/tests/demo_animation_test.cpp`
- Modify: `examples/cpu_raster/CMakeLists.txt` (add `demo_animation_test` to `CPURASTER_TESTS`; add `X3D_CPURASTER_TEST_DIR` define)

**Interfaces:**
- Consumes: `x3d::runtime::attachStandardRuntime(Scene&, X3DExecutionContext&)`; `cr::renderScene(ctx, extractor, opt)`; `cr::Framebuffer::colorAt/width/height`.
- Produces: a helper `renderAt(scene, ctx, extractor, opt, t)` pattern (local to the test) and three demo scenes used by Tasks 2–4.

- [ ] **Step 1: Write the three demo scenes**

`examples/cpu_raster/assets/demos/anim_position.x3d`:
```xml
<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0"><head/><Scene>
  <Viewpoint position="0 0 12" fieldOfView="0.8"/>
  <Background skyColor="0.10 0.12 0.18"/>
  <DirectionalLight direction="-0.3 -0.5 -0.8" intensity="1"/>
  <Transform DEF="Mover">
    <Shape><Appearance><Material diffuseColor="1 1 1"/><ImageTexture url='"proc:uvgrid"'/></Appearance><Sphere radius="1.2"/></Shape>
  </Transform>
  <TimeSensor DEF="Clock" cycleInterval="4" loop="true"/>
  <PositionInterpolator DEF="Path" key="0 0.25 0.5 0.75 1"
      keyValue="4 0 0  0 4 0  -4 0 0  0 -4 0  4 0 0"/>
  <ROUTE fromNode="Clock" fromField="fraction_changed" toNode="Path" toField="set_fraction"/>
  <ROUTE fromNode="Path" fromField="value_changed" toNode="Mover" toField="set_translation"/>
</Scene></X3D>
```

`examples/cpu_raster/assets/demos/anim_orientation.x3d`:
```xml
<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0"><head/><Scene>
  <Viewpoint position="0 0 7" fieldOfView="0.8"/>
  <Background skyColor="0.10 0.12 0.18"/>
  <DirectionalLight direction="-0.3 -0.5 -0.8" intensity="1"/>
  <Transform DEF="Spinner">
    <Shape><Appearance><Material diffuseColor="1 1 1"/><ImageTexture url='"proc:checker"'/></Appearance><Box size="3 3 3"/></Shape>
  </Transform>
  <TimeSensor DEF="Clock" cycleInterval="4" loop="true"/>
  <OrientationInterpolator DEF="Spin" key="0 0.25 0.5 0.75 1"
      keyValue="0 1 0 0  0 1 0 1.5708  0 1 0 3.1416  0 1 0 4.7124  0 1 0 6.2832"/>
  <ROUTE fromNode="Clock" fromField="fraction_changed" toNode="Spin" toField="set_fraction"/>
  <ROUTE fromNode="Spin" fromField="value_changed" toNode="Spinner" toField="set_rotation"/>
</Scene></X3D>
```

`examples/cpu_raster/assets/demos/anim_color.x3d`:
```xml
<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0"><head/><Scene>
  <Viewpoint position="0 0 6" fieldOfView="0.8"/>
  <Background skyColor="0.10 0.12 0.18"/>
  <DirectionalLight direction="-0.3 -0.5 -0.8" intensity="1"/>
  <Shape><Appearance><Material DEF="Mat" diffuseColor="1 0 0"/></Appearance><Sphere radius="2"/></Shape>
  <TimeSensor DEF="Clock" cycleInterval="4" loop="true"/>
  <ColorInterpolator DEF="Hue" key="0 0.333 0.667 1"
      keyValue="1 0 0  0 1 0  0 0 1  1 0 0"/>
  <ROUTE fromNode="Clock" fromField="fraction_changed" toNode="Hue" toField="set_fraction"/>
  <ROUTE fromNode="Hue" fromField="value_changed" toNode="Mat" toField="set_diffuseColor"/>
</Scene></X3D>
```

- [ ] **Step 2: Write the failing test (animates assertion)**

Create `examples/cpu_raster/tests/demo_animation_test.cpp`. (Golden compare is added in Task 3; this version only asserts the scenes animate.)
```cpp
// demo_animation_test.cpp — the interpolator demo scenes actually animate over
// time (TimeSensor -> interpolator -> field ROUTE moves pixels), and (Task 3)
// fixed sample frames match committed golden PPMs byte-for-byte.
#include "RenderItem.hpp"
#include "SceneExtractor.hpp"
#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DParse.hpp"
#include "X3DSceneBridge.hpp"

#include "cpuraster/Framebuffer.hpp"
#include "cpuraster/SceneRender.hpp"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace ex = x3d::runtime::extract;
namespace cr = x3d::cpuraster;
namespace g = x3d::cpuraster::glsl;

static int failures = 0;
#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::fprintf(stderr, "FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);     \
      ++failures;                                                              \
    }                                                                          \
  } while (0)

namespace {

constexpr int kW = 256, kH = 144, kFps = 30;

// Render demo `name` at the frame that lands on time `t = frameIndex/kFps`,
// stepping the sim from 0 to that frame exactly as --animate does (interpolators
// are memoryless in `now`, but we step to stay byte-identical to the binary).
cr::Framebuffer renderFrame(const std::string &name, int frameIndex) {
  const std::string scenePath =
      std::string(X3D_CPURASTER_ASSET_DIR) + "/demos/" + name + ".x3d";
  x3d::runtime::X3DDocument doc = x3d::codec::parseFile(scenePath);
  x3d::runtime::Scene &scene = doc.getScene();
  x3d::runtime::X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  ctx.buildFrom(scene);
  x3d::runtime::attachStandardRuntime(scene, ctx);

  ex::SceneExtractor extractor(ctx, scene);
  cr::RenderOptions opt;
  opt.width = kW;
  opt.height = kH;

  cr::Framebuffer fb(kW, kH);
  for (int f = 0; f <= frameIndex; ++f) {
    ctx.tick(static_cast<double>(f) / kFps);
    extractor.fullSnapshot();
    if (f == frameIndex) fb = cr::renderScene(ctx, extractor, opt);
  }
  return fb;
}

int frameDiff(const cr::Framebuffer &a, const cr::Framebuffer &b) {
  int n = 0;
  for (int y = 0; y < a.height(); ++y)
    for (int x = 0; x < a.width(); ++x) {
      g::vec4 ca = a.colorAt(x, y), cb = b.colorAt(x, y);
      if (std::fabs(ca.x - cb.x) + std::fabs(ca.y - cb.y) +
              std::fabs(ca.z - cb.z) > 0.05f)
        ++n;
    }
  return n;
}

} // namespace

int main() {
  const char *scenes[] = {"anim_position", "anim_orientation", "anim_color"};
  for (const char *s : scenes) {
    cr::Framebuffer early = renderFrame(s, 15); // t=0.5
    cr::Framebuffer late = renderFrame(s, 75);  // t=2.5
    int diff = frameDiff(early, late);
    std::fprintf(stderr, "%s: %d px changed between t=0.5 and t=2.5\n", s, diff);
    CHECK(diff > 50); // the interpolator visibly animated something.
  }
  if (failures) {
    std::fprintf(stderr, "demo_animation_test: %d failure(s)\n", failures);
    return 1;
  }
  std::printf("demo_animation_test: OK\n");
  return 0;
}
```

- [ ] **Step 3: Wire the test into CMake**

In `examples/cpu_raster/CMakeLists.txt`, add `demo_animation_test` to the `CPURASTER_TESTS` list (after `skybox_test`):
```cmake
set(CPURASTER_TESTS
    glsl_test
    rasterizer_test
    material_shader_test
    glsl_interp_test
    render_smoke_test
    text_render_test
    texture_render_test
    positional_light_test
    tier1_features_test
    skybox_test
    demo_animation_test)
```
And add the test-dir define inside the `foreach` block's `target_compile_definitions` (so Task 3's golden paths resolve), changing it to:
```cmake
    target_compile_definitions(x3d_cpuraster_${t} PRIVATE
        "X3D_CPURASTER_ASSET_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/assets\""
        "X3D_CPURASTER_SHADER_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/shaders\""
        "X3D_CPURASTER_TEST_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/tests\"")
```

- [ ] **Step 4: Build and run the test — verify it passes**

Run:
```bash
cmake -S . -B build-cpuraster -G Ninja -DX3D_CPP_BUILD_CPURASTER=ON -DX3D_CPP_BUILD_STB=ON
cmake --build build-cpuraster
ctest --test-dir build-cpuraster -R x3d_cpuraster_demo_animation_test --output-on-failure
```
Expected: `demo_animation_test: OK`, and stderr shows each scene with `>50` changed pixels. If any scene shows `0` changed pixels, the ROUTE was rejected — check `buildFrom` rejected-route count and confirm the `set_` field names.

- [ ] **Step 5: Commit**
```bash
git add examples/cpu_raster/assets/demos examples/cpu_raster/tests/demo_animation_test.cpp examples/cpu_raster/CMakeLists.txt
git commit -m "feat(cpuraster): interpolator demo scenes + animates test"
```

---

## Task 2: `--animate` frame-stepping mode in the CLI

Adds the multi-frame render mode that writes a numbered PPM per timestep — the producer for the WebMs.

**Files:**
- Modify: `examples/cpu_raster/main.cpp`

**Interfaces:**
- Consumes: `x3d::runtime::attachStandardRuntime`; the already-configured `cr::RenderOptions opt` (size, glyph atlas, skybox, author shader); `cr::Framebuffer::writePPM`.
- Produces: CLI flags `--animate`, `--fps <n>` (default 30), `--duration <s>` (default 4.0), `--frames-dir <dir>`; writes `<frames-dir>/frame_%04d.ppm`.

- [ ] **Step 1: Add flag parsing and state**

In `main.cpp`, add to the declarations block (near `bool headless = false;`):
```cpp
  bool animate = false;
  int fps = 30;
  double duration = 4.0;
  std::string framesDir;
```
And in the argument loop (after the `--headless` branch):
```cpp
    else if (a == "--animate") animate = true;
    else if (a == "--fps" && i + 1 < argc) fps = std::atoi(argv[++i]);
    else if (a == "--duration" && i + 1 < argc) duration = std::atof(argv[++i]);
    else if (a == "--frames-dir" && i + 1 < argc) framesDir = argv[++i];
```

- [ ] **Step 2: Attach behavior systems for animate mode**

In `main.cpp`, immediately after `x3d::runtime::BridgeResult bridge = ctx.buildFrom(scene);` and its `if (!bridge.ok())` log, BEFORE `ctx.tick(0.0);`, add:
```cpp
  if (animate) x3d::runtime::attachStandardRuntime(scene, ctx);
```
(`attachStandardRuntime` is declared in the already-included `X3DSceneBridge.hpp`.)

- [ ] **Step 3: Add the frame loop**

In `main.cpp`, locate the line `cr::Framebuffer fb = cr::renderScene(ctx, extractor, opt);` (the single-frame render, after the skybox + `--frag` setup). Insert the animate block immediately BEFORE it:
```cpp
  if (animate) {
    if (fps <= 0 || duration <= 0.0 || framesDir.empty()) {
      std::fprintf(stderr, "[cpu-raster] --animate needs --fps>0, --duration>0, "
                           "and --frames-dir <dir>\n");
      return 1;
    }
    const int nFrames = static_cast<int>(std::lround(duration * fps));
    for (int f = 0; f < nFrames; ++f) {
      ctx.tick(static_cast<double>(f) / fps);
      extractor.fullSnapshot();
      cr::Framebuffer frame = cr::renderScene(ctx, extractor, opt);
      char suffix[32];
      std::snprintf(suffix, sizeof suffix, "/frame_%04d.ppm", f);
      const std::string p = framesDir + suffix;
      if (!frame.writePPM(p)) {
        std::fprintf(stderr, "[cpu-raster] failed to write %s\n", p.c_str());
        return 1;
      }
    }
    std::fprintf(stderr, "[cpu-raster] wrote %d frame(s) -> %s\n", nFrames,
                 framesDir.c_str());
    return 0;
  }
```
Add `#include <cmath>` to the include block (for `std::lround`) if not already present.

- [ ] **Step 4: Build and run on a demo scene — verify frames are written**

Run:
```bash
cmake --build build-cpuraster
mkdir -p /tmp/anim_frames
./build-cpuraster/examples/cpu_raster/x3d_cpu_raster \
    examples/cpu_raster/assets/demos/anim_position.x3d \
    --animate --fps 30 --duration 4 -w 640 -H 360 --frames-dir /tmp/anim_frames
ls /tmp/anim_frames | wc -l
```
Expected: stderr `wrote 120 frame(s)`; `ls | wc -l` prints `120`. Spot-check two frames differ:
```bash
cmp /tmp/anim_frames/frame_0000.ppm /tmp/anim_frames/frame_0045.ppm && echo SAME || echo DIFFER
```
Expected: `DIFFER`.

- [ ] **Step 5: Verify the non-animate path is unchanged**

Run:
```bash
./build-cpuraster/examples/cpu_raster/x3d_cpu_raster -o /tmp/still.ppm \
    examples/cpu_raster/assets/demos/anim_color.x3d
```
Expected: stderr `rendered N item(s) -> /tmp/still.ppm` (single still frame, exit 0).

- [ ] **Step 6: Commit**
```bash
git add examples/cpu_raster/main.cpp
git commit -m "feat(cpuraster): --animate frame-stepping render mode"
```

---

## Task 3: Golden-frame regression + bless mode

Extends the test to byte-compare fixed sample frames against committed golden PPMs, with an env-gated bless mode to (re)generate them.

**Files:**
- Modify: `examples/cpu_raster/tests/demo_animation_test.cpp`
- Create: `examples/cpu_raster/tests/golden/demos/<scene>_f0030.ppm`, `<scene>_f0090.ppm` (6 files, via bless)

**Interfaces:**
- Consumes: `renderFrame(name, frameIndex)` from Task 1; `X3D_CPURASTER_TEST_DIR` (CMake define from Task 1); `Framebuffer::writePPM`.
- Produces: golden compare over indices {30, 90}; bless when `X3D_CPURASTER_BLESS` env is set.

- [ ] **Step 1: Add golden compare + bless to the test**

In `demo_animation_test.cpp`, add includes `#include <cstdlib>` and `#include <fstream>`. Add this helper inside the anonymous namespace (after `frameDiff`):
```cpp
std::string goldenPath(const std::string &name, int frameIndex) {
  char suffix[32];
  std::snprintf(suffix, sizeof suffix, "_f%04d.ppm", frameIndex);
  return std::string(X3D_CPURASTER_TEST_DIR) + "/golden/demos/" + name + suffix;
}

bool filesEqual(const std::string &a, const std::string &b) {
  std::ifstream fa(a, std::ios::binary), fb(b, std::ios::binary);
  if (!fa || !fb) return false;
  std::string sa((std::istreambuf_iterator<char>(fa)), {});
  std::string sb((std::istreambuf_iterator<char>(fb)), {});
  return sa == sb;
}
```
Then, in `main()`, after the existing animates loop (before the `if (failures)` block), add:
```cpp
  const bool bless = std::getenv("X3D_CPURASTER_BLESS") != nullptr;
  const int goldenFrames[] = {30, 90};
  for (const char *s : scenes) {
    for (int idx : goldenFrames) {
      cr::Framebuffer fb = renderFrame(s, idx);
      const std::string gp = goldenPath(s, idx);
      if (bless) {
        CHECK(fb.writePPM(gp));
        std::fprintf(stderr, "blessed %s\n", gp.c_str());
      } else {
        const std::string actual = std::string(s) + "_f" +
            (idx < 100 ? "00" : "0") + std::to_string(idx) + ".actual.ppm";
        CHECK(fb.writePPM(actual));
        if (!filesEqual(actual, gp)) {
          std::fprintf(stderr, "FAIL golden mismatch: %s vs %s\n",
                       actual.c_str(), gp.c_str());
          ++failures;
        }
      }
    }
  }
```

- [ ] **Step 2: Run the test to verify it fails (no goldens yet)**

Run:
```bash
cmake --build build-cpuraster
ctest --test-dir build-cpuraster -R x3d_cpuraster_demo_animation_test --output-on-failure
```
Expected: FAIL with `golden mismatch` lines (golden files do not exist yet).

- [ ] **Step 3: Bless the goldens**

Run:
```bash
mkdir -p examples/cpu_raster/tests/golden/demos
X3D_CPURASTER_BLESS=1 ./build-cpuraster/examples/cpu_raster/x3d_cpuraster_demo_animation_test
ls examples/cpu_raster/tests/golden/demos
```
Expected: stderr `blessed …` ×6; the dir lists `anim_position_f0030.ppm`, `anim_position_f0090.ppm`, `anim_orientation_f0030.ppm`, `anim_orientation_f0090.ppm`, `anim_color_f0030.ppm`, `anim_color_f0090.ppm`.

- [ ] **Step 4: Run the test again — verify it passes against committed goldens**

Run:
```bash
ctest --test-dir build-cpuraster -R x3d_cpuraster_demo_animation_test --output-on-failure
```
Expected: `demo_animation_test: OK`.

- [ ] **Step 5: Commit**
```bash
git add examples/cpu_raster/tests/demo_animation_test.cpp examples/cpu_raster/tests/golden/demos
git commit -m "test(cpuraster): byte-exact golden frames for interpolator demos"
```

---

## Task 4: `mise run demos` + committed WebMs

Adds the ffmpeg-driven encode task (and a golden-bless task) and produces the committed showcase WebMs.

**Files:**
- Modify: `mise.toml`
- Create: `docs/videos/demos/position.webm`, `orientation.webm`, `color.webm`

**Interfaces:**
- Consumes: the `x3d_cpu_raster` binary `--animate` mode (Task 2); the `x3d_cpuraster_demo_animation_test` binary bless mode (Task 3).
- Produces: `mise run demos` (frames → WebM), `mise run demos-bless` (regen goldens).

- [ ] **Step 1: Add the mise tasks**

In `mise.toml`, after the `[tasks.cpuraster]` block, add:
```toml
[tasks.demos]
description = "Render the interpolator demo scenes to WebM (docs/videos/demos/). Needs ffmpeg (libvpx-vp9). Depends on the cpuraster build."
run = """
set -euo pipefail
cmake -S . -B build-cpuraster -G Ninja -DX3D_CPP_BUILD_CPURASTER=ON -DX3D_CPP_BUILD_STB=ON
cmake --build build-cpuraster
command -v ffmpeg >/dev/null || { echo "demos: ffmpeg not found (install ffmpeg with libvpx-vp9)"; exit 1; }
BIN=build-cpuraster/examples/cpu_raster/x3d_cpu_raster
mkdir -p docs/videos/demos
for pair in position:anim_position orientation:anim_orientation color:anim_color; do
  out="${pair%%:*}"; scene="${pair##*:}"
  work="$(mktemp -d)"
  "$BIN" "examples/cpu_raster/assets/demos/${scene}.x3d" \
      --animate --fps 30 --duration 4 -w 640 -H 360 --frames-dir "$work"
  ffmpeg -y -hide_banner -loglevel error -framerate 30 -i "$work/frame_%04d.ppm" \
      -c:v libvpx-vp9 -pix_fmt yuv420p -b:v 0 -crf 32 "docs/videos/demos/${out}.webm"
  rm -rf "$work"
  echo "wrote docs/videos/demos/${out}.webm"
done
"""

[tasks.demos-bless]
description = "Regenerate the committed golden PPM frames for the interpolator demo regression test."
run = """
set -euo pipefail
cmake -S . -B build-cpuraster -G Ninja -DX3D_CPP_BUILD_CPURASTER=ON -DX3D_CPP_BUILD_STB=ON
cmake --build build-cpuraster
mkdir -p examples/cpu_raster/tests/golden/demos
X3D_CPURASTER_BLESS=1 build-cpuraster/examples/cpu_raster/x3d_cpuraster_demo_animation_test
"""
```

- [ ] **Step 2: Generate the WebMs**

Run:
```bash
mise run demos
ls -la docs/videos/demos
```
Expected: three `.webm` files written; sizes reported. If any single file exceeds ~1 MB, raise `-crf` (e.g. 36) or drop to `-w 512 -H 288` and re-run, then note the chosen values.

- [ ] **Step 3: Verify the WebMs play / are valid**

Run:
```bash
for v in position orientation color; do ffprobe -v error -show_entries \
  stream=codec_name,width,height,nb_frames -of default=nw=1 docs/videos/demos/$v.webm; done
```
Expected: `codec_name=vp9`, `width=640`, `height=360` for each (frame count ≈120).

- [ ] **Step 4: Commit**
```bash
git add mise.toml docs/videos/demos
git commit -m "feat(demos): mise run demos task + committed interpolator WebMs"
```

---

## Task 5: Docs

Documents the feature where the docs-drift discipline expects it, and links the showcase.

**Files:**
- Modify: `examples/cpu_raster/README.md`
- Modify: `README.md`

**Interfaces:**
- Consumes: nothing (documentation).
- Produces: an "Animation demos" section + the `--animate` CLI row.

- [ ] **Step 1: Add the `--animate` flags to the cpu_raster CLI table**

In `examples/cpu_raster/README.md`, in the `### CLI` table, add rows after the `--gentex` row:
```markdown
| `--animate` | render a frame per timestep instead of one still (drives the demo WebMs) |
| `--fps <n>` | frames per second in `--animate` (default 30) |
| `--duration <s>` | seconds of animation to render in `--animate` (default 4.0) |
| `--frames-dir <dir>` | output dir for `frame_%04d.ppm` (required with `--animate`) |
```

- [ ] **Step 2: Add an "Animation demos" section**

In `examples/cpu_raster/README.md`, before the `## Layout` section, add:
```markdown
## Animation demos

The `--animate` mode steps simulation time (`X3DExecutionContext::tick`) over a
range and writes one PPM frame per step, so the runtime's TimeSensor →
interpolator → ROUTE machinery produces a moving picture. Three demo scenes live
in `assets/demos/`:

- `anim_position.x3d` — a textured sphere orbiting via `PositionInterpolator`.
- `anim_orientation.x3d` — a checker `Box` spinning via `OrientationInterpolator` (SLERP).
- `anim_color.x3d` — a sphere cycling hue via `ColorInterpolator` (HSV arc).

Encode them to WebM (needs `ffmpeg` with `libvpx-vp9`):

```sh
mise run demos     # -> docs/videos/demos/{position,orientation,color}.webm
```

Regression is byte-exact and needs no ffmpeg: `demo_animation_test` renders fixed
sample frames and compares them to committed golden PPMs
(`tests/golden/demos/`). Regenerate goldens after an intentional visual change
with `mise run demos-bless`.
```

- [ ] **Step 3: Link the demos from the root README**

In `README.md`, immediately after the gallery image table (after the `![Primitive texcoords]...` block and its caption paragraph), add:
```markdown
**Animated demos** (interpolators over time, rendered headlessly to WebM):
[position](docs/videos/demos/position.webm) ·
[orientation](docs/videos/demos/orientation.webm) ·
[color](docs/videos/demos/color.webm) — see
[`examples/cpu_raster/`](examples/cpu_raster/README.md#animation-demos).
```

- [ ] **Step 4: Run docs-drift and the strict docs build**

Run:
```bash
mise run docs-drift working
mise run docs-build
```
Expected: `docs-drift` prints an advisory review list (act on anything that names code you changed); `docs-build` exits 0 (no dead links / nav orphans). If `docs-build` fails on the new `docs/videos/` path, either reference the WebMs with absolute repo links as written (they live outside the mkdocs `docs/wiki` tree) or add an exclusion per the existing mkdocs config — do not move the committed WebMs.

- [ ] **Step 5: Commit**
```bash
git add examples/cpu_raster/README.md README.md
git commit -m "docs(cpuraster): document --animate + animation demo gallery"
```

---

## Self-Review

**1. Spec coverage:**
- `--animate` multi-frame mode → Task 2. ✓
- Three interpolator scenes (Position/Orientation/Color) → Task 1. ✓
- Per-frame `fullSnapshot()` re-extract → Tasks 1 (test) & 2 (CLI). ✓
- Committed short WebMs (640×360/4s/30fps/VP9) → Task 4. ✓
- Byte-exact PPM golden frames + CI without ffmpeg → Task 3 (test rides `cpuraster` ctest suite). ✓
- `mise run demos` (ffmpeg encode) + golden refresh → Task 4 (`demos`, `demos-bless`). ✓
- Docs (cpu_raster README + root README link) + docs-drift/docs-build → Task 5. ✓
- Physics deferred / CoordinateInterpolator out → not in any task (correct). ✓

**2. Placeholder scan:** No TBD/TODO; every code step shows complete code; every run step shows the command + expected output. ✓

**3. Type/name consistency:** `renderFrame(name, frameIndex)`, `goldenPath`, `filesEqual`, `frameDiff` defined in Task 1/3 and used consistently. `X3D_CPURASTER_TEST_DIR` defined in Task 1 CMake, consumed in Task 3. Scene basenames (`anim_position`/`anim_orientation`/`anim_color`) and golden indices ({30,90}) consistent across Tasks 1, 3, 4. WebM output names (`position`/`orientation`/`color`) consistent across Tasks 4, 5. ✓

## Definition of Done

- `--animate` renders N frames; non-animate path unchanged (Task 2 Steps 4–5).
- Three demo scenes animate (Task 1) and produce three committed WebMs ≤ target size (Task 4).
- `demo_animation_test` passes in the `cpuraster` ctest suite with no ffmpeg (Task 3).
- `mise run demos` / `mise run demos-bless` reproduce WebMs / goldens.
- Docs updated; `docs-drift` reviewed and `docs-build` strict passes (Task 5).
