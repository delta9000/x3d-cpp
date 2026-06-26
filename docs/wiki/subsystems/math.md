---
title: Math Core
summary: SF*-native math module (Mat4, Aabb, Ray, Intersect) — no external math library in the core runtime.
tags: [subsystem, math, mat4, aabb, ray, intersect]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../decisions/0012-no-external-math-lib.md
---

# Math Core

## Purpose

The Math Core subsystem provides all geometry and linear-algebra primitives the runtime needs — world-transform composition, axis-aligned bounding volumes, ray construction, and ray-vs-primitive intersection tests. It carries zero external dependencies: no GLM, no Eigen, no third-party headers. Every type is defined over `SFVec3f` and `SFRotation` (the native X3D field types from `X3Dtypes.hpp`), so the math module speaks the same vocabulary as the node layer without an adapter.

The boundary this module owns: anything that takes two or more SF*/MF* values and computes a geometric result belongs here. Anything that reads scene-graph state or drives the event loop does not. Other subsystems import these headers directly; the math layer has no awareness of them.

## Key files

| File | Role |
|---|---|
| `runtime/math/Mat4.hpp` | Column-major 4×4 float matrix. Builders (`identity`, `translation`, `scale`, `rotation`), multiplication, `transformPoint` (w=1), `transformDirection` (w=0), `inverse` (adjugate form; returns `identity()` on near-singular). Free functions: `transformMatrix` (full X3D `T*C*R*SR*S*SR⁻¹*C⁻¹` composition) and `rotationFromMatrix` (Shepperd quaternion-from-matrix to `SFRotation`). |
| `runtime/math/Aabb.hpp` | Axis-aligned bounding box (float). Stateful `expand(point)`, `unionWith(Aabb)`, `transformed(Mat4)` (all-8-corners method), `center()`, `size()`, and static factory `fromCenterSize`. Empty box is the union identity. |
| `runtime/math/Ray.hpp` | Origin + direction struct (`SFVec3f` both), plus `pointAt(t)` for evaluating a position along the ray. |
| `runtime/math/Intersect.hpp` | Six free intersection functions, all returning `std::optional<float>` (entry parameter `t >= 0`): `rayAabb`, `raySphere`, `rayTriangle`, `rayTriangleBary` (with barycentric (u,v) out-params for texcoord interpolation), `rayCone` (X3D §13.3.2 geometry, `side`/`bottom` flags), `rayCylinder` (X3D §13.3.3 geometry, `side`/`top`/`bottom` flags). |
| `runtime/math/tests/` | Unit test executables: `mat4_test.cpp`, `mat4_inverse_test.cpp`, `aabb_test.cpp`, `intersect_test.cpp`, `vec_math_audit_test.cpp`. |

`Mat4.hpp` has one intra-runtime dependency: `runtime/events/Interpolation.hpp` for the `Quat` type and `quatFromRotation`/`rotationFromQuat` helpers it uses in `Mat4::rotation` and `rotationFromMatrix`. All other math headers depend only on `Mat4.hpp` (and transitively `X3Dtypes.hpp` and `Interpolation.hpp`).

## Interfaces and seams

### Exposed interface

All types live in `namespace x3d::runtime`. Headers are included directly — there is no umbrella math header.

```cpp
// --- Mat4.hpp ---
struct Mat4 {
  std::array<float, 16> m{};            // column-major; element (r,c) = m[c*4+r]

  static Mat4 identity();
  static Mat4 translation(const SFVec3f &t);
  static Mat4 scale(const SFVec3f &s);
  static Mat4 rotation(const SFRotation &rot);   // via quaternion path

  Mat4   operator*(const Mat4 &b) const;
  SFVec3f transformPoint(const SFVec3f &p) const;     // w=1
  SFVec3f transformDirection(const SFVec3f &d) const; // w=0
  Mat4   inverse() const;   // returns identity() if near-singular (|det|<1e-20)
};

// Full X3D Transform local matrix: T * C * R * SR * S * SR⁻¹ * C⁻¹
Mat4 transformMatrix(const SFVec3f &translation, const SFRotation &rotation,
                     const SFVec3f &scale, const SFVec3f &center,
                     const SFRotation &scaleOrientation);

// Upper-left 3×3 must be a pure rotation (no scale).
SFRotation rotationFromMatrix(const Mat4 &mat);

// --- Aabb.hpp ---
struct Aabb {
  SFVec3f min{0,0,0}, max{0,0,0};
  bool empty = true;

  void    expand(const SFVec3f &p);
  void    unionWith(const Aabb &o);
  Aabb    transformed(const Mat4 &m) const;   // expands all 8 corners
  SFVec3f center() const;
  SFVec3f size() const;
  static Aabb fromCenterSize(const SFVec3f &c, const SFVec3f &s);
};

// --- Ray.hpp ---
struct Ray {
  SFVec3f origin{0,0,0};
  SFVec3f direction{0,0,-1};
  SFVec3f pointAt(float t) const;
};

// --- Intersect.hpp — all return std::optional<float> (entry t >= 0) ---
std::optional<float> rayAabb(const Ray &, const Aabb &);
std::optional<float> raySphere(const Ray &, float radius);
std::optional<float> rayTriangle(const Ray &, const SFVec3f &v0, const SFVec3f &v1, const SFVec3f &v2);
std::optional<float> rayTriangleBary(const Ray &, const SFVec3f &v0, const SFVec3f &v1,
                                     const SFVec3f &v2, float &u, float &v);
std::optional<float> rayCone(const Ray &, float bottomRadius, float height,
                              bool side = true, bool bottom = true);
std::optional<float> rayCylinder(const Ray &, float radius, float height,
                                  bool side = true, bool top = true, bool bottom = true);
```

### Seam points

- **TransformSystem** — `runtime/scene/TransformSystem.hpp` calls `transformMatrix` for every dirty `Transform` node and accumulates world-space `Mat4` down the scene graph.
- **BoundsSystem** — `runtime/scene/BoundsSystem.hpp` and `runtime/scene/GeometryBounds.hpp` use `Aabb::expand`, `Aabb::unionWith`, and `Aabb::transformed` to propagate and merge bounding volumes up the hierarchy.
- **PickSystem** — `runtime/scene/PickSystem.hpp` builds a `Ray` from the pointer state and calls `rayAabb` (broad phase) then `rayTriangle`/`rayTriangleBary`/`raySphere`/etc. (narrow phase) to resolve pick hits for pointing-device sensors.
- **Drag sensors** — `runtime/events/drag/PlaneDrag.hpp`, `SphereDrag.hpp`, `CylinderDrag.hpp` each import the intersection primitives they need (`raySphere`, `rayCylinder`, `Mat4::inverse`) to compute drag deltas in sensor-local space.
- **Extract layer** — `runtime/extract/RenderItem.hpp`, `PackedMesh.hpp`, and `SceneExtractor.hpp` carry `Aabb` and `Mat4` fields in the structs they hand to the renderer consumer.
- **Navigation / viewpoint** — `runtime/events/NavigationSystem.hpp`, `ViewpointBindSystem.hpp`, and `ViewpointOffset.hpp` use `Mat4` (including `rotationFromMatrix`) for head-pose composition and viewpoint-offset accumulation.
- **Billboard** — `runtime/scene/Billboard.hpp` uses `Mat4` to align geometry to the viewer axis.
- **ViewDependentSystem** — `runtime/scene/ViewDependentSystem.hpp` uses `Mat4` for LOD distance and Billboard view-coupling.
- **LightSystem** — `runtime/extract/LightSystem.hpp` uses `transformDirection` / `transformPoint` to bring light direction and position into world space.

The math module has no upward dependencies. It sits at the bottom of the dependency graph.

## How it is tested

Five dedicated unit-test executables cover the math module. All are registered with CTest and pass without golden files (assert-based, self-contained):

- `ctest --preset dev -R x3d_mat4` — identity, translation, scale, rotation (90° about Z), matrix multiply order, `transformMatrix` with center offset, `transformMatrix` with translation+scale.
- `ctest --preset dev -R x3d_mat4_inverse` — round-trip `M * M.inverse() ≈ I` for a translate+rotate+scale matrix; `transformDirection` ignores translation; `transformPoint` does not; singular (all-zero) matrix returns `identity()`; `Ray::pointAt` parametric evaluation.
- `ctest --preset dev -R x3d_aabb` — empty union identity, `fromCenterSize`, `center()`, `size()`, `unionWith` grows extents correctly, `transformed` with 45° Z rotation, empty box stays empty after transform.
- `ctest --preset dev -R x3d_intersect` — `rayAabb` (front face hit, miss, origin-inside); `raySphere` (hit, miss, zero radius); `rayTriangle` (hit, outside); `rayCone` (lateral surface, corner AABB miss, bottom cap, `side`/`bottom` flag combinations, degenerate inputs); `rayCylinder` (lateral hit, height miss, top cap, bottom cap, flag combinations, degenerate inputs).
- `ctest --preset dev -R x3d_vec_math_audit` — edge-case audit covering: `quatFromRotation` with zero-axis fallback; `rotationFromQuat` with near-zero vector; singular-matrix inversion (all-zero + zero-scale component + pure translation round-trip); slerp antipodal quaternions; `transformMatrix` precision round-trip; `rayTriangle`/`rayTriangleBary` degenerate triangles (collinear, collapsed edge, ray parallel to plane); `slerpNormal` with parallel and antipodal vectors.

## Related specs and ADRs

- [ADR-0012: No External Math Lib](../decisions/0012-no-external-math-lib.md) — decision to keep the core math SF*-native with no GLM/Eigen dependency, leaving math-library choice to the renderer consumer at the seam.
- [Architecture overview](../architecture.md)
- X3D ISO/IEC 19775-1 §11.4.20 Transform field semantics (`T*C*R*SR*S*SR⁻¹*C⁻¹`) — grounded in `runtime/math/Mat4.hpp` comment + `transformMatrix` implementation.
- X3D ISO/IEC 19775-1 §13.3.2 Cone geometry, §13.3.3 Cylinder geometry — grounded in `runtime/math/Intersect.hpp` `rayCone`/`rayCylinder` comments.
