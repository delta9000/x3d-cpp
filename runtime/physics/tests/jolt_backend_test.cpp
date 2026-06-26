// jolt_backend_test.cpp — gated (X3D_CPP_BUILD_PHYSICS) unit test for the Jolt
// PhysicsBackend implementation. Proves the engine itself integrates correctly:
//   (1) free-fall: a dynamic body at y=10 under gravity (0,-9.8,0), stepped 60x
//       at 1/60 s, lands near y = 10 - 1/2*9.8*1^2 = 5.1.
//   (2) rest-on-ground: a dynamic sphere dropped onto a fixed ground body comes
//       to rest at ~the sphere radius above the ground top (does not tunnel).
//   (3) determinism: two identical sims produce bit-identical final positions.

#include "JoltBackend.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace x3d::runtime;

static int g_failures = 0;
#define CHECK(cond, msg)                                                        \
  do {                                                                          \
    if (!(cond)) {                                                              \
      std::fprintf(stderr, "FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__);      \
      ++g_failures;                                                             \
    }                                                                           \
  } while (0)

static const SFRotation kNoRot{0, 0, 1, 0};
static const SFVec3f kZero{0, 0, 0};

// Run a free-fall sim and return the final y of the single body.
static float freeFallFinalY() {
  JoltBackend backend;
  WorldHandle world = backend.createWorld(SFVec3f{0, -9.8f, 0});
  BodyHandle body = backend.addBody(world, ShapeDesc::sphere(0.5f),
                                    MassProperties{/*mass*/ 1.0f},
                                    /*fixed*/ false, SFVec3f{0, 10, 0}, kNoRot,
                                    kZero, kZero);
  for (int i = 0; i < 60; ++i)
    backend.step(world, 1.0 / 60.0);
  SFVec3f pos{0, 0, 0};
  SFRotation ori = kNoRot;
  backend.getBodyTransform(world, body, pos, ori);
  return pos.y;
}

int main() {
  // (1) Free-fall correctness.
  {
    float y = freeFallFinalY();
    std::fprintf(stderr, "free-fall y after 1s = %f (expected ~5.1)\n", y);
    CHECK(std::fabs(y - 5.1f) < 0.25f, "free-fall y near 5.1");
  }

  // (2) Rest-on-ground: a sphere (r=0.5) dropped onto a fixed thin box at y=0.
  {
    JoltBackend backend;
    WorldHandle world = backend.createWorld(SFVec3f{0, -9.8f, 0});
    // Fixed ground: a wide, thin box centered at y=0; its top surface is y=0.1.
    backend.addBody(world, ShapeDesc::box(SFVec3f{10, 0.1f, 10}),
                    MassProperties{/*mass*/ 0.0f},
                    /*fixed*/ true, SFVec3f{0, 0, 0}, kNoRot, kZero, kZero);
    BodyHandle ball = backend.addBody(world, ShapeDesc::sphere(0.5f),
                                      MassProperties{/*mass*/ 1.0f},
                                      /*fixed*/ false,
                                      SFVec3f{0, 3, 0}, kNoRot, kZero, kZero);
    for (int i = 0; i < 180; ++i)  // 3 s — plenty to fall + settle
      backend.step(world, 1.0 / 60.0);
    SFVec3f pos{0, 0, 0};
    SFRotation ori = kNoRot;
    backend.getBodyTransform(world, ball, pos, ori);
    std::fprintf(stderr, "rest-on-ground ball y = %f (expected ~0.6)\n", pos.y);
    // Ground top at 0.1 + sphere radius 0.5 = 0.6 center height.
    CHECK(pos.y > 0.45f && pos.y < 0.8f, "ball rests on ground near y=0.6");
  }

  // (3) Determinism: two identical free-fall sims => bit-identical final y.
  {
    float y1 = freeFallFinalY();
    float y2 = freeFallFinalY();
    CHECK(std::memcmp(&y1, &y2, sizeof(float)) == 0,
          "two identical sims are bit-identical");
  }

  // (4) PointConstraint pendulum: a dynamic ball pinned to a WORLD anchor at the
  //     origin, started 1 unit to the side, swings under gravity. The constraint
  //     must hold the bob ~1 unit from the anchor throughout (it does not fall
  //     freely, does not fly off) AND the bob must actually swing (y drops, x
  //     moves toward the anchor).
  {
    JoltBackend backend;
    WorldHandle world = backend.createWorld(SFVec3f{0, -9.8f, 0});
    const SFVec3f anchor{0, 0, 0};
    BodyHandle bob = backend.addBody(world, ShapeDesc::sphere(0.1f),
                                     MassProperties{1.0f}, false,
                                     SFVec3f{1, 0, 0}, kNoRot, kZero, kZero);
    ConstraintDesc d;
    d.kind = ConstraintDesc::Kind::Ball;
    d.bodyA = bob;
    d.bodyB = kInvalidBodyHandle;  // bind to world
    d.anchor = anchor;
    ConstraintHandle ch = backend.addConstraint(world, d);
    CHECK(ch != kInvalidConstraintHandle, "point constraint created");

    float maxDistErr = 0.0f, minY = 0.0f, maxX = -1e9f;
    for (int i = 0; i < 120; ++i) {
      backend.step(world, 1.0 / 60.0);
      SFVec3f p{0, 0, 0};
      SFRotation o = kNoRot;
      backend.getBodyTransform(world, bob, p, o);
      float dist = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
      maxDistErr = std::max(maxDistErr, std::fabs(dist - 1.0f));
      minY = std::min(minY, p.y);
      maxX = std::max(maxX, p.x);
    }
    std::fprintf(stderr,
                 "pendulum(point): maxDistErr=%f minY=%f (must swing down)\n",
                 maxDistErr, minY);
    CHECK(maxDistErr < 0.05f, "point pendulum stays ~1 unit from anchor");
    CHECK(minY < -0.3f, "point pendulum swings down (y drops well below start)");
  }

  // (5) HingeConstraint, free vs limited: two bobs hinged at the origin about z,
  //     each started horizontally at +x. A FREE hinge (limits ±pi) swings down to
  //     the bottom (y -> ~-1). A LOCKED hinge (minLimit == maxLimit == 0 — the
  //     start angle) is held at the stop and CANNOT swing (y stays ~0). The
  //     contrast proves the limits are honored. Both hold ~1 unit from the anchor.
  auto hingeRun = [&](float minLim, float maxLim) {
    JoltBackend backend;
    WorldHandle world = backend.createWorld(SFVec3f{0, -9.8f, 0});
    BodyHandle bob = backend.addBody(world, ShapeDesc::sphere(0.1f),
                                     MassProperties{1.0f}, false,
                                     SFVec3f{1, 0, 0}, kNoRot, kZero, kZero);
    ConstraintDesc d;
    d.kind = ConstraintDesc::Kind::Hinge;
    d.bodyA = bob;
    d.bodyB = kInvalidBodyHandle;
    d.anchor = SFVec3f{0, 0, 0};
    d.axis = SFVec3f{0, 0, 1};  // hinge about z
    d.minLimit = minLim;
    d.maxLimit = maxLim;
    CHECK(backend.addConstraint(world, d) != kInvalidConstraintHandle,
          "hinge constraint created");
    float maxDistErr = 0.0f, minY = 0.0f;
    for (int i = 0; i < 240; ++i) {  // 4s — plenty to settle
      backend.step(world, 1.0 / 60.0);
      SFVec3f p{0, 0, 0};
      SFRotation o = kNoRot;
      backend.getBodyTransform(world, bob, p, o);
      maxDistErr = std::max(maxDistErr,
                            std::fabs(std::sqrt(p.x * p.x + p.y * p.y) - 1.0f));
      minY = std::min(minY, p.y);
    }
    CHECK(maxDistErr < 0.05f, "hinge bob stays ~1 unit from anchor");
    return minY;
  };
  {
    const float pi = 3.14159265f;
    float freeMinY = hingeRun(-pi, pi);     // unconstrained: swings to bottom
    float lockedMinY = hingeRun(0.0f, 0.0f);  // locked at the start angle
    std::fprintf(stderr,
                 "hinge: free minY=%f (~-1) vs locked minY=%f (~0, stopped)\n",
                 freeMinY, lockedMinY);
    CHECK(freeMinY < -0.9f, "free hinge swings to the bottom (~-1)");
    CHECK(lockedMinY > -0.2f, "locked hinge limit stops the swing (~0)");
    CHECK(freeMinY < lockedMinY - 0.5f,
          "the limit demonstrably restricts the swing");
  }

  // (6) Asymmetric but SPEC-VALID hinge limits (minAngle <= 0 <= maxAngle, so the
  //     initial pose is in range — §37: limits are measured from the initial
  //     position). This proves §37 minAngle/maxAngle map to Jolt with the correct
  //     SIGN: Jolt's hinge angle is the NEGATIVE of the body's rotation about the
  //     axis, so the limits must be negated when handed to Jolt. Bob hinged about
  //     z, started at +x (angle 0), limited to [-0.3, 1.0]. Gravity pulls the
  //     angle down, so it must rest at the -0.3 lower stop and never pass it.
  //     (A symmetric range like the free/locked cases above can't catch a sign
  //     error — only an asymmetric one can.)
  {
    JoltBackend backend;
    WorldHandle world = backend.createWorld(SFVec3f{0, -9.8f, 0});
    BodyHandle bob = backend.addBody(world, ShapeDesc::sphere(0.1f),
                                     MassProperties{1.0f}, false,
                                     SFVec3f{1, 0, 0}, kNoRot, kZero, kZero);
    ConstraintDesc d;
    d.kind = ConstraintDesc::Kind::Hinge;
    d.bodyA = bob;
    d.bodyB = kInvalidBodyHandle;
    d.anchor = SFVec3f{0, 0, 0};
    d.axis = SFVec3f{0, 0, 1};
    d.minLimit = -0.3f;
    d.maxLimit = 1.0f;
    CHECK(backend.addConstraint(world, d) != kInvalidConstraintHandle,
          "asymmetric hinge constraint created");
    float finalAngle = 0.0f, minAngle = 1e9f;
    for (int i = 0; i < 240; ++i) {  // 4s — settle against the lower stop
      backend.step(world, 1.0 / 60.0);
      SFVec3f p{0, 0, 0};
      SFRotation o = kNoRot;
      backend.getBodyTransform(world, bob, p, o);
      finalAngle = std::atan2(p.y, p.x);
      minAngle = std::min(minAngle, finalAngle);
    }
    std::fprintf(stderr,
                 "asym hinge [-0.3,1.0]: final=%f min=%f (expect rest ~-0.3)\n",
                 finalAngle, minAngle);
    CHECK(finalAngle > -0.45f && finalAngle < -0.15f,
          "asymmetric hinge rests at the -0.3 lower stop (correct sign)");
    // The soft limit permits a small transient overshoot as the bob swings into
    // it; a SIGN bug would instead let it swing all the way to ~-1.0..-1.35.
    CHECK(minAngle > -0.7f,
          "asymmetric hinge does not swing far past the -0.3 stop (no sign flip)");
  }

  // (6) Per-contact restitution: a sphere dropped onto the ground rebounds clearly
  //     higher with bounce 0.8 than with restitution left at Jolt's default (0).
  {
    auto reboundApex = [](float restitution) {
      JoltBackend backend;
      WorldHandle world = backend.createWorld(SFVec3f{0, -9.8f, 0});
      backend.setContactResponse(world, /*friction*/ -1.0f, restitution);
      backend.addBody(world, ShapeDesc::box(SFVec3f{10, 0.1f, 10}),
                      MassProperties{0.0f}, true, SFVec3f{0, 0, 0}, kNoRot, kZero,
                      kZero);
      BodyHandle ball = backend.addBody(world, ShapeDesc::sphere(0.5f),
                                        MassProperties{1.0f}, false,
                                        SFVec3f{0, 3, 0}, kNoRot, kZero, kZero);
      float apex = 0.0f;
      bool landed = false;
      for (int i = 0; i < 180; ++i) {
        backend.step(world, 1.0 / 60.0);
        SFVec3f p{0, 0, 0};
        SFRotation o = kNoRot;
        backend.getBodyTransform(world, ball, p, o);
        if (p.y < 0.8f) landed = true;           // reached the ground
        if (landed) apex = std::max(apex, p.y);   // highest rebound after landing
      }
      return apex;
    };
    float bouncy = reboundApex(0.8f);
    float dead = reboundApex(-1.0f);  // unset -> Jolt default restitution 0
    std::fprintf(stderr, "bounce apex: restitution0.8=%.3f vs default=%.3f\n",
                 bouncy, dead);
    CHECK(bouncy > 1.0f, "restitution 0.8 sphere rebounds high (>1)");
    CHECK(dead < 0.85f, "default-restitution sphere does not bounce (settles)");  // Jolt leaves a small impulse residual (~0.79) even at restitution 0; 0.85 is above it, well below a real bounce.
    CHECK(bouncy > dead + 0.4f, "authored restitution clearly increases rebound");
  }

  if (g_failures == 0)
    std::fprintf(stderr, "jolt_backend_test: ALL PASS\n");
  return g_failures == 0 ? 0 : 1;
}
