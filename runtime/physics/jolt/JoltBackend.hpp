// Jolt implementation of PhysicsBackend, enabled by X3D_CPP_BUILD_PHYSICS.
// Keep Jolt headers in the implementation to isolate their types and warnings.
// See docs/wiki/explanation/decisions/0019-physics-seam.md.
#ifndef X3D_RUNTIME_PHYSICS_JOLT_BACKEND_HPP
#define X3D_RUNTIME_PHYSICS_JOLT_BACKEND_HPP

#include "PhysicsBackend.hpp"

#include <memory>

namespace x3d::runtime {

/**
 * @brief Jolt-backed PhysicsBackend. Owns all Jolt state; deterministic step.
 * @details Construction performs the one-time Jolt global init (allocator,
 *          Factory, type registration); destruction tears it down. Each
 *          createWorld() builds a JPH::PhysicsSystem with its own gravity;
 *          addBody() creates + adds a Jolt body from a ShapeDesc; step() runs one
 *          fixed-substep Update(); getBodyTransform() reads the body's
 *          center-of-mass pose back into SF* types.
 */
class JoltBackend : public PhysicsBackend {
public:
  /**
   * @brief Construct the backend.
   * @param workerThreads 0 (default) → a single-threaded, host-independent
   *        deterministic job system (the golden-trace contract: identical results
   *        regardless of CPU core count). A value > 0 uses a thread pool of that
   *        many workers — faster for large scenes, but determinism then holds only
   *        for a fixed thread count.
   */
  explicit JoltBackend(unsigned workerThreads = 0);
  ~JoltBackend() override;

  JoltBackend(const JoltBackend &) = delete;
  JoltBackend &operator=(const JoltBackend &) = delete;

  WorldHandle createWorld(const SFVec3f &gravity) override;
  BodyHandle addBody(WorldHandle world, const ShapeDesc &shape,
                     const MassProperties &mass, bool fixed, const SFVec3f &pos,
                     const SFRotation &ori, const SFVec3f &linVel,
                     const SFVec3f &angVel) override;
  ConstraintHandle addConstraint(WorldHandle world,
                                 const ConstraintDesc &desc) override;
  void applyForce(WorldHandle world, BodyHandle body, const SFVec3f &force,
                  const SFVec3f &torque) override;
  void setGravityFactor(WorldHandle world, BodyHandle body,
                        float factor) override;
  void setBodyAllowSleeping(WorldHandle world, BodyHandle body,
                            bool allow) override;
  void setSleepSettings(WorldHandle world, bool allowSleeping,
                        float timeBeforeSleep,
                        float pointVelocityThreshold) override;
  bool isBodyActive(WorldHandle world, BodyHandle body) const override;
  void setContactResponse(WorldHandle world, float friction,
                          float restitution) override;
  void setSolverSettings(WorldHandle world,
                         const SolverSettings &settings) override;
  void getBodyVelocity(WorldHandle world, BodyHandle body, SFVec3f &lin,
                       SFVec3f &ang) const override;
  void step(WorldHandle world, double dt) override;
  void getBodyTransform(WorldHandle world, BodyHandle body, SFVec3f &pos,
                        SFRotation &ori) const override;
  void drainContacts(WorldHandle world,
                     std::vector<ContactPoint> &out) override;

private:
  struct Impl;            // hides every JPH type from this header
  std::unique_ptr<Impl> impl_;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_PHYSICS_JOLT_BACKEND_HPP
