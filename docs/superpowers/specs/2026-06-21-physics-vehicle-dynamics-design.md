# §37 Vehicle Dynamics Demonstration (turn / cornering / incline / brake-reverse) — Design

**Date:** 2026-06-21
**Status:** Approved (brainstorm) → ready for implementation plan
**Nature:** A *demonstration* that the shipped RigidBodyPhysics seam handles vehicle dynamics
— **no new production code, no new seam, no new §37 field**. Tests + two CLI fixtures +
docs, built entirely from existing capabilities (torque, friction, hinge axles).

## 1. Problem

The vehicle sim so far proves a car drives forward. "Does the physics handle real vehicle
dynamics — turning, cornering stability, grades, braking?" is unproven. This adds rigorous
gated tests + CLI demos for four maneuvers, all driven by per-wheel torque on the existing
vehicle (chassis + 4 cylinder wheels on free `SingleAxisHingeJoint` axles + the merged
per-contact friction path).

## 2. Feasibility (de-risked by a throwaway spike; key finding reshaped the design)

The spike (real `JoltBackend` + `setContactResponse(world, 1.0, -1)` friction) showed:

- **Gentle differential steering does NOT turn a 4-wheel gripping car** — torques (4 vs 2,
  same direction) yawed only **0.019 rad (~1°)**; the front tyres' lateral grip kills the
  yaw. A rolling Ackermann arc requires *steered wheels*.
- **A skid-steer pivot DOES turn it** — opposing torques (left wheels +16, right −16)
  yawed the chassis **~2.24 rad in place** (pure +Y axis, position ~stationary). T=8 was
  insufficient (0.022 rad); breaking lateral grip needs strong torque.
- **Incline climb works** — ground tilted 0.12 rad (~7°), driving uphill at torque −6
  gained **+0.61 m** height (a same-direction control descended).
- **Brake/reverse works** — forward to z≈1.38, then reverse torque −6 → z≈−5.23 (cleanly
  arrested and reversed); torque −3 was too weak (coasted forward).

So the achievable turn is a **skid-steer pivot**, not Ackermann steering — stated honestly
in the design + docs.

## 3. Honest framing

A 4-wheel vehicle with gripping (µ=1) un-steered wheels turns by **overpowering lateral
grip** (a skid-steer / tank pivot), not by rolling through an arc. True steered-wheel
cornering needs a motorized or locked steering-pivot joint (compound joints — **deferred**,
CONF-RBP-JOINTS). The docs/tests label the maneuver a skid-steer pivot and do NOT claim
Ackermann steering.

## 4. Architecture

No production change. All maneuvers reuse the existing vehicle geometry (spike + prior-task
validated): ground Box 50×1×50; chassis Box 1×0.4×2 mass 5 at y=0.5; 4 wheels Cylinder
r=0.4 h=0.2 mass 1 at (±0.6, 0.4, ±0.8) orientation `0 0 1 -1.5708`; 4 hinge axles axis
(1,0,0) anchored at each wheel; friction via collider `frictionCoefficients 1 1` +
`appliedParameters` including `FRICTION_COEFFICIENT-2`.

Wheel index convention: 0=FL(−x,+z) 1=FR(+x,+z) 2=RL(−x,−z) 3=RR(+x,−z). "Left" = −x
(indices 0,2), "right" = +x (1,3).

## 5. The four maneuvers (spike-validated parameters)

- **Skid-steer pivot (turn):** left wheels (0,2) torque `+16 0 0`, right wheels (1,3)
  torque `−16 0 0`, held ~3 s on flat ground. Chassis yaws in place.
- **Stays upright (cornering load):** during the pivot the chassis rotation axis stays
  ~+Y and y is bounded — no roll-over.
- **Incline climb:** ground tilted `1 0 0 0.12` (~7°); rear wheels (2,3) torque `−6 0 0`
  (uphill) ~3 s → height gained.
- **Brake/reverse:** rear wheels torque `+3 0 0` ~1.5 s (forward), then `−6 0 0` ~3 s
  (reverse) → forward motion arrested, then z reverses past the start.

## 6. Testing

- **Gated C++** (`physics_system_test.cpp`, `X3D_CPP_BUILD_PHYSICS`): a backend-level
  vehicle-builder lambda (mirrors the spike: `JoltBackend` + `setContactResponse(world,
  1.0, -1)` + ground/chassis/4 wheels/4 hinges), then four maneuver blocks. Backend-level
  is sufficient (the §37→friction wiring is already covered by the existing vehicle test +
  the CLI fixtures); these tests isolate the *dynamics*.
  1. **Pivot:** apply opposing ±16 torque 180 ticks → assert `|yaw| > 1.0 rad`,
     `|Δx| < 0.5 ∧ |Δz| < 0.5` (in place), `|orientation.axis.y| > 0.9` (upright).
  2. **Incline:** tilted ground, drive uphill (−6) 180 ticks → assert `Δy > 0.3`; a
     no-torque control on the same slope yields `Δy ≤ 0.1` (doesn't climb).
  3. **Brake/reverse:** forward (+3) 90 ticks (record peak z), reverse (−6) 180 ticks →
     assert final `z < peakZ − 1.0` (arrested and reversed past where it was).
  4. **Determinism:** the pivot maneuver run twice is value-identical.
- **CLI fixtures + goldens** (gated harness assertions + golden-regression):
  - `sim-vehicle-pivot.x3d`: left wheels `torques="16 0 0"`, right `torques="-16 0 0"`,
    friction collider; route `Chassis.orientation → CarXform.rotation`. Golden watches
    `CarXform.rotation` (spins). Harness: non-empty rotation change + determinism + golden.
  - `sim-vehicle-ramp.x3d`: ground RigidBody `orientation="1 0 0 0.12"`, rear wheels
    `torques="-6 0 0"`; route `Chassis.position → CarXform.translation`. Golden watches
    `CarXform.translation` (y climbs). Harness: y increases + determinism + golden.
  - Capture each golden only after verifying the behavior (pivot spins / ramp climbs).
- **Regression:** full gated + ungated suites stay green; all existing goldens
  byte-identical (no production change, no new friction-authoring fixtures touched).

## 7. Files touched

- `runtime/physics/tests/physics_system_test.cpp` — gated vehicle-builder + 4 maneuver tests.
- `tools/x3d-cli/fixtures/sim-vehicle-pivot.x3d`, `tools/x3d-cli/goldens/sim-vehicle-pivot.trace.json`.
- `tools/x3d-cli/fixtures/sim-vehicle-ramp.x3d`, `tools/x3d-cli/goldens/sim-vehicle-ramp.trace.json`.
- `tools/tests/x3d_cli_test.sh` — pivot + ramp golden assertions.
- `docs/superpowers/BACKLOG.md` (+ optionally `docs/wiki/subsystems/physics.md`) — a note that
  vehicle dynamics (drive/pivot/incline/brake) are demonstrated; the skid-steer-vs-Ackermann
  caveat; steered wheels deferred under CONF-RBP-JOINTS.

## 8. Scope guardrails (YAGNI)

- No new joints, seam methods, or §37 fields; no `findings.yaml` change (no new spec field
  implemented — this is a demonstration of existing capability).
- Turn = skid-steer pivot only (no Ackermann / steered wheels).
- Brake/reverse is gated-C++-only (time-varying torque has no static-fixture form).
- No suspension, no steering, no drivetrain model — open-loop torque maneuvers.

## 9. Out of scope (deferred)

Steered-wheel (Ackermann) cornering + motorized/locked steering joints (CONF-RBP-JOINTS);
suspension springs; closed-loop control; anisotropic friction / slip / surfaceSpeed
(CONF-RBP-SOLVER).
