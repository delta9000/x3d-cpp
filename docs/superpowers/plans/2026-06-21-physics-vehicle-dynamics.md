# §37 Vehicle Dynamics Demonstration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Prove the shipped RigidBodyPhysics seam handles vehicle dynamics — a skid-steer pivot (turn), cornering stability, an incline climb, and brake/reverse — via gated tests + two CLI golden fixtures, with zero production-code change.

**Architecture:** Every maneuver reuses the existing vehicle (chassis + 4 cylinder wheels on free `SingleAxisHingeJoint` axles + the merged per-contact friction path) driven by per-wheel torque. Gated C++ tests build the vehicle at the backend level (`JoltBackend` + `setContactResponse`, mirroring the validated spike); two CLI fixtures (`sim-vehicle-pivot`, `sim-vehicle-ramp`) exercise the full §37-node path.

**Tech Stack:** C++20, Jolt Physics v5.5.0 (gated), CMake/CTest, the `x3d` CLI + golden harness, generated X3D node bindings.

## Global Constraints

- **No production code, no new seam/§37 field, no `findings.yaml` change.** This is a demonstration of existing capability — tests + fixtures + docs only.
- **Spike-validated geometry + parameters (use verbatim):** ground Box `50×1×50` at y=-0.5; chassis Box `1×0.4×2` mass 5 at y=0.5; 4 wheels Cylinder r=0.4 h=0.2 mass 1 at (±0.6, 0.4, ±0.8) orientation `0 0 1 -1.5708`; 4 hinge axles axis (1,0,0) anchored at each wheel, free limits `-π..π`; friction via `setContactResponse(world, 1.0f, -1.0f)` (backend tests) or collider `frictionCoefficients="1 1"` + `appliedParameters='"BOUNCE" "FRICTION_COEFFICIENT-2"'` (fixtures).
- **Wheel index convention:** 0=FL(−x,+z), 1=FR(+x,+z), 2=RL(−x,−z), 3=RR(+x,−z). Left = −x (0,2), right = +x (1,3).
- **Validated maneuver outcomes:** pivot (left +16 / right −16) → yaw ~2.24 rad in place, axis ~+Y; incline (ground `1 0 0 0.12`, rear −6) → Δy ≈ +0.6 m; brake/reverse (rear +3 then −6) → z arrests and reverses (z≈1.38 → ≈−5.2). Gentle same-direction differential does NOT turn (grip resists) — the turn is a **skid-steer pivot**, not Ackermann; do not claim steering.
- **Build/test:** gated `cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=ON && cmake --build --preset dev && ctest --preset dev -j "$(nproc)"`; ungated `… -DX3D_CPP_BUILD_PHYSICS=OFF …`. Single target: `cmake --build --preset dev --target <name>`. Respect the preset compile-job cap (no high build `-j`; `ctest -j nproc` fine). Cache sticky on last flag.
- **Golden discipline:** capture a golden only after verifying the behavior (pivot spins / ramp climbs). All existing goldens stay byte-identical (no production change).
- **TDD:** red → green → commit per step.
- **Verified facts:** `getBodyTransform` returns the chassis SFRotation (axis + angle); a pure-yaw pivot has axis ≈ (0,±1,0). `RigidBody.setTorques(MFVec3f)` is re-read every frame by PhysicsSystem (enables mid-sim torque change). Fixture node setters confirmed in prior tasks (Box `size`, Cylinder `radius`/`height`, RigidBody `mass`/`position`/`orientation`/`fixed`/`torques`, SingleAxisHingeJoint `axis`/`anchorPoint`/`body1`/`body2`, CollisionCollection `frictionCoefficients`/`bounce`/`appliedParameters`). The CLI sim enrols a RigidBodyCollection via `sim_runtime.attachPhysics`. The `<RigidBody USE="…" containerField="body1"/>` joint form works (proven by sim-vehicle.x3d).

## File Structure

- `runtime/physics/tests/physics_system_test.cpp` — **modify**: backend-level vehicle builder + 4 maneuver tests.
- `tools/x3d-cli/fixtures/sim-vehicle-pivot.x3d`, `tools/x3d-cli/goldens/sim-vehicle-pivot.trace.json` — **create**.
- `tools/x3d-cli/fixtures/sim-vehicle-ramp.x3d`, `tools/x3d-cli/goldens/sim-vehicle-ramp.trace.json` — **create**.
- `tools/tests/x3d_cli_test.sh` — **modify**: pivot + ramp assertions.
- `docs/superpowers/BACKLOG.md` — **modify**: vehicle-dynamics demonstration note + skid-steer caveat.

---

### Task 1: Gated vehicle-dynamics maneuver tests

**Files:**
- Modify: `runtime/physics/tests/physics_system_test.cpp`

**Interfaces:**
- Consumes: `JoltBackend`, `setContactResponse`, `addBody`/`addConstraint`/`applyForce`/`step`/`getBodyTransform` (all existing).
- Produces: nothing downstream (test-only).

- [ ] **Step 1: Add the vehicle builder + four maneuver blocks**

In `runtime/physics/tests/physics_system_test.cpp`, ensure these includes are present (add any missing): `#include "JoltBackend.hpp"`, `#include <cmath>`, `#include <utility>`. The file already defines `g_failures`, `CHECK`, `kNoRot`, `kZero`. Add this block to `main()` (before the final return / ALL-PASS print if present; otherwise at the end of `main` before `return`):

```cpp
  // ── Vehicle dynamics (backend-level, mirrors the validated spike): skid-steer
  //    pivot, incline climb, brake/reverse. Reuses the existing vehicle geometry;
  //    friction via setContactResponse. The turn is a skid-steer PIVOT (un-steered
  //    gripping wheels can't roll an Ackermann arc — gentle differential yaws ~1°).
  {
    struct Veh { x3d::runtime::WorldHandle world; x3d::runtime::BodyHandle chassis;
                 x3d::runtime::BodyHandle wheels[4]; };
    auto build = [&](JoltBackend &b, SFRotation groundOri) -> Veh {
      WorldHandle world = b.createWorld(SFVec3f{0, -9.8f, 0});
      b.setContactResponse(world, 1.0f, -1.0f);  // friction 1.0
      b.addBody(world, ShapeDesc::box(SFVec3f{25, 0.5f, 25}), MassProperties{0.0f},
                true, SFVec3f{0, -0.5f, 0}, groundOri, kZero, kZero);
      BodyHandle chassis =
          b.addBody(world, ShapeDesc::box(SFVec3f{0.5f, 0.2f, 1.0f}),
                    MassProperties{5.0f}, false, SFVec3f{0, 0.5f, 0}, kNoRot, kZero,
                    kZero);
      const SFRotation wo{0, 0, 1, -1.5707963f};
      const float wx = 0.6f, wy = 0.4f, wz = 0.8f;
      SFVec3f ctr[4] = {{-wx,wy,wz},{wx,wy,wz},{-wx,wy,-wz},{wx,wy,-wz}};
      Veh v; v.world = world; v.chassis = chassis;
      for (int i = 0; i < 4; ++i) {
        v.wheels[i] = b.addBody(world, ShapeDesc::cylinder(0.4f, 0.1f),
                                MassProperties{1.0f}, false, ctr[i], wo, kZero, kZero);
        ConstraintDesc d; d.kind = ConstraintDesc::Kind::Hinge;
        d.bodyA = v.wheels[i]; d.bodyB = chassis; d.anchor = ctr[i];
        d.axis = SFVec3f{1,0,0}; d.minLimit = -3.14159265f; d.maxLimit = 3.14159265f;
        b.addConstraint(world, d);
      }
      return v;
    };
    auto chassisPose = [&](JoltBackend &b, const Veh &v) {
      SFVec3f p{0,0,0}; SFRotation o = kNoRot;
      b.getBodyTransform(v.world, v.chassis, p, o); return std::make_pair(p, o);
    };

    // (a) Skid-steer pivot: left wheels +16, right wheels -16 -> yaw in place.
    auto pivotRun = [&]() {
      JoltBackend b; Veh v = build(b, kNoRot);
      for (int i = 0; i < 30; ++i) b.step(v.world, 1.0 / 60.0);  // settle
      auto p0 = chassisPose(b, v);
      for (int i = 0; i < 180; ++i) {
        b.applyForce(v.world, v.wheels[0], kZero, SFVec3f{16,0,0});
        b.applyForce(v.world, v.wheels[2], kZero, SFVec3f{16,0,0});
        b.applyForce(v.world, v.wheels[1], kZero, SFVec3f{-16,0,0});
        b.applyForce(v.world, v.wheels[3], kZero, SFVec3f{-16,0,0});
        b.step(v.world, 1.0 / 60.0);
      }
      auto p1 = chassisPose(b, v);
      return std::make_pair(p0, p1);
    };
    {
      auto [p0, p1] = pivotRun();
      const float yaw = p1.second.angle;
      const float dx = p1.first.x - p0.first.x, dz = p1.first.z - p0.first.z;
      std::fprintf(stderr,
                   "vehicle pivot: yaw=%.3f axisY=%.2f d=(%.2f,%.2f)\n",
                   yaw, p1.second.y, dx, dz);
      CHECK(std::fabs(yaw) > 1.0f, "skid-steer pivot yaws the chassis (>1 rad)");
      CHECK(std::fabs(p1.second.y) > 0.9f, "pivot is about +Y (upright, pure yaw)");
      CHECK(std::fabs(dx) < 0.5f && std::fabs(dz) < 0.5f,
            "pivot stays in place (no significant translation)");
    }

    // (b) Incline climb: ground tilted ~7deg; drive uphill (-6) gains height; a
    //     no-drive control on the same slope does not climb.
    auto inclineDy = [&](float torque) {
      JoltBackend b; Veh v = build(b, SFRotation{1, 0, 0, 0.12f});
      for (int i = 0; i < 60; ++i) b.step(v.world, 1.0 / 60.0);  // settle on slope
      auto p0 = chassisPose(b, v);
      for (int i = 0; i < 180; ++i) {
        b.applyForce(v.world, v.wheels[2], kZero, SFVec3f{torque,0,0});
        b.applyForce(v.world, v.wheels[3], kZero, SFVec3f{torque,0,0});
        b.step(v.world, 1.0 / 60.0);
      }
      return chassisPose(b, v).first.y - p0.first.y;
    };
    {
      float climb = inclineDy(-6.0f);   // uphill
      float coast = inclineDy(0.0f);    // no drive
      std::fprintf(stderr, "vehicle incline: climb dy=%.3f vs coast dy=%.3f\n",
                   climb, coast);
      CHECK(climb > 0.3f, "vehicle climbs the incline under drive (dy > 0.3)");
      CHECK(climb > coast + 0.3f, "drive climbs higher than coasting on the slope");
    }

    // (c) Brake/reverse: forward (+3), record peak z, then reverse (-6) past start.
    {
      JoltBackend b; Veh v = build(b, kNoRot);
      for (int i = 0; i < 30; ++i) b.step(v.world, 1.0 / 60.0);
      for (int i = 0; i < 90; ++i) {
        b.applyForce(v.world, v.wheels[2], kZero, SFVec3f{3,0,0});
        b.applyForce(v.world, v.wheels[3], kZero, SFVec3f{3,0,0});
        b.step(v.world, 1.0 / 60.0);
      }
      float peakZ = chassisPose(b, v).first.z;
      for (int i = 0; i < 180; ++i) {
        b.applyForce(v.world, v.wheels[2], kZero, SFVec3f{-6,0,0});
        b.applyForce(v.world, v.wheels[3], kZero, SFVec3f{-6,0,0});
        b.step(v.world, 1.0 / 60.0);
      }
      float finalZ = chassisPose(b, v).first.z;
      std::fprintf(stderr, "vehicle brake/reverse: peakZ=%.2f finalZ=%.2f\n",
                   peakZ, finalZ);
      CHECK(peakZ > 0.5f, "vehicle drove forward before braking");
      CHECK(finalZ < peakZ - 1.0f, "reverse torque arrests and reverses the vehicle");
    }

    // (d) Determinism: the pivot maneuver is value-identical across two runs.
    {
      auto a = pivotRun(); auto c = pivotRun();
      CHECK(std::fabs(a.second.second.angle - c.second.second.angle) < 1e-4f,
            "vehicle pivot is deterministic (two runs identical yaw)");
    }
  }
```

- [ ] **Step 2: Build gated + verify the maneuvers pass**

Run: `cd <repo-root> && cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=ON && cmake --build --preset dev --target x3d_physics_system_test && ctest --preset dev -R x3d_physics_system --output-on-failure 2>&1 | tail -20`
Expected: PASS. The stderr shows `vehicle pivot: yaw≈2.2 axisY≈1.0`, `incline: climb dy≈0.6 vs coast dy≤~0`, `brake/reverse: peakZ≈1.4 finalZ≈−5`. These are the validated spike values; if a maneuver fails, re-check the geometry/torque constants against Global Constraints (do NOT weaken the assertions — the spike proved these work).

> NOTE (conceptual RED): these maneuvers exercise existing physics, so there's no production code to add that flips red→green. The assertions ARE the proof; their printed values (yaw 2.2 not ~0; climb 0.6 not negative; finalZ −5 not +1.4) demonstrate they're meaningful, not tautological. If you want a literal RED, temporarily halve a torque (e.g. pivot ±8) and observe the yaw assertion fail (~0.02 rad), then restore ±16.

- [ ] **Step 3: Commit**

```bash
git add runtime/physics/tests/physics_system_test.cpp
git commit -m "test(physics): gated vehicle dynamics — skid-steer pivot, incline climb, brake/reverse"
```

---

### Task 2: CLI fixtures + goldens (pivot + ramp)

**Files:**
- Create: `tools/x3d-cli/fixtures/sim-vehicle-pivot.x3d`, `tools/x3d-cli/fixtures/sim-vehicle-ramp.x3d`
- Create: `tools/x3d-cli/goldens/sim-vehicle-pivot.trace.json`, `tools/x3d-cli/goldens/sim-vehicle-ramp.trace.json`
- Modify: `tools/tests/x3d_cli_test.sh`

**Interfaces:**
- Consumes: the merged friction path (`CollisionCollection` + `appliedParameters`) and `x3d sim`.

- [ ] **Step 1: Create the pivot fixture**

`tools/x3d-cli/fixtures/sim-vehicle-pivot.x3d`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE X3D PUBLIC "ISO//Web3D//DTD X3D 4.0//EN" "https://www.web3d.org/specifications/x3d-4.0.dtd">
<X3D profile="Full" version="4.0">
  <head>
    <meta name="description" content="sim vehicle skid-steer pivot: left wheels driven forward, right wheels backward (opposing torque) overpower lateral tyre grip and yaw the chassis in place (§37 friction)."/>
  </head>
  <Scene>
    <Transform DEF="CarXform"><Shape><Box size="1 0.4 2"/></Shape></Transform>
    <RigidBodyCollection DEF="World" gravity="0 -9.8 0">
      <CollisionCollection containerField="collider" frictionCoefficients="1 1" appliedParameters='"BOUNCE" "FRICTION_COEFFICIENT-2"'>
        <CollidableShape USE="GroundColl" containerField="collidables"/>
      </CollisionCollection>
      <RigidBody DEF="Ground" containerField="bodies" fixed="true" position="0 -0.5 0">
        <CollidableShape DEF="GroundColl" containerField="geometry"><Shape containerField="shape"><Box size="50 1 50"/></Shape></CollidableShape>
      </RigidBody>
      <RigidBody DEF="Chassis" containerField="bodies" mass="5" position="0 0.5 0">
        <CollidableShape containerField="geometry"><Shape containerField="shape"><Box size="1 0.4 2"/></Shape></CollidableShape>
      </RigidBody>
      <RigidBody DEF="WheelFL" containerField="bodies" mass="1" position="-0.6 0.4 0.8" orientation="0 0 1 -1.5708" torques="16 0 0">
        <CollidableShape containerField="geometry"><Shape containerField="shape"><Cylinder radius="0.4" height="0.2"/></Shape></CollidableShape>
      </RigidBody>
      <RigidBody DEF="WheelFR" containerField="bodies" mass="1" position="0.6 0.4 0.8" orientation="0 0 1 -1.5708" torques="-16 0 0">
        <CollidableShape containerField="geometry"><Shape containerField="shape"><Cylinder radius="0.4" height="0.2"/></Shape></CollidableShape>
      </RigidBody>
      <RigidBody DEF="WheelRL" containerField="bodies" mass="1" position="-0.6 0.4 -0.8" orientation="0 0 1 -1.5708" torques="16 0 0">
        <CollidableShape containerField="geometry"><Shape containerField="shape"><Cylinder radius="0.4" height="0.2"/></Shape></CollidableShape>
      </RigidBody>
      <RigidBody DEF="WheelRR" containerField="bodies" mass="1" position="0.6 0.4 -0.8" orientation="0 0 1 -1.5708" torques="-16 0 0">
        <CollidableShape containerField="geometry"><Shape containerField="shape"><Cylinder radius="0.4" height="0.2"/></Shape></CollidableShape>
      </RigidBody>
      <SingleAxisHingeJoint containerField="joints" axis="1 0 0" anchorPoint="-0.6 0.4 0.8"><RigidBody USE="WheelFL" containerField="body1"/><RigidBody USE="Chassis" containerField="body2"/></SingleAxisHingeJoint>
      <SingleAxisHingeJoint containerField="joints" axis="1 0 0" anchorPoint="0.6 0.4 0.8"><RigidBody USE="WheelFR" containerField="body1"/><RigidBody USE="Chassis" containerField="body2"/></SingleAxisHingeJoint>
      <SingleAxisHingeJoint containerField="joints" axis="1 0 0" anchorPoint="-0.6 0.4 -0.8"><RigidBody USE="WheelRL" containerField="body1"/><RigidBody USE="Chassis" containerField="body2"/></SingleAxisHingeJoint>
      <SingleAxisHingeJoint containerField="joints" axis="1 0 0" anchorPoint="0.6 0.4 -0.8"><RigidBody USE="WheelRR" containerField="body1"/><RigidBody USE="Chassis" containerField="body2"/></SingleAxisHingeJoint>
    </RigidBodyCollection>
    <ROUTE fromNode="Chassis" fromField="orientation" toNode="CarXform" toField="rotation"/>
  </Scene>
</X3D>
```

- [ ] **Step 2: Create the ramp fixture**

`tools/x3d-cli/fixtures/sim-vehicle-ramp.x3d` — same as the pivot fixture EXCEPT: (a) the `Ground` RigidBody gets `orientation="1 0 0 0.12"` (tilt ~7°); (b) all four wheels' `torques="-6 0 0"` (drive uphill, all-wheel-drive); (c) the ROUTE is `Chassis.position → CarXform.translation`. Full file:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE X3D PUBLIC "ISO//Web3D//DTD X3D 4.0//EN" "https://www.web3d.org/specifications/x3d-4.0.dtd">
<X3D profile="Full" version="4.0">
  <head>
    <meta name="description" content="sim vehicle incline climb: all-wheel drive (torque -6) carries the car up a ~7° tilted ground under tyre friction (§37 frictionCoefficients)."/>
  </head>
  <Scene>
    <Transform DEF="CarXform"><Shape><Box size="1 0.4 2"/></Shape></Transform>
    <RigidBodyCollection DEF="World" gravity="0 -9.8 0">
      <CollisionCollection containerField="collider" frictionCoefficients="1 1" appliedParameters='"BOUNCE" "FRICTION_COEFFICIENT-2"'>
        <CollidableShape USE="GroundColl" containerField="collidables"/>
      </CollisionCollection>
      <RigidBody DEF="Ground" containerField="bodies" fixed="true" position="0 -0.5 0" orientation="1 0 0 0.12">
        <CollidableShape DEF="GroundColl" containerField="geometry"><Shape containerField="shape"><Box size="50 1 50"/></Shape></CollidableShape>
      </RigidBody>
      <RigidBody DEF="Chassis" containerField="bodies" mass="5" position="0 0.5 0">
        <CollidableShape containerField="geometry"><Shape containerField="shape"><Box size="1 0.4 2"/></Shape></CollidableShape>
      </RigidBody>
      <RigidBody DEF="WheelFL" containerField="bodies" mass="1" position="-0.6 0.4 0.8" orientation="0 0 1 -1.5708" torques="-6 0 0">
        <CollidableShape containerField="geometry"><Shape containerField="shape"><Cylinder radius="0.4" height="0.2"/></Shape></CollidableShape>
      </RigidBody>
      <RigidBody DEF="WheelFR" containerField="bodies" mass="1" position="0.6 0.4 0.8" orientation="0 0 1 -1.5708" torques="-6 0 0">
        <CollidableShape containerField="geometry"><Shape containerField="shape"><Cylinder radius="0.4" height="0.2"/></Shape></CollidableShape>
      </RigidBody>
      <RigidBody DEF="WheelRL" containerField="bodies" mass="1" position="-0.6 0.4 -0.8" orientation="0 0 1 -1.5708" torques="-6 0 0">
        <CollidableShape containerField="geometry"><Shape containerField="shape"><Cylinder radius="0.4" height="0.2"/></Shape></CollidableShape>
      </RigidBody>
      <RigidBody DEF="WheelRR" containerField="bodies" mass="1" position="0.6 0.4 -0.8" orientation="0 0 1 -1.5708" torques="-6 0 0">
        <CollidableShape containerField="geometry"><Shape containerField="shape"><Cylinder radius="0.4" height="0.2"/></Shape></CollidableShape>
      </RigidBody>
      <SingleAxisHingeJoint containerField="joints" axis="1 0 0" anchorPoint="-0.6 0.4 0.8"><RigidBody USE="WheelFL" containerField="body1"/><RigidBody USE="Chassis" containerField="body2"/></SingleAxisHingeJoint>
      <SingleAxisHingeJoint containerField="joints" axis="1 0 0" anchorPoint="0.6 0.4 0.8"><RigidBody USE="WheelFR" containerField="body1"/><RigidBody USE="Chassis" containerField="body2"/></SingleAxisHingeJoint>
      <SingleAxisHingeJoint containerField="joints" axis="1 0 0" anchorPoint="-0.6 0.4 -0.8"><RigidBody USE="WheelRL" containerField="body1"/><RigidBody USE="Chassis" containerField="body2"/></SingleAxisHingeJoint>
      <SingleAxisHingeJoint containerField="joints" axis="1 0 0" anchorPoint="0.6 0.4 -0.8"><RigidBody USE="WheelRR" containerField="body1"/><RigidBody USE="Chassis" containerField="body2"/></SingleAxisHingeJoint>
    </RigidBodyCollection>
    <ROUTE fromNode="Chassis" fromField="position" toNode="CarXform" toField="translation"/>
  </Scene>
</X3D>
```

- [ ] **Step 3: Build the CLI + verify both fixtures behave (BEFORE capturing goldens)**

Run: `cd <repo-root> && cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=ON && cmake --build --preset dev --target x3d_cli`
Then:
- Pivot spins: `./build/x3d sim tools/x3d-cli/fixtures/sim-vehicle-pivot.x3d --fps 60 --ticks 210 --watch CarXform.rotation 2>/dev/null | tail -4` — the `CarXform.rotation` angle (4th value) should grow over time (chassis yawing about ~`0 1 0`).
- Ramp climbs: `./build/x3d sim tools/x3d-cli/fixtures/sim-vehicle-ramp.x3d --fps 60 --ticks 210 --watch CarXform.translation 2>/dev/null | tail -4` — the y component should be higher late than at the start.

If either doesn't behave (pivot doesn't rotate / ramp doesn't climb), STOP and report with the output — likely a joint-binding or torque-sign issue; do not capture a golden of wrong behavior.

- [ ] **Step 4: Capture both goldens**

```bash
cd <repo-root>
./build/x3d sim tools/x3d-cli/fixtures/sim-vehicle-pivot.x3d --fps 60 --ticks 210 --watch CarXform.rotation --json > tools/x3d-cli/goldens/sim-vehicle-pivot.trace.json
./build/x3d sim tools/x3d-cli/fixtures/sim-vehicle-ramp.x3d --fps 60 --ticks 210 --watch CarXform.translation --json > tools/x3d-cli/goldens/sim-vehicle-ramp.trace.json
```
Sanity-check: `tail -c 200 tools/x3d-cli/goldens/sim-vehicle-pivot.trace.json` shows a non-trivial rotation angle; `tail -c 200 …ramp.trace.json` shows the translation y elevated. If either golden is empty/static, STOP.

- [ ] **Step 5: Add the harness assertions**

In `tools/tests/x3d_cli_test.sh`, inside the existing `if [[ "$PHYSICS" == "1" ... ]]` block (alongside the other vehicle assertions), add:

```bash
    # Skid-steer pivot: the chassis yaws (rotation angle grows) + deterministic + golden.
    FIXTURE_PIV="$FIXTURES/sim-vehicle-pivot.x3d"
    if [[ -f "$FIXTURE_PIV" ]]; then
        piv=$("$CLI" sim "$FIXTURE_PIV" --fps 60 --ticks 210 --watch CarXform.rotation 2>/dev/null || true)
        if [[ "$piv" == *"CarXform.rotation"* && "$piv" != *"rotation = 0 0 1 0"*"rotation = 0 0 1 0"* ]]; then
            echo "ok:   sim vehicle pivot yaws the chassis"
        else
            echo "FAIL: sim vehicle pivot did not rotate (got tail: $(printf '%s' "$piv" | tail -2))"
            failures=$(( failures + 1 ))
        fi
        pp1="$TD/piv_r1.json"; pp2="$TD/piv_r2.json"
        "$CLI" sim "$FIXTURE_PIV" --fps 60 --ticks 210 --watch CarXform.rotation --json >"$pp1" 2>/dev/null || true
        "$CLI" sim "$FIXTURE_PIV" --fps 60 --ticks 210 --watch CarXform.rotation --json >"$pp2" 2>/dev/null || true
        cmp -s "$pp1" "$pp2" && echo "ok:   sim vehicle pivot is deterministic" || { echo "FAIL: pivot non-deterministic"; failures=$(( failures + 1 )); }
        if [[ -n "$GOLDENS" && -f "$GOLDENS/sim-vehicle-pivot.trace.json" ]]; then
            [[ "$(cat "$pp1")" == "$(cat "$GOLDENS/sim-vehicle-pivot.trace.json")" ]] && echo "ok:   sim vehicle pivot trace matches golden" || { echo "FAIL: pivot golden drift"; diff <(cat "$pp1") "$GOLDENS/sim-vehicle-pivot.trace.json" | head -20; failures=$(( failures + 1 )); }
        fi
    fi
    # Incline climb: the chassis y rises + deterministic + golden.
    FIXTURE_RMP="$FIXTURES/sim-vehicle-ramp.x3d"
    if [[ -f "$FIXTURE_RMP" ]]; then
        rmp=$("$CLI" sim "$FIXTURE_RMP" --fps 60 --ticks 210 --watch CarXform.translation --json 2>/dev/null || true)
        rr2="$TD/rmp_r2.json"
        "$CLI" sim "$FIXTURE_RMP" --fps 60 --ticks 210 --watch CarXform.translation --json >"$rr2" 2>/dev/null || true
        [[ -n "$rmp" ]] && printf '%s' "$rmp" > "$TD/rmp_r1.json"
        cmp -s "$TD/rmp_r1.json" "$rr2" && echo "ok:   sim vehicle ramp is deterministic" || { echo "FAIL: ramp non-deterministic"; failures=$(( failures + 1 )); }
        if [[ -n "$GOLDENS" && -f "$GOLDENS/sim-vehicle-ramp.trace.json" ]]; then
            [[ "$rmp" == "$(cat "$GOLDENS/sim-vehicle-ramp.trace.json")" ]] && echo "ok:   sim vehicle ramp trace matches golden" || { echo "FAIL: ramp golden drift"; diff <(printf '%s' "$rmp") "$GOLDENS/sim-vehicle-ramp.trace.json" | head -20; failures=$(( failures + 1 )); }
        fi
    fi
```

> NOTE: the pivot "yaws" glob is heuristic (the golden-regression is the real guard). If the negated double-`0 0 1 0` glob is awkward in your shell, replace the rotate check with: assert the LAST rotation record's angle (4th field, via `awk '{print $NF}'` on the last `CarXform.rotation` line of the non-JSON trace) exceeds ~0.5, consistent with the existing harness's `awk`-based extraction style.

- [ ] **Step 6: Run the harness + verify**

Run: `cd <repo-root> && ctest --preset dev -R x3d_cli_test --output-on-failure 2>&1 | tail -25`
Expected: PASS — pivot yaws + deterministic + golden; ramp deterministic + golden.

- [ ] **Step 7: Commit**

```bash
git add tools/x3d-cli/fixtures/sim-vehicle-pivot.x3d tools/x3d-cli/fixtures/sim-vehicle-ramp.x3d tools/x3d-cli/goldens/sim-vehicle-pivot.trace.json tools/x3d-cli/goldens/sim-vehicle-ramp.trace.json tools/tests/x3d_cli_test.sh
git commit -m "test(physics): x3d sim pivot + ramp golden fixtures (vehicle dynamics)"
```

---

### Task 3: Docs note + dual-config verification

**Files:**
- Modify: `docs/superpowers/BACKLOG.md`

**Interfaces:**
- Consumes: the merged demonstration (Tasks 1-2).

- [ ] **Step 1: Add the demonstration note to BACKLOG.md**

In `docs/superpowers/BACKLOG.md`, near the RigidBodyPhysics narrative paragraph (where the §37 closure pass is summarized), add a one-line note (mirroring the existing prose style), e.g.:

> Vehicle dynamics demonstrated (commit `<merge>`): straight drive, **skid-steer pivot** (opposing wheel torque overpowers lateral grip — un-steered gripping wheels can't roll an Ackermann arc), incline climb (~7° grade), and brake/reverse — all from existing torque + friction + hinge axles (gated maneuver tests + `sim-vehicle-pivot`/`sim-vehicle-ramp` goldens). Steered-wheel (Ackermann) cornering needs a steering-pivot joint → deferred under CONF-RBP-JOINTS.

No `findings.yaml` change (no new spec field implemented).

- [ ] **Step 2: Final dual-config verification**

Ungated (physics OFF, the default): `cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=OFF && cmake --build --preset dev && ctest --preset dev -j "$(nproc)" --output-on-failure 2>&1 | tail -4`
Expected: all pass (the maneuver tests + fixtures are gated, so this just confirms no accidental coupling).

Gated (physics ON): `cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=ON && cmake --build --preset dev && ctest --preset dev -j "$(nproc)" --output-on-failure 2>&1 | tail -4`
Expected: all pass — `x3d_physics_system` (the 4 maneuvers) + `x3d_cli_test` (pivot + ramp goldens, and existing goldens unchanged).

Capture both tallies. If either fails, STOP and report BLOCKED. Leave `build/` configured with physics ON.

- [ ] **Step 3: Commit**

```bash
git add docs/superpowers/BACKLOG.md
git commit -m "docs(physics): note vehicle-dynamics demonstration (pivot/incline/brake-reverse)"
```

> NOTE: update the auto-memory `memory/x3d-cpp-gen-physics-seam.md` after merge (vehicle dynamics demonstrated; skid-steer-not-Ackermann lesson) — a memory write, not a branch commit.

---

## Self-Review

**Spec coverage:**
- §5 four maneuvers (pivot, upright, incline, brake/reverse) → Task 1 (a/b/c) + determinism (d). ✓ (upright = the `axisY > 0.9` assertion in the pivot.)
- §6 gated C++ backend-level builder + maneuvers → Task 1. ✓
- §6 CLI pivot + ramp fixtures/goldens/harness → Task 2. ✓
- §3 honest skid-steer framing → Task 1 comment + Task 3 docs note. ✓
- §6 regression (existing goldens byte-identical, dual-config) → Task 3 Step 2. ✓
- §7 files ↔ File Structure. ✓
- §8 no production code / no findings.yaml change → honored (Task 3 says so explicitly). ✓

**Placeholder scan:** No TBD/TODO; full code/fixtures/commands in every step. The conceptual-RED NOTE (Task 1 Step 2) and the pivot-glob NOTE (Task 2 Step 5) give concrete fallbacks. `<merge>` in Task 3 is an intentional post-merge hash fill.

**Type consistency:** Vehicle geometry constants identical to the spike + Global Constraints across Task 1 and the fixtures; wheel index convention (0/2 left +16, 1/3 right −16 for pivot; all −6 for ramp) consistent; `setContactResponse(world, 1.0f, -1.0f)` matches the merged seam; fixture node attributes match the verified bindings + the working sim-vehicle.x3d patterns (USE-as-child for joint body1/body2, appliedParameters MFString form).
