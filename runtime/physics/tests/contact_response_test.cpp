// contact_response_test.cpp — ungated test that PhysicsSystem honours
// CollisionCollection.appliedParameters when forwarding friction/bounce to
// setContactResponse. §37: FRICTION_COEFFICIENT-2 gates frictionCoefficients;
// BOUNCE gates bounce (default ["BOUNCE"]). The SELECTOR gates, not the value
// (an explicit selection with value 0 still overrides; no selection leaves the
// engine default untouched). No engine.
#include "PhysicsSystem.hpp"

#include "X3DExecutionContext.hpp"

#include "x3d/nodes/Box.hpp"
#include "x3d/nodes/CollidableShape.hpp"
#include "x3d/nodes/CollisionCollection.hpp"
#include "x3d/nodes/RigidBody.hpp"
#include "x3d/nodes/RigidBodyCollection.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/core/X3Denums.hpp"

#include <cstdio>
#include <memory>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;

static int g_failures = 0;
#define CHECK(cond, msg)                                                        \
  do {                                                                          \
    if (!(cond)) { std::printf("FAIL: %s\n", msg); ++g_failures; }             \
    else { std::printf("ok:   %s\n", msg); }                                   \
  } while (0)

// Records the last setContactResponse args.
class RecordingBackend : public PhysicsBackend {
public:
  bool called = false;
  float friction = -99.0f, restitution = -99.0f;
  WorldHandle createWorld(const SFVec3f &) override { return 1; }
  BodyHandle addBody(WorldHandle, const ShapeDesc &, const MassProperties &, bool,
                     const SFVec3f &, const SFRotation &, const SFVec3f &,
                     const SFVec3f &) override { return ++h_; }
  ConstraintHandle addConstraint(WorldHandle, const ConstraintDesc &) override { return 1; }
  void applyForce(WorldHandle, BodyHandle, const SFVec3f &, const SFVec3f &) override {}
  void setGravityFactor(WorldHandle, BodyHandle, float) override {}
  void getBodyVelocity(WorldHandle, BodyHandle, SFVec3f &l, SFVec3f &a) const override { l = {0,0,0}; a = {0,0,0}; }
  void step(WorldHandle, double) override {}
  void getBodyTransform(WorldHandle, BodyHandle, SFVec3f &p, SFRotation &o) const override { p = {0,0,0}; o = {0,0,1,0}; }
  void setContactResponse(WorldHandle, float f, float r) override {
    called = true; friction = f; restitution = r;
  }
private:
  BodyHandle h_ = 0;
};

// One-body collection whose collider is a CollisionCollection configured with
// the given appliedParameters, frictionCoefficients, and bounce.
static std::shared_ptr<RigidBodyCollection>
makeColl(std::vector<AppliedParametersChoices> ap, SFVec2f fc, float bounce) {
  auto box = std::make_shared<Box>(); box->setSizeUnchecked(SFVec3f{1, 1, 1});
  auto shp = std::make_shared<Shape>();
  shp->setGeometry(std::static_pointer_cast<X3DNode>(box));
  auto coll = std::make_shared<CollidableShape>();
  coll->setShapeUnchecked(std::static_pointer_cast<X3DNode>(shp));
  auto body = std::make_shared<RigidBody>();
  body->setGeometry(MFNode{std::static_pointer_cast<X3DNode>(coll)});
  auto cc = std::make_shared<CollisionCollection>();
  cc->setAppliedParameters(ap);
  cc->setFrictionCoefficients(fc);
  cc->setBounce(bounce);
  auto rbc = std::make_shared<RigidBodyCollection>();
  rbc->setBodies(MFNode{std::static_pointer_cast<X3DNode>(body)});
  rbc->setColliderUnchecked(std::static_pointer_cast<X3DNode>(cc));
  return rbc;
}

int main() {
  // Case 1: FRICTION_COEFFICIENT_2 selected, frictionCoefficients (1,1), bounce 0.
  // Selector present -> frictionCoefficients.x applied; BOUNCE NOT selected -> restitution unset.
  {
    auto backend = std::make_shared<RecordingBackend>();
    PhysicsSystem sys(backend);
    X3DExecutionContext ctx;
    auto rbc = makeColl(
        std::vector<AppliedParametersChoices>{AppliedParametersChoices::FRICTION_COEFFICIENT_2},
        SFVec2f{1.0f, 1.0f}, 0.0f);
    sys.attach(rbc.get(), ctx);
    CHECK(backend->called, "setContactResponse called (friction selector)");
    CHECK(backend->friction == 1.0f,
          "FRICTION_COEFFICIENT-2 selector -> frictionCoefficients.x forwarded");
    CHECK(backend->restitution < 0.0f,
          "no BOUNCE selector -> restitution unset (-1)");
  }
  // Case 2: BOUNCE selected, frictionCoefficients (0,0), bounce 0.8.
  // Selector present -> bounce applied; FRICTION_COEFFICIENT-2 NOT selected -> friction unset.
  {
    auto backend = std::make_shared<RecordingBackend>();
    PhysicsSystem sys(backend);
    X3DExecutionContext ctx;
    auto rbc = makeColl(
        std::vector<AppliedParametersChoices>{AppliedParametersChoices::BOUNCE},
        SFVec2f{0.0f, 0.0f}, 0.8f);
    sys.attach(rbc.get(), ctx);
    CHECK(backend->friction < 0.0f,
          "no FRICTION_COEFFICIENT-2 selector -> friction unset (-1)");
    CHECK(backend->restitution == 0.8f,
          "BOUNCE selector -> bounce forwarded as restitution");
  }
  // Case 3: default selector {BOUNCE}, bounce = 0.
  // BOUNCE present -> restitution = bounce = 0 (explicit zero, not -1);
  // no FRICTION_COEFFICIENT-2 -> friction unset.
  {
    auto backend = std::make_shared<RecordingBackend>();
    PhysicsSystem sys(backend);
    X3DExecutionContext ctx;
    auto rbc = makeColl(
        std::vector<AppliedParametersChoices>{AppliedParametersChoices::BOUNCE},
        SFVec2f{0.0f, 0.0f}, 0.0f);
    sys.attach(rbc.get(), ctx);
    CHECK(backend->friction < 0.0f,
          "default selector (BOUNCE only) -> friction unset");
    CHECK(backend->restitution == 0.0f,
          "BOUNCE selector with bounce=0 -> restitution=0 (explicit, not -1)");
  }
  // Case 4: explicit empty appliedParameters {}.
  // Neither selector -> both unset (-1).
  {
    auto backend = std::make_shared<RecordingBackend>();
    PhysicsSystem sys(backend);
    X3DExecutionContext ctx;
    auto rbc = makeColl(
        std::vector<AppliedParametersChoices>{},
        SFVec2f{0.0f, 0.0f}, 0.0f);
    sys.attach(rbc.get(), ctx);
    CHECK(backend->friction < 0.0f && backend->restitution < 0.0f,
          "empty appliedParameters -> both unset (-1)");
  }
  // Case 5: selector gates, not the value: {} but frictionCoefficients (1,1).
  // No FRICTION_COEFFICIENT-2 selector -> friction NOT applied despite non-zero value.
  {
    auto backend = std::make_shared<RecordingBackend>();
    PhysicsSystem sys(backend);
    X3DExecutionContext ctx;
    auto rbc = makeColl(
        std::vector<AppliedParametersChoices>{},
        SFVec2f{1.0f, 1.0f}, 0.0f);
    sys.attach(rbc.get(), ctx);
    CHECK(backend->friction < 0.0f,
          "SELECTOR gates (not value): no FRICTION_COEFFICIENT-2 -> friction unset even when coefficients non-zero");
  }
  return g_failures == 0 ? 0 : 1;
}
