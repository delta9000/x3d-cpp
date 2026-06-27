// contact_reporter_test.cpp — ungated pure-logic tests for the §37 contact-
// reporting bridge (ContactPoint seam struct + ContactReporter). No physics
// engine: synthetic ResolvedContacts + hand-built nodes exercise the translator.
#include "PhysicsBackend.hpp"
#include "ContactReporter.hpp"
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/CollisionSensor.hpp"
#include "x3d/nodes/CollisionCollection.hpp"
#include "x3d/nodes/CollidableShape.hpp"
#include "x3d/nodes/RigidBody.hpp"
#include "x3d/nodes/Contact.hpp"
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

static void test_contact_point_defaults() {
  ContactPoint cp;
  CHECK(cp.bodyA == kInvalidBodyHandle, "ContactPoint.bodyA defaults invalid");
  CHECK(cp.bodyB == kInvalidBodyHandle, "ContactPoint.bodyB defaults invalid");
  CHECK(cp.normal.x == 0.0f && cp.normal.y == 1.0f && cp.normal.z == 0.0f,
        "ContactPoint.normal defaults to up");
  CHECK(cp.depth == 0.0f, "ContactPoint.depth defaults 0");
}

// Build: sensor -> collider(collection{bounce=0.5, collidables=[shapeA,shapeB]})
// One ResolvedContact between bodyA/bodyB whose shapes are shapeA/shapeB.
// Expect: one Contact emitted with geometric + collection-default fields,
// intersections = {shapeA, shapeB}, isActive = true.
static void test_single_contact_emitted() {
  auto bodyA = std::make_shared<RigidBody>();
  auto bodyB = std::make_shared<RigidBody>();
  auto shapeA = std::make_shared<CollidableShape>();
  auto shapeB = std::make_shared<CollidableShape>();

  auto collection = std::make_shared<CollisionCollection>();
  collection->setBounce(0.5f);
  collection->setMinBounceSpeed(0.2f);
  collection->setFrictionCoefficients(SFVec2f{0.3f, 0.4f});
  collection->setSlipFactors(SFVec2f{0.1f, 0.2f});
  collection->setCollidables(MFNode{shapeA, shapeB});

  auto sensor = std::make_shared<CollisionSensor>();
  sensor->setCollider(collection);

  ResolvedContact rc;
  rc.body1 = bodyA; rc.body2 = bodyB;
  rc.geometry1 = shapeA; rc.geometry2 = shapeB;
  rc.position = SFVec3f{1, 2, 3};
  rc.normal = SFVec3f{0, 1, 0};
  rc.depth = 0.05f;
  rc.pairKey = (std::uint64_t(0) << 32) | 1;

  x3d::runtime::X3DExecutionContext ctx;
  ContactReporter reporter;
  reporter.addSensor(sensor.get());
  reporter.report(std::vector<ResolvedContact>{rc}, ctx);
  ctx.process();

  MFNode contacts = sensor->getContacts();
  CHECK(contacts.size() == 1, "one Contact emitted");
  CHECK(sensor->getIsActive() == true, "isActive true when contacts");
  CHECK(sensor->getIntersections().size() == 2, "two intersecting collidables");

  if (contacts.size() == 1) {
    auto *c = dynamic_cast<Contact *>(contacts[0].get());
    CHECK(c != nullptr, "emitted node is a Contact");
    if (c) {
      CHECK(c->getPosition().y == 2.0f, "Contact.position from engine");
      CHECK(c->getContactNormal().y == 1.0f, "Contact.contactNormal from engine");
      CHECK(c->getDepth() == 0.05f, "Contact.depth from engine");
      CHECK(c->getBody1().get() == bodyA.get(), "Contact.body1 = bodyA");
      CHECK(c->getGeometry2().get() == shapeB.get(), "Contact.geometry2 = shapeB");
      CHECK(c->getBounce() == 0.5f, "Contact.bounce from collection default");
      CHECK(c->getFrictionCoefficients().x == 0.3f, "frictionCoefficients copied");
      CHECK(c->getSlipCoefficients().y == 0.2f, "slipFactors -> slipCoefficients");
      CHECK(c->getMinBounceSpeed() == 0.2f, "minBounceSpeed copied");
    }
  }
}

// collider == NULL -> no contacts, no isActive event (stays default false).
static void test_null_collider_silent() {
  auto sensor = std::make_shared<CollisionSensor>();  // collider defaults NULL
  x3d::runtime::X3DExecutionContext ctx;
  ContactReporter reporter;
  reporter.addSensor(sensor.get());
  reporter.report({}, ctx);
  ctx.process();
  CHECK(sensor->getContacts().empty(), "null collider -> no contacts");
  CHECK(sensor->getIsActive() == false, "null collider -> isActive false");
}

// collection.enabled == FALSE silences the sensor even with a matching contact.
static void test_disabled_collection_silent() {
  auto shapeA = std::make_shared<CollidableShape>();
  auto shapeB = std::make_shared<CollidableShape>();
  auto col = std::make_shared<CollisionCollection>();
  col->setEnabled(false);
  col->setCollidables(MFNode{shapeA, shapeB});
  auto sensor = std::make_shared<CollisionSensor>();
  sensor->setCollider(col);
  ResolvedContact rc;
  rc.geometry1 = shapeA; rc.geometry2 = shapeB; rc.pairKey = 1;
  x3d::runtime::X3DExecutionContext ctx;
  ContactReporter reporter;
  reporter.addSensor(sensor.get());
  reporter.report({rc}, ctx);
  ctx.process();
  CHECK(sensor->getContacts().empty(), "disabled collection -> no contacts");
}

// Two ResolvedContacts with the same pairKey collapse to one (keep max depth).
static void test_dedupe_by_pair_keep_max_depth() {
  auto shapeA = std::make_shared<CollidableShape>();
  auto shapeB = std::make_shared<CollidableShape>();
  auto col = std::make_shared<CollisionCollection>();
  col->setCollidables(MFNode{shapeA, shapeB});
  auto sensor = std::make_shared<CollisionSensor>();
  sensor->setCollider(col);
  ResolvedContact a; a.geometry1 = shapeA; a.geometry2 = shapeB; a.pairKey = 7; a.depth = 0.01f;
  ResolvedContact b; b.geometry1 = shapeA; b.geometry2 = shapeB; b.pairKey = 7; b.depth = 0.09f;
  x3d::runtime::X3DExecutionContext ctx;
  ContactReporter reporter;
  reporter.addSensor(sensor.get());
  reporter.report({a, b}, ctx);
  ctx.process();
  CHECK(sensor->getContacts().size() == 1, "same pairKey deduped to one Contact");
  auto *c = sensor->getContacts().empty()
                ? nullptr
                : dynamic_cast<Contact *>(sensor->getContacts()[0].get());
  CHECK(c && c->getDepth() == 0.09f, "dedupe keeps the deepest contact");
}

// Contacts emit sorted by pairKey regardless of input order.
static void test_deterministic_ordering() {
  auto s1 = std::make_shared<CollidableShape>();
  auto s2 = std::make_shared<CollidableShape>();
  auto s3 = std::make_shared<CollidableShape>();
  auto col = std::make_shared<CollisionCollection>();
  col->setCollidables(MFNode{s1, s2, s3});
  auto sensor = std::make_shared<CollisionSensor>();
  sensor->setCollider(col);
  ResolvedContact hi; hi.geometry1 = s2; hi.geometry2 = s3; hi.pairKey = 100;
  ResolvedContact lo; lo.geometry1 = s1; lo.geometry2 = s2; lo.pairKey = 5;
  x3d::runtime::X3DExecutionContext ctx;
  ContactReporter reporter;
  reporter.addSensor(sensor.get());
  reporter.report({hi, lo}, ctx);  // deliberately out of order
  ctx.process();
  auto cs = sensor->getContacts();
  CHECK(cs.size() == 2, "two distinct-pair contacts emitted");
  // lo (pairKey 5) must come first.
  auto *first = cs.empty() ? nullptr : dynamic_cast<Contact *>(cs[0].get());
  CHECK(first && first->getGeometry1().get() == s1.get(),
        "contacts ordered by pairKey (lowest first)");
}

// isActive only changes on transitions: true once, then false once contacts stop.
static void test_isactive_transition() {
  auto shapeA = std::make_shared<CollidableShape>();
  auto shapeB = std::make_shared<CollidableShape>();
  auto col = std::make_shared<CollisionCollection>();
  col->setCollidables(MFNode{shapeA, shapeB});
  auto sensor = std::make_shared<CollisionSensor>();
  sensor->setCollider(col);
  ResolvedContact rc; rc.geometry1 = shapeA; rc.geometry2 = shapeB; rc.pairKey = 1;

  x3d::runtime::X3DExecutionContext ctx;
  ContactReporter reporter;
  reporter.addSensor(sensor.get());

  reporter.report({rc}, ctx); ctx.process();
  CHECK(sensor->getIsActive() == true, "isActive true after a contact");

  reporter.report({}, ctx); ctx.process();
  CHECK(sensor->getIsActive() == false, "isActive false after contacts stop");
}

int main() {
  test_contact_point_defaults();
  test_single_contact_emitted();
  test_null_collider_silent();
  test_disabled_collection_silent();
  test_dedupe_by_pair_keep_max_depth();
  test_deterministic_ordering();
  test_isactive_transition();
  return g_failures == 0 ? 0 : 1;
}
