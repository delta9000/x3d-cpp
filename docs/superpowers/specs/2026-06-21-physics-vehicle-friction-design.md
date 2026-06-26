# §37 Wheeled Vehicle Sim + Per-Contact Friction & Bounce Authoring — Design

**Date:** 2026-06-21
**Status:** Approved (brainstorm) → ready for implementation plan
**Closes/narrows:** part of `CONF-RBP` response parameters — `CollisionCollection.frictionCoefficients`
(combined friction) and `CollisionCollection.bounce` (combined restitution) are now
applied per-contact. Friction is demonstrated by a rear-wheel-drive vehicle that drives
under wheel torque + tyre friction; bounce by a ball that rebounds.

## 1. Problem

The physics seam sets no friction, so every body uses Jolt's default μ=0.2 and
`CollisionCollection.frictionCoefficients` is ignored. That's too little grip to
drive a torque-powered wheeled vehicle, and it leaves a §37 collision-response
parameter unimplemented. This task adds friction authoring and uses it to run a
rear-wheel-drive vehicle (chassis + 4 cylinder wheels on free hinge axles).

## 2. Feasibility (de-risked by two throwaway spikes)

- **Spike 1 (AWD, blanket per-body friction 1.0):** the vehicle drove `dz=3.20 m`
  in 2 s, upright (`y` held 0.5), straight (`dx=0.002`).
- **Spike 2 (RWD, per-contact `mCombinedFriction=1.0` set in the ContactListener):**
  drove `dz=5.21 m` in 3 s (rear-only torque 3 N·m/wheel), upright (`dy=-0.001`),
  straight (`dx=0.001`).

Both the per-contact friction mechanism and rear-wheel drive are confirmed to work
and stay stable/deterministic. Spikes were discarded.

## 3. Spec grounding (ISO/IEC 19775-1 §37)

- **§37.4.4 CollisionCollection:** `SFVec2f frictionCoefficients [0,∞)` default `0 0`
  — friction in the two friction directions; `SFFloat bounce [0,1]` default `0` — how
  bouncy the contact is (0 = none, 1 = full rebound). Both applied to contacts in the
  collection.
- **§37.4.11 RigidBodyCollection:** `SFNode collider [CollisionCollection]` — the
  collection whose response parameters govern this body system's contacts.
- **§37.4.12 SingleAxisHingeJoint:** the wheel axle (free spin, `minAngle=-π`,
  `maxAngle=π`); now mapped correctly after the hinge-sign fix.
- Friction in §37 is a contact/collection property (not per-body), so applying it as
  Jolt's **per-contact combined friction** (rather than per-body) is the faithful
  mapping. Each `RigidBodyCollection` has one `collider` (one `CollisionCollection`)
  and the seam builds one Jolt world per collection — so friction is effectively
  per-world.

## 4. Architecture

### 4a. Per-contact friction + bounce authoring (`runtime/physics/`)

Both are applied through the **existing `ContactListener`** (the `ContactCollector`
already on every Jolt world for contact reporting):

- `PhysicsBackend`: new `virtual void setContactResponse(WorldHandle, float friction,
  float restitution)` (default no-op). A negative value = "don't override" (keep Jolt's
  default combine) for that parameter independently.
- `JoltBackend`:
  - `JoltWorld`'s `ContactCollector` gains `float combinedFriction = -1.0f` and
    `float combinedRestitution = -1.0f` (sentinel `<0` = unset).
  - `setContactResponse(world, f, r)` stores both on that world's collector.
  - `ContactCollector::OnContactAdded`/`OnContactPersisted`: if `combinedFriction >= 0`,
    set `ioSettings.mCombinedFriction`; if `combinedRestitution >= 0`, set
    `ioSettings.mCombinedRestitution` (then the existing `record()` runs unchanged).
- `PhysicsSystem::attach`: for each `RigidBodyCollection`, read
  `collider → CollisionCollection.{frictionCoefficients, bounce}`; pass
  `friction = (frictionCoefficients != 0) ? frictionCoefficients.x : -1` and
  `restitution = (bounce != 0) ? bounce : -1` to `setContactResponse` (non-default
  detection, matching the inertia pattern — each parameter independently).

**Golden drift:** §37 defaults are `frictionCoefficients (0,0)` and `bounce 0`, both
matching Jolt's defaults (friction 0.2 stays untouched when unset; restitution 0). So a
fixture that authors neither keeps its golden. **Exception — `sim-collision.x3d` authors
`bounce 0.2`**, which is currently inert: once bounce is applied, that box actually
rebounds, so its golden (`Sensor.isActive` trace) legitimately changes. This is correct
behavior finally taking effect, not a regression — its golden is **regenerated** as part
of this work and verified correct-by-construction (the box visibly bounces).

**Documented limitations:** scalar friction uses `frictionCoefficients.x` (anisotropic
`x≠y` deferred); a genuine frictionless intent `(0,0)` / fully-inelastic `bounce 0` read
as "unset" (inertia-style ambiguity — Jolt's defaults coincide, so behavior is correct
either way); `slipFactors`/`surfaceSpeed`/`minBounceSpeed`/solver tuning remain deferred
(CONF-RBP-SOLVER).

### 4b. The vehicle fixture (`sim-vehicle.x3d`)

One `RigidBodyCollection` (`gravity 0 -9.8 0`, `collider → CollisionCollection{
frictionCoefficients 1 1}`) containing:

- **Ground** DEF `Ground`: fixed `RigidBody`, Box `50 1 50`, position `0 -0.5 0`
  (top at y=0).
- **Chassis** DEF `Chassis`: dynamic `RigidBody`, Box `1 0.4 2`, mass 5, position
  `0 0.5 0`.
- **4 wheels** DEF `WheelFL/FR/RL/RR`: dynamic `RigidBody`, **Cylinder** (radius 0.4,
  height 0.2) collidable, `orientation 0 0 1 -1.5708` (lay the cylinder's local-Y onto
  the world-X axle), mass 1, at `(±0.6, 0.4, ±0.8)`. The two **rear** wheels (z=-0.8)
  carry `torques 3 0 0` (constant drive); front wheels roll passively.
- **4 `SingleAxisHingeJoint`** in `.joints`: `body1`=wheel, `body2`=Chassis,
  `axis 1 0 0`, `anchorPoint`=that wheel's center, default free limits (`-π..π`).
- Routes: `Chassis.position → CarXform.translation`, `Chassis.orientation →
  CarXform.rotation` (the visible car).

### 4c. Data flow

```
PhysicsSystem::attach(RigidBodyCollection)
  createWorld(gravity); addBody(ground, chassis, 4 wheels); attachJoints(4 hinges)
  c = collider->CollisionCollection
  fr = (c.frictionCoefficients != 0) ? c.frictionCoefficients.x : -1
  re = (c.bounce != 0)               ? c.bounce                  : -1
  backend_->setContactResponse(world, fr, re)
update(): applyBodyForces (rear-wheel torques) + step; getBodyTransform -> postEvent
JoltBackend: per contact, ContactCollector sets ioSettings.mCombinedFriction /
  mCombinedRestitution (when >=0) → rear tyres grip → vehicle accelerates forward.
```

## 5. Testing

- **Ungated** (`runtime/physics/tests/`, recording fake backend): assert
  `PhysicsSystem` calls `setContactResponse(world, friction, restitution)` with
  `(1.0, -1)` when the collider authors `frictionCoefficients (1,1)`/`bounce 0`; with
  `(-1, 0.8)` when it authors `frictionCoefficients (0,0)`/`bounce 0.8`; and `(-1, -1)`
  when both are default. Pure wiring test, no engine.
- **Gated** (`physics_system_test.cpp` extend, `X3D_CPP_BUILD_PHYSICS`):
  1. **Drives:** build the vehicle scene, settle 0.5 s, drive 3 s → chassis `dz` past a
     threshold (≈ several metres), `|dx|` small (straight), `y` ≈ initial (upright, not
     tipped/sunk).
  2. **Friction matters:** the same scene with `frictionCoefficients (0,0)` (→ Jolt's
     0.2) drives **measurably less** far — proving authored friction is applied.
  3. **Bounce:** a sphere dropped onto the ground with `bounce 0.8` rebounds to a
     clearly higher apex than the same drop with `bounce 0` (which settles) — proving
     restitution is applied.
  4. **Determinism:** two runs value-identical.
- **CLI golden:** `sim-vehicle.x3d`, `x3d sim ... --watch CarXform.translation --json`
  → committed golden trace, captured only after the gated test verifies the drive.
- **Regression:** full gated + ungated suites stay green; existing goldens byte-identical
  EXCEPT `sim-collision.trace.json`, which is **regenerated** because its authored
  `bounce 0.2` now takes effect (verify the regenerated trace shows the box bouncing).
- **Seam purity:** `PhysicsBackend.hpp`/`PhysicsSystem.hpp`/`ContactReporter.hpp` name
  no engine type.

## 6. Files touched

- `runtime/physics/PhysicsBackend.hpp` — `setContactResponse(world, friction, restitution)`.
- `runtime/physics/jolt/JoltBackend.{hpp,cpp}` — collector `combinedFriction` +
  `combinedRestitution` + apply both in callbacks + `setContactResponse` impl.
- `runtime/physics/PhysicsSystem.hpp` — read collider friction+bounce, call
  `setContactResponse`.
- `runtime/physics/tests/` — ungated response-wiring test (new or extend the recording
  fake); `physics_system_test.cpp` gated vehicle + bounce tests.
- `tools/x3d-cli/fixtures/sim-vehicle.x3d`, `tools/x3d-cli/goldens/sim-vehicle.trace.json`.
- `tools/x3d-cli/goldens/sim-collision.trace.json` — **regenerate** (authored bounce now
  applies).
- `tools/tests/x3d_cli_test.sh` — gated vehicle golden assertion.
- `CMakeLists.txt` — register any new ungated test.
- `docs/conformance/findings.yaml`, `docs/superpowers/BACKLOG.md` — record friction +
  bounce authoring (narrow CONF-RBP response-param gap; note anisotropic/slip/surfaceSpeed
  deferred).

## 7. Scope guardrails (YAGNI)

- **Drive only, no steering/braking** (straight-line RWD).
- **Scalar friction** from `frictionCoefficients.x`; anisotropic deferred.
- **Rigid hinge axles, no suspension springs** (spikes showed it's stable).
## 8. Out of scope (stays deferred)

Anisotropic friction, `slipFactors`/`surfaceSpeed`/`minBounceSpeed`/solver tuning
(CONF-RBP-SOLVER), suspension, steering, `massDensityModel`/`finiteRotation`
(CONF-RBP-INERTIA remainder), compound joints (CONF-RBP-JOINTS).
