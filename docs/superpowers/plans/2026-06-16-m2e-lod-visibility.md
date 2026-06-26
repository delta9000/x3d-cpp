# M2e — LOD / Visibility Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the view-dependent scene-graph runtime layer — LOD distance selection, Billboard view-facing rotation, ProximitySensor/VisibilitySensor, and visibility culling — driven by the bound Viewpoint.

**Architecture:** One `ViewDependentSystem` (run each `tick()`) owns the event-emitting logic (LOD `level_changed`, sensor enter/exit). Render-time selection (which LOD child, Billboard rotation) plugs into the per-path world accumulation already done by `SceneExtractor::walk` and `PickSystem::worldOfRec` (M2C-1 idiom — never the static `TransformSystem.world_` table). Runtime-only; the generated layer and golden output are untouched.

**Tech Stack:** C++20, header-only runtime, `runtime/math/Mat4`, reflection via `geombounds::getField`, ctest + `mise run build`, golden gate via `mise run golden`. Spec: ISO/IEC 19775-1 §23.4.3 (LOD), §23.4.1 (Billboard), §22.4.1/§22.4.3 (sensors), §23.4.4 (visibilityLimit). Full design: `docs/superpowers/specs/2026-06-16-m2e-lod-visibility-design.md`.

**Conventions (all tasks):**
- Tests are standalone `.cpp` with `int main()` returning 0, `assert`-based, registered in the **root** `CMakeLists.txt` via `add_test`. Build/run: `mise run build` (configures, builds, runs all ctest). Target one test in output by grepping its name. Do NOT pass a high `-j`.
- After each task: `mise run build` (all green) AND `mise run golden` (byte-identical — these changes are runtime-only). If golden drifts, you touched codegen — stop.
- Float compare helper used in tests: `static bool feq(float a,float b){return std::fabs(a-b)<1e-4f;}`.
- Field reads: `geombounds::getField<T>(node, "name", dflt)` (declared `runtime/scene/GeometryBounds.hpp:17`).

**File structure:**
- **Create:** `runtime/scene/ViewDependentSystem.hpp` — camera pose, LOD level tracking + `level_changed`, Proximity/VisibilitySensor evaluation, `billboardLocalMatrix` + view-volume (cone/frustum) helpers. One responsibility: viewer-dependent evaluation.
- **Create:** `runtime/scene/tests/view_dependent_test.cpp` — all M2e unit/integration tests.
- **Modify:** `runtime/events/X3DExecutionContext.hpp` — `cameraWorldPosition()`/`cameraWorldUp()` helpers; register `ViewDependentSystem` in `buildSceneGraph`.
- **Modify:** `runtime/extract/SceneExtractor.hpp` — Billboard rotation + LOD local-frame selection in `walk`; `visible=FALSE` skip; `beyondVisibilityLimit` tag in `emit`; `setViewVolume` seam.
- **Modify:** `runtime/scene/PickSystem.hpp` — Billboard rotation in `worldOfRec`.
- **Modify:** `runtime/extract/RenderItem.hpp` — `beyondVisibilityLimit` flag on the consumer mesh/item surface (engine `RenderItem` in SceneExtractor.hpp gains it too).
- **Modify:** root `CMakeLists.txt` — register `x3d_view_dependent`.

---

## Task 1: ViewDependentSystem skeleton + camera pose

**Files:**
- Create: `runtime/scene/ViewDependentSystem.hpp`
- Modify: `runtime/events/X3DExecutionContext.hpp` (add camera helpers + register system in `buildSceneGraph`)
- Create: `runtime/scene/tests/view_dependent_test.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add camera-pose helpers to X3DExecutionContext**

In `runtime/events/X3DExecutionContext.hpp`, immediately after `viewMatrix()` (ends ~line 176), add:

```cpp
  /// Viewer world-space position = camera-to-world translation (M2e).
  SFVec3f cameraWorldPosition() const {
    return viewMatrix().inverse().transformPoint(SFVec3f{0, 0, 0});
  }
  /// Viewer world-space up vector (+Y of the camera frame) (M2e).
  SFVec3f cameraWorldUp() const {
    return viewMatrix().inverse().transformDirection(SFVec3f{0, 1, 0});
  }
```

- [ ] **Step 2: Create the ViewDependentSystem skeleton**

Create `runtime/scene/ViewDependentSystem.hpp`:

```cpp
// ViewDependentSystem.hpp — M2e: viewer-dependent scene-graph evaluation.
// Run each tick() from the bound Viewpoint. Owns LOD level_changed tracking and
// ProximitySensor/VisibilitySensor enter/exit. Render-time selection (LOD child,
// Billboard rotation) lives in the per-path walk of SceneExtractor/PickSystem and
// uses the free helpers at the bottom of this header.
#ifndef X3D_RUNTIME_VIEW_DEPENDENT_SYSTEM_HPP
#define X3D_RUNTIME_VIEW_DEPENDENT_SYSTEM_HPP

#include "GeometryBounds.hpp"
#include "Mat4.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"

#include <cmath>
#include <string>
#include <unordered_map>

namespace x3d::runtime {

class ViewDependentSystem : public System {
public:
  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    (void)ctx;
    const std::string t = node ? node->nodeTypeName() : "";
    if (t == "LOD") lodLevel_.emplace(node, -1);
    else if (t == "ProximitySensor" || t == "VisibilitySensor")
      sensorActive_.emplace(node, false);
  }

  void update(double now, X3DExecutionContext &ctx) override {
    (void)now; (void)ctx;
    // Filled in by Tasks 4-6.
  }

private:
  std::unordered_map<X3DNode *, int> lodLevel_;      // last announced level
  std::unordered_map<X3DNode *, bool> sensorActive_; // last isActive
};

} // namespace x3d::runtime
#endif
```

- [ ] **Step 3: Register the system in buildSceneGraph**

In `runtime/events/X3DExecutionContext.hpp`, the `buildSceneGraph(Scene&)` body (~lines 59-69) currently builds transforms/bounds/bindings/pick and sets the field observer. This file cannot include `ViewDependentSystem.hpp` (that header includes this one — cycle). Instead, register the system from the caller. Add a public hook used by tests and the bridge — append after `cascade_.setFieldObserver(...)` line, leave `buildSceneGraph` as-is, and in the **test** (Step 4) construct + `addSystem(std::make_shared<ViewDependentSystem>())` then call `attach` per node. (Production wiring is Task 8.)

No code change here beyond Step 1; this step is a note. Proceed.

- [ ] **Step 4: Write the failing test (camera pose)**

Create `runtime/scene/tests/view_dependent_test.cpp`:

```cpp
// view_dependent_test.cpp — M2e: LOD / Billboard / sensors / visibility.
#include "ViewDependentSystem.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp" // out-of-line Scene::addRootNode
#include <any>
#include <cassert>
#include <cmath>
#include <memory>
using namespace x3d::runtime;

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }
static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields()) if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}

static void testCameraPose() {
  auto vp = createX3DNode("Viewpoint");
  setF(vp, "position", std::any(SFVec3f{1, 2, 3}));
  Scene scene; scene.addRootNode(vp);
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);     // binds the Viewpoint by default
  SFVec3f eye = ctx.cameraWorldPosition();
  assert(feq(eye.x, 1) && feq(eye.y, 2) && feq(eye.z, 3));
}

int main() {
  testCameraPose();
  return 0;
}
```

- [ ] **Step 5: Register the test in CMakeLists.txt**

In root `CMakeLists.txt`, find the block registering scene tests (search `x3d_dirty_tracker` or `x3d_transform_hanim_cadpart`) and add a sibling entry following the exact same `add_executable` + `target_link_libraries` + `add_test` pattern used there, naming the target `x3d_view_dependent` and source `runtime/scene/tests/view_dependent_test.cpp`.

- [ ] **Step 6: Run — verify it builds and passes**

Run: `mise run build 2>&1 | tail -5`
Expected: `100% tests passed` including `x3d_view_dependent`. (This test exercises only existing APIs + the Step-1 helpers, so it should pass once it compiles.) Then `mise run golden 2>&1 | tail -1` → byte-identical.

- [ ] **Step 7: Commit**

```bash
git add runtime/scene/ViewDependentSystem.hpp runtime/events/X3DExecutionContext.hpp runtime/scene/tests/view_dependent_test.cpp CMakeLists.txt
git commit -m "m2e: ViewDependentSystem skeleton + camera pose helpers"
```

---

## Task 2: Billboard rotation helper (pure math)

**Files:**
- Modify: `runtime/scene/ViewDependentSystem.hpp` (add free function `billboardLocalMatrix`)
- Modify: `runtime/scene/tests/view_dependent_test.cpp`

- [ ] **Step 1: Write the failing test**

Add to `view_dependent_test.cpp` before `main()`:

```cpp
// Billboard, axisOfRotation=(0,1,0): a camera on +X must rotate the billboard's
// local +Z to point toward the camera within the XZ plane.
static void testBillboardAxis() {
  Mat4 parent = Mat4::identity();
  SFVec3f cam{10, 0, 0};                 // viewer on +X
  Mat4 r = billboardLocalMatrix(parent, cam, SFVec3f{0, 1, 0}, SFVec3f{0, 1, 0});
  SFVec3f z = r.transformDirection(SFVec3f{0, 0, 1}); // rotated local +Z
  // +Z should now point toward +X (the viewer), i.e. ~ (1,0,0).
  assert(feq(z.x, 1) && feq(z.y, 0) && feq(z.z, 0));
}

// Billboard, axisOfRotation=(0,0,0): viewer-alignment. Camera on +Z keeps +Z
// pointing at the viewer and +Y aligned to the viewer up.
static void testBillboardViewerAlign() {
  Mat4 parent = Mat4::identity();
  SFVec3f cam{0, 0, 5};
  Mat4 r = billboardLocalMatrix(parent, cam, SFVec3f{0, 1, 0}, SFVec3f{0, 0, 0});
  SFVec3f z = r.transformDirection(SFVec3f{0, 0, 1});
  SFVec3f y = r.transformDirection(SFVec3f{0, 1, 0});
  assert(feq(z.x, 0) && feq(z.y, 0) && feq(z.z, 1)); // +Z toward viewer
  assert(feq(y.x, 0) && feq(y.y, 1) && feq(y.z, 0)); // +Y up
}
```

Add the calls in `main()`: `testBillboardAxis(); testBillboardViewerAlign();`

- [ ] **Step 2: Run to verify it fails**

Run: `mise run build 2>&1 | tail -15`
Expected: compile error — `billboardLocalMatrix` not declared.

- [ ] **Step 3: Implement billboardLocalMatrix**

In `ViewDependentSystem.hpp`, before the closing `} // namespace`, add these free helpers (use small local vec ops; `SFVec3f` is `{x,y,z}` floats):

```cpp
namespace viewdep {
inline SFVec3f sub(const SFVec3f &a, const SFVec3f &b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
inline float dot(const SFVec3f &a, const SFVec3f &b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline SFVec3f cross(const SFVec3f &a, const SFVec3f &b) {
  return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
}
inline float len(const SFVec3f &a) { return std::sqrt(dot(a, a)); }
inline SFVec3f norm(const SFVec3f &a) { float l = len(a); return l < 1e-9f ? SFVec3f{0,0,0} : SFVec3f{a.x/l, a.y/l, a.z/l}; }
} // namespace viewdep

// §23.4.1 Billboard. parentWorldM = the billboard's frame (world of its parent);
// returns the billboard's LOCAL rotation matrix (applied as `worldM * R`).
inline Mat4 billboardLocalMatrix(const Mat4 &parentWorldM, const SFVec3f &cameraWorldPos,
                                 const SFVec3f &viewerUpWorld, const SFVec3f &axisOfRotation) {
  using namespace viewdep;
  const Mat4 inv = parentWorldM.inverse();
  const SFVec3f camLocal = inv.transformPoint(cameraWorldPos);   // viewer in local frame
  const SFVec3f b2v = norm(camLocal);                            // billboard-origin -> viewer
  if (len(b2v) < 1e-9f) return Mat4::identity();                 // viewer at origin: undefined

  const bool viewerAlign = (axisOfRotation.x == 0.0f && axisOfRotation.y == 0.0f && axisOfRotation.z == 0.0f);
  if (!viewerAlign) {
    // Rotate local +Z about `axis` into the plane(axis, b2v), as close to b2v as possible.
    const SFVec3f a = norm(axisOfRotation);
    const SFVec3f zL{0, 0, 1};
    SFVec3f zProj = norm(sub(zL, SFVec3f{a.x*dot(zL,a), a.y*dot(zL,a), a.z*dot(zL,a)}));
    SFVec3f bProj = norm(sub(b2v, SFVec3f{a.x*dot(b2v,a), a.y*dot(b2v,a), a.z*dot(b2v,a)}));
    if (len(zProj) < 1e-9f || len(bProj) < 1e-9f) return Mat4::identity(); // axis ∥ b2v: undefined
    const float angle = std::atan2(dot(cross(zProj, bProj), a), dot(zProj, bProj));
    return Mat4::rotation(SFRotation{a.x, a.y, a.z, angle});
  }
  // Viewer-alignment: +Z -> b2v, +Y -> viewer up.
  const SFVec3f upL = norm(inv.transformDirection(viewerUpWorld));
  const SFVec3f newZ = b2v;
  SFVec3f newX = cross(upL, newZ);
  if (len(newX) < 1e-9f) newX = cross(SFVec3f{0, 1, 0}, newZ); // up ∥ b2v fallback
  newX = norm(newX);
  const SFVec3f newY = cross(newZ, newX);
  // Column-major basis: columns are images of local X,Y,Z. Build via Mat4 element layout.
  Mat4 m = Mat4::identity();
  m.m[0]=newX.x; m.m[1]=newX.y; m.m[2]=newX.z;   // column 0 = newX
  m.m[4]=newY.x; m.m[5]=newY.y; m.m[6]=newY.z;   // column 1 = newY
  m.m[8]=newZ.x; m.m[9]=newZ.y; m.m[10]=newZ.z;  // column 2 = newZ
  return m;
}
```

NOTE: confirm `Mat4` stores `float m[16]` column-major (it builds via `transformMatrix` column-major per `Mat4.hpp:108`). If the public member differs, read `Mat4.hpp:16-40` and set the basis through whatever accessor exists; the column semantics (col0=newX, col1=newY, col2=newZ) are the contract.

- [ ] **Step 4: Run to verify it passes**

Run: `mise run build 2>&1 | tail -5` → `x3d_view_dependent` passes. `mise run golden 2>&1 | tail -1` → byte-identical.

- [ ] **Step 5: Commit**

```bash
git add runtime/scene/ViewDependentSystem.hpp runtime/scene/tests/view_dependent_test.cpp
git commit -m "m2e: billboardLocalMatrix view-facing rotation (§23.4.1)"
```

---

## Task 3: Billboard integration into extractor + pick

**Files:**
- Modify: `runtime/extract/SceneExtractor.hpp` (`walk`)
- Modify: `runtime/scene/PickSystem.hpp` (`worldOfRec`)
- Modify: `runtime/scene/tests/view_dependent_test.cpp`

- [ ] **Step 1: Write the failing test (extractor applies billboard)**

Add to `view_dependent_test.cpp`. A Shape under a Billboard; the billboard faces the camera, so the shape's RenderItem world transform rotates local +Z toward the viewer.

```cpp
#include "SceneExtractor.hpp"
// ... (place include with the others at top)

static void testBillboardInExtractor() {
  auto bb = createX3DNode("Billboard");
  setF(bb, "axisOfRotation", std::any(SFVec3f{0, 1, 0}));
  auto shape = createX3DNode("Shape");
  auto box = createX3DNode("Box");
  setF(shape, "geometry", std::any(std::static_pointer_cast<X3DNode>(box)));
  // add shape as a child of bb
  for (auto &f : bb->fields())
    if (f.x3dName == "children" && f.set)
      f.set(*bb, std::any(std::vector<std::shared_ptr<X3DNode>>{shape}));
  auto vp = createX3DNode("Viewpoint");
  setF(vp, "position", std::any(SFVec3f{10, 0, 0})); // viewer on +X
  Scene scene; scene.addRootNode(vp); scene.addRootNode(bb);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  extract::SceneExtractor ex(ctx, scene);
  auto snap = ex.fullSnapshot();
  assert(!snap.added.empty());
  const auto &item = ex.item(snap.added[0]);
  SFVec3f z = item.worldTransform.transformDirection(SFVec3f{0, 0, 1});
  assert(feq(z.x, 1) && feq(z.y, 0) && feq(z.z, 0)); // billboard faces +X viewer
}
```

Add `testBillboardInExtractor();` to `main()`.

- [ ] **Step 2: Run to verify it fails**

Run: `mise run build 2>&1 | tail -15`
Expected: assertion fails (billboard currently pass-through; +Z stays (0,0,1), so `z.x==0`).

- [ ] **Step 3: Apply billboard in SceneExtractor::walk**

In `runtime/extract/SceneExtractor.hpp`, in `walk` (~line 459) the first line computes `here`:

```cpp
  const Mat4 here =
      isTransform(n) ? worldM * TransformSystem::localMatrix(n) : worldM;
```

Replace with a billboard-aware version (include `ViewDependentSystem.hpp` at the top of SceneExtractor.hpp):

```cpp
  Mat4 here = isTransform(n) ? worldM * TransformSystem::localMatrix(n) : worldM;
  if (n->nodeTypeName() == "Billboard") {
    const SFVec3f axis = geombounds::getField<SFVec3f>(*n, "axisOfRotation", {0, 1, 0});
    here = worldM * billboardLocalMatrix(worldM, ctx_.cameraWorldPosition(),
                                         ctx_.cameraWorldUp(), axis);
  }
```

(`ctx_` is the SceneExtractor's stored `X3DExecutionContext&`; confirm its member name by reading the SceneExtractor ctor.)

- [ ] **Step 4: Apply billboard in PickSystem::worldOfRec**

In `runtime/scene/PickSystem.hpp` (`worldOfRec`, ~line 139), the first line:

```cpp
  Mat4 here = isTransform(n) ? worldM * TransformSystem::localMatrix(n) : worldM;
```

PickSystem has no camera; add an optional camera position the pick entry point threads through. Minimal approach: give PickSystem a `SFVec3f cameraPos_` + `SFVec3f cameraUp_` set before a pick, defaulting to no-op when the node isn't a Billboard. Add after the `here` line:

```cpp
  if (n->nodeTypeName() == "Billboard") {
    const SFVec3f axis = geombounds::getField<SFVec3f>(*n, "axisOfRotation", {0, 1, 0});
    here = worldM * billboardLocalMatrix(worldM, cameraPos_, cameraUp_, axis);
  }
```

Add `SFVec3f cameraPos_{0,0,0}; SFVec3f cameraUp_{0,1,0};` members and a `setCamera(const SFVec3f&, const SFVec3f&)` setter; `X3DExecutionContext::pick` sets them from `cameraWorldPosition()/cameraWorldUp()` before calling pick. (Include `ViewDependentSystem.hpp` — beware include cycles; if PickSystem is included by X3DExecutionContext, instead put `billboardLocalMatrix` reachable via a header both can include. `ViewDependentSystem.hpp` includes `X3DExecutionContext.hpp`, so PickSystem including ViewDependentSystem would cycle. RESOLUTION: move `billboardLocalMatrix` + the `viewdep` vec helpers into a new tiny header `runtime/scene/Billboard.hpp` that includes only `Mat4.hpp`; both `ViewDependentSystem.hpp` and `PickSystem.hpp` and `SceneExtractor.hpp` include that. Do this refactor as the first action of this step.)

- [ ] **Step 5: Run to verify it passes**

Run: `mise run build 2>&1 | tail -5` → green incl. `testBillboardInExtractor`. `mise run golden 2>&1 | tail -1` → byte-identical.

- [ ] **Step 6: Commit**

```bash
git add runtime/scene/Billboard.hpp runtime/scene/ViewDependentSystem.hpp runtime/extract/SceneExtractor.hpp runtime/scene/PickSystem.hpp runtime/events/X3DExecutionContext.hpp runtime/scene/tests/view_dependent_test.cpp
git commit -m "m2e: apply Billboard rotation in extractor + pick per-path walk (closes M2C-4 Billboard)"
```

---

## Task 4: LOD distance-based level selection + level_changed

**Files:**
- Modify: `runtime/extract/SceneExtractor.hpp` (`walk` LOD branch)
- Modify: `runtime/scene/ViewDependentSystem.hpp` (`update` LOD tracking + `level_changed`)
- Modify: `runtime/scene/tests/view_dependent_test.cpp`

- [ ] **Step 1: Write the failing test (extractor selects level by distance)**

Add a free helper `lodSelectLevel` and test it + the extractor. Test:

```cpp
static void testLodSelect() {
  // 2 ranges -> 3 levels. center default 0,0,0. ranges [5,10].
  auto lod = createX3DNode("LOD");
  setF(lod, "range", std::any(std::vector<float>{5.0f, 10.0f}));
  // three children: Shapes with Box, Sphere, Cone to tell them apart
  auto mk = [&](const char *g){ auto s=createX3DNode("Shape"); auto geo=createX3DNode(g);
    setF(s,"geometry",std::any(std::static_pointer_cast<X3DNode>(geo))); return s; };
  std::vector<std::shared_ptr<X3DNode>> kids{mk("Box"), mk("Sphere"), mk("Cone")};
  for (auto &f : lod->fields()) if (f.x3dName=="children" && f.set) f.set(*lod, std::any(kids));
  // L(d): d<5 -> level0 (Box); 5<=d<10 -> level1 (Sphere); d>=10 -> level2 (Cone)
  assert(lodSelectLevel(*lod, /*distToCenter=*/3.0f) == 0);
  assert(lodSelectLevel(*lod, 7.0f) == 1);
  assert(lodSelectLevel(*lod, 20.0f) == 2);
  // empty range -> level 0
  auto lod2 = createX3DNode("LOD");
  assert(lodSelectLevel(*lod2, 999.0f) == 0);
}
```

Add `testLodSelect();` to `main()`.

- [ ] **Step 2: Run to verify it fails**

Run: `mise run build 2>&1 | tail -15` → compile error: `lodSelectLevel` not declared.

- [ ] **Step 3: Implement lodSelectLevel + extractor LOD branch**

In `runtime/scene/ViewDependentSystem.hpp` (free function, after `billboardLocalMatrix` or in `Billboard.hpp`'s sibling — put LOD math in `ViewDependentSystem.hpp`):

```cpp
// §23.4.3 LOD step function L(d): level = number of ranges d meets-or-exceeds.
inline int lodSelectLevel(const X3DNode &lod, float distToCenter) {
  const auto range = geombounds::getField<std::vector<float>>(lod, "range", {});
  if (range.empty()) return 0;                 // empty -> browser choice: highest detail
  int level = 0;
  for (float r : range) { if (distToCenter >= r) ++level; else break; }
  return level;
}
```

In `runtime/extract/SceneExtractor.hpp` `walk`, replace the static LOD branch (`if (t == "LOD") { ... kids[0] ... }`, ~line 509) with local-frame distance selection:

```cpp
  if (t == "LOD") {
    const auto kids = childrenOf(*n);
    if (!kids.empty()) {
      const SFVec3f center = geombounds::getField<SFVec3f>(*n, "center", {0, 0, 0});
      // §23.4.3: distance measured in the LOD's LOCAL frame (here includes scale).
      const SFVec3f eyeLocal = here.inverse().transformPoint(ctx_.cameraWorldPosition());
      const float d = viewdep::len(viewdep::sub(eyeLocal, center));
      int lvl = lodSelectLevel(*n, d);
      if (lvl >= static_cast<int>(kids.size())) lvl = static_cast<int>(kids.size()) - 1;
      if (kids[lvl]) walk(kids[lvl].get(), here, path, delta);
    }
    path.pop_back();
    return;
  }
```

- [ ] **Step 4: Run to verify it passes**

Run: `mise run build 2>&1 | tail -5` → green. `mise run golden` → byte-identical.

- [ ] **Step 5: Write the failing test (level_changed event)**

Add a test that drives `ViewDependentSystem::update` and checks a `level_changed` event is posted when the level changes. Use a route sink or the cascade observer. Simplest: register a one-field capture via `ctx.addRoute` to a Script-less sink is heavy; instead assert through a small recording observer. Add:

```cpp
static void testLodLevelChanged() {
  auto lod = createX3DNode("LOD");
  setF(lod, "range", std::any(std::vector<float>{5.0f}));
  auto mk=[&](const char*g){auto s=createX3DNode("Shape");auto x=createX3DNode(g);
    setF(s,"geometry",std::any(std::static_pointer_cast<X3DNode>(x)));return s;};
  std::vector<std::shared_ptr<X3DNode>> kids{mk("Box"), mk("Sphere")};
  for (auto &f : lod->fields()) if (f.x3dName=="children"&&f.set) f.set(*lod,std::any(kids));
  auto vp = createX3DNode("Viewpoint");
  Scene scene; scene.addRootNode(vp); scene.addRootNode(lod);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  auto vds = std::make_shared<ViewDependentSystem>();
  vds->attach(lod.get(), ctx); ctx.addSystem(vds);
  // viewer near (default position 0,0,10 -> d=10 >=5 -> level 1)
  setF(vp, "position", std::any(SFVec3f{0,0,10}));
  int captured = -99;
  vds->setLevelChangedHook([&](X3DNode *n, int lvl){ if (n==lod.get()) captured = lvl; });
  ctx.tick(0.0);
  assert(captured == 1);                 // first eval announces level 1
  captured = -99;
  setF(vp, "position", std::any(SFVec3f{0,0,1})); // d=1 <5 -> level 0
  ctx.tick(1.0);
  assert(captured == 0);                 // level changed -> announced
}
```

Add `testLodLevelChanged();` to `main()`. (The `setLevelChangedHook` test seam avoids wiring a full route; production also `postEvent`s `level_changed`.)

- [ ] **Step 6: Run to verify it fails**

Run: `mise run build 2>&1 | tail -15` → compile error: `setLevelChangedHook` not declared.

- [ ] **Step 7: Implement LOD tracking in ViewDependentSystem::update**

In `ViewDependentSystem.hpp`, add the hook member + LOD evaluation in `update`:

```cpp
public:
  // Test/observer seam: invoked when an LOD's announced level changes.
  void setLevelChangedHook(std::function<void(X3DNode *, int)> h) { levelHook_ = std::move(h); }

  void update(double now, X3DExecutionContext &ctx) override {
    (void)now;
    const SFVec3f eye = ctx.cameraWorldPosition();
    for (auto &[node, last] : lodLevel_) {
      // Per-node convenience level via primary-path world transform (documented
      // per-node-event / per-path-render split, M2C-1). worldTransform(node) is
      // the first-path world matrix; identity if unknown.
      const Mat4 w = ctx.worldTransform(node);
      const SFVec3f center = geombounds::getField<SFVec3f>(*node, "center", {0, 0, 0});
      const SFVec3f eyeLocal = w.inverse().transformPoint(eye);
      const float d = viewdep::len(viewdep::sub(eyeLocal, center));
      const int lvl = lodSelectLevel(*node, d);
      if (lvl != last) {
        last = lvl;
        ctx.postEvent(node, "level_changed", std::any(static_cast<SFInt32>(lvl)));
        if (levelHook_) levelHook_(node, lvl);
      }
    }
    // sensors handled in Tasks 5-6
  }

private:
  std::function<void(X3DNode *, int)> levelHook_;
```

Add `#include <functional>`.

- [ ] **Step 8: Run to verify it passes**

Run: `mise run build 2>&1 | tail -5` → green. `mise run golden` → byte-identical.

- [ ] **Step 9: Commit**

```bash
git add runtime/scene/ViewDependentSystem.hpp runtime/extract/SceneExtractor.hpp runtime/scene/tests/view_dependent_test.cpp
git commit -m "m2e: LOD distance-based level selection (local frame) + level_changed (§23.4.3)"
```

---

## Task 5: ProximitySensor

**Files:**
- Modify: `runtime/scene/ViewDependentSystem.hpp`
- Modify: `runtime/scene/tests/view_dependent_test.cpp`

- [ ] **Step 1: Write the failing test**

```cpp
static void testProximitySensor() {
  auto ps = createX3DNode("ProximitySensor");
  setF(ps, "size", std::any(SFVec3f{4, 4, 4})); // box [-2,2]^3 around origin
  auto vp = createX3DNode("Viewpoint");
  Scene scene; scene.addRootNode(vp); scene.addRootNode(ps);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  auto vds = std::make_shared<ViewDependentSystem>();
  vds->attach(ps.get(), ctx); ctx.addSystem(vds);
  bool active = false; double enter = -1, exit = -1;
  vds->setSensorHook([&](X3DNode *n, bool a, double t){ if (n==ps.get()){active=a; if(a)enter=t; else exit=t;} });
  setF(vp, "position", std::any(SFVec3f{0, 0, 1})); // inside box
  ctx.tick(2.0);
  assert(active && feq((float)enter, 2.0f));        // entered at t=2
  setF(vp, "position", std::any(SFVec3f{0, 0, 100})); // outside
  ctx.tick(5.0);
  assert(!active && feq((float)exit, 5.0f));        // exited at t=5
}
```

Add `testProximitySensor();` to `main()`.

- [ ] **Step 2: Run to verify it fails**

Run: `mise run build 2>&1 | tail -15` → compile error: `setSensorHook` not declared.

- [ ] **Step 3: Implement ProximitySensor evaluation**

In `ViewDependentSystem.hpp`, add the hook + evaluate in `update` (after the LOD loop). Box test in the sensor's local frame:

```cpp
public:
  // Test/observer seam: (sensor, isActive, eventTime) on each edge.
  void setSensorHook(std::function<void(X3DNode *, bool, double)> h) { sensorHook_ = std::move(h); }

private:
  std::function<void(X3DNode *, bool, double)> sensorHook_;

  static bool insideBox(const SFVec3f &p, const SFVec3f &center, const SFVec3f &size) {
    if (size.x <= 0 || size.y <= 0 || size.z <= 0) return false; // zero-volume => inert (§22.4.1)
    return std::fabs(p.x - center.x) <= size.x * 0.5f &&
           std::fabs(p.y - center.y) <= size.y * 0.5f &&
           std::fabs(p.z - center.z) <= size.z * 0.5f;
  }

  void updateProximity(X3DNode *node, bool &last, double now, X3DExecutionContext &ctx) {
    if (!geombounds::getField<bool>(*node, "enabled", true)) return;
    const Mat4 w = ctx.worldTransform(node);
    const Mat4 inv = w.inverse();
    const SFVec3f eyeLocal = inv.transformPoint(ctx.cameraWorldPosition());
    const SFVec3f center = geombounds::getField<SFVec3f>(*node, "center", {0, 0, 0});
    const SFVec3f size = geombounds::getField<SFVec3f>(*node, "size", {0, 0, 0});
    const bool inside = insideBox(eyeLocal, center, size);
    if (inside) {
      // position/orientation in sensor coordinate system (§22.4.1).
      ctx.postEvent(node, "position_changed", std::any(eyeLocal));
      const SFVec3f fwdLocal = viewdep::norm(inv.transformDirection(
          viewMatrixForward(ctx)));
      (void)fwdLocal; // orientation_changed: emit a rotation looking along fwdLocal
    }
    if (inside != last) {
      last = inside;
      ctx.postEvent(node, "isActive", std::any(inside));
      ctx.postEvent(node, inside ? "enterTime" : "exitTime", std::any(static_cast<SFTime>(now)));
      if (sensorHook_) sensorHook_(node, inside, now);
    }
  }

  static SFVec3f viewMatrixForward(X3DExecutionContext &ctx) {
    return ctx.viewMatrix().inverse().transformDirection(SFVec3f{0, 0, -1});
  }
```

Call `updateProximity` from `update` for each `sensorActive_` entry whose `nodeTypeName()=="ProximitySensor"`:

```cpp
    for (auto &[node, active] : sensorActive_) {
      if (node->nodeTypeName() == "ProximitySensor") updateProximity(node, active, now, ctx);
      // VisibilitySensor in Task 6
    }
```

NOTE: `orientation_changed` emission — compute an `SFRotation` that orients local −Z along the viewer forward in sensor space. If a `lookAtRotation(forward, up)` helper does not exist in `runtime/math`, add a minimal one in `ViewDependentSystem.hpp` that builds a quaternion from the forward/up basis and converts to axis-angle; emit `orientation_changed` with it. (The test asserts enter/exit/position; orientation is additionally emitted for conformance.)

- [ ] **Step 4: Run to verify it passes**

Run: `mise run build 2>&1 | tail -5` → green. `mise run golden` → byte-identical.

- [ ] **Step 5: Add load-time initial-event test + implementation**

Per §22.4.1 a ProximitySensor whose viewer is inside at load emits initial `isActive TRUE`+`enterTime`. The first `tick()` after `attach` already produces this (last=false → inside=true → edge). Add a test asserting that with the viewer placed inside before the first `tick`, the first tick fires enter:

```cpp
static void testProximityLoadTime() {
  auto ps = createX3DNode("ProximitySensor");
  setF(ps, "size", std::any(SFVec3f{100,100,100}));
  auto vp = createX3DNode("Viewpoint"); setF(vp,"position",std::any(SFVec3f{0,0,0}));
  Scene scene; scene.addRootNode(vp); scene.addRootNode(ps);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  auto vds=std::make_shared<ViewDependentSystem>(); vds->attach(ps.get(),ctx); ctx.addSystem(vds);
  bool fired=false; vds->setSensorHook([&](X3DNode*n,bool a,double){ if(n==ps.get()&&a)fired=true; });
  ctx.tick(0.0);
  assert(fired); // initial enter at load
}
```

Add `testProximityLoadTime();`. It should already pass (the edge logic covers it). Run `mise run build` → green.

- [ ] **Step 6: Commit**

```bash
git add runtime/scene/ViewDependentSystem.hpp runtime/scene/tests/view_dependent_test.cpp
git commit -m "m2e: ProximitySensor enter/exit/position/orientation + load-time events (§22.4.1)"
```

---

## Task 6: VisibilitySensor (cone fallback + frustum seam)

**Files:**
- Modify: `runtime/scene/ViewDependentSystem.hpp`
- Modify: `runtime/extract/SceneExtractor.hpp` (`setViewVolume` seam — optional, see Step 5)
- Modify: `runtime/scene/tests/view_dependent_test.cpp`

- [ ] **Step 1: Write the failing test (cone fallback)**

```cpp
static void testVisibilitySensorCone() {
  auto vs = createX3DNode("VisibilitySensor");
  setF(vs, "size", std::any(SFVec3f{2, 2, 2}));
  auto vp = createX3DNode("Viewpoint");
  setF(vp, "position", std::any(SFVec3f{0, 0, 10})); // looking down -Z toward origin
  Scene scene; scene.addRootNode(vp); scene.addRootNode(vs);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  auto vds=std::make_shared<ViewDependentSystem>(); vds->attach(vs.get(),ctx); ctx.addSystem(vds);
  bool active=false; vds->setSensorHook([&](X3DNode*n,bool a,double){ if(n==vs.get())active=a; });
  ctx.tick(0.0);
  assert(active); // box at origin is in front of the camera (within cone) -> visible
  // Move the box behind the camera (viewer still at +10 looking -Z).
  setF(vs, "center", std::any(SFVec3f{0,0,50})); // behind viewer (z>10)
  ctx.tick(1.0);
  assert(!active); // behind camera -> not visible
}
```

Add `testVisibilitySensorCone();`.

- [ ] **Step 2: Run to verify it fails**

Run: `mise run build 2>&1 | tail -15` → assertion fails (VisibilitySensor not yet evaluated; `active` stays false).

- [ ] **Step 3: Implement the cone visibility test**

In `ViewDependentSystem.hpp`, add `updateVisibility`. Cone from the viewer along the forward vector with half-angle from `fieldOfView` (default 0.7854), clipped at the effective far distance (Task 7 computes far; here use infinite for the cone test — far cull is separate):

```cpp
  void updateVisibility(X3DNode *node, bool &last, double now, X3DExecutionContext &ctx) {
    if (!geombounds::getField<bool>(*node, "enabled", true)) return;
    const SFVec3f size = geombounds::getField<SFVec3f>(*node, "size", {0, 0, 0});
    bool visible = false;
    if (size.x > 0 && size.y > 0 && size.z > 0) {
      const Mat4 w = ctx.worldTransform(node);
      const SFVec3f center = geombounds::getField<SFVec3f>(*node, "center", {0, 0, 0});
      const SFVec3f boxWorld = w.transformPoint(center);
      const SFVec3f eye = ctx.cameraWorldPosition();
      const SFVec3f fwd = viewdep::norm(
          ctx.viewMatrix().inverse().transformDirection(SFVec3f{0, 0, -1}));
      const SFVec3f toBox = viewdep::sub(boxWorld, eye);
      // conservative bounding radius of the box in world (half-diagonal, unscaled approx)
      const float radius = 0.5f * viewdep::len(size);
      // cone half-angle from fieldOfView (min viewing angle, §23.3.1)
      X3DNode *vp = ctx.boundViewpoint();
      const float fov = vp ? geombounds::getField<float>(*vp, "fieldOfView", 0.7854f) : 0.7854f;
      const float along = viewdep::dot(toBox, fwd);
      const float dist = viewdep::len(toBox);
      // visible if any part of the box can be within the forward cone:
      // angle(toBox, fwd) - asin(radius/dist) <= fov/2, and the box is not fully behind.
      if (dist < 1e-6f) visible = true;
      else {
        const float cosang = along / dist;
        const float ang = std::acos(std::max(-1.0f, std::min(1.0f, cosang)));
        const float slack = (radius < dist) ? std::asin(radius / dist) : 3.14159265f;
        visible = (along + radius > 0.0f) && (ang - slack <= fov * 0.5f);
      }
    }
    if (visible != last) {
      last = visible;
      ctx.postEvent(node, "isActive", std::any(visible));
      ctx.postEvent(node, visible ? "enterTime" : "exitTime", std::any(static_cast<SFTime>(now)));
      if (sensorHook_) sensorHook_(node, visible, now);
    }
  }
```

Dispatch from `update`'s sensor loop:

```cpp
      else if (node->nodeTypeName() == "VisibilitySensor") updateVisibility(node, active, now, ctx);
```

- [ ] **Step 4: Run to verify it passes**

Run: `mise run build 2>&1 | tail -5` → green. `mise run golden` → byte-identical.

- [ ] **Step 5: Frustum seam (optional consumer-supplied view volume)**

Add a `ViewVolume` struct + setter so a consumer can supply an exact frustum; when set, VisibilitySensor uses a frustum containment test instead of the cone. Add to `ViewDependentSystem`:

```cpp
public:
  struct ViewVolume { bool valid=false; float aspect=1.0f; /* extend: planes */ };
  void setViewVolume(const ViewVolume &vv) { viewVolume_ = vv; }
private:
  ViewVolume viewVolume_{};
```

In `updateVisibility`, if `viewVolume_.valid`, widen the effective horizontal half-angle by `aspect` (the minimal exact-aspect improvement); full 6-plane frustum is deferred (document in the design's frustum-seam note). Add a test that sets a wide aspect and asserts a box just outside the cone but inside the widened frustum is visible. Build → green.

- [ ] **Step 6: Commit**

```bash
git add runtime/scene/ViewDependentSystem.hpp runtime/scene/tests/view_dependent_test.cpp
git commit -m "m2e: VisibilitySensor cone view-volume + frustum seam (§22.4.3)"
```

---

## Task 7: visibilityLimit far-cull tag + visible=FALSE skip

**Files:**
- Modify: `runtime/extract/RenderItem.hpp` (and engine `RenderItem` in SceneExtractor.hpp) — `beyondVisibilityLimit` flag
- Modify: `runtime/extract/SceneExtractor.hpp` (`walk` visible skip; `emit` far tag)
- Modify: `runtime/scene/tests/view_dependent_test.cpp`

- [ ] **Step 1: Write the failing test (visible=FALSE skip)**

```cpp
static void testVisibleFalseSkip() {
  auto grp = createX3DNode("Group");
  setF(grp, "visible", std::any(false)); // X3D 4.0: not displayed
  auto shape = createX3DNode("Shape"); auto box = createX3DNode("Box");
  setF(shape, "geometry", std::any(std::static_pointer_cast<X3DNode>(box)));
  for (auto &f : grp->fields()) if (f.x3dName=="children"&&f.set)
    f.set(*grp, std::any(std::vector<std::shared_ptr<X3DNode>>{shape}));
  auto vp = createX3DNode("Viewpoint");
  Scene scene; scene.addRootNode(vp); scene.addRootNode(grp);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  extract::SceneExtractor ex(ctx, scene);
  auto snap = ex.fullSnapshot();
  assert(snap.added.empty()); // visible=FALSE subtree produces no render items
}
```

Add `testVisibleFalseSkip();`.

- [ ] **Step 2: Run to verify it fails**

Run: `mise run build 2>&1 | tail -15` → assertion fails (subtree still emitted).

- [ ] **Step 3: Implement visible=FALSE skip in walk**

In `SceneExtractor.hpp` `walk`, right after `if (!n) return;` add:

```cpp
  // X3D 4.0 visibility: a node with visible=FALSE (and its subtree) is not displayed.
  if (geombounds::hasField(*n, "visible") &&
      !geombounds::getField<bool>(*n, "visible", true))
    return;
```

(Placed before `path.push_back` so the node contributes nothing; the event cascade still sees the node — this only affects extraction.)

- [ ] **Step 4: Run to verify it passes**

Run: `mise run build 2>&1 | tail -5` → green. `mise run golden` → byte-identical.

- [ ] **Step 5: Write the failing test (beyondVisibilityLimit tag)**

```cpp
static void testVisibilityLimitTag() {
  auto nav = createX3DNode("NavigationInfo");
  setF(nav, "visibilityLimit", std::any(SFFloat(5.0f)));
  auto shape = createX3DNode("Shape"); auto box = createX3DNode("Box");
  setF(shape, "geometry", std::any(std::static_pointer_cast<X3DNode>(box)));
  auto tf = createX3DNode("Transform");
  setF(tf, "translation", std::any(SFVec3f{0, 0, -100})); // far from viewer
  for (auto &f : tf->fields()) if (f.x3dName=="children"&&f.set)
    f.set(*tf, std::any(std::vector<std::shared_ptr<X3DNode>>{shape}));
  auto vp = createX3DNode("Viewpoint"); setF(vp,"position",std::any(SFVec3f{0,0,0}));
  Scene scene; scene.addRootNode(vp); scene.addRootNode(nav); scene.addRootNode(tf);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  extract::SceneExtractor ex(ctx, scene);
  auto snap = ex.fullSnapshot();
  assert(!snap.added.empty());
  assert(ex.item(snap.added[0]).beyondVisibilityLimit); // >5 units away -> tagged
}
```

Add `testVisibilityLimitTag();`.

- [ ] **Step 6: Run to verify it fails**

Run: `mise run build 2>&1 | tail -15` → compile error: `beyondVisibilityLimit` not a member.

- [ ] **Step 7: Add the flag + compute it in emit**

In the engine `RenderItem` struct (`SceneExtractor.hpp:90-102`) add `bool beyondVisibilityLimit = false;`. In `emit`, after computing `worldM`, compute the effective far and tag:

```cpp
  // §23.4.4 effective far = Viewpoint.farDistance>0 ? : NavigationInfo.visibilityLimit>0 ? : inf.
  float far = 0.0f; // 0 => infinite
  if (X3DNode *vp = ctx_.boundViewpoint()) {
    float fd = geombounds::getField<float>(*vp, "farDistance", -1.0f);
    if (fd > 0.0f) far = fd;
  }
  if (far == 0.0f) {
    if (X3DNode *ni = ctx_.boundNavigationInfo())
      far = geombounds::getField<float>(*ni, "visibilityLimit", 0.0f);
  }
  bool beyond = false;
  if (far > 0.0f) {
    const SFVec3f origin = worldM.transformPoint(SFVec3f{0, 0, 0});
    const SFVec3f eye = ctx_.cameraWorldPosition();
    const float d = viewdep::len(viewdep::sub(origin, eye));
    beyond = d > far; // conservative: item origin beyond far
  }
```

Set `rec.beyondVisibilityLimit = beyond;` in both the new-item and existing-item branches. (For the consumer-facing surface, also surface the flag wherever items are exposed — confirm whether the consumer reads the engine `RenderItem` directly; if `RenderItem.hpp` has a separate exposed struct, add the field there and copy it.)

- [ ] **Step 8: Run to verify it passes**

Run: `mise run build 2>&1 | tail -5` → green. `mise run golden` → byte-identical.

- [ ] **Step 9: Commit**

```bash
git add runtime/extract/SceneExtractor.hpp runtime/extract/RenderItem.hpp runtime/scene/tests/view_dependent_test.cpp
git commit -m "m2e: visible=FALSE extraction skip + beyondVisibilityLimit far-cull tag (§23.4.4)"
```

---

## Task 8: Production wiring + BACKLOG + final gate

**Files:**
- Modify: `runtime/events/X3DExecutionContext.hpp` or `runtime/scene/X3DSceneBridge.hpp` (auto-register ViewDependentSystem during build)
- Modify: `docs/superpowers/BACKLOG.md` (M2e rows + deferrals)

- [ ] **Step 1: Wire ViewDependentSystem into scene build**

Production code must create + attach the system for every LOD/Proximity/Visibility node. The cleanest place is the bridge (`X3DSceneBridge.hpp`) or a helper the runtime calls after `buildSceneGraph`, since `X3DExecutionContext.hpp` cannot include `ViewDependentSystem.hpp` (cycle). Add (in a header that includes both, e.g. `X3DSceneBridge.hpp`):

```cpp
inline void attachViewDependent(Scene &scene, X3DExecutionContext &ctx) {
  auto vds = std::make_shared<ViewDependentSystem>();
  // walk the scene; attach vds to every LOD/ProximitySensor/VisibilitySensor.
  forEachNode(scene, [&](X3DNode *n) { vds->attach(n, ctx); });
  ctx.addSystem(vds);
}
```

Use the existing scene-walk utility (the same one `buildRoutes`/`enrollScene` use — find it: `grep -rn "forEachNode\|walkScene\|enrollScene" runtime/`). If none is reusable, mirror the recursive SFNode/MFNode descent from `SceneExtractor::walk`. Pick must also call `pick_.setCamera(cameraWorldPosition(), cameraWorldUp())` before `pickClosest` — add that in `X3DExecutionContext::pick`.

- [ ] **Step 2: Write the failing integration test**

Add a test that builds a scene with an LOD + ProximitySensor, calls only `ctx.buildFrom`/the production attach path (not manual `attach`), ticks, and asserts the sensor/LOD behave — proving production wiring attaches the system. (Model it on `testProximitySensor` but using the production `attachViewDependent` entry point.)

- [ ] **Step 3: Run to verify it fails, then implement wiring, then passes**

Run `mise run build 2>&1 | tail -10` red → wire per Step 1 → green.

- [ ] **Step 4: Update BACKLOG.md**

Add an M2e section to `docs/superpowers/BACKLOG.md` marking the milestone CLOSED with commits, plus deferral rows: `centerOfRotation_changed` (→ M2D-3), pointing-device sensors (→ M2D-1), TransformSensor (→ later), full 6-plane VisibilitySensor frustum (→ when a consumer needs exactness), per-layer view volumes (→ Layering). Update the footer "How this list is used".

- [ ] **Step 5: Final gate**

Run: `mise run ci 2>&1 | tail -8`
Expected: ctest 100% pass (incl. all `x3d_view_dependent` cases), `Golden tree OK ... byte-for-byte`, pytest all passed.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "m2e: production wiring + BACKLOG closure; M2 scene-graph runtime complete"
```

---

## Notes for the implementer
- **Never** read `TransformSystem.world_` for per-path math; always re-accumulate `worldM` down the walk (M2C-1). The `ctx.worldTransform(node)` convenience used in `ViewDependentSystem::update` for the per-node `level_changed`/sensor events is first-path-only **by design** (per-node-event / per-path-render split) — do not "fix" it to per-path; that is a documented approximation.
- **Golden must stay byte-identical** every task — these are runtime-only changes; if `mise run golden` drifts you accidentally touched `src/x3d_cpp_gen/`, `templates/`, or `generated_cpp_bindings/`.
- **Include-cycle guard:** `ViewDependentSystem.hpp` includes `X3DExecutionContext.hpp`, so anything `X3DExecutionContext.hpp` includes (PickSystem) must get `billboardLocalMatrix` from the standalone `runtime/scene/Billboard.hpp` (Task 3 Step 4), not from `ViewDependentSystem.hpp`.
- **clangd false positive:** tests include `X3DDocument.hpp` for the out-of-line `Scene::addRootNode`; keep it with the `// out-of-line Scene::addRootNode` comment even though clangd flags it unused.
