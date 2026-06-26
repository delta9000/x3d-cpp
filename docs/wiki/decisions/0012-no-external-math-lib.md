---
title: "ADR-0012: No External Math Library in the Core"
summary: The core runtime uses only the SF*-native math module (Mat4, Aabb, Ray, Intersect); external math libraries (GLM, Eigen) are the consumer's choice at the seam.
tags: [adr, math, no-glm, no-eigen, sf-native]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/math.md
---

# ADR-0012: No External Math Library in the Core

## Status

Accepted — 2026-06-13

## Context

By the time M2 scene-graph work reached M2b (bounding volumes, `BoundsSystem`) and M2d (picking/navigation, ray casting), the runtime needed a concrete math layer: 4×4 transforms for the `TransformSystem`, axis-aligned bounds for `BoundsSystem`, and ray-primitive intersection for `PickSystem`. Four forces shaped the choice.

**The field types are spec-generated and golden-locked.** The generator emits spec-typed fields — `SFVec3f`, `SFRotation` (axis-angle), `SFMatrix4f` — committed as golden files. Any external library would sit *behind* a conversion boundary (its own vector/matrix types → SF* or back), not *on* the same types. This negates the ergonomic value of a library like GLM (which shines when your runtime is *already* built around glm::vec3), and adds a handedness/layout/alignment bug surface at every crossing.

**The reflection layer type-erases every field through `std::any`.** `X3DEventCascade::postEvent` and `X3DEventGraph` route events as `std::any` values. Eigen's fixed-size types with mandatory alignment attributes (`EIGEN_MAKE_ALIGNED_OPERATOR_NEW`) are hostile to `std::any`: the standard makes no alignment guarantee for the contained object's inline storage, meaning Eigen matrices cannot be safely boxed in `std::any`. Avoiding Eigen globally sidesteps this class of subtle, hard-to-diagnose undefined behavior.

**The math surface for the runtime core is small and fixed.** The SDK runtime needs precisely: column-major `Mat4` with TRS-compose, multiply, inverse, and point/direction transform; a `Quat` for the axis-angle→quaternion path; an `Aabb` for bounds accumulation and transform; a `Ray`; and ray-vs-{AABB, sphere, triangle, cone, cylinder} tests for the X3D picking primitive set. These are closed, static, and compact. A full math library is overkill for this surface, and its compile-time cost is real (the `dev` Ninja preset with ccache builds at a deliberately conservative job cap — adding Eigen's large template headers would work against that investment).

**Math is the *consumer's* choice at the seam.** The M2.5 extraction API emits `RenderItem` and `PackedMesh` as plain spec-typed data. The embedder (the CAVE consumer, or any renderer) adapts that data into its own math: GLM, Eigen, a game-engine math lib, or a SIMD intrinsic layer. That choice belongs to the consumer, not to the SDK. If the SDK imposed GLM, every consumer that didn't use GLM would carry an unused dependency.

**Eigen has a legitimate future scope: numerics-heavy subsystems.** HAnim IK solvers, RigidBodyPhysics, and NURBS fitting are genuinely Eigen-appropriate workloads. The policy does not ban Eigen from the project; it scopes it to those subsystems, isolated from the hot core path.

The decision was confirmed by the user on 2026-06-13 (recorded in the project memory decisions file, math-policy paragraph).

## Decision

The core runtime (`runtime/math/`, `runtime/scene/`, `runtime/events/`, `runtime/extract/`, and `runtime/script/`) uses **no external math library**. The sole math module is the internally-authored, SF*-native module at `runtime/math/`:

- `runtime/math/Mat4.hpp` — column-major 4×4 float matrix (`std::array<float,16>`); `identity`, `translation`, `scale`, `rotation` builders; `operator*`; `transformPoint`/`transformDirection`; `inverse` (adjugate form); `transformMatrix` (full ISO/IEC 19775-1 `T*C*R*SR*S*SR⁻¹*C⁻¹` composition); `rotationFromMatrix` (Shepperd quaternion-from-matrix).
- `runtime/math/Aabb.hpp` — axis-aligned bounding box; `expand`, `unionWith`, `transformed` (all-8-corners), `center`, `size`, `fromCenterSize`.
- `runtime/math/Ray.hpp` — ray (origin + direction); `pointAt`.
- `runtime/math/Intersect.hpp` — `rayAabb`, `raySphere`, `rayTriangle`, `rayTriangleBary`, `rayCone`, `rayCylinder` — the exact set required by the X3D §13.3 primitive picking surface.
- `Quat` and `quatFromRotation`/`rotationFromQuat` live in `runtime/events/Interpolation.hpp` (shared with the interpolator system); `Mat4::rotation` calls this path.

All types operate directly on the generated spec types (`SFVec3f`, `SFRotation`) with no conversion. External libraries (GLM, Eigen, or any other) are **not** linked into `x3d_cpp` or `x3d_cpp_nodes`; they do not appear in `CMakeLists.txt` for the core targets.

The boundary-convention contract (so consumer conversion is deterministic): matrices are **column-major**, angles are **radians**, rotations are **axis-angle** (`SFRotation`), coordinate system is **right-handed Y-up**.

Eigen is permitted, scoped to numerics-heavy subsystem targets only (HAnim IK, RigidBodyPhysics, NURBS), linked into those targets exclusively, and never pulled into the core via transitive dependency.

## Consequences

**Positive:**

- No conversion boundary between the runtime's math and its field types. `TransformSystem`, `BoundsSystem`, `PickSystem`, and the navigation path all compose and pass `SFVec3f`/`SFRotation`/`Mat4` without any glue layer — eliminating a class of handedness, column/row, and coordinate-system bugs that typically live at conversion boundaries.
- `std::any` boxing is safe throughout the event cascade. The reflection layer can box any SF* field value, including any math result, without alignment-attribute clashes.
- No external dependency on the core build graph. The `dev` Ninja+ccache build keeps its fast cold-rebuild advantage. There is no CMake `find_package(glm)` / `find_package(Eigen3)` step, no vcpkg/conan integration, and no concern about version pinning across platforms (GCC 11 / Clang 14 / MSVC 19.3x).
- The consumer is free to choose any math library. The M2.5 extraction seam emits plain `SFVec3f` / `PackedMesh` data; an embedder using GLM or Eigen writes a thin adapter once, independently, without any version conflict with the SDK.
- Eigen can be used where it actually earns its weight (IK, physics, fitting) without being imposed on all SDK consumers.

**Trade-offs / costs:**

- The math module is hand-authored and must be maintained. `Intersect.hpp` implements six ray-primitive tests (~200 lines) that a library would provide. Any bug in those implementations is the project's responsibility. (Mitigation: the picking tests in `Testing/` and the conformance suite exercise these paths, and the code is straightforward fixed-geometry arithmetic.)
- `Mat4` is always `float`. Double-precision math (e.g. for large-world geospatial scenes, GEO component) is not available from this module. That is acceptable at this scope (GEO is deferred post-v1), but it is a real limitation if the GEO component is ever enabled.
- There is no SIMD path. `Mat4::operator*` is a plain triple nested loop. For the transform-propagation cadence of a scene-graph runtime (O(nodes) per tick, not O(triangles)), this is not a bottleneck — profiling would need to prove otherwise before changing this. An embedder doing high-frequency matrix batching should do it on their side of the seam.
- The boundary-convention contract (column-major, radians, axis-angle, right-handed Y-up) must be documented and stable; changing any element after consumers exist would be a breaking change. This is noted as a one-time nail-down at M2.5 landing.

## Related

- [Architecture](../architecture.md)
- [Math subsystem](../subsystems/math.md) — detailed subsystem page for `runtime/math/`
- `runtime/math/Mat4.hpp` — column-major Mat4; `transformMatrix` implements ISO/IEC 19775-1 Transform composition
- `runtime/math/Aabb.hpp` — bounding-box type; used by `BoundsSystem`
- `runtime/math/Ray.hpp` — ray type; used by `PickSystem` and navigation
- `runtime/math/Intersect.hpp` — ray-vs-primitive tests for the X3D §13.3 primitive set
- `runtime/events/Interpolation.hpp` — `Quat`, `quatFromRotation`, `rotationFromQuat`; shared with interpolator system
- `docs/superpowers/specs/2026-06-13-m2b-bounding-volumes-design.md` — M2b design where `Aabb` and `Mat4` were first introduced; first context where the no-external-math policy was articulated
- `docs/superpowers/specs/2026-06-13-m2d-picking-navigation-design.md` — M2d design; `Intersect.hpp` primitives; seam-boundary conventions first stated
