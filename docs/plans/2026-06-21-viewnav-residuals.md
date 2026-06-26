# Viewnav Residuals Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Close the three deferred-minor conformance findings (BIND-09, NAV-LOOKAT-SCALE, NAV-FLY-ROLL) remaining after the CONF-VIEWNAV cluster shipped.

**Architecture:** Three independent TDD sub-tasks, each its own commit. P1 verifies the LOOKAT-SCALE finding with a test first (RTC-8 pattern: if the test passes, the finding is INVALID). P2 threads a `BindTransition` side-channel through `X3DExecutionContext` so `ViewpointBindSystem` can distinguish a push (set_bind TRUE) from a pop (set_bind FALSE / delete) and apply §23.3.1 r6.3 (restore the popped-to vp's stored offset). P3 rewrites FLY orientation as yaw/pitch scalars reconstructed each step, with re-decomposition on mode/bind switches, eliminating horizon-roll drift. Single conformance-view regen at the end.

**Tech Stack:** C++17, CMake/Ninja, ccache, ctest. No new deps. Spec grounding: ISO/IEC 19775-1:2023 §23.3.1 (bind semantics), §23.4.4 (FLY/LOOKAT).

**Design doc:** `docs/superpowers/specs/2026-06-21-viewnav-residuals-design.md` (approved, commit `4dafa72`).

---

## Pre-flight

**Step 0a: Confirm clean-ish working tree**

There are uncommitted conformance edits in `docs/conformance/` (Sound audit SND-1..SND-9 from a separate workstream). **Do not touch them.** This plan only edits `runtime/` source + (at the very end) the three finding rows in `findings.yaml`. The Sound edits stay staged-but-uncommitted through the whole plan; the final `mise run conformance` regen will preserve them.

Run:
```bash
git status --short
git log --oneline -3
```

Expected: `4dafa72 docs(specs): viewnav-residuals design` is HEAD; `docs/conformance/{INDEX.md,components/Sound.md,components/Texturing.md,findings.yaml,model.json}` are modified but unstaged.

**Step 0b: Confirm baseline build + tests are green**

Run: `mise run build`
Expected: Configures + builds + runs ctest; all pass (including `navigation_test`, `viewpoint_bind_test`, `viewpoint_offset_test`).

If anything is red before you start, stop and report — the baseline isn't sound.

---

# Phase 1: NAV-LOOKAT-SCALE verification

**Files:**
- Modify: `runtime/events/tests/navigation_test.cpp` (append a new case before `main`'s `if (failures)` block)
- Read-only reference: `runtime/events/NavigationSystem.hpp:297-347` (`beginLookat`), `runtime/events/tests/navigation_test.cpp:184-219` (existing LOOKAT test pattern)

**Hypothesis (from design doc §1):** The audit note says LOOKAT mixes world-space radius with local-frame eye placement. Code trace says the round-trip is lossless: `targetLocal = parentInv.transformPoint(targetWorld)` and `worldOf(vp) * targetLocal == targetWorld`, so the camera ends at world distance `d = radius / tan(fov/2)` from the world center. **The math should be correct.** This test verifies that claim. If it passes, the finding is INVALID. If it fails, root-cause from the failing assertion.

### Task 1.1: Write the failing (or passing — either is informative) test

**Step 1: Add the test case to `navigation_test.cpp`**

Insert immediately before the `if (failures)` block at the end of `main()` (currently around line 251). Use the existing `makeWorld` helper — but it doesn't take a Transform ancestor, so build the scene inline.

```cpp
  // --- (6) LOOKAT framing under non-uniform ancestor scale (NAV-LOOKAT-SCALE) -
  // Audit claim: a non-uniformly-scaled ancestor Transform mis-sizes the framed
  // object because world-space radius is mixed with local-frame eye placement.
  // Verification: compute the world camera distance from the box's world center
  // and assert it equals radius / tan(fov/2). If it does, the math round-trips
  // and the finding is INVALID.
  {
    // Box of size (1,1,1) under Transform scale=(2, 0.5, 1):
    //   world AABB half-diagonal = (1, 0.25, 0.5); world radius = sqrt(1.3125).
    auto box = createX3DNode("Box"); setF(box, "size", std::any(SFVec3f{1,1,1}));
    auto shape = createX3DNode("Shape");
    setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(box)));
    auto xform = createX3DNode("Transform");
    setF(xform, "scale", std::any(SFVec3f{2.0f, 0.5f, 1.0f}));
    addChild(xform, shape);

    auto vp = createX3DNode("Viewpoint");
    setF(vp, "position", std::any(SFVec3f{0, 0, 10}));
    setF(vp, "fieldOfView", std::any(SFFloat{0.7853982f})); // pi/4
    auto nav = createX3DNode("NavigationInfo");
    dynamic_cast<NavigationInfo &>(*nav).setType({NavigationTypeValues::LOOKAT});
    dynamic_cast<NavigationInfo &>(*nav).setTransitionTime(0.0); // TELEPORT-like: snap

    Scene scene;
    scene.addRootNode(xform);
    scene.addRootNode(vp);
    scene.addRootNode(nav);
    X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
    ctx.addSystem(std::make_shared<NavigationSystem>());
    ctx.tick(0.0);

    // World AABB of the box: center (0,0,0), half-diag (1, 0.25, 0.5).
    Aabb wb = ctx.worldBounds(box.get());
    SFVec3f center{(wb.min.x+wb.max.x)*0.5f, (wb.min.y+wb.max.y)*0.5f, (wb.min.z+wb.max.z)*0.5f};
    SFVec3f half{(wb.max.x-wb.min.x)*0.5f, (wb.max.y-wb.min.y)*0.5f, (wb.max.z-wb.min.z)*0.5f};
    float radius = dist(half, {0,0,0}); // world-space radius
    float fov = 0.7853982f;
    float expected_d = radius / std::tan(fov * 0.5f);

    // Click on the box: pointer ray from the camera toward origin.
    ctx.setPointerPresent(true);
    ctx.setPointer(Ray{{0,0,10},{0,0,-1}});
    ctx.setPointerButton(true);
    ctx.tick(1.0);   // LOOKAT fires (transitionTime=0 -> completes same tick)
    ctx.setPointerButton(false);
    ctx.tick(2.0);   // drain

    SFVec3f cam = ctx.cameraWorldPosition();
    float actual_d = dist(cam, center);
    check(feq(actual_d, expected_d, 1e-3f),
          "lookat-scale: world camera distance == radius/tan(fov/2) "
          "(expected " + std::to_string(expected_d) +
          ", got " + std::to_string(actual_d) + ")");

    // Conservative frustum fit: the box's world AABB fits inside the view
    // sphere of radius `d` (sphere-contains-box).
    bool fits = (std::fabs(center.x - cam.x) <= actual_d + half.x + 1e-3f) &&
                (std::fabs(center.y - cam.y) <= actual_d + half.y + 1e-3f) &&
                (std::fabs(center.z - cam.z) <= actual_d + half.z + 1e-3f);
    check(fits, "lookat-scale: box world AABB fits inside the view frustum sphere");
  }
```

Note: `Aabb` and `Ray` are already used elsewhere in the test file (look at the existing LOOKAT test if unsure of the include path — they come transitively from `X3DExecutionContext.hpp`).

**Step 2: Build + run the test**

Run: `mise run build`
Expected: builds clean; `navigation_test` runs.

If it fails to compile, fix the include/types (e.g. `SFFloat` may not be needed if `setF` accepts `float` directly — check how other tests set float fields).

**Step 3: Inspect the test outcome**

```bash
ctest --output-on-failure -R navigation_test
```

Two outcomes:

- **Both checks PASS** → the math is correct → finding NAV-LOOKAT-SCALE is **INVALID**. Proceed to Task 1.2 (mark invalid).
- **Either check FAILS** → the bug is real. Read the failing assertion, root-cause (likely in `beginLookat` lines 297-324), apply the audit's suggested fix (compute framing distance in local frame, or scale `d` by the relevant ancestor factor). Add a regression check that the fix produces the expected distance. Then proceed to Task 1.2 (mark closed, not invalid).

### Task 1.2: Commit the verification test

**Step 1: Stage + commit (regardless of outcome — the test ships either way)**

```bash
git add runtime/events/tests/navigation_test.cpp
git commit -m "test(nav): LOOKAT framing under non-uniform ancestor scale (NAV-LOOKAT-SCALE verification)

Adds a test that constructs a Box of size (1,1,1) under a Transform with
scale=(2, 0.5, 1) and clicks LOOKAT, then asserts the world camera distance
from the box's world center equals radius/tan(fov/2) and that the box's
world AABB fits the view frustum sphere.

<Outcome: PASS (finding INVALID) | FAIL → fixed in <file:lines>>
"
```

Fill in the `<Outcome>` line based on Task 1.1 Step 3. If you applied a fix, mention the file and lines.

**Step 2: Record outcome in your task notes**

The actual `findings.yaml` flip happens in Phase 4 (single regen at the end). For now, just note the outcome so Phase 4 knows whether to mark `invalid` or `closed`.

---

# Phase 2: BIND-09 push-vs-pop signal + pop restore-stored-offset

**Files:**
- Modify: `runtime/events/X3DExecutionContext.hpp:242` (add `BindTransition` enum + accessor near `boundViewpoint`)
- Modify: `runtime/scene/BindingStack.hpp:25,35,47` (pass kind out of `bind`/`unbind`/`pushDefault`)
- Modify: `runtime/scene/BindingSystem.hpp:42,60,95` (emit lambdas set kind on viewpoint transitions)
- Modify: `runtime/events/ViewpointBindSystem.hpp:27,80` (consume kind in `onPostCascade`/`onBind`)
- Modify: `runtime/events/tests/viewpoint_bind_test.cpp` (3 new test cases)

### Task 2.1: Write the failing test for POP restores stored offset

**Step 1: Append test (4) → (9) renumber not needed; add as case (9) in `viewpoint_bind_test.cpp`**

Insert before `}` // namespace` at line 192. Pattern follows the existing `test_jump_true_snaps` and `test_delete_detach`.

```cpp
// (9) BIND-09: a pop (set_bind FALSE on the bound vp) restores the popped-to
// vp's stored offset (§23.3.1 r6.3), NOT resetting it like a fresh push.
void test_pop_restores_stored_offset() {
  auto A = std::make_shared<Viewpoint>(); A->setPosition(SFVec3f{0, 0, 10});
  auto B = std::make_shared<Viewpoint>(); B->setPosition(SFVec3f{0, 0, 100});
  // Both default jump=TRUE, retain=FALSE.
  Scene scene; scene.addRootNode(A); scene.addRootNode(B);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene); // A bound (first)
  attachViewpointBind(ctx);
  ctx.tick(0.0);
  // Give A a stored offset: eye nudged to (0,0,5).
  ctx.setViewpointOffset(A.get(), ViewpointOffset{Mat4::translation(SFVec3f{0, 0, -5})});
  ctx.tick(1.0); // capture lastCam

  // Push B onto the top (A's offset stays stored).
  ctx.postEvent(B.get(), "set_bind", std::any(SFBool{true}));
  ctx.tick(2.0);
  check(ctx.boundViewpoint() == B.get(), "B bound after push");

  // Pop B -> A becomes bound again. A's stored offset MUST be restored.
  ctx.postEvent(B.get(), "set_bind", std::any(SFBool{false}));
  ctx.tick(3.0);
  check(ctx.boundViewpoint() == A.get(), "A bound after pop");
  check(feq(ctx.cameraWorldPosition().z, 5),
        "BIND-09: pop restores A's stored offset (0,0,5), not zero");
}

// (10) PUSH still resets (negative control for BIND-09).
void test_push_still_resets() {
  auto A = std::make_shared<Viewpoint>(); A->setPosition(SFVec3f{0, 0, 10});
  auto B = std::make_shared<Viewpoint>(); B->setPosition(SFVec3f{0, 0, 100});
  Scene scene; scene.addRootNode(A); scene.addRootNode(B);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  attachViewpointBind(ctx);
  ctx.tick(0.0);
  ctx.setViewpointOffset(A.get(), ViewpointOffset{Mat4::translation(SFVec3f{0, 0, -5})});
  ctx.tick(1.0);

  // Push B.
  ctx.postEvent(B.get(), "set_bind", std::any(SFBool{true}));
  ctx.tick(2.0);
  // Push A back (NOT a pop — explicit set_bind TRUE on A).
  ctx.postEvent(A.get(), "set_bind", std::any(SFBool{true}));
  ctx.tick(3.0);
  check(feq(ctx.cameraWorldPosition().z, 10),
        "BIND-09 negative control: push A resets offset (0,0,10), NOT restore");
}

// (11) BIND-09: a pop still animates over transitionTime (continuous transition
// path, but the END pose is the popped-to vp's stored offset).
void test_pop_animates_over_transition_time() {
  auto nav = std::make_shared<NavigationInfo>(); nav->setTransitionTime(2.0);
  auto A = std::make_shared<Viewpoint>(); A->setPosition(SFVec3f{0, 0, 10});
  auto B = std::make_shared<Viewpoint>(); B->setPosition(SFVec3f{0, 0, 110});
  Scene scene; scene.addRootNode(nav); scene.addRootNode(A); scene.addRootNode(B);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  attachViewpointBind(ctx);
  ctx.tick(0.0);
  // A's stored offset: eye at (0,0,5).
  ctx.setViewpointOffset(A.get(), ViewpointOffset{Mat4::translation(SFVec3f{0, 0, -5})});
  ctx.tick(1.0);

  // Push B (camera moves to 110).
  ctx.postEvent(B.get(), "set_bind", std::any(SFBool{true}));
  ctx.tick(2.0); // B bound, cam ~110
  // Pop B -> A becomes bound, target is A's stored offset (eye at 5).
  ctx.postEvent(B.get(), "set_bind", std::any(SFBool{false}));
  ctx.tick(2.0); // bind change processed; transition starts at ~110
  check(feq(ctx.cameraWorldPosition().z, 110, 5.0f), "pop transition starts near B's cam (~110)");
  ctx.tick(3.0); // t=0.5 over 2s -> halfway between 110 and 5 -> ~57.5
  check(feq(ctx.cameraWorldPosition().z, 57.5f, 5.0f), "pop transition halfway (~57.5)");
  ctx.tick(4.0); // complete -> drives to A's stored (5)
  check(feq(ctx.cameraWorldPosition().z, 5, 1e-2f), "pop transition ends at A's stored offset (5)");
  ctx.tick(4.0); // drain transitionComplete
  check(nav->getTransitionComplete() == true, "BIND-09: pop emits transitionComplete");
}
```

**Step 2: Add the new test calls to `main()`**

After `test_rotation_only_transition();` (line 202), add:

```cpp
  test_pop_restores_stored_offset();
  test_push_still_resets();
  test_pop_animates_over_transition_time();
```

**Step 3: Build + run; verify all 3 new tests FAIL**

Run: `mise run build`
Expected: builds clean (no impl changes yet).

Run: `ctest --output-on-failure -R viewpoint_bind_test`
Expected: 3 new checks FAIL. Specifically:
- `test_pop_restores_stored_offset` "pop restores A's stored offset (0,0,5), not zero" — FAILS (current code resets to 10 because `onBind` applies jump/retain to A as if it were a push).
- `test_push_still_resets` — should already PASS (negative control).
- `test_pop_animates_over_transition_time` — the final "ends at A's stored offset (5)" check FAILS (resets to 10 instead).

If the negative control fails too, you have a bigger problem — stop and re-read the existing `onBind` logic.

### Task 2.2: Implement the BindTransition signal

**Step 1: Add the enum + accessor to `X3DExecutionContext.hpp`**

Right after line 245 (`boundNavigationInfo`), insert:

```cpp
  // BIND-09: side-channel that lets BindingSystem tell ViewpointBindSystem
  // whether the latest viewpoint bind change was a push (set_bind TRUE on a
  // non-bound vp) or a pop (set_bind FALSE / delete of the bound vp). Reset to
  // None by the consumer after each post-cascade. §23.3.1 r5.1 (push: apply
  // jump/retain) vs r6.3 (pop: restore stored offset).
  enum class BindTransition { None, Push, Pop };
  BindTransition lastViewpointBindTransition() const { return lastVpTransition_; }
  void setLastViewpointBindTransition(BindTransition k) { lastVpTransition_ = k; }
```

Then in the private member section (search for where other private members live — likely near `bindings_`, `transforms_`, etc.), add:

```cpp
  BindTransition lastVpTransition_ = BindTransition::None;
```

**Step 2: Make `BindingStack` report which kind of operation ran**

In `runtime/scene/BindingStack.hpp`, change the three mutating methods to take an additional `Emit &emit` (already there) — no signature change needed; the **caller** (`BindingSystem`) sets the side-channel based on which method it called. So **BindingStack.hpp does NOT need to change** — the design doc's mention of "pass kind out" is satisfied by the caller distinguishing by method. Re-read the design doc §2: "BindingSystem writes the kind based on which BindingStack method it called."

**Step 3: Set the kind in `BindingSystem`'s emit lambdas**

In `runtime/scene/BindingSystem.hpp`:

3a. In `bindDefaults` (around line 40-51), the emit lambda should set `Push` for the viewpoint category. But `bindDefaults` is generic across categories — we only care about Viewpoint. Easiest: thread an `isViewpoint` flag, or check the category inside the lambda. Use the category check (matches existing pattern):

```cpp
  void bindDefaults() {
    const double t = clock_();
    BindingStack::Emit emit = [this, t](X3DNode *n, bool bound) {
      setIsBound(n, bound);
      setBindTime(n, t);
    };
    for (auto &kv : enrolled_) {
      const std::string &cat = kv.first;
      if (!kv.second.empty() && !stacks_[cat].top()) {
        if (cat == "Viewpoint") lastVpTransition_ = BindTransition::Push;
        stacks_[cat].pushDefault(kv.second.front(), emit);
      }
    }
  }
```

Wait — `BindingSystem` doesn't have direct access to `ctx.lastVpTransition_`. The whole point is the side-channel lives on `ctx`. Re-read the design: the emit lambdas in `BindingSystem` set the kind via the `Poster` mechanism? No — the Poster posts *events to nodes*, not metadata on ctx.

**Correction:** `BindingSystem` needs a separate way to write `ctx.lastVpTransition_`. Two options:
  - (a) Give `BindingSystem` a `TransitionSink` callback (like `Poster`/`Clock`) that writes to ctx.
  - (b) Have `X3DExecutionContext` observe `BindingSystem`'s emit calls directly.

Simplest: option (a). Add a `TransitionSink` callback to `BindingSystem::enrollScene`:

Actually — re-reading `BindingSystem.hpp` more carefully — `enrollScene` takes `Poster` + `Clock`. Add a third callback `TransitionSink`:

```cpp
using TransitionSink = std::function<void(BindTransition)>;
void enrollScene(const Scene &scene, Poster poster, Clock clock, TransitionSink sink) {
  poster_ = std::move(poster);
  clock_ = std::move(clock);
  sink_ = std::move(sink);
  ...
}
```

But wait — `BindTransition` is defined on `X3DExecutionContext`. To avoid an include cycle (BindingSystem is included by X3DExecutionContext), define `BindTransition` in a tiny shared header or in `BindingStack.hpp`. Cleanest: put the enum in `BindingStack.hpp` (it's the natural owner) and have `X3DExecutionContext` re-export or use it directly.

**Revised Step 1:** Define `BindTransition` in `BindingStack.hpp` (top of file, after the include guard):

```cpp
namespace x3d::runtime {
enum class BindTransition { None, Push, Pop };
class BindingStack { ... };
}
```

Then `X3DExecutionContext.hpp` includes `BindingSystem.hpp` (transitively `BindingStack.hpp`), so it can use `BindTransition` directly. Move the enum out of `X3DExecutionContext` and into `BindingStack`.

**Revised Step 1 (actual):**

In `runtime/scene/BindingStack.hpp`, after `namespace x3d::runtime {` (line 13), insert:

```cpp
// BIND-09: kind of the latest viewpoint bind transition. Set by BindingSystem
// when a viewpoint stack mutation emits a bound/unbound transition; read by
// ViewpointBindSystem to distinguish §23.3.1 r5.1 (push: apply jump/retain)
// from r6.3 (pop: restore stored offset). Reset to None after each post-cascade.
enum class BindTransition { None, Push, Pop };
```

**Revised Step 2 (actual):** In `X3DExecutionContext.hpp`, after `boundNavigationInfo()` (line 245), insert accessors (no enum redefinition — use the one from BindingStack via the existing include):

```cpp
  BindTransition lastViewpointBindTransition() const { return lastVpTransition_; }
  void setLastViewpointBindTransition(BindTransition k) { lastVpTransition_ = k; }
```

And add the private member:

```cpp
  BindTransition lastVpTransition_ = BindTransition::None;
```

**Revised Step 3 (actual):** In `BindingSystem.hpp`, add the `TransitionSink` callback. Update `enrollScene` signature + the three emit sites.

In `BindingSystem.hpp`:

```cpp
class BindingSystem {
public:
  using Poster = std::function<void(X3DNode *, const std::string &, std::any)>;
  using Clock = std::function<double()>;
  using TransitionSink = std::function<void(BindTransition)>;

  void enrollScene(const Scene &scene, Poster poster, Clock clock,
                   TransitionSink sink = {}) {
    poster_ = std::move(poster);
    clock_ = std::move(clock);
    sink_ = std::move(sink);
    for (const auto &root : scene.rootNodes)
      if (root) walk(root.get());
  }
```

In `bindDefaults` (around line 42), the emit lambda doesn't know the category unless we capture `cat`. Since `bindDefaults` iterates `enrolled_` and knows `cat`, set the sink inline:

```cpp
  void bindDefaults() {
    const double t = clock_();
    BindingStack::Emit emit = [this, t](X3DNode *n, bool bound) {
      setIsBound(n, bound);
      setBindTime(n, t);
    };
    for (auto &kv : enrolled_) {
      const std::string &cat = kv.first;
      if (!kv.second.empty() && !stacks_[cat].top()) {
        if (sink_ && cat == "Viewpoint") sink_(BindTransition::Push);
        stacks_[cat].pushDefault(kv.second.front(), emit);
      }
    }
  }
```

In `removeNode` (around line 60), the emit lambda needs to know if the unbind was a top-pop. Easiest: query the stack before calling unbind. Actually, the emit lambda fires for the popped node (bound=false) and the new top (bound=true) — we want to set `Pop` once. Set it before calling `unbind`:

```cpp
  void removeNode(X3DNode *node) {
    if (!node || !poster_) return;
    auto it = stacks_.find(category(node));
    if (it == stacks_.end()) return;
    BindingStack::Emit emit = [this](X3DNode *t, bool bound) {
      poster_(t, "isBound", std::any(SFBool(bound)));
      poster_(t, "bindTime", std::any(SFTime(clock_())));
    };
    // BIND-09: if this is a viewpoint top-pop, signal Pop.
    if (sink_ && it->first == "Viewpoint" && it->second.top() == node)
      sink_(BindTransition::Pop);
    it->second.unbind(node, emit);
  }
```

In `enroll` (the `setOnSet_bindHandler` lambda, around line 95):

```cpp
    b->setOnSet_bindHandler([this, node, cat](const SFBool &v) {
      BindingStack::Emit emit = [this](X3DNode *t, bool bound) {
        poster_(t, "isBound", std::any(SFBool(bound)));
        poster_(t, "bindTime", std::any(SFTime(clock_())));
      };
      if (v) {
        if (sink_ && cat == "Viewpoint") sink_(BindTransition::Push);
        stacks_[cat].bind(node, emit);
      } else {
        // Pop only if `node` is the current top (matches BindingStack::unbind).
        if (sink_ && cat == "Viewpoint" && stacks_[cat].top() == node)
          sink_(BindTransition::Pop);
        stacks_[cat].unbind(node, emit);
      }
    });
```

Add the private member at the bottom of `BindingSystem`:

```cpp
  Poster poster_;
  Clock clock_;
  TransitionSink sink_;
  std::unordered_map<std::string, BindingStack> stacks_;
  std::unordered_map<std::string, std::vector<X3DNode *>> enrolled_;
```

**Step 4: Wire the sink at the `enrollScene` call site in `X3DExecutionContext`**

Find where `bindings_.enrollScene(...)` is called (the grep earlier showed `X3DExecutionContext.hpp:80`). Update it to pass a sink that writes `lastVpTransition_`:

```cpp
    bindings_.enrollScene(scene,
        [this](X3DNode *n, const std::string &f, std::any v) { postEvent(n, f, std::move(v)); },
        [this]() { return now(); },
        [this](BindTransition k) { lastVpTransition_ = k; });
```

(Adjust to match the existing call style — it may currently pass member-function pointers or bound lambdas. Match whatever's there.)

**Step 5: Build**

Run: `mise run build`
Expected: builds clean. If `BindTransition` is unresolved in `X3DExecutionContext.hpp`, check the include chain — `X3DExecutionContext.hpp` must include (directly or transitively) `BindingSystem.hpp`/`BindingStack.hpp`. It already does (it owns a `BindingSystem bindings_` member).

**Step 6: Run the BIND-09 tests — they should STILL FAIL**

Run: `ctest --output-on-failure -R viewpoint_bind_test`
Expected: same 3 failures as Task 2.1 Step 3 (we've added the signal but not the consumer). The negative control still passes.

### Task 2.3: Consume the kind in `ViewpointBindSystem::onBind`

**Step 1: Edit `ViewpointBindSystem.hpp`**

1a. In `onPostCascade` (line 27), pass the kind through:

```cpp
  void onPostCascade(X3DExecutionContext &ctx) {
    X3DNode *cur = ctx.boundViewpoint();
    if (cur != lastVp_) {
      anim_.active = false;
      const BindTransition kind = ctx.lastViewpointBindTransition();
      onBind(ctx, cur, kind);
      lastVp_ = cur;
      ctx.setLastViewpointBindTransition(BindTransition::None); // consume
    } else if (anim_.active && cur == anim_.vp) {
      advance(ctx);
    }
    if (cur) {
      lastCam_ = ctx.viewMatrix().inverse();
      haveCam_ = true;
    }
  }
```

1b. Change `onBind` signature + add the Pop branch:

```cpp
  void onBind(X3DExecutionContext &ctx, X3DNode *vp, BindTransition kind) {
    if (!vp) return;
    const bool jump = geombounds::getField<bool>(*vp, "jump", true);
    const bool retain = geombounds::getField<bool>(*vp, "retainUserOffsets", false);

    // 1. Decide the final offset (BIND-04/07/08 + BIND-09).
    if (kind == BindTransition::Pop) {
      // §23.3.1 r6.3: a pop RESTORES the popped-to vp's stored offset.
      // The offset store already holds it — leave it alone (no-op here).
    } else if (!jump) {
      if (haveCam_) driveCameraTo(ctx, vp, lastCam_); // continuous
    } else if (!retain) {
      ctx.setViewpointOffset(vp, ViewpointOffset{}); // snap to authored pose
    } // retain=TRUE: keep stored offset

    // 2. BIND-05: animate the effective camera (unchanged — a pop still animates).
    const Mat4 targetCam = ctx.viewMatrix().inverse();
    SFNode niField = geombounds::getField<SFNode>(*vp, "navigationInfo", nullptr);
    X3DNode *nav = niField ? niField.get() : ctx.boundNavigationInfo();
    const double dur = nav ? geombounds::getField<double>(*nav, "transitionTime", 1.0) : 0.0;
    if (haveCam_ && dur > 0.0 && !isTeleport(nav) && camsDiffer(lastCam_, targetCam)) {
      anim_ = Anim{true, vp, nav, lastCam_, targetCam, ctx.now(), dur};
      driveCameraTo(ctx, vp, lastCam_);
    }

    // 3. BIND-02: bind this viewpoint's navigationInfo, if it names one.
    if (niField) ctx.postEvent(niField.get(), "set_bind", std::any(SFBool{true}));
  }
```

**Step 2: Build**

Run: `mise run build`
Expected: clean.

**Step 3: Run the BIND-09 tests — all 3 should now PASS**

Run: `ctest --output-on-failure -R viewpoint_bind_test`
Expected: all 11 cases (8 original + 3 new) pass. If any pre-existing case broke, the Pop branch is too aggressive — re-check that `lastVpTransition_` is reset to `None` after each post-cascade (Step 1a above) so a normal push isn't misread as a pop.

**Step 4: Run the FULL test suite to catch regressions**

Run: `mise run build`
Expected: all ctest cases pass.

### Task 2.4: Commit Phase 2

```bash
git add runtime/events/X3DExecutionContext.hpp runtime/scene/BindingStack.hpp \
        runtime/scene/BindingSystem.hpp runtime/events/ViewpointBindSystem.hpp \
        runtime/events/tests/viewpoint_bind_test.cpp
git commit -m "fix(viewnav): BIND-09 pop restores stored offset (§23.3.1 r6.3)

ViewpointBindSystem previously couldn't distinguish a push (set_bind TRUE)
from a pop (set_bind FALSE / delete) — both change the bound viewpoint
A→B — so on a pop it applied jump/retain to the popped-to B, resetting B's
stored offset to zero. §23.3.1 r6.3 says a pop must RESTORE B's stored
offset.

Adds a BindTransition enum (None/Push/Pop) on BindingStack; BindingSystem
sets it via a TransitionSink callback wired through X3DExecutionContext.
ViewpointBindSystem::onBind branches on Pop to leave the stored offset
alone (no-op). The BIND-05 transition animation still runs on a pop.

Tests:
- test_pop_restores_stored_offset: pop B -> A restores A's stored (0,0,5)
- test_push_still_resets: push A (set_bind TRUE) still resets to (0,0,10)
- test_pop_animates_over_transition_time: pop animates 110 -> 5 over 2s

Closes: BIND-09."
```

---

# Phase 3: NAV-FLY-ROLL yaw/pitch decompose

**Files:**
- Modify: `runtime/events/NavigationSystem.hpp:265-294` (rewrite `flyUpdate` drag block), `:385-402` (add new state members), `:127-141` (clear flag on mode switch)
- Modify: `runtime/events/tests/navigation_test.cpp` (4 new test cases)

### Task 3.1: Write the 4 failing FLY-roll tests

**Step 1: Append tests to `navigation_test.cpp`**

Insert before `if (failures)` block, after the LOOKAT-SCALE test from Phase 1.

```cpp
  // --- (7) FLY: no roll after a closed-loop mixed drag (NAV-FLY-ROLL) -------
  // The bug: incremental q = Ryaw·Rpitch·q drifts roll on long mixed drags.
  // Fix: decompose to yaw/pitch scalars, reconstruct q each step (zero roll).
  // Assert: after a closed yaw+pitch loop, the world-up vector (rotated by q)
  // has x and z components < 1e-4 (i.e. no horizon tilt).
  {
    auto w = makeWorld({NavigationTypeValues::FLY}, {0,0,10});
    w->ctx.setPointerPresent(true);
    w->ctx.setPointer(Ray{{0,0,0},{0,0,-1}});
    w->ctx.setPointerButton(true);

    // Sequence: yaw +90°, pitch +30°, yaw -90°, pitch -30° (closed loop).
    // kRotScale = pi rad per unit; +90° = +0.5 units; +30° = +1/6 units.
    float yaw90   = 0.5f;       //  +pi/2
    float pitch30 = 1.0f / 6.0f; //  +pi/6
    struct Step { float dx, dy; };
    Step steps[] = {
      { yaw90,   0.0f   },
      { 0.0f,    pitch30},
      {-yaw90,   0.0f   },
      { 0.0f,   -pitch30},
    };
    w->ctx.tick(0.0);
    for (Step s : steps) {
      w->ctx.setPointer(Ray{{s.dx, s.dy, 0},{0,0,-1}});
      w->ctx.tick(0.016);
    }

    // Effective orientation's rotated up vector: q * (0,1,0) * q^-1.
    SFVec3f fwd = camFwd(w->ctx);
    SFVec3f up{0,1,0};
    // Right = fwd x up (the camera's local X in world).
    SFVec3f right{fwd.z*1 - 0, 0, -(fwd.x*1 - 0)}; // cross(fwd, up) simplified
    // Simpler: read the world-up vector as rotated by the effective orientation.
    // The view matrix's up column (column 1) is the camera's local +Y in world.
    Mat4 v = w->ctx.viewMatrix().inverse(); // camera->world
    SFVec3f camUpWorld{v.m[4], v.m[5], v.m[6]};
    // camUpWorld should equal (0,1,0) within 1e-4 (no roll).
    check(std::fabs(camUpWorld.x) < 1e-4f && std::fabs(camUpWorld.z) < 1e-4f,
          "fly-roll: no horizon roll after closed mixed drag "
          "(up.x=" + std::to_string(camUpWorld.x) +
          ", up.z=" + std::to_string(camUpWorld.z) + ")");
  }

  // --- (8) FLY: pitch clamps at vertical (no flip past pi/2) ----------------
  {
    auto w = makeWorld({NavigationTypeValues::FLY}, {0,0,10});
    w->ctx.setPointerPresent(true);
    w->ctx.setPointer(Ray{{0,0,0},{0,0,-1}});
    w->ctx.setPointerButton(true);
    w->ctx.tick(0.0);
    // Drag way past +90° pitch (1 unit = pi rad; +2 units = 2pi rad of pitch).
    w->ctx.setPointer(Ray{{0, 2.0f, 0},{0,0,-1}});
    w->ctx.tick(0.016);
    SFVec3f fwd = camFwd(w->ctx);
    // forward.y clamped to ~1 (looking straight up), not flipped to -1.
    check(fwd.y > 0.99f, "fly-roll: pitch clamps at +pi/2 (forward.y ~ 1, no flip)");
  }

  // --- (9) FLY: mode switch re-decomposes (EXAMINE-induced roll discarded) --
  {
    // Start in EXAMINE, induce a non-zero roll by orbiting with a tilted setup,
    // then switch to FLY. The first FLY frame must have zero roll.
    // Easiest reliable way: start in FLY, induce roll with the OLD impl (we
    // can't easily induce roll in EXAMINE). Instead, verify that on a fresh
    // FLY entry, the decompose matches the inherited orientation's yaw/pitch
    // with zero roll. So: bind a Viewpoint with a tilted (non-zero-roll)
    // orientation, switch to FLY, and assert zero roll on first drag.
    auto vp = createX3DNode("Viewpoint");
    setF(vp, "position", std::any(SFVec3f{0,0,10}));
    // Tilted orientation: 30° roll about -Z (a roll the FLY decompose must
    // discard).
    setF(vp, "orientation", std::any(SFRotation{0, 0, 1, 0.5236f})); // ~30° roll
    auto nav = createX3DNode("NavigationInfo");
    dynamic_cast<NavigationInfo &>(*nav).setType({NavigationTypeValues::FLY});

    Scene scene; scene.addRootNode(vp); scene.addRootNode(nav);
    X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
    ctx.addSystem(std::make_shared<NavigationSystem>());
    ctx.tick(0.0);

    // First FLY drag — the decompose should derive yaw/pitch from the tilted
    // orientation, discarding roll.
    ctx.setPointerPresent(true);
    ctx.setPointer(Ray{{0,0,0},{0,0,-1}});
    ctx.setPointerButton(true);
    ctx.tick(1.0);
    ctx.setPointer(Ray{{0.1f, 0, 0},{0,0,-1}}); // small yaw drag
    ctx.tick(1.016);

    Mat4 v = ctx.viewMatrix().inverse();
    SFVec3f camUpWorld{v.m[4], v.m[5], v.m[6]};
    check(std::fabs(camUpWorld.x) < 1e-3f && std::fabs(camUpWorld.z) < 1e-3f,
          "fly-roll: mode-entry re-decomposes (zero roll after tilted bind)");
  }

  // --- (10) FLY: viewpoint bind re-decomposes (no stale roll carry-over) ----
  {
    auto A = std::make_shared<Viewpoint>(); A->setPosition(SFVec3f{0,0,10});
    auto B = std::make_shared<Viewpoint>(); B->setPosition(SFVec3f{0,0,10});
    // B has a rolled orientation.
    B->setOrientation(SFRotation{0, 0, 1, 0.5236f});
    auto nav = createX3DNode("NavigationInfo");
    dynamic_cast<NavigationInfo &>(*nav).setType({NavigationTypeValues::FLY});
    Scene scene; scene.addRootNode(A); scene.addRootNode(B); scene.addRootNode(nav);
    X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
    attachViewpointBind(ctx);
    ctx.addSystem(std::make_shared<NavigationSystem>());
    ctx.tick(0.0);

    // Drag in FLY on A.
    ctx.setPointerPresent(true);
    ctx.setPointer(Ray{{0,0,0},{0,0,-1}});
    ctx.setPointerButton(true);
    ctx.tick(1.0);
    ctx.setPointer(Ray{{0.3f, 0, 0},{0,0,-1}});
    ctx.tick(1.016);

    // Bind B (rolled). Then drag in FLY again — should re-decompose from B.
    ctx.postEvent(B.get(), "set_bind", std::any(SFBool{true}));
    ctx.tick(2.0);
    ctx.setPointer(Ray{{0,0,0},{0,0,-1}});
    ctx.tick(2.016);
    ctx.setPointer(Ray{{0.2f, 0, 0},{0,0,-1}});
    ctx.tick(2.032);

    Mat4 v = ctx.viewMatrix().inverse();
    SFVec3f camUpWorld{v.m[4], v.m[5], v.m[6]};
    check(std::fabs(camUpWorld.x) < 1e-3f && std::fabs(camUpWorld.z) < 1e-3f,
          "fly-roll: bind re-decomposes (zero roll after binding rolled vp)");
  }
```

Note: `attachViewpointBind` is declared in `ViewpointBindSystem.hpp`. The existing `navigation_test.cpp` does NOT include it (it doesn't test bind). For test (10) only, add `#include "ViewpointBindSystem.hpp"` at the top of `navigation_test.cpp` alongside the other includes. If test (9) and (10) cause linker errors, the include is missing.

**Step 2: Build + run; verify the 4 new tests FAIL**

Run: `mise run build`
Expected: builds clean.

Run: `ctest --output-on-failure -R navigation_test`
Expected: 4 new checks FAIL (the existing 5 + LOOKAT-SCALE from Phase 1 still pass). Specifically:
- Test (7): `no horizon roll after closed mixed drag` — FAILS (incremental q accumulates roll).
- Test (8): `pitch clamps at +pi/2` — MIGHT PASS or FAIL depending on whether the old impl flips. Either way, the new impl will make it pass cleanly.
- Test (9): `mode-entry re-decomposes` — FAILS (old impl accumulates from the tilted orientation, preserving roll).
- Test (10): `bind re-decomposes` — FAILS (old impl carries roll across the bind).

### Task 3.2: Implement the yaw/pitch decompose

**Step 1: Add new state members to `NavigationSystem`**

In `runtime/events/NavigationSystem.hpp`, in the `private` section's cross-tick state block (around line 385-391, near `lookat_`), add:

```cpp
  // NAV-FLY-ROLL: FLY orientation is held as yaw/pitch scalars (about world-up
  // and local-right-after-yaw), reconstructed to a quaternion each step. This
  // is roll-free by construction (§23.4.4 FLY = yaw + pitch only). The scalars
  // are re-derived from the effective orientation whenever:
  //   - the FLY mode is entered (flyOrientValid_ == false),
  //   - the bound viewpoint changes (lastFlyVp_ != ctx.boundViewpoint()).
  float flyYaw_ = 0.0f;
  float flyPitch_ = 0.0f;
  bool flyOrientValid_ = false;
  X3DNode *lastFlyVp_ = nullptr;
```

**Step 2: Add the decompose helper**

Add as a private static helper near the other orientation helpers (e.g. after `lookRotation`, around line 212):

```cpp
  // NAV-FLY-ROLL: decompose an orientation into yaw (about world-up) and pitch
  // (about local-right-after-yaw), discarding roll. Sign convention matches
  // Ryaw = axisAngleQuat(up, -dx·kRotScale) so yaw increases as the camera
  // turns to its left.
  static void decomposeLookRotation(SFRotation ori, SFVec3f up,
                                    float &yaw, float &pitch) {
    Quat q = quatFromRotation(ori);
    SFVec3f forward = norm(rotateByQuat(q, SFVec3f{0, 0, -1}));
    // Yaw: heading in the world XZ plane (atan2 of -x, -z so yaw=0 -> looking
    // down -Z, yaw=+pi/2 -> looking down -X).
    yaw = std::atan2(-forward.x, -forward.z);
    // Pitch: positive when looking up. Clamp to asin's domain.
    float yClamped = forward.y;
    if (yClamped > 1.0f) yClamped = 1.0f;
    if (yClamped < -1.0f) yClamped = -1.0f;
    pitch = std::asin(yClamped);
  }
```

**Step 3: Rewrite `flyUpdate` (lines 265-294)**

Replace the entire `flyUpdate` method with:

```cpp
  // ---- FLY (free flight) ----------------------------------------------------
  void flyUpdate(X3DExecutionContext &ctx, X3DNode *vp, NavigationInfo *nav,
                 float dx, float dy, bool dragging, double dt) {
    const SFVec3f up{0,1,0};

    // NAV-FLY-ROLL: re-decompose from the effective orientation on mode entry
    // or viewpoint bind change (so we don't snap on entry, and don't carry
    // stale yaw/pitch across a bind).
    if (!flyOrientValid_ || lastFlyVp_ != vp) {
      decomposeLookRotation(effOri(ctx, vp), up, flyYaw_, flyPitch_);
      flyOrientValid_ = true;
      lastFlyVp_ = vp;
    }

    // Drag -> accumulate yaw/pitch scalars (NOT the quaternion).
    if (dragging && (dx != 0.0f || dy != 0.0f)) {
      flyYaw_   += -dx * kRotScale;
      flyPitch_ += -dy * kRotScale;
      // Clamp pitch to ±(pi/2 - eps) so we don't flip past vertical.
      const float maxPitch = kPi * 0.5f - 1e-3f;
      if (flyPitch_ >  maxPitch) flyPitch_ =  maxPitch;
      if (flyPitch_ < -maxPitch) flyPitch_ = -maxPitch;
    }

    // Reconstruct orientation: yaw about world-up, then pitch about local right.
    Quat Ryaw = axisAngleQuat(up, flyYaw_);
    SFVec3f right = norm(rotateByQuat(Ryaw, SFVec3f{1,0,0}));
    Quat Rpitch = axisAngleQuat(right, flyPitch_);
    Quat q = quatMul(Rpitch, Ryaw);

    // Keys -> translate along view dir / strafe, scaled by speed*dt (§23.4.4).
    SFVec3f P = effPos(ctx, vp);
    bool moved = false;
    float speed = nav ? nav->getSpeed() : 1.0f;
    const KeyState &ks = ctx.keyState();
    float fwdIn = (ks.isHeld(kKeyForward) ? 1.0f : 0.0f) - (ks.isHeld(kKeyBack) ? 1.0f : 0.0f);
    float strIn = (ks.isHeld(kKeyRight) ? 1.0f : 0.0f) - (ks.isHeld(kKeyLeft) ? 1.0f : 0.0f);
    if ((fwdIn != 0.0f || strIn != 0.0f) && speed > 0.0f && dt > 0.0) {
      SFVec3f fwd = norm(rotateByQuat(q, SFVec3f{0,0,-1}));
      SFVec3f rightV = norm(rotateByQuat(q, SFVec3f{1,0,0}));
      SFVec3f step = add(mul(fwd, fwdIn), mul(rightV, strIn));
      P = add(P, mul(step, speed * static_cast<float>(dt)));
      moved = true;
    }
    if (dragging && (dx != 0.0f || dy != 0.0f)) moved = true;
    if (moved) commitEye(ctx, vp, P, rotationFromQuat(q));
  }
```

Note: `rotationFromQuat` is from `Interpolation.hpp` (already included). Check the existing `flyUpdate` for whether `rotationFromQuat(q)` or `rotationFromQuat(q)` is the right name — the original code at line 293 uses `rotationFromQuat(q)`, so it's available.

**Step 4: Clear `flyOrientValid_` on mode switch**

In `update` (around line 99-115, the `switch (mode)` block), we need to detect "previous mode was not Fly, current mode is Fly" and clear the flag. Add a `lastMode_` member.

In the cross-tick state block (where you added `flyYaw_` etc. in Step 1), also add:

```cpp
  Mode lastMode_ = Mode::None;
```

Then in `update`, before the `switch (mode)`, add:

```cpp
    if (lastMode_ != Mode::Fly && mode == Mode::Fly) flyOrientValid_ = false;
    lastMode_ = mode;
```

Place this right after `const Mode mode = resolveMode(nav);` (line 77).

**Step 5: Build**

Run: `mise run build`
Expected: clean. If `rotationFromQuat` is the wrong name, check `Interpolation.hpp` for the exact signature.

**Step 6: Run the FLY-roll tests**

Run: `ctest --output-on-failure -R navigation_test`
Expected: all 10 navigation checks pass (5 original + LOOKAT-SCALE + 4 new FLY-roll).

If test (7) still fails with a small nonzero `up.x`/`up.z`, the decompose sign convention may be off. Re-check `atan2(-forward.x, -forward.z)` against `Ryaw = axisAngleQuat(up, flyYaw_)` — for `flyYaw_ = +pi/2`, the camera should look down `-X`, so `forward = (-1, 0, 0)`, and `atan2(-(-1), -0) = atan2(1, 0) = pi/2`. ✓

**Step 7: Run the full test suite**

Run: `mise run build`
Expected: all ctest cases pass.

### Task 3.3: Commit Phase 3

```bash
git add runtime/events/NavigationSystem.hpp runtime/events/tests/navigation_test.cpp
git commit -m "fix(nav): NAV-FLY-ROLL — decompose to yaw/pitch, reconstruct roll-free

flyUpdate previously accumulated the orientation incrementally:
  q = Ryaw·Rpitch·q
where Rpitch was about the *current* local right (already rotated by q),
so cross-axis coupling drifted roll into q over long mixed drags.

The fix holds FLY orientation as two scalars (yaw about world-up, pitch
about local-right-after-yaw) and reconstructs q each step:
  q = Rpitch · Ryaw
This is roll-free by construction (§23.4.4: FLY is yaw + pitch only).

The scalars are re-derived from the effective orientation on:
  - FLY mode entry (lastMode_ != Fly)
  - viewpoint bind change (lastFlyVp_ != vp)
so a rolled entry orientation (e.g. a tilted Viewpoint bind) has its roll
discarded rather than preserved, and stale yaw/pitch don't carry across
binds.

Pitch is clamped to ±(pi/2 - eps) to prevent flipping past vertical.

Tests:
- no roll after closed-loop mixed drag (up.x, up.z < 1e-4)
- pitch clamps at +pi/2 (forward.y ~ 1, no flip)
- mode-entry re-decomposes (tilted bind -> zero roll on first drag)
- bind re-decomposes (rolled vp bind -> zero roll on next drag)

Closes: NAV-FLY-ROLL."
```

---

# Phase 4: Conformance closure

### Task 4.1: Flip the three findings in `findings.yaml`

**Step 1: Read the current state of the three finding rows**

Run: `grep -n -A 6 "BIND-09\|NAV-LOOKAT-SCALE\|NAV-FLY-ROLL" docs/conformance/findings.yaml`
Expected: shows all three at `status: deferred` (around lines 978-999).

**Step 2: Edit each finding's `status` + add closure metadata**

For **BIND-09** (around line 983), change:
```yaml
  status: deferred
```
to:
```yaml
  status: closed
  closed_in: <BIND-09 commit SHA from Phase 2>
  closed_by: BindTransition side-channel (BindingStack enum) + ViewpointBindSystem Pop branch restores stored offset (§23.3.1 r6.3).
```

For **NAV-FLY-ROLL** (around line 997), same pattern with the Phase 3 SHA:
```yaml
  status: closed
  closed_in: <NAV-FLY-ROLL commit SHA from Phase 3>
  closed_by: FLY orientation held as yaw/pitch scalars, reconstructed roll-free each step (§23.4.4).
```

For **NAV-LOOKAT-SCALE** (around line 990), the closure depends on Phase 1's outcome:
- If Phase 1 test PASSED (finding invalid):
```yaml
  status: invalid
  closed_in: <NAV-LOOKAT-SCALE verification commit SHA from Phase 1>
  closed_by: Verification test (runtime/events/tests/navigation_test.cpp::x3d_navigation_lookat_scale) confirms world camera distance == radius/tan(fov/2) under non-uniform ancestor scale; the world->local round-trip is lossless. Audit's "world-space radius mixed with local-frame eye" claim does not reproduce.
```
- If Phase 1 test FAILED and you fixed it:
```yaml
  status: closed
  closed_in: <fix commit SHA>
  closed_by: <one-line description of the fix>
```

Get the actual SHAs with `git log --oneline -3` (most recent three commits).

### Task 4.2: Regenerate the conformance view

**Step 1: Regenerate**

Run: `mise run conformance`
Expected: regenerates `docs/conformance/model.json`, `INDEX.md`, `components/*.md` from the updated `findings.yaml`. The three findings should now show `closed`/`invalid` in the rendered view.

**Step 2: Verify the diff**

Run: `git diff docs/conformance/`
Expected: the three findings' rows changed; the Sound audit changes ( SND-1..SND-9) are also present (they were uncommitted from before — that's fine).

Spot-check `docs/conformance/components/Navigation.md` — the BIND-09, NAV-LOOKAT-SCALE, NAV-FLY-ROLL rows should reflect the new status.

### Task 4.3: Update the wiki page (BIND-09 only)

**Step 1: Read the current wiki page**

Run: `ls docs/wiki/subsystems/system-viewpointbind.md`
If it exists, read it. If not, skip this task — the wiki page doesn't exist yet (separate workstream).

**Step 2: Add a one-paragraph addendum on the pop-restore behavior**

Append to the appropriate section (likely "Bind semantics" or similar):

```markdown
### Pop (set_bind FALSE / delete) — §23.3.1 r6.3

A pop (set_bind FALSE on the bound viewpoint, or deletion of the bound
viewpoint) restores the popped-to viewpoint's stored user offset — it does
NOT apply that viewpoint's `jump`/`retainUserOffsets` (those govern push
binds only). The transition still animates over the bound NavigationInfo's
`transitionTime`. This is signal'd via a `BindTransition` side-channel
(`Push`/`Pop`/`None`) on `X3DExecutionContext`, set by `BindingSystem` and
consumed by `ViewpointBindSystem::onBind`.
```

### Task 4.4: Commit Phase 4

**Step 1: Stage everything conformance-related + the wiki**

```bash
git add docs/conformance/findings.yaml \
        docs/conformance/model.json \
        docs/conformance/INDEX.md \
        docs/conformance/components/Navigation.md \
        docs/conformance/components/Geospatial.md \
        docs/wiki/subsystems/system-viewpointbind.md
```

Note: the Sound audit files (`components/Sound.md`, `components/Texturing.md`, and the SND-* rows in `findings.yaml`) are also staged here — they were uncommitted from a separate workstream. That's acceptable: the regen preserved them. If you'd rather not commit them, unstage them with `git restore --staged docs/conformance/components/Sound.md docs/conformance/components/Texturing.md` and the corresponding `findings.yaml` hunks (use `git add -p` to stage only the BIND-09/NAV-* hunks). **Recommended:** keep it simple and commit the whole regen; the Sound audit was already in the tree.

**Step 2: Commit**

```bash
git commit -m "docs(conformance): close BIND-09, NAV-LOOKAT-SCALE, NAV-FLY-ROLL

BIND-09: closed — pop restores stored offset (§23.3.1 r6.3).
NAV-LOOKAT-SCALE: <invalid | closed> — <one-line reason>.
NAV-FLY-ROLL: closed — FLY yaw/pitch decompose (§23.4.4).

Regenerates the conformance view from findings.yaml. Also picks up the
uncommitted Sound audit (SND-1..SND-9) from the prior workstream.

Wiki: docs/wiki/subsystems/system-viewpointbind.md addendum on the
pop-restore behavior."
```

Fill in the NAV-LOOKAT-SCALE line based on Phase 1's outcome.

---

# Final verification

**Step 1: Full build + test**

Run: `mise run build`
Expected: all ctest cases pass, no regressions.

**Step 2: Conformance gate**

Run: `mise run conformance-gate`
Expected: passes (findings.yaml schema valid; no drift vs the generated view).

**Step 3: Clean working tree**

Run: `git status --short`
Expected: empty (or only files unrelated to this plan).

**Step 4: Confirm the three findings are closed/invalid**

Run: `grep -A 3 "BIND-09\|NAV-LOOKAT-SCALE\|NAV-FLY-ROLL" docs/conformance/findings.yaml | grep status`
Expected:
- `status: closed` (BIND-09)
- `status: invalid` OR `status: closed` (NAV-LOOKAT-SCALE, depending on Phase 1)
- `status: closed` (NAV-FLY-ROLL)

**Step 5: Confirm commits**

Run: `git log --oneline -6`
Expected: the four phase commits (P1 verification, P2 BIND-09, P3 NAV-FLY-ROLL, P4 conformance closure) on top of `4dafa72` (the design doc).

---

## Reference: Skills to invoke during execution

- @superpowers:executing-plans — task-by-task execution with checkpoints.
- @superpowers:test-driven-development — every fix is test-first.
- @superpowers:systematic-debugging — if a test fails unexpectedly, before patching code.
- @superpowers:verification-before-completion — run the Final verification block above before claiming done.
