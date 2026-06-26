# Followers Component Runtime Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Give the 14 inert Followers nodes (Chasers + Dampers) a runtime so routing `set_destination` produces smooth time-driven `value_changed` events.

**Architecture:** Two templated `System` subclasses mirroring `InterpolatorSystem` — `DamperSystem<NodeT,ValueT>` (IIR cascade of `order` lerp-filters, `α=e^(−dt/τ)`) and `ChaserSystem<NodeT,ValueT>` (re-basing linear ramp reaching the destination `duration` after the last input). Both are stateful (per-node state held in the System) and time-driven (`attach` registers `set_destination`/`set_value` handlers; `update(now)` advances + emits). A `FollowerArith<T>` trait supplies `lerp`/`dist` per value type (vector / slerp / element-wise). Registered via `makeFollowerSystems()` and wired by `attachFollowers` next to the interpolators.

**Tech Stack:** C++20, header-only runtime (`runtime/events/`), CTest, the generated X3D node bindings, the `x3d` CLI golden harness.

## Global Constraints

- **Pure runtime, ungated** (no `X3D_CPP_BUILD_PHYSICS`). One build config. Build/test: `cmake --preset dev && cmake --build --preset dev && ctest --preset dev -j "$(nproc)"`. Single target: `cmake --build --preset dev --target <name>`. Respect the preset compile-job cap.
- **Value types** (`generated_cpp_bindings/X3Dtypes.hpp`): `SFVec2f{float x,y}`, `SFVec3f{float x,y,z}`, `SFColor{float r,g,b}`, `SFRotation{float x,y,z,angle}`, `MFVec3f = std::vector<SFVec3f>`, `MFVec2f = std::vector<SFVec2f>`, `SFFloat = float`, `SFBool = bool`.
- **14 nodes → 7 value types × {Damper, Chaser}:** ScalarChaser/Damper (float), PositionChaser/Damper (SFVec3f), PositionChaser2D/Damper2D (SFVec2f), ColorChaser/Damper (SFColor), OrientationChaser/Damper (SFRotation), CoordinateChaser/Damper (MFVec3f), TexCoordChaser2D/Damper2D (MFVec2f). Each node header is `generated_cpp_bindings/<Name>.hpp`.
- **Spec algorithm (ISO/IEC 19775-1 §39):** Damper = `order∈[0..5]` (default 3) cascaded first-order filters; `tau∈[0,∞)` (default 0.3) is the time constant (63.2% = `1−1/e` of a step in `tau` s); `order=0` OR `tau=0` → forward destination directly; `tolerance` default `−1` → use **0.001**; end-of-transition test runs BEFORE the new output and snaps to destination when all filters are within tolerance. Chaser = reaches destination exactly `duration∈[0,∞)` (default 1) seconds after the last `set_destination`; `duration=0` → forward directly. `set_value` stops the transition and jumps. `initialValue` seeds output; `initialDestination` seeds destination (initial transition only if they differ).
- **Generated API per follower node:** `setOnSet_destinationHandler(std::function<void(const T&)>)`, `setOnSet_valueHandler(...)`, `onSet_destination(const T&)`/`onSet_value(const T&)` (invoke the handler), `emitValue_changed(const T&)`, `getValue_changed()`, `emitIsActive(const SFBool&)`, `getIsActive()`; base getters `getTau()`, `getOrder()`, `getTolerance()` (damper), `getDuration()` (chaser), and per-node `getInitialValue()`, `getInitialDestination()`.
- **Existing helpers to REUSE (don't reimplement):** `slerpRotation(const SFRotation&, const SFRotation&, float)` and `Quat`/`quatFromRotation`/`rotationFromQuat`/`slerp` in `runtime/events/Interpolation.hpp`; `lerpf`/`lerpVec3`/`lerpVec2` there too.
- **System base** (`runtime/events/X3DSystem.hpp`): `virtual void attach(X3DNode*, X3DExecutionContext&)` and `virtual void update(double now, X3DExecutionContext&)` (default no-op). Emit downstream via `ctx.postEvent(node, "value_changed", std::any(value))`.
- **Refinements from the spec (intentional, documented):** (1) no separate `Multi*` System classes — `FollowerArith<MF…>` element-wise specializations cover MF in the single templated System; (2) the chaser re-bases a single transition via `lerp` (uniform for all types incl. SFRotation) rather than literal FIR superposition — identical to superposition for a single transition, reaches the destination `duration` after the last input; multi-overlapping-transition fidelity is a deferred refinement (note it in `findings.yaml`).
- **TDD:** red → green → commit per step. **State-stability:** hold per-node state in heap-stable storage (`std::vector<std::unique_ptr<Entry>>`) so `set_*` handlers can capture a stable `Entry*`.
- **No existing goldens change** (followers were inert). `mise run conformance-gate` passes after regen.

## File Structure

- `runtime/events/FollowerArith.hpp` — `FollowerArith<T>` trait: `lerp(a,b,t)` + `dist(a,b)`, specialized for the 7 value types.
- `runtime/events/FollowerSystem.hpp` — `DamperSystem<NodeT,ValueT>` + `ChaserSystem<NodeT,ValueT>`.
- `runtime/events/FollowerRegistration.hpp` — `makeFollowerSystems()` + `attachFollowers(scene, ctx)`.
- `runtime/events/X3DSceneBridge.hpp` — call `attachFollowers` where `attachInterpolators` is called.
- `runtime/events/tests/follower_conformance_test.cpp` — behavioral tests (+ CMake registration as `x3d_event_followers`).
- `tools/x3d-cli/fixtures/sim-damper.x3d`, `tools/x3d-cli/goldens/sim-damper.trace.json`, `tools/tests/x3d_cli_test.sh` — CLI golden.
- `docs/conformance/findings.yaml`, `docs/superpowers/BACKLOG.md` — FOL-* closure + regen.

---

### Task 1: `FollowerArith<T>` trait + per-type specializations

**Files:**
- Create: `runtime/events/FollowerArith.hpp`
- Test: extend `runtime/events/tests/follower_conformance_test.cpp` (created here) + register `x3d_event_followers` in `CMakeLists.txt`

**Interfaces:**
- Produces: `template<typename T> struct FollowerArith { static T lerp(const T&, const T&, float); static float dist(const T&, const T&); };` specialized for `float, SFVec2f, SFVec3f, SFColor, SFRotation, MFVec3f, MFVec2f`. `lerp(a,b,t)` returns `a` at t=0 and `b` at t=1; `dist` is a non-negative magnitude of the a→b difference (0 iff equal), used for the damper tolerance test.

- [ ] **Step 1: Write the failing test** (create `runtime/events/tests/follower_conformance_test.cpp`)

```cpp
// follower_conformance_test.cpp
// Behavioral-conformance tests for the Followers component runtime (§39):
// FollowerArith ops, DamperSystem (IIR), ChaserSystem (re-basing ramp), wiring.
#include "FollowerArith.hpp"
#include <cmath>
#include <cstdio>
#include <vector>
using namespace x3d::runtime;
using namespace x3d;
static int g_fail = 0;
#define CHECK(c, m) do{ if(!(c)){ std::fprintf(stderr,"FAIL: %s (%s:%d)\n",m,__FILE__,__LINE__); ++g_fail; } }while(0)
static bool feq(float a, float b, float e=1e-4f){ return std::fabs(a-b) < e; }

static void test_arith() {
  // float
  CHECK(feq(FollowerArith<float>::lerp(0.f,10.f,0.25f), 2.5f), "float lerp");
  CHECK(feq(FollowerArith<float>::dist(1.f,4.f), 3.f), "float dist");
  // SFVec3f
  SFVec3f a{0,0,0}, b{2,4,6};
  auto m = FollowerArith<SFVec3f>::lerp(a,b,0.5f);
  CHECK(feq(m.x,1)&&feq(m.y,2)&&feq(m.z,3), "vec3 lerp");
  CHECK(feq(FollowerArith<SFVec3f>::dist(a,b), std::sqrt(56.f), 1e-3f), "vec3 dist");
  // SFColor
  SFColor c0{0,0,0}, c1{1,1,1};
  auto cm = FollowerArith<SFColor>::lerp(c0,c1,0.5f);
  CHECK(feq(cm.r,0.5f)&&feq(cm.g,0.5f)&&feq(cm.b,0.5f), "color lerp");
  // SFVec2f
  SFVec2f v0{0,0}, v1{4,8}; auto vm=FollowerArith<SFVec2f>::lerp(v0,v1,0.25f);
  CHECK(feq(vm.x,1)&&feq(vm.y,2), "vec2 lerp");
  // SFRotation: slerp end-points + stays unit, dist=angle
  SFRotation r0{0,1,0,0.f}, r1{0,1,0,1.5707963f};
  auto re = FollowerArith<SFRotation>::lerp(r0,r1,1.0f);
  CHECK(feq(re.angle,1.5707963f,1e-3f), "rotation slerp reaches end angle");
  CHECK(FollowerArith<SFRotation>::dist(r0,r1) > 1.4f, "rotation dist ~ angle");
  CHECK(FollowerArith<SFRotation>::dist(r0,r0) < 1e-4f, "rotation dist zero when equal");
  // MFVec3f element-wise; output adopts b's length
  MFVec3f ma{{0,0,0},{0,0,0}}, mb{{2,2,2},{4,4,4}};
  auto mm = FollowerArith<MFVec3f>::lerp(ma,mb,0.5f);
  CHECK(mm.size()==2 && feq(mm[0].x,1)&&feq(mm[1].x,2), "MFVec3f element-wise lerp");
  CHECK(FollowerArith<MFVec3f>::dist(ma,mb) > 0.f, "MFVec3f dist nonzero");
  // MFVec2f
  MFVec2f na{{0,0}}, nb{{4,4}};
  auto nm = FollowerArith<MFVec2f>::lerp(na,nb,0.5f);
  CHECK(nm.size()==1 && feq(nm[0].x,2), "MFVec2f element-wise lerp");
}

int main(){
  test_arith();
  if(g_fail==0) std::fprintf(stderr,"follower_conformance_test: ALL PASS\n");
  return g_fail==0?0:1;
}
```

- [ ] **Step 2: Register the test in CMake + verify it fails to build**

In `CMakeLists.txt`, next to the `x3d_event_interpolators` registration (search `x3d_event_interpolators`), add inside the same `X3D_CPP_BUILD_TESTS` block:
```cmake
    add_executable(x3d_event_followers
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/follower_conformance_test.cpp")
    target_link_libraries(x3d_event_followers PRIVATE x3d_cpp_nodes)
    target_include_directories(x3d_event_followers PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events"
        "${CMAKE_CURRENT_SOURCE_DIR}/generated_cpp_bindings")
    add_test(NAME x3d_event_followers COMMAND x3d_event_followers)
```
(Match the exact `target_link_libraries`/`target_include_directories` of `x3d_event_interpolators` if it differs — copy that target's lines verbatim, swapping the name + source.)
Run: `cmake --preset dev >/dev/null && cmake --build --preset dev --target x3d_event_followers 2>&1 | tail -5`
Expected: FAIL — `FollowerArith.hpp` not found.

- [ ] **Step 3: Implement `FollowerArith.hpp`**

```cpp
#ifndef X3D_RUNTIME_FOLLOWER_ARITH_HPP
#define X3D_RUNTIME_FOLLOWER_ARITH_HPP
#include "Interpolation.hpp" // slerpRotation, Quat, quatFromRotation
#include "X3Dtypes.hpp"
#include <algorithm>
#include <cmath>
#include <vector>
namespace x3d::runtime {
template <typename T> struct FollowerArith;

template <> struct FollowerArith<float> {
  static float lerp(const float &a, const float &b, float t){ return a + (b-a)*t; }
  static float dist(const float &a, const float &b){ return std::fabs(a-b); }
};
template <> struct FollowerArith<SFVec2f> {
  static SFVec2f lerp(const SFVec2f &a, const SFVec2f &b, float t){ return {a.x+(b.x-a.x)*t, a.y+(b.y-a.y)*t}; }
  static float dist(const SFVec2f &a, const SFVec2f &b){ return std::hypot(a.x-b.x, a.y-b.y); }
};
template <> struct FollowerArith<SFVec3f> {
  static SFVec3f lerp(const SFVec3f &a, const SFVec3f &b, float t){ return {a.x+(b.x-a.x)*t, a.y+(b.y-a.y)*t, a.z+(b.z-a.z)*t}; }
  static float dist(const SFVec3f &a, const SFVec3f &b){ float dx=a.x-b.x,dy=a.y-b.y,dz=a.z-b.z; return std::sqrt(dx*dx+dy*dy+dz*dz); }
};
template <> struct FollowerArith<SFColor> {
  static SFColor lerp(const SFColor &a, const SFColor &b, float t){ return {a.r+(b.r-a.r)*t, a.g+(b.g-a.g)*t, a.b+(b.b-a.b)*t}; }
  static float dist(const SFColor &a, const SFColor &b){ float dr=a.r-b.r,dg=a.g-b.g,db=a.b-b.b; return std::sqrt(dr*dr+dg*dg+db*db); }
};
template <> struct FollowerArith<SFRotation> {
  static SFRotation lerp(const SFRotation &a, const SFRotation &b, float t){ return slerpRotation(a, b, t); }
  static float dist(const SFRotation &a, const SFRotation &b){
    // angle between orientations via quaternion dot.
    Quat qa = quatFromRotation(a), qb = quatFromRotation(b);
    double d = std::fabs(qa.x*qb.x + qa.y*qb.y + qa.z*qb.z + qa.w*qb.w);
    d = std::min(1.0, d);
    return static_cast<float>(2.0 * std::acos(d));
  }
};
// MF: element-wise, output adopts b's length (the destination's shape).
template <typename Elem> struct MFFollowerArith {
  using V = std::vector<Elem>;
  static V lerp(const V &a, const V &b, float t){
    V out(b.size());
    for(size_t i=0;i<b.size();++i)
      out[i] = (i<a.size()) ? FollowerArith<Elem>::lerp(a[i], b[i], t) : b[i];
    return out;
  }
  static float dist(const V &a, const V &b){
    float m=0.f; size_t n=std::min(a.size(), b.size());
    for(size_t i=0;i<n;++i) m=std::max(m, FollowerArith<Elem>::dist(a[i], b[i]));
    if(a.size()!=b.size()) m=std::max(m, 1e9f); // length mismatch => not converged
    return m;
  }
};
template <> struct FollowerArith<MFVec3f> : MFFollowerArith<SFVec3f> {};
template <> struct FollowerArith<MFVec2f> : MFFollowerArith<SFVec2f> {};
} // namespace x3d::runtime
#endif
```
(If `quatFromRotation`/`Quat` field names differ in `Interpolation.hpp`, adapt the `dist` for SFRotation to that API — the goal is the angle between the two orientations.)

- [ ] **Step 4: Build + run the test → PASS**

Run: `cmake --build --preset dev --target x3d_event_followers && ctest --preset dev -R x3d_event_followers --output-on-failure 2>&1 | tail -6`
Expected: PASS — `follower_conformance_test: ALL PASS`. If the SFRotation `dist` or `quatFromRotation` API mismatches, fix per the actual `Interpolation.hpp` signatures (verify with `grep -nE "struct Quat|quatFromRotation" runtime/events/Interpolation.hpp`).

- [ ] **Step 5: Commit**

```bash
git add runtime/events/FollowerArith.hpp runtime/events/tests/follower_conformance_test.cpp CMakeLists.txt
git commit -m "feat(followers): FollowerArith<T> lerp/dist trait for the 7 value types"
```

---

### Task 2: `DamperSystem<NodeT,ValueT>` (IIR)

**Files:**
- Create: `runtime/events/FollowerSystem.hpp` (DamperSystem here; ChaserSystem added in Task 3)
- Test: extend `runtime/events/tests/follower_conformance_test.cpp`

**Interfaces:**
- Consumes: `FollowerArith<T>` (Task 1); `System`, `X3DExecutionContext` (`ctx.postEvent`).
- Produces: `template<typename NodeT, typename ValueT> class DamperSystem : public System` with `attach(X3DNode*, X3DExecutionContext&)` + `update(double now, X3DExecutionContext&)`.

- [ ] **Step 1: Write the failing test** (append to `follower_conformance_test.cpp`, and call from `main`)

```cpp
#include "FollowerSystem.hpp"
#include "PositionDamper.hpp"
#include "ScalarDamper.hpp"
#include "X3DExecutionContext.hpp"
#include <memory>

static void test_damper() {
  using namespace x3d::runtime;
  X3DExecutionContext ctx;
  // order-1, tau=0.3 scalar step 0 -> 1; sample at t=tau => ~0.6321.
  auto sd = std::make_shared<ScalarDamper>();
  sd->setOrderUnchecked(1); sd->setTauUnchecked(0.3); sd->setToleranceUnchecked(-1);
  sd->setInitialValueUnchecked(0.0f); sd->setInitialDestinationUnchecked(0.0f);
  DamperSystem<ScalarDamper, float> dsys;
  dsys.attach(sd.get(), ctx);
  sd->onSet_destination(1.0f);            // begin transition to 1
  // tick at dt=0.3 once: out ~ 0.6321
  dsys.update(0.3, ctx);
  CHECK(feq(sd->getValue_changed(), 0.6321f, 5e-3f), "damper order1 reaches ~63.2% at t=tau");
  CHECK(sd->getIsActive()==true, "damper active during transition");
  // continue: asymptotes upward toward 1
  for(double t=0.6;t<=3.0;t+=0.3) dsys.update(t, ctx);
  CHECK(sd->getValue_changed() > 0.95f, "damper asymptotes toward destination");

  // order=0 OR tau=0 => passthrough (immediate).
  auto sp = std::make_shared<ScalarDamper>();
  sp->setOrderUnchecked(0); sp->setTauUnchecked(0.3);
  sp->setInitialValueUnchecked(0.0f); sp->setInitialDestinationUnchecked(0.0f);
  DamperSystem<ScalarDamper, float> psys; psys.attach(sp.get(), ctx);
  sp->onSet_destination(5.0f); psys.update(0.016, ctx);
  CHECK(feq(sp->getValue_changed(), 5.0f), "damper order0 forwards destination immediately");

  // set_value jumps + stops.
  auto sv = std::make_shared<ScalarDamper>();
  sv->setOrderUnchecked(3); sv->setTauUnchecked(0.3);
  DamperSystem<ScalarDamper, float> vsys; vsys.attach(sv.get(), ctx);
  sv->onSet_destination(1.0f); vsys.update(0.1, ctx);
  sv->onSet_value(9.0f);
  CHECK(feq(sv->getValue_changed(), 9.0f), "damper set_value jumps to value");
  CHECK(sv->getIsActive()==false, "damper set_value stops transition");

  // determinism: two identical runs match.
  auto run=[&](){ auto n=std::make_shared<ScalarDamper>(); n->setOrderUnchecked(3); n->setTauUnchecked(0.3);
    DamperSystem<ScalarDamper,float> s; s.attach(n.get(),ctx); n->onSet_destination(1.0f);
    for(double t=0.1;t<=2.0;t+=0.1) s.update(t,ctx); return n->getValue_changed(); };
  CHECK(feq(run(), run(), 1e-6f), "damper deterministic");

  // Vec3 type works through the same template.
  auto pd = std::make_shared<PositionDamper>();
  pd->setOrderUnchecked(2); pd->setTauUnchecked(0.2);
  DamperSystem<PositionDamper, SFVec3f> p3; p3.attach(pd.get(), ctx);
  pd->onSet_destination(SFVec3f{10,0,0});
  for(double t=0.1;t<=3.0;t+=0.1) p3.update(t, ctx);
  CHECK(pd->getValue_changed().x > 9.5f, "PositionDamper converges in x");
}
```
Add `test_damper();` to `main` before the ALL PASS print. (Verify the exact `*Unchecked` setter names with `grep -nE "setOrderUnchecked|setTauUnchecked|setToleranceUnchecked|setInitialValueUnchecked|setInitialDestinationUnchecked" generated_cpp_bindings/ScalarDamper.hpp generated_cpp_bindings/X3DDamperNode.hpp` — adapt if the generated names differ.)

- [ ] **Step 2: Build → FAIL** (`FollowerSystem.hpp` missing)

Run: `cmake --build --preset dev --target x3d_event_followers 2>&1 | tail -5` → FAIL.

- [ ] **Step 3: Implement `DamperSystem` in `FollowerSystem.hpp`**

```cpp
#ifndef X3D_RUNTIME_FOLLOWER_SYSTEM_HPP
#define X3D_RUNTIME_FOLLOWER_SYSTEM_HPP
#include "FollowerArith.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"
#include <any>
#include <cmath>
#include <memory>
#include <vector>
namespace x3d::runtime {

template <typename NodeT, typename ValueT>
class DamperSystem : public System {
  struct Entry {
    NodeT *node;
    std::vector<ValueT> filters; // size order+1; [0] = input (destination)
    ValueT destination;
    bool active = false;
    double lastTick = -1.0;
  };
  std::vector<std::unique_ptr<Entry>> entries_;
public:
  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    auto *d = dynamic_cast<NodeT *>(node);
    if (!d) return;
    auto e = std::make_unique<Entry>();
    e->node = d;
    int order = std::max(0, d->getOrder());
    e->filters.assign(order + 1, d->getInitialValue());
    e->destination = d->getInitialDestination();
    e->active = FollowerArith<ValueT>::dist(d->getInitialValue(), d->getInitialDestination()) > 0.0f;
    Entry *ep = e.get();
    d->setOnSet_destinationHandler([ep, d](const ValueT &v){
      ep->destination = v;
      if (!ep->active) { ep->active = true; d->emitIsActive(true); }
    });
    d->setOnSet_valueHandler([ep, d, &ctx](const ValueT &v){
      for (auto &f : ep->filters) f = v;
      ep->destination = v; ep->active = false;
      d->emitValue_changed(v); ctx.postEvent(d, "value_changed", std::any(v));
      d->emitIsActive(false);
    });
    entries_.push_back(std::move(e));
  }
  void update(double now, X3DExecutionContext &ctx) override {
    for (auto &e : entries_) {
      if (!e->active) { e->lastTick = now; continue; }
      double dt = (e->lastTick < 0) ? 0.0 : (now - e->lastTick);
      e->lastTick = now;
      NodeT *d = e->node;
      int order = std::max(0, d->getOrder());
      double tau = d->getTau();
      float tol = d->getTolerance(); if (tol < 0) tol = 0.001f;
      if (order == 0 || tau <= 0.0) {                     // passthrough
        d->emitValue_changed(e->destination);
        ctx.postEvent(d, "value_changed", std::any(e->destination));
        e->active = false; d->emitIsActive(false); continue;
      }
      if ((int)e->filters.size() != order + 1) e->filters.assign(order + 1, e->destination);
      e->filters[0] = e->destination;
      bool done = true;                                   // §39.3.2 test BEFORE update
      for (int i = 1; i <= order && done; ++i)
        if (FollowerArith<ValueT>::dist(e->filters[i], e->filters[i-1]) > tol) done = false;
      if (done) {
        for (int i = 1; i <= order; ++i) e->filters[i] = e->destination;
        d->emitValue_changed(e->destination);
        ctx.postEvent(d, "value_changed", std::any(e->destination));
        e->active = false; d->emitIsActive(false); continue;
      }
      float a = static_cast<float>(std::exp(-dt / tau));
      for (int i = 1; i <= order; ++i)
        e->filters[i] = FollowerArith<ValueT>::lerp(e->filters[i-1], e->filters[i], a);
      d->emitValue_changed(e->filters[order]);
      ctx.postEvent(d, "value_changed", std::any(e->filters[order]));
    }
  }
};
} // namespace x3d::runtime
#endif
```

- [ ] **Step 4: Build + run → PASS**

Run: `cmake --build --preset dev --target x3d_event_followers && ctest --preset dev -R x3d_event_followers --output-on-failure 2>&1 | tail -6`
Expected: PASS (damper hits ~0.6321 at t=τ, passthrough, set_value jump, determinism, Vec3 converge).

- [ ] **Step 5: Commit**

```bash
git add runtime/events/FollowerSystem.hpp runtime/events/tests/follower_conformance_test.cpp
git commit -m "feat(followers): DamperSystem — IIR cascade with tolerance stop + passthrough"
```

---

### Task 3: `ChaserSystem<NodeT,ValueT>` (re-basing ramp)

**Files:**
- Modify: `runtime/events/FollowerSystem.hpp` (add `ChaserSystem`)
- Test: extend `runtime/events/tests/follower_conformance_test.cpp`

**Interfaces:**
- Consumes: `FollowerArith<T>`, `System`.
- Produces: `template<typename NodeT, typename ValueT> class ChaserSystem : public System` (`attach` + `update`).

- [ ] **Step 1: Write the failing test** (append + call from `main`)

```cpp
#include "PositionChaser.hpp"
#include "ScalarChaser.hpp"

static void test_chaser() {
  using namespace x3d::runtime;
  X3DExecutionContext ctx;
  // D=1 step 0->1: reaches exactly 1.0 at t=1.0, ~0.5 at t=0.5.
  auto sc = std::make_shared<ScalarChaser>();
  sc->setDurationUnchecked(1.0);
  sc->setInitialValueUnchecked(0.0f); sc->setInitialDestinationUnchecked(0.0f);
  ChaserSystem<ScalarChaser, float> cs; cs.attach(sc.get(), ctx);
  sc->onSet_destination(1.0f);            // event at t=0 (first update establishes start time)
  cs.update(0.0, ctx);
  cs.update(0.5, ctx);
  CHECK(feq(sc->getValue_changed(), 0.5f, 1e-2f), "chaser ~0.5 mid-transition");
  cs.update(1.0, ctx);
  CHECK(feq(sc->getValue_changed(), 1.0f, 1e-3f), "chaser reaches destination exactly at duration");
  CHECK(sc->getIsActive()==false, "chaser inactive after duration");

  // re-base on a new event at t=0.5 (output 0.5) -> 2.0 over D=1: reaches 2.0 at t=1.5.
  auto rc = std::make_shared<ScalarChaser>(); rc->setDurationUnchecked(1.0);
  ChaserSystem<ScalarChaser, float> rs; rs.attach(rc.get(), ctx);
  rc->onSet_destination(1.0f); rs.update(0.0, ctx); rs.update(0.5, ctx); // output ~0.5
  rc->onSet_destination(2.0f);                                            // re-base
  rs.update(1.0, ctx);
  CHECK(rc->getValue_changed() > 1.1f && rc->getValue_changed() < 1.4f, "chaser re-bases toward new dest");
  rs.update(1.5, ctx);
  CHECK(feq(rc->getValue_changed(), 2.0f, 1e-2f), "chaser reaches new dest duration after last event");

  // set_value jumps.
  auto vc = std::make_shared<ScalarChaser>(); vc->setDurationUnchecked(1.0);
  ChaserSystem<ScalarChaser, float> vs; vs.attach(vc.get(), ctx);
  vc->onSet_destination(1.0f); vs.update(0.0, ctx); vs.update(0.3, ctx);
  vc->onSet_value(7.0f);
  CHECK(feq(vc->getValue_changed(), 7.0f), "chaser set_value jumps");

  // Vec3 reaches.
  auto pc = std::make_shared<PositionChaser>(); pc->setDurationUnchecked(1.0);
  ChaserSystem<PositionChaser, SFVec3f> ps; ps.attach(pc.get(), ctx);
  pc->onSet_destination(SFVec3f{4,0,0}); ps.update(0.0, ctx); ps.update(1.0, ctx);
  CHECK(feq(pc->getValue_changed().x, 4.0f, 1e-2f), "PositionChaser reaches dest at duration");

  // determinism.
  auto run=[&](){ auto n=std::make_shared<ScalarChaser>(); n->setDurationUnchecked(1.0);
    ChaserSystem<ScalarChaser,float> s; s.attach(n.get(),ctx); n->onSet_destination(1.0f);
    for(double t=0.0;t<=1.0;t+=0.1) s.update(t,ctx); return n->getValue_changed(); };
  CHECK(feq(run(), run(), 1e-6f), "chaser deterministic");
}
```
Add `test_chaser();` to `main`.

- [ ] **Step 2: Build → FAIL** (`ChaserSystem` undefined). `… --target x3d_event_followers` → FAIL.

- [ ] **Step 3: Implement `ChaserSystem` (append to `FollowerSystem.hpp`, inside the namespace)**

```cpp
template <typename NodeT, typename ValueT>
class ChaserSystem : public System {
  struct Entry {
    NodeT *node;
    ValueT start, destination;
    double startTime = 0.0;
    bool active = false, started = false;
    double lastTick = -1.0;
  };
  std::vector<std::unique_ptr<Entry>> entries_;
public:
  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    auto *c = dynamic_cast<NodeT *>(node);
    if (!c) return;
    auto e = std::make_unique<Entry>();
    e->node = c;
    e->start = c->getInitialValue();
    e->destination = c->getInitialDestination();
    e->active = FollowerArith<ValueT>::dist(e->start, e->destination) > 0.0f;
    Entry *ep = e.get();
    c->setOnSet_destinationHandler([ep, c](const ValueT &v){
      ep->start = ep->started ? c->getValue_changed() : ep->start; // re-base from current output
      ep->destination = v;
      ep->startTime = ep->lastTick < 0 ? 0.0 : ep->lastTick;
      ep->started = true;
      if (!ep->active) { ep->active = true; c->emitIsActive(true); }
      else c->emitIsActive(true);
    });
    c->setOnSet_valueHandler([ep, c, &ctx](const ValueT &v){
      ep->start = v; ep->destination = v; ep->active = false;
      c->emitValue_changed(v); ctx.postEvent(c, "value_changed", std::any(v));
      c->emitIsActive(false);
    });
    entries_.push_back(std::move(e));
  }
  void update(double now, X3DExecutionContext &ctx) override {
    for (auto &e : entries_) {
      e->lastTick = now;
      if (!e->active) continue;
      NodeT *c = e->node;
      double D = c->getDuration();
      double f = (D <= 0.0) ? 1.0 : (now - e->startTime) / D;
      if (f < 0.0) f = 0.0; if (f > 1.0) f = 1.0;
      ValueT out = FollowerArith<ValueT>::lerp(e->start, e->destination, static_cast<float>(f));
      c->emitValue_changed(out);
      ctx.postEvent(c, "value_changed", std::any(out));
      if (f >= 1.0) { e->active = false; c->emitIsActive(false); }
    }
  }
};
```
Note: `startTime` is seeded from the tick the event arrived at (`lastTick`), so the first `update(0.0)` after a `set_destination` at scene start gives `f=0`. The test calls `update(0.0)` right after `onSet_destination` to establish `startTime=0`.

- [ ] **Step 4: Build + run → PASS**

Run: `cmake --build --preset dev --target x3d_event_followers && ctest --preset dev -R x3d_event_followers --output-on-failure 2>&1 | tail -6` → PASS.

- [ ] **Step 5: Commit**

```bash
git add runtime/events/FollowerSystem.hpp runtime/events/tests/follower_conformance_test.cpp
git commit -m "feat(followers): ChaserSystem — re-basing ramp reaching destination at duration"
```

---

### Task 4: Registration (`makeFollowerSystems`) + production wiring + end-to-end test

**Files:**
- Create: `runtime/events/FollowerRegistration.hpp`
- Modify: `runtime/events/X3DSceneBridge.hpp` (call `attachFollowers` next to `attachInterpolators`)
- Test: extend `runtime/events/tests/follower_conformance_test.cpp`

**Interfaces:**
- Consumes: `DamperSystem`, `ChaserSystem`, all 14 node headers.
- Produces: `std::vector<std::shared_ptr<System>> makeFollowerSystems();` and `void attachFollowers(Scene&, X3DExecutionContext&);`.

- [ ] **Step 1: Write the failing end-to-end test** (append + call from `main`)

```cpp
#include "FollowerRegistration.hpp"
#include "X3DSceneBridge.hpp"
#include "X3DDocument.hpp"
#include "X3DNodeFactory.hpp"

static void test_wiring() {
  using namespace x3d::runtime;
  // makeFollowerSystems registers all 14 (sanity: non-empty, attaches a damper).
  auto systems = makeFollowerSystems();
  CHECK(systems.size() == 14, "makeFollowerSystems registers all 14 follower systems");
  X3DExecutionContext ctx;
  auto pd = std::make_shared<PositionDamper>();
  pd->setOrderUnchecked(2); pd->setTauUnchecked(0.2);
  bool lit = false;
  for (auto &s : systems) s->attach(pd.get(), ctx);   // exactly one system claims it
  pd->onSet_destination(SFVec3f{5,0,0});
  for (auto &s : systems) for (double t=0.1;t<=3.0;t+=0.1) s->update(t, ctx);
  lit = pd->getValue_changed().x > 4.5f;
  CHECK(lit, "a PositionDamper attached via makeFollowerSystems behaves (was inert)");
}
```
Add `test_wiring();` to `main`.

- [ ] **Step 2: Build → FAIL** (`FollowerRegistration.hpp` missing). FAIL.

- [ ] **Step 3: Implement `FollowerRegistration.hpp`**

```cpp
#ifndef X3D_RUNTIME_FOLLOWER_REGISTRATION_HPP
#define X3D_RUNTIME_FOLLOWER_REGISTRATION_HPP
#include "FollowerSystem.hpp"
#include "X3DExecutionContext.hpp"
// node headers
#include "ColorChaser.hpp"
#include "ColorDamper.hpp"
#include "CoordinateChaser.hpp"
#include "CoordinateDamper.hpp"
#include "OrientationChaser.hpp"
#include "OrientationDamper.hpp"
#include "PositionChaser.hpp"
#include "PositionChaser2D.hpp"
#include "PositionDamper.hpp"
#include "PositionDamper2D.hpp"
#include "ScalarChaser.hpp"
#include "ScalarDamper.hpp"
#include "TexCoordChaser2D.hpp"
#include "TexCoordDamper2D.hpp"
#include <memory>
#include <vector>
namespace x3d::runtime {
inline std::vector<std::shared_ptr<System>> makeFollowerSystems() {
  std::vector<std::shared_ptr<System>> s;
  s.push_back(std::make_shared<DamperSystem<ScalarDamper, float>>());
  s.push_back(std::make_shared<ChaserSystem<ScalarChaser, float>>());
  s.push_back(std::make_shared<DamperSystem<PositionDamper, SFVec3f>>());
  s.push_back(std::make_shared<ChaserSystem<PositionChaser, SFVec3f>>());
  s.push_back(std::make_shared<DamperSystem<PositionDamper2D, SFVec2f>>());
  s.push_back(std::make_shared<ChaserSystem<PositionChaser2D, SFVec2f>>());
  s.push_back(std::make_shared<DamperSystem<ColorDamper, SFColor>>());
  s.push_back(std::make_shared<ChaserSystem<ColorChaser, SFColor>>());
  s.push_back(std::make_shared<DamperSystem<OrientationDamper, SFRotation>>());
  s.push_back(std::make_shared<ChaserSystem<OrientationChaser, SFRotation>>());
  s.push_back(std::make_shared<DamperSystem<CoordinateDamper, MFVec3f>>());
  s.push_back(std::make_shared<ChaserSystem<CoordinateChaser, MFVec3f>>());
  s.push_back(std::make_shared<DamperSystem<TexCoordDamper2D, MFVec2f>>());
  s.push_back(std::make_shared<ChaserSystem<TexCoordChaser2D, MFVec2f>>());
  return s;
}
} // namespace x3d::runtime
#endif
```
(Verify each node's value type against its header `getInitialValue()` return type, e.g. `grep -nE "getInitialValue" generated_cpp_bindings/ColorChaser.hpp` → `SFColor`. Coordinate*/TexCoord* must be the MF type.)

- [ ] **Step 4: Add `attachFollowers` + wire it into the bridge**

Append to `FollowerRegistration.hpp` (inside the namespace), mirroring `attachInterpolators`:
```cpp
inline void attachFollowers(Scene &scene, X3DExecutionContext &ctx) {
  for (auto &sys : makeFollowerSystems()) {
    detail::forEachNode(scene, [&](X3DNode *n) { sys->attach(n, ctx); });
    ctx.addSystem(std::move(sys));
  }
}
```
(Requires `#include "X3DSceneBridge.hpp"` for `Scene`/`detail::forEachNode` — add it; if it introduces a cycle, instead place `attachFollowers` directly in `X3DSceneBridge.hpp` next to `attachInterpolators`.) Then, in `X3DSceneBridge.hpp`, find every call site of `attachInterpolators(` (`grep -rn "attachInterpolators(" runtime/ tools/`) and add a sibling `attachFollowers(scene, ctx);` call so followers wire in production exactly where interpolators do.

- [ ] **Step 5: Build + run → PASS**

Run: `cmake --build --preset dev --target x3d_event_followers && ctest --preset dev -R x3d_event_followers --output-on-failure 2>&1 | tail -6` → PASS (14 systems, damper lights up).

- [ ] **Step 6: Commit**

```bash
git add runtime/events/FollowerRegistration.hpp runtime/events/X3DSceneBridge.hpp runtime/events/tests/follower_conformance_test.cpp
git commit -m "feat(followers): register all 14 follower systems + attachFollowers production wiring"
```

---

### Task 5: CLI golden + conformance closure + final verification

**Files:**
- Create: `tools/x3d-cli/fixtures/sim-damper.x3d`, `tools/x3d-cli/goldens/sim-damper.trace.json`
- Modify: `tools/tests/x3d_cli_test.sh`, `docs/conformance/findings.yaml`, `docs/superpowers/BACKLOG.md`

**Interfaces:**
- Consumes: the wired follower runtime (Task 4).

- [ ] **Step 1: Create the CLI fixture** `tools/x3d-cli/fixtures/sim-damper.x3d`

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE X3D PUBLIC "ISO//Web3D//DTD X3D 4.0//EN" "https://www.web3d.org/specifications/x3d-4.0.dtd">
<X3D profile="Full" version="4.0">
  <head><meta name="description" content="sim PositionDamper: set_destination drives a smooth eased transition on value_changed -> a Transform.translation (§39 Followers)."/></head>
  <Scene>
    <Transform DEF="Mover"><Shape><Box/></Shape></Transform>
    <PositionDamper DEF="Damp" tau="0.4" order="3" initialValue="0 0 0" initialDestination="0 0 0"/>
    <ROUTE fromNode="Damp" fromField="value_changed" toNode="Mover" toField="translation"/>
  </Scene>
</X3D>
```
The CLI sim must deliver a `set_destination` to drive it. If the `x3d sim` tool exposes an input/route to set `Damp.set_destination` (check `x3d sim --help` for a way to post an initial event, e.g. `--set Damp.set_destination="5 0 0"`), use it; **otherwise** add a second node that emits a destination at start — a `TimeSensor`+route is overkill; instead set `initialDestination="5 0 0"` (with `initialValue 0 0 0`) so the §39 "initial transition runs at load when they differ" path animates the mover from 0→(5,0,0) with no external event. Prefer this `initialDestination` approach (self-contained, deterministic).

- [ ] **Step 2: Build the CLI + verify the mover eases (BEFORE capturing the golden)**

Run: `cmake --build --preset dev --target x3d_cli && ./build/x3d sim tools/x3d-cli/fixtures/sim-damper.x3d --fps 60 --ticks 120 --watch Mover.translation 2>/dev/null | tail -4`
Expected: `Mover.translation` x rises smoothly from ~0 toward ~5 (asymptotic). If it stays 0, the initial transition isn't firing — confirm `attachFollowers` runs in the CLI sim path (Task 4 Step 4) and that `initialValue ≠ initialDestination`. STOP and report if it won't animate.

- [ ] **Step 3: Capture the golden**

```bash
./build/x3d sim tools/x3d-cli/fixtures/sim-damper.x3d --fps 60 --ticks 120 --watch Mover.translation --json > tools/x3d-cli/goldens/sim-damper.trace.json
tail -c 160 tools/x3d-cli/goldens/sim-damper.trace.json   # sanity: x near 5
```

- [ ] **Step 4: Add the harness assertion** (in `tools/tests/x3d_cli_test.sh`, in the non-physics general section alongside other sim goldens)

```bash
# Followers: a PositionDamper eases a Transform; converges + deterministic + golden.
FIXTURE_DMP="$FIXTURES/sim-damper.x3d"
if [[ -f "$FIXTURE_DMP" ]]; then
    dmp=$("$CLI" sim "$FIXTURE_DMP" --fps 60 --ticks 120 --watch Mover.translation --json 2>/dev/null || true)
    d2="$TD/dmp2.json"; "$CLI" sim "$FIXTURE_DMP" --fps 60 --ticks 120 --watch Mover.translation --json >"$d2" 2>/dev/null || true
    printf '%s' "$dmp" > "$TD/dmp1.json"
    cmp -s "$TD/dmp1.json" "$d2" && echo "ok:   sim damper is deterministic" || { echo "FAIL: damper non-deterministic"; failures=$(( failures + 1 )); }
    if [[ -n "$GOLDENS" && -f "$GOLDENS/sim-damper.trace.json" ]]; then
        [[ "$dmp" == "$(cat "$GOLDENS/sim-damper.trace.json")" ]] && echo "ok:   sim damper trace matches golden" || { echo "FAIL: damper golden drift"; diff <(printf '%s' "$dmp") "$GOLDENS/sim-damper.trace.json" | head -20; failures=$(( failures + 1 )); }
    fi
fi
```
(Match the harness's actual variable names — `$CLI`, `$FIXTURES`, `$GOLDENS`, `$TD`, `$failures` — by copying an existing sim-golden block's structure verbatim.)

- [ ] **Step 5: Update conformance findings + regenerate**

In `docs/conformance/findings.yaml`, set the Followers findings (FOL-9 and the other FOL-* / CONF-FOLLOWER entries covering "no System / inert") to `status: fixed` with a note: "Followers runtime shipped — DamperSystem (IIR cascade) + ChaserSystem (re-basing ramp); all 14 nodes wired via makeFollowerSystems/attachFollowers. Chaser uses a re-basing single-transition lerp (uniform incl. SFRotation slerp); literal FIR-superposition fidelity for overlapping transitions within `duration` is deferred." Then:
Run: `mise run conformance && mise run conformance-gate 2>&1 | tail -2`
Expected: gate passes (committed view matches sources). Add a one-line note to `docs/superpowers/BACKLOG.md` recording the Followers closure + the overlap-fidelity deferral.

- [ ] **Step 6: Full suite + commit**

Run: `cmake --preset dev >/dev/null && cmake --build --preset dev && ctest --preset dev -j "$(nproc)" --output-on-failure 2>&1 | tail -4`
Expected: all pass — `x3d_event_followers` + `x3d_cli_test` + every existing test; existing goldens byte-identical. Capture the tally.
```bash
git add tools/x3d-cli/fixtures/sim-damper.x3d tools/x3d-cli/goldens/sim-damper.trace.json tools/tests/x3d_cli_test.sh docs/conformance/findings.yaml docs/superpowers/BACKLOG.md
git commit -m "feat(followers): CLI damper golden + conformance closure (FOL-*)"
```

---

## Self-Review

**Spec coverage:**
- §3 damper IIR (order cascade, tau 63%, order0/tau0 passthrough, tolerance/−1→0.001, snap-before-update) → Task 2. ✓
- §3 chaser (reaches at duration, duration0 passthrough, set_value jump) → Task 3 (re-basing refinement noted). ✓
- §3 set_value/initialValue/initialDestination → Tasks 2+3 (handlers + attach seeding). ✓
- §4a FollowerArith 7 types incl. slerp + MF element-wise (FOL-9) → Task 1. ✓
- §4d registration + bridge wiring → Task 4. ✓
- §5 conformance tests (all listed behaviors) → Tasks 1-4 tests; CLI golden → Task 5. ✓
- §5 regression / existing goldens unchanged / conformance-gate → Task 5 Step 6 + Step 5. ✓
- §6 files ↔ File Structure. ✓
- §7 reuse slerp, tolerance 0.001 default → Task 1 + Task 2. ✓

**Refinements vs spec (documented, surfaced to the user):** (1) no separate Multi System classes — MF handled by `FollowerArith<MF…>` specialization in the one templated System; (2) chaser re-bases via lerp rather than literal FIR superposition — single-transition behavior identical, overlap fidelity deferred (recorded in findings.yaml). Neither changes a stated requirement (reach-at-duration, smoothness, all-type coverage).

**Placeholder scan:** No TBD/TODO. Each code step has complete code. The few "verify the generated setter/helper name and adapt" notes are grounded with the exact `grep` to run — necessary because generated-binding setter spellings (`*Unchecked`) and `Interpolation.hpp` quat API are external to this plan; the verify command + intent make them actionable, not vague.

**Type consistency:** `FollowerArith<T>::{lerp,dist}` signatures identical across Tasks 1-3; `DamperSystem<NodeT,ValueT>`/`ChaserSystem<NodeT,ValueT>` names + `attach`/`update` overrides consistent; `makeFollowerSystems()` returns `std::vector<std::shared_ptr<System>>` (matches the interpolator factory + `ctx.addSystem`); value-type↔node mapping consistent with Global Constraints (Coordinate*/TexCoord* = MF).
