# Physics seam + Jolt example backend (X3D §37 RigidBodyPhysics dynamics MVP)

**Date:** 2026-06-20
**Status:** Approved design; build next.
**Goal:** A physics-integrator **seam** in the core plus a flag-gated **Jolt** example backend that closes the core of the X3D §37 RigidBodyPhysics *dynamics* inert cluster — validated end-to-end by the `x3d sim` harness (whose `attachPhysics` hook was built for exactly this).

## Why
The wiki/conformance gap-map shows the §37 physics nodes (`RigidBodyCollection`, `RigidBody`, `CollidableShape`, joints, …) are **modeled but not simulated** — no System steps them. `sim` already ships a real-but-no-op `attachPhysics()` hook and the test harness (golden traces, determinism, behavioral assertions) to validate a physics backend. This is the queued post-sim initiative, and it exercises the SDK's proven **seam pattern** at a new domain: the runtime defines the contract; an engine (Jolt) is one backend an embedder plugs in — keeping the spec-correct core engine-agnostic.

## Architecture — mirror the ScriptEngine→Duktape seam
The existing `runtime/script/` seam is the exact template: an abstract `ScriptEngine.hpp` (pure-virtual, opaque handles, no engine types) + `EcmaScriptBackend` + `ScriptSystem` + vendored Duktape built as an isolated `x3d_duktape` static lib that nothing links unless the backend is built. Physics maps 1:1:

```
runtime/physics/
  PhysicsBackend.hpp      # CORE — abstract seam (pure-virtual, opaque BodyHandle, SF* types only)
  PhysicsSystem.hpp       # CORE — a System: reads §37 nodes, drives the backend, writes bodies back
  jolt/
    JoltBackend.{hpp,cpp} # implements PhysicsBackend with Jolt — behind X3D_CPP_BUILD_PHYSICS
  tests/
```

- **`PhysicsBackend.hpp` (CORE, abstract).** Opaque `BodyHandle` (e.g. `uint64_t`, 0 = invalid). Carries **no Jolt types** — every value is the runtime's own representation. v1 interface:
  - `virtual WorldHandle createWorld(SFVec3f gravity) = 0;`
  - `virtual BodyHandle addBody(WorldHandle, const ShapeDesc&, float mass, bool fixed, SFVec3f pos, SFRotation ori, SFVec3f linVel, SFVec3f angVel) = 0;`
  - `virtual void step(WorldHandle, double dt) = 0;`
  - `virtual void getBodyTransform(WorldHandle, BodyHandle, SFVec3f& pos, SFRotation& ori) const = 0;`
  - `ShapeDesc` = a small tagged struct: `{ Kind kind; SFVec3f boxHalfExtents; float sphereRadius; }` (Box | Sphere). Extensible later (no Jolt leakage).
- **`PhysicsSystem.hpp` (CORE, a `System`).** Depends only on the abstract `PhysicsBackend` (constructed with a `std::shared_ptr<PhysicsBackend>`; inert if none, exactly like `ScriptSystem` without a backend).
  - `attach(node, ctx)`: for each `RigidBodyCollection` → `createWorld(gravity)`; for each `RigidBody` in `bodies` → read its `CollidableShape` `geometry` (map `Box`→half-extents from `Box.size/2`, `Sphere`→`radius`; else skip + stderr note), read `mass`/`position`/`orientation`/`linearVelocity`/`angularVelocity`/`fixed` → `addBody(...)` → remember the `RigidBody*`↔`BodyHandle` mapping. Add a single fixed ground plane is NOT auto-injected — collision is body-body; a "ground" is authored as a `fixed=true` body (spec-correct).
  - `update(now, ctx)`: compute `dt = now - lastNow` (clamp to a sane max to avoid huge first-step), `step(dt)`; for each mapped body → `getBodyTransform` → `ctx.writeField(rigidBody, "position", pos)` and `"orientation", ori`. Because `position`/`orientation` are `inputOutput`, this emits `position_changed`/`orientation_changed`, and the **existing cascade/routes** carry them to the author-routed `Transform`s. No new mechanism.
- **`jolt/JoltBackend.{hpp,cpp}`** — implements `PhysicsBackend` via Jolt (`JPH::PhysicsSystem`, `BodyInterface`, `Body` creation from `ShapeDesc`, `Update`/step, read-back). Behind `X3D_CPP_BUILD_PHYSICS`. Owns Jolt init (temp allocator, job system, broadphase) internally.
- **Jolt vendoring.** CMake `FetchContent` pinned to a Jolt **release tag/commit hash**, building Jolt as an isolated `x3d_jolt` STATIC lib **only when `X3D_CPP_BUILD_PHYSICS=ON`**; default OFF → no fetch, no link, standard build/golden/ctest path completely unaffected (the `x3d_duktape` isolation rule). One-way dep: `JoltBackend` → core seam; **core never includes `jolt/`**. `X3D_HAVE_PHYSICS` compile define when the backend is built.
- **`sim` wiring.** `sim_runtime.hpp::attachPhysics(scene, ctx)` (today a real no-op) becomes: when `X3D_HAVE_PHYSICS`, construct a `JoltBackend` + `PhysicsSystem`, walk the scene for `RigidBodyCollection`, attach. So `sim` traces physics with no change to the tick loop or tracer.

## Data flow
```
§37 scene → buildSceneGraph + buildFrom (author ROUTEs RigidBody.position_changed → Transform.set_translation)
          → attachFullRuntime → PhysicsSystem(JoltBackend) attached
per tick:  PhysicsSystem.update → step(dt) → writeField(RigidBody.position/orientation)
          → cascade delivers position_changed/orientation_changed over the routes → Transform moves
          → SceneExtractor sees the moved geometry; sim traces both RigidBody.position and Transform.translation
```

## Validation (the seam's "prove it")
1. **Behavioral correctness** — a `RigidBody` at `position 0 10 0`, `mass 1`, in a `RigidBodyCollection` with `gravity 0 -9.8 0`, no other body → after `t` seconds `position.y ≈ 10 − ½·9.8·t²` within tolerance (free-fall). A body dropped onto a `fixed=true` ground body comes to rest at the expected height. Assert in a `PhysicsSystem` test and via a `sim` trace.
2. **Determinism** — Jolt with fixed `dt` + fixed config is deterministic → the `sim --json` trace is byte-identical across two runs → a committed **golden trace** (regression).
3. **sim integration** — `x3d sim physics-fall.x3d --fps 60 --ticks N` traces `RigidBody.position` descending and the routed `Transform.translation` following it.
4. **Firewall** — `X3D_CPP_BUILD_PHYSICS=OFF` (default): core builds, `mise run golden` zero-drift, `ctest` 150/150 unchanged, Jolt unfetched/unlinked. `ON`: the physics tests run + pass.

## Testing
- **`PhysicsBackend`/Jolt unit** (gated): create world + one body, step, assert free-fall position; bit-for-bit determinism across two identical sims.
- **`PhysicsSystem` unit** (gated): build a §37 scene programmatically (`RigidBodyCollection` → `RigidBody` → `CollidableShape`+`Box`) → attach + tick → assert `RigidBody.position` updates and `position_changed` is delivered to a wired sink.
- **`sim` physics fixture** (gated): `tools/x3d-cli/fixtures/sim-physics.x3d` (a falling box routed to a Transform) → golden trace + the free-fall/rest behavioral assertions + determinism.
- **Flag-OFF gates** unchanged (the proof of isolation).

## Scope cuts (v1, deliberate)
Joints (Ball/Hinge/Slider/Universal/Motor → Jolt constraints); CollisionSensor/Contact/CollisionSpace/Collision **reporting back into the event graph**; CollidableOffset; mesh/IndexedFaceSet collision shapes (Box/Sphere only); ParticleSystem (§38, different); `massDensityModel`, per-body `useFiniteRotation`/`finiteRotationAxis`, `autoDamp` fine-tuning; per-body `useGlobalGravity=false`. Body-body + authored-ground collision IS in (Jolt provides it). These are clean follow-ons against the same seam.

## Success criteria
1. A `PhysicsBackend` seam (core, abstract) + `PhysicsSystem` (core) + `JoltBackend` (flag-gated) exist and follow the `runtime/script/` isolation pattern.
2. `sim` traces a falling/colliding rigid body; the behavioral free-fall + rest-on-ground assertions hold; the golden trace is deterministic.
3. `X3D_CPP_BUILD_PHYSICS=OFF` leaves the standard build, golden (zero-drift), and `ctest` (150/150) completely unaffected; `ON` runs the physics tests green.
4. Core stays engine-agnostic: the seam carries no Jolt types; a different engine could fulfill it. Generated bindings + the default runtime are untouched.
