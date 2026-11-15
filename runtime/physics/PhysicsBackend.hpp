// Physics backend boundary for ISO/IEC 19775-1 §37 RigidBodyPhysics.
// See docs/wiki/explanation/decisions/0019-physics-seam.md.
#ifndef X3D_RUNTIME_PHYSICS_BACKEND_HPP
#define X3D_RUNTIME_PHYSICS_BACKEND_HPP

#include "x3d/core/X3Dtypes.hpp"  // SFVec3f, SFRotation — the runtime's own field types

#include <cstdint>
#include <vector>

namespace x3d::runtime {

using namespace x3d::core;

/**
 * @brief Opaque physics-world handle. Zero is invalid; return it only to the
 *        backend that created it.
 */
using WorldHandle = std::uint64_t;

/**
 * @brief Opaque handle a backend returns from addBody() to identify one body.
 *        Zero is the invalid handle.
 */
using BodyHandle = std::uint64_t;

/**
 * @brief Opaque handle a backend returns from addConstraint() to identify one
 *        joint/constraint. Zero is the invalid handle.
 */
using ConstraintHandle = std::uint64_t;

inline constexpr WorldHandle kInvalidWorldHandle = 0;

inline constexpr BodyHandle kInvalidBodyHandle = 0;

inline constexpr ConstraintHandle kInvalidConstraintHandle = 0;

/**
 * @brief Collision primitive parameters; see docs/wiki/explanation/decisions/0019-physics-seam.md.
 */
struct ShapeDesc {
  enum class Kind { Box, Sphere, Cylinder };

  Kind kind = Kind::Box;
  /** @brief Half-extents (x,y,z) when kind==Box. */
  SFVec3f boxHalfExtents{1, 1, 1};
  /** @brief Radius when kind==Sphere. */
  float sphereRadius = 1.0f;
  /** @brief Radius when kind==Cylinder (axis = Y, matching X3D Cylinder). */
  float cylinderRadius = 1.0f;
  /** @brief Half-height when kind==Cylinder. */
  float cylinderHalfHeight = 1.0f;

  static ShapeDesc box(const SFVec3f &halfExtents) {
    ShapeDesc d;
    d.kind = Kind::Box;
    d.boxHalfExtents = halfExtents;
    return d;
  }

  static ShapeDesc sphere(float radius) {
    ShapeDesc d;
    d.kind = Kind::Sphere;
    d.sphereRadius = radius;
    return d;
  }

  static ShapeDesc cylinder(float radius, float halfHeight) {
    ShapeDesc d;
    d.kind = Kind::Cylinder;
    d.cylinderRadius = radius;
    d.cylinderHalfHeight = halfHeight;
    return d;
  }
};

/**
 * @brief Joint parameters in world space.
 */
struct ConstraintDesc {
  enum class Kind { Ball, Hinge, Slider };

  Kind kind = Kind::Ball;
  BodyHandle bodyA = kInvalidBodyHandle;
  /** @brief The second body, or kInvalidBodyHandle to bind bodyA to the world. */
  BodyHandle bodyB = kInvalidBodyHandle;
  /** @brief anchorPoint (world space, as authored). Ignored for pure-axis use. */
  SFVec3f anchor{0, 0, 0};
  /** @brief Hinge/slider axis (world space). Ignored for Ball. */
  SFVec3f axis{0, 1, 0};
  /** @brief Lower limit: angle (Hinge, radians) or separation (Slider). */
  float minLimit = 0.0f;
  /** @brief Upper limit: angle (Hinge, radians) or separation (Slider). */
  float maxLimit = 0.0f;
};

/**
 * @brief bodyA/bodyB belong to the drained world; normal points from bodyA to bodyB.
 *        depth is penetration along normal.
 */
struct ContactPoint {
  BodyHandle bodyA = kInvalidBodyHandle;
  BodyHandle bodyB = kInvalidBodyHandle;
  SFVec3f position{0, 0, 0};
  SFVec3f normal{0, 1, 0};
  float depth = 0.0f;
};

/**
 * @brief §37 mass, inertia and center-of-mass overrides.
 *        Without overrideInertia, derive inertia from the collision shape; otherwise
 *        use the authored tensor verbatim (§37 RigidBody.inertia).
 *        centerOfMass offsets the shape centroid (§37 RigidBody.centerOfMass).
 */
struct MassProperties {
  float      mass = 1.0f;
  bool       overrideInertia = false;
  SFMatrix3f inertia{};              // 3x3, used iff overrideInertia
  SFVec3f    centerOfMass{0, 0, 0};  // (0,0,0) → no offset
};

/**
 * @brief Non-default §37 solver settings; sentinels preserve backend defaults.
 */
struct SolverSettings {
  /** @brief §37 iterations (solver velocity iterations). 0 sentinel → default. */
  int velocityIterations = 0;
  /** @brief §37 errorCorrection [0,1] (position-error correction fraction).
   *         < 0 sentinel → default. */
  float errorCorrection = -1.0f;
  /** @brief §37 contactSurfaceThickness (m, allowed interpenetration).
   *         < 0 sentinel → default. */
  float contactSurfaceThickness = -1.0f;
  /** @brief §37 maxCorrectionSpeed (m/s, < 0 = no limit; -1 is the §37 default).
   *         < 0 sentinel → default. */
  float maxCorrectionSpeed = -1.0f;
};

/**
 * @brief Owns worlds, bodies and engine state. PhysicsSystem retains the backend
 *        through a shared_ptr for the worlds' lifetime.
 */
class PhysicsBackend {
public:
  virtual ~PhysicsBackend() = default;

  /**
   * @brief Create a physics world with the given gravity vector (m/s^2).
   * @return A non-zero WorldHandle on success, kInvalidWorldHandle on failure.
   */
  virtual WorldHandle createWorld(const SFVec3f &gravity) = 0;

  /**
   * @brief Add one rigid body to a world.
   * @param world The world the body belongs to.
   * @param shape The collision/mass shape (Box, Sphere, or Cylinder).
   * @param mass Mass properties: mass in kilograms (fixed bodies ignore it);
   *             when overrideInertia=true the authored 3x3 inertia tensor is
   *             used verbatim (§37 RigidBody.inertia), otherwise the backend
   *             derives it from the collision shape; a non-zero centerOfMass
   *             offsets the body's COM from the shape centroid (§37
   *             RigidBody.centerOfMass).
   * @param fixed TRUE for an immovable (static) body; FALSE for a dynamic one.
   * @param pos Initial world-space position (the body's center).
   * @param ori Initial world-space orientation (axis-angle SFRotation).
   * @param linVel Initial linear velocity (m/s).
   * @param angVel Initial angular velocity (rad/s, axis-scaled).
   * @return A non-zero BodyHandle on success, kInvalidBodyHandle on failure.
   */
  virtual BodyHandle addBody(WorldHandle world, const ShapeDesc &shape,
                             const MassProperties &mass, bool fixed,
                             const SFVec3f &pos, const SFRotation &ori,
                             const SFVec3f &linVel, const SFVec3f &angVel) = 0;

  /**
   * @brief Add one joint/constraint to a world, between two bodies (or one body
   *        and the world).
   * @param world The world the bodies belong to.
   * @param desc The constraint description (kind, bodies, anchor, axis, limits).
   * @details All bodies the constraint references must already have been added
   *          (addBody) to `world` — constraints are built after the bodies exist.
   *          A backend with no joint support may return kInvalidConstraintHandle.
   * @return A non-zero ConstraintHandle on success, kInvalidConstraintHandle on
   *         failure.
   */
  virtual ConstraintHandle addConstraint(WorldHandle world,
                                         const ConstraintDesc &desc) = 0;

  /**
   * @brief Apply a linear force and torque to a body for the next step.
   * @details §37 forces/torques are "applied every presentation frame ...
   *          continuously applied until reset" — so the PhysicsSystem re-applies
   *          the body's summed forces/torques before each step (most engines,
   *          Jolt included, clear accumulated forces after each integration). A
   *          zero force+torque is a no-op. The body is (re)activated so a resting
   *          body responds to a newly-authored force.
   */
  virtual void applyForce(WorldHandle world, BodyHandle body,
                          const SFVec3f &force, const SFVec3f &torque) = 0;

  /**
   * @brief Set a body's gravity factor (1 = full world gravity, 0 = none).
   * @details Maps §37 RigidBody.useGlobalGravity: FALSE → 0 (the body ignores the
   *          collection's gravity), TRUE → 1. Set once at enrol time.
   */
  virtual void setGravityFactor(WorldHandle world, BodyHandle body,
                                float factor) = 0;

  /**
   * @brief Set whether a single body may sleep (auto-disable on/off).
   * @details Maps §37 RigidBody.autoDisable: FALSE (the default) → the body is
   *          never put to sleep (it keeps integrating, even at rest); TRUE → the
   *          backend's sleep rules may stop integrating it. Per-body (Jolt
   *          supports it); the world-wide thresholds are set by
   *          setSleepSettings(). Default: no-op (a backend without sleeping
   *          ignores it).
   */
  virtual void setBodyAllowSleeping(WorldHandle world, BodyHandle body,
                                    bool allow) {
    (void)world;
    (void)body;
    (void)allow;
  }

  /**
   * @brief Set a world's sleep rules (collection auto-disable + thresholds).
   * @details Maps §37 RigidBodyCollection.autoDisable / disableTime /
   *          disableLinearSpeed. Jolt's sleep tuning is world-wide
   *          (PhysicsSettings), not per-body, so a collection's values apply to
   *          the whole world it owns. A negative threshold means "leave the
   *          backend default", so the §37 defaults (0/0) keep Jolt's own values
   *          (0.5 s / 0.03 m/s). Default: no-op.
   */
  virtual void setSleepSettings(WorldHandle world, bool allowSleeping,
                                float timeBeforeSleep,
                                float pointVelocityThreshold) {
    (void)world;
    (void)allowSleeping;
    (void)timeBeforeSleep;
    (void)pointVelocityThreshold;
  }

  /**
   * @brief Whether a body is currently awake (still integrated each step).
   * @details The observable sleeping state behind §37 autoDisable. Default: true
   *          (a backend without sleeping reports every body awake).
   */
  virtual bool isBodyActive(WorldHandle world, BodyHandle body) const {
    (void)world;
    (void)body;
    return true;
  }

  /**
   * @brief Set a world's per-contact combined friction + restitution.
   * @details Maps §37 CollisionCollection.frictionCoefficients / bounce, applied
   *          to every contact in the world (one CollisionCollection governs one
   *          RigidBodyCollection = one world). A negative value means "do not
   *          override" — the backend keeps its own default combine for that
   *          parameter. Default: no-op (backends without contacts ignore it).
   */
  virtual void setContactResponse(WorldHandle world, float friction,
                                  float restitution) {
    (void)world;
    (void)friction;
    (void)restitution;
  }

  /**
   * @brief Read back a body's current linear + angular velocity.
   * @details The PhysicsSystem needs the previous-frame velocity to compute §37
   *          autoDamp damping (a force opposing the last frame's motion).
   * @param lin Out: linear velocity (m/s).
   * @param ang Out: angular velocity (rad/s, axis-scaled).
   */
  virtual void getBodyVelocity(WorldHandle world, BodyHandle body, SFVec3f &lin,
                               SFVec3f &ang) const = 0;

  /**
   * @brief Advance `world` by `dt` seconds (one integration step).
   * @details A fixed dt + fixed configuration must be deterministic so the sim
   *          golden trace is reproducible.
   */
  virtual void step(WorldHandle world, double dt) = 0;

  /**
   * @brief Read back a body's current world-space pose.
   * @param pos Out: the body's center position.
   * @param ori Out: the body's orientation (axis-angle SFRotation).
   */
  virtual void getBodyTransform(WorldHandle world, BodyHandle body,
                                SFVec3f &pos, SFRotation &ori) const = 0;

  /** Move out contacts buffered by step() since the previous drain.
   *  Default: no contacts. */
  virtual void drainContacts(WorldHandle world, std::vector<ContactPoint> &out) {
    (void)world;
    out.clear();
  }

  /**
   * @brief Apply §37 solver tuning to a world.
   * @details Maps the RigidBodyCollection's solver-tuning fields onto the
   *          backend's own solver. Each field is optional (its sentinel value
   *          means "leave the backend default"), so a world created at the §37
   *          defaults is byte-for-byte the backend's default configuration.
   *          Default: no-op (a backend without a tunable solver ignores it).
   */
  virtual void setSolverSettings(WorldHandle world,
                                 const SolverSettings &settings) {
    (void)world;
    (void)settings;
  }
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_PHYSICS_BACKEND_HPP
