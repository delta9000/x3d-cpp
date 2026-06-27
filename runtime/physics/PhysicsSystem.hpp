// PhysicsSystem.hpp
// The System that simulates X3D §37 RigidBodyPhysics dynamics: it reads each
// RigidBodyCollection / RigidBody / CollidableShape, drives an (abstract)
// PhysicsBackend, and writes each body's integrated pose back onto its RigidBody
// node via ctx.writeField — so position_changed / orientation_changed flow over
// the author's existing ROUTEs to the scene's Transforms. No new event
// mechanism: the standard cascade carries the motion.
//
// ENGINE-AGNOSTIC: this layer names NO physics-engine type. It depends only on
// the abstract PhysicsBackend seam (constructed with a shared_ptr; inert when
// none, exactly like ScriptSystem without a backend) and the generated §37
// nodes. A seam-purity check greps this header for Jolt/JPH leakage: there is
// none.
//
// MAPPING (per the design):
//   RigidBodyCollection -> createWorld(gravity)
//   each RigidBody in .bodies:
//     read its CollidableShape geometry (.geometry -> CollidableShape -> .shape
//       -> Shape -> .geometry -> Box | Sphere); Box -> half-extents = size/2;
//       Sphere -> radius; anything else -> skip the body + a one-line stderr note
//     read mass / position / orientation / linearVelocity / angularVelocity /
//       fixed -> addBody(...); remember RigidBody* <-> BodyHandle.
//   update(now): dt = now - lastNow (clamped); step(dt); for each mapped body
//     getBodyTransform -> writeField(rigidBody, "position"|"orientation", ...).
#ifndef X3D_RUNTIME_PHYSICS_SYSTEM_HPP
#define X3D_RUNTIME_PHYSICS_SYSTEM_HPP

#include "PhysicsBackend.hpp"

#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"

#include "x3d/nodes/BallJoint.hpp"
#include "x3d/nodes/Box.hpp"
#include "x3d/nodes/CollidableShape.hpp"
#include "x3d/nodes/Cone.hpp"
#include "x3d/nodes/Cylinder.hpp"
#include "x3d/nodes/RigidBody.hpp"
#include "x3d/nodes/RigidBodyCollection.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/SingleAxisHingeJoint.hpp"
#include "x3d/nodes/CollisionSensor.hpp"
#include "ContactReporter.hpp"
#include "x3d/nodes/SliderJoint.hpp"
#include "x3d/nodes/Sphere.hpp"
#include "x3d/nodes/X3DRigidJointNode.hpp"

#include <algorithm>
#include <any>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace x3d::runtime {

using namespace x3d::core;
namespace xn = x3d::nodes;

/**
 * @brief Drives §37 rigid-body dynamics through an abstract PhysicsBackend.
 * @details A time-driven System: attach() walks each RigidBodyCollection into a
 *          backend world + bodies, update() steps the worlds and writes each
 *          body's pose back onto its RigidBody node. Inert (a clean no-op) when
 *          constructed without a backend, so the System can always be attached.
 */
class PhysicsSystem : public System {
public:
  /**
   * @brief Construct with the backend engine (null -> inert).
   * @param backend The physics solver implementation. Shared so a test can hold
   *        it and so the System retains it for the worlds' lifetime.
   */
  explicit PhysicsSystem(std::shared_ptr<PhysicsBackend> backend)
      : backend_(std::move(backend)) {}

  // --------------------------------------------------------------------------
  // System interface.
  // --------------------------------------------------------------------------

  /**
   * @brief Enroll one RigidBodyCollection: create its world + all its bodies.
   * @details A no-op for any other node type, and inert without a backend. Reads
   *          gravity off the collection, then each RigidBody in .bodies: maps its
   *          CollidableShape geometry to a ShapeDesc (Box/Sphere; else skips with
   *          a stderr note) and reads its mass/pose/velocity/fixed into addBody.
   */
  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    (void)ctx;
    if (!backend_) return;
    if (auto *sensor = dynamic_cast<xn::CollisionSensor *>(node)) {
      reporter_.addSensor(sensor);
      return;
    }
    auto *collection = dynamic_cast<xn::RigidBodyCollection *>(node);
    if (!collection) return;
    if (!collection->getEnabled()) return;

    WorldHandle world = backend_->createWorld(collection->getGravity());
    if (world == kInvalidWorldHandle) return;

    // RigidBody* -> BodyHandle, for resolving joints' body1/body2 below. Scoped
    // to this collection (each world owns its own bodies + joints).
    std::unordered_map<const xn::RigidBody *, BodyHandle> handleFor;

    for (const auto &bodyNode : collection->getBodies()) {
      auto *body = dynamic_cast<xn::RigidBody *>(bodyNode.get());
      if (!body) continue;

      // §37: a RigidBody with enabled=FALSE "is not part of the calculations."
      // Exclude it from the simulation entirely (it keeps its authored pose).
      if (!body->getEnabled()) continue;

      ShapeDesc shape;
      SFNode collidableShape;
      if (!shapeForBody(*body, shape, &collidableShape)) {
        // Unsupported collidable geometry (not Box/Sphere): the body is excluded.
        // Count it so the exclusion is OBSERVABLE (droppedBodyCount), not a silent
        // data loss — non-primitive collidable shapes are a tracked §37 gap.
        ++droppedBodies_;
        std::cerr << "[PhysicsSystem] RigidBody has no supported collidable "
                     "shape (Box/Sphere/Cylinder/Cone); excluding body from "
                     "simulation (droppedBodyCount="
                  << droppedBodies_ << ")\n";
        continue;
      }

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
      if (handle == kInvalidBodyHandle) continue;
      // §37 useGlobalGravity: FALSE -> the body ignores the collection's gravity.
      backend_->setGravityFactor(world, handle,
                                 body->getUseGlobalGravity() ? 1.0f : 0.0f);
      bodies_.push_back(Mapped{body, world, handle});
      handleFor.emplace(body, handle);
      // Record the resolution so drained contacts (world,handle) map back to the
      // scene nodes, with a stable ordinal for deterministic contact ordering.
      bodyRef_[{world, handle}] =
          BodyRef{bodyNode, collidableShape, nextOrdinal_++};
    }

    // Joints: every body of this collection now exists (constraints require it),
    // so resolve each §37 RigidJoint into a ConstraintDesc and add it.
    attachJoints(*collection, world, handleFor);

    // §37 contact-response params: appliedParameters selects which response
    // params the solver applies — FRICTION_COEFFICIENT-2 gates
    // frictionCoefficients, BOUNCE gates bounce (default ["BOUNCE"]).
    // A selected param overrides the engine default even when its value is 0
    // (an explicit author choice); an unselected param stays -1 (engine default
    // left untouched). Scalar friction uses .x; anisotropic .y is deferred
    // (CONF-RBP-SOLVER).
    float contactFriction = -1.0f, contactRestitution = -1.0f;
    if (auto *cc =
            dynamic_cast<xn::CollisionCollection *>(collection->getCollider().get())) {
      const auto ap = cc->getAppliedParameters();
      const bool useFriction =
          std::find(ap.begin(), ap.end(),
                    AppliedParametersChoices::FRICTION_COEFFICIENT_2) != ap.end();
      const bool useBounce = std::find(ap.begin(), ap.end(),
                                       AppliedParametersChoices::BOUNCE) != ap.end();
      if (useFriction) contactFriction = cc->getFrictionCoefficients().x;
      if (useBounce) contactRestitution = cc->getBounce();
    }
    backend_->setContactResponse(world, contactFriction, contactRestitution);
  }

  /**
   * @brief Step every world to `now` and write each body's pose back.
   * @details dt = now - lastNow, clamped to kMaxStep so a huge first frame (or a
   *          paused/long gap) can't blow the integrator up. The first update for
   *          a world produces no step (no prior time). For each mapped body the
   *          integrated pose is read back and posted as an event on the RigidBody
   *          node via ctx.postEvent — which both writes the node's own
   *          position/orientation field (last-writer-wins seed) AND fans out along
   *          the author's ROUTEs (position_changed / orientation_changed flow to
   *          the Transforms). This is the same mechanism every other System uses
   *          (InterpolatorSystem, KeyDeviceSensorSystem); writeField alone would
   *          update the node + dirty tracker but NOT fire the routes.
   */
  void update(double now, X3DExecutionContext &ctx) override {
    if (!backend_ || bodies_.empty()) return;

    // Guard the step to fire once per timestamp: the context re-invokes update()
    // on every cascade do-while pass (§4.4.8.3 step 4); stepping the integrator
    // N times per tick would desynchronize physics from wall-clock and break
    // determinism. Only the first invocation for a given `now` steps + writes.
    if (haveStepped_ && now == steppedAt_) return;

    double dt = 0.0;
    if (haveLast_) dt = now - lastNow_;
    haveLast_ = true;
    lastNow_ = now;
    haveStepped_ = true;
    steppedAt_ = now;
    if (dt <= 0.0) return;  // no time elapsed yet (first frame) -> nothing to do
    // Bound a huge first/paused frame so the accumulator can't explode.
    if (dt > kMaxFrame) dt = kMaxFrame;

    // Fixed-timestep substepping: integrate the elapsed dt in kFixedStep chunks
    // (plus the leftover from prior frames). This makes the result independent of
    // the caller's fps and keeps it deterministic — the integrator always sees
    // the same fixed dt regardless of frame cadence. Each distinct world is
    // stepped the same number of substeps.
    accumulator_ += dt;
    int substeps = 0;
    while (accumulator_ >= kFixedStep && substeps < kMaxSubsteps) {
      // §37: forces/torques are applied EVERY frame (continuously, until the
      // author resets them). Re-apply before each substep — engines clear
      // accumulated forces after each integration.
      for (const auto &m : bodies_) applyBodyForces(m);
      WorldHandle lastWorld = kInvalidWorldHandle;
      for (const auto &m : bodies_) {
        if (m.world != lastWorld) {
          backend_->step(m.world, kFixedStep);
          lastWorld = m.world;
        }
      }
      accumulator_ -= kFixedStep;
      ++substeps;
    }
    if (substeps == 0) return;  // not enough time accumulated to take a step yet

    // Write back each body's integrated pose -> emits *_changed over the routes.
    // postEvent seeds the cascade: it writes the RigidBody's own position/
    // orientation field AND fans out along the author ROUTEs to the Transforms.
    for (const auto &m : bodies_) {
      SFVec3f pos{0, 0, 0};
      SFRotation ori{0, 0, 1, 0};
      backend_->getBodyTransform(m.world, m.handle, pos, ori);
      ctx.postEvent(m.body, "position", std::any(pos));
      ctx.postEvent(m.body, "orientation", std::any(ori));
    }

    // §37 contact reporting: drain each world's contacts, resolve handles to the
    // scene's RigidBody/CollidableShape nodes (with a stable pairKey), and let
    // the reporter emit CollisionSensor.contacts/intersections/isActive. The
    // response solve already happened inside the backend — this is read-only.
    std::vector<ResolvedContact> resolved;
    std::vector<ContactPoint> buf;
    // Precondition: bodies_ is grouped by world — attach() pushes all of a
    // collection's bodies contiguously and each collection gets its own world
    // (createWorld per RigidBodyCollection), so a simple lastDrained sentinel
    // drains each distinct world exactly once.
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
        // body1/geometry1 follow the engine's manifold body-A ordering (not the
        // pairKey ordinal order); pairKey below canonicalizes dedupe/emission
        // order, but which scene body lands in Contact.body1 vs body2 is
        // engine-determined (and reproducible for a fixed deterministic sim).
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
  }

  /** @brief Number of bodies enrolled in simulation (for tests). */
  std::size_t bodyCount() const { return bodies_.size(); }

  /**
   * @brief Number of RigidBodies EXCLUDED from the simulation because their
   *        collidable geometry was not a supported primitive (Box/Sphere).
   * @details Makes the "unsupported shape -> skip the body" path observable
   *          instead of a silent data loss: a consumer (or test) can detect that
   *          bodies were dropped. See shapeForBody().
   */
  std::size_t droppedBodyCount() const { return droppedBodies_; }

  /**
   * @brief Number of drained contacts skipped because a contact body had no
   *        enrolled RigidBody/CollidableShape (e.g. a collidable not tied to a
   *        simulated body). Makes the skip OBSERVABLE rather than silent
   *        (mirrors droppedBodyCount()); a consumer or test can detect it.
   */
  std::size_t unmappedContactCount() const { return unmappedContacts_; }

private:
  /// Per-enrolled-body mapping: the RigidBody node and its backend handles.
  struct Mapped {
    xn::RigidBody *body;
    WorldHandle world;
    BodyHandle handle;
  };

  /// The fixed integrator timestep (deterministic, fps-independent).
  static constexpr double kFixedStep = 1.0 / 60.0;
  /// Cap on one frame's elapsed dt so a long/paused frame can't explode the
  /// accumulator (it would otherwise queue a huge burst of substeps).
  static constexpr double kMaxFrame = 0.25;
  /// Hard cap on substeps per frame (belt-and-braces with kMaxFrame).
  static constexpr int kMaxSubsteps = 16;

  /**
   * @brief Apply a body's continuous §37 forces/torques for the next step.
   * @details Sums RigidBody.forces and .torques (each a set of MFVec3f vectors,
   *          §37.4.10) and hands the resultant to the backend. Called once per
   *          substep so the force is applied every frame (the spec's continuous-
   *          force model). A body with no forces/torques is a cheap no-op.
   */
  void applyBodyForces(const Mapped &m) {
    SFVec3f force{0, 0, 0};
    for (const SFVec3f &f : m.body->getForces()) {
      force.x += f.x;
      force.y += f.y;
      force.z += f.z;
    }
    SFVec3f torque{0, 0, 0};
    for (const SFVec3f &t : m.body->getTorques()) {
      torque.x += t.x;
      torque.y += t.y;
      torque.z += t.z;
    }

    // §37 autoDamp: apply a force/torque opposing the previous frame's motion,
    // scaled by linear/angularDampingFactor (the spec's worked example is
    // force = dampingFactor × velocity × -1).
    if (m.body->getAutoDamp()) {
      SFVec3f lin{0, 0, 0}, ang{0, 0, 0};
      backend_->getBodyVelocity(m.world, m.handle, lin, ang);
      const float kl = m.body->getLinearDampingFactor();
      const float ka = m.body->getAngularDampingFactor();
      force.x -= kl * lin.x;
      force.y -= kl * lin.y;
      force.z -= kl * lin.z;
      torque.x -= ka * ang.x;
      torque.y -= ka * ang.y;
      torque.z -= ka * ang.z;
    }
    backend_->applyForce(m.world, m.handle, force, torque);
  }

  /// True iff a 3x3 matrix equals the identity (the §37 inertia default). Exact
  /// compare: the codegen default is exactly identity; an author who writes
  /// identity has authored the degenerate case we treat as "derive from shape".
  static bool isIdentity3x3(const SFMatrix3f &m) {
    for (int r = 0; r < 3; ++r)
      for (int c = 0; c < 3; ++c)
        if (m.matrix[r][c] != (r == c ? 1.0f : 0.0f)) return false;
    return true;
  }

  /**
   * @brief Resolve a RigidBody's collidable primitive into a ShapeDesc.
   * @details Walks the first usable CollidableShape in .geometry -> its .shape
   *          (a Shape) -> Shape.geometry (a Box or Sphere). Box -> half-extents
   *          from size/2; Sphere -> radius. Returns false if no Box/Sphere is
   *          found (the caller skips + notes it).
   */
  static bool shapeForBody(const xn::RigidBody &body, ShapeDesc &out,
                           SFNode *outShape = nullptr) {
    for (const auto &geomNode : body.getGeometry()) {
      auto *collidable = dynamic_cast<xn::CollidableShape *>(geomNode.get());
      if (!collidable) continue;
      auto *shapeNode = dynamic_cast<xn::Shape *>(collidable->getShape().get());
      if (!shapeNode) continue;
      X3DNode *geom = shapeNode->getGeometry().get();
      if (auto *box = dynamic_cast<xn::Box *>(geom)) {
        const SFVec3f s = box->getSize();
        out = ShapeDesc::box(SFVec3f{s.x * 0.5f, s.y * 0.5f, s.z * 0.5f});
        if (outShape) *outShape = geomNode;
        return true;
      }
      if (auto *sphere = dynamic_cast<xn::Sphere *>(geom)) {
        out = ShapeDesc::sphere(sphere->getRadius());
        if (outShape) *outShape = geomNode;
        return true;
      }
      if (auto *cyl = dynamic_cast<xn::Cylinder *>(geom)) {
        out = ShapeDesc::cylinder(cyl->getRadius(), cyl->getHeight() * 0.5f);
        if (outShape) *outShape = geomNode;
        return true;
      }
      if (auto *cone = dynamic_cast<xn::Cone *>(geom)) {
        // Jolt has no cone primitive; use the cone's analytic AABB as a Box
        // collider (bottomRadius × height × bottomRadius). Approximate but keeps
        // the body in the simulation instead of dropping it.
        const float r = cone->getBottomRadius();
        out = ShapeDesc::box(SFVec3f{r, cone->getHeight() * 0.5f, r});
        if (outShape) *outShape = geomNode;
        return true;
      }
    }
    return false;
  }

  /**
   * @brief Resolve a joint's body1/body2 SFNode into a BodyHandle.
   * @details Unset (nullptr) or a body not enrolled in this world -> the
   *          world-anchor sentinel (kInvalidBodyHandle), which the backend binds
   *          to the world (a fixed anchor — the pendulum case).
   */
  static BodyHandle resolveBody(
      const SFNode &node,
      const std::unordered_map<const xn::RigidBody *, BodyHandle> &handleFor) {
    if (!node) return kInvalidBodyHandle;
    auto *rb = dynamic_cast<const xn::RigidBody *>(node.get());
    if (!rb) return kInvalidBodyHandle;
    auto it = handleFor.find(rb);
    return it == handleFor.end() ? kInvalidBodyHandle : it->second;
  }

  /**
   * @brief Read a collection's joints into ConstraintDescs and add them.
   * @details Iterates RigidBodyCollection.joints; branches on nodeTypeName:
   *            BallJoint            -> Kind::Ball   (anchorPoint).
   *            SingleAxisHingeJoint -> Kind::Hinge  (anchorPoint, axis, min/max).
   *            SliderJoint          -> Kind::Slider (axis, min/max separation;
   *                                    anchorPoint defaults to the body1 base —
   *                                    SliderJoint has no anchorPoint field, so
   *                                    the constraint point rides body1's pose).
   *          body1/body2 resolve via the body map; an unset body binds the other
   *          to the world. Unsupported joint types are skipped + noted on stderr.
   */
  void attachJoints(
      const xn::RigidBodyCollection &collection, WorldHandle world,
      const std::unordered_map<const xn::RigidBody *, BodyHandle> &handleFor) {
    for (const auto &jointNode : collection.getJoints()) {
      auto *joint = dynamic_cast<xn::X3DRigidJointNode *>(jointNode.get());
      if (!joint) continue;

      BodyHandle a = resolveBody(joint->getBody1(), handleFor);
      BodyHandle b = resolveBody(joint->getBody2(), handleFor);
      // bodyA must be a real body (the constrained one). If body1 is unset/not
      // found but body2 is real, swap so bodyA is the real body bound to world.
      if (a == kInvalidBodyHandle && b != kInvalidBodyHandle) std::swap(a, b);
      if (a == kInvalidBodyHandle) {
        std::cerr << "[PhysicsSystem] joint has no resolvable body; skipping\n";
        continue;
      }

      ConstraintDesc desc;
      desc.bodyA = a;
      desc.bodyB = b;

      const std::string &kind = joint->nodeTypeName();
      if (kind == "BallJoint") {
        auto *bj = dynamic_cast<xn::BallJoint *>(joint);
        if (!bj) continue;
        desc.kind = ConstraintDesc::Kind::Ball;
        desc.anchor = bj->getAnchorPoint();
      } else if (kind == "SingleAxisHingeJoint") {
        auto *hj = dynamic_cast<xn::SingleAxisHingeJoint *>(joint);
        if (!hj) continue;
        desc.kind = ConstraintDesc::Kind::Hinge;
        desc.anchor = hj->getAnchorPoint();
        desc.axis = hj->getAxis();
        desc.minLimit = hj->getMinAngle();
        desc.maxLimit = hj->getMaxAngle();
      } else if (kind == "SliderJoint") {
        auto *sj = dynamic_cast<xn::SliderJoint *>(joint);
        if (!sj) continue;
        desc.kind = ConstraintDesc::Kind::Slider;
        // SliderJoint has no anchorPoint; the slider rides body1's center.
        desc.anchor = aPosition(joint, handleFor);
        desc.axis = sj->getAxis();
        desc.minLimit = sj->getMinSeparation();
        desc.maxLimit = sj->getMaxSeparation();
      } else {
        std::cerr << "[PhysicsSystem] unsupported joint type '" << kind
                  << "'; skipping\n";
        continue;
      }

      backend_->addConstraint(world, desc);
    }
  }

  /// The world-space position of a joint's body1 (its anchor when none is
  /// authored, e.g. SliderJoint). Falls back to the origin if unresolved.
  static SFVec3f aPosition(
      const xn::X3DRigidJointNode *joint,
      const std::unordered_map<const xn::RigidBody *, BodyHandle> &handleFor) {
    (void)handleFor;
    if (auto *rb = dynamic_cast<const xn::RigidBody *>(joint->getBody1().get()))
      return rb->getPosition();
    if (auto *rb = dynamic_cast<const xn::RigidBody *>(joint->getBody2().get()))
      return rb->getPosition();
    return SFVec3f{0, 0, 0};
  }

  std::shared_ptr<PhysicsBackend> backend_;
  std::vector<Mapped> bodies_;
  std::size_t droppedBodies_ = 0;  // excluded for unsupported collidable geometry
  bool haveLast_ = false;
  double lastNow_ = 0.0;
  double accumulator_ = 0.0;  // leftover sub-kFixedStep time carried to next frame
  // Once-per-timestamp step guard (mirrors ScriptSystem's prepareEvents guard).
  bool haveStepped_ = false;
  double steppedAt_ = 0.0;

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
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_PHYSICS_SYSTEM_HPP
