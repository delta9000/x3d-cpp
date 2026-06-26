# M2b — Bounding Volumes Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add axis-aligned bounding volumes — a per-node local-frame AABB computed from geometry, unioned bottom-up through the grouping hierarchy, with world-space bounds as a lazy query composing M2a's `worldTransform()` — entirely in side tables so the generated header tree stays byte-identical.

**Architecture:** New header-only runtime: `runtime/math/Aabb.hpp` (AABB type), `runtime/scene/GeometryBounds.hpp` (`localGeometryBounds` type dispatch over primitives + a reflection-generic `coord`/`controlPoint` mesh path + ElevationGrid/Extrusion/Text/Geo), `runtime/scene/BoundsSystem.hpp` (full-graph parent index + local-bounds side table + bottom-up build/propagate + world-bounds query). Wired into `X3DExecutionContext::tick` after the M2a transform pass, with a pull API. Side tables keyed by `const X3DNode*` — no codegen, golden byte-identical.

**Tech Stack:** C++20 header-only runtime; reflection-driven (`X3DNode::fields()`); CMake + ctest via `mise run build` (Ninja `dev` preset). Builds on M2a `runtime/math/Mat4.hpp`, `runtime/scene/DirtyTracker.hpp`, `runtime/scene/TransformSystem.hpp`.

**Build/test:** `mise run build`. Single test: `ctest --preset dev -R <name> --output-on-failure`. **Golden gate** must stay byte-identical: sha256 `7226b3a07e744b4aea1c3d9f34897384e5d8c085cb0c79743b8a23872c7183c0`. Runtime-only — any golden drift is a bug. (`mise run build` = cmake+build+ctest; the golden pytest test `tests/test_golden_tree.py` runs under `mise run ci`/`uv run pytest` — verify golden by both the sha256 check AND that test.)

**Spec:** `docs/superpowers/specs/2026-06-13-m2b-bounding-volumes-design.md`.

**Verified facts:**
- `Box.getSize()` SFVec3f; `Sphere.getRadius()` SFFloat; `Cone.getBottomRadius()/getHeight()`; `Cylinder.getRadius()/getHeight()`.
- Coord-based geometry share an SFNode field `coord` → `Coordinate`/`CoordinateDouble` with `getPoint()` (MFVec3f / MFVec3d). NURBS surfaces expose `controlPoint` (resolve like `coord`).
- `ElevationGrid`: `xDimension/zDimension` (SFInt32), `xSpacing/zSpacing` (SFFloat), `height` (MFFloat). `GeoElevationGrid` has the same dim/spacing/height fields.
- `Extrusion`: `crossSection` (MFVec2f), `spine` (MFVec3f), `scale` (MFVec2f).
- `Text`: `string` (MFString), `length` (MFFloat), `maxExtent` (SFFloat), `fontStyle` (SFNode → `FontStyle.getSize()` SFFloat, `getSpacing()` SFFloat).
- `X3DBoundedObject`: `bboxCenter` (SFVec3f), `bboxSize` (SFVec3f, default `-1 -1 -1`).
- M2a: `Mat4` (`transformPoint`, `operator*`), `worldTransform(node)`, `DirtyTracker`/`DirtyBounds (1u<<4)`, `TransformSystem` (its `localMatrix` is a **private** static — Task 4 makes it public).
- Field reflection: `for (auto& f : node->fields()) if (f.x3dName==name) { if(!f.get) ...; return std::any_cast<T>(f.get(*node)); }`. **Guard `if(!f.get) continue;`** — InputOnly MFNode fields (`addChildren`) have null getters (the M2a Task-4 trap).
- Tests live in `runtime/<area>/tests/*.cpp`, wired in the ROOT `CMakeLists.txt`. `runtime/math` + `runtime/scene` are already on the interface target's include dirs (added in M2a).
- SFVec2f is `{float x,y;}`, SFVec3f `{float x,y,z;}` (X3Dtypes.hpp). `createX3DNode(type)` builds a node.

---

## File Structure

| File | Responsibility |
|------|----------------|
| `runtime/math/Aabb.hpp` (new) | AABB: expand/union/transformed/center/size/fromCenterSize |
| `runtime/math/tests/aabb_test.cpp` (new) | union, expand, transformed-by-rotation |
| `runtime/scene/GeometryBounds.hpp` (new) | `localGeometryBounds` + reflection field helpers; primitives + coord/controlPoint path (Task 2); ElevationGrid/Extrusion/Text/Geo (Task 3) |
| `runtime/scene/tests/geometry_bounds_test.cpp` (new) | each geometry family (grown across Task 2 + 3) |
| `runtime/scene/TransformSystem.hpp` (modify) | make `localMatrix` a public static |
| `runtime/scene/BoundsSystem.hpp` (new) | parent index, local-bounds table, build + incremental propagate, world-bounds query |
| `runtime/scene/tests/bounds_system_test.cpp` (new) | build, author override, incremental |
| `runtime/events/X3DExecutionContext.hpp` (modify) | own BoundsSystem; build + tick step; DirtyBounds classify; pull API |
| `runtime/events/tests/m2b_tick_test.cpp` (new) | end-to-end build + worldBounds via tick |
| `CMakeLists.txt` (modify) | register 4 new test executables |

---

## Task 1: `Aabb` — axis-aligned bounding box

**Files:** Create `runtime/math/Aabb.hpp`, `runtime/math/tests/aabb_test.cpp`; modify `CMakeLists.txt`.

- [ ] **Step 1: Write the failing test** — `runtime/math/tests/aabb_test.cpp`:

```cpp
// aabb_test.cpp
#include "Aabb.hpp"
#include "Mat4.hpp"
#include <cassert>
#include <cmath>
using namespace x3d::runtime;
static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }

int main() {
  Aabb e;                       // empty is union identity
  assert(e.empty);
  Aabb b = Aabb::fromCenterSize({0,0,0}, {2,2,2}); // [-1,1]^3
  assert(feq(b.min.x,-1) && feq(b.max.z,1) && !b.empty);
  assert(feq(b.center().y,0) && feq(b.size().x,2));

  e.unionWith(b);               // empty ∪ b == b
  assert(feq(e.min.x,-1) && feq(e.max.x,1));

  Aabb c = Aabb::fromCenterSize({3,0,0}, {2,2,2}); // [2,4]x[-1,1]x[-1,1]
  b.unionWith(c);
  assert(feq(b.min.x,-1) && feq(b.max.x,4));

  // transform [-0.5,0.5]^3 by 45deg about +Z: x/y half-extent -> 0.5*(cos45+sin45)=0.7071
  Aabb u = Aabb::fromCenterSize({0,0,0}, {1,1,1});
  Aabb r = u.transformed(Mat4::rotation(SFRotation{0,0,1, static_cast<float>(M_PI/4)}));
  assert(feq(r.max.x, 0.70710f) && feq(r.max.y, 0.70710f) && feq(r.max.z, 0.5f));

  Aabb t = Aabb{}.transformed(Mat4::identity()); // empty stays empty
  assert(t.empty);
  return 0;
}
```

- [ ] **Step 2: Register + run to verify FAIL.** In `CMakeLists.txt` (near the M2a `x3d_mat4` block):

```cmake
    add_executable(x3d_aabb
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/math/tests/aabb_test.cpp")
    target_link_libraries(x3d_aabb PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_aabb COMMAND x3d_aabb)
```

Run `mise run build` → FAIL (`Aabb.hpp` not found).

- [ ] **Step 3: Implement `runtime/math/Aabb.hpp`:**

```cpp
// Aabb.hpp — axis-aligned bounding box (float). Empty is the union identity.
// namespace x3d::runtime.
#ifndef X3D_RUNTIME_AABB_HPP
#define X3D_RUNTIME_AABB_HPP

#include "Mat4.hpp"      // Mat4, SFVec3f
#include <algorithm>

namespace x3d::runtime {

struct Aabb {
  SFVec3f min{0,0,0}, max{0,0,0};
  bool empty = true;

  void expand(const SFVec3f &p) {
    if (empty) { min = max = p; empty = false; return; }
    min.x = std::min(min.x, p.x); min.y = std::min(min.y, p.y); min.z = std::min(min.z, p.z);
    max.x = std::max(max.x, p.x); max.y = std::max(max.y, p.y); max.z = std::max(max.z, p.z);
  }
  void unionWith(const Aabb &o) {
    if (o.empty) return;
    expand(o.min); expand(o.max);
  }
  Aabb transformed(const Mat4 &m) const {
    if (empty) return *this;
    Aabb r;
    for (int i = 0; i < 8; ++i) {
      SFVec3f c{ (i & 1) ? max.x : min.x, (i & 2) ? max.y : min.y, (i & 4) ? max.z : min.z };
      r.expand(m.transformPoint(c));
    }
    return r;
  }
  SFVec3f center() const {
    return empty ? SFVec3f{0,0,0}
                 : SFVec3f{(min.x+max.x)*0.5f, (min.y+max.y)*0.5f, (min.z+max.z)*0.5f};
  }
  SFVec3f size() const {
    return empty ? SFVec3f{0,0,0} : SFVec3f{max.x-min.x, max.y-min.y, max.z-min.z};
  }
  static Aabb fromCenterSize(const SFVec3f &c, const SFVec3f &s) {
    Aabb r;
    SFVec3f h{s.x*0.5f, s.y*0.5f, s.z*0.5f};
    r.min = {c.x-h.x, c.y-h.y, c.z-h.z};
    r.max = {c.x+h.x, c.y+h.y, c.z+h.z};
    r.empty = false;
    return r;
  }
};

} // namespace x3d::runtime
#endif // X3D_RUNTIME_AABB_HPP
```

- [ ] **Step 4: Run to verify PASS.** `mise run build` then `ctest --preset dev -R x3d_aabb --output-on-failure`. Golden byte-identical.

- [ ] **Step 5: Commit.**
```bash
git add runtime/math/Aabb.hpp runtime/math/tests/aabb_test.cpp CMakeLists.txt
git commit -m "M2b: Aabb axis-aligned bounding box"
```

---

## Task 2: `GeometryBounds` — primitives + reflection-generic mesh path

**Files:** Create `runtime/scene/GeometryBounds.hpp`, `runtime/scene/tests/geometry_bounds_test.cpp`; modify `CMakeLists.txt`.

- [ ] **Step 1: Write the failing test** — `runtime/scene/tests/geometry_bounds_test.cpp`:

```cpp
// geometry_bounds_test.cpp
#include "GeometryBounds.hpp"
#include "X3DNodeFactory.hpp"
#include <any>
#include <cassert>
#include <cmath>
#include <memory>
#include <vector>
using namespace x3d::runtime;
static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }
static void setF(const std::shared_ptr<X3DNode>& n, const char* name, std::any v) {
  for (auto& f : n->fields()) if (f.x3dName == name && f.set) { f.set(*n, std::move(v)); return; }
}

int main() {
  auto box = createX3DNode("Box");
  setF(box, "size", std::any(SFVec3f{2,4,6}));
  Aabb b = localGeometryBounds(box.get());
  assert(feq(b.min.x,-1) && feq(b.max.y,2) && feq(b.max.z,3));

  auto sph = createX3DNode("Sphere");
  setF(sph, "radius", std::any(2.5f));
  Aabb s = localGeometryBounds(sph.get());
  assert(feq(s.min.x,-2.5f) && feq(s.max.z,2.5f));

  auto cyl = createX3DNode("Cylinder");
  setF(cyl, "radius", std::any(1.0f)); setF(cyl, "height", std::any(10.0f));
  Aabb c = localGeometryBounds(cyl.get());
  assert(feq(c.min.x,-1) && feq(c.max.y,5) && feq(c.min.y,-5));

  // IndexedFaceSet with a Coordinate -> AABB over points
  auto ifs = createX3DNode("IndexedFaceSet");
  auto coord = createX3DNode("Coordinate");
  setF(coord, "point", std::any(std::vector<SFVec3f>{{0,0,0},{1,2,3},{-1,-1,0}}));
  setF(ifs, "coord", std::any(std::shared_ptr<X3DNode>(coord)));
  Aabb m = localGeometryBounds(ifs.get());
  assert(feq(m.min.x,-1) && feq(m.min.y,-1) && feq(m.max.y,2) && feq(m.max.z,3));

  // unknown / empty geometry -> empty
  auto txt = createX3DNode("WorldInfo"); // not geometry
  assert(localGeometryBounds(txt.get()).empty);
  return 0;
}
```

Note: confirm `Coordinate.point` is `MFVec3f` == `std::vector<SFVec3f>` in this tree (verified). If MFVec3f is a distinct struct wrapper, adapt the `std::any(...)` payload type accordingly by checking `generated_cpp_bindings/X3Dtypes.hpp` for the `MFVec3f` alias.

- [ ] **Step 2: Register + run FAIL.** In `CMakeLists.txt`:

```cmake
    add_executable(x3d_geometry_bounds
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/geometry_bounds_test.cpp")
    target_link_libraries(x3d_geometry_bounds PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_geometry_bounds COMMAND x3d_geometry_bounds)
```

Run `mise run build` → FAIL (`GeometryBounds.hpp` not found).

- [ ] **Step 3: Implement `runtime/scene/GeometryBounds.hpp` (primitives + coord/controlPoint path; long tail returns empty for now):**

```cpp
// GeometryBounds.hpp — local-frame AABB of a geometry node, by type dispatch +
// reflection field reads. namespace x3d::runtime.
#ifndef X3D_RUNTIME_GEOMETRY_BOUNDS_HPP
#define X3D_RUNTIME_GEOMETRY_BOUNDS_HPP

#include "Aabb.hpp"
#include "X3DNode.hpp"
#include <any>
#include <memory>
#include <string>
#include <vector>

namespace x3d::runtime {
namespace geombounds {

// Safe reflection read: returns dflt if the field is absent / unreadable / wrong type.
template <class T>
T getField(const X3DNode &n, const char *name, T dflt) {
  for (const auto &f : n.fields())
    if (f.x3dName == name) {
      if (!f.get) return dflt;
      try { return std::any_cast<T>(f.get(n)); } catch (...) { return dflt; }
    }
  return dflt;
}

inline std::shared_ptr<X3DNode> getNode(const X3DNode &n, const char *name) {
  return getField<std::shared_ptr<X3DNode>>(n, name, nullptr);
}

inline bool hasField(const X3DNode &n, const char *name) {
  for (const auto &f : n.fields()) if (f.x3dName == name) return true;
  return false;
}

// AABB over a Coordinate-like node's "point" (MFVec3f). Empty if none.
inline Aabb pointsBounds(const std::shared_ptr<X3DNode> &coordNode) {
  Aabb r;
  if (!coordNode) return r;
  auto pts = getField<std::vector<SFVec3f>>(*coordNode, "point", {});
  for (const auto &p : pts) r.expand(p);
  return r;
}

} // namespace geombounds

/// Local-frame AABB of `geom` (the node in a Shape's `geometry` slot). Empty for
/// unsupported / null / degenerate geometry.
inline Aabb localGeometryBounds(const X3DNode *geom) {
  using namespace geombounds;
  if (!geom) return {};
  const std::string t = geom->nodeTypeName();

  if (t == "Box")
    return Aabb::fromCenterSize({0,0,0}, getField<SFVec3f>(*geom, "size", {2,2,2}));
  if (t == "Sphere") {
    float r = getField<float>(*geom, "radius", 1.0f);
    return Aabb::fromCenterSize({0,0,0}, {2*r, 2*r, 2*r});
  }
  if (t == "Cone") {
    float br = getField<float>(*geom, "bottomRadius", 1.0f);
    float h  = getField<float>(*geom, "height", 2.0f);
    return Aabb::fromCenterSize({0,0,0}, {2*br, h, 2*br});
  }
  if (t == "Cylinder") {
    float r = getField<float>(*geom, "radius", 1.0f);
    float h = getField<float>(*geom, "height", 2.0f);
    return Aabb::fromCenterSize({0,0,0}, {2*r, h, 2*r});
  }
  // Generic mesh: any geometry carrying a Coordinate via "coord" or "controlPoint".
  if (hasField(*geom, "coord"))
    return pointsBounds(getNode(*geom, "coord"));
  if (hasField(*geom, "controlPoint"))
    return pointsBounds(getNode(*geom, "controlPoint"));

  // Long-tail types (ElevationGrid/Extrusion/Text/Geo) are added in a later task.
  return {};
}

} // namespace x3d::runtime
#endif // X3D_RUNTIME_GEOMETRY_BOUNDS_HPP
```

- [ ] **Step 4: Run PASS.** `mise run build` then `ctest --preset dev -R x3d_geometry_bounds`. Golden byte-identical.

- [ ] **Step 5: Commit.**
```bash
git add runtime/scene/GeometryBounds.hpp runtime/scene/tests/geometry_bounds_test.cpp CMakeLists.txt
git commit -m "M2b: GeometryBounds primitives + reflection-generic coord/controlPoint path"
```

---

## Task 3: `GeometryBounds` long tail — ElevationGrid, Extrusion, Text, Geo

**Files:** Modify `runtime/scene/GeometryBounds.hpp`, `runtime/scene/tests/geometry_bounds_test.cpp`.

- [ ] **Step 1: Extend the test** — append before `return 0;` in `geometry_bounds_test.cpp`:

```cpp
  // ElevationGrid: x in [0,(xDim-1)*xSp], z in [0,(zDim-1)*zSp], y over height.
  auto eg = createX3DNode("ElevationGrid");
  setF(eg, "xDimension", std::any(3)); setF(eg, "zDimension", std::any(2));
  setF(eg, "xSpacing", std::any(2.0f)); setF(eg, "zSpacing", std::any(5.0f));
  setF(eg, "height", std::any(std::vector<float>{0,1,2,3,4,5}));
  Aabb g = localGeometryBounds(eg.get());
  assert(feq(g.min.x,0) && feq(g.max.x,4) && feq(g.max.z,5) && feq(g.min.y,0) && feq(g.max.y,5));

  // Extrusion: conservative spine±maxSectionRadius. Square section radius ~1.414,
  // spine along y from 0..10.
  auto ex = createX3DNode("Extrusion");
  setF(ex, "crossSection", std::any(std::vector<SFVec2f>{{1,1},{-1,1},{-1,-1},{1,-1},{1,1}}));
  setF(ex, "spine", std::any(std::vector<SFVec3f>{{0,0,0},{0,10,0}}));
  Aabb x = localGeometryBounds(ex.get());
  assert(!x.empty && x.max.y >= 10.0f && x.max.x >= 1.0f && x.min.x <= -1.0f);

  // Text: non-empty, and shrinks to maxExtent when set.
  auto te = createX3DNode("Text");
  setF(te, "string", std::any(std::vector<std::string>{"hello","world"}));
  Aabb tb = localGeometryBounds(te.get());
  assert(!tb.empty);
  setF(te, "maxExtent", std::any(3.0f));
  Aabb tb2 = localGeometryBounds(te.get());
  assert(tb2.size().x <= 6.01f); // width capped by 2*maxExtent (symmetric box)
```

- [ ] **Step 2: Run FAIL.** `mise run build` then `ctest --preset dev -R x3d_geometry_bounds --output-on-failure` → FAIL (ElevationGrid/Extrusion/Text currently return empty).

- [ ] **Step 3: Add the long-tail cases** to `localGeometryBounds`, **immediately before** the final `return {};`:

```cpp
  if (t == "ElevationGrid" || t == "GeoElevationGrid") {
    // Best-effort grid bound in the raw local frame (Geo* is not geo-projected —
    // see backlog M2B-2). x/z from dimensions*spacing, y from the height range.
    int xd = getField<int>(*geom, "xDimension", 0);
    int zd = getField<int>(*geom, "zDimension", 0);
    float xs = getField<float>(*geom, "xSpacing", 1.0f);
    float zs = getField<float>(*geom, "zSpacing", 1.0f);
    auto hs = getField<std::vector<float>>(*geom, "height", {});
    Aabb r;
    if (xd > 0 && zd > 0) {
      float ymin = 0, ymax = 0;
      bool first = true;
      for (float h : hs) { if (first){ymin=ymax=h;first=false;} else {ymin=std::min(ymin,h);ymax=std::max(ymax,h);} }
      r.expand({0, ymin, 0});
      r.expand({(xd-1)*xs, ymax, (zd-1)*zs});
    }
    return r;
  }
  if (t == "Extrusion") {
    // Conservative: AABB over spine points, expanded by the max cross-section
    // radius (scaled). Over-bounds safely without the spine-aligned-frame math.
    auto spine = getField<std::vector<SFVec3f>>(*geom, "spine", {});
    auto sect  = getField<std::vector<SFVec2f>>(*geom, "crossSection", {});
    auto scale = getField<std::vector<SFVec2f>>(*geom, "scale", {});
    float maxS = 1.0f;
    for (const auto &s : scale) maxS = std::max(maxS, std::max(std::fabs(s.x), std::fabs(s.y)));
    float rad = 0.0f;
    for (const auto &p : sect) rad = std::max(rad, std::sqrt(p.x*p.x + p.y*p.y));
    rad *= maxS;
    Aabb r;
    for (const auto &p : spine) {
      r.expand({p.x - rad, p.y - rad, p.z - rad});
      r.expand({p.x + rad, p.y + rad, p.z + rad});
    }
    return r;
  }
  if (t == "Text") {
    // Conservative symmetric estimate. Exact glyph bounds need a font engine
    // (backlog M2B-1). width ~ longest-string * size * 0.6, capped by maxExtent;
    // height ~ lineCount * size * spacing. Centered box (justification-agnostic).
    auto strs = getField<std::vector<std::string>>(*geom, "string", {});
    float size = 1.0f, spacing = 1.0f;
    if (auto fs = getNode(*geom, "fontStyle")) {
      size = getField<float>(*fs, "size", 1.0f);
      spacing = getField<float>(*fs, "spacing", 1.0f);
    }
    std::size_t maxLen = 0;
    for (const auto &s : strs) maxLen = std::max(maxLen, s.size());
    float width = static_cast<float>(maxLen) * size * 0.6f;
    float maxExtent = getField<float>(*geom, "maxExtent", 0.0f);
    if (maxExtent > 0.0f) width = std::min(width, maxExtent);
    float height = static_cast<float>(strs.empty() ? 1 : strs.size()) * size * spacing;
    if (width <= 0.0f && height <= 0.0f) return {};
    Aabb r;
    r.expand({-width, -height, 0});
    r.expand({ width,  height, 0});
    return r;
  }
```

Add `#include <cmath>` and `#include <string>` to the header's includes if not present (for `std::sqrt`, `std::string`).

- [ ] **Step 4: Run PASS.** `mise run build` then `ctest --preset dev -R x3d_geometry_bounds`. Golden byte-identical.

- [ ] **Step 5: Commit.**
```bash
git add runtime/scene/GeometryBounds.hpp runtime/scene/tests/geometry_bounds_test.cpp
git commit -m "M2b: GeometryBounds long tail (ElevationGrid, Extrusion, Text, Geo best-effort)"
```

---

## Task 4: `BoundsSystem` — parent index + bottom-up build/propagate

**Files:** Modify `runtime/scene/TransformSystem.hpp` (expose `localMatrix`); create `runtime/scene/BoundsSystem.hpp`, `runtime/scene/tests/bounds_system_test.cpp`; modify `CMakeLists.txt`.

- [ ] **Step 1: Expose `TransformSystem::localMatrix` as public static.**

In `runtime/scene/TransformSystem.hpp`, the `localMatrix` static currently lives under `private:`. Move its declaration/definition into a `public:` section (or add a `public:` line before it). It is `static Mat4 localMatrix(const X3DNode *n)`. No body change — only visibility. (BoundsSystem composes child Transform local matrices via it.)

- [ ] **Step 2: Write the failing test** — `runtime/scene/tests/bounds_system_test.cpp`:

```cpp
// bounds_system_test.cpp
#include "BoundsSystem.hpp"
#include "TransformSystem.hpp"
#include "DirtyTracker.hpp"
#include "X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp" // out-of-line Scene::addRootNode
#include <any>
#include <cassert>
#include <cmath>
#include <memory>
#include <vector>
using namespace x3d::runtime;
static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }
static void setF(const std::shared_ptr<X3DNode>& n, const char* name, std::any v) {
  for (auto& f : n->fields()) if (f.x3dName == name && f.set) { f.set(*n, std::move(v)); return; }
}
static void addChild(const std::shared_ptr<X3DNode>& p, const std::shared_ptr<X3DNode>& c) {
  for (auto& f : p->fields()) if (f.x3dName == "children" && f.set) {
    auto k = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*p));
    k.push_back(c); f.set(*p, std::any(std::move(k))); return;
  }
}

int main() {
  // Transform(translate +10x) > Shape > Box(size 2) : local bounds of the Transform
  // is the box [-1,1] (children share T's frame; Shape has no transform). World
  // bounds = local * worldTransform(translate +10x) => [9,11]x[-1,1]x[-1,1].
  auto T = createX3DNode("Transform"); setF(T, "translation", std::any(SFVec3f{10,0,0}));
  auto shape = createX3DNode("Shape");
  auto box = createX3DNode("Box"); setF(box, "size", std::any(SFVec3f{2,2,2}));
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(box)));
  addChild(T, shape);
  Scene scene; scene.addRootNode(T);

  TransformSystem ts; ts.buildIndex(scene);
  BoundsSystem bs; bs.buildBounds(scene, ts);

  Aabb lb = bs.localBounds(T.get());
  assert(feq(lb.min.x,-1) && feq(lb.max.x,1));
  Aabb wb = bs.worldBounds(T.get(), ts);
  assert(feq(wb.min.x,9) && feq(wb.max.x,11));

  // Author override: a Group with explicit bboxSize ignores its (bigger) child.
  auto G = createX3DNode("Group");
  setF(G, "bboxCenter", std::any(SFVec3f{0,0,0}));
  setF(G, "bboxSize", std::any(SFVec3f{2,2,2}));
  auto bigShape = createX3DNode("Shape");
  auto bigBox = createX3DNode("Box"); setF(bigBox, "size", std::any(SFVec3f{100,100,100}));
  setF(bigShape, "geometry", std::any(std::shared_ptr<X3DNode>(bigBox)));
  addChild(G, bigShape);
  Scene s2; s2.addRootNode(G);
  TransformSystem ts2; ts2.buildIndex(s2);
  BoundsSystem bs2; bs2.buildBounds(s2, ts2);
  assert(feq(bs2.localBounds(G.get()).size().x, 2)); // author bbox, not 100

  // Incremental: grow the box, mark dirty, propagate -> Transform bounds grow.
  setF(box, "size", std::any(SFVec3f{4,4,4})); // box now [-2,2]
  DirtyTracker dirty; dirty.markDirty(box.get(), DirtyBounds);
  bs.propagate(dirty, ts);
  assert(feq(bs.localBounds(T.get()).max.x, 2)); // grew from 1 to 2
  assert(dirty.flags(T.get()) & DirtyBounds);     // ancestor re-marked
  return 0;
}
```

- [ ] **Step 3: Register + run FAIL.** In `CMakeLists.txt`:

```cmake
    add_executable(x3d_bounds_system
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/scene/tests/bounds_system_test.cpp")
    target_link_libraries(x3d_bounds_system PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_bounds_system COMMAND x3d_bounds_system)
```

Run `mise run build` → FAIL (`BoundsSystem.hpp` not found).

- [ ] **Step 4: Implement `runtime/scene/BoundsSystem.hpp`:**

```cpp
// BoundsSystem.hpp — local-frame AABB per node + bottom-up bounds propagation.
// World bounds are a lazy query composing TransformSystem::worldTransform.
// Side table keyed by const X3DNode*. namespace x3d::runtime.
#ifndef X3D_RUNTIME_BOUNDS_SYSTEM_HPP
#define X3D_RUNTIME_BOUNDS_SYSTEM_HPP

#include "Aabb.hpp"
#include "DirtyTracker.hpp"
#include "GeometryBounds.hpp"
#include "TransformSystem.hpp"
#include "X3DNode.hpp"
#include "X3DScene.hpp"

#include <any>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace x3d::runtime {

class BoundsSystem {
public:
  void buildBounds(const Scene &scene, const TransformSystem &ts) {
    parent_.clear(); children_.clear(); local_.clear();
    for (const auto &root : scene.rootNodes)
      if (root) index(root.get(), nullptr);
    for (const auto &root : scene.rootNodes)
      if (root) compute(root.get(), ts);
  }

  const Aabb &localBounds(const X3DNode *n) const {
    static const Aabb kEmpty{};
    auto it = local_.find(n);
    return it == local_.end() ? kEmpty : it->second;
  }

  Aabb worldBounds(const X3DNode *n, const TransformSystem &ts) const {
    return localBounds(n).transformed(ts.worldTransform(n));
  }

  // Recompute dirtied subtrees bottom-up; mark each recomputed node DirtyBounds.
  void propagate(DirtyTracker &dirty, const TransformSystem &ts) {
    // Snapshot the changed nodes (markDirty during the walk would mutate the list).
    std::vector<const X3DNode *> seed(dirty.changedNodes().begin(),
                                      dirty.changedNodes().end());
    for (const X3DNode *n : seed) {
      if (!local_.count(n)) continue;            // not bounds-participating
      recomputeUp(n, ts, dirty);
    }
  }

private:
  bool isTransform(const X3DNode *n) const { return n && n->nodeTypeName() == "Transform"; }

  // DFS index: parent + children over node-typed fields (guard null getters).
  void index(const X3DNode *n, const X3DNode *parent) {
    parent_[n] = parent;
    if (parent) children_[parent].push_back(n);
    for (const auto &f : n->fields()) {
      if (!f.get) continue;
      if (f.type == X3DFieldType::SFNode) {
        auto c = std::any_cast<std::shared_ptr<X3DNode>>(f.get(*n));
        if (c) index(c.get(), n);
      } else if (f.type == X3DFieldType::MFNode) {
        for (const auto &c : std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*n)))
          if (c) index(c.get(), n);
      }
    }
  }

  // Author bbox override iff every component of bboxSize >= 0.
  bool authorBounds(const X3DNode *n, Aabb &out) const {
    if (!geombounds::hasField(*n, "bboxSize")) return false;
    SFVec3f sz = geombounds::getField<SFVec3f>(*n, "bboxSize", {-1,-1,-1});
    if (sz.x < 0 || sz.y < 0 || sz.z < 0) return false;
    SFVec3f c = geombounds::getField<SFVec3f>(*n, "bboxCenter", {0,0,0});
    out = Aabb::fromCenterSize(c, sz);
    return true;
  }

  // Compute a node's local AABB in its own frame (post-order build). EVERY node
  // gets a local_ entry — including geometry leaves (Box/IFS/...), reached as graph
  // children via the Shape's "geometry" SFNode field — so a later change to a leaf
  // is found by propagate. A node's bounds = its own geometry (non-empty only if it
  // IS a geometry node) unioned with its children (child-Transform frames mapped in
  // via their local matrix). An author bbox overrides (children still get computed
  // for their own entries, but are not unioned into this node).
  Aabb compute(const X3DNode *n, const TransformSystem &ts) {
    Aabb childUnion;
    auto it = children_.find(n);
    if (it != children_.end())
      for (const X3DNode *c : it->second) {
        Aabb cb = compute(c, ts); // post-order: child entry set first
        if (isTransform(c)) cb = cb.transformed(TransformSystem::localMatrix(c));
        childUnion.unionWith(cb);
      }
    Aabb a;
    if (!authorBounds(n, a)) {       // author bbox is authoritative; else compute
      a = localGeometryBounds(n);    // empty unless n is itself a geometry node
      a.unionWith(childUnion);
    }
    local_[n] = a;
    return a;
  }

  // Recompute n's local AABB and re-union ancestors, stopping when unchanged.
  void recomputeUp(const X3DNode *n, const TransformSystem &ts, DirtyTracker &dirty) {
    Aabb before = localBounds(n);
    Aabb now = recomputeLocal(n, ts);
    local_[n] = now;
    dirty.markDirty(n, DirtyBounds);
    if (equalish(before, now)) return; // no change to propagate further? still update self
    const X3DNode *p = parentOf(n);
    if (p) recomputeUp(p, ts, dirty);
  }

  // Recompute one node's local AABB from its CURRENT geometry + its children's
  // already-stored local bounds (mirrors compute() but non-recursive: children's
  // local_ entries are reused, since propagate walks bottom-up from the changed leaf).
  Aabb recomputeLocal(const X3DNode *n, const TransformSystem &ts) {
    (void)ts;
    Aabb a;
    if (authorBounds(n, a)) return a;
    a = localGeometryBounds(n);                 // empty unless n is a geometry node
    auto it = children_.find(n);
    if (it != children_.end())
      for (const X3DNode *c : it->second) {
        Aabb cb = localBounds(c);
        if (isTransform(c)) cb = cb.transformed(TransformSystem::localMatrix(c));
        a.unionWith(cb);
      }
    return a;
  }

  const X3DNode *parentOf(const X3DNode *n) const {
    auto it = parent_.find(n);
    return it == parent_.end() ? nullptr : it->second;
  }
  static bool equalish(const Aabb &a, const Aabb &b) {
    if (a.empty != b.empty) return false;
    if (a.empty) return true;
    auto f = [](float x, float y) { return (x - y) * (x - y) < 1e-10f; };
    return f(a.min.x,b.min.x) && f(a.min.y,b.min.y) && f(a.min.z,b.min.z) &&
           f(a.max.x,b.max.x) && f(a.max.y,b.max.y) && f(a.max.z,b.max.z);
  }

  std::unordered_map<const X3DNode *, const X3DNode *> parent_;
  std::unordered_map<const X3DNode *, std::vector<const X3DNode *>> children_;
  std::unordered_map<const X3DNode *, Aabb> local_;
};

} // namespace x3d::runtime
#endif // X3D_RUNTIME_BOUNDS_SYSTEM_HPP
```

Note on `recomputeUp`: it always updates `local_[n]` and marks `DirtyBounds` for `n`, then recurses to the parent only when the AABB actually changed — so an unrelated sibling subtree (never seeded, never an ancestor of a seed) is never visited or re-marked, satisfying the incremental test.

- [ ] **Step 5: Run PASS.** `mise run build` then `ctest --preset dev -R 'x3d_bounds_system|x3d_transform_system' --output-on-failure` (the localMatrix visibility change must not break TransformSystem). Golden byte-identical.

- [ ] **Step 6: Commit.**
```bash
git add runtime/scene/TransformSystem.hpp runtime/scene/BoundsSystem.hpp \
        runtime/scene/tests/bounds_system_test.cpp CMakeLists.txt
git commit -m "M2b: BoundsSystem parent index + bottom-up build/propagate + world-bounds query"
```

---

## Task 5: `X3DExecutionContext` integration + pull API

**Files:** Modify `runtime/events/X3DExecutionContext.hpp`; create `runtime/events/tests/m2b_tick_test.cpp`; modify `CMakeLists.txt`.

- [ ] **Step 1: Write the failing test** — `runtime/events/tests/m2b_tick_test.cpp`:

```cpp
// m2b_tick_test.cpp — buildSceneGraph wires bounds; after a tick the world bounds
// of a translated Shape>Box are correct, and changing the box via the cascade
// updates them.
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
static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }
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
  auto T = createX3DNode("Transform"); setF(T, "translation", std::any(SFVec3f{5,0,0}));
  auto shape = createX3DNode("Shape");
  auto box = createX3DNode("Box"); setF(box, "size", std::any(SFVec3f{2,2,2}));
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(box)));
  addChild(T, shape);
  Scene scene; scene.addRootNode(T);

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  ctx.tick(0.0);

  Aabb wb = ctx.worldBounds(T.get());        // [4,6]x[-1,1]x[-1,1]
  assert(feq(wb.min.x,4) && feq(wb.max.x,6) && feq(wb.max.y,1));
  Aabb lb = ctx.localBounds(shape.get());    // box bounds in shape's frame
  assert(feq(lb.max.x,1) && feq(lb.min.x,-1));
  return 0;
}
```

- [ ] **Step 2: Register + run FAIL.** In `CMakeLists.txt`:

```cmake
    add_executable(x3d_m2b_tick
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/events/tests/m2b_tick_test.cpp")
    target_link_libraries(x3d_m2b_tick PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_m2b_tick COMMAND x3d_m2b_tick)
```

Run `mise run build` → FAIL (`localBounds`/`worldBounds` not members).

- [ ] **Step 3: Wire BoundsSystem into the context.** In `runtime/events/X3DExecutionContext.hpp`:

Add include: `#include "BoundsSystem.hpp"`. Add a private member next to `transforms_`:
```cpp
  BoundsSystem bounds_;
```

In `buildSceneGraph`, after `transforms_.buildIndex(scene);`, add:
```cpp
    bounds_.buildBounds(scene, transforms_);
```

In `classifyDirty`, after computing the transform/children flags, also flag `DirtyBounds` for geometry/coord/bbox-affecting fields and for children changes. Replace the body's final `dirty_.markDirty(a.node, flags);` with:
```cpp
    static const char *kBounds[] = {"size", "radius", "height", "bottomRadius",
        "point", "coord", "controlPoint", "crossSection", "spine", "scale",
        "geometry", "bboxSize", "string", "maxExtent"};
    for (const char *f : kBounds)
      if (a.field == f) { flags |= DirtyBounds; break; }
    if (flags & DirtyChildren) flags |= DirtyBounds;
    dirty_.markDirty(a.node, flags);
```

In `tick`, after `transforms_.propagate(dirty_);` add:
```cpp
    bounds_.propagate(dirty_, transforms_);
```

Add the pull API (public), next to M2a's `worldTransform`:
```cpp
  /// Pull surface: local-frame AABB of a node (empty if unknown).
  Aabb localBounds(const X3DNode *n) const { return bounds_.localBounds(n); }
  /// Pull surface: world-space AABB (= local bounds x world transform).
  Aabb worldBounds(const X3DNode *n) const { return bounds_.worldBounds(n, transforms_); }
```

- [ ] **Step 4: Run PASS (no regression).** `mise run build` then `ctest --preset dev -R 'x3d_m2b_tick|x3d_m2a_tick|x3d_event|animation|scene_bridge' --output-on-failure`. Golden byte-identical.

- [ ] **Step 5: Commit.**
```bash
git add runtime/events/X3DExecutionContext.hpp runtime/events/tests/m2b_tick_test.cpp CMakeLists.txt
git commit -m "M2b: wire BoundsSystem into the execution context (build + tick + pull API)"
```

---

## Task 6: Full verification

**Files:** none.

- [ ] **Step 1: Full build + suite.** `mise run build` → ctest all green (adds `x3d_aabb`, `x3d_geometry_bounds`, `x3d_bounds_system`, `x3d_m2b_tick`); plus `uv run pytest` → all green incl. `tests/test_golden_tree.py`. No M2a/event regression.
- [ ] **Step 2: Golden byte-identical.** `bash scripts/check_golden.sh` → "byte-for-byte"; sha256 unchanged from `7226b3a07e744b4aea1c3d9f34897384e5d8c085cb0c79743b8a23872c7183c0`. If changed, a runtime edit leaked into a generated template — stop and fix.
- [ ] **Step 3: Record outcome.** Note the new ctest count, golden unchanged, and that M2b (bounds) is in place for M2d (picking) + M2e (LOD/culling) + M2.5 extraction. Confirm backlog rows M2B-1 (Text) + M2B-2 (Geo) are present.

---

## Notes for the implementer

- **TDD discipline:** test first, confirm RED, then GREEN.
- **Golden gate is the safety net** — nothing here is codegen; the golden sha256 must never move.
- **Reflection, not casts:** read geometry/bbox fields via `node->fields()` (the `geombounds::getField`/`getNode` helpers), node-agnostic. Always guard `if(!f.get) continue;` in graph walks (InputOnly MFNode getter trap).
- **Conservative geometry bounds are intentional** for Extrusion (spine±radius) and Text (font-metric-free estimate) — they over-bound, which is safe for culling/picking; exact versions are backlog M2B-1/M2B-2.
- **Author bbox** is used iff every component of `bboxSize ≥ 0`.
- **Don't touch codegen** — all edits in `runtime/**` + `CMakeLists.txt`.
