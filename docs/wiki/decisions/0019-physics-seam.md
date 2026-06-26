---
title: "ADR-0019: Physics via a Flag-Gated Engine Backend Behind a Core Seam"
summary: "Rigid-body physics is a core engine-agnostic seam plus an isolated, flag-gated Jolt backend — not a physics engine baked into the spec-correct core."
tags: [adr, physics, seam]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/physics.md
  - ../decisions/0001-ext-firewall.md
---

# ADR-0019: Physics via a Flag-Gated Engine Backend Behind a Core Seam

## Status

Accepted

## Context

The X3D §37 RigidBodyPhysics nodes (`RigidBodyCollection`, `RigidBody`, `CollidableShape`, joints) were modeled in the generated bindings but behaviorally **inert** — no System stepped them, so ROUTEs into them were silently dropped. Closing this gap requires a real physics integrator (collision detection, constraint solving, numerical integration), which is a large, specialized body of code best supplied by an existing engine (Jolt, Bullet, PhysX).

The forces: (1) the spec-correct core must stay **engine-agnostic and dependency-light** — a renderer/embedder should not be forced to pull a physics engine, and a different engine should be substitutable; (2) the SDK already has a proven pattern for exactly this — the [Script/SAI](../subsystems/system-script-sai.md) `ScriptEngine`→Duktape seam and the [ext firewall](../decisions/0001-ext-firewall.md) — where foreign code lives behind a contract, gated, isolated; (3) `x3d sim` already shipped a real-but-no-op `attachPhysics()` hook and the test harness (golden traces, determinism, behavioral assertions) built to validate a backend.

## Decision

Rigid-body physics is implemented as a **core engine-agnostic seam** (`runtime/physics/PhysicsBackend.hpp` — pure-virtual, opaque handles, only runtime `SF*` types) plus a **core `PhysicsSystem`** that reads the §37 nodes and drives the seam, plus a **single flag-gated backend** (`runtime/physics/jolt/JoltBackend`, Jolt v5.5.0) that is the only place engine types appear. Jolt is fetched, built, and linked **only** under the CMake option `X3D_CPP_BUILD_PHYSICS` (default OFF), as an isolated library linked **PRIVATE**. A body drives the visible scene by `PhysicsSystem` writing `RigidBody.position`/`orientation` each tick, whose `inputOutput` `*_changed` events flow over the author's existing ROUTEs to the `Transform`s — reusing the cascade, adding no new mechanism.

## Consequences

**Positive:**
- The spec-correct core stays engine-agnostic: `PhysicsBackend.hpp`/`PhysicsSystem.hpp` carry zero Jolt types (a pImpl hides Jolt even from `JoltBackend.hpp`), so Bullet/PhysX/a custom integrator can fulfill the same seam by touching only `runtime/physics/jolt/` + CMake.
- The default build is completely unaffected: flag OFF → no Jolt fetch, no link, `ctest` 150/150, golden zero-drift, zero engine symbols. The firewall is verifiable, like the ext/script patterns.
- `sim` traces physics for free (the `attachPhysics` hook is filled), giving headless, deterministic, golden-gated physics regression in CI — a capability no other X3D tool has.
- Consistency: physics is the third instance of the seam pattern (after `ScriptEngine` and the renderer `RenderItem` seam), reinforcing the architecture rather than adding a new shape.

**Trade-offs / costs:**
- A physics engine is a heavy optional dependency (Jolt ~136 TUs); turning the flag ON adds fetch + build time.
- Determinism is per-host: Jolt's internal floats depend on microarchitecture (AVX2/FMA) **and on the job-system thread count** (`hardware_concurrency()-1`), so the committed golden trace is a **same-host, same-core-count** regression; the portable hard gate is the numeric-range behavioral assertions (`y≈5.1`, rest≈radius).
- Build-dir staleness gotcha: flipping `X3D_CPP_BUILD_PHYSICS` ON over a cached-OFF build dir needs a re-configure before the backend links.
- Coverage is **PARTIAL** — Box/Sphere + gravity + collision **response** (not reporting) + the three canonical joints (Ball/Hinge/Slider) + pose route-out. Roughly two-thirds of §37's fields/nodes are read-but-ignored or unsupported: `RigidBody.forces`/`torques` (no force application), damping/sleep/inertia/COM, collection solver-tuning, compound joints (DoubleAxisHinge/Universal/Motor), joint output events, `CollisionSensor`/`Contact` reporting, `CollidableOffset`/mesh shapes, particles. The full field-by-field ground truth is the **[§37 coverage matrix](../subsystems/physics.md)** + the `CONF-RBP*` conformance findings (`docs/conformance/components/RigidBodyPhysics.md`). Each gap extends the same seam with no core change (as joints did).

## Related

- [Architecture](../architecture.md) — where the physics seam sits in the System set + the seam table.
- [Physics subsystem](../subsystems/physics.md) — the implementation detail.
- [ADR-0001: Ext Firewall](../decisions/0001-ext-firewall.md) — the sibling one-way isolation pattern for foreign code.
- Spec: `docs/superpowers/specs/2026-06-20-physics-jolt-seam-design.md`.
