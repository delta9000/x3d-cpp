// §37 solver fields retain backend defaults unless authored off their X3D defaults.
// A recording backend checks which settings cross the PhysicsSystem boundary.
#include "doctest/doctest.h"
#include "PhysicsSystem.hpp"

#include "X3DExecutionContext.hpp"
#include "X3DScene.hpp"
#include "X3DSceneBridge.hpp"

#include "x3d/nodes/Box.hpp"
#include "x3d/nodes/CollidableShape.hpp"
#include "x3d/nodes/RigidBody.hpp"
#include "x3d/nodes/RigidBodyCollection.hpp"
#include "x3d/nodes/Shape.hpp"

#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;

// A backend that records every setSolverSettings() call and no-ops the rest.
class RecordingBackend : public PhysicsBackend {
public:
  struct Call {
    WorldHandle world;
    SolverSettings settings;
  };
  std::vector<Call> solverCalls;

  WorldHandle createWorld(const SFVec3f &) override { return ++nextWorld_; }
  BodyHandle addBody(WorldHandle, const ShapeDesc &, const MassProperties &,
                     bool, const SFVec3f &, const SFRotation &, const SFVec3f &,
                     const SFVec3f &) override {
    return ++nextBody_;
  }
  ConstraintHandle addConstraint(WorldHandle, const ConstraintDesc &) override {
    return kInvalidConstraintHandle;
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
  void getBodyTransform(WorldHandle, BodyHandle, SFVec3f &,
                        SFRotation &) const override {}
  void setSolverSettings(WorldHandle world,
                         const SolverSettings &settings) override {
    solverCalls.push_back({world, settings});
  }

private:
  WorldHandle nextWorld_ = 0;
  BodyHandle nextBody_ = 0;
};

static std::shared_ptr<RigidBodyCollection> makeCollection() {
  auto box = std::make_shared<Box>();
  box->setSizeUnchecked(SFVec3f{1, 1, 1});
  auto shape = std::make_shared<Shape>();
  shape->setGeometry(std::static_pointer_cast<X3DNode>(box));
  auto collidable = std::make_shared<CollidableShape>();
  collidable->setShapeUnchecked(std::static_pointer_cast<X3DNode>(shape));
  auto body = std::make_shared<RigidBody>();
  body->setMass(1.0f);
  body->setPosition(SFVec3f{0, 10, 0});
  body->setGeometry(MFNode{std::static_pointer_cast<X3DNode>(collidable)});
  auto collection = std::make_shared<RigidBodyCollection>();
  collection->setBodies(MFNode{std::static_pointer_cast<X3DNode>(body)});
  return collection;
}

TEST_CASE("physics_solver_settings_test") {
  // (a) §37 defaults: every solver field is left at its sentinel.
  {
    auto backend = std::make_shared<RecordingBackend>();
    PhysicsSystem physics(backend);
    X3DExecutionContext ctx;
    auto collection = makeCollection();
    physics.attach(collection.get(), ctx);

    CHECK_MESSAGE((backend->solverCalls.size() == 1), "solver settings delivered once on attach");
    if (!backend->solverCalls.empty()) {
      const SolverSettings &s = backend->solverCalls.front().settings;
      std::fprintf(stderr,
                   "defaults -> iterations=%d errorCorrection=%g thickness=%g "
                   "maxCorrectionSpeed=%g\n",
                   s.velocityIterations, s.errorCorrection,
                   s.contactSurfaceThickness, s.maxCorrectionSpeed);
      CHECK_MESSAGE((s.velocityIterations == 0),
                    "default iterations leaves the sentinel (engine default untouched)");
      CHECK_MESSAGE((s.errorCorrection < 0.0f), "default errorCorrection leaves the sentinel");
      CHECK_MESSAGE((s.contactSurfaceThickness < 0.0f),
                    "default contactSurfaceThickness leaves the sentinel");
      CHECK_MESSAGE((s.maxCorrectionSpeed < 0.0f),
                    "default maxCorrectionSpeed leaves the sentinel");
    }
  }

  // (b) authored non-default values cross the seam verbatim.
  {
    auto backend = std::make_shared<RecordingBackend>();
    PhysicsSystem physics(backend);
    X3DExecutionContext ctx;
    auto collection = makeCollection();
    collection->setIterations(25);
    collection->setErrorCorrection(0.5f);
    collection->setContactSurfaceThickness(0.03f);
    collection->setMaxCorrectionSpeed(5.0f);
    // No engine mapping — must not change anything on the seam.
    collection->setConstantForceMix(0.2f);
    collection->setPreferAccuracy(true);
    physics.attach(collection.get(), ctx);

    CHECK_MESSAGE((backend->solverCalls.size() == 1), "solver settings delivered once on attach");
    if (!backend->solverCalls.empty()) {
      const SolverSettings &s = backend->solverCalls.front().settings;
      std::fprintf(stderr,
                   "authored -> iterations=%d errorCorrection=%g thickness=%g "
                   "maxCorrectionSpeed=%g\n",
                   s.velocityIterations, s.errorCorrection,
                   s.contactSurfaceThickness, s.maxCorrectionSpeed);
      CHECK_MESSAGE((s.velocityIterations == 25), "authored iterations crosses the seam");
      CHECK_MESSAGE((std::fabs(s.errorCorrection - 0.5f) < 1e-6f),
                    "authored errorCorrection crosses the seam");
      CHECK_MESSAGE((std::fabs(s.contactSurfaceThickness - 0.03f) < 1e-6f),
                    "authored contactSurfaceThickness crosses the seam");
      CHECK_MESSAGE((std::fabs(s.maxCorrectionSpeed - 5.0f) < 1e-6f),
                    "authored maxCorrectionSpeed crosses the seam");
    }
  }

}
