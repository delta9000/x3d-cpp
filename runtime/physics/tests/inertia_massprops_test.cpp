// inertia_massprops_test.cpp — ungated tests for the §37 inertia/centerOfMass
// path: the MassProperties seam struct (Task 1) and PhysicsSystem's non-default
// detection (Task 2). No physics engine required.
#include "PhysicsBackend.hpp"
#include "PhysicsSystem.hpp"
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/Box.hpp"
#include "x3d/nodes/CollidableShape.hpp"
#include "x3d/nodes/RigidBody.hpp"
#include "x3d/nodes/RigidBodyCollection.hpp"
#include "x3d/nodes/Shape.hpp"

#include <cstdio>
#include <memory>

using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;

static int g_failures = 0;
#define CHECK(cond, msg)                                                        \
  do {                                                                          \
    if (!(cond)) { std::printf("FAIL: %s\n", msg); ++g_failures; }             \
    else { std::printf("ok:   %s\n", msg); }                                   \
  } while (0)

static void test_massprops_defaults() {
  MassProperties mp;
  CHECK(mp.mass == 1.0f, "MassProperties.mass defaults to 1");
  CHECK(mp.overrideInertia == false, "overrideInertia defaults false");
  CHECK(mp.centerOfMass.x == 0.0f && mp.centerOfMass.y == 0.0f &&
            mp.centerOfMass.z == 0.0f,
        "centerOfMass defaults to origin");
  CHECK(mp.inertia.matrix[0][0] == 0.0f && mp.inertia.matrix[2][2] == 0.0f,
        "inertia tensor zero-initialises");
}

// A fake backend that records the MassProperties passed to the LAST addBody, so
// we can assert PhysicsSystem's non-default detection without a physics engine.
class RecordingBackend : public PhysicsBackend {
public:
  MassProperties lastMass;
  WorldHandle createWorld(const SFVec3f &) override { return 1; }
  BodyHandle addBody(WorldHandle, const ShapeDesc &, const MassProperties &m,
                     bool, const SFVec3f &, const SFRotation &, const SFVec3f &,
                     const SFVec3f &) override {
    lastMass = m;
    return ++h_;
  }
  ConstraintHandle addConstraint(WorldHandle, const ConstraintDesc &) override {
    return 1;
  }
  void applyForce(WorldHandle, BodyHandle, const SFVec3f &,
                  const SFVec3f &) override {}
  void setGravityFactor(WorldHandle, BodyHandle, float) override {}
  void getBodyVelocity(WorldHandle, BodyHandle, SFVec3f &l,
                       SFVec3f &a) const override { l = {0,0,0}; a = {0,0,0}; }
  void step(WorldHandle, double) override {}
  void getBodyTransform(WorldHandle, BodyHandle, SFVec3f &p,
                        SFRotation &o) const override {
    p = {0,0,0}; o = {0,0,1,0};
  }

private:
  BodyHandle h_ = 0;
};

// Build a one-body collection with a Box collidable; the caller sets inertia/COM.
static std::shared_ptr<RigidBodyCollection>
makeOneBody(std::shared_ptr<RigidBody> &outBody) {
  auto box = std::make_shared<Box>();
  box->setSizeUnchecked(SFVec3f{1, 1, 1});
  auto shape = std::make_shared<Shape>();
  shape->setGeometry(std::static_pointer_cast<X3DNode>(box));
  auto coll = std::make_shared<CollidableShape>();
  coll->setShapeUnchecked(std::static_pointer_cast<X3DNode>(shape));
  outBody = std::make_shared<RigidBody>();
  outBody->setGeometry(MFNode{std::static_pointer_cast<X3DNode>(coll)});
  auto collection = std::make_shared<RigidBodyCollection>();
  collection->setBodies(MFNode{std::static_pointer_cast<X3DNode>(outBody)});
  return collection;
}

static void test_default_inertia_not_overridden() {
  std::shared_ptr<RigidBody> body;
  auto collection = makeOneBody(body);  // inertia defaults to identity
  auto backend = std::make_shared<RecordingBackend>();
  PhysicsSystem sys(backend);
  x3d::runtime::X3DExecutionContext ctx;
  sys.attach(collection.get(), ctx);
  CHECK(backend->lastMass.overrideInertia == false,
        "identity inertia -> overrideInertia false (shape-derived)");
}

static void test_nondefault_inertia_overridden() {
  std::shared_ptr<RigidBody> body;
  auto collection = makeOneBody(body);
  SFMatrix3f tensor{};                  // diag(0.01, 0.02, 0.03)
  tensor.matrix[0][0] = 0.01f; tensor.matrix[1][1] = 0.02f; tensor.matrix[2][2] = 0.03f;
  body->setInertia(tensor);
  auto backend = std::make_shared<RecordingBackend>();
  PhysicsSystem sys(backend);
  x3d::runtime::X3DExecutionContext ctx;
  sys.attach(collection.get(), ctx);
  CHECK(backend->lastMass.overrideInertia == true,
        "non-identity inertia -> overrideInertia true");
  CHECK(backend->lastMass.inertia.matrix[1][1] == 0.02f,
        "authored inertia tensor passed through");
}

static void test_centerofmass_passed_through() {
  std::shared_ptr<RigidBody> body;
  auto collection = makeOneBody(body);
  body->setCenterOfMass(SFVec3f{0.5f, 0.25f, 0});
  auto backend = std::make_shared<RecordingBackend>();
  PhysicsSystem sys(backend);
  x3d::runtime::X3DExecutionContext ctx;
  sys.attach(collection.get(), ctx);
  CHECK(backend->lastMass.centerOfMass.x == 0.5f,
        "centerOfMass.x passed through to the backend");
  CHECK(backend->lastMass.centerOfMass.y == 0.25f,
        "centerOfMass.y passed through to the backend");
}

int main() {
  test_massprops_defaults();
  test_default_inertia_not_overridden();
  test_nondefault_inertia_overridden();
  test_centerofmass_passed_through();
  return g_failures == 0 ? 0 : 1;
}
