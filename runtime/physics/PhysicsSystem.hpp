// Drive §37 rigid-body dynamics through PhysicsBackend and post integrated poses
// as RigidBody events. See docs/wiki/decisions/0019-physics-seam.md.
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

/** Inert without a backend. */
class PhysicsSystem : public System {
public:
  /**
   * @brief Construct with the backend engine (null -> inert).
   * @param backend The physics solver implementation. Shared so a test can hold
   *        it and so the System retains it for the worlds' lifetime.
   */
  explicit PhysicsSystem(std::shared_ptr<PhysicsBackend> backend)
      : backend_(std::move(backend)) {}

  /** No-op for unsupported node types or without a backend. */
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
    worlds_[collection] = world;

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
        // Count unsupported bodies for droppedBodyCount().
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
      // §37 autoDisable: FALSE (the default) -> the body never sleeps.
      backend_->setBodyAllowSleeping(world, handle, body->getAutoDisable());
      bodies_.push_back(Mapped{body, world, handle});
      handleFor.emplace(body, handle);
      // Record the resolution so drained contacts (world,handle) map back to the
      // scene nodes, with a stable ordinal for deterministic contact ordering.
      bodyRef_[{world, handle}] =
          BodyRef{bodyNode, collidableShape, nextOrdinal_++};
    }

    // §37 RigidBodyCollection auto-disable: autoDisable gates whether anything in
    // this world may sleep; disableTime / disableLinearSpeed tune Jolt's
    // world-wide sleep thresholds (Jolt has no per-body equivalent, so the
    // collection's values apply to the whole world). disableAngularSpeed has no
    // world-wide Jolt counterpart and stays deferred (see CONF-RBP-DAMP).
    const double disableTime = collection->getDisableTime();
    const float disableLinSpeed = collection->getDisableLinearSpeed();
    backend_->setSleepSettings(
        world, collection->getAutoDisable(),
        disableTime > 0.0 ? static_cast<float>(disableTime) : -1.0f,
        disableLinSpeed > 0.0f ? disableLinSpeed : -1.0f);

    // Joints require every body in the collection to exist,
    // so resolve each §37 RigidJoint into a ConstraintDesc and add it.
    attachJoints(*collection, world, handleFor);

    // §37 contact-response params: appliedParameters selects which response
    // params the solver applies — FRICTION_COEFFICIENT-2 gates
    // frictionCoefficients, BOUNCE gates bounce (default ["BOUNCE"]).
    // A selected param overrides the engine default even when its value is 0
    // (an explicit author choice); an unselected param stays -1 (engine default
    // left untouched). Scalar friction uses .x; anisotropic .y is unsupported.
    // Unsupported response parameters retain backend defaults.
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

    // §37 solver tuning: map the collection's solver fields onto the backend.
    // Each field is sent only when it differs from the §37 default — at the spec
    // defaults every field stays at its sentinel and the backend's own solver
    // settings are left untouched (spec-default behavior is preserved). Fields
    // with no honest backend equivalent (constantForceMix, preferAccuracy) are
    // deliberately not carried across the seam (CONF-RBP-SOLVER).
    SolverSettings solver;
    if (collection->getIterations() !=
        xn::RigidBodyCollection::getDefaultIterations())
      solver.velocityIterations = collection->getIterations();
    if (collection->getErrorCorrection() !=
        xn::RigidBodyCollection::getDefaultErrorCorrection())
      solver.errorCorrection = collection->getErrorCorrection();
    if (collection->getContactSurfaceThickness() !=
        xn::RigidBodyCollection::getDefaultContactSurfaceThickness())
      solver.contactSurfaceThickness = collection->getContactSurfaceThickness();
    if (collection->getMaxCorrectionSpeed() !=
        xn::RigidBodyCollection::getDefaultMaxCorrectionSpeed())
      solver.maxCorrectionSpeed = collection->getMaxCorrectionSpeed();
    backend_->setSolverSettings(world, solver);
  }

  void detach(X3DNode *node, X3DExecutionContext &) override {
    if (auto *sensor = dynamic_cast<xn::CollisionSensor *>(node))
      reporter_.removeSensor(sensor);
    WorldHandle world = kInvalidWorldHandle;
    if (auto it = worlds_.find(node); it != worlds_.end()) {
      world = it->second;
      worlds_.erase(it);
    }
    bodies_.erase(std::remove_if(bodies_.begin(), bodies_.end(), [&](const Mapped &m) {
      return m.body == node || (world != kInvalidWorldHandle && m.world == world);
    }), bodies_.end());
    for (auto it = bodyRef_.begin(); it != bodyRef_.end();) {
      if (it->second.bodyNode.get() == node ||
          (world != kInvalidWorldHandle && it->first.first == world))
        it = bodyRef_.erase(it);
      else ++it;
    }
  }

  /** Clamp long frame gaps and integrate fixed substeps; the first update does not step.
   *  Post poses as RigidBody events to update fields and dispatch ROUTEs. */
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
      // §37: forces/torques are applied every frame (continuously, until the
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
    // orientation field and fans out along the author ROUTEs to the Transforms.
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

  std::size_t bodyCount() const { return bodies_.size(); }

  /**
   * @brief Whether the enrolled body at `index` is awake (observable sleeping).
   * @details Exposes the backend's §37 autoDisable sleeping state so a consumer
   *          (or test) can tell a resting body that is asleep from one that keeps
   *          integrating. Out-of-range -> true.
   */
  bool isBodyActive(std::size_t index) const {
    if (!backend_ || index >= bodies_.size()) return true;
    const Mapped &m = bodies_[index];
    return backend_->isBodyActive(m.world, m.handle);
  }

  /** Count bodies skipped because their collision geometry is unsupported. */
  std::size_t droppedBodyCount() const { return droppedBodies_; }

  /** Count contacts skipped because a body has no enrolled scene node. */
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

  /** §37.4.10 forces/torques persist until reset. Reapply them before each substep. */
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

  /** Use the first supported CollidableShape; return false if none is found. */
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

  /** Unset or unenrolled bodies resolve to the world-anchor sentinel. */
  static BodyHandle resolveBody(
      const SFNode &node,
      const std::unordered_map<const xn::RigidBody *, BodyHandle> &handleFor) {
    if (!node) return kInvalidBodyHandle;
    auto *rb = dynamic_cast<const xn::RigidBody *>(node.get());
    if (!rb) return kInvalidBodyHandle;
    auto it = handleFor.find(rb);
    return it == handleFor.end() ? kInvalidBodyHandle : it->second;
  }

  /** Unsupported joint types are skipped with a stderr diagnostic. */
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
  std::unordered_map<X3DNode *, WorldHandle> worlds_;
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
