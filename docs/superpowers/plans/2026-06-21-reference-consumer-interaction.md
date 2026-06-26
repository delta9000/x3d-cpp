# Reference Browser Consumer — Interaction Increment Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the existing `examples/poc_renderer` interactive — clicking geometry fires pointing-device sensors and mouse+WASD drives navigation — by feeding glfw input into the SDK seam and attaching the two interaction systems, with a spec-faithful nav-vs-sensor arbitration referee.

**Architecture:** Approach A — the consumer owns *input* (device → `setPointer`/`setKey`), the SDK systems own *behavior* (`PickSystem` picks, `PointingSensorSystem` fires sensors, `NavigationSystem` moves the bound Viewpoint). A per-tick "pointer consumed by sensor" flag on `X3DExecutionContext`, set by `PointingSensorSystem` and honored by `NavigationSystem`, gives the sensor grab exclusive pointer ownership (§20.2.1).

**Tech Stack:** C++20, header-only SDK (`x3d_cpp`), GLFW 3.4 + glad core-3.3 (Wayland-native) for the consumer, the project's own `Mat4`/`SFVec3f`/`Ray` math.

## Global Constraints

- C++20; the SDK layer (`runtime/`) is header-only — edits are to `.hpp` files.
- No new third-party deps. Consumer uses the already-vendored GLFW/glad/stb.
- The consumer (`examples/poc_renderer/`) is firewalled behind `X3D_CPP_BUILD_POC=OFF`; the default `mise run build` / `ctest` path must stay green and never compile consumer code.
- SDK-side tests (Tasks 1–4) register in the root `CMakeLists.txt` test block and run under `ctest --preset dev`. Consumer-side tests (Task 5) register in `examples/poc_renderer/CMakeLists.txt` and run only when `X3D_CPP_BUILD_POC=ON`.
- Build command for SDK tests: `mise run build` (Ninja+ccache configure+build+ctest). Run a single test: `ctest --preset dev -R <name> --output-on-failure`.
- TDD throughout: failing test first, minimal code, green, commit. Frequent commits.

---

## File Structure

**SDK core (header-only, runtime/):**
- `runtime/events/X3DExecutionContext.hpp` — add the per-tick `pointerConsumedBySensor_` flag + accessors + tick-start reset.
- `runtime/events/PointingSensorSystem.hpp` — set the flag when a grab is active.
- `runtime/events/NavigationSystem.hpp` — honor the flag (skip pointer-drag) + `setForcedMode` override.
- `runtime/events/X3DSceneBridge.hpp` — add `attachInteractive(scene, ctx)` helper (adds PointingSensorSystem then NavigationSystem, returns the nav system).

**SDK tests (runtime/events/tests/):**
- `pointer_arbitration_test.cpp` (new) — the flag default/reset + pointing sets it.
- `nav_arbitration_test.cpp` (new) — nav forcedMode + nav skips drag when consumed.
- `interactive_wiring_test.cpp` (new) — the headless interaction acceptance gate.

**Consumer (examples/poc_renderer/):**
- `input.hpp` / `input.cpp` (new) — `unproject()` pure function, GLFW→X3D keycode map, `InputBridge` (glfw callbacks + per-frame ray feed + nav-mode cycle).
- `tests/input_test.cpp` (new) — `unproject()` + keycode-map unit tests (POC-gated).
- `main.cpp` — register the bridge, feed input before tick, stash lastView/lastProj, call `attachInteractive`.
- `CMakeLists.txt` — add `input.cpp` to the exe; add the `x3d_poc_input` test target.

---

### Task 1: Arbitration flag on X3DExecutionContext

**Files:**
- Modify: `runtime/events/X3DExecutionContext.hpp` (tick body ~line 193; input-seam block ~line 270; private members ~line 419)
- Test: `runtime/events/tests/pointer_arbitration_test.cpp` (create)
- Modify: `CMakeLists.txt` (test registration, near line 1042)

**Interfaces:**
- Produces: `bool X3DExecutionContext::pointerConsumedBySensor() const;` and `void X3DExecutionContext::setPointerConsumedBySensor(bool);`. The flag is reset to `false` at the start of every `tick(now)`.

- [ ] **Step 1: Write the failing test**

Create `runtime/events/tests/pointer_arbitration_test.cpp`:

```cpp
// pointer_arbitration_test.cpp — the per-tick "pointer consumed by a sensor"
// flag on X3DExecutionContext (nav-vs-sensor arbitration seam). Default false;
// settable within a tick; reset to false at the start of each tick().
#include "X3DExecutionContext.hpp"
#include <iostream>

using namespace x3d::runtime;

static int failures = 0;
static void check(bool c, const char *what) {
  if (!c) { std::cerr << "FAIL: " << what << "\n"; ++failures; }
  else    { std::cout << "ok: " << what << "\n"; }
}

// A System that sets the flag during update — proves a system can claim the
// pointer within a tick.
struct Claimer : System {
  void attach(X3DNode *, X3DExecutionContext &) override {}
  void update(double, X3DExecutionContext &ctx) override {
    ctx.setPointerConsumedBySensor(true);
  }
};

int main() {
  X3DExecutionContext ctx;
  check(ctx.pointerConsumedBySensor() == false, "default is false");

  ctx.setPointerConsumedBySensor(true);
  check(ctx.pointerConsumedBySensor() == true, "setter works");

  // tick() must reset the flag at tick start; with no claiming system it ends false.
  ctx.tick(1.0);
  check(ctx.pointerConsumedBySensor() == false, "tick() resets flag to false");

  // A claiming system leaves it true at end of tick.
  ctx.addSystem(std::make_shared<Claimer>());
  ctx.tick(2.0);
  check(ctx.pointerConsumedBySensor() == true, "claiming system sets flag during tick");

  return failures ? 1 : 0;
}
```

Register it in `CMakeLists.txt` next to the other event tests (mirror the `x3d_pointing_sensor` block at ~line 1042):

```cmake
    add_executable(x3d_pointer_arbitration
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/pointer_arbitration_test.cpp")
    target_link_libraries(x3d_pointer_arbitration PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_pointer_arbitration COMMAND x3d_pointer_arbitration)
```

- [ ] **Step 2: Run test to verify it fails**

Run: `mise run build` (configure picks up the new target), expect compile failure: `pointerConsumedBySensor` / `setPointerConsumedBySensor` are not members of `X3DExecutionContext`.

- [ ] **Step 3: Add the flag, accessors, and tick reset**

In `runtime/events/X3DExecutionContext.hpp`, in the public input-seam block (right after `keyState()`'s area near line 308), add:

```cpp
  // Nav-vs-sensor arbitration (§20.2.1): true while a pointing-device sensor
  // grab owns the pointer this tick. Reset at tick start; PointingSensorSystem
  // sets it; NavigationSystem honors it (skips pointer-drag). See ADR / the
  // reference-consumer interaction spec.
  bool pointerConsumedBySensor() const { return pointerConsumedBySensor_; }
  void setPointerConsumedBySensor(bool v) { pointerConsumedBySensor_ = v; }
```

In the private members block (near the existing `bool ticking_ = false;` at ~line 419), add:

```cpp
  bool pointerConsumedBySensor_ = false; // per-tick nav/sensor arbitration flag
```

In `tick(double now)`, immediately after `now_ = now;` (line 211), add the reset:

```cpp
    now_ = now;
    pointerConsumedBySensor_ = false;     // reset per-tick arbitration flag
    dirty_.clear();                       // drop last tick's changed-set
```

- [ ] **Step 4: Run test to verify it passes**

Run: `ctest --preset dev -R x3d_pointer_arbitration --output-on-failure`
Expected: PASS (4 `ok:` lines).

- [ ] **Step 5: Commit**

```bash
git add runtime/events/X3DExecutionContext.hpp runtime/events/tests/pointer_arbitration_test.cpp CMakeLists.txt
git commit -m "feat(runtime): per-tick pointer-consumed-by-sensor arbitration flag"
```

---

### Task 2: PointingSensorSystem sets the flag on an active grab

**Files:**
- Modify: `runtime/events/PointingSensorSystem.hpp` (end of `update(...)`)
- Test: `runtime/events/tests/pointer_arbitration_test.cpp` (extend)

**Interfaces:**
- Consumes: `ctx.setPointerConsumedBySensor(bool)` from Task 1.
- Produces: after a tick where a pointing-device sensor grab is active (`active_ != nullptr`), `ctx.pointerConsumedBySensor() == true`.

- [ ] **Step 1: Write the failing test (extend the arbitration test)**

Add these includes at the top of `runtime/events/tests/pointer_arbitration_test.cpp`:

```cpp
#include "PointingSensorSystem.hpp"
#include "X3DDocument.hpp"
#include "Box.hpp"
#include "Group.hpp"
#include "Shape.hpp"
#include "TouchSensor.hpp"
#include "Ray.hpp"
#include <any>
#include <memory>
#include <vector>
```

Add these helpers above `main()` (mirrors `pointing_sensor_test.cpp`):

```cpp
static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
static void addChild(const std::shared_ptr<X3DNode> &p, const std::shared_ptr<X3DNode> &c) {
  for (auto &f : p->fields())
    if (f.x3dName == "children" && f.set) {
      auto k = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*p));
      k.push_back(c);
      f.set(*p, std::any(std::move(k)));
      return;
    }
}
```

Add this test function and call it from `main()` before `return`:

```cpp
static void test_pointing_sets_flag() {
  using namespace x3d;
  auto group = std::make_shared<Group>();
  auto sensor = std::make_shared<TouchSensor>();
  auto shape = std::make_shared<Shape>();
  auto box = std::make_shared<Box>();
  setF(box, "size", std::any(SFVec3f{2, 2, 2}));
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(box)));
  addChild(group, sensor);
  addChild(group, shape);
  Scene scene; scene.addRootNode(group);

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  ctx.addSystem(std::make_shared<PointingSensorSystem>());

  // Pointer straight down -Z at the box, present, button up: over but no grab.
  ctx.setPointerPresent(true);
  ctx.setPointer(Ray{{0, 0, 10}, {0, 0, -1}});
  ctx.tick(1.0);
  check(ctx.pointerConsumedBySensor() == false, "over-but-no-grab ⇒ flag false");

  // Button down while over ⇒ grab ⇒ flag true.
  ctx.setPointerButton(true);
  ctx.tick(2.0);
  check(ctx.pointerConsumedBySensor() == true, "active grab ⇒ flag true");

  // Button up ⇒ grab released ⇒ flag false again.
  ctx.setPointerButton(false);
  ctx.tick(3.0);
  check(ctx.pointerConsumedBySensor() == false, "grab released ⇒ flag false");
}
```

In `main()`, add `test_pointing_sets_flag();` before `return failures ? 1 : 0;`.

- [ ] **Step 2: Run test to verify it fails**

Run: `ctest --preset dev -R x3d_pointer_arbitration --output-on-failure`
Expected: FAIL on "active grab ⇒ flag true" (the flag is never set by PointingSensorSystem yet).

- [ ] **Step 3: Set the flag at the end of PointingSensorSystem::update**

In `runtime/events/PointingSensorSystem.hpp`, at the very end of the `update(...)` method body (after the final `buttonWasDown_ = ps.buttonDown;`), add:

```cpp
    // Arbitration (§20.2.1): while this system holds a grab, the pointer is
    // exclusively ours — tell NavigationSystem (which runs after us) to skip
    // pointer-drag this tick. Reset each tick by X3DExecutionContext::tick.
    if (active_) ctx.setPointerConsumedBySensor(true);
```

- [ ] **Step 4: Run test to verify it passes**

Run: `ctest --preset dev -R x3d_pointer_arbitration --output-on-failure`
Expected: PASS (all `ok:` lines incl. the three new ones).

- [ ] **Step 5: Commit**

```bash
git add runtime/events/PointingSensorSystem.hpp runtime/events/tests/pointer_arbitration_test.cpp
git commit -m "feat(runtime): PointingSensorSystem claims the pointer while grabbed"
```

---

### Task 3: NavigationSystem — honor the flag + forced-mode override

**Files:**
- Modify: `runtime/events/NavigationSystem.hpp` (add `#include <optional>`; `mode` line; `dragging` line; Lookat begin guard; `setForcedMode`; `forcedMode_` member)
- Test: `runtime/events/tests/nav_arbitration_test.cpp` (create)
- Modify: `CMakeLists.txt` (test registration)

**Interfaces:**
- Consumes: `ctx.pointerConsumedBySensor()` from Task 1; `NavigationSystem::Mode` enum `{ None, Examine, Fly, Lookat }`; `ctx.viewMatrix()` (world→camera `Mat4`).
- Produces: `void NavigationSystem::setForcedMode(std::optional<Mode> m);` — when set, overrides `NavigationInfo.type`. When the arbitration flag is set, `NavigationSystem` performs no pointer-drag rotation that tick.

- [ ] **Step 1: Write the failing test**

Create `runtime/events/tests/nav_arbitration_test.cpp`:

```cpp
// nav_arbitration_test.cpp — NavigationSystem honors the pointer-consumed flag
// (a sensor grab suppresses nav drag) and the forced-mode override.
#include "NavigationSystem.hpp"
#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "Viewpoint.hpp"
#include "Ray.hpp"
#include <any>
#include <iostream>
#include <memory>

using namespace x3d;
using namespace x3d::runtime;

static int failures = 0;
static void check(bool c, const char *what) {
  if (!c) { std::cerr << "FAIL: " << what << "\n"; ++failures; }
  else    { std::cout << "ok: " << what << "\n"; }
}
static bool sameMatrix(const Mat4 &a, const Mat4 &b) {
  for (int i = 0; i < 16; ++i) if (std::fabs(a.m[i] - b.m[i]) > 1e-6f) return false;
  return true;
}

// A system that claims the pointer before NavigationSystem runs (added first).
struct Claimer : System {
  void attach(X3DNode *, X3DExecutionContext &) override {}
  void update(double, X3DExecutionContext &ctx) override {
    ctx.setPointerConsumedBySensor(true);
  }
};

static std::shared_ptr<Scene> sceneWithViewpoint() {
  auto vp = std::make_shared<Viewpoint>();
  auto scene = std::make_shared<Scene>();
  scene->addRootNode(vp);
  return scene;
}

// Feed a two-sample drag (button down, pointer moves) so NavigationSystem sees a
// nonzero dx. Returns the view matrix after the drag tick.
static Mat4 dragAndView(X3DExecutionContext &ctx) {
  ctx.setPointerPresent(true);
  ctx.setPointerButton(true);
  ctx.setPointer(Ray{{0, 0, 10}, {0, 0, -1}});
  ctx.tick(1.0); // establishes lastPx_/lastPy_ (dragActive_ becomes true)
  ctx.setPointer(Ray{{0.3f, 0, 10}, {0, 0, -1}}); // move in x ⇒ dx != 0
  ctx.tick(2.0);
  return ctx.viewMatrix();
}

static void test_examine_drag_rotates_without_flag() {
  auto scene = sceneWithViewpoint();
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(*scene);
  ctx.addSystem(std::make_shared<NavigationSystem>()); // default EXAMINE
  Mat4 before = ctx.viewMatrix();
  Mat4 after = dragAndView(ctx);
  check(!sameMatrix(before, after), "examine drag rotates the view (control)");
}

static void test_flag_suppresses_nav_drag() {
  auto scene = sceneWithViewpoint();
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(*scene);
  ctx.addSystem(std::make_shared<Claimer>());          // claims pointer FIRST
  ctx.addSystem(std::make_shared<NavigationSystem>());
  Mat4 before = ctx.viewMatrix();
  Mat4 after = dragAndView(ctx);
  check(sameMatrix(before, after), "consumed flag suppresses nav drag");
}

static void test_forced_mode_fly_moves_on_key() {
  auto scene = sceneWithViewpoint();
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(*scene);
  auto nav = std::make_shared<NavigationSystem>();
  nav->setForcedMode(NavigationSystem::Mode::Fly);
  ctx.addSystem(nav);
  ctx.tick(0.0); // seed time
  Mat4 before = ctx.viewMatrix();
  ctx.setKey(NavigationSystem::kKeyForward, true);
  ctx.tick(0.5); // dt = 0.5 ⇒ fly translates forward
  check(!sameMatrix(before, ctx.viewMatrix()), "forced FLY + forward key moves camera");
}

int main() {
  test_examine_drag_rotates_without_flag();
  test_flag_suppresses_nav_drag();
  test_forced_mode_fly_moves_on_key();
  return failures ? 1 : 0;
}
```

Register in `CMakeLists.txt`:

```cmake
    add_executable(x3d_nav_arbitration
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/nav_arbitration_test.cpp")
    target_link_libraries(x3d_nav_arbitration PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_nav_arbitration COMMAND x3d_nav_arbitration)
```

- [ ] **Step 2: Run test to verify it fails**

Run: `mise run build` then `ctest --preset dev -R x3d_nav_arbitration --output-on-failure`
Expected: compile failure (`setForcedMode` not a member) — or, once that compiles, FAIL on "consumed flag suppresses nav drag" (nav doesn't honor the flag yet).

- [ ] **Step 3: Implement forced-mode + flag honoring**

In `runtime/events/NavigationSystem.hpp`:

Add the include near the top (with the other includes):

```cpp
#include <optional>
```

Add the public override method (right after the `attach` no-op at ~line 64):

```cpp
  // Override the scene's NavigationInfo.type (dev affordance: the consumer's
  // mode-cycle key). std::nullopt = scene-driven (default).
  void setForcedMode(std::optional<Mode> m) { forcedMode_ = m; }
```

Change the `mode` resolution line (line 79) from:

```cpp
    const Mode mode = resolveMode(nav);
```

to:

```cpp
    const Mode mode = forcedMode_ ? *forcedMode_ : resolveMode(nav);
```

Change the `dragging` line (line 90) from:

```cpp
    const bool dragging = ps.present && ps.buttonDown;
```

to:

```cpp
    // Arbitration: a pointing-device sensor grab owns the pointer (§20.2.1);
    // do not also drag-navigate this tick.
    const bool dragging = ps.present && ps.buttonDown && !ctx.pointerConsumedBySensor();
```

Change the Lookat begin guard (line 115) from:

```cpp
      if (ps.present && ps.buttonDown && !buttonWasDown_)
```

to:

```cpp
      if (ps.present && ps.buttonDown && !buttonWasDown_ && !ctx.pointerConsumedBySensor())
```

Add the member in the cross-tick state block (after `Mode lastMode_ = Mode::None;` at ~line 435):

```cpp
  std::optional<Mode> forcedMode_; // consumer mode override (dev mode-cycle key)
```

- [ ] **Step 4: Run test to verify it passes**

Run: `ctest --preset dev -R x3d_nav_arbitration --output-on-failure`
Expected: PASS (3 `ok:` lines).

- [ ] **Step 5: Commit**

```bash
git add runtime/events/NavigationSystem.hpp runtime/events/tests/nav_arbitration_test.cpp CMakeLists.txt
git commit -m "feat(runtime): NavigationSystem honors sensor-grab arbitration + forced-mode override"
```

---

### Task 4: `attachInteractive` helper + headless interaction acceptance gate

**Files:**
- Modify: `runtime/events/X3DSceneBridge.hpp` (add `attachInteractive`)
- Test: `runtime/events/tests/interactive_wiring_test.cpp` (create)
- Modify: `CMakeLists.txt` (test registration)

**Interfaces:**
- Consumes: `PointingSensorSystem`, `NavigationSystem`, `ctx.addSystem`.
- Produces: `std::shared_ptr<NavigationSystem> attachInteractive(Scene &scene, X3DExecutionContext &ctx);` — adds `PointingSensorSystem` then `NavigationSystem` (order = arbitration: pointing claims before nav reads), returns the nav system so the consumer can call `setForcedMode`.

- [ ] **Step 1: Write the failing test (the acceptance gate)**

Create `runtime/events/tests/interactive_wiring_test.cpp`:

```cpp
// interactive_wiring_test.cpp — headless interaction acceptance gate (no GL).
// Proves attachInteractive wires both systems so that (a) a click fires a
// TouchSensor through the cascade, (b) WASD drives FLY navigation, and (c) a
// grab suppresses nav drag (arbitration ordering: pointing before nav).
#include "X3DSceneBridge.hpp"
#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "Box.hpp"
#include "Group.hpp"
#include "Shape.hpp"
#include "TouchSensor.hpp"
#include "Viewpoint.hpp"
#include "Ray.hpp"
#include <any>
#include <cmath>
#include <iostream>
#include <memory>
#include <vector>

using namespace x3d;
using namespace x3d::runtime;

static int failures = 0;
static void check(bool c, const char *what) {
  if (!c) { std::cerr << "FAIL: " << what << "\n"; ++failures; }
  else    { std::cout << "ok: " << what << "\n"; }
}
static bool sameMatrix(const Mat4 &a, const Mat4 &b) {
  for (int i = 0; i < 16; ++i) if (std::fabs(a.m[i] - b.m[i]) > 1e-6f) return false;
  return true;
}
static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
static void addChild(const std::shared_ptr<X3DNode> &p, const std::shared_ptr<X3DNode> &c) {
  for (auto &f : p->fields())
    if (f.x3dName == "children" && f.set) {
      auto k = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*p));
      k.push_back(c);
      f.set(*p, std::any(std::move(k)));
      return;
    }
}

int main() {
  // Scene: a viewpoint + a Group{ TouchSensor, Shape{ Box } }.
  auto group = std::make_shared<Group>();
  auto sensor = std::make_shared<TouchSensor>();
  auto shape = std::make_shared<Shape>();
  auto box = std::make_shared<Box>();
  setF(box, "size", std::any(SFVec3f{2, 2, 2}));
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(box)));
  addChild(group, sensor);
  addChild(group, shape);
  auto vp = std::make_shared<Viewpoint>();
  Scene scene; scene.addRootNode(vp); scene.addRootNode(group);

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  auto nav = attachInteractive(scene, ctx);
  check(nav != nullptr, "attachInteractive returns the NavigationSystem");

  // (a) Click on the box fires the TouchSensor through the cascade.
  ctx.setPointerPresent(true);
  ctx.setPointer(Ray{{0, 0, 10}, {0, 0, -1}}); // at the box
  ctx.setPointerButton(true);
  ctx.tick(1.0);                                // grab begins
  check(sensor->getIsActive() == true, "(a) TouchSensor isActive on click");
  ctx.setPointerButton(false);
  ctx.tick(2.0);                                // release while over ⇒ touchTime
  check(std::fabs(sensor->getTouchTime() - 2.0) < 1e-9, "(a) touchTime == release time");

  // (b) FLY + forward key moves the camera.
  nav->setForcedMode(NavigationSystem::Mode::Fly);
  ctx.setPointerPresent(false);
  ctx.tick(3.0);
  Mat4 beforeNav = ctx.viewMatrix();
  ctx.setKey(NavigationSystem::kKeyForward, true);
  ctx.tick(3.5);
  check(!sameMatrix(beforeNav, ctx.viewMatrix()), "(b) WASD drives FLY navigation");

  return failures ? 1 : 0;
}
```

Register in `CMakeLists.txt`:

```cmake
    add_executable(x3d_interactive_wiring
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/interactive_wiring_test.cpp")
    target_link_libraries(x3d_interactive_wiring PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_interactive_wiring COMMAND x3d_interactive_wiring)
```

- [ ] **Step 2: Run test to verify it fails**

Run: `mise run build` then `ctest --preset dev -R x3d_interactive_wiring --output-on-failure`
Expected: compile failure — `attachInteractive` not declared.

- [ ] **Step 3: Add the helper**

In `runtime/events/X3DSceneBridge.hpp`, alongside the other `attach*` free functions (after `attachFollowers`), add (ensure the file `#include`s `PointingSensorSystem.hpp` and `NavigationSystem.hpp`; add the includes near the top if missing):

```cpp
// Interactive consumer wiring (reference browser / CAVE-preview). Adds the two
// pointer-driven systems in arbitration order: PointingSensorSystem FIRST so a
// sensor grab claims the pointer before NavigationSystem reads it the same tick.
// Returns the NavigationSystem so the embedder can drive setForcedMode (the
// dev mode-cycle key). NavigationSystem/PointingSensorSystem both resolve their
// targets live from the input seam each tick, so neither needs per-node attach.
inline std::shared_ptr<NavigationSystem>
attachInteractive(Scene & /*scene*/, X3DExecutionContext &ctx) {
  ctx.addSystem(std::make_shared<PointingSensorSystem>()); // claims pointer first
  auto nav = std::make_shared<NavigationSystem>();
  ctx.addSystem(nav);                                      // reads pointer after
  return nav;
}
```

- [ ] **Step 4: Run test to verify it passes**

Run: `ctest --preset dev -R x3d_interactive_wiring --output-on-failure`
Expected: PASS (4 `ok:` lines). Then run the full suite to confirm no regressions: `mise run build`.

- [ ] **Step 5: Commit**

```bash
git add runtime/events/X3DSceneBridge.hpp runtime/events/tests/interactive_wiring_test.cpp CMakeLists.txt
git commit -m "feat(runtime): attachInteractive helper + headless interaction acceptance gate"
```

---

### Task 5: Consumer `unproject()` + keycode map (POC-gated unit tests)

**Files:**
- Create: `examples/poc_renderer/input.hpp`, `examples/poc_renderer/input.cpp`
- Create: `examples/poc_renderer/tests/input_test.cpp`
- Modify: `examples/poc_renderer/CMakeLists.txt`

**Interfaces:**
- Produces:
  - `x3d::runtime::Ray pocUnproject(double cx, double cy, int w, int h, const x3d::runtime::Mat4 &view, const x3d::runtime::Mat4 &proj);` — cursor pixel (top-left origin, glfw convention) → world-space ray (origin on near plane, normalized direction).
  - `int glfwToX3DNavKey(int glfwKey);` — GLFW key code → `NavigationSystem::kKey*` (0 if not a nav key).

- [ ] **Step 1: Write the failing test**

Create `examples/poc_renderer/tests/input_test.cpp`:

```cpp
// input_test.cpp — pure unprojection + keycode-map unit tests for the reference
// consumer input layer. No GL context required.
#include "../input.hpp"
#include "NavigationSystem.hpp"
#include <GLFW/glfw3.h>
#include <cmath>
#include <iostream>

using x3d::runtime::Mat4;
using x3d::runtime::Ray;

static int failures = 0;
static void check(bool c, const char *what) {
  if (!c) { std::cerr << "FAIL: " << what << "\n"; ++failures; }
  else    { std::cout << "ok: " << what << "\n"; }
}

// A simple right-handed perspective (matches examples/poc_renderer perspective()):
// here we just use identity view and a known projection, and assert the centre
// pixel maps to a ray pointing down -Z from near the origin.
static Mat4 perspectiveTest(float fovY, float aspect, float zn, float zf) {
  float f = 1.0f / std::tan(fovY * 0.5f);
  Mat4 m; // zero
  m.m[0] = f / aspect; m.m[5] = f;
  m.m[10] = (zf + zn) / (zn - zf); m.m[11] = -1.0f;
  m.m[14] = (2.0f * zf * zn) / (zn - zf);
  return m;
}

int main() {
  // Keycode map.
  check(glfwToX3DNavKey(GLFW_KEY_W) == x3d::runtime::NavigationSystem::kKeyForward, "W → forward");
  check(glfwToX3DNavKey(GLFW_KEY_S) == x3d::runtime::NavigationSystem::kKeyBack, "S → back");
  check(glfwToX3DNavKey(GLFW_KEY_A) == x3d::runtime::NavigationSystem::kKeyLeft, "A → left");
  check(glfwToX3DNavKey(GLFW_KEY_D) == x3d::runtime::NavigationSystem::kKeyRight, "D → right");
  check(glfwToX3DNavKey(GLFW_KEY_UP) == x3d::runtime::NavigationSystem::kKeyForward, "UP → forward");
  check(glfwToX3DNavKey(GLFW_KEY_SPACE) == 0, "non-nav key → 0");

  // Unproject the centre pixel of an 800x600 view with identity view matrix.
  Mat4 view = Mat4::identity();
  Mat4 proj = perspectiveTest(0.7854f, 800.0f / 600.0f, 0.1f, 1000.0f);
  Ray r = pocUnproject(400.0, 300.0, 800, 600, view, proj);
  // Centre ray points along -Z, direction normalized.
  float len = std::sqrt(r.direction.x * r.direction.x + r.direction.y * r.direction.y +
                        r.direction.z * r.direction.z);
  check(std::fabs(len - 1.0f) < 1e-4f, "centre ray direction is normalized");
  check(r.direction.z < -0.9f, "centre ray points down -Z");
  check(std::fabs(r.direction.x) < 1e-3f && std::fabs(r.direction.y) < 1e-3f,
        "centre ray has no x/y tilt");

  return failures ? 1 : 0;
}
```

- [ ] **Step 2: Write the headers/impl skeleton so it compiles to a FAIL**

Create `examples/poc_renderer/input.hpp`:

```cpp
// input.hpp — reference-consumer input layer: pure unprojection + GLFW→X3D key
// mapping + the InputBridge that feeds the SDK seam. Pure functions are testable
// without a GL context.
#pragma once
#include "Mat4.hpp"
#include "Ray.hpp"

// Cursor pixel (top-left origin, glfw convention) → world-space ray. The ray
// origin is on the near plane; the direction is normalized toward the far plane.
x3d::runtime::Ray pocUnproject(double cx, double cy, int w, int h,
                               const x3d::runtime::Mat4 &view,
                               const x3d::runtime::Mat4 &proj);

// GLFW key code → NavigationSystem::kKey* (0 when not a navigation key).
int glfwToX3DNavKey(int glfwKey);
```

Create `examples/poc_renderer/input.cpp` with the keycode map and a deliberately-wrong unproject stub (so the test fails first):

```cpp
#include "input.hpp"
#include "NavigationSystem.hpp"
#include <GLFW/glfw3.h>

int glfwToX3DNavKey(int glfwKey) {
  using NS = x3d::runtime::NavigationSystem;
  switch (glfwKey) {
    case GLFW_KEY_W: case GLFW_KEY_UP:    return NS::kKeyForward;
    case GLFW_KEY_S: case GLFW_KEY_DOWN:  return NS::kKeyBack;
    case GLFW_KEY_A: case GLFW_KEY_LEFT:  return NS::kKeyLeft;
    case GLFW_KEY_D: case GLFW_KEY_RIGHT: return NS::kKeyRight;
    default: return 0;
  }
}

x3d::runtime::Ray pocUnproject(double, double, int, int,
                               const x3d::runtime::Mat4 &,
                               const x3d::runtime::Mat4 &) {
  return {}; // STUB — fails the test until implemented in Step 4.
}
```

Register the test exe in `examples/poc_renderer/CMakeLists.txt` (after the `x3d_poc_renderer` target). It needs glad/glfw headers for `GLFW/glfw3.h` and the SDK include:

```cmake
# Pure input-layer unit test (no GL context; only needs the GLFW + SDK headers).
add_executable(x3d_poc_input
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/input_test.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/input.cpp")
target_link_libraries(x3d_poc_input PRIVATE x3d_cpp::x3d_cpp glfw)
target_compile_features(x3d_poc_input PRIVATE cxx_std_20)
add_test(NAME x3d_poc_input COMMAND x3d_poc_input)
```

- [ ] **Step 3: Run test to verify it fails**

Run:
```bash
cmake -S . -B build-poc -G Ninja -DX3D_CPP_BUILD_POC=ON -DX3D_CPP_BUILD_TESTS=ON
cmake --build build-poc --target x3d_poc_input
ctest --test-dir build-poc -R x3d_poc_input --output-on-failure
```
Expected: FAIL on the unproject ray checks (stub returns a default Ray); keycode-map checks PASS.

- [ ] **Step 4: Implement `pocUnproject`**

Replace the stub in `examples/poc_renderer/input.cpp`:

```cpp
#include <array>
#include <cmath>

x3d::runtime::Ray pocUnproject(double cx, double cy, int w, int h,
                               const x3d::runtime::Mat4 &view,
                               const x3d::runtime::Mat4 &proj) {
  using x3d::runtime::Mat4;
  using x3d::runtime::SFVec3f;
  // Pixel → NDC. glfw cursor origin is top-left, +y down; NDC +y is up.
  const float ndcX = 2.0f * static_cast<float>(cx) / static_cast<float>(w) - 1.0f;
  const float ndcY = 1.0f - 2.0f * static_cast<float>(cy) / static_cast<float>(h);
  const Mat4 invVP = (proj * view).inverse();

  // Homogeneous transform with perspective divide (Mat4::transformPoint assumes
  // w==1, which is wrong through an inverse projection — do the w-divide here).
  auto unprojClip = [&](float z) -> SFVec3f {
    const float x = ndcX, y = ndcY, w4 = 1.0f;
    const auto &m = invVP.m;
    float ox = m[0]*x + m[4]*y + m[8]*z  + m[12]*w4;
    float oy = m[1]*x + m[5]*y + m[9]*z  + m[13]*w4;
    float oz = m[2]*x + m[6]*y + m[10]*z + m[14]*w4;
    float ow = m[3]*x + m[7]*y + m[11]*z + m[15]*w4;
    if (std::fabs(ow) < 1e-20f) ow = 1.0f;
    return SFVec3f{ox / ow, oy / ow, oz / ow};
  };

  SFVec3f nearP = unprojClip(-1.0f); // near plane
  SFVec3f farP  = unprojClip(1.0f);  // far plane
  SFVec3f dir{farP.x - nearP.x, farP.y - nearP.y, farP.z - nearP.z};
  float len = std::sqrt(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
  if (len > 1e-20f) { dir.x /= len; dir.y /= len; dir.z /= len; }
  return x3d::runtime::Ray{nearP, dir};
}
```

- [ ] **Step 5: Run test to verify it passes**

Run:
```bash
cmake --build build-poc --target x3d_poc_input
ctest --test-dir build-poc -R x3d_poc_input --output-on-failure
```
Expected: PASS (all `ok:` lines).

- [ ] **Step 6: Commit**

```bash
git add examples/poc_renderer/input.hpp examples/poc_renderer/input.cpp \
        examples/poc_renderer/tests/input_test.cpp examples/poc_renderer/CMakeLists.txt
git commit -m "feat(poc): consumer input layer — unproject + GLFW→X3D keycode map"
```

---

### Task 6: `InputBridge` + wire interaction into `main.cpp`

**Files:**
- Modify: `examples/poc_renderer/input.hpp`, `examples/poc_renderer/input.cpp` (add `InputBridge`)
- Modify: `examples/poc_renderer/main.cpp` (construct bridge, feed ray pre-tick, attach interactive, stash lastView/lastProj)

**Interfaces:**
- Consumes: `pocUnproject`, `glfwToX3DNavKey`, `attachInteractive`, `ctx.setPointer*`/`setKey`, `NavigationSystem::setForcedMode`.
- Produces: `class InputBridge` — installs glfw callbacks (key, mouse-button, cursor-enter), feeds the pointer ray each frame, and cycles nav mode on a key.

This task is integration; it is verified by the Task 5 unit test (already green) plus a manual GUI run. Each step is still small and committed.

- [ ] **Step 1: Add the `InputBridge` declaration**

Append to `examples/poc_renderer/input.hpp`:

```cpp
#include "NavigationSystem.hpp"
#include "X3DExecutionContext.hpp"
#include <memory>
struct GLFWwindow;

// Installs glfw callbacks that translate device input into the SDK input seam,
// and feeds the per-frame pointer ray. Holds NO scene logic.
class InputBridge {
public:
  InputBridge(x3d::runtime::X3DExecutionContext &ctx, GLFWwindow *win,
              std::shared_ptr<x3d::runtime::NavigationSystem> nav);

  // Call once per frame BEFORE ctx.tick(), with the PREVIOUS frame's view/proj
  // (the image the user is pointing at) and the current framebuffer size.
  void feedPointerRay(const x3d::runtime::Mat4 &view,
                      const x3d::runtime::Mat4 &proj, int w, int h);

private:
  static void onKey(GLFWwindow *, int key, int sc, int action, int mods);
  static void onMouseButton(GLFWwindow *, int button, int action, int mods);
  static void onCursorEnter(GLFWwindow *, int entered);

  x3d::runtime::X3DExecutionContext &ctx_;
  GLFWwindow *win_;
  std::shared_ptr<x3d::runtime::NavigationSystem> nav_;
  int modeIndex_ = 0; // cycles Examine→Fly→Lookat on the mode key
};
```

- [ ] **Step 2: Implement `InputBridge`**

Append to `examples/poc_renderer/input.cpp`:

```cpp
#include <GLFW/glfw3.h>

InputBridge::InputBridge(x3d::runtime::X3DExecutionContext &ctx, GLFWwindow *win,
                         std::shared_ptr<x3d::runtime::NavigationSystem> nav)
    : ctx_(ctx), win_(win), nav_(std::move(nav)) {
  glfwSetWindowUserPointer(win, this);
  glfwSetKeyCallback(win, &InputBridge::onKey);
  glfwSetMouseButtonCallback(win, &InputBridge::onMouseButton);
  glfwSetCursorEnterCallback(win, &InputBridge::onCursorEnter);
}

void InputBridge::onKey(GLFWwindow *win, int key, int, int action, int) {
  auto *self = static_cast<InputBridge *>(glfwGetWindowUserPointer(win));
  if (!self) return;
  const bool down = (action != GLFW_RELEASE); // PRESS or REPEAT ⇒ held
  // Dev mode-cycle key: TAB cycles Examine→Fly→Lookat.
  if (key == GLFW_KEY_TAB && action == GLFW_PRESS && self->nav_) {
    using M = x3d::runtime::NavigationSystem::Mode;
    static const M order[] = {M::Examine, M::Fly, M::Lookat};
    self->modeIndex_ = (self->modeIndex_ + 1) % 3;
    self->nav_->setForcedMode(order[self->modeIndex_]);
    return;
  }
  if (int navCode = glfwToX3DNavKey(key)) self->ctx_.setKey(navCode, down);
}

void InputBridge::onMouseButton(GLFWwindow *win, int button, int action, int) {
  auto *self = static_cast<InputBridge *>(glfwGetWindowUserPointer(win));
  if (!self) return;
  if (button == GLFW_MOUSE_BUTTON_LEFT)
    self->ctx_.setPointerButton(action == GLFW_PRESS);
}

void InputBridge::onCursorEnter(GLFWwindow *win, int entered) {
  auto *self = static_cast<InputBridge *>(glfwGetWindowUserPointer(win));
  if (!self) return;
  self->ctx_.setPointerPresent(entered != 0);
}

void InputBridge::feedPointerRay(const x3d::runtime::Mat4 &view,
                                 const x3d::runtime::Mat4 &proj, int w, int h) {
  if (w <= 0 || h <= 0) return;
  double cx = 0, cy = 0;
  glfwGetCursorPos(win_, &cx, &cy);
  ctx_.setPointer(pocUnproject(cx, cy, w, h, view, proj));
}
```

- [ ] **Step 3: Wire it into `main.cpp`**

In `examples/poc_renderer/main.cpp`:

1. Add the include near the top (with the other consumer includes):

```cpp
#include "input.hpp"
```

2. After the scene graph + routes are built and the existing runtime systems are attached (where the scene is wired, before the render loop at line 1034), attach the interactive systems and create the bridge. Add:

```cpp
  auto navSys = x3d::runtime::attachInteractive(scene, ctx);
  InputBridge input(ctx, win, navSys);
  Mat4 lastView = Mat4::identity();
  Mat4 lastProj = Mat4::identity();
```

3. At the **top** of the loop body, immediately after `double now = glfwGetTime() - t0;` and **before** `ctx.tick(now);` (line 1036), feed the pointer ray using the previous frame's camera:

```cpp
    int fbw = 0, fbh = 0;
    glfwGetFramebufferSize(win, &fbw, &fbh);
    input.feedPointerRay(lastView, lastProj, fbw, fbh);
```

4. After `proj` is computed (right after line 1106 where `proj` is assigned), stash both for next frame's unprojection:

```cpp
      lastView = view;
      lastProj = proj;
```

5. Add `input.cpp` to the renderer executable in `examples/poc_renderer/CMakeLists.txt` — change the `add_executable(x3d_poc_renderer ...)` line to include both sources:

```cmake
add_executable(x3d_poc_renderer
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/input.cpp")
```

- [ ] **Step 4: Build the renderer**

Run:
```bash
cmake -S . -B build-poc -G Ninja -DX3D_CPP_BUILD_POC=ON
cmake --build build-poc --target x3d_poc_renderer
```
Expected: links cleanly.

- [ ] **Step 5: Manual GUI verification**

Run the renderer on a scene containing a `TouchSensor` and on a general scene:
```bash
./build-poc/examples/poc_renderer/x3d_poc_renderer <path/to/scene-with-touchsensor.x3d>
```
Verify:
- Press **TAB** to cycle to FLY; **W/A/S/D** (or arrows) flies the camera.
- In EXAMINE mode (default / TAB back), left-drag orbits.
- Left-click on a `TouchSensor`-influenced shape activates it (observe any ROUTEd behavior animate); a drag that starts on the sensor does **not** also rotate the camera (arbitration).

If no local scene with a TouchSensor is handy, use any conformance-archive scene that contains one (search the corpus for `TouchSensor`).

- [ ] **Step 6: Commit**

```bash
git add examples/poc_renderer/input.hpp examples/poc_renderer/input.cpp \
        examples/poc_renderer/main.cpp examples/poc_renderer/CMakeLists.txt
git commit -m "feat(poc): InputBridge — glfw input → SDK seam; reference consumer is interactive"
```

---

## Self-Review

**Spec coverage:**
- Approach A (consumer owns input, SDK owns behavior) → Tasks 5–6 (input) + Tasks 1–4 (SDK behavior). ✓
- `InputBridge` device→seam translation → Task 6. ✓
- `attachInteractive` (pointing before nav) → Task 4. ✓
- Arbitration flag (ctx + pointing sets + nav honors) → Tasks 1, 2, 3. ✓
- `unproject` pure function (ray uses last frame's camera) → Task 5 + Task 6 wiring. ✓
- Keyboard map + KeyDeviceSensor forwarding — keycode map in Task 5; nav keys wired in Task 6. (KeyDeviceSensor character/action/modifier forwarding is optional polish noted in the spec; nav keys are the load-bearing path and are covered. Char-key forwarding can be a trivial follow-on — not gating interaction.) ✓ (scoped)
- Nav-mode cycle key (`setForcedMode`) → Task 3 (override) + Task 6 (TAB cycle). ✓
- Headless interaction acceptance gate → Task 4. ✓
- Edge cases (cursor leaves → present false; ray uses last frame) → Task 6 (`onCursorEnter`, `feedPointerRay`). ✓
- Deferred (rename/promotion, scroll/gamepad, CAVE serialization, fidelity) → not in any task, by design. ✓

**Placeholder scan:** The Task 5 Step 2 unproject is an intentional, labeled failing stub (TDD red), replaced in Step 4 — not a residual placeholder. No other TBDs.

**Type consistency:** `pointerConsumedBySensor()`/`setPointerConsumedBySensor(bool)` consistent across Tasks 1–3. `NavigationSystem::Mode` enumerators `{None,Examine,Fly,Lookat}` used consistently. `setForcedMode(std::optional<Mode>)` defined in Task 3, called in Tasks 4 & 6. `attachInteractive(Scene&, X3DExecutionContext&) → shared_ptr<NavigationSystem>` defined in Task 4, called in Task 6. `pocUnproject`/`glfwToX3DNavKey` signatures identical in Task 5 header, impl, and Task 6 usage. ✓

**Read-out API confirmed:** `X3DExecutionContext::viewMatrix()`, `cameraWorldPosition()`, and `cameraWorldUp()` are all public (`runtime/events/X3DExecutionContext.hpp:329–358`, before `private:` at 361). The Task 3/4 "camera moved" assertions use `ctx.viewMatrix()` directly — verified, no substitution needed.
