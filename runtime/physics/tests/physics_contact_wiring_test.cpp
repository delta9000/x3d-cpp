// physics_contact_wiring_test.cpp — ungated integration test for PhysicsSystem's
// contact path, using a FAKE PhysicsBackend that scripts one contact. Proves:
// attach enrols a CollisionSensor; update drains the backend, resolves body
// handles to the scene's RigidBody/CollidableShape nodes, and the reporter emits
// CollisionSensor.contacts over the cascade. No physics engine required.
#include "PhysicsSystem.hpp"

#include "X3DExecutionContext.hpp"

#include "x3d/nodes/Box.hpp"
#include "x3d/nodes/CollidableShape.hpp"
#include "x3d/nodes/CollisionCollection.hpp"
#include "x3d/nodes/CollisionSensor.hpp"
#include "x3d/nodes/Contact.hpp"
#include "x3d/nodes/RigidBody.hpp"
#include "x3d/nodes/RigidBodyCollection.hpp"
#include "x3d/nodes/Shape.hpp"

#include <cstdio>
#include <memory>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;

static int g_failures = 0;
#define CHECK(cond, msg)                                                        \
  do {                                                                          \
    if (!(cond)) { std::printf("FAIL: %s\n", msg); ++g_failures; }             \
    else { std::printf("ok:   %s\n", msg); }                                   \
  } while (0)

// A minimal backend: addBody hands out 1,2,3...; step is a no-op; drainContacts
// reports a persistent contact between handles 1 and 2 on every drain (models
// two bodies in sustained contact, so the sensor stays active across frames).
class FakeContactBackend : public PhysicsBackend {
public:
  WorldHandle createWorld(const SFVec3f &) override { return 1; }
  BodyHandle addBody(WorldHandle, const ShapeDesc &, const MassProperties &, bool,
                     const SFVec3f &, const SFRotation &, const SFVec3f &,
                     const SFVec3f &) override {
    return ++lastHandle_;
  }
  ConstraintHandle addConstraint(WorldHandle, const ConstraintDesc &) override {
    return 1;
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
  void getBodyTransform(WorldHandle, BodyHandle, SFVec3f &pos,
                        SFRotation &ori) const override {
    pos = SFVec3f{0, 0, 0};
    ori = SFRotation{0, 0, 1, 0};
  }
  void drainContacts(WorldHandle, std::vector<ContactPoint> &out) override {
    out.clear();
    ContactPoint cp;
    cp.bodyA = 1;
    cp.bodyB = 2;
    cp.position = SFVec3f{0, 0.5f, 0};
    cp.normal = SFVec3f{0, 1, 0};
    cp.depth = 0.02f;
    out.push_back(cp);
  }

private:
  BodyHandle lastHandle_ = 0;
};

// helper: a RigidBody whose geometry is a CollidableShape(Box). The same
// CollidableShape instance is shared into the CollisionCollection (USE-style).
static std::shared_ptr<RigidBody>
makeBody(std::shared_ptr<CollidableShape> &outShape) {
  auto box = std::make_shared<Box>();
  box->setSizeUnchecked(SFVec3f{1, 1, 1});
  auto shapeWrap = std::make_shared<Shape>();
  shapeWrap->setGeometry(std::static_pointer_cast<X3DNode>(box));
  outShape = std::make_shared<CollidableShape>();
  outShape->setShapeUnchecked(std::static_pointer_cast<X3DNode>(shapeWrap));
  auto body = std::make_shared<RigidBody>();
  body->setGeometry(MFNode{std::static_pointer_cast<X3DNode>(outShape)});
  return body;
}

int main() {
  std::shared_ptr<CollidableShape> shapeA, shapeB;
  auto bodyA = makeBody(shapeA);
  auto bodyB = makeBody(shapeB);

  auto collection = std::make_shared<RigidBodyCollection>();
  collection->setBodies(MFNode{bodyA, bodyB});

  auto collCol = std::make_shared<CollisionCollection>();
  collCol->setCollidables(MFNode{shapeA, shapeB});
  auto sensor = std::make_shared<CollisionSensor>();
  sensor->setCollider(collCol);

  X3DExecutionContext ctx;
  auto physics = std::make_shared<PhysicsSystem>(
      std::make_shared<FakeContactBackend>());
  physics->attach(collection.get(), ctx);
  physics->attach(sensor.get(), ctx);
  ctx.addSystem(physics);

  double t = 0.0;
  ctx.tick(t);                 // t=0: establish clock, no dt
  for (int i = 0; i < 3; ++i) { t += 1.0 / 60.0; ctx.tick(t); }

  CHECK(sensor->getContacts().size() == 1, "PhysicsSystem -> sensor got 1 contact");
  CHECK(sensor->getIsActive() == true, "sensor isActive after wired contact");
  auto cs = sensor->getContacts();
  auto *c = cs.empty() ? nullptr : dynamic_cast<Contact *>(cs[0].get());
  CHECK(c && c->getBody1().get() == bodyA.get(),
        "Contact.body1 resolved to the scene RigidBody");
  CHECK(c && c->getGeometry2().get() == shapeB.get(),
        "Contact.geometry2 resolved to the scene CollidableShape");
  return g_failures == 0 ? 0 : 1;
}
