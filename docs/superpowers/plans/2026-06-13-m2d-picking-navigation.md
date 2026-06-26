# M2d — Picking / Ray + Navigation Math Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** A renderer-agnostic ray/picking engine (broad-phase by M2b world AABBs, narrow-phase against geometry in local frame, closest hit) plus the navigation-math primitive (camera view matrix from the M2c-bound Viewpoint), all pure math + a node-graph traversal — golden byte-identical.

**Architecture:** `runtime/math/Mat4.hpp` gains `transformDirection` + general `inverse`; `runtime/math/Ray.hpp` (Ray); `runtime/math/Intersect.hpp` (ray-AABB/sphere/triangle); `runtime/scene/PickSystem.hpp` (DFS accumulating world transforms, broad+narrow phase, triangle extraction, `worldOf`). Wired into `X3DExecutionContext` with `pick()` + `viewMatrix()`. Reuses M2b's `geombounds::getField/getNode/hasField` + `localGeometryBounds` and `TransformSystem::localMatrix`. No codegen.

**Tech Stack:** C++20 header-only runtime; CMake + ctest via `mise run build`. **Golden gate** byte-identical: sha256 `7226b3a07e744b4aea1c3d9f34897384e5d8c085cb0c79743b8a23872c7183c0` (runtime-only — drift is a bug). `mise run build` = cmake+build+ctest; golden pytest `tests/test_golden_tree.py` under `uv run pytest`.

**Spec:** `docs/superpowers/specs/2026-06-13-m2d-picking-navigation-design.md`.

**Verified facts:**
- `Mat4` (column-major `std::array<float,16>`, element (row r,col c)=`m[c*4+r]`) has identity/operator*/transformPoint/translation/scale/rotation. Our storage == OpenGL column-major, so the standard `gluInvertMatrix` inverse applies directly.
- `Aabb` (`runtime/math/Aabb.hpp`): `min/max` SFVec3f, `empty`, `transformed`. M2b `BoundsSystem::localBounds(node)`. `geombounds::getField<T>/getNode/hasField` + `localGeometryBounds` in `runtime/scene/GeometryBounds.hpp`. `TransformSystem::localMatrix(node)` public static.
- `coord`→`Coordinate.point` (`MFVec3f`=`std::vector<SFVec3f>`); `IndexedFaceSet.coordIndex` (`MFInt32`=`std::vector<int>`, `-1`-delimited); `IndexedTriangleSet.index` (triples); `TriangleSet` implicit triples; `Sphere.radius`; `Box` (AABB-exact).
- `Viewpoint.getPosition()` SFVec3f, `X3DViewpointNode::getOrientation()` SFRotation; reflection field names `position`/`orientation`. `boundViewpoint()` (M2c).
- `X3DExecutionContext` (x3d::runtime): `buildSceneGraph(scene)`, members `transforms_`/`bounds_`/`bindings_`; `boundViewpoint()`. `Scene.rootNodes`.
- Node-graph walk: `for (auto& f : n->fields()){ if(!f.get) continue; if(f.type==X3DFieldType::SFNode)...; else if(MFNode)...; }`. Generated node types GLOBAL namespace; runtime classes x3d::runtime.
- Tests in `runtime/<area>/tests/`, wired in ROOT `CMakeLists.txt`; `runtime/math`+`runtime/scene` already on interface include dirs.

---

## File Structure

| File | Responsibility |
|------|----------------|
| `runtime/math/Mat4.hpp` (modify) | `transformDirection`, `inverse` |
| `runtime/math/Ray.hpp` (new) | Ray + pointAt |
| `runtime/math/tests/mat4_inverse_test.cpp` (new) | inverse round-trip, transformDirection, Ray |
| `runtime/math/Intersect.hpp` (new) | rayAabb/raySphere/rayTriangle |
| `runtime/math/tests/intersect_test.cpp` (new) | the three intersections |
| `runtime/scene/PickSystem.hpp` (new) | traversal, broad+narrow phase, triangle extraction, worldOf |
| `runtime/scene/tests/pick_system_test.cpp` (new) | pick Box/Sphere/IFS, closest-of-two, miss |
| `runtime/events/X3DExecutionContext.hpp` (modify) | own PickSystem; build hook; pick()/viewMatrix() |
| `runtime/events/tests/m2d_tick_test.cpp` (new) | end-to-end pick + viewMatrix |
| `CMakeLists.txt` (modify) | register 4 new test executables |

---

## Task 1: `Mat4::transformDirection` + `inverse`, and `Ray`

**Files:** Modify `runtime/math/Mat4.hpp`; create `runtime/math/Ray.hpp`, `runtime/math/tests/mat4_inverse_test.cpp`; modify `CMakeLists.txt`.

- [ ] **Step 1: Write the failing test** — `runtime/math/tests/mat4_inverse_test.cpp`:

```cpp
// mat4_inverse_test.cpp
#include "Mat4.hpp"
#include "Ray.hpp"
#include <cassert>
#include <cmath>
using namespace x3d::runtime;
static bool feq(float a, float b) { return std::fabs(a - b) < 1e-3f; }

int main() {
  // M * inverse(M) ~= I for a non-trivial translate*rotate*scale.
  Mat4 M = Mat4::translation({3,-2,5})
         * Mat4::rotation(SFRotation{0,1,0, static_cast<float>(M_PI/3)})
         * Mat4::scale({2,4,0.5f});
  Mat4 I = M * M.inverse();
  for (int c=0;c<4;++c) for (int r=0;r<4;++r)
    assert(feq(I.m[c*4+r], c==r ? 1.0f : 0.0f));

  // transformDirection ignores translation; transformPoint does not.
  Mat4 T = Mat4::translation({10,20,30});
  assert(feq(T.transformDirection({1,0,0}).x, 1) && feq(T.transformDirection({1,0,0}).y, 0));
  assert(feq(T.transformPoint({0,0,0}).x, 10));

  // singular -> identity (documented)
  Mat4 zero; // all-zero (empty Mat4)
  Mat4 zi = zero.inverse();
  assert(feq(zi.m[0],1) && feq(zi.m[5],1) && feq(zi.m[15],1));

  // Ray::pointAt
  Ray ray{{0,0,0},{0,0,-1}};
  assert(feq(ray.pointAt(5).z, -5));
  return 0;
}
```

- [ ] **Step 2: Register + run FAIL.** In `CMakeLists.txt` (near `x3d_aabb`):

```cmake
    add_executable(x3d_mat4_inverse
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/math/tests/mat4_inverse_test.cpp")
    target_link_libraries(x3d_mat4_inverse PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_mat4_inverse COMMAND x3d_mat4_inverse)
```

Run `mise run build` → FAIL (`Ray.hpp` not found / `inverse` not a member).

- [ ] **Step 3a: Create `runtime/math/Ray.hpp`:**

```cpp
// Ray.hpp — a ray (origin + direction). namespace x3d::runtime.
#ifndef X3D_RUNTIME_RAY_HPP
#define X3D_RUNTIME_RAY_HPP
#include "Mat4.hpp" // SFVec3f
namespace x3d::runtime {
struct Ray {
  SFVec3f origin{0,0,0};
  SFVec3f direction{0,0,-1};
  SFVec3f pointAt(float t) const {
    return SFVec3f{origin.x + direction.x*t, origin.y + direction.y*t, origin.z + direction.z*t};
  }
};
} // namespace x3d::runtime
#endif
```

- [ ] **Step 3b: Add `transformDirection` + `inverse` to `runtime/math/Mat4.hpp`** (inside `struct Mat4`, after `transformPoint`; ensure `#include <cmath>` is present):

```cpp
  // Transform a direction (w=0): rotation/scale only, no translation.
  SFVec3f transformDirection(const SFVec3f &d) const {
    return SFVec3f{
        m[0]*d.x + m[4]*d.y + m[8]*d.z,
        m[1]*d.x + m[5]*d.y + m[9]*d.z,
        m[2]*d.x + m[6]*d.y + m[10]*d.z};
  }

  // General 4x4 inverse (column-major == OpenGL layout, standard adjugate form).
  // Returns identity() if (near-)singular.
  Mat4 inverse() const {
    std::array<float,16> inv;
    inv[0]  =  m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15] + m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
    inv[4]  = -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15] - m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
    inv[8]  =  m[4]*m[9]*m[15]  - m[4]*m[11]*m[13] - m[8]*m[5]*m[15] + m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
    inv[12] = -m[4]*m[9]*m[14]  + m[4]*m[10]*m[13] + m[8]*m[5]*m[14] - m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
    inv[1]  = -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15] - m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
    inv[5]  =  m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15] + m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
    inv[9]  = -m[0]*m[9]*m[15]  + m[0]*m[11]*m[13] + m[8]*m[1]*m[15] - m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
    inv[13] =  m[0]*m[9]*m[14]  - m[0]*m[10]*m[13] - m[8]*m[1]*m[14] + m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
    inv[2]  =  m[1]*m[6]*m[15]  - m[1]*m[7]*m[14]  - m[5]*m[2]*m[15] + m[5]*m[3]*m[14] + m[13]*m[2]*m[7]  - m[13]*m[3]*m[6];
    inv[6]  = -m[0]*m[6]*m[15]  + m[0]*m[7]*m[14]  + m[4]*m[2]*m[15] - m[4]*m[3]*m[14] - m[12]*m[2]*m[7]  + m[12]*m[3]*m[6];
    inv[10] =  m[0]*m[5]*m[15]  - m[0]*m[7]*m[13]  - m[4]*m[1]*m[15] + m[4]*m[3]*m[13] + m[12]*m[1]*m[7]  - m[12]*m[3]*m[5];
    inv[14] = -m[0]*m[5]*m[14]  + m[0]*m[6]*m[13]  + m[4]*m[1]*m[14] - m[4]*m[2]*m[13] - m[12]*m[1]*m[6]  + m[12]*m[2]*m[5];
    inv[3]  = -m[1]*m[6]*m[11]  + m[1]*m[7]*m[10]  + m[5]*m[2]*m[11] - m[5]*m[3]*m[10] - m[9]*m[2]*m[7]   + m[9]*m[3]*m[6];
    inv[7]  =  m[0]*m[6]*m[11]  - m[0]*m[7]*m[10]  - m[4]*m[2]*m[11] + m[4]*m[3]*m[10] + m[8]*m[2]*m[7]   - m[8]*m[3]*m[6];
    inv[11] = -m[0]*m[5]*m[11]  + m[0]*m[7]*m[9]   + m[4]*m[1]*m[11] - m[4]*m[3]*m[9]  - m[8]*m[1]*m[7]   + m[8]*m[3]*m[5];
    inv[15] =  m[0]*m[5]*m[10]  - m[0]*m[6]*m[9]   - m[4]*m[1]*m[10] + m[4]*m[2]*m[9]  + m[8]*m[1]*m[6]   - m[8]*m[2]*m[5];
    float det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
    if (std::fabs(det) < 1e-20f) return identity();
    float idet = 1.0f / det;
    Mat4 r;
    for (int i = 0; i < 16; ++i) r.m[i] = inv[i] * idet;
    return r;
  }
```

If `<cmath>` / `<array>` aren't already included in `Mat4.hpp`, add them.

- [ ] **Step 4: Run PASS.** `mise run build` then `ctest --preset dev -R 'x3d_mat4_inverse|x3d_mat4'`. Golden byte-identical.

- [ ] **Step 5: Commit.**
```bash
git add runtime/math/Mat4.hpp runtime/math/Ray.hpp runtime/math/tests/mat4_inverse_test.cpp CMakeLists.txt
git commit -m "M2d: Mat4 transformDirection + general inverse, and Ray"
```

---

## Task 2: `Intersect` — ray vs AABB / sphere / triangle

**Files:** Create `runtime/math/Intersect.hpp`, `runtime/math/tests/intersect_test.cpp`; modify `CMakeLists.txt`.

- [ ] **Step 1: Write the failing test** — `runtime/math/tests/intersect_test.cpp`:

```cpp
// intersect_test.cpp
#include "Intersect.hpp"
#include <cassert>
#include <cmath>
using namespace x3d::runtime;
static bool feq(float a, float b) { return std::fabs(a - b) < 1e-3f; }

int main() {
  Aabb box = Aabb::fromCenterSize({0,0,0}, {2,2,2}); // [-1,1]^3
  // ray from +z toward -z hits front face z=1 at t=9 (origin z=10)
  auto a = rayAabb(Ray{{0,0,10},{0,0,-1}}, box);
  assert(a && feq(*a, 9));
  // miss (parallel, outside)
  assert(!rayAabb(Ray{{5,5,10},{0,0,-1}}, box));
  // origin inside -> entry t is the exit (>0)
  auto inside = rayAabb(Ray{{0,0,0},{0,0,-1}}, box);
  assert(inside && feq(*inside, 1));

  // unit sphere at origin, ray from +z
  auto s = raySphere(Ray{{0,0,10},{0,0,-1}}, 1.0f);
  assert(s && feq(*s, 9));
  assert(!raySphere(Ray{{5,0,10},{0,0,-1}}, 1.0f)); // miss
  assert(!raySphere(Ray{{0,0,10},{0,0,-1}}, 0.0f)); // radius 0 -> none

  // triangle in z=0 plane; ray from +z through (0.25,0.25)
  SFVec3f v0{0,0,0}, v1{1,0,0}, v2{0,1,0};
  auto t = rayTriangle(Ray{{0.25f,0.25f,5},{0,0,-1}}, v0,v1,v2);
  assert(t && feq(*t, 5));
  // outside the triangle
  assert(!rayTriangle(Ray{{1,1,5},{0,0,-1}}, v0,v1,v2));
  return 0;
}
```

- [ ] **Step 2: Register + run FAIL.** In `CMakeLists.txt`:

```cmake
    add_executable(x3d_intersect
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/math/tests/intersect_test.cpp")
    target_link_libraries(x3d_intersect PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_intersect COMMAND x3d_intersect)
```

Run `mise run build` → FAIL (`Intersect.hpp` not found).

- [ ] **Step 3: Implement `runtime/math/Intersect.hpp`:**

```cpp
// Intersect.hpp — ray vs AABB / sphere / triangle. Each returns the entry
// parameter t>=0 along the ray (std::optional). namespace x3d::runtime.
#ifndef X3D_RUNTIME_INTERSECT_HPP
#define X3D_RUNTIME_INTERSECT_HPP

#include "Aabb.hpp"
#include "Ray.hpp"
#include <algorithm>
#include <cmath>
#include <optional>

namespace x3d::runtime {

inline std::optional<float> rayAabb(const Ray &ray, const Aabb &box) {
  if (box.empty) return std::nullopt;
  const float o[3] = {ray.origin.x, ray.origin.y, ray.origin.z};
  const float d[3] = {ray.direction.x, ray.direction.y, ray.direction.z};
  const float lo[3] = {box.min.x, box.min.y, box.min.z};
  const float hi[3] = {box.max.x, box.max.y, box.max.z};
  float tmin = -1e30f, tmax = 1e30f;
  for (int i = 0; i < 3; ++i) {
    if (std::fabs(d[i]) < 1e-12f) {
      if (o[i] < lo[i] || o[i] > hi[i]) return std::nullopt;
    } else {
      float t1 = (lo[i] - o[i]) / d[i], t2 = (hi[i] - o[i]) / d[i];
      if (t1 > t2) std::swap(t1, t2);
      tmin = std::max(tmin, t1);
      tmax = std::min(tmax, t2);
    }
  }
  if (tmax < tmin || tmax < 0) return std::nullopt;
  return tmin >= 0 ? tmin : tmax; // entry, or (origin inside) the exit
}

inline std::optional<float> raySphere(const Ray &ray, float radius) {
  if (radius <= 0) return std::nullopt;
  const SFVec3f &o = ray.origin, &d = ray.direction;
  float a = d.x*d.x + d.y*d.y + d.z*d.z;
  if (a < 1e-20f) return std::nullopt;
  float b = 2.0f * (o.x*d.x + o.y*d.y + o.z*d.z);
  float c = o.x*o.x + o.y*o.y + o.z*o.z - radius*radius;
  float disc = b*b - 4*a*c;
  if (disc < 0) return std::nullopt;
  float s = std::sqrt(disc);
  float t1 = (-b - s) / (2*a), t2 = (-b + s) / (2*a);
  if (t1 >= 0) return t1;
  if (t2 >= 0) return t2;
  return std::nullopt;
}

inline std::optional<float> rayTriangle(const Ray &ray, const SFVec3f &v0,
                                        const SFVec3f &v1, const SFVec3f &v2) {
  auto sub = [](const SFVec3f &a, const SFVec3f &b) { return SFVec3f{a.x-b.x, a.y-b.y, a.z-b.z}; };
  auto cross = [](const SFVec3f &a, const SFVec3f &b) {
    return SFVec3f{a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
  };
  auto dot = [](const SFVec3f &a, const SFVec3f &b) { return a.x*b.x + a.y*b.y + a.z*b.z; };
  SFVec3f e1 = sub(v1, v0), e2 = sub(v2, v0);
  SFVec3f p = cross(ray.direction, e2);
  float det = dot(e1, p);
  if (std::fabs(det) < 1e-12f) return std::nullopt;
  float inv = 1.0f / det;
  SFVec3f tv = sub(ray.origin, v0);
  float u = dot(tv, p) * inv;
  if (u < 0 || u > 1) return std::nullopt;
  SFVec3f q = cross(tv, e1);
  float v = dot(ray.direction, q) * inv;
  if (v < 0 || u + v > 1) return std::nullopt;
  float t = dot(e2, q) * inv;
  return t < 0 ? std::nullopt : std::optional<float>(t);
}

} // namespace x3d::runtime
#endif
```

- [ ] **Step 4: Run PASS.** `mise run build` then `ctest --preset dev -R x3d_intersect`. Golden byte-identical.

- [ ] **Step 5: Commit.**
```bash
git add runtime/math/Intersect.hpp runtime/math/tests/intersect_test.cpp CMakeLists.txt
git commit -m "M2d: ray-AABB / ray-sphere / ray-triangle intersections"
```

---

## Task 3: `PickSystem` — traversal + broad/narrow phase + worldOf

**Files:** Create `runtime/scene/PickSystem.hpp`, `runtime/scene/tests/pick_system_test.cpp`; modify `CMakeLists.txt`.

- [ ] **Step 1: Write the failing test** — `runtime/scene/tests/pick_system_test.cpp`:

```cpp
// pick_system_test.cpp
#include "PickSystem.hpp"
#include "TransformSystem.hpp"
#include "BoundsSystem.hpp"
#include "X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp"
#include <any>
#include <cassert>
#include <cmath>
#include <memory>
#include <vector>
using namespace x3d::runtime;
static bool feq(float a, float b) { return std::fabs(a - b) < 1e-2f; }
static void setF(const std::shared_ptr<X3DNode>& n, const char* nm, std::any v) {
  for (auto& f : n->fields()) if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
static void addChild(const std::shared_ptr<X3DNode>& p, const std::shared_ptr<X3DNode>& c) {
  for (auto& f : p->fields()) if (f.x3dName == "children" && f.set) {
    auto k = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*p));
    k.push_back(c); f.set(*p, std::any(std::move(k))); return;
  }
}
static std::shared_ptr<X3DNode> shapeWith(const std::shared_ptr<X3DNode>& geom) {
  auto s = createX3DNode("Shape");
  setF(s, "geometry", std::any(std::shared_ptr<X3DNode>(geom)));
  return s;
}

int main() {
  // Transform(+5x) > Shape > Box(size 2). Ray from (5,0,10) toward -z hits z=1.
  auto T = createX3DNode("Transform"); setF(T, "translation", std::any(SFVec3f{5,0,0}));
  auto box = createX3DNode("Box"); setF(box, "size", std::any(SFVec3f{2,2,2}));
  auto shape = shapeWith(box);
  addChild(T, shape);
  Scene scene; scene.addRootNode(T);

  TransformSystem ts; ts.buildIndex(scene);
  BoundsSystem bs; bs.buildBounds(scene, ts);
  PickSystem ps; ps.build(scene);

  auto hit = ps.pickClosest(Ray{{5,0,10},{0,0,-1}}, bs);
  assert(hit.hit && hit.node == shape.get());
  assert(feq(hit.point.x,5) && feq(hit.point.z,1));
  assert(feq(hit.distance, 9));

  // Miss: ray well outside.
  auto miss = ps.pickClosest(Ray{{100,0,10},{0,0,-1}}, bs);
  assert(!miss.hit);

  // Sphere narrow-phase under no transform.
  auto sph = createX3DNode("Sphere"); setF(sph, "radius", std::any(2.0f));
  auto sshape = shapeWith(sph);
  Scene s2; s2.addRootNode(sshape);
  TransformSystem ts2; ts2.buildIndex(s2);
  BoundsSystem bs2; bs2.buildBounds(s2, ts2);
  PickSystem ps2; ps2.build(s2);
  auto hs = ps2.pickClosest(Ray{{0,0,10},{0,0,-1}}, bs2);
  assert(hs.hit && feq(hs.point.z, 2)); // sphere front at z=2

  // Closest-of-two along the same ray: a near Shape (front face z~1) and a far one
  // (translated to z=-20); the nearer Shape is returned.
  auto nearBox = createX3DNode("Box"); setF(nearBox, "size", std::any(SFVec3f{2,2,2}));
  auto farBox  = createX3DNode("Box"); setF(farBox,  "size", std::any(SFVec3f{2,2,2}));
  auto nearShape = shapeWith(nearBox);
  auto farT = createX3DNode("Transform"); setF(farT, "translation", std::any(SFVec3f{0,0,-20}));
  addChild(farT, shapeWith(farBox));
  Scene s3; s3.addRootNode(nearShape); s3.addRootNode(farT);
  TransformSystem ts3; ts3.buildIndex(s3);
  BoundsSystem bs3; bs3.buildBounds(s3, ts3);
  PickSystem ps3; ps3.build(s3);
  auto h2 = ps3.pickClosest(Ray{{0,0,10},{0,0,-1}}, bs3);
  assert(h2.hit && h2.node == nearShape.get() && feq(h2.point.z, 1));
  return 0;
}
```

- [ ] **Step 2: Register + run FAIL.** In `CMakeLists.txt`:

```cmake
    add_executable(x3d_pick_system
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/pick_system_test.cpp")
    target_link_libraries(x3d_pick_system PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_pick_system COMMAND x3d_pick_system)
```

Run `mise run build` → FAIL (`PickSystem.hpp` not found).

- [ ] **Step 3: Implement `runtime/scene/PickSystem.hpp`:**

```cpp
// PickSystem.hpp — on-demand ray pick over the scene graph. Broad phase by M2b
// world AABBs; narrow phase against geometry in its local frame. Accumulates the
// world transform during the traversal (no per-node world table needed).
// namespace x3d::runtime.
#ifndef X3D_RUNTIME_PICK_SYSTEM_HPP
#define X3D_RUNTIME_PICK_SYSTEM_HPP

#include "BoundsSystem.hpp"
#include "GeometryBounds.hpp" // geombounds::getField/getNode/hasField, localGeometryBounds
#include "Intersect.hpp"
#include "Mat4.hpp"
#include "Ray.hpp"
#include "TransformSystem.hpp"
#include "X3DNode.hpp"
#include "X3DScene.hpp"

#include <any>
#include <array>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace x3d::runtime {

struct PickResult {
  bool hit = false;
  X3DNode *node = nullptr;
  SFVec3f point{0,0,0};
  float distance = 0.0f;
};

class PickSystem {
public:
  void build(const Scene &scene) {
    roots_.clear();
    for (const auto &r : scene.rootNodes) if (r) roots_.push_back(r.get());
  }

  PickResult pickClosest(const Ray &worldRay, const BoundsSystem &bounds) const {
    PickResult best;
    for (X3DNode *r : roots_) pickNode(r, Mat4::identity(), worldRay, bounds, best);
    return best;
  }

  // Accumulated world transform at `target` (product of ancestor Transform local
  // matrices). Identity if not found. Used for camera pose in viewMatrix.
  Mat4 worldOf(const X3DNode *target) const {
    Mat4 out = Mat4::identity();
    for (X3DNode *r : roots_) if (worldOfRec(r, Mat4::identity(), target, out)) break;
    return out;
  }

private:
  static bool isTransform(const X3DNode *n) { return n && n->nodeTypeName() == "Transform"; }

  static std::vector<std::array<SFVec3f,3>> extractTriangles(const X3DNode *geom) {
    std::vector<std::array<SFVec3f,3>> tris;
    auto coord = geombounds::getNode(*geom, "coord");
    if (!coord) return tris;
    auto pts = geombounds::getField<std::vector<SFVec3f>>(*coord, "point", {});
    if (pts.empty()) return tris;
    auto ok = [&](int i) { return i >= 0 && i < static_cast<int>(pts.size()); };
    const std::string t = geom->nodeTypeName();
    if (t == "IndexedFaceSet") {
      auto idx = geombounds::getField<std::vector<int>>(*geom, "coordIndex", {});
      std::vector<int> face;
      auto flush = [&] {
        for (std::size_t k = 1; k + 1 < face.size(); ++k)
          if (ok(face[0]) && ok(face[k]) && ok(face[k+1]))
            tris.push_back({pts[face[0]], pts[face[k]], pts[face[k+1]]});
        face.clear();
      };
      for (int i : idx) { if (i < 0) flush(); else face.push_back(i); }
      if (face.size() >= 3) flush();
    } else if (t == "IndexedTriangleSet") {
      auto idx = geombounds::getField<std::vector<int>>(*geom, "index", {});
      for (std::size_t i = 0; i + 2 < idx.size(); i += 3)
        if (ok(idx[i]) && ok(idx[i+1]) && ok(idx[i+2]))
          tris.push_back({pts[idx[i]], pts[idx[i+1]], pts[idx[i+2]]});
    } else if (t == "TriangleSet") {
      for (std::size_t i = 0; i + 2 < pts.size(); i += 3)
        tris.push_back({pts[i], pts[i+1], pts[i+2]});
    }
    return tris;
  }

  static std::optional<float> narrowPhase(const X3DNode *geom, const Ray &local) {
    const std::string t = geom->nodeTypeName();
    if (t == "Sphere")
      return raySphere(local, geombounds::getField<float>(*geom, "radius", 1.0f));
    auto tris = extractTriangles(geom);
    if (!tris.empty()) {
      std::optional<float> best;
      for (const auto &tr : tris) {
        auto h = rayTriangle(local, tr[0], tr[1], tr[2]);
        if (h && (!best || *h < *best)) best = h;
      }
      return best;
    }
    return rayAabb(local, localGeometryBounds(geom)); // proxy (exact for Box)
  }

  void pickNode(const X3DNode *n, const Mat4 &worldM, const Ray &worldRay,
                const BoundsSystem &bounds, PickResult &best) const {
    Mat4 childM = isTransform(n) ? worldM * TransformSystem::localMatrix(n) : worldM;
    if (geombounds::hasField(*n, "geometry")) {
      if (auto geom = geombounds::getNode(*n, "geometry")) {
        Aabb wb = bounds.localBounds(n).transformed(worldM);
        if (rayAabb(worldRay, wb)) {
          Mat4 inv = worldM.inverse();
          Ray local{inv.transformPoint(worldRay.origin), inv.transformDirection(worldRay.direction)};
          if (auto h = narrowPhase(geom.get(), local)) {
            SFVec3f wp = worldM.transformPoint(local.pointAt(*h));
            float dx = wp.x-worldRay.origin.x, dy = wp.y-worldRay.origin.y, dz = wp.z-worldRay.origin.z;
            float d = std::sqrt(dx*dx + dy*dy + dz*dz);
            if (!best.hit || d < best.distance) {
              best.hit = true; best.node = const_cast<X3DNode*>(n); best.point = wp; best.distance = d;
            }
          }
        }
      }
    }
    forEachChild(n, [&](const X3DNode *c) { pickNode(c, childM, worldRay, bounds, best); });
  }

  bool worldOfRec(const X3DNode *n, const Mat4 &worldM, const X3DNode *target, Mat4 &out) const {
    Mat4 here = isTransform(n) ? worldM * TransformSystem::localMatrix(n) : worldM;
    if (n == target) { out = here; return true; }
    bool found = false;
    forEachChild(n, [&](const X3DNode *c) { if (!found && worldOfRec(c, here, target, out)) found = true; });
    return found;
  }

  template <class F>
  static void forEachChild(const X3DNode *n, F &&f) {
    for (const auto &fi : n->fields()) {
      if (!fi.get) continue;
      if (fi.type == X3DFieldType::SFNode) {
        auto c = std::any_cast<std::shared_ptr<X3DNode>>(fi.get(*n));
        if (c) f(c.get());
      } else if (fi.type == X3DFieldType::MFNode) {
        for (const auto &c : std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(fi.get(*n)))
          if (c) f(c.get());
      }
    }
  }

  std::vector<X3DNode *> roots_;
};

} // namespace x3d::runtime
#endif
```

- [ ] **Step 4: Run PASS.** `mise run build` then `ctest --preset dev -R x3d_pick_system`. Golden byte-identical.

- [ ] **Step 5: Commit.**
```bash
git add runtime/scene/PickSystem.hpp runtime/scene/tests/pick_system_test.cpp CMakeLists.txt
git commit -m "M2d: PickSystem broad/narrow ray pick + worldOf traversal"
```

---

## Task 4: `X3DExecutionContext` integration — `pick()` + `viewMatrix()`

**Files:** Modify `runtime/events/X3DExecutionContext.hpp`; create `runtime/events/tests/m2d_tick_test.cpp`; modify `CMakeLists.txt`.

- [ ] **Step 1: Write the failing test** — `runtime/events/tests/m2d_tick_test.cpp`:

```cpp
// m2d_tick_test.cpp — pick + viewMatrix via the context.
#include "X3DExecutionContext.hpp"
#include "X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp"
#include <any>
#include <cassert>
#include <cmath>
#include <memory>
#include <vector>
using namespace x3d::runtime;
static bool feq(float a, float b) { return std::fabs(a - b) < 1e-2f; }
static void setF(const std::shared_ptr<X3DNode>& n, const char* nm, std::any v) {
  for (auto& f : n->fields()) if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
static void addChild(const std::shared_ptr<X3DNode>& p, const std::shared_ptr<X3DNode>& c) {
  for (auto& f : p->fields()) if (f.x3dName == "children" && f.set) {
    auto k = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*p));
    k.push_back(c); f.set(*p, std::any(std::move(k))); return;
  }
}

int main() {
  auto shape = createX3DNode("Shape");
  auto box = createX3DNode("Box"); setF(box, "size", std::any(SFVec3f{2,2,2}));
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(box)));
  auto vp = createX3DNode("Viewpoint"); setF(vp, "position", std::any(SFVec3f{0,0,10}));
  Scene scene; scene.addRootNode(shape); scene.addRootNode(vp);

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);

  auto hit = ctx.pick(Ray{{0,0,10},{0,0,-1}});
  assert(hit.hit && hit.node == shape.get() && feq(hit.point.z, 1));

  // Viewpoint at (0,0,10), default orientation -> view = translate(0,0,-10).
  Mat4 view = ctx.viewMatrix();
  SFVec3f origin = view.transformPoint({0,0,0});
  assert(feq(origin.z, -10) && feq(origin.x, 0) && feq(origin.y, 0));
  return 0;
}
```

- [ ] **Step 2: Register + run FAIL.** In `CMakeLists.txt`:

```cmake
    add_executable(x3d_m2d_tick
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/m2d_tick_test.cpp")
    target_link_libraries(x3d_m2d_tick PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_m2d_tick COMMAND x3d_m2d_tick)
```

Run `mise run build` → FAIL (`pick`/`viewMatrix` not members).

- [ ] **Step 3: Wire PickSystem into the context.** In `runtime/events/X3DExecutionContext.hpp`:

Add include `#include "PickSystem.hpp"`. Add a private member next to `bindings_`:
```cpp
  PickSystem pick_;
```

In `buildSceneGraph`, after the bindings build, add:
```cpp
    pick_.build(scene);
```

Add the pull API (public), next to the bound* methods:
```cpp
  /// Closest pick along a world-space ray (PickResult.hit == false on a miss).
  PickResult pick(const Ray &worldRay) const { return pick_.pickClosest(worldRay, bounds_); }

  /// World->camera (view) matrix from the bound Viewpoint (identity if none).
  Mat4 viewMatrix() const {
    X3DNode *vp = boundViewpoint();
    if (!vp) return Mat4::identity();
    SFVec3f pos = geombounds::getField<SFVec3f>(*vp, "position", {0,0,0});
    SFRotation ori = geombounds::getField<SFRotation>(*vp, "orientation", {0,0,1,0});
    Mat4 cam = pick_.worldOf(vp) * Mat4::translation(pos) * Mat4::rotation(ori);
    return cam.inverse();
  }
```

(`geombounds::getField` comes from `GeometryBounds.hpp`, transitively included via `PickSystem.hpp`.)

- [ ] **Step 4: Run PASS (no regression).** `mise run build` then `ctest --preset dev -R 'x3d_m2d_tick|x3d_pick_system|x3d_m2c_tick|x3d_m2b_tick|x3d_m2a_tick|x3d_event|animation' --output-on-failure`. Golden byte-identical.

- [ ] **Step 5: Commit.**
```bash
git add runtime/events/X3DExecutionContext.hpp runtime/events/tests/m2d_tick_test.cpp CMakeLists.txt
git commit -m "M2d: wire PickSystem into the context (pick + viewMatrix pull API)"
```

---

## Task 5: Full verification

**Files:** none.

- [ ] **Step 1: Full build + suite.** `mise run build` → ctest all green (adds `x3d_mat4_inverse`, `x3d_intersect`, `x3d_pick_system`, `x3d_m2d_tick`); `uv run pytest` green incl. `tests/test_golden_tree.py`. No M2a/M2b/M2c/event regression.
- [ ] **Step 2: Golden byte-identical.** `bash scripts/check_golden.sh` → byte-for-byte; sha256 unchanged from `7226b3a07e744b4aea1c3d9f34897384e5d8c085cb0c79743b8a23872c7183c0`. If changed, a runtime edit leaked into a generated template — stop and fix.
- [ ] **Step 3: Record outcome.** New ctest count, golden unchanged; M2d (pick engine + viewMatrix) in place for the M2.5 extraction seam + the deferred pick-sensor/navigation-interaction work (backlog M2D-1/2/3).

---

## Notes for the implementer

- **TDD discipline:** test first, confirm RED, then GREEN.
- **Golden gate** — nothing here is codegen; sha256 must never move.
- **Frame discipline:** broad phase in WORLD space (worldRay vs world AABB); narrow phase in LOCAL space (ray × `worldM.inverse()`); distance always measured in world space from `worldRay.origin`.
- **Reuse, don't duplicate:** read fields via `geombounds::getField/getNode` (from `GeometryBounds.hpp`); transform composition via `TransformSystem::localMatrix`. Guard `if(!f.get) continue;` in graph walks.
- **Conservative proxy** for non-Box primitives / unsupported meshes (local-AABB) is intentional (backlog M2D-2) — it never misses a real hit.
- **Don't touch codegen** — all edits in `runtime/**` + `CMakeLists.txt`.
```
