// physics_system_test.cpp — gated (X3D_CPP_BUILD_PHYSICS) unit test for the CORE
// PhysicsSystem driving the Jolt backend over a programmatically-built §37 scene.
//
// Proves the end-to-end path the design specifies:
//   RigidBodyCollection -> RigidBody -> CollidableShape -> Shape -> Box
//   attach(collection) + tick():
//     (a) RigidBody.position is updated by the simulation (it fell), AND
//     (b) position_changed was DELIVERED over a wired ROUTE to a Transform's
//         translation (proving ctx.writeField -> event -> cascade, not just the
//         backend call). We assert the Transform moved, which can ONLY happen if
//         the event was routed.

#include "JoltBackend.hpp"
#include "PhysicsSystem.hpp"

#include "X3DExecutionContext.hpp"
#include "X3DSceneBridge.hpp"
#include "X3DScene.hpp"

#include "BallJoint.hpp"
#include "Box.hpp"
#include "CollidableShape.hpp"
#include "CollisionCollection.hpp"
#include "CollisionSensor.hpp"
#include "Cone.hpp"
#include "Contact.hpp"
#include "Cylinder.hpp"
#include "IndexedFaceSet.hpp"
#include "RigidBody.hpp"
#include "RigidBodyCollection.hpp"
#include "Shape.hpp"
#include "SingleAxisHingeJoint.hpp"
#include "Sphere.hpp"
#include "Transform.hpp"
#include "X3Denums.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <tuple>
#include <utility>

using namespace x3d::runtime;

static const SFRotation kNoRot{0, 0, 1, 0};
static const SFVec3f    kZero{0, 0, 0};

static int g_failures = 0;
#define CHECK(cond, msg)                                                        \
  do {                                                                          \
    if (!(cond)) {                                                              \
      std::fprintf(stderr, "FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__);      \
      ++g_failures;                                                             \
    }                                                                           \
  } while (0)

int main() {
  // ── Build a §37 scene programmatically: a single dynamic box falling.
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
  collection->setGravity(SFVec3f{0, -9.8f, 0});
  collection->setBodies(MFNode{std::static_pointer_cast<X3DNode>(body)});

  // A Transform that the body's position_changed is routed to (the wired sink).
  auto transform = std::make_shared<Transform>();
  transform->setTranslation(SFVec3f{0, 0, 0});

  Scene scene;
  scene.rootNodes.push_back(std::static_pointer_cast<X3DNode>(collection));
  scene.rootNodes.push_back(std::static_pointer_cast<X3DNode>(transform));

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  // The wired sink: RigidBody.position_changed -> Transform.set_translation.
  // Both alias to the base field names, so this routes the integrated position
  // onto the Transform's translation (proving the cascade delivery).
  ctx.addRoute(FieldAddress{body.get(), "position"},
               FieldAddress{transform.get(), "translation"});

  auto backend = std::make_shared<JoltBackend>();
  auto physics = std::make_shared<PhysicsSystem>(backend);
  physics->attach(collection.get(), ctx);
  ctx.addSystem(physics);
  CHECK(physics->bodyCount() == 1, "one body enrolled in simulation");

  // Tick 60 frames at 1/60 s (~1 s of fall). First tick establishes t0.
  double t = 0.0;
  ctx.tick(t);  // t=0: no dt yet
  for (int i = 0; i < 60; ++i) {
    t += 1.0 / 60.0;
    ctx.tick(t);
  }

  // (a) RigidBody.position was updated by simulation (it fell from y=10).
  SFVec3f bodyPos = body->getPosition();
  std::fprintf(stderr, "RigidBody.position.y = %f (started 10, expect ~5.1)\n",
               bodyPos.y);
  CHECK(bodyPos.y < 9.0f, "RigidBody fell (position updated by simulation)");
  CHECK(std::fabs(bodyPos.y - 5.1f) < 0.5f, "RigidBody near free-fall y=5.1");

  // (b) position_changed was DELIVERED to the routed Transform (cascade proof).
  SFVec3f tPos = transform->getTranslation();
  std::fprintf(stderr, "Transform.translation.y = %f (routed from body)\n",
               tPos.y);
  CHECK(std::fabs(tPos.y - bodyPos.y) < 1e-4f,
        "Transform.translation tracks RigidBody.position via the ROUTE");
  CHECK(tPos.y < 9.0f, "Transform moved (event delivered, not just backend)");

  // ── (c) §37 JOINT via the CORE PhysicsSystem path: a BallJoint pins a dynamic
  //     RigidBody to a fixed world anchor at the origin; the body starts 1 unit
  //     out and swings under gravity. Proves attach() reads RigidBodyCollection
  //     .joints, resolves body1 (body2 unset -> world anchor), and the
  //     constraint holds: the body swings (y drops) while staying ~1 unit from
  //     the anchor (it does not free-fall, does not fly off). The motion is also
  //     routed onto a Transform (cascade delivery, same as the falling case).
  {
    auto jBox = std::make_shared<Sphere>();
    jBox->setRadiusUnchecked(0.1f);
    auto jShape = std::make_shared<Shape>();
    jShape->setGeometry(std::static_pointer_cast<X3DNode>(jBox));
    auto jColl = std::make_shared<CollidableShape>();
    jColl->setShapeUnchecked(std::static_pointer_cast<X3DNode>(jShape));

    auto jBody = std::make_shared<RigidBody>();
    jBody->setMass(1.0f);
    jBody->setPosition(SFVec3f{1, 0, 0});  // 1 unit out from the anchor
    jBody->setGeometry(MFNode{std::static_pointer_cast<X3DNode>(jColl)});

    auto jJoint = std::make_shared<BallJoint>();
    jJoint->setAnchorPoint(SFVec3f{0, 0, 0});
    jJoint->setBody1(std::static_pointer_cast<X3DNode>(jBody));  // body2 unset

    auto jWorld = std::make_shared<RigidBodyCollection>();
    jWorld->setGravity(SFVec3f{0, -9.8f, 0});
    jWorld->setBodies(MFNode{std::static_pointer_cast<X3DNode>(jBody)});
    jWorld->setJoints(MFNode{std::static_pointer_cast<X3DNode>(jJoint)});

    auto jXf = std::make_shared<Transform>();

    Scene jScene;
    jScene.rootNodes.push_back(std::static_pointer_cast<X3DNode>(jWorld));
    jScene.rootNodes.push_back(std::static_pointer_cast<X3DNode>(jXf));

    X3DExecutionContext jCtx;
    jCtx.buildSceneGraph(jScene);
    jCtx.addRoute(FieldAddress{jBody.get(), "position"},
                  FieldAddress{jXf.get(), "translation"});

    auto jBackend = std::make_shared<JoltBackend>();
    auto jPhysics = std::make_shared<PhysicsSystem>(jBackend);
    jPhysics->attach(jWorld.get(), jCtx);
    jCtx.addSystem(jPhysics);
    CHECK(jPhysics->bodyCount() == 1, "joint scene: one body enrolled");

    double jt = 0.0;
    jCtx.tick(jt);
    float maxDistErr = 0.0f, minY = 0.0f;
    for (int i = 0; i < 120; ++i) {
      jt += 1.0 / 60.0;
      jCtx.tick(jt);
      SFVec3f p = jBody->getPosition();
      maxDistErr = std::max(
          maxDistErr, std::fabs(std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z) - 1.0f));
      minY = std::min(minY, p.y);
    }
    std::fprintf(stderr,
                 "[joint] BallJoint pendulum: maxDistErr=%f minY=%f\n",
                 maxDistErr, minY);
    CHECK(maxDistErr < 0.05f, "ball joint holds body ~1 unit from anchor");
    CHECK(minY < -0.3f, "ball joint body swings down under gravity");
    // The swing is routed to the Transform (cascade delivery, not just backend).
    SFVec3f jTr = jXf->getTranslation();
    CHECK(std::fabs(jTr.y - jBody->getPosition().y) < 1e-4f,
          "joint motion routed onto the Transform via the ROUTE");
  }

  // ── (d) per-body enabled=FALSE excludes the body from the simulation.
  //     §37: a RigidBody with enabled FALSE "is not part of the calculations."
  //     A disabled body must NOT be enrolled (and must not move).
  {
    auto dBox = std::make_shared<Box>();
    dBox->setSizeUnchecked(SFVec3f{1, 1, 1});
    auto dShape = std::make_shared<Shape>();
    dShape->setGeometry(std::static_pointer_cast<X3DNode>(dBox));
    auto dColl = std::make_shared<CollidableShape>();
    dColl->setShapeUnchecked(std::static_pointer_cast<X3DNode>(dShape));

    auto disabled = std::make_shared<RigidBody>();
    disabled->setMass(1.0f);
    disabled->setPosition(SFVec3f{0, 10, 0});
    disabled->setEnabled(false);  // <-- excluded from the simulation
    disabled->setGeometry(MFNode{std::static_pointer_cast<X3DNode>(dColl)});

    auto dWorld = std::make_shared<RigidBodyCollection>();
    dWorld->setGravity(SFVec3f{0, -9.8f, 0});
    dWorld->setBodies(MFNode{std::static_pointer_cast<X3DNode>(disabled)});

    Scene dScene;
    dScene.rootNodes.push_back(std::static_pointer_cast<X3DNode>(dWorld));
    X3DExecutionContext dCtx;
    dCtx.buildSceneGraph(dScene);

    auto dBackend = std::make_shared<JoltBackend>();
    auto dPhysics = std::make_shared<PhysicsSystem>(dBackend);
    dPhysics->attach(dWorld.get(), dCtx);
    CHECK(dPhysics->bodyCount() == 0,
          "enabled=FALSE RigidBody is excluded from the simulation");

    // And it must not move when stepped.
    dCtx.addSystem(dPhysics);
    double dt = 0.0;
    dCtx.tick(dt);
    for (int i = 0; i < 30; ++i) {
      dt += 1.0 / 60.0;
      dCtx.tick(dt);
    }
    CHECK(std::fabs(disabled->getPosition().y - 10.0f) < 1e-4f,
          "disabled RigidBody does not fall (not simulated)");
  }

  // ── (e) a body whose collidable geometry is an unsupported mesh (IndexedFaceSet,
  //     no analytic primitive) is excluded — but OBSERVABLY (droppedBodyCount),
  //     not silently dropped. (Box/Sphere/Cylinder/Cone are supported.)
  {
    auto mesh = std::make_shared<IndexedFaceSet>();  // unsupported mesh shape
    auto cShape = std::make_shared<Shape>();
    cShape->setGeometry(std::static_pointer_cast<X3DNode>(mesh));
    auto cColl = std::make_shared<CollidableShape>();
    cColl->setShapeUnchecked(std::static_pointer_cast<X3DNode>(cShape));

    auto cBody = std::make_shared<RigidBody>();
    cBody->setMass(1.0f);
    cBody->setGeometry(MFNode{std::static_pointer_cast<X3DNode>(cColl)});

    auto cWorld = std::make_shared<RigidBodyCollection>();
    cWorld->setBodies(MFNode{std::static_pointer_cast<X3DNode>(cBody)});

    Scene cScene;
    cScene.rootNodes.push_back(std::static_pointer_cast<X3DNode>(cWorld));
    X3DExecutionContext cCtx;
    cCtx.buildSceneGraph(cScene);

    auto cBackend = std::make_shared<JoltBackend>();
    auto cPhysics = std::make_shared<PhysicsSystem>(cBackend);
    cPhysics->attach(cWorld.get(), cCtx);
    CHECK(cPhysics->bodyCount() == 0, "unsupported-geometry body not enrolled");
    CHECK(cPhysics->droppedBodyCount() == 1,
          "dropped body is observable via droppedBodyCount(), not silent");
  }

  // ── (f) RigidBody.forces applies a continuous force every frame (§37: forces
  //     "are applied to the object every presentation frame ... continuously
  //     applied until reset"). With zero gravity, a constant upward force of 10 N
  //     on a 1 kg body accelerates it: after 1 s, y = ½·a·t² = ½·10·1² = 5.
  {
    auto fBox = std::make_shared<Box>();
    fBox->setSizeUnchecked(SFVec3f{1, 1, 1});
    auto fShape = std::make_shared<Shape>();
    fShape->setGeometry(std::static_pointer_cast<X3DNode>(fBox));
    auto fColl = std::make_shared<CollidableShape>();
    fColl->setShapeUnchecked(std::static_pointer_cast<X3DNode>(fShape));

    auto fBody = std::make_shared<RigidBody>();
    fBody->setMass(1.0f);
    fBody->setPosition(SFVec3f{0, 0, 0});
    fBody->setForces(MFVec3f{SFVec3f{0, 10, 0}});  // constant 10 N up
    fBody->setGeometry(MFNode{std::static_pointer_cast<X3DNode>(fColl)});

    auto fWorld = std::make_shared<RigidBodyCollection>();
    fWorld->setGravity(SFVec3f{0, 0, 0});  // zero gravity: isolate the force
    fWorld->setBodies(MFNode{std::static_pointer_cast<X3DNode>(fBody)});

    Scene fScene;
    fScene.rootNodes.push_back(std::static_pointer_cast<X3DNode>(fWorld));
    X3DExecutionContext fCtx;
    fCtx.buildSceneGraph(fScene);

    auto fBackend = std::make_shared<JoltBackend>();
    auto fPhysics = std::make_shared<PhysicsSystem>(fBackend);
    fPhysics->attach(fWorld.get(), fCtx);
    fCtx.addSystem(fPhysics);

    double ft = 0.0;
    fCtx.tick(ft);
    for (int i = 0; i < 60; ++i) {
      ft += 1.0 / 60.0;
      fCtx.tick(ft);
    }
    std::fprintf(stderr, "[forces] y after 1s under F=10N = %f (expect ~5)\n",
                 fBody->getPosition().y);
    CHECK(fBody->getPosition().y > 3.0f,
          "constant RigidBody.forces accelerates the body upward");
    CHECK(std::fabs(fBody->getPosition().y - 5.0f) < 1.0f,
          "force integrates to ~½·a·t² (a=F/m=10)");
  }

  // ── (g) useGlobalGravity=FALSE: the body ignores the collection's gravity
  //     (§37.4.10). In a -9.8 field, a body with useGlobalGravity FALSE does not
  //     fall, while the default (TRUE) body would.
  {
    auto gBox = std::make_shared<Box>();
    gBox->setSizeUnchecked(SFVec3f{1, 1, 1});
    auto gShape = std::make_shared<Shape>();
    gShape->setGeometry(std::static_pointer_cast<X3DNode>(gBox));
    auto gColl = std::make_shared<CollidableShape>();
    gColl->setShapeUnchecked(std::static_pointer_cast<X3DNode>(gShape));

    auto gBody = std::make_shared<RigidBody>();
    gBody->setMass(1.0f);
    gBody->setPosition(SFVec3f{0, 5, 0});
    gBody->setUseGlobalGravity(false);  // <-- opt out of the field
    gBody->setGeometry(MFNode{std::static_pointer_cast<X3DNode>(gColl)});

    auto gWorld = std::make_shared<RigidBodyCollection>();
    gWorld->setGravity(SFVec3f{0, -9.8f, 0});
    gWorld->setBodies(MFNode{std::static_pointer_cast<X3DNode>(gBody)});

    Scene gScene;
    gScene.rootNodes.push_back(std::static_pointer_cast<X3DNode>(gWorld));
    X3DExecutionContext gCtx;
    gCtx.buildSceneGraph(gScene);
    auto gBackend = std::make_shared<JoltBackend>();
    auto gPhysics = std::make_shared<PhysicsSystem>(gBackend);
    gPhysics->attach(gWorld.get(), gCtx);
    gCtx.addSystem(gPhysics);
    double gt = 0.0;
    gCtx.tick(gt);
    for (int i = 0; i < 60; ++i) {
      gt += 1.0 / 60.0;
      gCtx.tick(gt);
    }
    std::fprintf(stderr, "[useGlobalGravity=F] y after 1s = %f (expect ~5)\n",
                 gBody->getPosition().y);
    CHECK(std::fabs(gBody->getPosition().y - 5.0f) < 0.1f,
          "useGlobalGravity=FALSE body ignores gravity (does not fall)");
  }

  // ── (h) autoDamp: the damping factors apply a velocity-opposing force
  //     (§37.4.10 worked example: force = factor × velocity × -1). In zero
  //     gravity a body launched up at 5 m/s with autoDamp + linearDampingFactor=5
  //     decays quickly (v(t)=5·e^-5t → y(1s)≈1), vs ~5 undamped.
  {
    auto hBox = std::make_shared<Box>();
    hBox->setSizeUnchecked(SFVec3f{1, 1, 1});
    auto hShape = std::make_shared<Shape>();
    hShape->setGeometry(std::static_pointer_cast<X3DNode>(hBox));
    auto hColl = std::make_shared<CollidableShape>();
    hColl->setShapeUnchecked(std::static_pointer_cast<X3DNode>(hShape));

    auto hBody = std::make_shared<RigidBody>();
    hBody->setMass(1.0f);
    hBody->setPosition(SFVec3f{0, 0, 0});
    hBody->setLinearVelocity(SFVec3f{0, 5, 0});
    hBody->setAutoDamp(true);
    hBody->setLinearDampingFactor(5.0f);
    hBody->setGeometry(MFNode{std::static_pointer_cast<X3DNode>(hColl)});

    auto hWorld = std::make_shared<RigidBodyCollection>();
    hWorld->setGravity(SFVec3f{0, 0, 0});  // zero gravity: isolate damping
    hWorld->setBodies(MFNode{std::static_pointer_cast<X3DNode>(hBody)});

    Scene hScene;
    hScene.rootNodes.push_back(std::static_pointer_cast<X3DNode>(hWorld));
    X3DExecutionContext hCtx;
    hCtx.buildSceneGraph(hScene);
    auto hBackend = std::make_shared<JoltBackend>();
    auto hPhysics = std::make_shared<PhysicsSystem>(hBackend);
    hPhysics->attach(hWorld.get(), hCtx);
    hCtx.addSystem(hPhysics);
    double ht = 0.0;
    hCtx.tick(ht);
    for (int i = 0; i < 60; ++i) {
      ht += 1.0 / 60.0;
      hCtx.tick(ht);
    }
    std::fprintf(stderr, "[autoDamp] y after 1s = %f (expect ~1, undamped ~5)\n",
                 hBody->getPosition().y);
    CHECK(hBody->getPosition().y > 0.3f, "damped body still moved up some");
    CHECK(hBody->getPosition().y < 2.5f,
          "autoDamp decays the motion (well short of the undamped ~5)");
  }

  // ── (i) Cylinder + Cone collidable geometry now enroll (Cylinder exact, Cone
  //     via an analytic AABB box) instead of being dropped — both fall.
  {
    auto cyl = std::make_shared<Cylinder>();
    cyl->setRadiusUnchecked(0.5f);
    cyl->setHeightUnchecked(2.0f);
    auto cylShape = std::make_shared<Shape>();
    cylShape->setGeometry(std::static_pointer_cast<X3DNode>(cyl));
    auto cylColl = std::make_shared<CollidableShape>();
    cylColl->setShapeUnchecked(std::static_pointer_cast<X3DNode>(cylShape));
    auto cylBody = std::make_shared<RigidBody>();
    cylBody->setMass(1.0f);
    cylBody->setPosition(SFVec3f{0, 10, 0});
    cylBody->setGeometry(MFNode{std::static_pointer_cast<X3DNode>(cylColl)});

    auto cone = std::make_shared<Cone>();
    cone->setBottomRadiusUnchecked(0.5f);
    cone->setHeightUnchecked(1.0f);
    auto coneShape = std::make_shared<Shape>();
    coneShape->setGeometry(std::static_pointer_cast<X3DNode>(cone));
    auto coneColl = std::make_shared<CollidableShape>();
    coneColl->setShapeUnchecked(std::static_pointer_cast<X3DNode>(coneShape));
    auto coneBody = std::make_shared<RigidBody>();
    coneBody->setMass(1.0f);
    coneBody->setPosition(SFVec3f{3, 10, 0});
    coneBody->setGeometry(MFNode{std::static_pointer_cast<X3DNode>(coneColl)});

    auto sWorld = std::make_shared<RigidBodyCollection>();
    sWorld->setGravity(SFVec3f{0, -9.8f, 0});
    sWorld->setBodies(MFNode{std::static_pointer_cast<X3DNode>(cylBody),
                             std::static_pointer_cast<X3DNode>(coneBody)});

    Scene sScene;
    sScene.rootNodes.push_back(std::static_pointer_cast<X3DNode>(sWorld));
    X3DExecutionContext sCtx;
    sCtx.buildSceneGraph(sScene);
    auto sBackend = std::make_shared<JoltBackend>();
    auto sPhysics = std::make_shared<PhysicsSystem>(sBackend);
    sPhysics->attach(sWorld.get(), sCtx);
    sCtx.addSystem(sPhysics);
    CHECK(sPhysics->bodyCount() == 2, "Cylinder + Cone bodies both enrolled");
    CHECK(sPhysics->droppedBodyCount() == 0, "no body dropped for Cylinder/Cone");
    double st = 0.0;
    sCtx.tick(st);
    for (int i = 0; i < 30; ++i) {
      st += 1.0 / 60.0;
      sCtx.tick(st);
    }
    CHECK(cylBody->getPosition().y < 9.5f, "cylinder body fell (simulated)");
    CHECK(coneBody->getPosition().y < 9.5f, "cone body fell (simulated)");
  }

  // ── Collision reporting: a dynamic box falls onto a fixed floor; a
  //    CollisionSensor watching both collidables must emit a Contact + isActive.
  {
    auto floorBox = std::make_shared<Box>();
    floorBox->setSizeUnchecked(SFVec3f{10, 1, 10});
    auto floorShapeWrap = std::make_shared<Shape>();
    floorShapeWrap->setGeometry(std::static_pointer_cast<X3DNode>(floorBox));
    auto floorColl = std::make_shared<CollidableShape>();
    floorColl->setShapeUnchecked(std::static_pointer_cast<X3DNode>(floorShapeWrap));
    auto floor = std::make_shared<RigidBody>();
    floor->setFixed(true);
    floor->setPosition(SFVec3f{0, 0, 0});
    floor->setGeometry(MFNode{std::static_pointer_cast<X3DNode>(floorColl)});

    auto fallBox = std::make_shared<Box>();
    fallBox->setSizeUnchecked(SFVec3f{1, 1, 1});
    auto fallShapeWrap = std::make_shared<Shape>();
    fallShapeWrap->setGeometry(std::static_pointer_cast<X3DNode>(fallBox));
    auto fallColl = std::make_shared<CollidableShape>();
    fallColl->setShapeUnchecked(std::static_pointer_cast<X3DNode>(fallShapeWrap));
    auto faller = std::make_shared<RigidBody>();
    faller->setMass(1);
    faller->setPosition(SFVec3f{0, 3, 0});
    faller->setGeometry(MFNode{std::static_pointer_cast<X3DNode>(fallColl)});

    auto collection = std::make_shared<RigidBodyCollection>();
    collection->setGravity(SFVec3f{0, -9.8f, 0});
    collection->setBodies(MFNode{std::static_pointer_cast<X3DNode>(floor),
                                 std::static_pointer_cast<X3DNode>(faller)});

    auto collCol = std::make_shared<CollisionCollection>();
    collCol->setBounce(0.2f);
    collCol->setCollidables(MFNode{std::static_pointer_cast<X3DNode>(floorColl),
                                   std::static_pointer_cast<X3DNode>(fallColl)});
    auto sensor = std::make_shared<CollisionSensor>();
    sensor->setCollider(collCol);

    X3DExecutionContext ctx;
    auto physics = std::make_shared<PhysicsSystem>(std::make_shared<JoltBackend>());
    physics->attach(collection.get(), ctx);
    physics->attach(sensor.get(), ctx);
    ctx.addSystem(physics);

    double t = 0.0;
    ctx.tick(t);
    bool sawContact = false;
    for (int i = 0; i < 120 && !sawContact; ++i) {  // up to 2 s
      t += 1.0 / 60.0;
      ctx.tick(t);
      if (!sensor->getContacts().empty()) sawContact = true;
    }
    CHECK(sawContact, "CollisionSensor emitted a Contact when boxes collided");
    CHECK(sensor->getIsActive() == true, "CollisionSensor isActive on collision");
    auto cs = sensor->getContacts();
    if (!cs.empty()) {
      auto *c = dynamic_cast<Contact *>(cs[0].get());
      CHECK(c != nullptr, "emitted node is a Contact");
      if (c) {
        // The contact normal should be roughly vertical (floor vs faller).
        CHECK(std::fabs(c->getContactNormal().y) > 0.5f,
              "Contact normal is roughly vertical");
        CHECK(c->getBounce() == 0.2f, "Contact.bounce from collection default");
      }
    }
  }

  // Determinism: a second identical run must yield the same first-contact data.
  {
    auto run_once = []() -> std::tuple<float, float, float> {
      auto up = [](auto p) { return std::static_pointer_cast<X3DNode>(p); };
      auto floorBox = std::make_shared<Box>(); floorBox->setSizeUnchecked(SFVec3f{10,1,10});
      auto fsw = std::make_shared<Shape>(); fsw->setGeometry(up(floorBox));
      auto fc = std::make_shared<CollidableShape>(); fc->setShapeUnchecked(up(fsw));
      auto floor = std::make_shared<RigidBody>(); floor->setFixed(true);
      floor->setPosition(SFVec3f{0,0,0}); floor->setGeometry(MFNode{up(fc)});
      auto db = std::make_shared<Box>(); db->setSizeUnchecked(SFVec3f{1,1,1});
      auto dsw = std::make_shared<Shape>(); dsw->setGeometry(up(db));
      auto dc = std::make_shared<CollidableShape>(); dc->setShapeUnchecked(up(dsw));
      auto faller = std::make_shared<RigidBody>(); faller->setMass(1);
      faller->setPosition(SFVec3f{0,3,0}); faller->setGeometry(MFNode{up(dc)});
      auto col = std::make_shared<RigidBodyCollection>();
      col->setGravity(SFVec3f{0,-9.8f,0}); col->setBodies(MFNode{up(floor), up(faller)});
      auto cc = std::make_shared<CollisionCollection>();
      cc->setCollidables(MFNode{up(fc), up(dc)});
      auto sensor = std::make_shared<CollisionSensor>(); sensor->setCollider(cc);
      X3DExecutionContext ctx;
      auto phys = std::make_shared<PhysicsSystem>(std::make_shared<JoltBackend>());
      phys->attach(col.get(), ctx); phys->attach(sensor.get(), ctx);
      ctx.addSystem(phys);
      double t = 0.0; ctx.tick(t);
      for (int i = 0; i < 120; ++i) {
        t += 1.0/60.0; ctx.tick(t);
        if (!sensor->getContacts().empty()) break;
      }
      auto cs = sensor->getContacts();
      if (cs.empty()) return {999.f, 999.f, 999.f};
      auto *c = dynamic_cast<Contact *>(cs[0].get());
      return {c->getPosition().y, c->getContactNormal().y, c->getDepth()};
    };
    auto r1 = run_once();
    auto r2 = run_once();
    CHECK(r1 == r2, "collision reporting is deterministic (two runs identical)");
  }

  // ── Inertia honored: under a constant torque about x, the roll angle after t
  //    must match theta = 1/2 * (tau/Ixx) * t^2, AND differ from the shape-derived
  //    (default-inertia) case — proving the authored tensor is actually used.
  {
    auto run = [](bool authored) -> float {
      auto box = std::make_shared<Box>();
      box->setSizeUnchecked(SFVec3f{1, 0.2f, 1});            // flat frame
      auto shp = std::make_shared<Shape>();
      shp->setGeometry(std::static_pointer_cast<X3DNode>(box));
      auto coll = std::make_shared<CollidableShape>();
      coll->setShapeUnchecked(std::static_pointer_cast<X3DNode>(shp));
      auto body = std::make_shared<RigidBody>();
      body->setMass(1.0f);
      if (authored) {
        SFMatrix3f I{};
        I.matrix[0][0] = 0.01f; I.matrix[1][1] = 0.02f; I.matrix[2][2] = 0.01f;
        body->setInertia(I);                                 // Ixx = 0.01
      }
      body->setTorques(std::vector<SFVec3f>{SFVec3f{0.02f, 0, 0}});  // tau_x
      body->setGeometry(MFNode{std::static_pointer_cast<X3DNode>(coll)});
      auto world = std::make_shared<RigidBodyCollection>();
      world->setGravity(SFVec3f{0, 0, 0});                   // isolate rotation
      world->setBodies(MFNode{std::static_pointer_cast<X3DNode>(body)});
      X3DExecutionContext ctx;
      auto phys = std::make_shared<PhysicsSystem>(std::make_shared<JoltBackend>());
      phys->attach(world.get(), ctx);
      ctx.addSystem(phys);
      double t = 0.0; ctx.tick(t);
      for (int i = 0; i < 30; ++i) { t += 1.0 / 60.0; ctx.tick(t); } // 0.5 s
      return body->getOrientation().angle;                  // roll about x
    };
    float authored = run(true);
    float derived = run(false);
    std::fprintf(stderr, "[inertia] authored roll=%.4f derived roll=%.4f (expect authored~0.25)\n",
                 authored, derived);
    // theta = 1/2 * (0.02/0.01) * 0.5^2 = 0.25 rad (within discrete-integration tol).
    CHECK(std::fabs(authored - 0.25f) < 0.05f,
          "authored inertia: roll matches tau/Ixx (theta ~= 0.25 rad)");
    CHECK(std::fabs(authored - derived) > 0.05f,
          "authored inertia DIFFERS from shape-derived (tensor is honored)");
  }

  // ── centerOfMass honored: a body given a pure spin (no forces/gravity) with an
  //    offset COM has its reported ORIGIN orbit the COM (origin != COM), while a
  //    centered body's origin stays put. Requires getBodyTransform = GetPosition.
  {
    auto originMoved = [](bool offset) -> float {
      auto box = std::make_shared<Box>();
      box->setSizeUnchecked(SFVec3f{1, 1, 1});
      auto shp = std::make_shared<Shape>();
      shp->setGeometry(std::static_pointer_cast<X3DNode>(box));
      auto coll = std::make_shared<CollidableShape>();
      coll->setShapeUnchecked(std::static_pointer_cast<X3DNode>(shp));
      auto body = std::make_shared<RigidBody>();
      body->setMass(1.0f);
      if (offset) body->setCenterOfMass(SFVec3f{0.5f, 0, 0});
      body->setAngularVelocity(SFVec3f{0, 0, 4});            // spin about z
      body->setGeometry(MFNode{std::static_pointer_cast<X3DNode>(coll)});
      auto world = std::make_shared<RigidBodyCollection>();
      world->setGravity(SFVec3f{0, 0, 0});
      world->setBodies(MFNode{std::static_pointer_cast<X3DNode>(body)});
      X3DExecutionContext ctx;
      auto phys = std::make_shared<PhysicsSystem>(std::make_shared<JoltBackend>());
      phys->attach(world.get(), ctx);
      ctx.addSystem(phys);
      double t = 0.0; ctx.tick(t);
      for (int i = 0; i < 15; ++i) { t += 1.0 / 60.0; ctx.tick(t); }
      const SFVec3f p = body->getPosition();
      return std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z); // origin displacement
    };
    float displacedOffset = originMoved(true);
    float displacedCentered = originMoved(false);
    std::fprintf(stderr, "[COM] offset origin displacement=%.4f centered=%.4f\n",
                 displacedOffset, displacedCentered);
    CHECK(displacedOffset > 0.05f,
          "offset COM: body origin orbits the COM under spin");
    CHECK(displacedCentered < 1e-3f,
          "centered COM: body origin stays put under spin");
  }

  // ── (j) Rear-wheel-drive vehicle: chassis + 4 cylinder wheels on free hinge
  //     axles. With appliedParameters including FRICTION_COEFFICIENT-2 and
  //     frictionCoefficients (1,1) the rear tyres grip the ground and drive the
  //     vehicle forward; with default appliedParameters {BOUNCE} only (no
  //     FRICTION_COEFFICIENT-2 selector) Jolt's 0.2 default applies, and it
  //     barely moves. Proves the §37 appliedParameters-gated friction path
  //     end-to-end through PhysicsSystem. Also asserts the vehicle tracks
  //     straight, stays upright, and is deterministic.
  {
    struct DriveResult { float dz, dx, dy; };
    // gripFriction=true:  appliedParameters={BOUNCE, FRICTION_COEFFICIENT-2} +
    //                     frictionCoefficients={1,1} → authored friction applied.
    // gripFriction=false: appliedParameters default {BOUNCE} only (no selector)
    //                     + frictionCoefficients={1,1} → FRICTION_COEFFICIENT-2
    //                     selector absent → Jolt 0.2 default friction (slip).
    auto driveDistance = [](bool gripFriction) -> DriveResult {
      auto up = [](auto p) { return std::static_pointer_cast<X3DNode>(p); };
      auto makeBox = [&](SFVec3f half) {
        auto b = std::make_shared<Box>();
        b->setSizeUnchecked(SFVec3f{half.x * 2, half.y * 2, half.z * 2});
        auto s = std::make_shared<Shape>(); s->setGeometry(up(b));
        auto c = std::make_shared<CollidableShape>(); c->setShapeUnchecked(up(s));
        return c;
      };
      auto makeCyl = [&]() {
        auto cy = std::make_shared<Cylinder>();
        cy->setRadiusUnchecked(0.4f); cy->setHeightUnchecked(0.2f);
        auto s = std::make_shared<Shape>(); s->setGeometry(up(cy));
        auto c = std::make_shared<CollidableShape>(); c->setShapeUnchecked(up(s));
        return c;
      };
      auto ground = std::make_shared<RigidBody>();
      ground->setFixed(true); ground->setPosition(SFVec3f{0, -0.5f, 0});
      ground->setGeometry(MFNode{up(makeBox(SFVec3f{25, 0.5f, 25}))});
      auto chassis = std::make_shared<RigidBody>();
      chassis->setMass(5); chassis->setPosition(SFVec3f{0, 0.5f, 0});
      chassis->setGeometry(MFNode{up(makeBox(SFVec3f{0.5f, 0.2f, 1.0f}))});
      const SFRotation wheelOri{0, 0, 1, -1.5707963f};
      const float wx = 0.6f, wy = 0.4f, wz = 0.8f;
      SFVec3f ctr[4] = {{-wx,wy,wz},{wx,wy,wz},{-wx,wy,-wz},{wx,wy,-wz}};
      MFNode bodies{up(ground), up(chassis)};
      MFNode joints;
      for (int i = 0; i < 4; ++i) {
        auto wheel = std::make_shared<RigidBody>();
        wheel->setMass(1); wheel->setPosition(ctr[i]); wheel->setOrientation(wheelOri);
        wheel->setGeometry(MFNode{up(makeCyl())});
        if (i >= 2) wheel->setTorques(MFVec3f{SFVec3f{3, 0, 0}});  // rear wheels
        bodies.push_back(up(wheel));
        auto hinge = std::make_shared<SingleAxisHingeJoint>();
        hinge->setBody1(up(wheel)); hinge->setBody2(up(chassis));
        hinge->setAxis(SFVec3f{1, 0, 0}); hinge->setAnchorPoint(ctr[i]);
        joints.push_back(up(hinge));
      }
      auto cc = std::make_shared<CollisionCollection>();
      // Both grip and slip runs author frictionCoefficients (1,1); the
      // DIFFERENCE is appliedParameters: grip selects FRICTION_COEFFICIENT-2,
      // slip uses the default {BOUNCE} only (no friction selector).
      cc->setFrictionCoefficients(SFVec2f{1.0f, 1.0f});
      if (gripFriction) {
        cc->setAppliedParameters(std::vector<AppliedParametersChoices>{
            AppliedParametersChoices::BOUNCE,
            AppliedParametersChoices::FRICTION_COEFFICIENT_2});
      }
      // slip: leave appliedParameters default {BOUNCE} — no FRICTION_COEFFICIENT-2
      // selector → Jolt 0.2 default applies.
      auto world = std::make_shared<RigidBodyCollection>();
      world->setGravity(SFVec3f{0, -9.8f, 0});
      world->setBodies(bodies); world->setJoints(joints);
      world->setColliderUnchecked(up(cc));

      X3DExecutionContext ctx;
      auto phys = std::make_shared<PhysicsSystem>(std::make_shared<JoltBackend>());
      phys->attach(world.get(), ctx);
      ctx.addSystem(phys);
      double t = 0.0; ctx.tick(t);
      for (int i = 0; i < 30; ++i) { t += 1.0 / 60.0; ctx.tick(t); }  // 0.5 s settle
      const SFVec3f settle = chassis->getPosition();
      for (int i = 0; i < 180; ++i) { t += 1.0 / 60.0; ctx.tick(t); } // 3 s drive
      const SFVec3f end = chassis->getPosition();
      DriveResult r{end.z - settle.z, end.x - settle.x, end.y - settle.y};
      std::fprintf(stderr,
                   "vehicle grip=%s: dz=%.3f dx=%.3f dy=%.3f\n",
                   gripFriction ? "true" : "false", r.dz, r.dx, r.dy);
      return r;
    };

    DriveResult grip  = driveDistance(true);
    DriveResult slip  = driveDistance(false);
    DriveResult grip2 = driveDistance(true);

    CHECK(grip.dz > 2.0f, "high-friction vehicle drives forward (>2 m)");
    CHECK(std::fabs(grip.dx) < 0.3f, "vehicle tracks straight (small dx)");
    CHECK(std::fabs(grip.dy) < 0.2f, "vehicle stays upright (y ~ constant)");
    CHECK(grip.dz > slip.dz + 1.0f, "authored friction drives farther than mu=0.2");
    CHECK(std::fabs(grip.dz - grip2.dz) < 1e-3f, "vehicle drive is deterministic");
  }

  // ── Vehicle dynamics (backend-level, mirrors the validated spike): skid-steer
  //    pivot, incline climb, brake/reverse. Reuses the existing vehicle geometry;
  //    friction via setContactResponse. The turn is a skid-steer PIVOT (un-steered
  //    gripping wheels can't roll an Ackermann arc — gentle differential yaws ~1°).
  {
    struct Veh { x3d::runtime::WorldHandle world; x3d::runtime::BodyHandle chassis;
                 x3d::runtime::BodyHandle wheels[4]; };
    auto build = [&](JoltBackend &b, SFRotation groundOri) -> Veh {
      WorldHandle world = b.createWorld(SFVec3f{0, -9.8f, 0});
      b.setContactResponse(world, 1.0f, -1.0f);  // friction 1.0
      b.addBody(world, ShapeDesc::box(SFVec3f{25, 0.5f, 25}), MassProperties{0.0f},
                true, SFVec3f{0, -0.5f, 0}, groundOri, kZero, kZero);
      BodyHandle chassis =
          b.addBody(world, ShapeDesc::box(SFVec3f{0.5f, 0.2f, 1.0f}),
                    MassProperties{5.0f}, false, SFVec3f{0, 0.5f, 0}, kNoRot, kZero,
                    kZero);
      const SFRotation wo{0, 0, 1, -1.5707963f};
      const float wx = 0.6f, wy = 0.4f, wz = 0.8f;
      SFVec3f ctr[4] = {{-wx,wy,wz},{wx,wy,wz},{-wx,wy,-wz},{wx,wy,-wz}};
      Veh v; v.world = world; v.chassis = chassis;
      for (int i = 0; i < 4; ++i) {
        v.wheels[i] = b.addBody(world, ShapeDesc::cylinder(0.4f, 0.1f),
                                MassProperties{1.0f}, false, ctr[i], wo, kZero, kZero);
        ConstraintDesc d; d.kind = ConstraintDesc::Kind::Hinge;
        d.bodyA = v.wheels[i]; d.bodyB = chassis; d.anchor = ctr[i];
        d.axis = SFVec3f{1,0,0}; d.minLimit = -3.14159265f; d.maxLimit = 3.14159265f;
        b.addConstraint(world, d);
      }
      return v;
    };
    auto chassisPose = [&](JoltBackend &b, const Veh &v) {
      SFVec3f p{0,0,0}; SFRotation o = kNoRot;
      b.getBodyTransform(v.world, v.chassis, p, o); return std::make_pair(p, o);
    };

    // (a) Skid-steer pivot: left wheels +16, right wheels -16 -> yaw in place.
    auto pivotRun = [&]() {
      JoltBackend b; Veh v = build(b, kNoRot);
      for (int i = 0; i < 30; ++i) b.step(v.world, 1.0 / 60.0);  // settle
      auto p0 = chassisPose(b, v);
      for (int i = 0; i < 180; ++i) {
        // applyForce(world, body, force, torque): zero force, opposing axle TORQUE
        // (left wheels +16, right -16) -> skid-steer yaw.
        b.applyForce(v.world, v.wheels[0], kZero, SFVec3f{16,0,0});
        b.applyForce(v.world, v.wheels[2], kZero, SFVec3f{16,0,0});
        b.applyForce(v.world, v.wheels[1], kZero, SFVec3f{-16,0,0});
        b.applyForce(v.world, v.wheels[3], kZero, SFVec3f{-16,0,0});
        b.step(v.world, 1.0 / 60.0);
      }
      auto p1 = chassisPose(b, v);
      return std::make_pair(p0, p1);
    };
    {
      auto [p0, p1] = pivotRun();
      const float yaw = p1.second.angle;
      const float dx = p1.first.x - p0.first.x, dz = p1.first.z - p0.first.z;
      std::fprintf(stderr,
                   "vehicle pivot: yaw=%.3f axisY=%.2f d=(%.2f,%.2f)\n",
                   yaw, p1.second.y, dx, dz);
      CHECK(std::fabs(yaw) > 1.0f, "skid-steer pivot yaws the chassis (>1 rad)");
      CHECK(std::fabs(p1.second.y) > 0.9f, "pivot is about +Y (upright, pure yaw)");
      CHECK(std::fabs(dx) < 0.5f && std::fabs(dz) < 0.5f,
            "pivot stays in place (no significant translation)");
    }

    // (b) Incline climb: ground tilted ~7deg; drive uphill (-6) gains height; a
    //     no-drive control on the same slope does not climb.
    auto inclineDy = [&](float torque) {
      JoltBackend b; Veh v = build(b, SFRotation{1, 0, 0, 0.12f});
      for (int i = 0; i < 60; ++i) b.step(v.world, 1.0 / 60.0);  // settle on slope
      auto p0 = chassisPose(b, v);
      for (int i = 0; i < 180; ++i) {
        b.applyForce(v.world, v.wheels[2], kZero, SFVec3f{torque,0,0});
        b.applyForce(v.world, v.wheels[3], kZero, SFVec3f{torque,0,0});
        b.step(v.world, 1.0 / 60.0);
      }
      return chassisPose(b, v).first.y - p0.first.y;
    };
    {
      float climb = inclineDy(-6.0f);   // uphill
      float coast = inclineDy(0.0f);    // no drive
      std::fprintf(stderr, "vehicle incline: climb dy=%.3f vs coast dy=%.3f\n",
                   climb, coast);
      CHECK(climb > 0.3f, "vehicle climbs the incline under drive (dy > 0.3)");
      CHECK(climb > coast + 0.3f, "drive climbs higher than coasting on the slope");
    }

    // (c) Brake/reverse: forward (+3), record peak z, then reverse (-6) past start.
    {
      JoltBackend b; Veh v = build(b, kNoRot);
      for (int i = 0; i < 30; ++i) b.step(v.world, 1.0 / 60.0);
      for (int i = 0; i < 90; ++i) {
        b.applyForce(v.world, v.wheels[2], kZero, SFVec3f{3,0,0});
        b.applyForce(v.world, v.wheels[3], kZero, SFVec3f{3,0,0});
        b.step(v.world, 1.0 / 60.0);
      }
      float peakZ = chassisPose(b, v).first.z;
      for (int i = 0; i < 180; ++i) {
        b.applyForce(v.world, v.wheels[2], kZero, SFVec3f{-6,0,0});
        b.applyForce(v.world, v.wheels[3], kZero, SFVec3f{-6,0,0});
        b.step(v.world, 1.0 / 60.0);
      }
      float finalZ = chassisPose(b, v).first.z;
      std::fprintf(stderr, "vehicle brake/reverse: peakZ=%.2f finalZ=%.2f\n",
                   peakZ, finalZ);
      CHECK(peakZ > 0.5f, "vehicle drove forward before braking");
      CHECK(finalZ < peakZ - 1.0f, "reverse torque arrests and reverses the vehicle");
    }

    // (d) Determinism: the pivot maneuver is value-identical across two runs.
    {
      auto a = pivotRun(); auto c = pivotRun();
      CHECK(std::fabs(a.second.second.angle - c.second.second.angle) < 1e-4f,
            "vehicle pivot is deterministic (two runs identical yaw)");
    }
  }

  if (g_failures == 0)
    std::fprintf(stderr, "physics_system_test: ALL PASS\n");
  return g_failures == 0 ? 0 : 1;
}
