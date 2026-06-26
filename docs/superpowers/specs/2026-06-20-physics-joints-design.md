# Physics v2 — joints (X3D §37 RigidJoint family, Ball/Hinge/Slider)

**Date:** 2026-06-20
**Status:** Approved (self-approved per author); extends the shipped physics seam.
**Goal:** Add the three canonical X3D §37 rigid-body joints — `BallJoint`, `SingleAxisHingeJoint`, `SliderJoint` — to the physics seam, mapped to Jolt constraints, validated via the `sim` harness. Builds directly on the shipped seam (`docs/superpowers/specs/2026-06-20-physics-jolt-seam-design.md`); reuses everything (the engine-agnostic `PhysicsBackend`, `PhysicsSystem`, the Jolt backend, the firewall, the sim golden/determinism harness).

## Why
The physics MVP shipped rigid-body *dynamics* (bodies fall + collide) and explicitly deferred joints as a clean follow-on against the same seam. Joints are the satisfying next step (pendulums, hinged doors, pistons) and a low-risk extension: the seam, node-reading, backend isolation, and validation harness all exist — only the constraint mapping is new.

## Scope (v1)
The three constraint types that map directly to a well-documented Jolt constraint:
- **`BallJoint`** (point-to-point, 3-DOF rotation) → Jolt `PointConstraint`. Fields: `anchorPoint`.
- **`SingleAxisHingeJoint`** (1-DOF rotation) → Jolt `HingeConstraint` (with `minAngle`/`maxAngle` limits — cheap in Jolt). Fields: `anchorPoint`, `axis`, `minAngle`, `maxAngle`.
- **`SliderJoint`** (1-DOF translation) → Jolt `SliderConstraint` (with `minSeparation`/`maxSeparation` limits). Fields: `axis`, `minSeparation`, `maxSeparation`.

All read `body1`/`body2` (`SFNode` → `RigidBody`) from the `X3DRigidJointNode` base. A joint with one body unset binds the other body to the world (a fixed anchor) — the common pendulum case.

**Deferred (v2.1, same seam):** `DoubleAxisHingeJoint`, `UniversalJoint`, `MotorJoint`; joint **output** events (`angle_changed`, `body1AnchorPoint_changed`, `separation_changed`, …) routed back into the cascade; motor drive; `stopBounce`/`stopErrorCorrection` tuning.

## Architecture (extends the seam)
- **`PhysicsBackend.hpp` (CORE, engine-agnostic) — add:**
  ```cpp
  struct ConstraintDesc {
    enum class Kind { Ball, Hinge, Slider } kind;
    BodyHandle bodyA, bodyB;   // bodyB may be kInvalidBody = bind bodyA to the world
    SFVec3f anchor;            // anchorPoint (world space, as authored)
    SFVec3f axis;              // hinge/slider axis (ignored for Ball)
    float minLimit, maxLimit;  // angle (hinge) or separation (slider); ignored for Ball
  };
  virtual ConstraintHandle addConstraint(WorldHandle, const ConstraintDesc&) = 0;
  ```
  Opaque `ConstraintHandle`. Zero Jolt types — same purity rule as the body seam (the seam-purity gate greps for leakage).
- **`PhysicsSystem.hpp` (CORE) — extend `attach`:** after the bodies of a `RigidBodyCollection` are added (so the `RigidBody*`↔`BodyHandle` map is populated), iterate `RigidBodyCollection.joints` (MFNode). For each joint: identify its kind (`nodeTypeName`), resolve `body1`/`body2` → `BodyHandle`s via the body map (unset → `kInvalidBody`), read `anchorPoint`/`axis`/limits → build a `ConstraintDesc` → `addConstraint`. Unsupported joint types → skip + stderr note. Bodies must be added before constraints (ordering requirement).
- **`jolt/JoltBackend.cpp` — implement `addConstraint`:** map `ConstraintDesc` → `JPH::PointConstraintSettings` / `HingeConstraintSettings` (set `mLimitsMin/Max`) / `SliderConstraintSettings` (set limits), `Create(bodyA, bodyB)`, `physics->AddConstraint(c)`. For a world anchor (`bodyB` invalid), use Jolt's fixed/static body or `Body::sFixedToWorld`. The pImpl owns the constraint pointers.

No change to the route-out, the firewall, the CMake gating, or `sim` wiring — joints ride the existing `PhysicsSystem`/`attachPhysics` path.

## Validation (the "prove it")
- **Behavioral (the key one):** a **pendulum** — a dynamic `RigidBody` ball-jointed (or hinged) at an `anchorPoint`, released horizontally under gravity → it swings; the **constrained anchor stays fixed** (the body's distance from the anchor stays ≈ constant within tolerance over the swing) and the body does **not** fly off. A **hinge** with `minAngle`/`maxAngle` stops at its limit. A **slider** constrains motion to its axis (off-axis position ≈ 0).
- **Determinism:** the `sim` joint fixture trace is byte-identical across two runs → committed golden (same-host regression; numeric-range asserts are the portable hard gate).
- **Firewall unchanged:** flag-OFF → ctest **150/150**, golden zero-drift, zero Jolt symbols. Flag-ON → the existing physics tests + the new joint tests pass.

## Testing
- **Constraint unit** (gated): two bodies + one `HingeConstraint` (and a `PointConstraint`) created directly in the backend → step → assert the constrained relationship holds (anchored point stays put; hinge respects limits).
- **`PhysicsSystem` joint unit** (gated): a §37 scene built programmatically (`RigidBodyCollection` with a `RigidBody` + a `BallJoint`/`SingleAxisHingeJoint` to a fixed anchor) → attach + tick → assert the body swings while the anchor stays fixed.
- **`sim` joint fixture** (gated): `tools/x3d-cli/fixtures/sim-joints.x3d` (a pendulum, `RigidBody.position_changed` → a `Transform`) → `x3d sim` golden trace + the pendulum behavioral asserts + determinism.

## De-risk first
A throwaway Jolt-constraint spike (two bodies + a `HingeConstraint`, step, confirm the constrained body swings and the pivot stays fixed) — confirms the Jolt v5.5.0 constraint API (`TwoBodyConstraintSettings`, `Create`, `AddConstraint`, limits) works in our setup before the full build. Same discipline as the MVP de-risk; the spike is removed after (deliverable = GO/NO-GO + the working constraint boilerplate).

## Success criteria
1. `BallJoint`/`SingleAxisHingeJoint`/`SliderJoint` map through the seam to Jolt constraints; the seam stays engine-agnostic (`ConstraintDesc`, no Jolt types in `PhysicsBackend.hpp`/`PhysicsSystem.hpp`).
2. The pendulum behavioral assertions hold (anchor fixed, body swings, hinge limits respected); the sim joint trace is deterministic + golden-gated.
3. Firewall unchanged: flag-OFF 150/150 + golden zero-drift + zero Jolt symbols; flag-ON physics+joints green.
4. Generated bindings + default runtime untouched; the wiki physics page is updated (joints no longer "deferred") after merge.
