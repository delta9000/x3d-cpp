# §37 RigidBody inertia + centerOfMass — Design (CONF-RBP-INERTIA, partial)

**Date:** 2026-06-21
**Status:** Approved (brainstorm) → ready for implementation plan
**Partially closes:** `CONF-RBP-INERTIA` (RigidBody mass-fidelity fields ignored). This task
covers **`inertia`** (explicit tensor) and **`centerOfMass`** (COM offset). It narrows
CONF-RBP-INERTIA to its remainder: `massDensityModel` and `finiteRotationAxis`/
`useFiniteRotation` (deferred — see §8).

## 1. Problem

The §37 physics seam derives every body's inertia tensor from its collision shape (Jolt
`EOverrideMassProperties::CalculateInertia`, overriding only mass — `JoltBackend.cpp`
`addBody`) and places the center of mass at the shape centroid. The authoring fields
`RigidBody.inertia` (SFMatrix3f) and `RigidBody.centerOfMass` (SFVec3f) are read by nothing
and silently ignored. For uniform-density primitives shape-derived inertia is physically
correct, so this is a fidelity gap (minor), not a missing capability — but it prevents
modelling bodies with deliberately non-uniform mass distribution (vehicles, a weighted die,
an off-center load), where rotational dynamics depend on the authored tensor.

## 2. Spec grounding (ISO/IEC 19775-1 §37.4.10 RigidBody)

Verified against the local prose mirror (`scripts/spec_rag.py`):

- `SFMatrix3f [in,out] inertia` default `1 0 0  0 1 0  0 0 1`. Prose: "The *inertia* field
  represents a 3x3 inertia tensor matrix." (Mantis 405: fewer than six items →
  implementation-dependent; more than six → first six used. We accept full 3×3 input.)
- `SFVec3f [in,out] centerOfMass` default `0 0 0`.
- "A body is modelled as a collection of shapes that describe mass distribution rather than
  renderable geometry." … "Values provided may be defined in relative proportions rather
  than absolute physical values" (§37.2.5.1) — so a non-identity inertia is an explicit
  authoring choice, in possibly-relative units.
- **No prose rule exists** for distinguishing an author-set value from the default. The
  override-detection heuristic (§3) is therefore a design decision, made explicit here.

## 3. Architecture

Both fields take effect at **body-creation time** in the engine — Jolt bakes the COM into
the shape (`OffsetCenterOfMassShape`) and reads the inertia tensor from
`BodyCreationSettings`. So the work flows through the existing `addBody` seam call, not a
post-hoc setter (a setter cannot cleanly move the COM after the shape is built). Seam purity
is preserved: only runtime types cross the boundary.

### 3a. Seam change (`runtime/physics/PhysicsBackend.hpp`)

Replace `addBody`'s bare `float mass` parameter with a small runtime-typed struct:

```cpp
/**
 * @brief A body's mass properties, in the runtime's own terms.
 * @details mass is always used. overrideInertia=false (default) → the backend
 *          derives the inertia tensor from the collision shape; true → it uses
 *          the provided 3x3 `inertia` verbatim (§37 RigidBody.inertia). A
 *          centerOfMass other than (0,0,0) offsets the body's COM from the shape
 *          centroid (§37 RigidBody.centerOfMass). No engine type appears here.
 */
struct MassProperties {
  float      mass = 1.0f;
  bool       overrideInertia = false;
  SFMatrix3f inertia{};                 // 3x3, used iff overrideInertia
  SFVec3f    centerOfMass{0, 0, 0};     // (0,0,0) → no offset
};
```

`addBody(world, shape, float mass, fixed, …)` becomes
`addBody(world, shape, const MassProperties &mass, fixed, …)`. This is preferred over a
10-argument signature or a separate setter: COM must be known when the shape is built, and
a struct extends cleanly (massDensityModel later is one more field). The change ripples to
every `addBody` implementer/caller — the abstract declaration, `JoltBackend`, the
`FakeContactBackend` in `physics_contact_wiring_test.cpp`, and the direct calls in
`jolt_backend_test.cpp` and `physics_system_test.cpp` — each constructs `MassProperties{m}`
where it previously passed a float. Contained.

### 3b. PhysicsSystem — non-default detection (`runtime/physics/PhysicsSystem.hpp`)

In `attach`'s body loop, build `MassProperties` from the RigidBody node:

- `mass = body->getMass()`.
- `overrideInertia = !isIdentity3x3(body->getInertia())` — a static helper comparing the
  `float matrix[3][3]` elementwise against the identity (`[i][i]==1`, off-diagonal `==0`).
  Exact float comparison against the literal default is correct: the codegen default is
  exactly identity, and an author who writes identity has authored the degenerate case we
  intentionally treat as "derive from shape."
- `centerOfMass = body->getCenterOfMass()` (passed through; the backend treats `(0,0,0)`
  as no offset).

Rationale (spec §2): a non-identity tensor / non-zero COM is unambiguously authored; the
defaults are what we derive. This needs no non-standard marker field.

### 3c. JoltBackend — the mapping (`runtime/physics/jolt/JoltBackend.cpp`)

In `addBody`, after building the base `shapeRef`:

- **COM offset** (do this before `BodyCreationSettings`, while the shape is mutable): if
  `mass.centerOfMass != (0,0,0)`, wrap the shape —
  `OffsetCenterOfMassShapeSettings(Vec3(com.x,com.y,com.z), shapeRef).Create()` — and use
  the wrapped result as the body's shape. Offset = `centerOfMass` directly, since our
  primitives are origin-centered (shape centroid = 0).
- **Inertia override**: if `mass.overrideInertia` (dynamic bodies only), set
  `settings.mOverrideMassProperties = EOverrideMassProperties::MassAndInertiaProvided`,
  `settings.mMassPropertiesOverride.mMass = mass.mass`, and
  `settings.mMassPropertiesOverride.mInertia` = a `Mat44` whose upper-left 3×3 is the
  authored tensor (`m.matrix[r][c]`) with `mInertia(3,3) = 1`. Else keep the current
  `CalculateInertia` path (mass only).
- **Pose readback fix (required by COM):** change `getBodyTransform` from
  `GetCenterOfMassPosition(id)` → `GetPosition(id)` (the body **origin**), so
  `RigidBody.position` reports the authored reference point regardless of COM offset. For
  centered/symmetric bodies origin == COM, so existing golden traces (sim-physics,
  sim-collision, the gated suite) **must not drift** — a verification gate (§6), not an
  assumption.

### 3d. Data flow

```
PhysicsSystem::attach(RigidBodyCollection)
  per RigidBody:
    mass = getMass()
    overrideInertia = !isIdentity3x3(getInertia());  inertia = getInertia()
    centerOfMass = getCenterOfMass()
    backend_->addBody(world, shapeDesc, MassProperties{...}, fixed, pos, ori, lin, ang)

JoltBackend::addBody
  shapeRef = primitive(shapeDesc)
  if centerOfMass != 0:  shapeRef = OffsetCenterOfMassShape(com, shapeRef)
  if overrideInertia:    settings → MassAndInertiaProvided + Mat44(inertia) + mass
  else:                  settings → CalculateInertia + mass
JoltBackend::getBodyTransform → GetPosition (origin), GetRotation
```

## 4. The quadcopter sim

A deterministic open-loop "flight" that both demonstrates and *verifies* the feature.

- **Fixture `tools/x3d-cli/fixtures/sim-quadcopter.x3d`:** a `RigidBodyCollection`
  (`gravity 0 -9.8 0`) with one dynamic `RigidBody` DEF `Quad` — a flat Box collidable
  (the frame), `mass` (e.g. 1.0), an **authored non-uniform `inertia`** (e.g.
  `diag(0.01, 0.02, 0.01)`), constant `forces` = lift slightly above `mass·g`
  (`0  ~10  0` → rises) + constant `torques` about one axis (`0.02 0 0` → continuous roll
  at α = τ/Ixx). Routes `Quad.position → QuadXform.translation` and
  `Quad.orientation → QuadXform.rotation`. Pure forces/torques (no Script) → deterministic.
- **CLI golden:** `x3d sim sim-quadcopter.x3d --fps 60 --ticks 120 --watch Quad.translation
  --watch Quad.rotation --json` (≈2 s — enough for a visible climb + roll; the plan may
  tune the count) → committed `tools/x3d-cli/goldens/sim-quadcopter.trace.json`
  (captured only after the gated test verifies the behavior — golden discipline).

## 5. Testing

- **Ungated** (`runtime/physics/tests/`, links `x3d_cpp::x3d_cpp`, no Jolt): a fake backend
  records the `MassProperties` it receives. Build a scene programmatically and assert the
  detection logic: identity inertia → `overrideInertia == false`; a non-identity tensor →
  `overrideInertia == true` with the tensor passed through; a non-zero `centerOfMass`
  passed through; default COM `(0,0,0)`. (Extend `physics_contact_wiring_test.cpp`'s fake,
  or a new small `inertia_massprops_test.cpp`.)
- **Gated** (extend `physics_system_test.cpp`, `X3D_CPP_BUILD_PHYSICS`):
  1. **Quad rises** — under lift > mg, `position.y` increases.
  2. **Inertia honored** — under constant torque τ about a principal axis, the accumulated
     roll over t matches **α = τ/Ixx** for the authored tensor (within tolerance).
  3. **Differs from derived** — the same scene with default (identity) inertia produces a
     measurably *different* roll rate. This is the honesty check that proves the tensor is
     actually used, not ignored ([[verify-shipped-claims-against-code]]).
  4. **COM honored** — a body given a pure initial spin (no forces) with
     `centerOfMass=(d,0,0)` has its reported **origin** (`GetPosition`) orbit the COM over
     time (position changes), while an identical centered body's origin stays fixed.
  5. **Determinism** — two runs of the quad scene byte/value-identical.
- **CLI golden** — the quad flight trace matches `sim-quadcopter.trace.json`, gated on
  `$PHYSICS==1` in `tools/tests/x3d_cli_test.sh`.
- **Regression gate** — existing goldens (sim-physics, sim-collision) and the full gated +
  ungated ctest suites stay green; the `GetPosition` switch must produce **zero golden
  drift** for centered bodies.
- **Seam purity** — `grep -E 'JPH|Jolt'` over `PhysicsBackend.hpp`, `PhysicsSystem.hpp`,
  `ContactReporter.hpp` shows only doc-comment word mentions.

## 6. Files touched

- `runtime/physics/PhysicsBackend.hpp` — `MassProperties` struct; `addBody` signature.
- `runtime/physics/PhysicsSystem.hpp` — `isIdentity3x3` helper; build `MassProperties` in
  `attach`; pass to `addBody`.
- `runtime/physics/jolt/JoltBackend.{hpp,cpp}` — `addBody` signature; inertia-override +
  COM-offset mapping; `getBodyTransform` origin readback.
- `runtime/physics/tests/jolt_backend_test.cpp`, `physics_system_test.cpp`,
  `physics_contact_wiring_test.cpp` — update `addBody` call sites; add inertia/COM tests.
- New ungated mass-properties detection test (or extend the wiring test).
- `tools/x3d-cli/fixtures/sim-quadcopter.x3d`, `tools/x3d-cli/goldens/sim-quadcopter.trace.json`.
- `tools/tests/x3d_cli_test.sh` — quad golden assertion.
- `CMakeLists.txt` — register any new ungated test.
- `docs/conformance/findings.yaml`, `docs/conformance/components/RigidBodyPhysics.md`,
  `docs/superpowers/BACKLOG.md` — narrow CONF-RBP-INERTIA to its remainder.

## 7. Scope guardrails (YAGNI)

- COM offset assumes origin-centered primitives (true for our Box/Sphere/Cylinder/Cone);
  offset = `centerOfMass` directly. A general shape-centroid subtraction is unneeded now.
- Full 3×3 tensor input only (the Mantis-405 partial-value cases are implementation-defined
  per spec; we accept the codegen-provided 3×3).
- Inertia override applies to dynamic bodies only (fixed/static bodies have no inertia).

## 8. Out of scope (CONF-RBP-INERTIA remainder, stays deferred)

- `massDensityModel` (alternate Sphere/Box/Cone mass-distribution shape).
- `finiteRotationAxis` / `useFiniteRotation` (integrator option for fast spinners).
- All other CONF-RBP* items (compound joints, solver tuning, sleep thresholds, etc.).
