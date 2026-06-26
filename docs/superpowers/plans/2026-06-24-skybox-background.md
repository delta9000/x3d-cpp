# Skybox (Background 6-URL panorama cube) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Render the X3D `Background` node's six panorama faces (`frontUrl/backUrl/leftUrl/rightUrl/topUrl/bottomUrl`) as a textured cube around the viewer in the headless CPU rasterizer, composited over the existing sky/ground gradient with alpha.

**Architecture:** Extends the per-pixel Background gradient already in `SceneRender.hpp` (PR #27). Each background pixel already unprojects its view ray to a world-space direction; a skybox just (a) selects which cube face that direction hits and the face UV, (b) samples that face's texture, and (c) composites it over the gradient using the texel alpha (§Background: panorama draws in front of the gradient, alpha lets the gradient show through). Face textures are resolved by `main.cpp` through the existing `makeTextureResolver` (`proc:`/`.ppm`/`PixelTexture`) and handed to the renderer via a new `RenderOptions` hook, mirroring the `glyphAtlas` pattern. No SDK/extraction changes — the rasterizer reads the bound `Background` node's URL fields directly, exactly as it already reads `skyColor`.

**Tech Stack:** C++20, header-only `examples/cpu_raster/cpuraster/`, the `x3d_cpp::x3d_cpp` extraction seam (read-only). No new third-party dependencies.

## Global Constraints

- Out-of-SDK CONSUMER code only — touch `examples/cpu_raster/**` exclusively; no changes under `runtime/`, `generated_cpp_bindings/`, or `include/`. (`Background` + its six URL fields already exist in the X3D 4.0 bindings — verified `getBackUrl()`/`getBottomUrl()`/… in `generated_cpp_bindings/Background.hpp`.)
- No new dependencies: reuse `Texture` + `makeTextureResolver` (`proc:`/`.ppm`/`PixelTexture`); the suite must stay buildable and runnable on a bare CI runner.
- Namespace `x3d::cpuraster`; match the existing file style (the `glsl::` math helpers, the `CHECK` test-macro harness used by `tier1_features_test.cpp`).
- Build/test gate: `cmake -S . -B build-cpuraster -G Ninja -DX3D_CPP_BUILD_CPURASTER=ON -DX3D_CPP_BUILD_TESTS=ON -DX3D_CPP_PER_HEADER_CHECKS=OFF` then `ctest --test-dir build-cpuraster -R cpuraster` (or `mise run cpuraster`).
- Coordinate frame (ISO/IEC 19775-1 Background): the cube is at infinity, **rotates** with the viewer but does **not** translate. Face directions, looking from the origin: **front = −Z, back = +Z, right = +X, left = −X, top = +Y, bottom = −Y**; each image is mapped "with the same orientation as if displayed normally in 2D" when viewed from the origin. UV orientation per face is pinned by the asymmetric test fixture in Task 1.

---

### Task 1: Cube-face selection + UV helper (pure)

**Files:**
- Modify: `examples/cpu_raster/cpuraster/SceneRender.hpp` (add to `namespace render_detail`)
- Test: `examples/cpu_raster/tests/skybox_test.cpp` (new; register in `examples/cpu_raster/CMakeLists.txt` `CPURASTER_TESTS`)

**Interfaces:**
- Produces: `enum class CubeFace { Front, Back, Right, Left, Top, Bottom };` and
  `struct FaceSample { CubeFace face; glsl::vec2 uv; };`
  `inline FaceSample cubeFaceUv(const glsl::vec3 &dir);` — `dir` is a (not necessarily unit) world-space ray direction; returns the face it pierces and the in-face UV in `[0,1]²`.

- [ ] **Step 1: Register the new test target**

In `examples/cpu_raster/CMakeLists.txt`, add `skybox_test` to the `CPURASTER_TESTS` list (after `tier1_features_test`).

- [ ] **Step 2: Write the failing test**

```cpp
// skybox_test.cpp — Background panorama cube: face selection + UV, and alpha
// compositing of a face texture over the sky/ground gradient.
#include "RenderItem.hpp"
#include "cpuraster/MaterialShader.hpp"
#include "cpuraster/SceneRender.hpp"
#include <cstdio>

using namespace x3d::cpuraster;
namespace g = x3d::cpuraster::glsl;
using render_detail::CubeFace;

static int failures = 0;
#define CHECK(c) do { if(!(c)){ std::fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#c); ++failures; } } while(0)

int main() {
  // Looking down -Z from the origin hits the FRONT face at its centre.
  auto fr = render_detail::cubeFaceUv({0, 0, -1});
  CHECK(fr.face == CubeFace::Front);
  CHECK(fr.uv.x > 0.45f && fr.uv.x < 0.55f && fr.uv.y > 0.45f && fr.uv.y < 0.55f);
  // Principal axes pick the expected faces.
  CHECK(render_detail::cubeFaceUv({1, 0, 0}).face == CubeFace::Right);
  CHECK(render_detail::cubeFaceUv({-1, 0, 0}).face == CubeFace::Left);
  CHECK(render_detail::cubeFaceUv({0, 1, 0}).face == CubeFace::Top);
  CHECK(render_detail::cubeFaceUv({0, -1, 0}).face == CubeFace::Bottom);
  CHECK(render_detail::cubeFaceUv({0, 0, 1}).face == CubeFace::Back);
  // A direction biased +X off the -Z axis moves the front-face U toward the
  // right edge (the +X side of the front face is the right of the 2D image).
  CHECK(render_detail::cubeFaceUv({0.4f, 0, -1}).uv.x > 0.55f);

  if (failures) { std::fprintf(stderr, "skybox_test: %d failure(s)\n", failures); return 1; }
  std::printf("skybox_test: OK\n");
  return 0;
}
```

- [ ] **Step 3: Run it to verify it fails**

Run: `cmake --build build-cpuraster --target x3d_cpuraster_skybox_test`
Expected: FAIL to compile — `cubeFaceUv`/`CubeFace` undeclared.

- [ ] **Step 4: Implement the helper**

Add to `namespace render_detail` in `SceneRender.hpp` (near `skyGroundColor`):

```cpp
enum class CubeFace { Front, Back, Right, Left, Top, Bottom };
struct FaceSample { CubeFace face; glsl::vec2 uv; };

// Pick the cube face a world-space direction pierces and the in-face UV.
// Faces (from the origin): front=-Z, back=+Z, right=+X, left=-X, top=+Y,
// bottom=-Y. UV orientation matches "image displayed normally in 2D" viewed
// from the origin (§Background). Pinned by skybox_test.
inline FaceSample cubeFaceUv(const glsl::vec3 &dir) {
  const float ax = std::fabs(dir.x), ay = std::fabs(dir.y), az = std::fabs(dir.z);
  auto remap = [](float a, float b) { // [-1,1] -> [0,1]
    return glsl::vec2{a * 0.5f + 0.5f, b * 0.5f + 0.5f};
  };
  if (az >= ax && az >= ay) {
    const float u = dir.x / az, v = dir.y / az;
    return dir.z < 0.0f ? FaceSample{CubeFace::Front, remap(u, v)}    // -Z
                        : FaceSample{CubeFace::Back, remap(-u, v)};   // +Z
  }
  if (ax >= ay) {
    const float u = -dir.z / ax, v = dir.y / ax;
    return dir.x > 0.0f ? FaceSample{CubeFace::Right, remap(-u, v)}   // +X
                        : FaceSample{CubeFace::Left, remap(u, v)};    // -X
  }
  const float u = dir.x / ay, v = -dir.z / ay;
  return dir.y > 0.0f ? FaceSample{CubeFace::Top, remap(u, -v)}       // +Y
                      : FaceSample{CubeFace::Bottom, remap(u, v)};    // -Y
}
```

- [ ] **Step 5: Run it to verify it passes**

Run: `cmake --build build-cpuraster --target x3d_cpuraster_skybox_test && ./build-cpuraster/examples/cpu_raster/x3d_cpuraster_skybox_test`
Expected: `skybox_test: OK`. (If a face-orientation `CHECK` fails, flip the sign of the corresponding `remap` argument — the face *selection* asserts must pass first.)

- [ ] **Step 6: Commit**

```bash
git add examples/cpu_raster/cpuraster/SceneRender.hpp examples/cpu_raster/tests/skybox_test.cpp examples/cpu_raster/CMakeLists.txt
git commit -m "feat(cpu_raster): cube-face selection + UV helper for skybox"
```

---

### Task 2: Skybox struct + composite over the gradient

**Files:**
- Modify: `examples/cpu_raster/cpuraster/SceneRender.hpp` (`RenderOptions`, the background fill loop, plus a pure composite helper)
- Test: `examples/cpu_raster/tests/skybox_test.cpp` (extend)

**Interfaces:**
- Consumes: `cubeFaceUv` (Task 1), `skyGroundColor` (PR #27), `Texture::sample` / `Texture::valid`.
- Produces:
  `struct SkyboxTextures { Texture front, back, right, left, top, bottom; bool any() const; const Texture &face(CubeFace) const; };`
  `inline glsl::vec3 skyboxColor(const glsl::vec3 &dir, const SkyboxTextures &sb, const glsl::vec3 &gradient);`
  and a new `const SkyboxTextures *skybox = nullptr;` field on `RenderOptions`.

- [ ] **Step 1: Write the failing test (extend skybox_test.cpp, before the summary block)**

```cpp
  // A front-face texture (solid green, opaque) fully replaces the gradient when
  // looking down -Z; alpha < 1 lets the gradient (here red) show through.
  {
    render_detail::SkyboxTextures sb;
    std::uint8_t green[4] = {0, 255, 0, 255};
    sb.front = Texture::fromRGBA8(green, 1, 1, true, true, /*srgb=*/true);
    g::vec3 grad{1, 0, 0};
    g::vec3 opaque = render_detail::skyboxColor({0, 0, -1}, sb, grad);
    CHECK(opaque.y > 0.9f && opaque.x < 0.1f); // green wins (alpha 1)

    std::uint8_t halfgreen[4] = {0, 255, 0, 128};
    sb.front = Texture::fromRGBA8(halfgreen, 1, 1, true, true, true);
    g::vec3 blended = render_detail::skyboxColor({0, 0, -1}, sb, grad);
    CHECK(blended.x > 0.2f && blended.y > 0.2f); // red gradient shows through

    // A face with no texture falls back to the gradient unchanged.
    render_detail::SkyboxTextures empty;
    g::vec3 fall = render_detail::skyboxColor({0, 0, -1}, empty, grad);
    CHECK(fall.x > 0.9f && fall.y < 0.1f);
  }
```

- [ ] **Step 2: Run it to verify it fails**

Run: `cmake --build build-cpuraster --target x3d_cpuraster_skybox_test`
Expected: FAIL — `SkyboxTextures`/`skyboxColor` undeclared.

- [ ] **Step 3: Implement the struct + composite helper**

Add to `namespace render_detail` (after `cubeFaceUv`):

```cpp
struct SkyboxTextures {
  Texture front, back, right, left, top, bottom;
  bool any() const {
    return front.valid() || back.valid() || right.valid() || left.valid() ||
           top.valid() || bottom.valid();
  }
  const Texture &face(CubeFace f) const {
    switch (f) {
      case CubeFace::Front:  return front;
      case CubeFace::Back:   return back;
      case CubeFace::Right:  return right;
      case CubeFace::Left:   return left;
      case CubeFace::Top:    return top;
      case CubeFace::Bottom: return bottom;
    }
    return front;
  }
};

// Panorama colour for a view direction: the face texel composited over the
// sky/ground gradient by its alpha (§Background). No/empty face -> gradient.
inline glsl::vec3 skyboxColor(const glsl::vec3 &dir, const SkyboxTextures &sb,
                              const glsl::vec3 &gradient) {
  const FaceSample fs = cubeFaceUv(dir);
  const Texture &t = sb.face(fs.face);
  if (!t.valid()) return gradient;
  const glsl::vec4 texel = t.sample(fs.uv);
  const float a = glsl::clampf(texel.w, 0.0f, 1.0f);
  return gradient + (texel.xyz() - gradient) * a; // mix(gradient, texel, a)
}
```

- [ ] **Step 4: Add the `RenderOptions` hook**

In `RenderOptions` (`SceneRender.hpp`), after `glyphAtlas`:

```cpp
  // Optional skybox: six resolved panorama faces (Background *Url fields).
  // Null => no skybox; faces composite over the sky/ground gradient by alpha.
  const SkyboxTextures *skybox = nullptr;
```

(`SkyboxTextures` is declared in `render_detail`; reference it as `render_detail::SkyboxTextures` here, or hoist the declaration above `RenderOptions` if ordering requires.)

- [ ] **Step 5: Composite in the background fill loop**

In `render()`, inside the existing per-pixel Background loop, replace the
`fb.setColor(...)` line with a skybox-aware version:

```cpp
          glsl::vec3 bgcol = skyGroundColor(ang, skyC, skyA, grC, grA);
          if (opt.skybox && opt.skybox->any())
            bgcol = skyboxColor(d, *opt.skybox, bgcol);
          fb.setColor(x, y, glsl::vec4(bgcol, 1.0f));
```

Also relax the loop guard so it runs when a skybox is present even with a lone
skyColor: change `if (skyC.size() > 1 || !grC.empty())` to
`if (skyC.size() > 1 || !grC.empty() || (opt.skybox && opt.skybox->any()))`.

- [ ] **Step 6: Run tests to verify they pass**

Run: `cmake --build build-cpuraster --target x3d_cpuraster_skybox_test && ./build-cpuraster/examples/cpu_raster/x3d_cpuraster_skybox_test`
Expected: `skybox_test: OK`.

- [ ] **Step 7: Commit**

```bash
git add examples/cpu_raster/cpuraster/SceneRender.hpp examples/cpu_raster/tests/skybox_test.cpp
git commit -m "feat(cpu_raster): composite Background skybox faces over the gradient"
```

---

### Task 3: Resolve the six Background URLs in main.cpp

**Files:**
- Modify: `examples/cpu_raster/main.cpp`
- Test: `examples/cpu_raster/assets/skybox_smoke.x3d` (new fixture) + a manual render check (documented below).

**Interfaces:**
- Consumes: `render_detail::SkyboxTextures` + `RenderOptions::skybox` (Task 2), `makeTextureResolver` (`ProceduralTexture.hpp`), `Texture::fromRGBA8`, `ctx.boundBackground()`, `rt::geombounds::getField`.

- [ ] **Step 1: Add a skybox fixture (proc faces, distinct per side)**

`examples/cpu_raster/assets/skybox_smoke.x3d`:

```xml
<X3D profile="Immersive" version="4.0"><Scene>
<Background frontUrl='"proc:checker"' backUrl='"proc:brick"'
            rightUrl='"proc:uvgrid"' leftUrl='"proc:bars"'
            topUrl='"proc:gradient"' bottomUrl='"proc:checker"'
            skyColor="0.1 0.1 0.2"/>
<Viewpoint position="0 0 0"/>
<Shape><Appearance><Material diffuseColor="0.8 0.8 0.8"/></Appearance><Box size="1 1 1"/></Shape>
</Scene></X3D>
```

- [ ] **Step 2: Resolve the six URLs into `opt.skybox` (after `opt.glyphAtlas` is set, ~main.cpp:170)**

```cpp
  // Resolve the bound Background's six panorama faces (if any) through the same
  // proc:/.ppm resolver used for ImageTexture, then hand them to the renderer.
  cr::render_detail::SkyboxTextures skybox;
  ex::TextureResolver texResolve = cr::makeTextureResolver(dirOf(scenePath));
  if (const x3d::runtime::X3DNode *bg = ctx.boundBackground()) {
    auto faceTex = [&](const char *field) -> cr::Texture {
      auto urls = x3d::runtime::geombounds::getField<MFString>(*bg, field, {});
      for (const std::string &u : urls) {           // ordered fallback
        ex::TexturePixelResult r = texResolve(u);
        if (r.ready() && !r.pixels.rgba.empty()) {
          const auto &p = r.pixels;
          return cr::Texture::fromRGBA8(p.rgba.data(), (int)p.width, (int)p.height,
                                        true, true, /*srgb=*/true);
        }
      }
      return {};
    };
    skybox.front  = faceTex("frontUrl");
    skybox.back   = faceTex("backUrl");
    skybox.right  = faceTex("rightUrl");
    skybox.left   = faceTex("leftUrl");
    skybox.top    = faceTex("topUrl");
    skybox.bottom = faceTex("bottomUrl");
  }
  if (skybox.any()) opt.skybox = &skybox;
```

(Confirm the include for `MFString`/`geombounds` is already transitively available — `SceneRender.hpp` uses both; add the header if the compiler disagrees.)

- [ ] **Step 3: Build the renderer and the smoke fixture**

Run: `cmake --build build-cpuraster --target x3d_cpu_raster`
Expected: builds clean.

- [ ] **Step 4: Render the fixture and verify the faces appear**

Run:
```bash
./build-cpuraster/examples/cpu_raster/x3d_cpu_raster \
  examples/cpu_raster/assets/skybox_smoke.x3d -o /tmp/skybox.ppm
```
Expected: the frame shows the proc textures on the surrounding cube — looking down −Z (default camera at origin, no orientation) the **front** face shows the `checker` pattern; the small Box sits in the centre. Convert to PNG and eyeball that the panorama (not the flat `skyColor`) fills the frame.

- [ ] **Step 5: Commit**

```bash
git add examples/cpu_raster/main.cpp examples/cpu_raster/assets/skybox_smoke.x3d
git commit -m "feat(cpu_raster): resolve Background panorama URLs and bind the skybox"
```

---

### Task 4: Docs

**Files:**
- Modify: `examples/cpu_raster/README.md`

- [ ] **Step 1: Add a capability bullet** (next to the Background gradient bullet from PR #27):

```markdown
- **`Background` skybox**: the bound `Background`'s six panorama faces
  (`frontUrl`/`backUrl`/`leftUrl`/`rightUrl`/`topUrl`/`bottomUrl`) are resolved
  through the `proc:`/`.ppm` resolver and drawn as an infinite cube around the
  viewer, composited over the sky/ground gradient by texel alpha (§Background).
```

- [ ] **Step 2: Commit**

```bash
git add examples/cpu_raster/README.md
git commit -m "docs(cpu_raster): document the Background skybox"
```

---

## Out of scope (follow-ups, noted not dropped)

- **`TextureBackground`** (six `*Texture` *node* children — `ImageTexture`/`PixelTexture`/`MovieTexture`/`MultiTexture`). Needs walking the `SFNode` children and resolving each child's `url`, vs. `Background`'s flat `MFString` fields. A clean Task-3 sibling once this lands; the Task 1/2 cube machinery is reused verbatim.
- **Per-face `TextureProperties`/repeat modes** — faces use clamp-ish default sampling; not exercised here.
- **Box-projected (finite) skyboxes** — X3D's cube is at infinity (direction-only), which is what this implements.

## Self-Review

- **Spec coverage:** six-URL faces (Task 1 selection + Task 3 resolution), cube-at-infinity rotate-not-translate (reuses PR #27's ray unprojection, which already ignores translation), alpha-over-gradient compositing (Task 2 `skyboxColor`). `TextureBackground` explicitly deferred. ✓
- **Type consistency:** `CubeFace`, `FaceSample{face,uv}`, `SkyboxTextures{front..bottom, any(), face()}`, `cubeFaceUv`, `skyboxColor`, `RenderOptions::skybox` used identically across Tasks 1–3. ✓
- **Placeholder scan:** every code step is concrete; the only judgement call (per-face UV sign flips) is bounded by Task 1's asserts and called out. ✓
