# §37 Vehicle Sim + Per-Contact Friction & Bounce — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Author `CollisionCollection.frictionCoefficients` + `bounce` as per-contact combined friction/restitution, and demonstrate it with a rear-wheel-drive vehicle that drives under tyre friction (plus a bounce test).

**Architecture:** A new `setContactResponse(world, friction, restitution)` seam method routes authored values to the existing per-world Jolt `ContactListener` (`ContactCollector`), which sets `ioSettings.mCombinedFriction`/`mCombinedRestitution` per contact. `PhysicsSystem` reads each `RigidBodyCollection`'s `collider → CollisionCollection` and applies non-default values. The vehicle = chassis + 4 cylinder wheels on free `SingleAxisHingeJoint` axles; rear wheels driven by `torques`.

**Tech Stack:** C++20, header-only runtime core, Jolt Physics v5.5.0 (gated), CMake/CTest, generated X3D node bindings.

## Global Constraints

- **Seam purity:** `runtime/physics/PhysicsBackend.hpp`, `PhysicsSystem.hpp`, `ContactReporter.hpp` name no Jolt/JPH *type* (doc-comment word mentions OK). Engine coupling only in `runtime/physics/jolt/JoltBackend.{hpp,cpp}`. Verify: `grep -rE 'JPH|Jolt' <those three> | grep -vE '//|\*'` → empty.
- **Non-default detection (verbatim):** `friction = (frictionCoefficients.x != 0 || frictionCoefficients.y != 0) ? frictionCoefficients.x : -1.0f`; `restitution = (bounce != 0) ? bounce : -1.0f`. A negative value means "don't override" — the backend leaves Jolt's default combine. Scalar friction uses `.x` (anisotropic deferred).
- **Golden drift:** only `sim-collision.trace.json` may change (its `CollisionCollection` authors `bounce="0.2"`, now applied → the box bounces); it is **regenerated + verified** (box visibly bounces). Every other golden stays byte-identical.
- **Physics gated:** Jolt backend + gated tests build under `-DX3D_CPP_BUILD_PHYSICS=ON`. The seam method + PhysicsSystem wiring are header-only and build with the flag OFF (the default distribution).
- **Build/test:** ungated `cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=OFF && cmake --build --preset dev && ctest --preset dev -j "$(nproc)"`; gated `… -DX3D_CPP_BUILD_PHYSICS=ON …`. Single target: `cmake --build --preset dev --target <name>`. Respect the preset's compile-job cap (no high build `-j`; `ctest -j nproc` is fine). Cache is sticky on the last `X3D_CPP_BUILD_PHYSICS`.
- **TDD:** red → green → commit per step. DRY, YAGNI.
- **Verified facts (bindings + Jolt v5.5):**
  - `RigidBodyCollection`: `getCollider()`→SFNode, `getGravity()`, `setBodies/setJoints(MFNode)`, `setColliderUnchecked(SFNode)`, `setGravity(SFVec3f)`.
  - `CollisionCollection`: `getFrictionCoefficients()`→SFVec2f, `getBounce()`→SFFloat; setters `setFrictionCoefficients`, `setBounce`.
  - `SingleAxisHingeJoint`: `setAxis(SFVec3f)`, `setAnchorPoint(SFVec3f)`; `setBody1/setBody2(SFNode)` (from X3DRigidJointNode). Default `minAngle=-π, maxAngle=π` (free) — no need to set.
  - `RigidBody`: `setMass`, `setPosition`, `setOrientation(SFRotation)`, `setFixed(bool)`, `setGeometry(MFNode)`, `setTorques(MFVec3f)`, `getPosition()`.
  - `Cylinder`: `setRadiusUnchecked`, `setHeightUnchecked`. `Box`: `setSizeUnchecked`. Node-child: `Shape::setGeometry(static_pointer_cast<X3DNode>)`, `CollidableShape::setShapeUnchecked(...)`. `X3DNode` is global.
  - Jolt: `ContactSettings.mCombinedFriction`/`mCombinedRestitution` (set in `OnContactAdded`/`OnContactPersisted`); `BodyInterface::SetFriction` not used (we go per-contact). `setGravityFactor` is the seam pattern to mirror (world lookup → store).
  - Wheel orientation: Cylinder axis = local Y; lay it onto the world-X axle with `SFRotation{0,0,1,-1.5707963f}`.
  - **Spike-validated vehicle geometry:** ground Box `50×1×50` at y=-0.5; chassis Box `1×0.4×2` mass 5 at y=0.5; wheels Cylinder r=0.4 h=0.2 mass 1 at (±0.6, 0.4, ±0.8); hinge axis (1,0,0) anchored at each wheel; rear wheels (z=-0.8) `torques 3 0 0`; `frictionCoefficients 1 1` → drives ~5 m in 3 s, straight, upright.

## File Structure

- `runtime/physics/PhysicsBackend.hpp` — **modify**: `setContactResponse` declaration.
- `runtime/physics/jolt/JoltBackend.hpp` — **modify**: `setContactResponse` override decl.
- `runtime/physics/jolt/JoltBackend.cpp` — **modify**: `ContactCollector` friction/restitution members + apply in callbacks; `setContactResponse` impl.
- `runtime/physics/PhysicsSystem.hpp` — **modify**: read collider, call `setContactResponse`.
- `runtime/physics/tests/jolt_backend_test.cpp` — **modify**: gated bounce test.
- `runtime/physics/tests/contact_response_test.cpp` — **create**: ungated wiring test.
- `runtime/physics/tests/physics_system_test.cpp` — **modify**: gated vehicle drive + friction-matters + determinism.
- `tools/x3d-cli/fixtures/sim-vehicle.x3d`, `tools/x3d-cli/goldens/sim-vehicle.trace.json` — **create**.
- `tools/x3d-cli/goldens/sim-collision.trace.json` — **regenerate**.
- `tools/tests/x3d_cli_test.sh` — **modify**: vehicle golden assertion.
- `CMakeLists.txt` — **modify**: register `x3d_contact_response`.
- `docs/conformance/findings.yaml`, `docs/superpowers/BACKLOG.md` — **modify**.

---

### Task 1: `setContactResponse` seam + Jolt per-contact friction/restitution

**Files:**
- Modify: `runtime/physics/PhysicsBackend.hpp`, `runtime/physics/jolt/JoltBackend.hpp`, `runtime/physics/jolt/JoltBackend.cpp`
- Test: `runtime/physics/tests/jolt_backend_test.cpp` (gated bounce test)

**Interfaces:**
- Produces: `virtual void PhysicsBackend::setContactResponse(WorldHandle, float friction, float restitution)` (default no-op; negative = don't override).

- [ ] **Step 1: Write the failing gated bounce test**

In `runtime/physics/tests/jolt_backend_test.cpp`, add a block to `main()` before the final `ALL PASS` print:

```cpp
  // (6) Per-contact restitution: a sphere dropped onto the ground rebounds clearly
  //     higher with bounce 0.8 than with restitution left at Jolt's default (0).
  {
    auto reboundApex = [](float restitution) {
      JoltBackend backend;
      WorldHandle world = backend.createWorld(SFVec3f{0, -9.8f, 0});
      backend.setContactResponse(world, /*friction*/ -1.0f, restitution);
      backend.addBody(world, ShapeDesc::box(SFVec3f{10, 0.1f, 10}),
                      MassProperties{0.0f}, true, SFVec3f{0, 0, 0}, kNoRot, kZero,
                      kZero);
      BodyHandle ball = backend.addBody(world, ShapeDesc::sphere(0.5f),
                                        MassProperties{1.0f}, false,
                                        SFVec3f{0, 3, 0}, kNoRot, kZero, kZero);
      float apex = 0.0f;
      bool landed = false;
      for (int i = 0; i < 180; ++i) {
        backend.step(world, 1.0 / 60.0);
        SFVec3f p{0, 0, 0};
        SFRotation o = kNoRot;
        backend.getBodyTransform(world, ball, p, o);
        if (p.y < 0.8f) landed = true;           // reached the ground
        if (landed) apex = std::max(apex, p.y);   // highest rebound after landing
      }
      return apex;
    };
    float bouncy = reboundApex(0.8f);
    float dead = reboundApex(-1.0f);  // unset -> Jolt default restitution 0
    std::fprintf(stderr, "bounce apex: restitution0.8=%.3f vs default=%.3f\n",
                 bouncy, dead);
    CHECK(bouncy > 1.0f, "restitution 0.8 sphere rebounds high (>1)");
    CHECK(dead < 0.75f, "default-restitution sphere does not bounce (settles)");
    CHECK(bouncy > dead + 0.4f, "authored restitution clearly increases rebound");
  }
```

- [ ] **Step 2: Build gated + verify it fails**

Run: `cd <repo-root> && cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=ON && cmake --build --preset dev --target x3d_physics_jolt_test 2>&1 | tail -20`
Expected: FAIL to compile — `backend.setContactResponse` does not exist.

- [ ] **Step 3: Declare `setContactResponse` on the seam**

In `runtime/physics/PhysicsBackend.hpp`, after the `setGravityFactor` declaration, add:

```cpp
  /**
   * @brief Set a world's per-contact combined friction + restitution.
   * @details Maps §37 CollisionCollection.frictionCoefficients / bounce, applied
   *          to every contact in the world (one CollisionCollection governs one
   *          RigidBodyCollection = one world). A NEGATIVE value means "do not
   *          override" — the backend keeps its own default combine for that
   *          parameter. Default: no-op (backends without contacts ignore it).
   */
  virtual void setContactResponse(WorldHandle world, float friction,
                                  float restitution) {
    (void)world;
    (void)friction;
    (void)restitution;
  }
```

- [ ] **Step 4: Declare the override in `JoltBackend.hpp`**

After the `setGravityFactor` override declaration:

```cpp
  void setContactResponse(WorldHandle world, float friction,
                          float restitution) override;
```

- [ ] **Step 5: Add friction/restitution to `ContactCollector` + apply in callbacks**

In `runtime/physics/jolt/JoltBackend.cpp`, in the `ContactCollector` class, add the two members (near the `bodies` pointer):

```cpp
  // Per-contact response override (negative = unset, keep Jolt's default combine).
  // Set by setContactResponse from §37 CollisionCollection.frictionCoefficients/bounce.
  float combinedFriction = -1.0f;
  float combinedRestitution = -1.0f;
```

Update both callbacks to apply them (replace the existing `OnContactAdded`/`OnContactPersisted`):

```cpp
  void OnContactAdded(const Body &b1, const Body &b2,
                      const ContactManifold &manifold,
                      ContactSettings &ioSettings) override {
    applyResponse(ioSettings);
    record(b1, b2, manifold);
  }
  void OnContactPersisted(const Body &b1, const Body &b2,
                          const ContactManifold &manifold,
                          ContactSettings &ioSettings) override {
    applyResponse(ioSettings);
    record(b1, b2, manifold);
  }
```

Add a private helper (next to `record`):

```cpp
  void applyResponse(ContactSettings &ioSettings) const {
    if (combinedFriction >= 0.0f) ioSettings.mCombinedFriction = combinedFriction;
    if (combinedRestitution >= 0.0f)
      ioSettings.mCombinedRestitution = combinedRestitution;
  }
```

- [ ] **Step 6: Implement `setContactResponse` (mirror `setGravityFactor`)**

In `runtime/physics/jolt/JoltBackend.cpp`, near `setGravityFactor`:

```cpp
void JoltBackend::setContactResponse(WorldHandle world, float friction,
                                     float restitution) {
  auto it = impl_->worlds.find(world);
  if (it == impl_->worlds.end() || !it->second.contacts) return;
  it->second.contacts->combinedFriction = friction;
  it->second.contacts->combinedRestitution = restitution;
}
```

- [ ] **Step 7: Build gated + verify the bounce test passes**

Run: `cd <repo-root> && cmake --build --preset dev --target x3d_physics_jolt_test && ctest --preset dev -R x3d_physics_jolt --output-on-failure 2>&1 | tail -8`
Expected: PASS (the `bounce apex` line shows restitution0.8 ≫ default).

- [ ] **Step 8: Verify seam purity + commit**

Run: `grep -rE 'JPH|Jolt' runtime/physics/PhysicsBackend.hpp | grep -vE '//|\*'; echo "exit $?"` → no engine-type matches.

```bash
git add runtime/physics/PhysicsBackend.hpp runtime/physics/jolt/JoltBackend.hpp runtime/physics/jolt/JoltBackend.cpp runtime/physics/tests/jolt_backend_test.cpp
git commit -m "feat(physics): per-contact friction + restitution seam (setContactResponse)"
```

---

### Task 2: PhysicsSystem reads collider friction + bounce (ungated)

**Files:**
- Modify: `runtime/physics/PhysicsSystem.hpp`
- Create: `runtime/physics/tests/contact_response_test.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `setContactResponse` (Task 1).
- Produces: `PhysicsSystem::attach` calls `backend_->setContactResponse(world, friction, restitution)` derived from the collection's collider.

- [ ] **Step 1: Write the failing ungated wiring test**

Create `runtime/physics/tests/contact_response_test.cpp`:

```cpp
// contact_response_test.cpp — ungated test that PhysicsSystem reads a
// RigidBodyCollection's collider -> CollisionCollection.frictionCoefficients/bounce
// and forwards them to setContactResponse with non-default detection. No engine.
#include "PhysicsSystem.hpp"

#include "X3DExecutionContext.hpp"

#include "Box.hpp"
#include "CollidableShape.hpp"
#include "CollisionCollection.hpp"
#include "RigidBody.hpp"
#include "RigidBodyCollection.hpp"
#include "Shape.hpp"

#include <cstdio>
#include <memory>

using namespace x3d::runtime;

static int g_failures = 0;
#define CHECK(cond, msg)                                                        \
  do {                                                                          \
    if (!(cond)) { std::printf("FAIL: %s\n", msg); ++g_failures; }             \
    else { std::printf("ok:   %s\n", msg); }                                   \
  } while (0)

// Records the last setContactResponse args.
class RecordingBackend : public PhysicsBackend {
public:
  bool called = false;
  float friction = -99.0f, restitution = -99.0f;
  WorldHandle createWorld(const SFVec3f &) override { return 1; }
  BodyHandle addBody(WorldHandle, const ShapeDesc &, const MassProperties &, bool,
                     const SFVec3f &, const SFRotation &, const SFVec3f &,
                     const SFVec3f &) override { return ++h_; }
  ConstraintHandle addConstraint(WorldHandle, const ConstraintDesc &) override { return 1; }
  void applyForce(WorldHandle, BodyHandle, const SFVec3f &, const SFVec3f &) override {}
  void setGravityFactor(WorldHandle, BodyHandle, float) override {}
  void getBodyVelocity(WorldHandle, BodyHandle, SFVec3f &l, SFVec3f &a) const override { l = {0,0,0}; a = {0,0,0}; }
  void step(WorldHandle, double) override {}
  void getBodyTransform(WorldHandle, BodyHandle, SFVec3f &p, SFRotation &o) const override { p = {0,0,0}; o = {0,0,1,0}; }
  void setContactResponse(WorldHandle, float f, float r) override {
    called = true; friction = f; restitution = r;
  }
private:
  BodyHandle h_ = 0;
};

// One-body collection whose collider is a CollisionCollection with the given
// frictionCoefficients + bounce.
static std::shared_ptr<RigidBodyCollection>
makeColl(SFVec2f fc, float bounce) {
  auto box = std::make_shared<Box>(); box->setSizeUnchecked(SFVec3f{1, 1, 1});
  auto shp = std::make_shared<Shape>();
  shp->setGeometry(std::static_pointer_cast<X3DNode>(box));
  auto coll = std::make_shared<CollidableShape>();
  coll->setShapeUnchecked(std::static_pointer_cast<X3DNode>(shp));
  auto body = std::make_shared<RigidBody>();
  body->setGeometry(MFNode{std::static_pointer_cast<X3DNode>(coll)});
  auto cc = std::make_shared<CollisionCollection>();
  cc->setFrictionCoefficients(fc);
  cc->setBounce(bounce);
  auto rbc = std::make_shared<RigidBodyCollection>();
  rbc->setBodies(MFNode{std::static_pointer_cast<X3DNode>(body)});
  rbc->setColliderUnchecked(std::static_pointer_cast<X3DNode>(cc));
  return rbc;
}

static RecordingBackend *attachAndGet(std::shared_ptr<RigidBodyCollection> rbc) {
  auto backend = std::make_shared<RecordingBackend>();
  RecordingBackend *raw = backend.get();
  PhysicsSystem sys(backend);
  X3DExecutionContext ctx;
  sys.attach(rbc.get(), ctx);
  return raw;  // backend kept alive by sys; raw valid for the assertions below
}

int main() {
  {
    auto *b = attachAndGet(makeColl(SFVec2f{1.0f, 1.0f}, 0.0f));
    CHECK(b->called, "setContactResponse called when friction authored");
    CHECK(b->friction == 1.0f, "authored friction -> frictionCoefficients.x");
    CHECK(b->restitution < 0.0f, "default bounce -> restitution unset (-1)");
  }
  {
    auto *b = attachAndGet(makeColl(SFVec2f{0.0f, 0.0f}, 0.8f));
    CHECK(b->friction < 0.0f, "default friction -> unset (-1)");
    CHECK(b->restitution == 0.8f, "authored bounce -> restitution");
  }
  {
    auto *b = attachAndGet(makeColl(SFVec2f{0.0f, 0.0f}, 0.0f));
    CHECK(b->friction < 0.0f && b->restitution < 0.0f,
          "all-default collider -> both unset (-1)");
  }
  return g_failures == 0 ? 0 : 1;
}
```

> NOTE: `attachAndGet` returns a raw pointer whose owner (`backend` shared_ptr) is held by the local `PhysicsSystem sys` — but `sys` is destroyed when `attachAndGet` returns. Inline the body into `main()` per-case if the pointer dangles; the brief's three cases are independent, so write each as its own `{ auto backend = std::make_shared<RecordingBackend>(); PhysicsSystem sys(backend); X3DExecutionContext ctx; sys.attach(makeColl(...).get(), ctx); CHECK(... backend->...); }` block. (Prefer this inlined form to avoid the lifetime footgun.)

- [ ] **Step 2: Rewrite the test cases inlined (avoid the lifetime footgun)**

Replace `main()` + `attachAndGet` with three self-contained blocks, e.g.:

```cpp
int main() {
  {
    auto backend = std::make_shared<RecordingBackend>();
    PhysicsSystem sys(backend);
    X3DExecutionContext ctx;
    auto rbc = makeColl(SFVec2f{1.0f, 1.0f}, 0.0f);
    sys.attach(rbc.get(), ctx);
    CHECK(backend->called, "setContactResponse called when friction authored");
    CHECK(backend->friction == 1.0f, "authored friction -> frictionCoefficients.x");
    CHECK(backend->restitution < 0.0f, "default bounce -> restitution unset (-1)");
  }
  {
    auto backend = std::make_shared<RecordingBackend>();
    PhysicsSystem sys(backend);
    X3DExecutionContext ctx;
    auto rbc = makeColl(SFVec2f{0.0f, 0.0f}, 0.8f);
    sys.attach(rbc.get(), ctx);
    CHECK(backend->friction < 0.0f, "default friction -> unset (-1)");
    CHECK(backend->restitution == 0.8f, "authored bounce -> restitution");
  }
  {
    auto backend = std::make_shared<RecordingBackend>();
    PhysicsSystem sys(backend);
    X3DExecutionContext ctx;
    auto rbc = makeColl(SFVec2f{0.0f, 0.0f}, 0.0f);
    sys.attach(rbc.get(), ctx);
    CHECK(backend->friction < 0.0f && backend->restitution < 0.0f,
          "all-default collider -> both unset (-1)");
  }
  return g_failures == 0 ? 0 : 1;
}
```

Delete the standalone `attachAndGet` helper.

- [ ] **Step 3: Register the test target in CMake**

In `CMakeLists.txt`, by the other ungated runtime physics tests (e.g. after `x3d_inertia_massprops`):

```cmake
    add_executable(x3d_contact_response
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics/tests/contact_response_test.cpp")
    target_link_libraries(x3d_contact_response PRIVATE x3d_cpp::x3d_cpp)
    target_include_directories(x3d_contact_response PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics")
    add_test(NAME x3d_contact_response COMMAND x3d_contact_response)
```

- [ ] **Step 4: Build ungated + verify it fails**

Run: `cd <repo-root> && cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=OFF && cmake --build --preset dev --target x3d_contact_response && ctest --preset dev -R x3d_contact_response --output-on-failure 2>&1 | tail -8`
Expected: FAIL — `PhysicsSystem::attach` doesn't call `setContactResponse` yet (`called` is false; friction/restitution unset).

- [ ] **Step 5: Wire it in `PhysicsSystem::attach`**

In `runtime/physics/PhysicsSystem.hpp`, immediately after `attachJoints(*collection, world, handleFor);` (the tail of `attach`), add:

```cpp
    // §37 contact-response params: read the collection's collider
    // (CollisionCollection) and apply its friction/bounce per-contact. Non-zero =
    // authored (non-default detection); negative = leave the engine default.
    float contactFriction = -1.0f, contactRestitution = -1.0f;
    if (auto *cc =
            dynamic_cast<CollisionCollection *>(collection->getCollider().get())) {
      const SFVec2f fc = cc->getFrictionCoefficients();
      if (fc.x != 0.0f || fc.y != 0.0f) contactFriction = fc.x;
      const float b = cc->getBounce();
      if (b != 0.0f) contactRestitution = b;
    }
    backend_->setContactResponse(world, contactFriction, contactRestitution);
```

`CollisionCollection.hpp` is already available (via `ContactReporter.hpp`); if the build complains it isn't, add `#include "CollisionCollection.hpp"` with the other node includes.

- [ ] **Step 6: Build ungated + verify it passes**

Run: `cd <repo-root> && cmake --build --preset dev --target x3d_contact_response && ctest --preset dev -R x3d_contact_response --output-on-failure 2>&1 | tail -8`
Expected: all `ok:`; PASS.

- [ ] **Step 7: Seam purity + full ungated suite + commit**

Run: `grep -rE 'JPH|Jolt' runtime/physics/PhysicsSystem.hpp | grep -vE '//|\*'; echo "exit $?"` → no engine types.
Run: `ctest --preset dev -j "$(nproc)" --output-on-failure 2>&1 | tail -4` → all pass.

```bash
git add runtime/physics/PhysicsSystem.hpp runtime/physics/tests/contact_response_test.cpp CMakeLists.txt
git commit -m "feat(physics): PhysicsSystem applies collider friction+bounce via setContactResponse"
```

---

### Task 3: Gated vehicle drive + friction-matters + determinism

**Files:**
- Modify: `runtime/physics/tests/physics_system_test.cpp`

**Interfaces:**
- Consumes: Tasks 1-2 (the full friction path). Builds a §37 vehicle and drives it through `PhysicsSystem`.

- [ ] **Step 1: Write the failing gated vehicle test**

In `runtime/physics/tests/physics_system_test.cpp`, add includes if missing: `#include "Cylinder.hpp"`, `#include "CollisionCollection.hpp"`, `#include "SingleAxisHingeJoint.hpp"`, `#include "RigidBodyCollection.hpp"`. Add this block to `main()`:

```cpp
  // Rear-wheel-drive vehicle: chassis + 4 cylinder wheels on free hinge axles.
  // With authored friction (1,1) the rear tyres grip and drive it forward; with
  // (0,0) -> Jolt's 0.2 they slip and it barely moves. Proves friction authoring
  // end-to-end through PhysicsSystem.
  auto driveDistance = [](float frictionX) -> float {
    using x3d::runtime::X3DNode;
    auto up = [](auto p) { return std::static_pointer_cast<X3DNode>(p); };
    auto makeBox = [&](SFVec3f half) {
      auto b = std::make_shared<Box>();
      b->setSizeUnchecked(SFVec3f{half.x * 2, half.y * 2, half.z * 2});
      auto s = std::make_shared<Shape>(); s->setGeometry(up(b));
      auto c = std::make_shared<CollidableShape>(); c->setShapeUnchecked(up(s));
      return c;
    };
    auto makeCyl = [&]() {
      auto cy = std::make_shared<Cylinder>();
      cy->setRadiusUnchecked(0.4f); cy->setHeightUnchecked(0.2f);
      auto s = std::make_shared<Shape>(); s->setGeometry(up(cy));
      auto c = std::make_shared<CollidableShape>(); c->setShapeUnchecked(up(s));
      return c;
    };
    auto ground = std::make_shared<RigidBody>();
    ground->setFixed(true); ground->setPosition(SFVec3f{0, -0.5f, 0});
    ground->setGeometry(MFNode{up(makeBox(SFVec3f{25, 0.5f, 25}))});
    auto chassis = std::make_shared<RigidBody>();
    chassis->setMass(5); chassis->setPosition(SFVec3f{0, 0.5f, 0});
    chassis->setGeometry(MFNode{up(makeBox(SFVec3f{0.5f, 0.2f, 1.0f}))});
    const SFRotation wheelOri{0, 0, 1, -1.5707963f};
    const float wx = 0.6f, wy = 0.4f, wz = 0.8f;
    SFVec3f ctr[4] = {{-wx,wy,wz},{wx,wy,wz},{-wx,wy,-wz},{wx,wy,-wz}};
    MFNode bodies{up(ground), up(chassis)};
    MFNode joints;
    for (int i = 0; i < 4; ++i) {
      auto wheel = std::make_shared<RigidBody>();
      wheel->setMass(1); wheel->setPosition(ctr[i]); wheel->setOrientation(wheelOri);
      wheel->setGeometry(MFNode{up(makeCyl())});
      if (i >= 2) wheel->setTorques(std::vector<SFVec3f>{SFVec3f{3, 0, 0}});  // rear
      bodies.push_back(up(wheel));
      auto hinge = std::make_shared<SingleAxisHingeJoint>();
      hinge->setBody1(up(wheel)); hinge->setBody2(up(chassis));
      hinge->setAxis(SFVec3f{1, 0, 0}); hinge->setAnchorPoint(ctr[i]);
      joints.push_back(up(hinge));
    }
    auto cc = std::make_shared<CollisionCollection>();
    cc->setFrictionCoefficients(SFVec2f{frictionX, frictionX});
    auto world = std::make_shared<RigidBodyCollection>();
    world->setGravity(SFVec3f{0, -9.8f, 0});
    world->setBodies(bodies); world->setJoints(joints);
    world->setColliderUnchecked(up(cc));

    X3DExecutionContext ctx;
    auto phys = std::make_shared<PhysicsSystem>(std::make_shared<JoltBackend>());
    phys->attach(world.get(), ctx);
    ctx.addSystem(phys);
    double t = 0.0; ctx.tick(t);
    for (int i = 0; i < 30; ++i) { t += 1.0 / 60.0; ctx.tick(t); }  // settle 0.5s
    const SFVec3f settle = chassis->getPosition();
    for (int i = 0; i < 180; ++i) { t += 1.0 / 60.0; ctx.tick(t); } // drive 3s
    const SFVec3f end = chassis->getPosition();
    std::fprintf(stderr,
                 "vehicle mu=%.1f: dz=%.3f dx=%.3f dy=%.3f\n", frictionX,
                 end.z - settle.z, end.x - settle.x, end.y - settle.y);
    g_lastDx = end.x - settle.x; g_lastDy = end.y - settle.y;
    return end.z - settle.z;
  };
  static float g_lastDx, g_lastDy;  // (declare above the lambda if your compiler needs it)
  float gripDz = driveDistance(1.0f);
  float lastDxGrip = g_lastDx, lastDyGrip = g_lastDy;
  float slipDz = driveDistance(0.0f);  // (0,0) -> Jolt default 0.2
  CHECK(gripDz > 2.0f, "high-friction vehicle drives forward (>2 m)");
  CHECK(std::fabs(lastDxGrip) < 0.3f, "vehicle tracks straight (small dx)");
  CHECK(std::fabs(lastDyGrip) < 0.2f, "vehicle stays upright (y ~ constant)");
  CHECK(gripDz > slipDz + 1.0f, "authored friction drives farther than mu=0.2");
  // Determinism: a second high-friction run matches the first.
  float gripDz2 = driveDistance(1.0f);
  CHECK(std::fabs(gripDz - gripDz2) < 1e-3f, "vehicle drive is deterministic");
```

> NOTE: the `g_lastDx/g_lastDy` captured-out pattern is ugly; prefer returning a small struct `{dz,dx,dy}` from the lambda. Implement whichever compiles cleanly — the assertions (gripDz>2, |dx|<0.3, |dy|<0.2, grip>slip+1, deterministic) are the contract.

- [ ] **Step 2: Build gated + verify it fails, then passes**

Run: `cd <repo-root> && cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=ON && cmake --build --preset dev --target x3d_physics_system_test && ctest --preset dev -R x3d_physics_system --output-on-failure 2>&1 | tail -15`
Expected: with Tasks 1-2 done, this should PASS (the friction path is implemented). If it FAILS on `gripDz>2` or upright/straight, the wheel orientation/torque/geometry needs the spike values exactly (radius 0.4, height 0.2, wheelOri `0 0 1 -1.5708`, rear torque `3 0 0`) — re-check against the spike-validated geometry in Global Constraints. Do NOT weaken the assertions to pass; fix the scene.

- [ ] **Step 3: Commit**

```bash
git add runtime/physics/tests/physics_system_test.cpp
git commit -m "test(physics): gated RWD vehicle drive + friction-matters + determinism"
```

---

### Task 4: CLI vehicle fixture + golden; regenerate sim-collision golden

**Files:**
- Create: `tools/x3d-cli/fixtures/sim-vehicle.x3d`, `tools/x3d-cli/goldens/sim-vehicle.trace.json`
- Regenerate: `tools/x3d-cli/goldens/sim-collision.trace.json`
- Modify: `tools/tests/x3d_cli_test.sh`

**Interfaces:**
- Consumes: the full path (Tasks 1-3). `x3d sim` enrols the RigidBodyCollection via `sim_runtime.attachPhysics`.

- [ ] **Step 1: Create the fixture**

`tools/x3d-cli/fixtures/sim-vehicle.x3d`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE X3D PUBLIC "ISO//Web3D//DTD X3D 4.0//EN" "https://www.web3d.org/specifications/x3d-4.0.dtd">
<X3D profile="Full" version="4.0">
  <head>
    <meta name="description" content="sim vehicle: a rear-wheel-drive car (chassis + 4 cylinder wheels on free hinge axles) drives forward under wheel torque + authored tyre friction (§37 CollisionCollection.frictionCoefficients)."/>
  </head>
  <Scene>
    <Transform DEF="CarXform">
      <Shape><Box size="1 0.4 2"/></Shape>
    </Transform>

    <RigidBodyCollection DEF="World" gravity="0 -9.8 0">
      <CollisionCollection containerField="collider" frictionCoefficients="1 1">
        <CollidableShape USE="GroundColl" containerField="collidables"/>
      </CollisionCollection>

      <RigidBody DEF="Ground" containerField="bodies" fixed="true" position="0 -0.5 0">
        <CollidableShape DEF="GroundColl" containerField="geometry">
          <Shape containerField="shape"><Box size="50 1 50"/></Shape>
        </CollidableShape>
      </RigidBody>

      <RigidBody DEF="Chassis" containerField="bodies" mass="5" position="0 0.5 0">
        <CollidableShape containerField="geometry">
          <Shape containerField="shape"><Box size="1 0.4 2"/></Shape>
        </CollidableShape>
      </RigidBody>

      <RigidBody DEF="WheelFL" containerField="bodies" mass="1" position="-0.6 0.4 0.8" orientation="0 0 1 -1.5708">
        <CollidableShape containerField="geometry"><Shape containerField="shape"><Cylinder radius="0.4" height="0.2"/></Shape></CollidableShape>
      </RigidBody>
      <RigidBody DEF="WheelFR" containerField="bodies" mass="1" position="0.6 0.4 0.8" orientation="0 0 1 -1.5708">
        <CollidableShape containerField="geometry"><Shape containerField="shape"><Cylinder radius="0.4" height="0.2"/></Shape></CollidableShape>
      </RigidBody>
      <RigidBody DEF="WheelRL" containerField="bodies" mass="1" position="-0.6 0.4 -0.8" orientation="0 0 1 -1.5708" torques="3 0 0">
        <CollidableShape containerField="geometry"><Shape containerField="shape"><Cylinder radius="0.4" height="0.2"/></Shape></CollidableShape>
      </RigidBody>
      <RigidBody DEF="WheelRR" containerField="bodies" mass="1" position="0.6 0.4 -0.8" orientation="0 0 1 -1.5708" torques="3 0 0">
        <CollidableShape containerField="geometry"><Shape containerField="shape"><Cylinder radius="0.4" height="0.2"/></Shape></CollidableShape>
      </RigidBody>

      <SingleAxisHingeJoint containerField="joints" axis="1 0 0" anchorPoint="-0.6 0.4 0.8"><RigidBody USE="WheelFL" containerField="body1"/><RigidBody USE="Chassis" containerField="body2"/></SingleAxisHingeJoint>
      <SingleAxisHingeJoint containerField="joints" axis="1 0 0" anchorPoint="0.6 0.4 0.8"><RigidBody USE="WheelFR" containerField="body1"/><RigidBody USE="Chassis" containerField="body2"/></SingleAxisHingeJoint>
      <SingleAxisHingeJoint containerField="joints" axis="1 0 0" anchorPoint="-0.6 0.4 -0.8"><RigidBody USE="WheelRL" containerField="body1"/><RigidBody USE="Chassis" containerField="body2"/></SingleAxisHingeJoint>
      <SingleAxisHingeJoint containerField="joints" axis="1 0 0" anchorPoint="0.6 0.4 -0.8"><RigidBody USE="WheelRR" containerField="body1"/><RigidBody USE="Chassis" containerField="body2"/></SingleAxisHingeJoint>
    </RigidBodyCollection>

    <ROUTE fromNode="Chassis" fromField="position" toNode="CarXform" toField="translation"/>
    <ROUTE fromNode="Chassis" fromField="orientation" toNode="CarXform" toField="rotation"/>
  </Scene>
</X3D>
```

> NOTE: verify the parser accepts `<RigidBody USE="…" containerField="body1"/>` for the joint's `body1`/`body2` SFNode fields. If USE-as-child-with-containerField isn't honored for these fields, fall back to the form the other sim fixtures use for SFNode-by-USE. Confirm by Step 3's "vehicle drives" assertion — if the joints don't bind, the car won't move.

- [ ] **Step 2: Add the non-golden harness assertion**

In `tools/tests/x3d_cli_test.sh`, inside the existing `if [[ "$PHYSICS" == "1" ... ]]` block (after the quadcopter assertions), add:

```bash
    FIXTURE_VEH="$FIXTURES/sim-vehicle.x3d"
    if [[ -f "$FIXTURE_VEH" ]]; then
        veh=$("$CLI" sim "$FIXTURE_VEH" --fps 60 --ticks 210 --watch CarXform.translation 2>/dev/null || true)
        # Drives forward in +z: expect a clearly non-zero z late in the trace.
        if [[ "$veh" == *"CarXform.translation = 0 0.5"*[1-9]* || "$veh" == *".translation = "*" "*" "[1-9]* ]]; then
            echo "ok:   sim vehicle drives forward"
        else
            echo "FAIL: sim vehicle did not drive (got tail: $(printf '%s' "$veh" | tail -2))"
            failures=$(( failures + 1 ))
        fi
        vr1="$TD/veh_r1.json"; vr2="$TD/veh_r2.json"
        "$CLI" sim "$FIXTURE_VEH" --fps 60 --ticks 210 --watch CarXform.translation --json >"$vr1" 2>/dev/null || true
        "$CLI" sim "$FIXTURE_VEH" --fps 60 --ticks 210 --watch CarXform.translation --json >"$vr2" 2>/dev/null || true
        if cmp -s "$vr1" "$vr2"; then
            echo "ok:   sim vehicle is deterministic"
        else
            echo "FAIL: sim vehicle non-deterministic"; failures=$(( failures + 1 ))
        fi
    fi
```

> NOTE: the "drives forward" glob is heuristic; the authoritative check is the golden-regression block (Step 5). If the glob is fragile, assert simply that the final `CarXform.translation` z component parsed from the JSON exceeds ~1.0 using `awk`/`jq`-free string handling consistent with the existing harness style.

- [ ] **Step 3: Build CLI (gated) + confirm the vehicle drives**

Run: `cd <repo-root> && cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=ON && cmake --build --preset dev --target x3d_cli && ./build/x3d sim tools/x3d-cli/fixtures/sim-vehicle.x3d --fps 60 --ticks 210 --watch CarXform.translation 2>/dev/null | tail -4`
Expected: the trace's `CarXform.translation` z grows over time (car drives forward). If it doesn't move, STOP and report (joint binding / fixture issue) — do not capture a golden.

- [ ] **Step 4: Capture the vehicle golden (after Step 3 looks right)**

Run: `cd <repo-root> && ./build/x3d sim tools/x3d-cli/fixtures/sim-vehicle.x3d --fps 60 --ticks 210 --watch CarXform.translation --json > tools/x3d-cli/goldens/sim-vehicle.trace.json`
Sanity-check: `tail -c 300 tools/x3d-cli/goldens/sim-vehicle.trace.json` shows the translation z well above 1.0. If empty/static, STOP.

- [ ] **Step 5: Regenerate the sim-collision golden (bounce now applies)**

The `sim-collision.x3d` `CollisionCollection` authors `bounce="0.2"`, now applied → the box rebounds → its `Sensor.isActive` trace changes. Regenerate + verify:

Run: `cd <repo-root> && ./build/x3d sim tools/x3d-cli/fixtures/sim-collision.x3d --fps 60 --ticks 90 --watch Sensor.isActive --json > /tmp/coll_new.json && diff <(cat tools/x3d-cli/goldens/sim-collision.trace.json) /tmp/coll_new.json | head -20`
Inspect the diff: it should reflect the box bouncing (isActive may toggle more than once instead of a single true). This is the intended new behavior. Then commit the regenerated golden:

Run: `cp /tmp/coll_new.json tools/x3d-cli/goldens/sim-collision.trace.json`

> If the diff is EMPTY (bounce 0.2 too small to separate the contact, so isActive doesn't change), that's fine — the golden is unchanged and no regen is needed. Either outcome is acceptable; just don't force a change.

- [ ] **Step 6: Add the vehicle golden-regression block + run the harness**

In `tools/tests/x3d_cli_test.sh`, alongside the other golden checks (gated on PHYSICS==1), add:

```bash
    if [[ "$PHYSICS" == "1" && -f "$GOLDENS/sim-vehicle.trace.json" ]]; then
        got_veh=$("$CLI" sim "$FIXTURES/sim-vehicle.x3d" --fps 60 --ticks 210 --watch CarXform.translation --json 2>/dev/null || true)
        if [[ "$got_veh" == "$(cat "$GOLDENS/sim-vehicle.trace.json")" ]]; then
            echo "ok:   sim vehicle trace matches golden"
        else
            echo "FAIL: sim vehicle trace drifted from golden"
            diff <(printf '%s' "$got_veh") "$GOLDENS/sim-vehicle.trace.json" | head -20
            failures=$(( failures + 1 ))
        fi
    fi
```

Run: `cd <repo-root> && ctest --preset dev -R x3d_cli_test --output-on-failure 2>&1 | tail -25`
Expected: PASS — vehicle drives, deterministic, matches golden; sim-collision golden matches (the regenerated one).

- [ ] **Step 7: Commit**

```bash
git add tools/x3d-cli/fixtures/sim-vehicle.x3d tools/x3d-cli/goldens/sim-vehicle.trace.json tools/x3d-cli/goldens/sim-collision.trace.json tools/tests/x3d_cli_test.sh
git commit -m "test(physics): x3d sim vehicle golden + regenerate sim-collision (bounce now applies)"
```

---

### Task 5: Conformance docs + dual-config final verification

**Files:**
- Modify: `docs/conformance/findings.yaml`, `docs/conformance/components/RigidBodyPhysics.md` (regenerated), `docs/superpowers/BACKLOG.md`

**Interfaces:**
- Consumes: the merged implementation (Tasks 1-4) + commit hashes.

- [ ] **Step 1: Record friction + bounce authoring in findings.yaml**

Add a new finding (or fold into the CONF-RBP response-param note) in `docs/conformance/findings.yaml`, mirroring the existing entry shape:

```yaml
- id: CONF-RBP-FRICTION-BOUNCE
  component: RigidBodyPhysics
  nodes: [CollisionCollection]
  clause: "37"
  severity: minor
  status: fixed
  commit: <merge-commit-short-hash>
  summary: "CollisionCollection.frictionCoefficients (scalar, .x) and bounce are applied as per-contact combined friction/restitution via the Jolt ContactListener (non-default detection). Demonstrated by a rear-wheel-drive vehicle that drives under tyre friction + a rebounding ball."
  note: "PhysicsSystem reads RigidBodyCollection.collider -> CollisionCollection and calls setContactResponse(world, friction, restitution); the per-world ContactCollector sets ioSettings.mCombinedFriction/mCombinedRestitution. Deferred: anisotropic friction (frictionCoefficients.y), slipFactors, surfaceSpeed, minBounceSpeed, softness/solver tuning (CONF-RBP-SOLVER)."
```

- [ ] **Step 2: Regenerate + gate the conformance view**

Run: `cd <repo-root> && mise run conformance && mise run conformance-gate`
Expected: regenerates the view; gate passes.

- [ ] **Step 3: Update BACKLOG.md**

Add/adjust a `CONF-RBP-FRICTION-BOUNCE` row in `docs/superpowers/BACKLOG.md` marking friction+bounce authoring **FIXED** (cite the merge commit), noting anisotropic/slip/surfaceSpeed/solver still DEFERRED — mirroring the existing row style.

- [ ] **Step 4: Final full verification — BOTH configs**

Ungated: `cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=OFF && cmake --build --preset dev && ctest --preset dev -j "$(nproc)" --output-on-failure 2>&1 | tail -4`
Expected: all pass (incl. `x3d_contact_response`).

Gated: `cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=ON && cmake --build --preset dev && ctest --preset dev -j "$(nproc)" --output-on-failure 2>&1 | tail -4`
Expected: all pass (incl. `x3d_physics_jolt` bounce, `x3d_physics_system` vehicle, `x3d_cli_test` vehicle + sim-collision goldens).

Capture both tallies. If either config fails, STOP and report BLOCKED. Leave `build/` configured with physics ON.

- [ ] **Step 5: Commit**

```bash
git add docs/conformance docs/superpowers/BACKLOG.md
git commit -m "docs(physics): friction + bounce authoring shipped (CONF-RBP-FRICTION-BOUNCE)"
```

> NOTE: update the auto-memory `memory/x3d-cpp-gen-physics-seam.md` after merge (friction+bounce authoring + the vehicle sim), as a memory write, not a branch commit.

---

## Self-Review

**Spec coverage:**
- §4a `setContactResponse` seam + ContactCollector friction/restitution → Task 1. ✓
- §4a PhysicsSystem reads collider friction+bounce (non-default detection) → Task 2. ✓
- §4b vehicle (chassis + 4 wheels + hinge axles, RWD) → Task 3 (gated C++) + Task 4 (CLI fixture/golden). ✓
- §5 testing: ungated wiring (Task 2), gated bounce (Task 1), gated vehicle drive/friction-matters/determinism (Task 3), CLI golden + sim-collision regen (Task 4), dual-config (Task 5), seam purity (Tasks 1-2). ✓
- §4a golden drift: sim-collision regenerated → Task 4 Step 5. ✓
- §6 files ↔ File Structure. ✓
- §7/§8 deferrals (anisotropic/slip/surfaceSpeed/solver/suspension/steering) noted in Task 5 docs. ✓

**Placeholder scan:** No TBD/TODO; code shown for every code step. Two NOTE blocks (the lifetime-footgun rewrite in Task 2; the joint-`USE` parser caveat in Task 4) give concrete fallbacks, not deferrals. The `<merge-commit>` in Task 5 is an intentional post-merge fill.

**Type consistency:** `setContactResponse(WorldHandle, float friction, float restitution)` identical across PhysicsBackend/JoltBackend/RecordingBackend and the PhysicsSystem call; `combinedFriction`/`combinedRestitution` (-1 sentinel) consistent; non-default detection formula identical in spec + Task 2; vehicle geometry matches the spike-validated constants; node setters match the verified bindings (`setColliderUnchecked`, `setRadiusUnchecked`/`setHeightUnchecked`, `setTorques`, `setOrientation`, `setBody1/2`, `setFrictionCoefficients`/`setBounce`).
