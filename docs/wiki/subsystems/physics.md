---
title: "Physics (RigidBody dynamics)"
summary: "X3D §37 rigid-body dynamics via a CORE engine-agnostic seam + a flag-gated Jolt example backend; sim traces it."
tags: [subsystem, physics, seam, jolt]
updated: 2026-06-21
related:
  - ../architecture.md
  - ../decisions/0019-physics-seam.md
  - ../decisions/0001-ext-firewall.md
  - ../subsystems/cli-suite.md
---

# Physics (RigidBody dynamics)

## Purpose

Simulates the X3D §37 RigidBodyPhysics nodes (`RigidBodyCollection`, `RigidBody`, `CollidableShape`) — previously modeled but behaviorally inert — by stepping a physics engine each tick and writing the results back into the scene. It owns the boundary between the X3D §37 node model and a third-party physics engine, using the SDK's **seam pattern** (the same shape as the [Script/SAI](../subsystems/system-script-sai.md) `ScriptEngine`→Duktape seam): the runtime defines an engine-agnostic contract; an engine (Jolt) is one backend an embedder plugs in. The spec-correct core never depends on the engine.

## Key files

| File / directory | Role |
|---|---|
| `runtime/physics/PhysicsBackend.hpp` | **CORE** abstract seam. Pure-virtual; opaque `WorldHandle`/`BodyHandle`; `ShapeDesc` (Box \| Sphere); `createWorld`/`addBody`/`step`/`getBodyTransform`. Contains **zero engine types** — every value is the runtime's own `SFVec3f`/`SFRotation`. |
| `runtime/physics/PhysicsSystem.hpp` | **CORE** `System`. Reads §37 nodes, drives the backend, writes bodies back. Constructed with a `shared_ptr<PhysicsBackend>` — inert if none (like `ScriptSystem`). Engine-agnostic. |
| `runtime/physics/jolt/JoltBackend.{hpp,cpp}` | The Jolt v5.5.0 implementation of `PhysicsBackend`. The **only** translation unit where Jolt (`JPH::`) appears; a pImpl (`struct Impl`) hides Jolt even from `JoltBackend.hpp`. Built only when `X3D_CPP_BUILD_PHYSICS=ON`. |
| `tools/x3d-cli/fixtures/sim-physics.x3d` + `tools/x3d-cli/goldens/sim-physics.trace.json` | A falling-box §37 fixture + its committed `x3d sim` golden trace (same-host regression). |
| `CMakeLists.txt` (gated physics block) | `X3D_CPP_BUILD_PHYSICS` option (default OFF) + Jolt via `FetchContent` (pinned v5.5.0) built as an isolated lib; `X3D_HAVE_PHYSICS` define on targets that link the backend. |

## Interfaces and seams

### Exposed interface

```cpp
// runtime/physics/PhysicsBackend.hpp (CORE, engine-agnostic)
struct ShapeDesc { enum class Kind { Box, Sphere } kind; SFVec3f boxHalfExtents; float sphereRadius; };
class PhysicsBackend {
public:
  virtual WorldHandle createWorld(const SFVec3f &gravity) = 0;
  virtual BodyHandle  addBody(WorldHandle, const ShapeDesc&, float mass, bool fixed,
                              const SFVec3f &pos, const SFRotation &ori,
                              const SFVec3f &linVel, const SFVec3f &angVel) = 0;
  virtual void        step(WorldHandle, double dt) = 0;
  virtual void        getBodyTransform(WorldHandle, BodyHandle, SFVec3f &pos, SFRotation &ori) const = 0;
};
```

`PhysicsSystem::attach` finds each `RigidBodyCollection` → `createWorld(gravity)`; for each `RigidBody` in `bodies` it reads the `CollidableShape` geometry (`Box`→half-extents = `Box.size`/2, `Sphere`→`radius`; unsupported shapes are skipped with a note) plus `mass`/`position`/`orientation`/`linearVelocity`/`angularVelocity`/`fixed` → `addBody`. `PhysicsSystem::update(now)` computes `dt`, `step`s, reads each body back, and writes it into the scene.

### Seam points

- **`PhysicsBackend`** — the engine seam. `JoltBackend` is the shipped backend; Bullet/PhysX/a custom integrator could implement the same interface. The seam carries no engine types, so swapping backends touches only `runtime/physics/jolt/` + CMake.
- **Route-out via `postEvent` (no new mechanism)** — `PhysicsSystem` calls `ctx.postEvent(rigidBody, "position", pos)` and `"orientation", ori` each tick. `postEvent` writes the `RigidBody`'s own `position`/`orientation` (both `inputOutput`) **and** seeds the cascade with `position_changed`/`orientation_changed`, so the existing [event cascade](../subsystems/event-cascade.md) carries them over the author's ROUTEs to the visible `Transform`s. See [Execution Context](../subsystems/execution-context.md).
- **Contact reporting via `ContactReporter`** — after stepping, `PhysicsSystem` drains each world's contacts (`backend_->drainContacts`), resolves engine handles back to scene `RigidBody`/`CollidableShape` nodes, and hands the engine-agnostic, handle-free `ContactReporter` a vector of `ResolvedContact`. The reporter builds `Contact` nodes per watched `CollisionSensor` and posts `contacts`/`intersections`/`isActive` (`isActive` only on change) — read-only output, never routed back into the solver (§37.4.5). `ContactReporter.hpp` names no engine type, so it compiles without `X3D_CPP_BUILD_PHYSICS`.
- **`sim` integration** — `tools/x3d-cli/sim_runtime.hpp`'s `attachPhysics(scene, ctx)` hook (a real no-op when `X3D_HAVE_PHYSICS` is undefined) constructs a `JoltBackend` + `PhysicsSystem` and attaches them when the backend is built, so [`x3d sim`](../subsystems/cli-suite.md) traces physics with no change to the tick loop or tracer.
- **The build firewall** — Jolt is fetched/built/linked **only** under `X3D_CPP_BUILD_PHYSICS=ON`, as an isolated lib linked **PRIVATE** (so Jolt's `-mavx2/-mfma` arch flags don't leak onto other targets and perturb FP results). Default OFF → no fetch, no link, standard build/golden/ctest unaffected. See [ADR-0001: Ext Firewall](../decisions/0001-ext-firewall.md) for the sibling isolation pattern.

## How it is tested

- `ctest --preset dev -R x3d_physics_jolt` (gated) — Jolt free-fall (`y(1s) ≈ 5.10` = `10 − ½·9.8·1²`), rest-on-ground (a sphere settles at radius above a fixed ground, no tunnelling), and bit-identical determinism across two sequential runs.
- `ctest --preset dev -R x3d_physics_system` (gated) — a §37 scene built programmatically (`RigidBodyCollection`→`RigidBody`→`CollidableShape`+`Box`) → attach + tick → asserts `RigidBody.position` updates **and** `position_changed` is delivered to a wired sink (proves the `postEvent`→event path, not just the backend call).
- `x3d sim` physics fixture (gated) — golden trace `sim-physics.trace.json` + the free-fall/rest behavioral asserts + two-run byte-identical determinism, in `tools/tests/x3d_cli_test.sh`.
- **Firewall gate** — with `X3D_CPP_BUILD_PHYSICS=OFF` (default) the full `ctest` is **150/150** and `mise run golden` is zero-drift, with zero Jolt symbols in any binary; ON adds the 2 physics tests (**152/152**). This is the proof the engine never touches the core build.

## §37 field & node coverage (ground truth)

This is a **partial** §37 implementation, now covering the core dynamics: Box/Sphere/Cylinder/Cone bodies under gravity with collision *response* **and** *reporting* (`CollisionSensor` `contacts`/`intersections`/`isActive`), applied forces/torques, damping, and three joint types, with pose routed back and host-independent determinism. The fields/nodes still read-but-ignored or unsupported are below. This table is the single source of truth; the conformance findings (`CONF-RBP*` in `docs/conformance/components/RigidBodyPhysics.md`) and the deferral backlog (`docs/superpowers/BACKLOG.md`) point here.

**RigidBody — read:** `geometry`, `mass`, `fixed`, `enabled`, `position`, `orientation`, `linearVelocity`, `angularVelocity`, `forces`, `torques`, `autoDamp`, `linear`/`angularDampingFactor`, `useGlobalGravity`.

| RigidBody field | Status | Notes |
|---|---|---|
| `enabled` | ✅ honored | `enabled=FALSE` excludes the body (`75a7878`) |
| `forces`, `torques` | ✅ applied | per-frame continuous force/torque (`9560686`) |
| `autoDamp`, `linear`/`angularDampingFactor` | ✅ applied | damping force `−factor·velocity` (`2e9d8f3`) |
| `useGlobalGravity` | ✅ honored | FALSE → body ignores collection gravity (`2e9d8f3`) |
| `inertia`, `centerOfMass`, `massDensityModel`, `finiteRotationAxis` | ❌ ignored | inertia/COM always shape-derived |
| `autoDisable`, `disableTime`, `disable*Speed` | ❌ ignored | no sleeping (Jolt sleep thresholds are global, not per-body) |

**RigidBodyCollection — read:** `gravity`, `enabled`, `bodies`, `joints`. **Ignored:** `collider`, `iterations`, `errorCorrection`, `constantForceMix`, `contactSurfaceThickness`, `maxCorrectionSpeed`, `preferAccuracy`, `autoDisable` (all solver tuning → Jolt defaults).

| Node / capability | Status |
|---|---|
| `CollidableShape` Box / Sphere / Cylinder (exact) / Cone (analytic AABB) | ✅ |
| `CollidableShape` non-analytic mesh (IndexedFaceSet, …) | ❌ body excluded (observable via `droppedBodyCount()`) |
| `CollidableOffset`; compound (multiple `CollidableShape`) | ❌ unhandled / only first used |
| Collision *response* (bodies don't interpenetrate / rest) | ✅ (Jolt broadphase + contacts) |
| `CollisionSensor` / `Contact` / `CollisionCollection` / `CollisionSpace` reporting | ✅ `contacts` / `intersections` / `isActive` emitted (engine-agnostic `ContactReporter`; per-pair dedupe, deterministic order, `isActive` on change) |
| `BallJoint` / `SingleAxisHingeJoint` / `SliderJoint` | ✅ (input only) |
| `DoubleAxisHingeJoint` / `MotorJoint` / `UniversalJoint` | ❌ skipped with a note |
| Joint **output** events (`angle`/`separation`/`*Rate`/anchor-point `*_changed`) | ❌ none produced |

**Engine caveats:** **determinism is host-independent** — `JoltBackend` defaults to a single-threaded deterministic job system (identical results regardless of CPU core count; `JoltBackend(workerThreads>0)` opts into a thread pool for large scenes, deterministic only for a fixed count). One world is capped at **1024** bodies/pairs/contacts (overflow now *warns* rather than dropping silently; raising it needs a larger temp allocator), and there is a **single** collision substep per `Update` (fast bodies can tunnel).

## Related specs and ADRs

- [ADR-0019: Physics via a Flag-Gated Engine Backend](../decisions/0019-physics-seam.md) — why physics is a core seam + an isolated backend, not engine code in the core.
- [ADR-0001: Ext Firewall](../decisions/0001-ext-firewall.md) — the sibling one-way-isolation pattern for foreign code.
- [Script / SAI Runtime](../subsystems/system-script-sai.md) — the seam this one mirrors (`ScriptEngine`→Duktape).
- [Event Cascade](../subsystems/event-cascade.md) + [Execution Context](../subsystems/execution-context.md) — the route-out path the bodies drive.
- Spec: `docs/superpowers/specs/2026-06-20-physics-jolt-seam-design.md` — the design (architecture, scope, validation).
- **Scope (partial — see the [§37 coverage matrix](#37-field-node-coverage-ground-truth) above):** Box/Sphere/Cylinder/Cone + gravity + applied forces/torques + damping + collision **response** + collision **reporting** (`CollisionSensor` `contacts`/`intersections`/`isActive` via the engine-agnostic `ContactReporter`) + route-out **and joints** — `BallJoint`→`PointConstraint`, `SingleAxisHingeJoint`→`HingeConstraint` (with min/max limits), `SliderJoint`→`SliderConstraint`, via `ConstraintDesc` + `addConstraint` on the same seam; a joint with one body unset binds the other to the world (`Body::sFixedToWorld`), the pendulum case. Deferred follow-ons against the same seam: sleep/inertia/COM fields, collection solver-tuning fields, the compound joints (`DoubleAxisHingeJoint`/`UniversalJoint`/`MotorJoint`), joint **output** events (`angle_changed`/`separation_changed`/…), `CollidableOffset` + compound/mesh shapes, `ParticleSystem`. Specs: `docs/superpowers/specs/2026-06-20-physics-joints-design.md`, `docs/superpowers/plans/2026-06-21-physics-contact-reporting.md`.
