# §37 Contact Reporting Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Surface Jolt-detected collisions back into the X3D event graph so `CollisionSensor` emits `contacts` (`Contact` nodes), `intersections`, and `isActive` — closing `CONF-RBP`, the largest remaining RigidBodyPhysics gap.

**Architecture:** A read-only output bridge (the response solve already happens inside Jolt; §37.4.5 forbids feeding contacts back). The backend seam gains a neutral `ContactPoint` + a pull method `drainContacts`. `JoltBackend` buffers contacts in a `ContactListener` during `step()`. `PhysicsSystem` resolves each drained contact's body handles to scene nodes and hands a `std::vector<ResolvedContact>` to a focused, backend-agnostic `ContactReporter`, which builds `Contact` nodes and emits the three output fields via the existing event cascade.

**Tech Stack:** C++20, header-only runtime core, Jolt Physics v5.5.0 (gated), CMake/CTest, the generated X3D node bindings.

## Global Constraints

- **Seam purity (verbatim invariant):** `runtime/physics/PhysicsSystem.hpp`, `runtime/physics/PhysicsBackend.hpp`, and the new `runtime/physics/ContactReporter.hpp` must name **no** Jolt/JPH type. All engine coupling lives only in `runtime/physics/jolt/JoltBackend.cpp`. Verify with: `grep -rE 'JPH|Jolt' runtime/physics/PhysicsSystem.hpp runtime/physics/PhysicsBackend.hpp runtime/physics/ContactReporter.hpp` → **no matches**.
- **Determinism:** emitted contact ordering must not depend on pointer addresses or engine callback order. Sort/dedupe by a **stable enrollment ordinal**, never by node pointer.
- **Physics is gated:** the Jolt backend + its tests build only under `-DX3D_CPP_BUILD_PHYSICS=ON`. The `ContactReporter` and the `PhysicsSystem` wiring are header-only and must build/test **without** the flag.
- **Build/test commands:**
  - Core (ungated) build + ctest: `mise run build` (= `cmake --preset dev && cmake --build --preset dev && ctest --preset dev -j "$(nproc)"`). Respect the `-j` compile-job cap already baked into the preset (OOM at high -j).
  - Gated build: `cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=ON && cmake --build --preset dev`.
  - Run one ctest: `ctest --preset dev -R <name> --output-on-failure`.
- **TDD:** red → green → commit per step. Frequent commits. DRY, YAGNI.
- **Node field facts (verified against the bindings):**
  - `CollisionSensor` (base `X3DSensorNode`): `getCollider()`, `getEnabled()` (base), `getIsActive()` (base), `getContacts()`, `getIntersections()`. Reflection x3dNames `"contacts"`, `"intersections"`, `"isActive"` each carry a `set` thunk that calls `emitContacts`/`emitIntersections`/`emitIsActive` — so `postEvent(sensor, "contacts"|"intersections"|"isActive", ...)` delivers (CollisionSensor.cpp:31-126).
  - `Contact` setters: `setPosition(SFVec3f)`, `setContactNormal(SFVec3f)`, `setDepth(SFFloat)`, `setBody1(SFNode)`, `setBody2(SFNode)`, `setGeometry1(SFNode)`, `setGeometry2(SFNode)`, `setBounce(SFFloat)` (range-checked; collection defaults are in range), `setFrictionCoefficients(SFVec2f)`, `setMinBounceSpeed(SFFloat)`, `setSurfaceSpeed(SFVec2f)`, `setSlipCoefficients(SFVec2f)`, `setSoftnessConstantForceMix(SFFloat)`, `setSoftnessErrorCorrection(SFFloat)`, `setAppliedParameters(std::vector<AppliedParametersChoices>)`.
  - `CollisionCollection` getters: `getCollidables()` (MFNode), `getEnabled()`, `getBounce()`, `getFrictionCoefficients()`, `getMinBounceSpeed()`, `getSurfaceSpeed()`, `getSlipFactors()` (→ Contact `slipCoefficients`), `getSoftnessConstantForceMix()`, `getSoftnessErrorCorrection()`, `getAppliedParameters()`.
  - `AppliedParametersChoices` is one shared enum (`X3Denums.hpp`); the collection's vector passes straight to `Contact::setAppliedParameters`.
  - `CollisionSpace` getter: `getCollidables()` (MFNode), `getEnabled()`.
  - Types: `SFNode = std::shared_ptr<X3DNode>`, `MFNode = std::vector<std::shared_ptr<X3DNode>>`, `SFVec2f{float x,y}`, `SFVec3f{float x,y,z}`.
  - Cascade drive in tests: `ctx.tick(now)` (advances + drains) or seed then `ctx.process()` to drain. Read results via the getters above.
  - **Node-construction idiom (verified against `physics_system_test.cpp`):** node-child/initializeOnly fields have **only** `*Unchecked` setters and want an upcast `SFNode`. Build scenes exactly like:
    ```cpp
    auto box = std::make_shared<Box>();
    box->setSizeUnchecked(SFVec3f{1, 1, 1});           // NOT setSize
    auto shape = std::make_shared<Shape>();
    shape->setGeometry(std::static_pointer_cast<X3DNode>(box));
    auto coll = std::make_shared<CollidableShape>();
    coll->setShapeUnchecked(std::static_pointer_cast<X3DNode>(shape));  // NOT setShape
    auto body = std::make_shared<RigidBody>();
    body->setGeometry(MFNode{std::static_pointer_cast<X3DNode>(coll)});
    auto collection = std::make_shared<RigidBodyCollection>();
    collection->setBodies(MFNode{std::static_pointer_cast<X3DNode>(body)});
    ```
    Sphere radius uses `setRadiusUnchecked`. The test code below follows this idiom; if a literal `setSize`/`setShape` slips in, it will not compile — use the `*Unchecked` form.

## File Structure

- `runtime/physics/PhysicsBackend.hpp` — **modify**: add `ContactPoint` struct + virtual `drainContacts`.
- `runtime/physics/ContactReporter.hpp` — **create**: `ResolvedContact` struct + `ContactReporter` class (node-only, no handles/engine).
- `runtime/physics/PhysicsSystem.hpp` — **modify**: capture the `CollidableShape` per body, build a `(world,handle)→{node,shape,ordinal}` map, accept `CollisionSensor` in `attach`, drain + resolve + report in `update`.
- `runtime/physics/jolt/JoltBackend.cpp` — **modify**: per-world `ContactListener` buffer + `drainContacts`.
- `runtime/physics/jolt/JoltBackend.hpp` — **modify**: declare the `drainContacts` override.
- `runtime/physics/tests/contact_reporter_test.cpp` — **create**: ungated pure-logic test of `ContactReporter`.
- `runtime/physics/tests/physics_contact_wiring_test.cpp` — **create**: ungated `PhysicsSystem`+fake-backend integration test.
- `runtime/physics/tests/physics_system_test.cpp` — **modify**: gated Jolt collision test (two boxes) + determinism.
- `tools/x3d-cli/fixtures/sim-collision.x3d` — **create**: collision scene with a `CollisionSensor`.
- `tools/x3d-cli/goldens/sim-collision.trace.json` — **create**: golden trace.
- `tools/tests/x3d_cli_test.sh` — **modify**: gated collision-reporting assertions.
- `CMakeLists.txt` — **modify**: register the two new ungated tests.
- `docs/conformance/findings.yaml`, `docs/conformance/components/RigidBodyPhysics.md`, `docs/superpowers/BACKLOG.md` — **modify**: mark `CONF-RBP` fixed; log deferred refinements.

---

### Task 1: Backend seam — `ContactPoint` + `drainContacts`

**Files:**
- Modify: `runtime/physics/PhysicsBackend.hpp` (add struct near `ConstraintDesc`; add method to `PhysicsBackend`)
- Test: `runtime/physics/tests/contact_reporter_test.cpp` (new — first assertion only)

**Interfaces:**
- Produces: `struct ContactPoint { BodyHandle bodyA, bodyB; SFVec3f position, normal; float depth; }`; `virtual void PhysicsBackend::drainContacts(WorldHandle, std::vector<ContactPoint>&)` with an empty default.

- [ ] **Step 1: Write the failing test**

Create `runtime/physics/tests/contact_reporter_test.cpp`:

```cpp
// contact_reporter_test.cpp — ungated pure-logic tests for the §37 contact-
// reporting bridge (ContactPoint seam struct + ContactReporter). No physics
// engine: synthetic ResolvedContacts + hand-built nodes exercise the translator.
#include "PhysicsBackend.hpp"

#include <cstdio>

using namespace x3d::runtime;

static int g_failures = 0;
#define CHECK(cond, msg)                                                        \
  do {                                                                          \
    if (!(cond)) { std::printf("FAIL: %s\n", msg); ++g_failures; }             \
    else { std::printf("ok:   %s\n", msg); }                                   \
  } while (0)

static void test_contact_point_defaults() {
  ContactPoint cp;
  CHECK(cp.bodyA == kInvalidBodyHandle, "ContactPoint.bodyA defaults invalid");
  CHECK(cp.bodyB == kInvalidBodyHandle, "ContactPoint.bodyB defaults invalid");
  CHECK(cp.normal.x == 0.0f && cp.normal.y == 1.0f && cp.normal.z == 0.0f,
        "ContactPoint.normal defaults to up");
  CHECK(cp.depth == 0.0f, "ContactPoint.depth defaults 0");
}

int main() {
  test_contact_point_defaults();
  return g_failures == 0 ? 0 : 1;
}
```

- [ ] **Step 2: Verify it fails to compile**

Run: `cd <repo-root> && g++ -std=c++20 -Iruntime/physics -Igenerated_cpp_bindings -fsyntax-only runtime/physics/tests/contact_reporter_test.cpp`
Expected: FAIL — `'ContactPoint' was not declared`.

- [ ] **Step 3: Add the struct + method**

In `runtime/physics/PhysicsBackend.hpp`, add immediately after the `ConstraintDesc` struct (before `class PhysicsBackend`):

```cpp
/**
 * @brief One contact between two bodies, in the runtime's own terms.
 * @details The seam never names an engine contact type. Filled by the backend
 *          during step() and handed back via drainContacts(). bodyA/bodyB are
 *          handles within the drained world; normal points from bodyA toward
 *          bodyB; depth is the penetration along normal.
 */
struct ContactPoint {
  BodyHandle bodyA = kInvalidBodyHandle;
  BodyHandle bodyB = kInvalidBodyHandle;
  SFVec3f position{0, 0, 0};
  SFVec3f normal{0, 1, 0};
  float depth = 0.0f;
};
```

In `class PhysicsBackend`, add after `getBodyTransform`:

```cpp
  /**
   * @brief Move out every contact recorded since the previous drain for `world`.
   * @details Pull model (mirrors getBodyTransform readback): the backend buffers
   *          contacts during step(); the PhysicsSystem drains them after the
   *          frame's substeps. Default: no contacts — a backend without contact
   *          support stays silent, and contact reporting is a clean no-op.
   */
  virtual void drainContacts(WorldHandle world, std::vector<ContactPoint> &out) {
    (void)world;
    out.clear();
  }
```

Ensure `#include <vector>` is present in the header (add if missing).

- [ ] **Step 4: Verify the test compiles + passes**

Run: `cd <repo-root> && g++ -std=c++20 -Iruntime/physics -Igenerated_cpp_bindings -o /tmp/cr_t runtime/physics/tests/contact_reporter_test.cpp && /tmp/cr_t`
Expected: all `ok:`; exit 0.

- [ ] **Step 5: Commit**

```bash
git add runtime/physics/PhysicsBackend.hpp runtime/physics/tests/contact_reporter_test.cpp
git commit -m "feat(physics): add ContactPoint + drainContacts seam (CONF-RBP)"
```

---

### Task 2: `ContactReporter` — resolve, filter, populate, emit

**Files:**
- Create: `runtime/physics/ContactReporter.hpp`
- Test: `runtime/physics/tests/contact_reporter_test.cpp` (extend)

**Interfaces:**
- Consumes: `ContactPoint` (Task 1) is *not* used here — the reporter is handle-free.
- Produces:
  - `struct ResolvedContact { SFNode body1, body2, geometry1, geometry2; SFVec3f position, normal; float depth; std::uint64_t pairKey; }`
  - `class ContactReporter { void addSensor(CollisionSensor*); void report(const std::vector<ResolvedContact>&, X3DExecutionContext&); }`

- [ ] **Step 1: Write the failing happy-path test**

Append to `runtime/physics/tests/contact_reporter_test.cpp` (and add includes at the top: `#include "ContactReporter.hpp"`, `#include "X3DExecutionContext.hpp"`, `#include "CollisionSensor.hpp"`, `#include "CollisionCollection.hpp"`, `#include "CollidableShape.hpp"`, `#include "RigidBody.hpp"`, `#include <memory>`):

```cpp
// Build: sensor -> collider(collection{bounce=0.5, collidables=[shapeA,shapeB]})
// One ResolvedContact between bodyA/bodyB whose shapes are shapeA/shapeB.
// Expect: one Contact emitted with geometric + collection-default fields,
// intersections = {shapeA, shapeB}, isActive = true.
static void test_single_contact_emitted() {
  auto bodyA = std::make_shared<RigidBody>();
  auto bodyB = std::make_shared<RigidBody>();
  auto shapeA = std::make_shared<CollidableShape>();
  auto shapeB = std::make_shared<CollidableShape>();

  auto collection = std::make_shared<CollisionCollection>();
  collection->setBounce(0.5f);
  collection->setMinBounceSpeed(0.2f);
  collection->setFrictionCoefficients(SFVec2f{0.3f, 0.4f});
  collection->setSlipFactors(SFVec2f{0.1f, 0.2f});
  collection->setCollidables(MFNode{shapeA, shapeB});

  auto sensor = std::make_shared<CollisionSensor>();
  sensor->setCollider(collection);

  ResolvedContact rc;
  rc.body1 = bodyA; rc.body2 = bodyB;
  rc.geometry1 = shapeA; rc.geometry2 = shapeB;
  rc.position = SFVec3f{1, 2, 3};
  rc.normal = SFVec3f{0, 1, 0};
  rc.depth = 0.05f;
  rc.pairKey = (std::uint64_t(0) << 32) | 1;

  x3d::runtime::X3DExecutionContext ctx;
  ContactReporter reporter;
  reporter.addSensor(sensor.get());
  reporter.report(std::vector<ResolvedContact>{rc}, ctx);
  ctx.process();

  MFNode contacts = sensor->getContacts();
  CHECK(contacts.size() == 1, "one Contact emitted");
  CHECK(sensor->getIsActive() == true, "isActive true when contacts");
  CHECK(sensor->getIntersections().size() == 2, "two intersecting collidables");

  if (contacts.size() == 1) {
    auto *c = dynamic_cast<Contact *>(contacts[0].get());
    CHECK(c != nullptr, "emitted node is a Contact");
    if (c) {
      CHECK(c->getPosition().y == 2.0f, "Contact.position from engine");
      CHECK(c->getContactNormal().y == 1.0f, "Contact.contactNormal from engine");
      CHECK(c->getDepth() == 0.05f, "Contact.depth from engine");
      CHECK(c->getBody1().get() == bodyA.get(), "Contact.body1 = bodyA");
      CHECK(c->getGeometry2().get() == shapeB.get(), "Contact.geometry2 = shapeB");
      CHECK(c->getBounce() == 0.5f, "Contact.bounce from collection default");
      CHECK(c->getFrictionCoefficients().x == 0.3f, "frictionCoefficients copied");
      CHECK(c->getSlipCoefficients().y == 0.2f, "slipFactors -> slipCoefficients");
      CHECK(c->getMinBounceSpeed() == 0.2f, "minBounceSpeed copied");
    }
  }
}
```

Add `test_single_contact_emitted();` to `main()` (also add `#include "Contact.hpp"`).

- [ ] **Step 2: Verify it fails**

Run: `cd <repo-root> && g++ -std=c++20 -Iruntime/physics -Iruntime/events -Igenerated_cpp_bindings -fsyntax-only runtime/physics/tests/contact_reporter_test.cpp`
Expected: FAIL — `ContactReporter.hpp: No such file`.

- [ ] **Step 3: Create `runtime/physics/ContactReporter.hpp`**

```cpp
// ContactReporter.hpp
// Translates already-resolved §37 collision contacts into X3D events on the
// watching CollisionSensor nodes. ENGINE-AGNOSTIC and HANDLE-FREE: it names no
// physics-engine type and no backend handle — the PhysicsSystem resolves each
// drained ContactPoint to scene nodes (RigidBody + CollidableShape) and a stable
// pair ordinal before handing a vector of ResolvedContact here. This file builds
// without X3D_CPP_BUILD_PHYSICS. A seam-purity check greps it for engine leakage.
//
// §37.4.5: the collision system runs regardless of any CollisionSensor; the
// sensor only REPORTS. We never feed contacts back into the solver (the spec
// strongly advises against routing contacts -> set_contacts). This is pure
// read-only output: build Contact nodes, emit contacts/intersections/isActive.
#ifndef X3D_RUNTIME_PHYSICS_CONTACT_REPORTER_HPP
#define X3D_RUNTIME_PHYSICS_CONTACT_REPORTER_HPP

#include "X3DExecutionContext.hpp"

#include "CollidableShape.hpp"
#include "CollisionCollection.hpp"
#include "CollisionSensor.hpp"
#include "CollisionSpace.hpp"
#include "Contact.hpp"
#include "RigidBody.hpp"

#include <algorithm>
#include <any>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace x3d::runtime {

/**
 * @brief One contact resolved to scene nodes, ready to report.
 * @details The PhysicsSystem fills this from a backend ContactPoint: handles ->
 *          the RigidBody + CollidableShape SFNodes, plus a STABLE pairKey built
 *          from enrollment ordinals (min<<32 | max) so dedupe + ordering are
 *          deterministic regardless of pointer addresses or engine callback order.
 */
struct ResolvedContact {
  SFNode body1;
  SFNode body2;
  SFNode geometry1;
  SFNode geometry2;
  SFVec3f position{0, 0, 0};
  SFVec3f normal{0, 1, 0};
  float depth = 0.0f;
  std::uint64_t pairKey = 0;
};

/**
 * @brief Emits §37 CollisionSensor outputs from resolved contacts.
 * @details Per frame, report() is called once with all resolved contacts. For
 *          each enrolled sensor it: gates on sensor/collection enabled + a
 *          non-null collider; flattens collider->collidables (descending nested
 *          CollisionSpaces) into a CollidableShape set; keeps contacts whose both
 *          geometries are in that set; dedupes per body-pair (max depth) and
 *          sorts by pairKey; builds Contact nodes (geometry from the engine,
 *          response fields from the collection defaults); and posts contacts /
 *          intersections / isActive (isActive only on change).
 */
class ContactReporter {
public:
  /** @brief Enroll a CollisionSensor to watch (idempotent per pointer). */
  void addSensor(CollisionSensor *sensor) {
    if (sensor) sensors_.push_back(sensor);
  }

  /** @brief Build + emit this frame's contact outputs for every sensor. */
  void report(const std::vector<ResolvedContact> &contacts,
              X3DExecutionContext &ctx) {
    for (CollisionSensor *sensor : sensors_) {
      reportSensor(sensor, contacts, ctx);
    }
  }

private:
  void reportSensor(CollisionSensor *sensor,
                    const std::vector<ResolvedContact> &contacts,
                    X3DExecutionContext &ctx) {
    // Gate: a disabled sensor / null collider / disabled collection reports
    // nothing this frame (§37.4.4/.4.5). Drop isActive on the active->inactive
    // transition so a downstream route sees the sensor go quiet.
    auto *collection =
        sensor->getEnabled()
            ? dynamic_cast<CollisionCollection *>(sensor->getCollider().get())
            : nullptr;
    if (!collection || !collection->getEnabled()) {
      setActive(sensor, false, ctx);
      return;
    }

    std::unordered_set<const X3DNode *> watched;
    collectCollidables(collection->getCollidables(), watched);

    // Filter to contacts whose BOTH geometries are in the watched set, deduped
    // per pairKey keeping the deepest, then ordered by pairKey (deterministic).
    std::unordered_map<std::uint64_t, const ResolvedContact *> byPair;
    for (const ResolvedContact &c : contacts) {
      if (!watched.count(c.geometry1.get()) ||
          !watched.count(c.geometry2.get()))
        continue;
      auto it = byPair.find(c.pairKey);
      if (it == byPair.end() || c.depth > it->second->depth)
        byPair[c.pairKey] = &c;
    }
    if (byPair.empty()) {
      setActive(sensor, false, ctx);
      return;
    }

    std::vector<const ResolvedContact *> ordered;
    ordered.reserve(byPair.size());
    for (auto &kv : byPair) ordered.push_back(kv.second);
    std::sort(ordered.begin(), ordered.end(),
              [](const ResolvedContact *a, const ResolvedContact *b) {
                return a->pairKey < b->pairKey;
              });

    MFNode out;
    MFNode intersections;
    std::unordered_set<const X3DNode *> seenGeom;
    out.reserve(ordered.size());
    for (const ResolvedContact *c : ordered) {
      out.push_back(makeContact(*c, *collection));
      addUnique(intersections, seenGeom, c->geometry1);
      addUnique(intersections, seenGeom, c->geometry2);
    }

    ctx.postEvent(sensor, "contacts", std::any(out));
    ctx.postEvent(sensor, "intersections", std::any(intersections));
    setActive(sensor, true, ctx);
  }

  /// Flatten collidables into a CollidableShape set, descending nested
  /// CollisionSpaces (visited-guard against shared/cyclic spaces).
  void collectCollidables(const MFNode &nodes,
                          std::unordered_set<const X3DNode *> &out) {
    for (const SFNode &n : nodes) {
      if (!n) continue;
      if (auto *shape = dynamic_cast<CollidableShape *>(n.get())) {
        out.insert(shape);
      } else if (auto *space = dynamic_cast<CollisionSpace *>(n.get())) {
        if (visitedSpaces_.insert(space).second)
          collectCollidables(space->getCollidables(), out);
      }
    }
    visitedSpaces_.clear();
  }

  static void addUnique(MFNode &out, std::unordered_set<const X3DNode *> &seen,
                        const SFNode &n) {
    if (n && seen.insert(n.get()).second) out.push_back(n);
  }

  /// Build one Contact: geometry from the engine, response from the collection.
  static SFNode makeContact(const ResolvedContact &c,
                            const CollisionCollection &col) {
    auto contact = std::make_shared<Contact>();
    contact->setPosition(c.position);
    contact->setContactNormal(c.normal);
    contact->setDepth(c.depth);
    contact->setBody1(c.body1);
    contact->setBody2(c.body2);
    contact->setGeometry1(c.geometry1);
    contact->setGeometry2(c.geometry2);
    // Response defaults assigned from the generating CollisionCollection
    // (§37.4.4): all generated contacts inherit the collection's parameters.
    // Use the *Unchecked setters for range-checked fields — the values were
    // already validated on the collection (or kept by a permissive reader), and
    // throwing mid-simulation would be wrong.
    contact->setBounceUnchecked(col.getBounce());
    contact->setMinBounceSpeedUnchecked(col.getMinBounceSpeed());
    contact->setFrictionCoefficientsUnchecked(col.getFrictionCoefficients());
    contact->setSurfaceSpeed(col.getSurfaceSpeed());            // unchecked field
    contact->setSlipCoefficients(col.getSlipFactors());         // unchecked field
    contact->setSoftnessConstantForceMixUnchecked(col.getSoftnessConstantForceMix());
    contact->setSoftnessErrorCorrectionUnchecked(col.getSoftnessErrorCorrection());
    contact->setAppliedParameters(col.getAppliedParameters());
    // frictionDirection left at default — the system computes it unless
    // USER_FRICTION is set (spec-correct to omit here).
    return contact;
  }

  /// Emit isActive only when it changes (per-sensor previous state).
  void setActive(CollisionSensor *sensor, bool active,
                 X3DExecutionContext &ctx) {
    bool &prev = active_[sensor];  // value-initialises to false on first lookup
    if (prev == active) return;
    prev = active;
    ctx.postEvent(sensor, "isActive", std::any(SFBool(active)));
  }

  std::vector<CollisionSensor *> sensors_;
  std::unordered_map<CollisionSensor *, bool> active_;
  std::unordered_set<const X3DNode *> visitedSpaces_;  // scratch for flatten
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_PHYSICS_CONTACT_REPORTER_HPP
```

- [ ] **Step 4: Verify the test passes**

Run: `cd <repo-root> && g++ -std=c++20 -Iruntime/physics -Iruntime/events -Igenerated_cpp_bindings -o /tmp/cr_t runtime/physics/tests/contact_reporter_test.cpp generated_cpp_bindings/CollisionSensor.cpp generated_cpp_bindings/Contact.cpp generated_cpp_bindings/CollisionCollection.cpp generated_cpp_bindings/CollidableShape.cpp generated_cpp_bindings/CollisionSpace.cpp generated_cpp_bindings/RigidBody.cpp $(echo) && /tmp/cr_t`

Note: the generated nodes have transitive deps. If the manual `g++` link is unwieldy, skip it — Step 5 wires the test into CMake (linking `x3d_cpp::x3d_cpp`), and Task 2's real green gate is `ctest -R x3d_contact_reporter` after the CMake step. Prefer running via CMake.

- [ ] **Step 5: Wire the test into CMake**

In `CMakeLists.txt`, near the other ungated runtime tests (e.g. after the `x3d_navigation` block around line 1065), add:

```cmake
    # §37 contact reporting: ContactReporter translates resolved collisions into
    # CollisionSensor.contacts/intersections/isActive events (CONF-RBP). Ungated —
    # the reporter is engine-free; only the Jolt drain path needs the physics flag.
    add_executable(x3d_contact_reporter
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics/tests/contact_reporter_test.cpp")
    target_link_libraries(x3d_contact_reporter PRIVATE x3d_cpp::x3d_cpp)
    target_include_directories(x3d_contact_reporter PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics")
    add_test(NAME x3d_contact_reporter COMMAND x3d_contact_reporter)
```

- [ ] **Step 6: Build + run the test via CMake**

Run: `cd <repo-root> && cmake --preset dev && cmake --build --preset dev --target x3d_contact_reporter && ctest --preset dev -R x3d_contact_reporter --output-on-failure`
Expected: PASS.

- [ ] **Step 7: Commit**

```bash
git add runtime/physics/ContactReporter.hpp runtime/physics/tests/contact_reporter_test.cpp CMakeLists.txt
git commit -m "feat(physics): ContactReporter emits CollisionSensor outputs (CONF-RBP)"
```

---

### Task 3: `ContactReporter` edge cases — dedupe, ordering, gating, isActive transition

**Files:**
- Test: `runtime/physics/tests/contact_reporter_test.cpp` (extend)

**Interfaces:**
- Consumes: `ContactReporter`, `ResolvedContact` (Task 2). No new production API — these assert behaviour already implemented in Task 2; if any fails, fix `ContactReporter.hpp`.

- [ ] **Step 1: Write the edge-case tests**

Append these functions and call them from `main()`:

```cpp
// collider == NULL -> no contacts, no isActive event (stays default false).
static void test_null_collider_silent() {
  auto sensor = std::make_shared<CollisionSensor>();  // collider defaults NULL
  x3d::runtime::X3DExecutionContext ctx;
  ContactReporter reporter;
  reporter.addSensor(sensor.get());
  reporter.report({}, ctx);
  ctx.process();
  CHECK(sensor->getContacts().empty(), "null collider -> no contacts");
  CHECK(sensor->getIsActive() == false, "null collider -> isActive false");
}

// collection.enabled == FALSE silences the sensor even with a matching contact.
static void test_disabled_collection_silent() {
  auto shapeA = std::make_shared<CollidableShape>();
  auto shapeB = std::make_shared<CollidableShape>();
  auto col = std::make_shared<CollisionCollection>();
  col->setEnabled(false);
  col->setCollidables(MFNode{shapeA, shapeB});
  auto sensor = std::make_shared<CollisionSensor>();
  sensor->setCollider(col);
  ResolvedContact rc;
  rc.geometry1 = shapeA; rc.geometry2 = shapeB; rc.pairKey = 1;
  x3d::runtime::X3DExecutionContext ctx;
  ContactReporter reporter;
  reporter.addSensor(sensor.get());
  reporter.report({rc}, ctx);
  ctx.process();
  CHECK(sensor->getContacts().empty(), "disabled collection -> no contacts");
}

// Two ResolvedContacts with the same pairKey collapse to one (keep max depth).
static void test_dedupe_by_pair_keep_max_depth() {
  auto shapeA = std::make_shared<CollidableShape>();
  auto shapeB = std::make_shared<CollidableShape>();
  auto col = std::make_shared<CollisionCollection>();
  col->setCollidables(MFNode{shapeA, shapeB});
  auto sensor = std::make_shared<CollisionSensor>();
  sensor->setCollider(col);
  ResolvedContact a; a.geometry1 = shapeA; a.geometry2 = shapeB; a.pairKey = 7; a.depth = 0.01f;
  ResolvedContact b; b.geometry1 = shapeA; b.geometry2 = shapeB; b.pairKey = 7; b.depth = 0.09f;
  x3d::runtime::X3DExecutionContext ctx;
  ContactReporter reporter;
  reporter.addSensor(sensor.get());
  reporter.report({a, b}, ctx);
  ctx.process();
  CHECK(sensor->getContacts().size() == 1, "same pairKey deduped to one Contact");
  auto *c = sensor->getContacts().empty()
                ? nullptr
                : dynamic_cast<Contact *>(sensor->getContacts()[0].get());
  CHECK(c && c->getDepth() == 0.09f, "dedupe keeps the deepest contact");
}

// Contacts emit sorted by pairKey regardless of input order.
static void test_deterministic_ordering() {
  auto s1 = std::make_shared<CollidableShape>();
  auto s2 = std::make_shared<CollidableShape>();
  auto s3 = std::make_shared<CollidableShape>();
  auto col = std::make_shared<CollisionCollection>();
  col->setCollidables(MFNode{s1, s2, s3});
  auto sensor = std::make_shared<CollisionSensor>();
  sensor->setCollider(col);
  ResolvedContact hi; hi.geometry1 = s2; hi.geometry2 = s3; hi.pairKey = 100;
  ResolvedContact lo; lo.geometry1 = s1; lo.geometry2 = s2; lo.pairKey = 5;
  x3d::runtime::X3DExecutionContext ctx;
  ContactReporter reporter;
  reporter.addSensor(sensor.get());
  reporter.report({hi, lo}, ctx);  // deliberately out of order
  ctx.process();
  auto cs = sensor->getContacts();
  CHECK(cs.size() == 2, "two distinct-pair contacts emitted");
  // lo (pairKey 5) must come first.
  auto *first = cs.empty() ? nullptr : dynamic_cast<Contact *>(cs[0].get());
  CHECK(first && first->getGeometry1().get() == s1.get(),
        "contacts ordered by pairKey (lowest first)");
}

// isActive only changes on transitions: true once, then false once contacts stop.
static void test_isactive_transition() {
  auto shapeA = std::make_shared<CollidableShape>();
  auto shapeB = std::make_shared<CollidableShape>();
  auto col = std::make_shared<CollisionCollection>();
  col->setCollidables(MFNode{shapeA, shapeB});
  auto sensor = std::make_shared<CollisionSensor>();
  sensor->setCollider(col);
  ResolvedContact rc; rc.geometry1 = shapeA; rc.geometry2 = shapeB; rc.pairKey = 1;

  x3d::runtime::X3DExecutionContext ctx;
  ContactReporter reporter;
  reporter.addSensor(sensor.get());

  reporter.report({rc}, ctx); ctx.process();
  CHECK(sensor->getIsActive() == true, "isActive true after a contact");

  reporter.report({}, ctx); ctx.process();
  CHECK(sensor->getIsActive() == false, "isActive false after contacts stop");
}
```

- [ ] **Step 2: Run — verify they pass (Task 2 already implements the behaviour)**

Run: `cd <repo-root> && cmake --build --preset dev --target x3d_contact_reporter && ctest --preset dev -R x3d_contact_reporter --output-on-failure`
Expected: PASS. If any fails, fix `ContactReporter.hpp` until green (do not weaken the test).

- [ ] **Step 3: Commit**

```bash
git add runtime/physics/tests/contact_reporter_test.cpp runtime/physics/ContactReporter.hpp
git commit -m "test(physics): ContactReporter dedupe/ordering/gating/isActive edge cases"
```

---

### Task 4: `PhysicsSystem` wiring — resolve handles, enrol sensors, drain + report

**Files:**
- Modify: `runtime/physics/PhysicsSystem.hpp`
- Test: `runtime/physics/tests/physics_contact_wiring_test.cpp` (new — ungated, fake backend)

**Interfaces:**
- Consumes: `ContactPoint`, `drainContacts` (Task 1); `ResolvedContact`, `ContactReporter` (Task 2).
- Produces: `PhysicsSystem::attach` now also enrols `CollisionSensor`; `PhysicsSystem::update` drains each world, resolves contacts to `ResolvedContact`, calls `reporter_.report(...)` once per frame.

- [ ] **Step 1: Write the failing wiring test**

Create `runtime/physics/tests/physics_contact_wiring_test.cpp`:

```cpp
// physics_contact_wiring_test.cpp — ungated integration test for PhysicsSystem's
// contact path, using a FAKE PhysicsBackend that scripts one contact. Proves:
// attach enrols a CollisionSensor; update drains the backend, resolves body
// handles to the scene's RigidBody/CollidableShape nodes, and the reporter emits
// CollisionSensor.contacts over the cascade. No physics engine required.
#include "PhysicsSystem.hpp"

#include "X3DExecutionContext.hpp"

#include "Box.hpp"
#include "CollidableShape.hpp"
#include "CollisionCollection.hpp"
#include "CollisionSensor.hpp"
#include "Contact.hpp"
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

// A minimal backend: addBody hands out 1,2,3...; step is a no-op; drainContacts
// reports a single contact between handles 1 and 2 on the first drain only.
class FakeContactBackend : public PhysicsBackend {
public:
  WorldHandle createWorld(const SFVec3f &) override { return 1; }
  BodyHandle addBody(WorldHandle, const ShapeDesc &, float, bool,
                     const SFVec3f &, const SFRotation &, const SFVec3f &,
                     const SFVec3f &) override {
    return ++lastHandle_;
  }
  ConstraintHandle addConstraint(WorldHandle, const ConstraintDesc &) override {
    return 1;
  }
  void applyForce(WorldHandle, BodyHandle, const SFVec3f &,
                  const SFVec3f &) override {}
  void setGravityFactor(WorldHandle, BodyHandle, float) override {}
  void getBodyVelocity(WorldHandle, BodyHandle, SFVec3f &lin,
                       SFVec3f &ang) const override {
    lin = SFVec3f{0, 0, 0};
    ang = SFVec3f{0, 0, 0};
  }
  void step(WorldHandle, double) override {}
  void getBodyTransform(WorldHandle, BodyHandle, SFVec3f &pos,
                        SFRotation &ori) const override {
    pos = SFVec3f{0, 0, 0};
    ori = SFRotation{0, 0, 1, 0};
  }
  void drainContacts(WorldHandle, std::vector<ContactPoint> &out) override {
    out.clear();
    if (!fired_) {
      fired_ = true;
      ContactPoint cp;
      cp.bodyA = 1;
      cp.bodyB = 2;
      cp.position = SFVec3f{0, 0.5f, 0};
      cp.normal = SFVec3f{0, 1, 0};
      cp.depth = 0.02f;
      out.push_back(cp);
    }
  }

private:
  BodyHandle lastHandle_ = 0;
  bool fired_ = false;
};

// helper: a RigidBody whose geometry is a CollidableShape(Box). The same
// CollidableShape instance is shared into the CollisionCollection (USE-style).
static std::shared_ptr<RigidBody>
makeBody(std::shared_ptr<CollidableShape> &outShape) {
  auto box = std::make_shared<Box>();
  box->setSizeUnchecked(SFVec3f{1, 1, 1});
  auto shapeWrap = std::make_shared<Shape>();
  shapeWrap->setGeometry(std::static_pointer_cast<X3DNode>(box));
  outShape = std::make_shared<CollidableShape>();
  outShape->setShapeUnchecked(std::static_pointer_cast<X3DNode>(shapeWrap));
  auto body = std::make_shared<RigidBody>();
  body->setGeometry(MFNode{std::static_pointer_cast<X3DNode>(outShape)});
  return body;
}

int main() {
  std::shared_ptr<CollidableShape> shapeA, shapeB;
  auto bodyA = makeBody(shapeA);
  auto bodyB = makeBody(shapeB);

  auto collection = std::make_shared<RigidBodyCollection>();
  collection->setBodies(MFNode{bodyA, bodyB});

  auto collCol = std::make_shared<CollisionCollection>();
  collCol->setCollidables(MFNode{shapeA, shapeB});
  auto sensor = std::make_shared<CollisionSensor>();
  sensor->setCollider(collCol);

  X3DExecutionContext ctx;
  auto physics = std::make_shared<PhysicsSystem>(
      std::make_shared<FakeContactBackend>());
  physics->attach(collection.get(), ctx);
  physics->attach(sensor.get(), ctx);
  ctx.addSystem(physics);

  double t = 0.0;
  ctx.tick(t);                 // t=0: establish clock, no dt
  for (int i = 0; i < 3; ++i) { t += 1.0 / 60.0; ctx.tick(t); }

  CHECK(sensor->getContacts().size() == 1, "PhysicsSystem -> sensor got 1 contact");
  CHECK(sensor->getIsActive() == true, "sensor isActive after wired contact");
  auto cs = sensor->getContacts();
  auto *c = cs.empty() ? nullptr : dynamic_cast<Contact *>(cs[0].get());
  CHECK(c && c->getBody1().get() == bodyA.get(),
        "Contact.body1 resolved to the scene RigidBody");
  CHECK(c && c->getGeometry2().get() == shapeB.get(),
        "Contact.geometry2 resolved to the scene CollidableShape");
  return g_failures == 0 ? 0 : 1;
}
```

- [ ] **Step 2: Wire the test into CMake + verify it fails**

In `CMakeLists.txt`, right after the `x3d_contact_reporter` block from Task 2:

```cmake
    # PhysicsSystem contact wiring: drain -> resolve handles -> ContactReporter.
    # Ungated (a fake backend scripts a contact); proves the engine-free wiring.
    add_executable(x3d_physics_contact_wiring
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics/tests/physics_contact_wiring_test.cpp")
    target_link_libraries(x3d_physics_contact_wiring PRIVATE x3d_cpp::x3d_cpp)
    target_include_directories(x3d_physics_contact_wiring PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/physics")
    add_test(NAME x3d_physics_contact_wiring COMMAND x3d_physics_contact_wiring)
```

Run: `cd <repo-root> && cmake --preset dev && cmake --build --preset dev --target x3d_physics_contact_wiring 2>&1 | tail -20`
Expected: FAIL — `PhysicsSystem` has no `attach` for `CollisionSensor` / no contact resolution (the contacts assertion fails at runtime, or it may build but the test fails). Run `ctest --preset dev -R x3d_physics_contact_wiring --output-on-failure` → FAIL on "got 1 contact".

- [ ] **Step 3: Add the includes + reporter member to `PhysicsSystem.hpp`**

Add includes near the existing node includes (after `#include "SliderJoint.hpp"`):

```cpp
#include "CollisionSensor.hpp"
#include "ContactReporter.hpp"
```

Add `#include <map>` and `#include <cstdint>` to the `<...>` include block.

Extend the `Mapped` struct (currently `{RigidBody* body; WorldHandle world; BodyHandle handle;}`) — leave it as is; we add a *separate* resolution map. After the `bodies_` member declarations near the bottom (around line 404), add:

```cpp
  /// A body resolved to its scene nodes + a stable enrolment ordinal, keyed by
  /// (world, handle) so contacts (per-world handles) map back to the scene.
  struct BodyRef {
    SFNode bodyNode;    // the RigidBody, as an SFNode (for Contact.body1/2)
    SFNode shapeNode;   // its CollidableShape (for Contact.geometry1/2)
    std::uint32_t ordinal = 0;  // stable enrolment order -> deterministic pairKey
  };
  std::map<std::pair<WorldHandle, BodyHandle>, BodyRef> bodyRef_;
  std::uint32_t nextOrdinal_ = 0;
  ContactReporter reporter_;
  std::size_t unmappedContacts_ = 0;  // contacts whose body wasn't enrolled
```

- [ ] **Step 4: Make `shapeForBody` also yield the chosen CollidableShape (as an SFNode), capture it, enrol sensors**

`X3DNode` does **not** derive from `enable_shared_from_this` (verified: no `enable_shared_from_this` in `generated_cpp_bindings/X3DNode.hpp`), so we capture the `CollidableShape`'s existing `shared_ptr` — the `geomNode` loop variable is already an `SFNode`. Replace the `shapeForBody` signature + body to add an `SFNode *outShape` out-param:

```cpp
  static bool shapeForBody(const RigidBody &body, ShapeDesc &out,
                           SFNode *outShape) {
    for (const auto &geomNode : body.getGeometry()) {
      auto *collidable = dynamic_cast<CollidableShape *>(geomNode.get());
      if (!collidable) continue;
      auto *shapeNode = dynamic_cast<Shape *>(collidable->getShape().get());
      if (!shapeNode) continue;
      X3DNode *geom = shapeNode->getGeometry().get();
      if (auto *box = dynamic_cast<Box *>(geom)) {
        const SFVec3f s = box->getSize();
        out = ShapeDesc::box(SFVec3f{s.x * 0.5f, s.y * 0.5f, s.z * 0.5f});
        if (outShape) *outShape = geomNode;
        return true;
      }
      if (auto *sphere = dynamic_cast<Sphere *>(geom)) {
        out = ShapeDesc::sphere(sphere->getRadius());
        if (outShape) *outShape = geomNode;
        return true;
      }
      if (auto *cyl = dynamic_cast<Cylinder *>(geom)) {
        out = ShapeDesc::cylinder(cyl->getRadius(), cyl->getHeight() * 0.5f);
        if (outShape) *outShape = geomNode;
        return true;
      }
      if (auto *cone = dynamic_cast<Cone *>(geom)) {
        const float r = cone->getBottomRadius();
        out = ShapeDesc::box(SFVec3f{r, cone->getHeight() * 0.5f, r});
        if (outShape) *outShape = geomNode;
        return true;
      }
    }
    return false;
  }
```

In `attach`, the body loop currently calls `shapeForBody(*body, shape)`. Change:

```cpp
      ShapeDesc shape;
      if (!shapeForBody(*body, shape)) {
```
to:
```cpp
      ShapeDesc shape;
      SFNode collidableShape;
      if (!shapeForBody(*body, shape, &collidableShape)) {
```

After the existing `bodies_.push_back(Mapped{body, world, handle});` line, add (note: `bodyNode` is the `const SFNode &` loop variable from `for (const auto &bodyNode : collection->getBodies())`):

```cpp
      // Record the resolution so drained contacts (world,handle) map back to the
      // scene nodes, with a stable ordinal for deterministic contact ordering.
      bodyRef_[{world, handle}] =
          BodyRef{bodyNode, collidableShape, nextOrdinal_++};
```

At the top of `attach`, the early `return` for non-`RigidBodyCollection` nodes must now also handle `CollisionSensor`. Replace:

```cpp
    auto *collection = dynamic_cast<RigidBodyCollection *>(node);
    if (!collection) return;
```
with:
```cpp
    if (auto *sensor = dynamic_cast<CollisionSensor *>(node)) {
      reporter_.addSensor(sensor);
      return;
    }
    auto *collection = dynamic_cast<RigidBodyCollection *>(node);
    if (!collection) return;
```

> NOTE: `attach` currently early-returns `if (!backend_) return;` before any of this. Sensors should still enrol when inert? No — without a backend there are no contacts. Keep the existing `if (!backend_) return;` at the top; sensor enrolment below it is fine (inert system never reports, which is correct).

- [ ] **Step 5: Drain + resolve + report in `update`**

At the end of `update`, after the existing pose-readback loop (after the `for (const auto &m : bodies_) { ... postEvent orientation ... }` block, before the method's closing brace), add:

```cpp
    // §37 contact reporting: drain each world's contacts, resolve handles to the
    // scene's RigidBody/CollidableShape nodes (with a stable pairKey), and let
    // the reporter emit CollisionSensor.contacts/intersections/isActive. The
    // response solve already happened inside the backend — this is read-only.
    std::vector<ResolvedContact> resolved;
    std::vector<ContactPoint> buf;
    WorldHandle lastDrained = kInvalidWorldHandle;
    for (const auto &m : bodies_) {
      if (m.world == lastDrained) continue;
      lastDrained = m.world;
      buf.clear();
      backend_->drainContacts(m.world, buf);
      for (const ContactPoint &cp : buf) {
        auto a = bodyRef_.find({m.world, cp.bodyA});
        auto b = bodyRef_.find({m.world, cp.bodyB});
        if (a == bodyRef_.end() || b == bodyRef_.end()) {
          ++unmappedContacts_;  // a collidable not tied to an enrolled body
          continue;
        }
        ResolvedContact rc;
        rc.body1 = a->second.bodyNode;
        rc.body2 = b->second.bodyNode;
        rc.geometry1 = a->second.shapeNode;
        rc.geometry2 = b->second.shapeNode;
        rc.position = cp.position;
        rc.normal = cp.normal;
        rc.depth = cp.depth;
        const std::uint32_t oa = a->second.ordinal;
        const std::uint32_t ob = b->second.ordinal;
        rc.pairKey = (std::uint64_t(std::min(oa, ob)) << 32) | std::max(oa, ob);
        resolved.push_back(std::move(rc));
      }
    }
    reporter_.report(resolved, ctx);
```

Add `#include <algorithm>` to the include block if not already present (for `std::min`/`std::max`).

> NOTE: `report` is called every stepped frame so `isActive` can fall to false when contacts stop. It is cheap when there are no sensors (the reporter loops an empty vector).

- [ ] **Step 6: Verify the wiring test passes**

Run: `cd <repo-root> && cmake --build --preset dev --target x3d_physics_contact_wiring && ctest --preset dev -R x3d_physics_contact_wiring --output-on-failure`
Expected: PASS.

- [ ] **Step 7: Verify seam purity + full ungated ctest still green**

Run: `cd <repo-root> && grep -rE 'JPH|Jolt' runtime/physics/PhysicsSystem.hpp runtime/physics/PhysicsBackend.hpp runtime/physics/ContactReporter.hpp; echo "exit: $?"`
Expected: no matches (grep exit 1 — no lines).

Run: `ctest --preset dev -j "$(nproc)" --output-on-failure 2>&1 | tail -15`
Expected: all pass (no regressions).

- [ ] **Step 8: Commit**

```bash
git add runtime/physics/PhysicsSystem.hpp runtime/physics/tests/physics_contact_wiring_test.cpp CMakeLists.txt
git commit -m "feat(physics): PhysicsSystem drains contacts + reports via CollisionSensor (CONF-RBP)"
```

---

### Task 5: Jolt backend — `ContactListener` + `drainContacts` (gated)

**Files:**
- Modify: `runtime/physics/jolt/JoltBackend.hpp` (declare override)
- Modify: `runtime/physics/jolt/JoltBackend.cpp` (listener + buffer + drain)
- Test: `runtime/physics/tests/physics_system_test.cpp` (extend — gated two-box collision)

**Interfaces:**
- Consumes: `ContactPoint`, `drainContacts` (Task 1); the full wiring (Task 4).
- Produces: `JoltBackend::drainContacts` returns real contacts; the gated test proves an end-to-end emitted `Contact`.

- [ ] **Step 1: Write the failing gated test**

In `runtime/physics/tests/physics_system_test.cpp`, add includes at the top (with the other node includes):

```cpp
#include "CollisionCollection.hpp"
#include "CollisionSensor.hpp"
#include "Contact.hpp"
```

Add a new test block in `main()` (after the existing falling-box block). It builds a fixed floor box at y=0 and a dynamic box falling onto it, plus a `CollisionSensor` watching a `CollisionCollection` of both collidables:

```cpp
  // ── Collision reporting: a dynamic box falls onto a fixed floor; a
  //    CollisionSensor watching both collidables must emit a Contact + isActive.
  {
    auto floorBox = std::make_shared<Box>();
    floorBox->setSizeUnchecked(SFVec3f{10, 1, 10});
    auto floorShapeWrap = std::make_shared<Shape>();
    floorShapeWrap->setGeometry(std::static_pointer_cast<X3DNode>(floorBox));
    auto floorColl = std::make_shared<CollidableShape>();
    floorColl->setShapeUnchecked(std::static_pointer_cast<X3DNode>(floorShapeWrap));
    auto floor = std::make_shared<RigidBody>();
    floor->setFixed(true);
    floor->setPosition(SFVec3f{0, 0, 0});
    floor->setGeometry(MFNode{std::static_pointer_cast<X3DNode>(floorColl)});

    auto fallBox = std::make_shared<Box>();
    fallBox->setSizeUnchecked(SFVec3f{1, 1, 1});
    auto fallShapeWrap = std::make_shared<Shape>();
    fallShapeWrap->setGeometry(std::static_pointer_cast<X3DNode>(fallBox));
    auto fallColl = std::make_shared<CollidableShape>();
    fallColl->setShapeUnchecked(std::static_pointer_cast<X3DNode>(fallShapeWrap));
    auto faller = std::make_shared<RigidBody>();
    faller->setMass(1);
    faller->setPosition(SFVec3f{0, 3, 0});
    faller->setGeometry(MFNode{std::static_pointer_cast<X3DNode>(fallColl)});

    auto collection = std::make_shared<RigidBodyCollection>();
    collection->setGravity(SFVec3f{0, -9.8f, 0});
    collection->setBodies(MFNode{std::static_pointer_cast<X3DNode>(floor),
                                 std::static_pointer_cast<X3DNode>(faller)});

    auto collCol = std::make_shared<CollisionCollection>();
    collCol->setBounce(0.2f);
    collCol->setCollidables(MFNode{std::static_pointer_cast<X3DNode>(floorColl),
                                   std::static_pointer_cast<X3DNode>(fallColl)});
    auto sensor = std::make_shared<CollisionSensor>();
    sensor->setCollider(collCol);

    X3DExecutionContext ctx;
    auto physics = std::make_shared<PhysicsSystem>(std::make_shared<JoltBackend>());
    physics->attach(collection.get(), ctx);
    physics->attach(sensor.get(), ctx);
    ctx.addSystem(physics);

    double t = 0.0;
    ctx.tick(t);
    bool sawContact = false;
    for (int i = 0; i < 120 && !sawContact; ++i) {  // up to 2 s
      t += 1.0 / 60.0;
      ctx.tick(t);
      if (!sensor->getContacts().empty()) sawContact = true;
    }
    CHECK(sawContact, "CollisionSensor emitted a Contact when boxes collided");
    CHECK(sensor->getIsActive() == true, "CollisionSensor isActive on collision");
    auto cs = sensor->getContacts();
    if (!cs.empty()) {
      auto *c = dynamic_cast<Contact *>(cs[0].get());
      CHECK(c != nullptr, "emitted node is a Contact");
      if (c) {
        // The contact normal should be roughly vertical (floor vs faller).
        CHECK(std::fabs(c->getContactNormal().y) > 0.5f,
              "Contact normal is roughly vertical");
        CHECK(c->getBounce() == 0.2f, "Contact.bounce from collection default");
      }
    }
  }
```

- [ ] **Step 2: Build gated + verify the test fails**

Run: `cd <repo-root> && cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=ON && cmake --build --preset dev --target x3d_physics_system_test && ctest --preset dev -R x3d_physics_system --output-on-failure 2>&1 | tail -20`
Expected: FAIL — "CollisionSensor emitted a Contact" fails (`JoltBackend::drainContacts` still returns the empty default).

- [ ] **Step 3: Declare the override in `JoltBackend.hpp`**

In `class JoltBackend`, after the `getBodyTransform` declaration:

```cpp
  void drainContacts(WorldHandle world, std::vector<ContactPoint> &out) override;
```

- [ ] **Step 4: Implement the listener + buffer + drain in `JoltBackend.cpp`**

Add the Jolt contact-listener include with the other Jolt includes near the top:

```cpp
#include <Jolt/Physics/Collision/ContactListener.h>
#include <mutex>
```

Add a contact-collector type in the anonymous namespace (after the broadphase filter types, before `struct JoltWorld`):

```cpp
// Records contacts during PhysicsSystem::Update into a buffer the runtime drains
// after each frame. Single-threaded job system (the deterministic default)
// serialises these callbacks; the mutex keeps it correct under a thread pool too.
class ContactCollector : public ContactListener {
public:
  void OnContactAdded(const Body &b1, const Body &b2,
                      const ContactManifold &manifold,
                      ContactSettings &) override {
    record(b1, b2, manifold);
  }
  void OnContactPersisted(const Body &b1, const Body &b2,
                          const ContactManifold &manifold,
                          ContactSettings &) override {
    record(b1, b2, manifold);
  }

  // Map a Jolt BodyID to the runtime's BodyHandle (index+1 into world.bodies).
  const std::vector<BodyID> *bodies = nullptr;

  void drain(std::vector<ContactPoint> &out) {
    std::lock_guard<std::mutex> lk(mutex_);
    out = std::move(buffer_);
    buffer_.clear();
  }

private:
  BodyHandle handleFor(const BodyID &id) const {
    if (!bodies) return kInvalidBodyHandle;
    for (std::size_t i = 0; i < bodies->size(); ++i)
      if ((*bodies)[i] == id)
        return static_cast<BodyHandle>(i + 1);
    return kInvalidBodyHandle;
  }
  void record(const Body &b1, const Body &b2,
              const ContactManifold &manifold) {
    ContactPoint cp;
    cp.bodyA = handleFor(b1.GetID());
    cp.bodyB = handleFor(b2.GetID());
    if (cp.bodyA == kInvalidBodyHandle || cp.bodyB == kInvalidBodyHandle)
      return;
    RVec3 p = manifold.GetWorldSpaceContactPointOn1(0);
    cp.position = SFVec3f{static_cast<float>(p.GetX()),
                          static_cast<float>(p.GetY()),
                          static_cast<float>(p.GetZ())};
    Vec3 n = manifold.mWorldSpaceNormal;
    cp.normal = SFVec3f{n.GetX(), n.GetY(), n.GetZ()};
    cp.depth = manifold.mPenetrationDepth;
    std::lock_guard<std::mutex> lk(mutex_);
    buffer_.push_back(cp);
  }
  std::vector<ContactPoint> buffer_;
  std::mutex mutex_;
};
```

> NOTE on the manifold API (Jolt v5.5): `ContactManifold::GetWorldSpaceContactPointOn1(uint index)` returns an `RVec3` world point; `mWorldSpaceNormal` is a `Vec3` (points to move body 2 out of body 1); `mPenetrationDepth` is a `float`. If `GetWorldSpaceContactPointOn1` is unavailable in the pinned version, use `manifold.mBaseOffset + manifold.mRelativeContactPointsOn1[0]`. Confirm with `grep -rn "GetWorldSpaceContactPointOn1\|mRelativeContactPointsOn1\|mWorldSpaceNormal" build/_deps/*jolt*/Jolt/Physics/Collision/ContactListener.h build/_deps/*jolt*/Jolt/Physics/Collision/ContactManifold.h 2>/dev/null` after the first gated configure.

Add the collector to `JoltWorld`:

```cpp
struct JoltWorld {
  std::unique_ptr<PhysicsSystem> physics;
  std::vector<BodyID> bodies;
  std::vector<Body *> bodyPtrs;
  std::vector<Ref<Constraint>> constraints;
  std::unique_ptr<ContactCollector> contacts;   // NEW: per-world contact buffer
  bool optimized = false;
};
```

In `createWorld`, after `world.physics->Init(...)` and `SetGravity(...)`, create + register the collector, then fix its `bodies` pointer after the world is in the map (the map gives stable references):

```cpp
  world.contacts = std::make_unique<ContactCollector>();
  world.physics->SetContactListener(world.contacts.get());

  WorldHandle h = impl_->nextWorld++;
  auto [it, ok] = impl_->worlds.emplace(h, std::move(world));
  // The collector resolves BodyID -> handle via the world's bodies vector;
  // bind it now that the world (and its bodies vector) lives in the map.
  it->second.contacts->bodies = &it->second.bodies;
  return h;
```

(Replace the existing tail of `createWorld` — the `WorldHandle h = ...; impl_->worlds.emplace(...); return h;` — with the block above.)

Add the `drainContacts` implementation near `getBodyTransform`:

```cpp
void JoltBackend::drainContacts(WorldHandle world,
                                std::vector<ContactPoint> &out) {
  out.clear();
  auto it = impl_->worlds.find(world);
  if (it == impl_->worlds.end() || !it->second.contacts) return;
  it->second.contacts->drain(out);
}
```

- [ ] **Step 5: Build gated + verify the test passes**

Run: `cd <repo-root> && cmake --build --preset dev --target x3d_physics_system_test && ctest --preset dev -R x3d_physics_system --output-on-failure 2>&1 | tail -20`
Expected: PASS — including the new collision assertions.

- [ ] **Step 6: Add a determinism assertion (two runs identical)**

Append to the collision block in `physics_system_test.cpp` (reuse the scene-builder by extracting a lambda, or duplicate the build) a second run and compare the first contact's fields:

```cpp
  // Determinism: a second identical run must yield the same first-contact data.
  {
    auto run_once = []() -> std::tuple<float, float, float> {
      auto up = [](auto p) { return std::static_pointer_cast<X3DNode>(p); };  // X3DNode is global
      auto floorBox = std::make_shared<Box>(); floorBox->setSizeUnchecked(SFVec3f{10,1,10});
      auto fsw = std::make_shared<Shape>(); fsw->setGeometry(up(floorBox));
      auto fc = std::make_shared<CollidableShape>(); fc->setShapeUnchecked(up(fsw));
      auto floor = std::make_shared<RigidBody>(); floor->setFixed(true);
      floor->setPosition(SFVec3f{0,0,0}); floor->setGeometry(MFNode{up(fc)});
      auto db = std::make_shared<Box>(); db->setSizeUnchecked(SFVec3f{1,1,1});
      auto dsw = std::make_shared<Shape>(); dsw->setGeometry(up(db));
      auto dc = std::make_shared<CollidableShape>(); dc->setShapeUnchecked(up(dsw));
      auto faller = std::make_shared<RigidBody>(); faller->setMass(1);
      faller->setPosition(SFVec3f{0,3,0}); faller->setGeometry(MFNode{up(dc)});
      auto col = std::make_shared<RigidBodyCollection>();
      col->setGravity(SFVec3f{0,-9.8f,0}); col->setBodies(MFNode{up(floor), up(faller)});
      auto cc = std::make_shared<CollisionCollection>();
      cc->setCollidables(MFNode{up(fc), up(dc)});
      auto sensor = std::make_shared<CollisionSensor>(); sensor->setCollider(cc);
      X3DExecutionContext ctx;
      auto phys = std::make_shared<PhysicsSystem>(std::make_shared<JoltBackend>());
      phys->attach(col.get(), ctx); phys->attach(sensor.get(), ctx);
      ctx.addSystem(phys);
      double t = 0.0; ctx.tick(t);
      for (int i = 0; i < 120; ++i) {
        t += 1.0/60.0; ctx.tick(t);
        if (!sensor->getContacts().empty()) break;
      }
      auto cs = sensor->getContacts();
      if (cs.empty()) return {999.f, 999.f, 999.f};
      auto *c = dynamic_cast<Contact *>(cs[0].get());
      return {c->getPosition().y, c->getContactNormal().y, c->getDepth()};
    };
    auto r1 = run_once();
    auto r2 = run_once();
    CHECK(r1 == r2, "collision reporting is deterministic (two runs identical)");
  }
```

Add `#include <tuple>` at the top if needed.

- [ ] **Step 7: Re-run gated test + verify seam purity unchanged**

Run: `cd <repo-root> && cmake --build --preset dev --target x3d_physics_system_test && ctest --preset dev -R x3d_physics_system --output-on-failure 2>&1 | tail -10`
Expected: PASS.

Run: `grep -rE 'JPH|Jolt' runtime/physics/PhysicsSystem.hpp runtime/physics/PhysicsBackend.hpp runtime/physics/ContactReporter.hpp; echo "exit $?"`
Expected: no matches.

- [ ] **Step 8: Commit**

```bash
git add runtime/physics/jolt/JoltBackend.hpp runtime/physics/jolt/JoltBackend.cpp runtime/physics/tests/physics_system_test.cpp
git commit -m "feat(physics): Jolt ContactListener surfaces contacts via drainContacts (CONF-RBP)"
```

---

### Task 6: CLI golden — `sim-collision.x3d` fixture + harness assertions (gated)

**Files:**
- Create: `tools/x3d-cli/fixtures/sim-collision.x3d`
- Create: `tools/x3d-cli/goldens/sim-collision.trace.json`
- Modify: `tools/tests/x3d_cli_test.sh`

**Interfaces:**
- Consumes: the full pipeline (Tasks 1-5). `x3d sim` already links `x3d_physics_jolt` when built with the flag.
- Produces: a `sim` golden proving contacts surface through the CLI deterministically.

- [ ] **Step 1: Create the fixture**

`tools/x3d-cli/fixtures/sim-collision.x3d`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE X3D PUBLIC "ISO//Web3D//DTD X3D 4.0//EN" "https://www.web3d.org/specifications/x3d-4.0.dtd">
<X3D profile="Full" version="4.0">
  <head>
    <meta name="description" content="sim collision fixture: a dynamic box falls onto a fixed floor; a CollisionSensor reports the contact (§37 CONF-RBP)."/>
  </head>
  <Scene>
    <RigidBodyCollection DEF="World" gravity="0 -9.8 0">
      <RigidBody DEF="Floor" containerField="bodies" fixed="true" position="0 0 0">
        <CollidableShape DEF="FloorColl" containerField="geometry">
          <Shape containerField="shape"><Box size="10 1 10"/></Shape>
        </CollidableShape>
      </RigidBody>
      <RigidBody DEF="Faller" containerField="bodies" mass="1" position="0 3 0">
        <CollidableShape DEF="FallColl" containerField="geometry">
          <Shape containerField="shape"><Box size="1 1 1"/></Shape>
        </CollidableShape>
      </RigidBody>
    </RigidBodyCollection>

    <CollisionSensor DEF="Sensor">
      <CollisionCollection containerField="collider" bounce="0.2">
        <CollidableShape USE="FloorColl" containerField="collidables"/>
        <CollidableShape USE="FallColl" containerField="collidables"/>
      </CollisionCollection>
    </CollisionSensor>
  </Scene>
</X3D>
```

- [ ] **Step 2: Add a non-golden gated assertion to the harness**

In `tools/tests/x3d_cli_test.sh`, inside the existing `if [[ "$PHYSICS" == "1" ... ]]` physics block (after the determinism check, before its closing `fi`), add:

```bash
    # §37 contact reporting: the CollisionSensor must go active when the boxes
    # collide. Watch Sensor.isActive over enough ticks for the faller to land.
    FIXTURE_COLL="$FIXTURES/sim-collision.x3d"
    if [[ -f "$FIXTURE_COLL" ]]; then
        coll=$("$CLI" sim "$FIXTURE_COLL" --fps 60 --ticks 90 --watch Sensor.isActive 2>/dev/null || true)
        if [[ "$coll" == *"Sensor.isActive = true"* ]]; then
            echo "ok:   sim collision -> CollisionSensor.isActive fires"
        else
            echo "FAIL: sim collision did not report CollisionSensor.isActive (got: $coll)"
            failures=$(( failures + 1 ))
        fi
        # Determinism: two --json runs byte-identical.
        cr1="$TD/coll_r1.json"; cr2="$TD/coll_r2.json"
        "$CLI" sim "$FIXTURE_COLL" --fps 60 --ticks 90 --watch Sensor.isActive --json >"$cr1" 2>/dev/null || true
        "$CLI" sim "$FIXTURE_COLL" --fps 60 --ticks 90 --watch Sensor.isActive --json >"$cr2" 2>/dev/null || true
        if cmp -s "$cr1" "$cr2"; then
            echo "ok:   sim collision is deterministic (two runs byte-identical)"
        else
            echo "FAIL: sim collision is non-deterministic (traces differ)"
            failures=$(( failures + 1 ))
        fi
    fi
```

- [ ] **Step 3: Run the harness (gated) + verify**

Run: `cd <repo-root> && cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=ON && cmake --build --preset dev --target x3d_cli && ctest --preset dev -R x3d_cli_test --output-on-failure 2>&1 | tail -25`
Expected: PASS — including the two new collision lines.

- [ ] **Step 4: Capture the golden trace**

Generate the golden from the now-verified output (only after Step 3 passes):

Run: `cd <repo-root> && ./build/tools/x3d-cli/x3d sim tools/x3d-cli/fixtures/sim-collision.x3d --fps 60 --ticks 90 --watch Sensor.isActive --json > tools/x3d-cli/goldens/sim-collision.trace.json`

> NOTE: confirm the binary path with `find build -name x3d -path '*x3d-cli*'` first; adjust if the preset puts it elsewhere.

Then add a golden-regression block to the harness alongside the existing `sim-anim.trace.json` golden check (in the `if [[ -n "$GOLDENS" ... ]]` block, gated additionally on `$PHYSICS == 1`):

```bash
    if [[ "$PHYSICS" == "1" && -f "$GOLDENS/sim-collision.trace.json" ]]; then
        got_coll=$("$CLI" sim "$FIXTURES/sim-collision.x3d" --fps 60 --ticks 90 --watch Sensor.isActive --json 2>/dev/null || true)
        if [[ "$got_coll" == "$(cat "$GOLDENS/sim-collision.trace.json")" ]]; then
            echo "ok:   sim collision trace matches golden"
        else
            echo "FAIL: sim collision trace drifted from golden"
            diff <(printf '%s' "$got_coll") "$GOLDENS/sim-collision.trace.json" | head -20
            failures=$(( failures + 1 ))
        fi
    fi
```

- [ ] **Step 5: Re-run harness + verify golden matches**

Run: `cd <repo-root> && ctest --preset dev -R x3d_cli_test --output-on-failure 2>&1 | tail -25`
Expected: PASS — "sim collision trace matches golden".

- [ ] **Step 6: Commit**

```bash
git add tools/x3d-cli/fixtures/sim-collision.x3d tools/x3d-cli/goldens/sim-collision.trace.json tools/tests/x3d_cli_test.sh
git commit -m "test(physics): x3d sim collision golden trace (CONF-RBP)"
```

---

### Task 7: Close the conformance finding + log deferrals + docs

**Files:**
- Modify: `docs/conformance/findings.yaml`
- Modify: `docs/conformance/components/RigidBodyPhysics.md` (regenerated)
- Modify: `docs/superpowers/BACKLOG.md`
- Modify: the auto-memory pointer (`x3d-cpp-gen-physics-seam.md`) — optional, see Step 5

**Interfaces:**
- Consumes: the merged implementation (Tasks 1-6) and its commit hashes.

- [ ] **Step 1: Mark `CONF-RBP` fixed in `findings.yaml`**

Edit the `- id: CONF-RBP` entry (currently `status: deferred`) to:

```yaml
- id: CONF-RBP
  component: RigidBodyPhysics
  nodes: [CollisionSensor, Contact, CollisionCollection, CollisionSpace]
  clause: "37"
  severity: major
  status: fixed
  commit: <merge-commit-short-hash>
  summary: "CollisionSensor reports collisions: contacts (Contact nodes), intersections, and isActive are emitted from the watched CollisionCollection's collidables (§37.4.4-.4.7)."
  note: "Read-only output bridge (the solver already resolves contacts; §37.4.5 forbids feeding them back). PhysicsBackend.drainContacts pull seam + Jolt ContactListener; ContactReporter builds Contact nodes (geometry from the engine, response fields from the CollisionCollection defaults) and emits over the cascade. Deterministic ordering via stable enrolment ordinals. Deferred refinements: one Contact per body-pair per frame (not per manifold point); space-vs-space broadphase grouping (folded into CONF-RBP-GEOM)."
```

- [ ] **Step 2: Regenerate the conformance view**

Run: `cd <repo-root> && mise run conformance`
Expected: regenerates `docs/conformance/model.json` + `INDEX.md` + `components/RigidBodyPhysics.md` with `CONF-RBP` now fixed.

Run: `mise run conformance-gate`
Expected: PASS (no schema/consistency errors).

- [ ] **Step 3: Update `BACKLOG.md`**

In `docs/superpowers/BACKLOG.md`, change the `CONF-RBP` row from `DEFERRED` to `CLOSED` with the merge commit, mirroring the existing closed-row style (e.g. the `CONF-RBP-FORCES` ✓FIXED convention). Add a one-line deferral note for the two refinements (per-manifold-point contacts; space-vs-space broadphase) pointing at `CONF-RBP-GEOM`.

- [ ] **Step 4: Final full verification**

Run (ungated): `cd <repo-root> && mise run build 2>&1 | tail -15`
Expected: configure + build + full ctest all pass.

Run (gated): `cmake --preset dev -DX3D_CPP_BUILD_PHYSICS=ON && cmake --build --preset dev && ctest --preset dev -j "$(nproc)" --output-on-failure 2>&1 | tail -15`
Expected: all pass, including `x3d_contact_reporter`, `x3d_physics_contact_wiring`, `x3d_physics_system`, `x3d_cli_test`.

- [ ] **Step 5: Commit the docs**

```bash
git add docs/conformance/ docs/superpowers/BACKLOG.md
git commit -m "docs(physics): close CONF-RBP — §37 contact reporting shipped"
```

> NOTE: the auto-memory `memory/x3d-cpp-gen-physics-seam.md` should be updated after merge to move "contact reporting" from *remaining* to *closed* — do this as a memory write, not in this branch's commits.

---

## Self-Review

**Spec coverage:**
- §3a backend seam (`ContactPoint`+`drainContacts`) → Task 1. ✓
- §3b Jolt `ContactListener`+buffer+drain → Task 5. ✓
- §3c `ContactReporter` (resolve/flatten/filter/dedupe/order/populate/emit, isActive transitions, gating) → Tasks 2-3. ✓
- §3d `PhysicsSystem` (CollisionSensor enrol, handle→node map, drain+report) → Task 4. ✓
- §3 response-fields-from-collection + geometry-from-engine population → Task 2 Step 3 + Task 2 happy-path test. ✓
- §6 testing: ungated reporter test (Tasks 2-3), ungated wiring test (Task 4), gated Jolt test + determinism (Task 5), CLI golden (Task 6), seam-purity grep (Tasks 4-5). ✓
- §5 guardrails (one contact/pair/frame; deep CollisionSpace deferred; unmapped collidables counted) → encoded in code + logged in Task 7. ✓
- §7 files touched ↔ plan File Structure. ✓

**Placeholder scan:** No TBD/TODO; every code step shows full code. The node-construction idiom (`*Unchecked` setters + `std::static_pointer_cast<X3DNode>`) and the `X3DNode`-has-no-`enable_shared_from_this` fact are both resolved against the real bindings. The one remaining "NOTE" (Jolt v5.5 manifold API) gives a concrete verification command + a fallback expression rather than deferring the decision. ✓

**Type consistency:** `ResolvedContact`/`ContactReporter`/`ContactPoint` signatures match across Tasks 1-5; `pairKey` is `std::uint64_t` everywhere; `bodyRef_` keyed by `std::pair<WorldHandle,BodyHandle>`; setter/getter names match the verified bindings; `postEvent` field names (`"contacts"`,`"intersections"`,`"isActive"`) match the reflection. ✓
