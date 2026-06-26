# RigidBodyPhysics — conformance

_Generated. Levels 1,2 · 14 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| BallJoint | 2 | ✓ | — | — | CONF-RBP-JOINT-OUT | X3DRigidJointNode |
| CollidableOffset | 1 | ✓ | — | — | CONF-RBP-GEOM | X3DBoundedObject, X3DChildNode, X3DNBodyCollidableNode |
| CollidableShape | 1 | ✓ | — | — | CONF-RBP-GEOM | X3DBoundedObject, X3DChildNode, X3DNBodyCollidableNode |
| CollisionCollection | 1 | ✓ | — | — | CONF-RBP, CONF-RBP-FRICTION-BOUNCE | X3DBoundedObject, X3DChildNode |
| CollisionSensor | 1 | ✓ | — | ◑ | CONF-NAV-COLLISION, CONF-RBP | X3DChildNode, X3DSensorNode |
| CollisionSpace | 1 | ✓ | — | — | CONF-RBP | X3DBoundedObject, X3DNBodyCollisionSpaceNode |
| Contact | 2 | ✓ | — | — | CONF-RBP |  |
| DoubleAxisHingeJoint | 2 | ✓ | — | — | CONF-RBP-JOINTS | X3DRigidJointNode |
| MotorJoint | 2 | ✓ | — | — | CONF-RBP-JOINTS | X3DRigidJointNode |
| RigidBody | 2 | ✓ | — | — | CONF-RBP-DAMP, CONF-RBP-ENABLED, CONF-RBP-FORCES, CONF-RBP-INERTIA | X3DBoundedObject, X3DChildNode |
| RigidBodyCollection | 2 | ✓ | — | — | CONF-RBP-CAP, CONF-RBP-DETERMINISM, CONF-RBP-SOLVER | X3DBoundedObject, X3DChildNode |
| SingleAxisHingeJoint | 2 | ✓ | — | — | CONF-RBP-HINGE-LIMITS, CONF-RBP-JOINT-OUT | X3DRigidJointNode |
| SliderJoint | 2 | ✓ | — | — | CONF-RBP-JOINT-OUT | X3DRigidJointNode |
| UniversalJoint | 2 | ✓ | — | — | CONF-RBP-JOINTS | X3DRigidJointNode |

## Findings

- **CONF-RBP-JOINTS** [major/DEFERRED] — §37: Compound joints are unsupported (only BallJoint/SingleAxisHingeJoint/SliderJoint map to constraints); DoubleAxisHingeJoint/MotorJoint/UniversalJoint are skipped with a note.
  - The seam (ConstraintDesc + addConstraint) is extensible — add a Kind per joint. No motor drive on the hinge either.
- **CONF-RBP-GEOM** [minor/DEFERRED] — §37: Box/Sphere/Cylinder (exact) + Cone (analytic AABB) collidables are supported; non-analytic meshes (IndexedFaceSet etc.) still exclude the body. CollidableOffset is unhandled; only the first CollidableShape per body is used (no compound).
  - As of 68eb39b Cylinder/Cone enroll; the mesh exclusion is OBSERVABLE via droppedBodyCount() (was silent). Remaining closure = a mesh/convex-hull shape path + CollidableOffset transform + compound shapes.
- **CONF-RBP-JOINT-OUT** [minor/DEFERRED] — §37: Joint output events are never produced (angle/separation/*Rate/body*AnchorPoint *_changed), and joint sub-fields stopBounce/stopErrorCorrection are ignored.
  - Constraints drive motion but report nothing back to the scene. Needs a per-tick joint-state readback mirroring the body pose readback.
- **CONF-RBP-INERTIA** [minor/DEFERRED] — §37: inertia (explicit tensor) + centerOfMass are now HONORED (Jolt MassAndInertiaProvided + OffsetCenterOfMassShape; non-default detection). Remaining: massDensityModel, finiteRotationAxis/useFiniteRotation.
  - inertia/centerOfMass shipped 3a9595c: non-identity inertia → authored 3x3 tensor; non-zero centerOfMass → COM-offset shape wrap; pose readback uses the body origin (GetPosition) so RigidBody.position tracks the authored reference point. Verified by the quadcopter sim (authored inertia governs roll, differs from shape-derived) + a COM-orbit test. Still derived/ignored: massDensityModel (alt mass shape), finiteRotation (integrator option).
- **CONF-RBP-SOLVER** [minor/DEFERRED] — §37: RigidBodyCollection solver-tuning fields are ignored: iterations, errorCorrection, constantForceMix, contactSurfaceThickness, maxCorrectionSpeed, preferAccuracy, autoDisable.
  - All solver behavior is the Jolt default (single collision step per update). gravity and enabled ARE read.
- **CONF-RBP-DAMP** [low/DEFERRED] — §37: Sleeping/auto-disable thresholds are ignored: autoDisable/disableTime/disableLinearSpeed/disableAngularSpeed have no effect. (autoDamp damping IS now applied — 2e9d8f3.)
  - autoDamp + linear/angularDampingFactor ship (force = -factor·velocity, §37.4.10). The per-body sleep thresholds remain — Jolt's sleep thresholds are global PhysicsSettings, not per-body, so a faithful mapping needs more than a flag.
- **CONF-RBP-CAP** [low/DEFERRED] — §37: 1024 body/pair/contact cap per world (overflow now WARNS, no longer a silent drop — 97d8a5c); raising the value needs a larger temp allocator. A single collision substep per Update can still let fast bodies tunnel.
  - Co-size the TempAllocator with the cap to raise it; raise the collision substep count for high-velocity scenes.
- **CONF-RBP** [major/FIXED `bde5134`] — §37: CollisionSensor reports collisions: contacts (Contact nodes), intersections, and isActive are emitted from the watched CollisionCollection's collidables (§37.4.4-.4.7).
  - Read-only output bridge (the solver already resolves contacts; §37.4.5 forbids feeding them back). PhysicsBackend.drainContacts pull seam + Jolt ContactListener; ContactReporter builds Contact nodes (geometry from the engine, response fields from the CollisionCollection defaults) and emits over the cascade. Deterministic ordering via stable enrolment ordinals. Deferred refinements: one Contact per body-pair per frame (not per manifold point); space-vs-space broadphase grouping (folded into CONF-RBP-GEOM).
- **CONF-RBP-FORCES** [major/FIXED `9560686`] — §37: RigidBody.forces/.torques are applied every frame (§37.4.10 continuous-force model) via the new seam method applyForce.
  - PhysicsSystem sums forces+torques and applies the resultant before each substep. TDD — zero-g body under 10 N reaches y=4.997 after 1 s (analytic 5).
- **CONF-RBP-ENABLED** [major/FIXED `75a7878`] — §37: RigidBody.enabled was ignored — a body with enabled=FALSE was still simulated. Now excluded from the calculations per §37.
  - Fixed under TDD in x3d_physics_system (disabled body not enrolled, does not fall).
- **CONF-RBP-HINGE-LIMITS** [minor/FIXED] — §37: SingleAxisHingeJoint minAngle/maxAngle are honored with the correct sign. §37 limits are measured from the initial pose (valid range minAngle<=0<=maxAngle, <=pi each way) — exactly Jolt's hinge convention. Spec-valid ranges, including asymmetric ones, are honored; out-of-domain/malformed limits are clamped to the valid domain + warned.
  - Jolt's hinge angle is the NEGATIVE of the body's rotation about the axis, so the authored limits are negated before being handed to Jolt ([lo,hi] -> [-hi,-lo]); a TDD asymmetric-limit test caught a latent sign bug that symmetric ranges (the default -pi..pi and the locked 0,0) had hidden. A zero-width (locked) range is widened by a 1e-4 gap (Jolt asserts on min==max). Out-of-domain input (initial pose outside the limits, |angle|>pi) is malformed per §37 and clamped + warned. stopBounce / stopErrorCorrection (limit softness/overshoot) remain deferred under CONF-RBP-SOLVER.
- **CONF-RBP-FRICTION-BOUNCE** [minor/CLOSED `e038494`] — §37: CollisionCollection.frictionCoefficients (scalar, .x) and bounce are applied as per-contact combined friction/restitution via the Jolt ContactListener, now HONORING appliedParameters: FRICTION_COEFFICIENT-2 gates frictionCoefficients, BOUNCE gates bounce (default ["BOUNCE"]). Demonstrated by a rear-wheel-drive vehicle that drives under tyre friction + a rebounding ball.
  - PhysicsSystem reads RigidBodyCollection.collider -> CollisionCollection.appliedParameters to gate each param (a selected param overrides the engine default even at value 0; an unselected param leaves the engine default untouched). Scalar friction uses .x (anisotropic .y deferred). Deferred: USER_FRICTION (frictionDirection), anisotropic friction (.y), ERROR_REDUCTION/CONSTANT_FORCE/SPEED-1/2/SLIP-1/2 selectors, slipFactors, surfaceSpeed, minBounceSpeed, softness/solver tuning (CONF-RBP-SOLVER).
- **CONF-RBP-DETERMINISM** [low/FIXED `97d8a5c`] — §37: Determinism is now host-INDEPENDENT — JoltBackend defaults to a single-threaded deterministic job system (identical results regardless of CPU core count).
  - A JoltBackend(workerThreads>0) opts into a thread pool for large scenes (then deterministic only for a fixed count). Single-body sim golden unchanged.

