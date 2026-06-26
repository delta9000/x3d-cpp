# §37 Contact Reporting — Design (CONF-RBP closure)

**Date:** 2026-06-21
**Status:** Approved (brainstorm) → ready for implementation plan
**Closes:** `CONF-RBP` (the largest remaining RigidBodyPhysics gap — collision is detected
and resolved but never *reported* to the scene).

## 1. Problem

The Jolt-backed physics seam already detects and resolves collisions (bodies don't
interpenetrate), but nothing surfaces those contacts back into the X3D event graph.
The §37 nodes for reporting — `CollisionSensor`, `Contact`, `CollisionCollection`,
`CollisionSpace` — exist as generated bindings but emit nothing; `RigidBodyCollection`/
`CollisionSensor.collider` is unread. This makes the RigidBodyPhysics component
behaviourally incomplete at support level 2 (the `Contact` node / `CollisionSensor.contacts`
field is L2; `intersections` is L1).

## 2. Spec grounding (ISO/IEC 19775-1 §37)

Verified against the local prose mirror (`scripts/spec_rag.py`):

- **§37.4.5 CollisionSensor** `: X3DSensorNode`
  - `SFNode [in,out] collider NULL [CollisionCollection]`
  - `MFNode [out] intersections [X3DNBodyCollidableNode]` (L1)
  - `MFNode [out] contacts [Contact]` (L2)
  - `SFBool [out] isActive`
  - "The collision-detection system does not require an instance of a CollisionSensor
    node to be present in the scene in order for the physics model to run … used to
    report contact information should the information be required for other purposes."
  - **NOTE (load-bearing):** routing `contacts` → `RigidBodyCollection.set_contacts` is
    *strongly advised against* — "The collision system will have already taken these into
    account internally … Setting the values again … will result in undefined behaviour."
    ⇒ contact reporting is **read-only output**; the response loop is untouched.
  - `isActive == TRUE` when contacts were detected from movement during the last
    presentation frame.

- **§37.4.7 Contact** `: X3DNode` — geometric fields `position`, `contactNormal`,
  `depth`, `body1`/`body2` (`[RigidBody]`), `geometry1`/`geometry2`
  (`[X3DNBodyCollidableNode]`); response-tuning fields `bounce`,
  `frictionCoefficients`, `frictionDirection`, `minBounceSpeed`, `surfaceSpeed`,
  `slipCoefficients`, `softnessConstantForceMix`, `softnessErrorCorrection`,
  `appliedParameters`. The response-tuning fields are *defaults assigned from the
  generating `CollisionCollection`* (§37.4.4: "A set of parameters … specify default
  values that will be assigned to all Contact nodes generated from the CollisionSensor").

- **§37.4.4 CollisionCollection** `: X3DChildNode, X3DBoundedObject` — holds
  `collidables [X3DNBodyCollisionSpaceNode, X3DNBodyCollidableNode]` plus the default
  response params (`bounce`, `frictionCoefficients`, `minBounceSpeed`, `slipFactors`,
  `softnessConstantForceMix`, `softnessErrorCorrection`, `surfaceSpeed`,
  `appliedParameters`). `enabled == FALSE` ⇒ "A CollisionSensor node watching this
  collection does not report any outputs for this collection for this presentation frame."

- **§37.4.6 CollisionSpace** `: X3DNBodyCollisionSpaceNode` — a `collidables` grouping
  for broadphase efficiency; may nest. We flatten it for collidable resolution.

## 3. Architecture

The response solve already happens inside Jolt. Contact reporting is a **read-only
output bridge**: drain what the engine already computed, translate it into `Contact`
nodes, emit on the watching `CollisionSensor`. No change to the integration/response loop.

Three pieces, all **seam-pure** (no engine type — `JPH::` — escapes the backend):

### 3a. `PhysicsBackend` seam (extend `runtime/physics/PhysicsBackend.hpp`)

New backend-neutral struct + one pull method (non-pure default so other/no backends
still compile):

```cpp
/// One contact between two bodies in a world, in the runtime's own terms.
struct ContactPoint {
  BodyHandle bodyA = kInvalidBodyHandle;
  BodyHandle bodyB = kInvalidBodyHandle;
  SFVec3f position{0, 0, 0};   // world-space contact location
  SFVec3f normal{0, 1, 0};     // unit normal, bodyA -> bodyB convention
  float    depth = 0.0f;       // penetration depth along the normal
};

/// Move out all contacts recorded since the previous drain for `world`.
/// Default: no contacts (a backend without contact support stays silent).
virtual void drainContacts(WorldHandle world, std::vector<ContactPoint> &out) {
  (void)world; out.clear();
}
```

**Pull, not push** (decided): the backend buffers contacts during `step()`, the runtime
drains after — deterministic, single point of marshalling, mirrors the existing
`getBodyTransform` readback. A push/callback seam would invert control and risk
worker-thread ordering nondeterminism in golden traces.

### 3b. `JoltBackend` (extend `runtime/physics/jolt/JoltBackend.cpp`)

- Register a `JPH::ContactListener` on each world's `PhysicsSystem`.
- `OnContactAdded` / `OnContactPersisted` push a `ContactPoint` into a per-world
  `std::vector<ContactPoint>` buffer (record both body IDs, a representative world
  contact position, the world-space normal, penetration depth). The buffer is owned by
  the `JoltWorld` struct.
- `drainContacts(world, out)` swaps the buffer out and clears it.
- Single-threaded deterministic job system (the existing default) keeps callback order
  reproducible. The reporter also sorts (3c) so ordering never depends on the engine.

### 3c. `ContactReporter` (new header `runtime/physics/ContactReporter.hpp`)

A focused, backend-agnostic translator owned by `PhysicsSystem` (kept *out* of the
already-large `PhysicsSystem.hpp`; unit-testable with no backend). It names no engine
type — it operates on `ContactPoint` structs and generated nodes only.

Inputs fed by `PhysicsSystem`:
- A handle → `{RigidBody*, CollidableShape*}` map (the shape `shapeForBody` picked).
- The set of enrolled `CollisionSensor*`.

Per-frame `report(const std::vector<ContactPoint>&, X3DExecutionContext&)`, for each
enrolled sensor:

1. **Gate.** Skip if `sensor->getEnabled() == FALSE`, `collider == NULL`, or the
   `CollisionCollection.enabled == FALSE`. On a gate that silences a previously-active
   sensor, emit `isActive = FALSE` (change-tracked).
2. **Resolve collidables.** Flatten `collider → CollisionCollection.collidables` into a
   `set<CollidableShape*>`, recursively descending nested `CollisionSpace.collidables`
   with a visited-guard. (Satisfies the "flat collidables" scope; space-vs-space
   broadphase grouping is deferred — see §5.)
3. **Filter.** Keep only `ContactPoint`s where *both* bodies' `CollidableShape`s are in
   that set (resolved via the handle map).
4. **Dedupe + order.** Collapse to one `Contact` per unordered body-pair per frame
   (keep max `depth`); sort by `(min(handleA,handleB), max(...))` → deterministic for
   golden traces.
5. **Build `Contact` nodes.** `make_shared<Contact>()`, populate:
   - geometric (from the engine): `position`, `contactNormal`, `depth`,
     `body1`/`body2` (the `RigidBody` SFNodes), `geometry1`/`geometry2` (the
     `CollidableShape` SFNodes).
   - response defaults (copied from the watched `CollisionCollection`): `bounce`,
     `frictionCoefficients`, `minBounceSpeed`, `softnessConstantForceMix`,
     `softnessErrorCorrection`, `surfaceSpeed`, `slipFactors → slipCoefficients`,
     `appliedParameters`.
   - `frictionDirection` left at default (the system normally computes it unless
     `USER_FRICTION` is set — spec-correct to omit).
6. **Emit** via the existing cascade so author ROUTEs fan out automatically:
   - `postEvent(sensor, "contacts", MFNode{contacts...})`
   - `postEvent(sensor, "intersections", MFNode{distinct CollidableShapes...})`
   - `postEvent(sensor, "isActive", bool)` — emitted on change (per-sensor prev state).

### 3d. `PhysicsSystem` (extend `runtime/physics/PhysicsSystem.hpp`)

- `attach`: in addition to `RigidBodyCollection`, accept `CollisionSensor` nodes →
  register them with the owned `ContactReporter`. (Every scene node already flows
  through `attach` via `forEachNode` in `X3DSceneBridge.hpp`, so no new enrollment loop.)
- `attach`'s body loop: capture the `CollidableShape*` that `shapeForBody` selected so
  the handle → `{RigidBody*, CollidableShape*}` map can be built (small refactor of
  `shapeForBody` to also yield the chosen `CollidableShape*`).
- `update`: after the substep loop and pose readback, `backend_->drainContacts(world, buf)`
  for each world, then `reporter_.report(buf, ctx)`. No new System class — the
  body↔handle↔node maps stay in one owner.

Contacts accumulate across the frame's substeps in the Jolt buffer and are drained once
after substepping, then deduped per body-pair — matching the spec's "contacts generated
… last presentation frame" (one Contact per colliding pair per frame).

## 4. Data flow

```
PhysicsSystem::attach(RigidBodyCollection)
  └─ per body: addBody; record handle -> {RigidBody*, CollidableShape*}   (NEW capture)
PhysicsSystem::attach(CollisionSensor)                                    (NEW branch)
  └─ reporter_.addSensor(sensor)

PhysicsSystem::update(now)
  ├─ substep loop (unchanged) — Jolt ContactListener buffers ContactPoints
  ├─ getBodyTransform -> postEvent position/orientation                   (unchanged)
  └─ for each world: drainContacts(world, buf)
        reporter_.report(buf, ctx)
          -> postEvent(sensor, "contacts"|"intersections"|"isActive")
```

## 5. Scope guardrails (YAGNI — logged to BACKLOG)

- **One Contact per body-pair per frame**, not per manifold point. Sufficient for §37
  reporting; per-point reporting deferred.
- **`frictionDirection`** omitted unless `USER_FRICTION` (system-computed by spec).
- **Unmapped collidables**: a `CollidableShape` in a collection that doesn't correspond
  to an enrolled body is skipped (well-formed scenes USE-share the same `CollidableShape`
  with `RigidBody.geometry`). Logged, not silent.
- **Deep `CollisionSpace` broadphase grouping** (space-vs-space culling) deferred; the
  recursive flatten covers correctness. Tracked as a refinement of `CONF-RBP-GEOM`.

## 6. Testing (TDD)

- **`x3d_contact_reporter`** (ungated, new): pure-logic test of `ContactReporter` —
  synthetic `ContactPoint`s + a hand-built `CollisionSensor`/`CollisionCollection`/
  `CollidableShape`/`RigidBody` scene + stub handle map. Asserts: `Contact` field
  population (geometric + collection-default response fields), `intersections` set,
  `isActive` true/false transitions, per-pair dedupe + deterministic ordering,
  `enabled=FALSE` (sensor and collection) silencing, `collider=NULL` no-op.
- **`x3d_physics_system`** (gated by `X3D_CPP_BUILD_PHYSICS`, extend): two boxes
  colliding under gravity → a `Contact` emitted with a plausible upward normal and a
  contact position between them; `isActive == TRUE`.
- **`x3d sim` golden trace**: a collision scene with a `CollisionSensor` → byte-identical
  golden output (determinism proof).
- **Seam-purity grep** stays green: `ContactReporter.hpp` and `PhysicsSystem.hpp` name
  no `JPH::`/`Jolt` symbol.

## 7. Files touched

- `runtime/physics/PhysicsBackend.hpp` — `ContactPoint` + `drainContacts`.
- `runtime/physics/jolt/JoltBackend.cpp` (+`.hpp` impl) — `ContactListener`, per-world
  buffer, `drainContacts`.
- `runtime/physics/ContactReporter.hpp` — new translator.
- `runtime/physics/PhysicsSystem.hpp` — `CollisionSensor` enrol, handle→node map, drain
  + report call.
- `runtime/physics/tests/contact_reporter_test.cpp` — new ungated test.
- `runtime/physics/tests/physics_system_test.cpp` — extend.
- `CMakeLists.txt` — register `x3d_contact_reporter` test; wire reporter test (ungated).
- golden sim fixture + expected trace under the sim golden dir.
- `docs/superpowers/BACKLOG.md` + `docs/conformance/components/RigidBodyPhysics.md` —
  mark `CONF-RBP` closed (commit), log the deferred refinements (per-point manifold,
  space-vs-space grouping).

## 8. Out of scope

CONF-RBP-JOINT-OUT (joint output events), CONF-RBP-SOLVER (solver-tuning fields),
CONF-RBP-INERTIA, CONF-RBP-CAP, deep CollisionSpace broadphase grouping. Unchanged.
