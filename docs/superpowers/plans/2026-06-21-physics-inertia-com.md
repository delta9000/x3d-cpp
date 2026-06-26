# §37 RigidBody inertia + centerOfMass Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Honor `RigidBody.inertia` (explicit tensor) and `RigidBody.centerOfMass` (COM offset) in the Jolt physics seam, demonstrated and verified by an open-loop quadcopter flight sim — partially closing CONF-RBP-INERTIA.

**Architecture:** Both fields take effect at body-creation time. A new backend-neutral `MassProperties` struct replaces `addBody`'s `float mass`. `PhysicsSystem` reads the RigidBody fields and uses non-default detection (inertia ≠ identity, COM ≠ 0) to populate it. `JoltBackend` maps it to `EOverrideMassProperties::MassAndInertiaProvided` + a `Mat44` tensor and an `OffsetCenterOfMassShape` wrap, and reads pose back from the body origin (`GetPosition`) so `RigidBody.position` stays the authored reference point regardless of COM offset.

**Tech Stack:** C++20, header-only runtime core, Jolt Physics v5.5.0 (gated), CMake/CTest, generated X3D node bindings.

## Global Constraints

- **Seam purity (verbatim invariant):** `runtime/physics/PhysicsBackend.hpp`, `PhysicsSystem.hpp`, `ContactReporter.hpp` must name **no** Jolt/JPH *type* (doc-comment word mentions are fine). All engine coupling lives only in `runtime/physics/jolt/JoltBackend.{hpp,cpp}`. Verify: `grep -rE 'JPH|Jolt' runtime/physics/PhysicsBackend.hpp runtime/physics/PhysicsSystem.hpp runtime/physics/ContactReporter.hpp` → only comment lines.
- **Override rule (non-default detection):** `overrideInertia = (getInertia() != identity)` via exact elementwise compare of `SFMatrix3f.matrix[3][3]` against `1 0 0 / 0 1 0 / 0 0 1`; `centerOfMass` passed through, `(0,0,0)` = no offset. No new/non-standard fields.
- **Zero golden drift for centered bodies:** switching `getBodyTransform` from `GetCenterOfMassPosition` → `GetPosition` must leave every existing golden trace byte-identical (origin == COM for symmetric origin-centered bodies). This is a hard gate, not an assumption.
- **Physics is gated:** Jolt backend + its tests build only under `-DX3D_CPP_BUILD_PHYSICS=ON`. The `MassProperties` struct + `PhysicsSystem` detection are header-only and must build/test **without** the flag (physics-OFF is the default distribution).
- **Build/test commands:**
  - Ungated build + ctest: `cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=OFF && cmake --build --preset dev && ctest --preset dev -j "$(nproc)" --output-on-failure`.
  - Gated: `cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=ON && cmake --build --preset dev && ctest --preset dev -j "$(nproc)" --output-on-failure`.
  - Build a single target while iterating: `cmake --build --preset dev --target <name>`. The dev preset caps compile jobs (OOM at high `-j`); do NOT override the build `-j`. `ctest -j "$(nproc)"` is fine.
  - The CMake cache is sticky: re-running `cmake --preset dev` keeps the last `X3D_CPP_BUILD_PHYSICS` value unless you pass it explicitly.
- **TDD:** red → green → commit per step. Frequent commits. DRY, YAGNI.
- **Verified node/type facts (against the bindings + Jolt v5.5 headers):**
  - `SFMatrix3f { float matrix[3][3]; }` (X3Dtypes.hpp); default `inertia` = identity.
  - `RigidBody`: `getMass()`→SFFloat, `getInertia()`→SFMatrix3f, `getCenterOfMass()`→SFVec3f; setters `setMass`, `setInertia(const SFMatrix3f&)`, `setCenterOfMass(const SFVec3f&)` (no `Unchecked` needed — no range constraints).
  - Node-build idiom (from `physics_system_test.cpp`): `box->setSizeUnchecked(SFVec3f{...})`, `shape->setGeometry(std::static_pointer_cast<X3DNode>(box))`, `coll->setShapeUnchecked(std::static_pointer_cast<X3DNode>(shape))`, `body->setGeometry(MFNode{std::static_pointer_cast<X3DNode>(coll)})`, `collection->setBodies(MFNode{std::static_pointer_cast<X3DNode>(body)})`. `X3DNode` is global.
  - Jolt: `EOverrideMassProperties::MassAndInertiaProvided`; `MassProperties.mMass` (float), `.mInertia` (Mat44); `Mat44::sZero()`, `SetColumn3(uint,Vec3Arg)` (sets 4th comp 0 for cols 0-2), `SetColumn4(uint,Vec4Arg)`; `OffsetCenterOfMassShapeSettings(Vec3Arg offset, const Shape*)` → `.Create()`; `BodyInterface::GetPosition(id)` (origin) vs `GetCenterOfMassPosition(id)` (COM). `applyForce` already does `AddForceAndTorque`, so `RigidBody.torques` already drive rotation.

## File Structure

- `runtime/physics/PhysicsBackend.hpp` — **modify**: add `MassProperties`; change `addBody` signature.
- `runtime/physics/PhysicsSystem.hpp` — **modify**: `isIdentity3x3` helper; build `MassProperties` in `attach`.
- `runtime/physics/jolt/JoltBackend.hpp` — **modify**: `addBody` signature.
- `runtime/physics/jolt/JoltBackend.cpp` — **modify**: `addBody` inertia+COM mapping; `getBodyTransform` origin readback.
- `runtime/physics/tests/inertia_massprops_test.cpp` — **create**: ungated `MassProperties` defaults + `PhysicsSystem` detection (recording fake backend).
- `runtime/physics/tests/jolt_backend_test.cpp` — **modify**: update `addBody` call sites.
- `runtime/physics/tests/physics_contact_wiring_test.cpp` — **modify**: update the fake backend's `addBody` signature.
- `runtime/physics/tests/physics_system_test.cpp` — **modify**: gated inertia/COM/quad assertions.
- `tools/x3d-cli/fixtures/sim-quadcopter.x3d`, `tools/x3d-cli/goldens/sim-quadcopter.trace.json` — **create**.
- `tools/tests/x3d_cli_test.sh` — **modify**: quad golden assertion.
- `CMakeLists.txt` — **modify**: register `x3d_inertia_massprops`.
- `docs/conformance/findings.yaml`, `docs/conformance/components/RigidBodyPhysics.md`, `docs/superpowers/BACKLOG.md` — **modify**: narrow CONF-RBP-INERTIA.

---

### Task 1: `MassProperties` seam + propagate the `addBody` signature

**Files:**
- Modify: `runtime/physics/PhysicsBackend.hpp`, `runtime/physics/jolt/JoltBackend.hpp`, `runtime/physics/jolt/JoltBackend.cpp`, `runtime/physics/PhysicsSystem.hpp`, `runtime/physics/tests/jolt_backend_test.cpp`, `runtime/physics/tests/physics_contact_wiring_test.cpp`
- Create: `runtime/physics/tests/inertia_massprops_test.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `struct MassProperties { float mass; bool overrideInertia; SFMatrix3f inertia; SFVec3f centerOfMass; }`; `addBody(WorldHandle, const ShapeDesc&, const MassProperties&, bool fixed, const SFVec3f&, const SFRotation&, const SFVec3f&, const SFVec3f&)`.

This is a refactor (no new physics behavior): introduce the struct, thread it through every `addBody` site, keep behavior identical (Jolt still `CalculateInertia` from mass only). Gate = the new defaults test + both build configs green.

- [ ] **Step 1: Write the failing defaults test**

Create `runtime/physics/tests/inertia_massprops_test.cpp`:

```cpp
// inertia_massprops_test.cpp — ungated tests for the §37 inertia/centerOfMass
// path: the MassProperties seam struct (Task 1) and PhysicsSystem's non-default
// detection (Task 2). No physics engine required.
#include "PhysicsBackend.hpp"

#include <cstdio>

using namespace x3d::runtime;

static int g_failures = 0;
#define CHECK(cond, msg)                                                        \
  do {                                                                          \
    if (!(cond)) { std::printf("FAIL: %s\n", msg); ++g_failures; }             \
    else { std::printf("ok:   %s\n", msg); }                                   \
  } while (0)

static void test_massprops_defaults() {
  MassProperties mp;
  CHECK(mp.mass == 1.0f, "MassProperties.mass defaults to 1");
  CHECK(mp.overrideInertia == false, "overrideInertia defaults false");
  CHECK(mp.centerOfMass.x == 0.0f && mp.centerOfMass.y == 0.0f &&
            mp.centerOfMass.z == 0.0f,
        "centerOfMass defaults to origin");
}

int main() {
  test_massprops_defaults();
  return g_failures == 0 ? 0 : 1;
}
```

- [ ] **Step 2: Verify it fails to compile**

Run: `cd <repo-root> && g++ -std=c++20 -Iruntime/physics -Igenerated_cpp_bindings -fsyntax-only runtime/physics/tests/inertia_massprops_test.cpp`
Expected: FAIL — `'MassProperties' was not declared`.

- [ ] **Step 3: Add the `MassProperties` struct**

In `runtime/physics/PhysicsBackend.hpp`, add immediately after the `ContactPoint` struct:

```cpp
/**
 * @brief A body's mass properties, in the runtime's own terms.
 * @details mass is always used. overrideInertia=false (default) → the backend
 *          derives the inertia tensor from the collision shape (the historical
 *          behavior); true → it uses the 3x3 `inertia` verbatim (§37
 *          RigidBody.inertia). A centerOfMass other than (0,0,0) offsets the
 *          body's COM from the shape centroid (§37 RigidBody.centerOfMass). No
 *          engine type appears here.
 */
struct MassProperties {
  float      mass = 1.0f;
  bool       overrideInertia = false;
  SFMatrix3f inertia{};              // 3x3, used iff overrideInertia
  SFVec3f    centerOfMass{0, 0, 0};  // (0,0,0) → no offset
};
```

- [ ] **Step 4: Verify the defaults test passes**

Run: `cd <repo-root> && g++ -std=c++20 -Iruntime/physics -Igenerated_cpp_bindings -o /tmp/imp_t runtime/physics/tests/inertia_massprops_test.cpp && /tmp/imp_t`
Expected: all `ok:`; exit 0.

- [ ] **Step 5: Change the `addBody` signature (declaration)**

In `runtime/physics/PhysicsBackend.hpp`, change the `addBody` declaration's `float mass` parameter to `const MassProperties &mass`:

```cpp
  virtual BodyHandle addBody(WorldHandle world, const ShapeDesc &shape,
                             const MassProperties &mass, bool fixed,
                             const SFVec3f &pos, const SFRotation &ori,
                             const SFVec3f &linVel, const SFVec3f &angVel) = 0;
```

- [ ] **Step 6: Change `JoltBackend` declaration + definition**

In `runtime/physics/jolt/JoltBackend.hpp`, change the `addBody` override's `float mass` → `const MassProperties &mass`:

```cpp
  BodyHandle addBody(WorldHandle world, const ShapeDesc &shape,
                     const MassProperties &mass, bool fixed, const SFVec3f &pos,
                     const SFRotation &ori, const SFVec3f &linVel,
                     const SFVec3f &angVel) override;
```

In `runtime/physics/jolt/JoltBackend.cpp`, change the definition signature the same way, and update the two `mass` reads inside the body (no inertia/COM yet — that is Task 3):

```cpp
BodyHandle JoltBackend::addBody(WorldHandle world, const ShapeDesc &shape,
                                const MassProperties &mass, bool fixed,
                                const SFVec3f &pos, const SFRotation &ori,
                                const SFVec3f &linVel, const SFVec3f &angVel) {
```
and inside the `if (!fixed)` block:
```cpp
    if (mass.mass > 0.0f) {
      settings.mOverrideMassProperties =
          EOverrideMassProperties::CalculateInertia;
      settings.mMassPropertiesOverride.mMass = mass.mass;
    }
```

- [ ] **Step 7: Update the real caller in `PhysicsSystem`**

In `runtime/physics/PhysicsSystem.hpp` (`attach`, the `backend_->addBody(...)` call ~line 128), change the mass argument `body->getMass()` to `MassProperties{body->getMass()}`:

```cpp
      BodyHandle handle = backend_->addBody(
          world, shape, MassProperties{body->getMass()}, body->getFixed(),
          body->getPosition(), body->getOrientation(),
          body->getLinearVelocity(), body->getAngularVelocity());
```

- [ ] **Step 8: Update the fake backend in the wiring test**

In `runtime/physics/tests/physics_contact_wiring_test.cpp`, change the fake's `addBody` override signature (`float` → `const MassProperties &`) and keep the body unchanged:

```cpp
  BodyHandle addBody(WorldHandle, const ShapeDesc &, const MassProperties &, bool,
                     const SFVec3f &, const SFRotation &, const SFVec3f &,
                     const SFVec3f &) override {
    return ++lastHandle_;
  }
```

- [ ] **Step 9: Update the gated `jolt_backend_test.cpp` call sites**

In `runtime/physics/tests/jolt_backend_test.cpp`, wrap each `addBody` mass float argument in `MassProperties{...}`. There are calls at ~lines 36, 60, 62, 92, 128. Each currently passes a bare float (e.g. `1.0f`, `0.0f`) as the 3rd argument; change each to `MassProperties{<that float>}`. Example:

```cpp
  // before: backend.addBody(world, ShapeDesc::sphere(0.5f), /*mass*/ 1.0f, false, ...)
  // after:
  BodyHandle body = backend.addBody(world, ShapeDesc::sphere(0.5f),
                                    MassProperties{/*mass*/ 1.0f}, false, ...);
```
Apply the same wrap to all five call sites (including the fixed floor `MassProperties{0.0f}`).

- [ ] **Step 10: Register the ungated test in CMake**

In `CMakeLists.txt`, near the other ungated runtime tests (e.g. after the `x3d_contact_reporter` block), add:

```cmake
    # §37 inertia/centerOfMass: MassProperties seam + PhysicsSystem detection.
    # Ungated — the detection is engine-free; only the Jolt mapping needs the flag.
    add_executable(x3d_inertia_massprops
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics/tests/inertia_massprops_test.cpp")
    target_link_libraries(x3d_inertia_massprops PRIVATE x3d_cpp::x3d_cpp)
    target_include_directories(x3d_inertia_massprops PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics")
    add_test(NAME x3d_inertia_massprops COMMAND x3d_inertia_massprops)
```

- [ ] **Step 11: Build + test BOTH configs (refactor must not regress either)**

Ungated:
```
cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=OFF && cmake --build --preset dev && ctest --preset dev -j "$(nproc)" --output-on-failure 2>&1 | tail -4
```
Expected: all pass, including `x3d_inertia_massprops`.

Gated:
```
cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=ON && cmake --build --preset dev && ctest --preset dev -j "$(nproc)" --output-on-failure 2>&1 | tail -4
```
Expected: all pass (jolt_backend_test, physics_system, etc. compile with the new signature, behavior unchanged).

- [ ] **Step 12: Commit**

```bash
git add runtime/physics/PhysicsBackend.hpp runtime/physics/jolt/JoltBackend.hpp runtime/physics/jolt/JoltBackend.cpp runtime/physics/PhysicsSystem.hpp runtime/physics/tests/inertia_massprops_test.cpp runtime/physics/tests/jolt_backend_test.cpp runtime/physics/tests/physics_contact_wiring_test.cpp CMakeLists.txt
git commit -m "refactor(physics): MassProperties seam replaces addBody float mass (CONF-RBP-INERTIA)"
```

---

### Task 2: PhysicsSystem non-default detection (ungated)

**Files:**
- Modify: `runtime/physics/PhysicsSystem.hpp`
- Test: `runtime/physics/tests/inertia_massprops_test.cpp` (extend)

**Interfaces:**
- Consumes: `MassProperties` (Task 1).
- Produces: `PhysicsSystem::attach` now fills `MassProperties{mass, overrideInertia, inertia, centerOfMass}` from each RigidBody; static helper `isIdentity3x3(const SFMatrix3f&)`.

- [ ] **Step 1: Write the failing detection test**

Append to `runtime/physics/tests/inertia_massprops_test.cpp` (add includes at top: `#include "PhysicsSystem.hpp"`, `#include "X3DExecutionContext.hpp"`, `#include "Box.hpp"`, `#include "CollidableShape.hpp"`, `#include "RigidBody.hpp"`, `#include "RigidBodyCollection.hpp"`, `#include "Shape.hpp"`, `#include <memory>`):

```cpp
// A fake backend that records the MassProperties passed to the LAST addBody, so
// we can assert PhysicsSystem's non-default detection without a physics engine.
class RecordingBackend : public PhysicsBackend {
public:
  MassProperties lastMass;
  WorldHandle createWorld(const SFVec3f &) override { return 1; }
  BodyHandle addBody(WorldHandle, const ShapeDesc &, const MassProperties &m,
                     bool, const SFVec3f &, const SFRotation &, const SFVec3f &,
                     const SFVec3f &) override {
    lastMass = m;
    return ++h_;
  }
  ConstraintHandle addConstraint(WorldHandle, const ConstraintDesc &) override {
    return 1;
  }
  void applyForce(WorldHandle, BodyHandle, const SFVec3f &,
                  const SFVec3f &) override {}
  void setGravityFactor(WorldHandle, BodyHandle, float) override {}
  void getBodyVelocity(WorldHandle, BodyHandle, SFVec3f &l,
                       SFVec3f &a) const override { l = {0,0,0}; a = {0,0,0}; }
  void step(WorldHandle, double) override {}
  void getBodyTransform(WorldHandle, BodyHandle, SFVec3f &p,
                        SFRotation &o) const override {
    p = {0,0,0}; o = {0,0,1,0};
  }

private:
  BodyHandle h_ = 0;
};

// Build a one-body collection with a Box collidable; the caller sets inertia/COM.
static std::shared_ptr<RigidBodyCollection>
makeOneBody(std::shared_ptr<RigidBody> &outBody) {
  auto box = std::make_shared<Box>();
  box->setSizeUnchecked(SFVec3f{1, 1, 1});
  auto shape = std::make_shared<Shape>();
  shape->setGeometry(std::static_pointer_cast<X3DNode>(box));
  auto coll = std::make_shared<CollidableShape>();
  coll->setShapeUnchecked(std::static_pointer_cast<X3DNode>(shape));
  outBody = std::make_shared<RigidBody>();
  outBody->setGeometry(MFNode{std::static_pointer_cast<X3DNode>(coll)});
  auto collection = std::make_shared<RigidBodyCollection>();
  collection->setBodies(MFNode{std::static_pointer_cast<X3DNode>(outBody)});
  return collection;
}

static void test_default_inertia_not_overridden() {
  std::shared_ptr<RigidBody> body;
  auto collection = makeOneBody(body);  // inertia defaults to identity
  auto backend = std::make_shared<RecordingBackend>();
  PhysicsSystem sys(backend);
  x3d::runtime::X3DExecutionContext ctx;
  sys.attach(collection.get(), ctx);
  CHECK(backend->lastMass.overrideInertia == false,
        "identity inertia -> overrideInertia false (shape-derived)");
}

static void test_nondefault_inertia_overridden() {
  std::shared_ptr<RigidBody> body;
  auto collection = makeOneBody(body);
  SFMatrix3f tensor{};                  // diag(0.01, 0.02, 0.03)
  tensor.matrix[0][0] = 0.01f; tensor.matrix[1][1] = 0.02f; tensor.matrix[2][2] = 0.03f;
  body->setInertia(tensor);
  auto backend = std::make_shared<RecordingBackend>();
  PhysicsSystem sys(backend);
  x3d::runtime::X3DExecutionContext ctx;
  sys.attach(collection.get(), ctx);
  CHECK(backend->lastMass.overrideInertia == true,
        "non-identity inertia -> overrideInertia true");
  CHECK(backend->lastMass.inertia.matrix[1][1] == 0.02f,
        "authored inertia tensor passed through");
}

static void test_centerofmass_passed_through() {
  std::shared_ptr<RigidBody> body;
  auto collection = makeOneBody(body);
  body->setCenterOfMass(SFVec3f{0.5f, 0, 0});
  auto backend = std::make_shared<RecordingBackend>();
  PhysicsSystem sys(backend);
  x3d::runtime::X3DExecutionContext ctx;
  sys.attach(collection.get(), ctx);
  CHECK(backend->lastMass.centerOfMass.x == 0.5f,
        "centerOfMass passed through to the backend");
}
```

Add calls in `main()`:
```cpp
  test_default_inertia_not_overridden();
  test_nondefault_inertia_overridden();
  test_centerofmass_passed_through();
```

- [ ] **Step 2: Build + verify the detection tests FAIL**

Run: `cd <repo-root> && cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=OFF && cmake --build --preset dev --target x3d_inertia_massprops && ctest --preset dev -R x3d_inertia_massprops --output-on-failure 2>&1 | tail -8`
Expected: FAIL — the non-identity/COM cases fail because `attach` still passes `MassProperties{getMass()}` only (overrideInertia false, centerOfMass zero).

- [ ] **Step 3: Add the `isIdentity3x3` helper to `PhysicsSystem`**

In `runtime/physics/PhysicsSystem.hpp`, add a private static helper (near `shapeForBody`):

```cpp
  /// True iff a 3x3 matrix equals the identity (the §37 inertia default). Exact
  /// compare: the codegen default is exactly identity; an author who writes
  /// identity has authored the degenerate case we treat as "derive from shape".
  static bool isIdentity3x3(const SFMatrix3f &m) {
    for (int r = 0; r < 3; ++r)
      for (int c = 0; c < 3; ++c)
        if (m.matrix[r][c] != (r == c ? 1.0f : 0.0f)) return false;
    return true;
  }
```

- [ ] **Step 4: Populate `MassProperties` in `attach`**

In `runtime/physics/PhysicsSystem.hpp`, replace the `addBody` call (from Task 1 Step 7) so it builds the full `MassProperties`:

```cpp
      MassProperties mp;
      mp.mass = body->getMass();
      const SFMatrix3f inertia = body->getInertia();
      if (!isIdentity3x3(inertia)) {
        mp.overrideInertia = true;
        mp.inertia = inertia;
      }
      mp.centerOfMass = body->getCenterOfMass();
      BodyHandle handle = backend_->addBody(
          world, shape, mp, body->getFixed(), body->getPosition(),
          body->getOrientation(), body->getLinearVelocity(),
          body->getAngularVelocity());
```

- [ ] **Step 5: Build + verify the detection tests PASS**

Run: `cd <repo-root> && cmake --build --preset dev --target x3d_inertia_massprops && ctest --preset dev -R x3d_inertia_massprops --output-on-failure 2>&1 | tail -8`
Expected: all `ok:`; PASS.

- [ ] **Step 6: Verify seam purity + the full ungated suite**

Run: `grep -rE 'JPH|Jolt' runtime/physics/PhysicsSystem.hpp | grep -v '//' | grep -v '\*'; echo "exit $?"`
Expected: no engine-type matches (only comment lines, if any).

Run: `ctest --preset dev -j "$(nproc)" --output-on-failure 2>&1 | tail -4`
Expected: all pass (ungated config still selected from Task 1 Step 11 / Task 2 Step 2).

- [ ] **Step 7: Commit**

```bash
git add runtime/physics/PhysicsSystem.hpp runtime/physics/tests/inertia_massprops_test.cpp
git commit -m "feat(physics): PhysicsSystem detects authored inertia/centerOfMass (CONF-RBP-INERTIA)"
```

---

### Task 3: JoltBackend inertia + COM mapping + origin readback (gated)

**Files:**
- Modify: `runtime/physics/jolt/JoltBackend.cpp`
- Test: `runtime/physics/tests/physics_system_test.cpp` (extend)

**Interfaces:**
- Consumes: `MassProperties` (Task 1), detection (Task 2).
- Produces: `JoltBackend::addBody` honors `overrideInertia` + `centerOfMass`; `getBodyTransform` returns the body origin.

- [ ] **Step 1: Write the failing gated tests**

In `runtime/physics/tests/physics_system_test.cpp`, add a block to `main()` (it already includes Box/Shape/CollidableShape/RigidBody/RigidBodyCollection/Transform and uses the node-build idiom). Use a zero-gravity collection so the roll angle is isolated from linear motion:

```cpp
  // ── Inertia honored: under a constant torque about x, the roll angle after t
  //    must match theta = 1/2 * (tau/Ixx) * t^2, AND differ from the shape-derived
  //    (default-inertia) case — proving the authored tensor is actually used.
  {
    auto run = [](bool authored) -> float {
      auto box = std::make_shared<Box>();
      box->setSizeUnchecked(SFVec3f{1, 0.2f, 1});            // flat frame
      auto shp = std::make_shared<Shape>();
      shp->setGeometry(std::static_pointer_cast<X3DNode>(box));
      auto coll = std::make_shared<CollidableShape>();
      coll->setShapeUnchecked(std::static_pointer_cast<X3DNode>(shp));
      auto body = std::make_shared<RigidBody>();
      body->setMass(1.0f);
      if (authored) {
        SFMatrix3f I{};
        I.matrix[0][0] = 0.01f; I.matrix[1][1] = 0.02f; I.matrix[2][2] = 0.01f;
        body->setInertia(I);                                 // Ixx = 0.01
      }
      body->setTorques(std::vector<SFVec3f>{SFVec3f{0.02f, 0, 0}});  // tau_x
      body->setGeometry(MFNode{std::static_pointer_cast<X3DNode>(coll)});
      auto world = std::make_shared<RigidBodyCollection>();
      world->setGravity(SFVec3f{0, 0, 0});                   // isolate rotation
      world->setBodies(MFNode{std::static_pointer_cast<X3DNode>(body)});
      X3DExecutionContext ctx;
      auto phys = std::make_shared<PhysicsSystem>(std::make_shared<JoltBackend>());
      phys->attach(world.get(), ctx);
      ctx.addSystem(phys);
      double t = 0.0; ctx.tick(t);
      for (int i = 0; i < 30; ++i) { t += 1.0 / 60.0; ctx.tick(t); } // 0.5 s
      return body->getOrientation().angle;                  // roll about x
    };
    float authored = run(true);
    float derived = run(false);
    // theta = 1/2 * (0.02/0.01) * 0.5^2 = 0.25 rad (within discrete-integration tol).
    CHECK(std::fabs(authored - 0.25f) < 0.05f,
          "authored inertia: roll matches tau/Ixx (theta ~= 0.25 rad)");
    CHECK(std::fabs(authored - derived) > 0.05f,
          "authored inertia DIFFERS from shape-derived (tensor is honored)");
  }

  // ── centerOfMass honored: a body given a pure spin (no forces/gravity) with an
  //    offset COM has its reported ORIGIN orbit the COM (origin != COM), while a
  //    centered body's origin stays put. Requires getBodyTransform = GetPosition.
  {
    auto originMoved = [](bool offset) -> float {
      auto box = std::make_shared<Box>();
      box->setSizeUnchecked(SFVec3f{1, 1, 1});
      auto shp = std::make_shared<Shape>();
      shp->setGeometry(std::static_pointer_cast<X3DNode>(box));
      auto coll = std::make_shared<CollidableShape>();
      coll->setShapeUnchecked(std::static_pointer_cast<X3DNode>(shp));
      auto body = std::make_shared<RigidBody>();
      body->setMass(1.0f);
      if (offset) body->setCenterOfMass(SFVec3f{0.5f, 0, 0});
      body->setAngularVelocity(SFVec3f{0, 0, 4});            // spin about z
      body->setGeometry(MFNode{std::static_pointer_cast<X3DNode>(coll)});
      auto world = std::make_shared<RigidBodyCollection>();
      world->setGravity(SFVec3f{0, 0, 0});
      world->setBodies(MFNode{std::static_pointer_cast<X3DNode>(body)});
      X3DExecutionContext ctx;
      auto phys = std::make_shared<PhysicsSystem>(std::make_shared<JoltBackend>());
      phys->attach(world.get(), ctx);
      ctx.addSystem(phys);
      double t = 0.0; ctx.tick(t);
      for (int i = 0; i < 15; ++i) { t += 1.0 / 60.0; ctx.tick(t); }
      const SFVec3f p = body->getPosition();
      return std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z); // origin displacement
    };
    CHECK(originMoved(true) > 0.05f,
          "offset COM: body origin orbits the COM under spin");
    CHECK(originMoved(false) < 1e-3f,
          "centered COM: body origin stays put under spin");
  }
```

Ensure `#include <cmath>` and `<memory>` are present (they are in this test).

- [ ] **Step 2: Build gated + verify the new tests FAIL**

Run: `cd <repo-root> && cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=ON && cmake --build --preset dev --target x3d_physics_system_test && ctest --preset dev -R x3d_physics_system --output-on-failure 2>&1 | tail -12`
Expected: FAIL — authored inertia == derived (override not applied yet), and the offset-COM origin doesn't move (COM ignored + readback is still COM, which is fixed under spin).

- [ ] **Step 3: Implement inertia override + COM wrap in `addBody`**

In `runtime/physics/jolt/JoltBackend.cpp`, in `addBody`, after `shapeRef` is built (the if/else chain that creates the Box/Sphere/Cylinder shape) and BEFORE `BodyCreationSettings settings(...)`, insert the COM wrap:

```cpp
  // §37 centerOfMass: offset the COM from the shape centroid by wrapping the
  // shape. Our primitives are origin-centered (centroid 0), so offset == com.
  if (mass.centerOfMass.x != 0.0f || mass.centerOfMass.y != 0.0f ||
      mass.centerOfMass.z != 0.0f) {
    OffsetCenterOfMassShapeSettings comSettings(
        Vec3(mass.centerOfMass.x, mass.centerOfMass.y, mass.centerOfMass.z),
        shapeRef);
    comSettings.SetEmbedded();
    ShapeSettings::ShapeResult comRes = comSettings.Create();
    if (comRes.HasError()) return kInvalidBodyHandle;
    shapeRef = comRes.Get();
  }
```

Add the include with the other Jolt shape includes near the top of the file:
```cpp
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>
```

Then replace the existing mass-properties block inside `if (!fixed)` with the inertia-override branch:

```cpp
    if (mass.mass > 0.0f) {
      settings.mMassPropertiesOverride.mMass = mass.mass;
      if (mass.overrideInertia) {
        // §37 RigidBody.inertia: use the authored 3x3 tensor verbatim. SFMatrix3f
        // is matrix[row][col]; build the Jolt Mat44 column-by-column (symmetric
        // tensor → row/col convention is immaterial).
        const SFMatrix3f &m = mass.inertia;
        Mat44 inertia = Mat44::sZero();
        inertia.SetColumn3(0, Vec3(m.matrix[0][0], m.matrix[1][0], m.matrix[2][0]));
        inertia.SetColumn3(1, Vec3(m.matrix[0][1], m.matrix[1][1], m.matrix[2][1]));
        inertia.SetColumn3(2, Vec3(m.matrix[0][2], m.matrix[1][2], m.matrix[2][2]));
        inertia.SetColumn4(3, Vec4(0, 0, 0, 1));
        settings.mMassPropertiesOverride.mInertia = inertia;
        settings.mOverrideMassProperties =
            EOverrideMassProperties::MassAndInertiaProvided;
      } else {
        settings.mOverrideMassProperties =
            EOverrideMassProperties::CalculateInertia;
      }
    }
```

- [ ] **Step 4: Switch `getBodyTransform` to the body origin**

In `runtime/physics/jolt/JoltBackend.cpp`, in `getBodyTransform`, change the position read from the COM to the origin:

```cpp
  // §37 RigidBody.position is the body's origin (the authored reference point),
  // NOT its center of mass — these differ when centerOfMass is offset. Report
  // the origin so a routed Transform tracks the authored placement. For centered
  // bodies origin == COM, so existing golden traces are unchanged.
  RVec3 p = bi.GetPosition(id);
```
(Replace the existing `RVec3 p = bi.GetCenterOfMassPosition(id);` line.)

- [ ] **Step 5: Build gated + verify the new tests PASS**

Run: `cd <repo-root> && cmake --build --preset dev --target x3d_physics_system_test && ctest --preset dev -R x3d_physics_system --output-on-failure 2>&1 | tail -12`
Expected: PASS — including the inertia-honored, differs-from-derived, and COM-orbit assertions.

- [ ] **Step 6: Verify ZERO golden drift + full gated suite (the readback gate)**

Run: `cd <repo-root> && cmake --build --preset dev && ctest --preset dev -j "$(nproc)" --output-on-failure 2>&1 | tail -6`
Expected: ALL pass — critically `x3d_cli_test` (sim-physics + sim-collision goldens) must still match byte-for-byte, proving the `GetPosition` switch is a no-op for centered bodies.

- [ ] **Step 7: Verify seam purity unchanged**

Run: `grep -rE 'JPH|Jolt' runtime/physics/PhysicsBackend.hpp runtime/physics/PhysicsSystem.hpp runtime/physics/ContactReporter.hpp | grep -vE '//|\*'; echo "exit $?"`
Expected: no engine-type matches.

- [ ] **Step 8: Commit**

```bash
git add runtime/physics/jolt/JoltBackend.cpp runtime/physics/tests/physics_system_test.cpp
git commit -m "feat(physics): Jolt honors authored inertia tensor + centerOfMass; origin readback (CONF-RBP-INERTIA)"
```

---

### Task 4: Quadcopter fixture + CLI golden (gated)

**Files:**
- Create: `tools/x3d-cli/fixtures/sim-quadcopter.x3d`, `tools/x3d-cli/goldens/sim-quadcopter.trace.json`
- Modify: `tools/tests/x3d_cli_test.sh`

**Interfaces:**
- Consumes: the full inertia/COM path (Tasks 1-3). `x3d sim` links the Jolt backend and enrols `RigidBodyCollection` via `sim_runtime.attachPhysics`.

- [ ] **Step 1: Create the fixture**

`tools/x3d-cli/fixtures/sim-quadcopter.x3d`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE X3D PUBLIC "ISO//Web3D//DTD X3D 4.0//EN" "https://www.web3d.org/specifications/x3d-4.0.dtd">
<X3D profile="Full" version="4.0">
  <head>
    <meta name="description" content="sim quadcopter: an open-loop flight — constant lift (rises) + constant roll torque, with an authored non-uniform inertia tensor governing the roll rate (§37 RigidBody.inertia)."/>
  </head>
  <Scene>
    <!-- The renderable frame the body's pose drives. -->
    <Transform DEF="QuadXform">
      <Shape><Box size="1 0.2 1"/></Shape>
    </Transform>

    <RigidBodyCollection DEF="World" gravity="0 -9.8 0">
      <RigidBody DEF="Quad" containerField="bodies" mass="1"
                 position="0 1 0"
                 inertia="0.01 0 0  0 0.02 0  0 0 0.01"
                 forces="0 10 0"
                 torques="0.02 0 0">
        <CollidableShape containerField="geometry">
          <Shape containerField="shape"><Box size="1 0.2 1"/></Shape>
        </CollidableShape>
      </RigidBody>
    </RigidBodyCollection>

    <ROUTE fromNode="Quad" fromField="position" toNode="QuadXform" toField="translation"/>
    <ROUTE fromNode="Quad" fromField="orientation" toNode="QuadXform" toField="rotation"/>
  </Scene>
</X3D>
```

- [ ] **Step 2: Add the non-golden harness assertion**

In `tools/tests/x3d_cli_test.sh`, inside the existing `if [[ "$PHYSICS" == "1" ... ]]` physics block (after the sim-collision assertions, before its closing `fi`), add:

```bash
    # §37 inertia: the quadcopter rises (lift > mg) and rolls (torque about x).
    FIXTURE_QUAD="$FIXTURES/sim-quadcopter.x3d"
    if [[ -f "$FIXTURE_QUAD" ]]; then
        quad=$("$CLI" sim "$FIXTURE_QUAD" --fps 60 --ticks 120 --watch QuadXform.translation 2>/dev/null || true)
        if [[ "$quad" == *"QuadXform.translation = 0 1."* || "$quad" == *"QuadXform.translation = 0 2."* ]]; then
            echo "ok:   sim quadcopter rises under lift"
        else
            echo "FAIL: sim quadcopter did not rise (got: $quad)"
            failures=$(( failures + 1 ))
        fi
        # Determinism: two --json runs byte-identical.
        qr1="$TD/quad_r1.json"; qr2="$TD/quad_r2.json"
        "$CLI" sim "$FIXTURE_QUAD" --fps 60 --ticks 120 --watch QuadXform.translation --watch QuadXform.rotation --json >"$qr1" 2>/dev/null || true
        "$CLI" sim "$FIXTURE_QUAD" --fps 60 --ticks 120 --watch QuadXform.translation --watch QuadXform.rotation --json >"$qr2" 2>/dev/null || true
        if cmp -s "$qr1" "$qr2"; then
            echo "ok:   sim quadcopter is deterministic (two runs byte-identical)"
        else
            echo "FAIL: sim quadcopter is non-deterministic"
            failures=$(( failures + 1 ))
        fi
    fi
```

- [ ] **Step 3: Build the CLI (gated) + run the harness, confirm the new lines PASS**

Run: `cd <repo-root> && cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=ON && cmake --build --preset dev --target x3d_cli && ctest --preset dev -R x3d_cli_test --output-on-failure 2>&1 | tail -25`
Expected: PASS — including "sim quadcopter rises" and "deterministic".

- [ ] **Step 4: Capture the golden (only after Step 3 passes)**

Confirm the binary path: `find build -name x3d -path '*x3d-cli*' -type f` (expected `./build/x3d`). Then:

Run: `cd <repo-root> && ./build/x3d sim tools/x3d-cli/fixtures/sim-quadcopter.x3d --fps 60 --ticks 120 --watch QuadXform.translation --watch QuadXform.rotation --json > tools/x3d-cli/goldens/sim-quadcopter.trace.json`

SANITY-CHECK: `head -c 400 tools/x3d-cli/goldens/sim-quadcopter.trace.json` — must be a JSON array whose later records show `QuadXform.translation` climbing in y AND `QuadXform.rotation` with a growing angle about the x-axis (`1 0 0 <angle>`). If empty / not rising / not rotating, STOP — do not commit a bad golden.

- [ ] **Step 5: Add the golden-regression block to the harness**

In `tools/tests/x3d_cli_test.sh`, alongside the existing sim-collision golden block (gated on `$PHYSICS == 1`), add:

```bash
    if [[ "$PHYSICS" == "1" && -f "$GOLDENS/sim-quadcopter.trace.json" ]]; then
        got_quad=$("$CLI" sim "$FIXTURES/sim-quadcopter.x3d" --fps 60 --ticks 120 --watch QuadXform.translation --watch QuadXform.rotation --json 2>/dev/null || true)
        if [[ "$got_quad" == "$(cat "$GOLDENS/sim-quadcopter.trace.json")" ]]; then
            echo "ok:   sim quadcopter trace matches golden"
        else
            echo "FAIL: sim quadcopter trace drifted from golden"
            diff <(printf '%s' "$got_quad") "$GOLDENS/sim-quadcopter.trace.json" | head -20
            failures=$(( failures + 1 ))
        fi
    fi
```

- [ ] **Step 6: Re-run the harness + verify the golden matches**

Run: `cd <repo-root> && ctest --preset dev -R x3d_cli_test --output-on-failure 2>&1 | tail -25`
Expected: PASS — "sim quadcopter trace matches golden".

- [ ] **Step 7: Commit**

```bash
git add tools/x3d-cli/fixtures/sim-quadcopter.x3d tools/x3d-cli/goldens/sim-quadcopter.trace.json tools/tests/x3d_cli_test.sh
git commit -m "test(physics): quadcopter sim golden — authored inertia governs roll (CONF-RBP-INERTIA)"
```

---

### Task 5: Narrow CONF-RBP-INERTIA + docs + final dual-config verification

**Files:**
- Modify: `docs/conformance/findings.yaml`, `docs/conformance/components/RigidBodyPhysics.md` (regenerated), `docs/superpowers/BACKLOG.md`

**Interfaces:**
- Consumes: the merged implementation (Tasks 1-4) and its commit hashes.

- [ ] **Step 1: Narrow the `CONF-RBP-INERTIA` finding**

Edit the `- id: CONF-RBP-INERTIA` entry in `docs/conformance/findings.yaml` to record the partial closure (keep `status: deferred` — `massDensityModel`/`finiteRotation` remain), updating the summary and note:

```yaml
- id: CONF-RBP-INERTIA
  component: RigidBodyPhysics
  nodes: [RigidBody]
  clause: "37"
  severity: minor
  status: deferred
  summary: "inertia (explicit tensor) + centerOfMass are now HONORED (Jolt MassAndInertiaProvided + OffsetCenterOfMassShape; non-default detection). Remaining: massDensityModel, finiteRotationAxis/useFiniteRotation."
  note: "inertia/centerOfMass shipped <merge-commit>: non-identity inertia → authored 3x3 tensor; non-zero centerOfMass → COM-offset shape wrap; pose readback uses the body origin (GetPosition) so RigidBody.position tracks the authored reference point. Verified by the quadcopter sim (authored inertia governs roll, differs from shape-derived) + a COM-orbit test. Still derived/ignored: massDensityModel (alt mass shape), finiteRotation (integrator option)."
```

- [ ] **Step 2: Regenerate the conformance view + gate**

Run: `cd <repo-root> && mise run conformance && mise run conformance-gate`
Expected: regenerates `model.json` + `INDEX.md` + `components/RigidBodyPhysics.md`; gate passes.

- [ ] **Step 3: Update BACKLOG.md**

In `docs/superpowers/BACKLOG.md`, update the `CONF-RBP-INERTIA` row to note inertia + centerOfMass are shipped (cite the merge commit), with the remainder (massDensityModel, finiteRotation) still DEFERRED — mirroring the existing partial/closed-row style in that file.

- [ ] **Step 4: Final full verification — BOTH configurations**

Ungated (physics OFF — the default distribution):
```
cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=OFF && cmake --build --preset dev && ctest --preset dev -j "$(nproc)" --output-on-failure 2>&1 | tail -4
```
Expected: ALL pass (incl. `x3d_inertia_massprops`). The gated physics tests are not built here.

Gated (physics ON):
```
cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=ON && cmake --build --preset dev && ctest --preset dev -j "$(nproc)" --output-on-failure 2>&1 | tail -4
```
Expected: ALL pass (incl. `x3d_physics_system`, `x3d_inertia_massprops`, `x3d_cli_test` with the quad golden).

Capture both tallies in the report. If EITHER config has a failure, STOP and report BLOCKED — do not commit docs claiming closure over a red build. Leave `build/` configured with physics ON when done.

- [ ] **Step 5: Commit**

```bash
git add docs/conformance docs/superpowers/BACKLOG.md
git commit -m "docs(physics): inertia + centerOfMass shipped — narrow CONF-RBP-INERTIA"
```

> NOTE: update the auto-memory `memory/x3d-cpp-gen-physics-seam.md` after merge (move inertia/COM from "remaining" to shipped) — a memory write, not a branch commit.

---

## Self-Review

**Spec coverage:**
- §3a `MassProperties` seam + signature → Task 1. ✓
- §3b non-default detection (`isIdentity3x3`, COM passthrough) → Task 2. ✓
- §3c Jolt mapping (MassAndInertiaProvided + Mat44, OffsetCenterOfMassShape, GetPosition readback) → Task 3. ✓
- §4 quadcopter fixture + CLI golden → Task 4. ✓
- §5 testing: ungated defaults+detection (Tasks 1-2), gated inertia-honored/differs/COM-orbit/determinism (Task 3), CLI golden (Task 4), zero-golden-drift gate (Task 3 Step 6 + Task 5), seam purity (Tasks 2-3). ✓
- §6 files ↔ plan File Structure. ✓
- §7 guardrails (origin-centered COM, full 3×3, dynamic-only) honored in Task 3 code + comments. ✓
- §8 deferred (massDensityModel, finiteRotation) → Task 5 narrows, not closes. ✓

**Placeholder scan:** No TBD/TODO; every code step shows full code. The only `<merge-commit>` placeholder (Task 5 Step 1) is an intentional post-merge hash fill, flagged as such. ✓

**Type consistency:** `MassProperties{mass, overrideInertia, inertia, centerOfMass}` identical across Tasks 1-3; `isIdentity3x3(const SFMatrix3f&)` defined Task 2, used Task 2; `addBody(... const MassProperties&, bool fixed, ...)` signature identical in PhysicsBackend.hpp / JoltBackend.{hpp,cpp} / both fake backends; `SFMatrix3f.matrix[r][c]`, `SFVec3f.{x,y,z}`, `getOrientation().angle`, `getPosition()` match the bindings; Jolt `SetColumn3`/`SetColumn4`/`MassAndInertiaProvided`/`OffsetCenterOfMassShapeSettings`/`GetPosition` verified present in v5.5. ✓
