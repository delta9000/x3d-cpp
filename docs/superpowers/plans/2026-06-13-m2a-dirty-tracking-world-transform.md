# M2a — Dirty-Tracking + World-Transform Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the M2 foundation — a per-node dirty-tracking layer fed by the event cascade, plus incremental world-transform propagation down the Transform hierarchy — entirely in side tables so the generated header tree stays byte-identical.

**Architecture:** New header-only runtime: `runtime/math/Mat4.hpp` (4×4 float math + X3D TRS composition), `runtime/scene/DirtyTracker.hpp` (per-node category bits + changed list), `runtime/scene/TransformSystem.hpp` (transform-hierarchy index + world-transform side table + incremental propagation). The `EventCascade` gains a field-observer hook; `X3DExecutionContext::tick` runs a post-cascade propagation pass and exposes a pull API (`changedNodes()` + `worldTransform(node)`). World transforms and dirty state are keyed by `const X3DNode*` — no codegen, golden stays byte-identical.

**Tech Stack:** C++20 header-only runtime; reflection-driven (`X3DNode::fields()`); CMake + ctest via `mise run build` (Ninja `dev` preset). Builds on existing `Quat`/`quatFromRotation` in `runtime/events/Interpolation.hpp`.

**Build/test:** `mise run build` (pytest + ctest). Single test: `ctest --preset dev -R <name> --output-on-failure`. **Golden gate** (pytest `tests/test_golden_tree.py`) must stay byte-identical: sha256 `7226b3a07e744b4aea1c3d9f34897384e5d8c085cb0c79743b8a23872c7183c0`. This whole plan is runtime-only — any golden drift is a bug.

**Spec:** `docs/superpowers/specs/2026-06-13-m2a-dirty-tracking-world-transform-design.md`.

**Key facts (verified against the tree):**
- `SFVec3f{float x,y,z;}`, `SFRotation{float x,y,z,angle;}` (axis + angle), `SFMatrix4f{float matrix[4][4];}` — in `generated_cpp_bindings/X3Dtypes.hpp`.
- `runtime/events/Interpolation.hpp` (namespace `x3d::runtime`) has `struct Quat{double x,y,z,w;}` and `inline Quat quatFromRotation(const SFRotation&)`.
- `generated_cpp_bindings/Transform.hpp`: `getTranslation/getRotation/getScale/getCenter/getScaleOrientation`, `getChildren()`, `nodeTypeName()=="Transform"`.
- `runtime/events/X3DFieldAddress.hpp`: `FieldAddress{ X3DNode* node; std::string field; }`.
- `runtime/events/X3DEventCascade.hpp`: `EventCascade::deliver` is a private `static` method (~line 87) — becomes a non-static member to notify an observer.
- Reflection read of a value field: `for (auto& f : node->fields()) if (f.x3dName==name) return std::any_cast<T>(f.get(*node));`.
- Tests live in `runtime/<area>/tests/*.cpp`, each wired in the ROOT `CMakeLists.txt` (NOT a per-area CMake).

---

## File Structure

| File | Responsibility |
|------|----------------|
| `runtime/math/Mat4.hpp` (new) | column-major 4×4 float matrix: identity, multiply, translation/scale/rotation builders, X3D TRS composition, transformPoint |
| `runtime/math/tests/mat4_test.cpp` (new) | TRS + multiply + rotation correctness |
| `runtime/scene/DirtyTracker.hpp` (new) | per-node dirty category bits + changed-node list |
| `runtime/scene/tests/dirty_tracker_test.cpp` (new) | flag accumulation + changed list |
| `runtime/events/X3DEventCascade.hpp` (modify) | field-observer hook; `deliver` notifies |
| `runtime/events/tests/cascade_observer_test.cpp` (new) | observer fires on every delivered field |
| `runtime/scene/TransformSystem.hpp` (new) | transform-hierarchy index, world-transform side table, incremental propagation |
| `runtime/scene/tests/transform_system_test.cpp` (new) | composition + incremental propagation |
| `runtime/events/X3DExecutionContext.hpp` (modify) | own DirtyTracker+TransformSystem, classifier wiring, post-cascade tick step, pull API |
| `runtime/events/tests/m2a_tick_test.cpp` (new) | end-to-end: ROUTE into Transform.translation → tick → changed-set + world transform |
| `CMakeLists.txt` (modify) | register the 5 new test executables; add `runtime/math` + `runtime/scene` to test include dirs if needed |

---

## Task 1: `Mat4` — column-major matrix math + X3D TRS composition

**Files:**
- Create: `runtime/math/Mat4.hpp`
- Create: `runtime/math/tests/mat4_test.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing test**

Create `runtime/math/tests/mat4_test.cpp`:

```cpp
// mat4_test.cpp — column-major 4x4 math + X3D Transform composition.
#include "Mat4.hpp"
#include <cassert>
#include <cmath>
using namespace x3d::runtime;

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }
static bool veq(const SFVec3f& a, const SFVec3f& b) {
  return feq(a.x,b.x) && feq(a.y,b.y) && feq(a.z,b.z);
}

int main() {
  // identity transforms a point to itself
  assert(veq(Mat4::identity().transformPoint({1,2,3}), {1,2,3}));

  // pure translation
  Mat4 t = Mat4::translation({10,20,30});
  assert(veq(t.transformPoint({1,2,3}), {11,22,33}));

  // pure scale
  Mat4 s = Mat4::scale({2,3,4});
  assert(veq(s.transformPoint({1,1,1}), {2,3,4}));

  // 90deg rotation about +Z maps +X -> +Y
  Mat4 r = Mat4::rotation(SFRotation{0,0,1, static_cast<float>(M_PI/2.0)});
  assert(veq(r.transformPoint({1,0,0}), {0,1,0}));

  // multiply order: (T * S) applied to p scales THEN translates
  Mat4 ts = t * s;
  assert(veq(ts.transformPoint({1,1,1}), {12,23,34})); // (2,3,4) then +(10,20,30)

  // X3D Transform composition: translation + scale, no rotation/center.
  Mat4 m = transformMatrix(/*translation*/{5,0,0}, /*rotation*/{0,0,1,0},
                           /*scale*/{2,2,2}, /*center*/{0,0,0},
                           /*scaleOrientation*/{0,0,1,0});
  assert(veq(m.transformPoint({1,0,0}), {7,0,0})); // 1*2 + 5

  // center offset: scale about center (1,0,0) leaves that point fixed.
  Mat4 mc = transformMatrix({0,0,0}, {0,0,1,0}, {3,3,3}, {1,0,0}, {0,0,1,0});
  assert(veq(mc.transformPoint({1,0,0}), {1,0,0}));   // center fixed point
  assert(veq(mc.transformPoint({2,0,0}), {4,0,0}));   // (2-1)*3 + 1 = 4
  return 0;
}
```

- [ ] **Step 2: Register the test + run to verify it FAILS**

In `CMakeLists.txt`, alongside the other `add_executable`/`add_test` blocks (e.g. near the proto tests), add:

```cmake
    add_executable(x3d_mat4
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/math/tests/mat4_test.cpp")
    target_link_libraries(x3d_mat4 PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_mat4 COMMAND x3d_mat4)
```

Confirm the `x3d_cpp::x3d_cpp` INTERFACE target's include dirs cover `runtime/math` (it already adds `runtime` and `generated_cpp_bindings`; `#include "Mat4.hpp"` needs `runtime/math` on the include path). If the test can't find `Mat4.hpp`, add `${CMAKE_CURRENT_SOURCE_DIR}/runtime/math` and `${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene` to the interface target's `target_include_directories` (search `target_include_directories` in CMakeLists.txt and append to the existing list). Do the same for `runtime/scene` now to avoid repeating it later.

Run: `mise run build`
Expected: FAIL to compile (`Mat4.hpp` not found).

- [ ] **Step 3: Implement `Mat4.hpp`**

Create `runtime/math/Mat4.hpp`:

```cpp
// Mat4.hpp — minimal column-major 4x4 float matrix for world-transform math.
// Storage: std::array<float,16>, COLUMN-MAJOR. Element (row r, col c) is m[c*4+r].
// A column vector p (w=1) transforms as out = M * p. Builders + X3D Transform
// composition. No external dependency. namespace x3d::runtime.
#ifndef X3D_RUNTIME_MAT4_HPP
#define X3D_RUNTIME_MAT4_HPP

#include "Interpolation.hpp" // Quat, quatFromRotation
#include "X3Dtypes.hpp"      // SFVec3f, SFRotation

#include <array>

namespace x3d::runtime {

struct Mat4 {
  std::array<float, 16> m{}; // column-major; default zero

  static Mat4 identity() {
    Mat4 r;
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
  }

  // C = (*this) * rhs, column-major: C[c*4+r] = sum_k A[k*4+r]*B[c*4+k].
  Mat4 operator*(const Mat4 &b) const {
    Mat4 c; // zero
    for (int col = 0; col < 4; ++col)
      for (int row = 0; row < 4; ++row) {
        float s = 0.0f;
        for (int k = 0; k < 4; ++k)
          s += m[k * 4 + row] * b.m[col * 4 + k];
        c.m[col * 4 + row] = s;
      }
    return c;
  }

  // Transform a point (w=1).
  SFVec3f transformPoint(const SFVec3f &p) const {
    return SFVec3f{
        m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12],
        m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13],
        m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14]};
  }

  static Mat4 translation(const SFVec3f &t) {
    Mat4 r = identity();
    r.m[12] = t.x; r.m[13] = t.y; r.m[14] = t.z;
    return r;
  }

  static Mat4 scale(const SFVec3f &s) {
    Mat4 r; // zero
    r.m[0] = s.x; r.m[5] = s.y; r.m[10] = s.z; r.m[15] = 1.0f;
    return r;
  }

  // Axis-angle SFRotation -> rotation matrix, via the existing quaternion path.
  static Mat4 rotation(const SFRotation &rot) {
    Quat q = quatFromRotation(rot); // unit quaternion (x,y,z,w)
    const float x = static_cast<float>(q.x), y = static_cast<float>(q.y),
                z = static_cast<float>(q.z), w = static_cast<float>(q.w);
    Mat4 r;
    r.m[0] = 1 - 2 * (y * y + z * z); r.m[1] = 2 * (x * y + w * z); r.m[2] = 2 * (x * z - w * y); r.m[3] = 0;
    r.m[4] = 2 * (x * y - w * z);     r.m[5] = 1 - 2 * (x * x + z * z); r.m[6] = 2 * (y * z + w * x); r.m[7] = 0;
    r.m[8] = 2 * (x * z + w * y);     r.m[9] = 2 * (y * z - w * x);     r.m[10] = 1 - 2 * (x * x + y * y); r.m[11] = 0;
    r.m[12] = 0; r.m[13] = 0; r.m[14] = 0; r.m[15] = 1;
    return r;
  }
};

// X3D Transform local matrix: M = T * C * R * SR * S * SR^-1 * C^-1
// (ISO/IEC 19775-1 Transform). Applied to a column vector right-to-left.
inline Mat4 transformMatrix(const SFVec3f &translation, const SFRotation &rotation,
                            const SFVec3f &scale, const SFVec3f &center,
                            const SFRotation &scaleOrientation) {
  Mat4 T = Mat4::translation(translation);
  Mat4 C = Mat4::translation(center);
  Mat4 Cinv = Mat4::translation(SFVec3f{-center.x, -center.y, -center.z});
  Mat4 R = Mat4::rotation(rotation);
  Mat4 SR = Mat4::rotation(scaleOrientation);
  Mat4 SRinv = Mat4::rotation(
      SFRotation{scaleOrientation.x, scaleOrientation.y, scaleOrientation.z,
                 -scaleOrientation.angle});
  Mat4 S = Mat4::scale(scale);
  return T * C * R * SR * S * SRinv * Cinv;
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_MAT4_HPP
```

- [ ] **Step 4: Run to verify it PASSES**

Run: `mise run build` then `ctest --preset dev -R x3d_mat4 --output-on-failure`
Expected: PASS. Golden byte-identical.

- [ ] **Step 5: Commit**

```bash
git add runtime/math/Mat4.hpp runtime/math/tests/mat4_test.cpp CMakeLists.txt
git commit -m "M2a: Mat4 column-major math + X3D TRS composition"
```

---

## Task 2: `DirtyTracker` — per-node category bits

**Files:**
- Create: `runtime/scene/DirtyTracker.hpp`
- Create: `runtime/scene/tests/dirty_tracker_test.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing test**

Create `runtime/scene/tests/dirty_tracker_test.cpp`:

```cpp
// dirty_tracker_test.cpp
#include "DirtyTracker.hpp"
#include <cassert>
using namespace x3d::runtime;

int main() {
  DirtyTracker d;
  int a = 0, b = 0; // stand-ins for node identities (only the pointer matters)
  const X3DNode* na = reinterpret_cast<const X3DNode*>(&a);
  const X3DNode* nb = reinterpret_cast<const X3DNode*>(&b);

  assert(d.flags(na) == DirtyNone);
  assert(d.changedNodes().empty());

  d.markDirty(na, DirtyLocalTransform);
  assert(d.flags(na) == DirtyLocalTransform);
  assert(d.changedNodes().size() == 1 && d.changedNodes()[0] == na);

  // OR-ing more flags onto the same node does NOT duplicate it in the list
  d.markDirty(na, DirtyWorldTransform);
  assert(d.flags(na) == (DirtyLocalTransform | DirtyWorldTransform));
  assert(d.changedNodes().size() == 1);

  // a second node appends
  d.markDirty(nb, DirtyField);
  assert(d.changedNodes().size() == 2);

  d.clear();
  assert(d.flags(na) == DirtyNone && d.changedNodes().empty());
  return 0;
}
```

- [ ] **Step 2: Register + run to verify FAIL**

In `CMakeLists.txt`:

```cmake
    add_executable(x3d_dirty_tracker
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/dirty_tracker_test.cpp")
    target_link_libraries(x3d_dirty_tracker PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_dirty_tracker COMMAND x3d_dirty_tracker)
```

Run: `mise run build`
Expected: FAIL (`DirtyTracker.hpp` not found).

- [ ] **Step 3: Implement `DirtyTracker.hpp`**

Create `runtime/scene/DirtyTracker.hpp`:

```cpp
// DirtyTracker.hpp — per-node dirty category bits + changed-node list.
// The HdChangeTracker analog: the cascade feeds it during a tick; the consumer
// pulls changedNodes() after the tick. Keyed by const X3DNode*. Side table —
// nothing is stored on the node. namespace x3d::runtime.
#ifndef X3D_RUNTIME_DIRTY_TRACKER_HPP
#define X3D_RUNTIME_DIRTY_TRACKER_HPP

#include <unordered_map>
#include <vector>

class X3DNode;

namespace x3d::runtime {

enum DirtyFlags : unsigned {
  DirtyNone           = 0,
  DirtyLocalTransform = 1u << 0, // a Transform's TRS field changed
  DirtyWorldTransform = 1u << 1, // world matrix recomputed this tick
  DirtyChildren       = 1u << 2, // children/addChildren/removeChildren changed
  DirtyField          = 1u << 3, // any other field changed
  // DirtyBounds = 1u << 4 reserved for M2b
};

class DirtyTracker {
public:
  void markDirty(const X3DNode *n, unsigned flags) {
    if (!n) return;
    unsigned &cur = flags_[n];
    if (cur == DirtyNone) changed_.push_back(n); // first transition -> list it
    cur |= flags;
  }

  unsigned flags(const X3DNode *n) const {
    auto it = flags_.find(n);
    return it == flags_.end() ? DirtyNone : it->second;
  }

  const std::vector<const X3DNode *> &changedNodes() const { return changed_; }

  void clear() {
    flags_.clear();
    changed_.clear();
  }

private:
  std::unordered_map<const X3DNode *, unsigned> flags_;
  std::vector<const X3DNode *> changed_;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_DIRTY_TRACKER_HPP
```

- [ ] **Step 4: Run to verify PASS**

Run: `mise run build` then `ctest --preset dev -R x3d_dirty_tracker --output-on-failure`
Expected: PASS. Golden byte-identical.

- [ ] **Step 5: Commit**

```bash
git add runtime/scene/DirtyTracker.hpp runtime/scene/tests/dirty_tracker_test.cpp CMakeLists.txt
git commit -m "M2a: DirtyTracker per-node category bits + changed list"
```

---

## Task 3: `EventCascade` field-observer hook

**Files:**
- Modify: `runtime/events/X3DEventCascade.hpp`
- Create: `runtime/events/tests/cascade_observer_test.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing test**

Create `runtime/events/tests/cascade_observer_test.cpp`:

```cpp
// cascade_observer_test.cpp — the cascade notifies an observer for every field
// it delivers (the dirty-tracking feed).
#include "X3DEventCascade.hpp"
#include "X3DEventGraph.hpp"
#include "X3DNodeFactory.hpp"
#include <cassert>
#include <string>
#include <vector>
using namespace x3d::runtime;

int main() {
  auto a = createX3DNode("TimeSensor");
  auto b = createX3DNode("PositionInterpolator");
  // ROUTE a.fraction_changed -> b.set_fraction so a delivered event fans out.
  EventGraph g;
  g.addRoute(FieldAddress{a.get(), "fraction_changed"},
             FieldAddress{b.get(), "set_fraction"});
  EventCascade cascade(g);

  std::vector<std::pair<const X3DNode*, std::string>> seen;
  cascade.setFieldObserver([&](const FieldAddress &addr) {
    seen.emplace_back(addr.node, addr.field);
  });

  cascade.postEvent(a.get(), "fraction_changed", std::any(0.5f));
  cascade.process();

  // Observer fired for the seeded delivery AND the routed sink delivery.
  bool sawSource = false, sawSink = false;
  for (auto &p : seen) {
    if (p.first == a.get() && p.second == "fraction_changed") sawSource = true;
    if (p.first == b.get() && p.second == "set_fraction") sawSink = true;
  }
  assert(sawSource && sawSink);
  return 0;
}
```

- [ ] **Step 2: Register + run to verify FAIL**

In `CMakeLists.txt`:

```cmake
    add_executable(x3d_cascade_observer
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/cascade_observer_test.cpp")
    target_link_libraries(x3d_cascade_observer PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_cascade_observer COMMAND x3d_cascade_observer)
```

Run: `mise run build`
Expected: FAIL (`setFieldObserver` not a member).

- [ ] **Step 3: Add the observer hook to `X3DEventCascade.hpp`**

Add the include `#include <functional>` near the top.

Add a public setter and a private member, and make `deliver` notify. Change `deliver` from a `static` method to a non-static member so it can read `observer_`. The current `deliver` (around lines 87-99):

```cpp
  static void deliver(const FieldAddress &addr, const std::any &value) {
    if (!addr.node) {
      return;
    }
    for (const auto &info : addr.node->fields()) {
      if (info.x3dName == addr.field) {
        if (info.set) {
          info.set(*addr.node, value);
        }
        return;
      }
    }
  }
```

Replace with:

```cpp
  // Deliver a value to one field endpoint via the node's reflection table, then
  // notify the field observer (the dirty-tracking feed). Fields with no writable
  // thunk are ignored; a ROUTE to an undeliverable field is a no-op.
  void deliver(const FieldAddress &addr, const std::any &value) {
    if (!addr.node) {
      return;
    }
    for (const auto &info : addr.node->fields()) {
      if (info.x3dName == addr.field) {
        if (info.set) {
          info.set(*addr.node, value);
          if (observer_) observer_(addr);
        }
        return;
      }
    }
  }
```

Add to the public section (e.g. after `process()`):

```cpp
  /// Register a callback invoked after each field is successfully delivered.
  /// Used by the runtime to feed the dirty-tracking layer; null by default.
  void setFieldObserver(std::function<void(const FieldAddress &)> obs) {
    observer_ = std::move(obs);
  }
```

Add to the private members (next to `pending_`):

```cpp
  std::function<void(const FieldAddress &)> observer_;
```

The call sites of `deliver` inside `process()` are already unqualified member calls (`deliver(d.target, d.value);`), so no change there.

- [ ] **Step 4: Run to verify PASS (and no cascade regression)**

Run: `mise run build` then `ctest --preset dev -R 'x3d_cascade_observer|x3d_event_cascade|x3d_event_interpolators|x3d_event_timesensor|animation' --output-on-failure`
Expected: PASS — the new test plus all existing cascade/animation tests (the static→member change must not alter cascade semantics). Golden byte-identical.

- [ ] **Step 5: Commit**

```bash
git add runtime/events/X3DEventCascade.hpp runtime/events/tests/cascade_observer_test.cpp CMakeLists.txt
git commit -m "M2a: EventCascade field-observer hook (dirty-tracking feed)"
```

---

## Task 4: `TransformSystem` — index, world table, incremental propagation

**Files:**
- Create: `runtime/scene/TransformSystem.hpp`
- Create: `runtime/scene/tests/transform_system_test.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing test**

Create `runtime/scene/tests/transform_system_test.cpp`:

```cpp
// transform_system_test.cpp — transform-hierarchy index + incremental world
// propagation. Builds A>B>C plus a sibling D under A; dirties B; asserts B and C
// world transforms update and the sibling D is NOT re-propagated.
#include "TransformSystem.hpp"
#include "DirtyTracker.hpp"
#include "X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include <cassert>
#include <cmath>
#include <memory>
using namespace x3d::runtime;

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }

// helper: set a Transform's translation via reflection (the data-layer setter).
static void setTranslation(const std::shared_ptr<X3DNode>& n, SFVec3f v) {
  for (auto& f : n->fields())
    if (f.x3dName == "translation" && f.set) { f.set(*n, std::any(v)); return; }
}
static void addChild(const std::shared_ptr<X3DNode>& parent,
                     const std::shared_ptr<X3DNode>& child) {
  for (auto& f : parent->fields())
    if (f.x3dName == "children" && f.set) {
      auto kids = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*parent));
      kids.push_back(child);
      f.set(*parent, std::any(std::move(kids)));
      return;
    }
}

int main() {
  auto A = createX3DNode("Transform");
  auto B = createX3DNode("Transform");
  auto C = createX3DNode("Transform");
  auto D = createX3DNode("Transform"); // sibling of B under A
  setTranslation(A, {1,0,0});
  setTranslation(B, {0,2,0});
  setTranslation(C, {0,0,3});
  setTranslation(D, {5,5,5});
  addChild(A, B); addChild(B, C); addChild(A, D);

  Scene scene; scene.addRootNode(A);

  TransformSystem ts;
  ts.buildIndex(scene);

  // Initial world transforms (column-translation = sum down the chain).
  assert(feq(ts.worldTransform(A.get()).transformPoint({0,0,0}).x, 1));
  // C world translation = A + B + C = (1,2,3)
  auto cw = ts.worldTransform(C.get()).transformPoint({0,0,0});
  assert(feq(cw.x,1) && feq(cw.y,2) && feq(cw.z,3));
  auto dw = ts.worldTransform(D.get()).transformPoint({0,0,0});
  assert(feq(dw.x,6) && feq(dw.y,5) && feq(dw.z,5)); // A(1,0,0)+D(5,5,5)

  // Change B's local translation, mark it dirty, propagate.
  setTranslation(B, {0,10,0});
  DirtyTracker dirty;
  dirty.markDirty(B.get(), DirtyLocalTransform);
  ts.propagate(dirty);

  // B and C world transforms updated; D untouched.
  auto bw2 = ts.worldTransform(B.get()).transformPoint({0,0,0});
  assert(feq(bw2.x,1) && feq(bw2.y,10) && feq(bw2.z,0));
  auto cw2 = ts.worldTransform(C.get()).transformPoint({0,0,0});
  assert(feq(cw2.x,1) && feq(cw2.y,10) && feq(cw2.z,3));
  // Incremental: B and C marked DirtyWorldTransform; D and A are NOT.
  assert(dirty.flags(B.get()) & DirtyWorldTransform);
  assert(dirty.flags(C.get()) & DirtyWorldTransform);
  assert(!(dirty.flags(D.get()) & DirtyWorldTransform));
  assert(!(dirty.flags(A.get()) & DirtyWorldTransform));
  return 0;
}
```

- [ ] **Step 2: Register + run to verify FAIL**

In `CMakeLists.txt`:

```cmake
    add_executable(x3d_transform_system
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/transform_system_test.cpp")
    target_link_libraries(x3d_transform_system PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_transform_system COMMAND x3d_transform_system)
```

Run: `mise run build`
Expected: FAIL (`TransformSystem.hpp` not found).

- [ ] **Step 3: Implement `TransformSystem.hpp`**

Create `runtime/scene/TransformSystem.hpp`:

```cpp
// TransformSystem.hpp — transform-hierarchy index + world-transform side table
// + incremental world-transform propagation. M2a scope: Transform nodes only.
// World transforms are stored in a side table keyed by const X3DNode* (nothing
// on the node). namespace x3d::runtime.
#ifndef X3D_RUNTIME_TRANSFORM_SYSTEM_HPP
#define X3D_RUNTIME_TRANSFORM_SYSTEM_HPP

#include "DirtyTracker.hpp"
#include "Mat4.hpp"
#include "X3DNode.hpp"
#include "X3DScene.hpp"

#include <any>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace x3d::runtime {

class TransformSystem {
public:
  /// Build the Transform hierarchy index + initial world transforms from a Scene.
  void buildIndex(const Scene &scene) {
    parent_.clear(); children_.clear(); world_.clear();
    for (const auto &root : scene.rootNodes)
      if (root) walk(root.get(), /*parentTransform=*/nullptr);
  }

  /// World matrix for a Transform node (identity if not indexed).
  Mat4 worldTransform(const X3DNode *n) const {
    auto it = world_.find(n);
    return it == world_.end() ? Mat4::identity() : it->second;
  }

  /// Recompute world transforms for every dirtied subtree, marking each
  /// recomputed node DirtyWorldTransform. Only subtrees under a dirtied local
  /// transform are revisited (incremental).
  void propagate(DirtyTracker &dirty) {
    // Collect the dirtied Transform roots: nodes flagged DirtyLocalTransform with
    // NO ancestor also so flagged (the ancestor's subtree pass covers them).
    std::vector<const X3DNode *> roots;
    for (const X3DNode *n : dirty.changedNodes()) {
      if (!(dirty.flags(n) & DirtyLocalTransform)) continue;
      if (!parent_.count(n) && !world_.count(n)) continue; // not a known Transform
      bool ancestorDirty = false;
      for (const X3DNode *p = parentOf(n); p; p = parentOf(p))
        if (dirty.flags(p) & DirtyLocalTransform) { ancestorDirty = true; break; }
      if (!ancestorDirty) roots.push_back(n);
    }
    std::unordered_set<const X3DNode *> visited;
    for (const X3DNode *r : roots)
      recompute(r, worldTransform(parentOf(r)), dirty, visited);
  }

private:
  const X3DNode *parentOf(const X3DNode *n) const {
    auto it = parent_.find(n);
    return it == parent_.end() ? nullptr : it->second;
  }

  // Read a Transform's local matrix from its TRS fields via reflection.
  static Mat4 localMatrix(const X3DNode *n) {
    return transformMatrix(getVec(n, "translation"), getRot(n, "rotation"),
                           getVec(n, "scale"), getVec(n, "center"),
                           getRot(n, "scaleOrientation"));
  }
  static SFVec3f getVec(const X3DNode *n, const std::string &name) {
    for (const auto &f : n->fields())
      if (f.x3dName == name) return std::any_cast<SFVec3f>(f.get(*n));
    return SFVec3f{0, 0, 0};
  }
  static SFRotation getRot(const X3DNode *n, const std::string &name) {
    for (const auto &f : n->fields())
      if (f.x3dName == name) return std::any_cast<SFRotation>(f.get(*n));
    return SFRotation{0, 0, 1, 0};
  }

  static bool isTransform(const X3DNode *n) {
    return n && n->nodeTypeName() == "Transform";
  }

  // DFS the scene graph over node-typed fields; index Transforms + seed worlds.
  void walk(const X3DNode *n, const X3DNode *parentTransform) {
    const X3DNode *nextParent = parentTransform;
    if (isTransform(n)) {
      parent_[n] = parentTransform;
      if (parentTransform) children_[parentTransform].push_back(n);
      world_[n] = (parentTransform ? worldTransform(parentTransform) : Mat4::identity())
                  * localMatrix(n);
      nextParent = n;
    }
    for (const auto &f : n->fields()) {
      if (f.type == X3DFieldType::SFNode) {
        auto c = std::any_cast<std::shared_ptr<X3DNode>>(f.get(*n));
        if (c) walk(c.get(), nextParent);
      } else if (f.type == X3DFieldType::MFNode) {
        for (const auto &c :
             std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*n)))
          if (c) walk(c.get(), nextParent);
      }
    }
  }

  // Recompute world = parentWorld * local for `n`, then recurse child Transforms.
  void recompute(const X3DNode *n, const Mat4 &parentWorld, DirtyTracker &dirty,
                 std::unordered_set<const X3DNode *> &visited) {
    if (!visited.insert(n).second) return;
    Mat4 w = parentWorld * localMatrix(n);
    world_[n] = w;
    dirty.markDirty(n, DirtyWorldTransform);
    auto it = children_.find(n);
    if (it == children_.end()) return;
    for (const X3DNode *c : it->second)
      recompute(c, w, dirty, visited);
  }

  std::unordered_map<const X3DNode *, const X3DNode *> parent_;
  std::unordered_map<const X3DNode *, std::vector<const X3DNode *>> children_;
  std::unordered_map<const X3DNode *, Mat4> world_;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_TRANSFORM_SYSTEM_HPP
```

Note: `X3DFieldType::SFNode`/`MFNode` and `FieldInfo` (`f.type`, `f.x3dName`, `f.get`) come from `X3DReflection.hpp` (already pulled in transitively via `X3DNode.hpp`); if the type enum isn't visible, add `#include "X3DReflection.hpp"`.

- [ ] **Step 4: Run to verify PASS**

Run: `mise run build` then `ctest --preset dev -R x3d_transform_system --output-on-failure`
Expected: PASS. Golden byte-identical.

- [ ] **Step 5: Commit**

```bash
git add runtime/scene/TransformSystem.hpp runtime/scene/tests/transform_system_test.cpp CMakeLists.txt
git commit -m "M2a: TransformSystem index + incremental world-transform propagation"
```

---

## Task 5: `X3DExecutionContext` integration + pull API

**Files:**
- Modify: `runtime/events/X3DExecutionContext.hpp`
- Create: `runtime/events/tests/m2a_tick_test.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing test**

Create `runtime/events/tests/m2a_tick_test.cpp`:

```cpp
// m2a_tick_test.cpp — end-to-end: after buildSceneGraph, a ROUTE delivering into
// a Transform's translation during tick() marks it dirty and updates its world
// transform; the consumer pulls the changed-set + world matrix after tick.
#include "X3DExecutionContext.hpp"
#include "DirtyTracker.hpp"
#include "X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include <cassert>
#include <cmath>
#include <memory>
using namespace x3d::runtime;

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }
static void addChild(const std::shared_ptr<X3DNode>& parent,
                     const std::shared_ptr<X3DNode>& child) {
  for (auto& f : parent->fields())
    if (f.x3dName == "children" && f.set) {
      auto kids = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*parent));
      kids.push_back(child); f.set(*parent, std::any(std::move(kids))); return;
    }
}

int main() {
  auto root = createX3DNode("Transform");
  auto T = createX3DNode("Transform");
  addChild(root, T);
  Scene scene; scene.addRootNode(root);

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);

  // Seed a translation event onto T and tick. (Directly posting the event models
  // what a ROUTE into T.translation delivers during the cascade.)
  ctx.postEvent(T.get(), "translation", std::any(SFVec3f{4, 0, 0}));
  ctx.tick(0.0);

  // Pull: T appears in the changed-set, flagged local + world dirty, and its
  // world transform reflects the new translation.
  bool found = false;
  for (const X3DNode* n : ctx.dirtyTracker().changedNodes())
    if (n == T.get()) found = true;
  assert(found);
  assert(ctx.dirtyTracker().flags(T.get()) & DirtyLocalTransform);
  assert(ctx.dirtyTracker().flags(T.get()) & DirtyWorldTransform);
  auto w = ctx.worldTransform(T.get()).transformPoint({0,0,0});
  assert(feq(w.x,4) && feq(w.y,0) && feq(w.z,0));

  // A second tick with no events clears the prior changed-set.
  ctx.tick(1.0);
  assert(ctx.dirtyTracker().changedNodes().empty());
  return 0;
}
```

- [ ] **Step 2: Register + run to verify FAIL**

In `CMakeLists.txt`:

```cmake
    add_executable(x3d_m2a_tick
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/m2a_tick_test.cpp")
    target_link_libraries(x3d_m2a_tick PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_m2a_tick COMMAND x3d_m2a_tick)
```

Run: `mise run build`
Expected: FAIL (`buildSceneGraph` / `dirtyTracker` / `worldTransform` not members).

- [ ] **Step 3: Wire the dirty layer + transform system into the context**

In `runtime/events/X3DExecutionContext.hpp`:

Add includes near the top:
```cpp
#include "DirtyTracker.hpp"
#include "TransformSystem.hpp"
```

Add private members (next to `systems_`):
```cpp
  DirtyTracker dirty_;
  TransformSystem transforms_;
```

Add a public `buildSceneGraph` that indexes the hierarchy and wires the cascade
observer to the classifier (place after `buildFrom`'s declaration):
```cpp
  /// Build the M2a scene-graph layer for a parsed Scene: index the Transform
  /// hierarchy and route the cascade's field deliveries into the dirty tracker.
  void buildSceneGraph(Scene &scene) {
    transforms_.buildIndex(scene);
    cascade_.setFieldObserver(
        [this](const FieldAddress &a) { classifyDirty(a); });
  }
```

Add the classifier as a private method:
```cpp
  /// Map a delivered field to dirty flags on its node.
  void classifyDirty(const FieldAddress &a) {
    if (!a.node) return;
    static const char *kTRS[] = {"translation", "rotation", "scale", "center",
                                 "scaleOrientation"};
    static const char *kChildren[] = {"children", "addChildren", "removeChildren"};
    unsigned flags = DirtyField;
    bool isTransform = a.node->nodeTypeName() == "Transform";
    for (const char *f : kChildren)
      if (a.field == f) flags = DirtyChildren;
    if (isTransform)
      for (const char *f : kTRS)
        if (a.field == f) flags = DirtyLocalTransform;
    dirty_.markDirty(a.node, flags);
  }
```

Change `tick` to clear the prior changed-set first and run the post-cascade
propagation. Replace the current `tick`:
```cpp
  void tick(double now) {
    now_ = now;
    dirty_.clear();                       // drop last tick's changed-set
    for (const auto &s : systems_) {
      s->update(now, *this);
    }
    cascade_.process();                   // observer fills dirty_
    transforms_.propagate(dirty_);        // dirtied locals -> world transforms
  }
```

Add the pull API (public):
```cpp
  /// Pull surface (read after tick): the per-node dirty set for this tick.
  const DirtyTracker &dirtyTracker() const { return dirty_; }
  /// Pull surface: world transform of a Transform node (identity if unknown).
  Mat4 worldTransform(const X3DNode *n) const {
    return transforms_.worldTransform(n);
  }
```

Note: `process()` (the bare drain) does NOT clear `dirty_` — only `tick` owns the
per-tick clear. Leave `process()` as-is.

- [ ] **Step 4: Run to verify PASS (and no regression)**

Run: `mise run build` then `ctest --preset dev -R 'x3d_m2a_tick|x3d_event|animation|scene_bridge' --output-on-failure`
Expected: PASS — the new test plus existing event/animation/bridge tests (the `tick` change adds a clear + a post-cascade pass; existing tests that call `tick` without `buildSceneGraph` still work because `dirty_` stays empty and `transforms_` has an empty index, so `propagate` is a no-op). Golden byte-identical.

- [ ] **Step 5: Commit**

```bash
git add runtime/events/X3DExecutionContext.hpp runtime/events/tests/m2a_tick_test.cpp CMakeLists.txt
git commit -m "M2a: wire DirtyTracker + TransformSystem into the execution context tick + pull API"
```

---

## Task 6: Full verification

**Files:** none (verification only)

- [ ] **Step 1: Full build + test suite**

Run: `mise run build`
Expected: pytest all green (incl. `tests/test_golden_tree.py`), ctest all green (now includes `x3d_mat4`, `x3d_dirty_tracker`, `x3d_cascade_observer`, `x3d_transform_system`, `x3d_m2a_tick`). No existing event/animation/bridge test regressed.

- [ ] **Step 2: Confirm golden byte-identical**

Run: `bash scripts/check_golden.sh`
Expected: "Golden tree OK ... byte-for-byte." sha256 unchanged from
`7226b3a07e744b4aea1c3d9f34897384e5d8c085cb0c79743b8a23872c7183c0`. **If it
changed, a runtime edit leaked into a generated template — stop and fix.**

- [ ] **Step 3: Record outcome**

No code commit needed. Note the new ctest count, golden status (unchanged), and that
the M2a foundation (dirty tracking + world transforms) is in place for M2b (bounds)
to build on `DirtyBounds` + `worldTransform`.

---

## Notes for the implementer

- **TDD discipline:** every task writes the test first and confirms RED before GREEN.
- **Golden gate is the safety net:** nothing here is codegen; the golden sha256 must never move. Any movement is a bug in your edit.
- **Column-major convention** is fixed in `Mat4.hpp` — keep TRS factor order and `operator*` consistent; the `mat4_test` pins it.
- **Reflection, not casts:** read Transform TRS fields via `node->fields()` (node-agnostic), matching the cascade/codec style — do not `dynamic_cast` to the generated `Transform`.
- **Known limitations (documented, not bugs):** DEF/USE shared nodes get one canonical parent in the index (a USE'd Transform under two parents has one world transform — deferred); runtime structural mutation (add/remove children) flags `DirtyChildren` but does not re-index (M2a builds the index once); system direct field writes are not auto-tracked (call `markDirty` explicitly); non-Transform transform-bearing nodes (HAnim) are out of scope.
- **Don't touch codegen** — all edits live in `runtime/**` + `CMakeLists.txt`.
