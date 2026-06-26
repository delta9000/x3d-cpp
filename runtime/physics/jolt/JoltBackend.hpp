// JoltBackend.hpp
// A PhysicsBackend implemented with Jolt Physics (https://github.com/jrouwe/
// JoltPhysics). Behind X3D_CPP_BUILD_PHYSICS: this header is only included by
// the gated backend TU + the gated physics tests; nothing in the core links it.
//
// The core seam (PhysicsBackend.hpp) names no Jolt type. This class is the one
// place Jolt and the runtime meet: it maps ShapeDesc -> Jolt shapes and
// SFVec3f/SFRotation <-> JPH::Vec3/Quat, and owns all Jolt global + per-world
// state (Factory, type registration, temp allocator, job system, the broadphase
// + collision-layer filter interfaces, and each JPH::PhysicsSystem). The Jolt
// headers are NOT pulled in here (they are warning-noisy and we keep them in the
// .cpp); a pImpl hides every JPH type so even this header stays Jolt-free at the
// type level for its includers.
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
   *        DETERMINISTIC job system (the golden-trace contract: identical results
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
  void setContactResponse(WorldHandle world, float friction,
                          float restitution) override;
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
