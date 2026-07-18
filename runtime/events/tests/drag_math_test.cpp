#include "doctest/doctest.h"
// drag_math_test.cpp — TDD for the three PURE drag-sensor math headers
// (M2D interaction layer). No node/System coupling: each header is a pure
// geometry function taking/returning plain SF* values.
//
// Spec basis (ISO/IEC 19775-1:2023):
//   PlaneSensor  §20.4.2 — tracking plane (Z=0 of sensor frame, coincident with
//                          the initial hit), translation_changed + per-component
//                          min/max clamp (min>max⇒unclamped; min==max⇒locked).
//   SphereSensor §20.4.3 — virtual sphere (radius = |initial hit|), relative
//                          rotation from p0_hat to p_hat, composed with offset.
//   CylinderSensor §20.4.1 — diskAngle decision (acute angle vs local Y) selects
//                          disk vs cylinder mode; rotation about Y; min/max clamp.
//
// Hand-computed expected geometry for each. Exit 0 on success, nonzero on fail.

#include "drag/CylinderDrag.hpp"
#include "drag/PlaneDrag.hpp"
#include "drag/SphereDrag.hpp"

#include "Mat4.hpp"

#include <cmath>
#include <numbers>
#include <iostream>
#include <string>

using namespace x3d::runtime;

namespace {

int failures = 0;

void check(bool cond, const std::string &what) {
  if (!cond) {
    std::cerr << "FAIL: " << what << "\n";
    ++failures;
  } else {
    std::cout << "ok: " << what << "\n";
  }
}

constexpr float kEps = 1e-4f;
bool feq(float a, float b) { return std::fabs(a - b) < kEps; }
bool veq(const SFVec3f &a, const SFVec3f &b) {
  return feq(a.x, b.x) && feq(a.y, b.y) && feq(a.z, b.z);
}

// ---------------------------------------------------------------------------
// PlaneSensor
// ---------------------------------------------------------------------------
void testPlane() {
  // Identity sensor frame (no axisRotation, parent = world identity).
  Mat4 frame = Mat4::identity();

  // Activation hit on the Z=0 plane at (1, 2, 0); tracking plane is z=0.
  SFVec3f p0{1.0f, 2.0f, 0.0f};

  // --- Per-motion: ray straight down -Z through (4, 5, 10) hits plane at (4,5,0).
  {
    Ray ray;
    ray.origin = SFVec3f{4.0f, 5.0f, 10.0f};
    ray.direction = SFVec3f{0.0f, 0.0f, -1.0f};
    // default unclamped: min=(0,0) max=(-1,-1) -> min>max on both axes
    PlaneDragResult r =
        planeDrag(frame, p0, ray, SFVec3f{0, 0, 0}, SFVec2f{0, 0},
                  SFVec2f{-1, -1});
    check(veq(r.trackPoint, SFVec3f{4, 5, 0}), "plane trackPoint unclamped");
    // delta = (4-1, 5-2, 0) = (3,3,0); offset 0 -> translation (3,3,0)
    check(veq(r.translation, SFVec3f{3, 3, 0}), "plane translation default-unclamped");
  }

  // --- Offset pass-through (incl. offset.z): same hit, offset (10,20,7).
  {
    Ray ray;
    ray.origin = SFVec3f{4.0f, 5.0f, 10.0f};
    ray.direction = SFVec3f{0.0f, 0.0f, -1.0f};
    PlaneDragResult r =
        planeDrag(frame, p0, ray, SFVec3f{10, 20, 7}, SFVec2f{0, 0},
                  SFVec2f{-1, -1});
    // (3+10, 3+20, offset.z=7)
    check(veq(r.translation, SFVec3f{13, 23, 7}), "plane offset incl z pass-through");
  }

  // --- Per-component clamp: minPosition=(0,0) maxPosition=(2,2). delta (3,3,0)
  //     -> clamp both to 2.
  {
    Ray ray;
    ray.origin = SFVec3f{4.0f, 5.0f, 10.0f};
    ray.direction = SFVec3f{0.0f, 0.0f, -1.0f};
    PlaneDragResult r =
        planeDrag(frame, p0, ray, SFVec3f{0, 0, 0}, SFVec2f{0, 0},
                  SFVec2f{2, 2});
    check(veq(r.translation, SFVec3f{2, 2, 0}), "plane both-axes clamped to max");
    check(veq(r.trackPoint, SFVec3f{4, 5, 0}), "plane trackPoint still unclamped under clamp");
  }

  // --- Line sensor: min==max on Y locks Y to that value; X free.
  {
    Ray ray;
    ray.origin = SFVec3f{4.0f, 5.0f, 10.0f};
    ray.direction = SFVec3f{0.0f, 0.0f, -1.0f};
    // X: min0<max10 -> clamp delta.x=3 stays 3; Y: min==max==0 -> locked 0
    PlaneDragResult r =
        planeDrag(frame, p0, ray, SFVec3f{0, 0, 0}, SFVec2f{0, 0},
                  SFVec2f{10, 0});
    check(veq(r.translation, SFVec3f{3, 0, 0}), "plane line-sensor locks Y");
  }

  // --- axisRotation: rotate sensor frame 90deg about Z so world +X maps to
  //     sensor +Y. A world-space hit displaced +X should appear as +Y delta.
  {
    SFRotation axis{0, 0, 1, static_cast<float>(std::numbers::pi) / 2.0f};
    Mat4 rframe = Mat4::rotation(axis); // M_sensor = world(I) * R(axisRotation)
    // initial hit at world origin -> sensor-local also origin (on z=0)
    SFVec3f p0r{0, 0, 0};
    Ray ray;
    ray.origin = SFVec3f{1.0f, 0.0f, 10.0f}; // world +X
    ray.direction = SFVec3f{0.0f, 0.0f, -1.0f};
    PlaneDragResult r =
        planeDrag(rframe, p0r, ray, SFVec3f{0, 0, 0}, SFVec2f{0, 0},
                  SFVec2f{-1, -1});
    // world (1,0,0) in sensor-local (inverse 90deg about Z) -> (0,-1,0)
    check(veq(r.translation, SFVec3f{0, -1, 0}), "plane axisRotation reorients delta");
  }
}

// ---------------------------------------------------------------------------
// SphereSensor
// ---------------------------------------------------------------------------
void testSphere() {
  Mat4 frame = Mat4::identity();

  // Activation hit at (0,0,2): radius 2, p0_hat = +Z.
  SFVec3f p0{0.0f, 0.0f, 2.0f};

  // --- Rotate 90deg: aim a ray at +X side of the sphere. Ray from (10,0,0)
  //     toward -X hits the sphere at (2,0,0). p_hat = +X. Rotation from +Z to
  //     +X is about -Y (right-handed) by 90deg... verify axis-angle.
  {
    Ray ray;
    ray.origin = SFVec3f{10.0f, 0.0f, 0.0f};
    ray.direction = SFVec3f{-1.0f, 0.0f, 0.0f};
    SFRotation offset{0, 1, 0, 0}; // identity
    SphereDragResult r = sphereDrag(frame, p0, ray, offset);
    check(veq(r.trackPoint, SFVec3f{2, 0, 0}), "sphere trackPoint on +X surface");
    // cross(+Z,+X) = (0*0-1*0, 1*1-0*0, 0*0-0*1)... cross(z,x)= +Y? compute:
    // z=(0,0,1) x=(1,0,0): cross = (0*0-1*0, 1*1-0*0, 0*0-0*1) = (0,1,0)
    check(feq(std::fabs(r.rotation.angle), static_cast<float>(std::numbers::pi) / 2.0f),
          "sphere rotation angle 90deg");
    // axis should be +Y (normalized)
    check(feq(r.rotation.x, 0) && feq(r.rotation.y, 1) && feq(r.rotation.z, 0),
          "sphere rotation axis +Y");
  }

  // --- No motion (ray back to activation point) -> identity rotation.
  {
    Ray ray;
    ray.origin = SFVec3f{0.0f, 0.0f, 10.0f};
    ray.direction = SFVec3f{0.0f, 0.0f, -1.0f};
    SFRotation offset{0, 1, 0, 0};
    SphereDragResult r = sphereDrag(frame, p0, ray, offset);
    check(veq(r.trackPoint, SFVec3f{0, 0, 2}), "sphere trackPoint back at p0");
    check(feq(r.rotation.angle, 0.0f), "sphere identity rotation when no motion");
  }

  // --- Offset composition: with no relative motion, rotation_changed == offset.
  {
    Ray ray;
    ray.origin = SFVec3f{0.0f, 0.0f, 10.0f};
    ray.direction = SFVec3f{0.0f, 0.0f, -1.0f};
    SFRotation offset{0, 1, 0, static_cast<float>(std::numbers::pi) / 2.0f};
    SphereDragResult r = sphereDrag(frame, p0, ray, offset);
    check(feq(r.rotation.angle, static_cast<float>(std::numbers::pi) / 2.0f),
          "sphere offset passes through when no motion (angle)");
    check(feq(std::fabs(r.rotation.y), 1.0f), "sphere offset axis +Y");
  }
}

// ---------------------------------------------------------------------------
// CylinderSensor
// ---------------------------------------------------------------------------
void testCylinder() {
  Mat4 frame = Mat4::identity();
  const float diskAngle = static_cast<float>(std::numbers::pi) / 12.0f; // 15deg default

  // --- CYLINDER mode: bearing nearly perpendicular to Y (along -Z) so the
  //     acute angle to Y is ~90deg >= diskAngle. Activation hit at (2,0,0):
  //     radius 2, zero_vec = +X.
  {
    SFVec3f p0{2.0f, 0.0f, 0.0f};
    // bearing direction -Z, so acute angle to Y = 90deg -> CYLINDER
    SFVec3f bearingDir{0.0f, 0.0f, -1.0f};
    // current ray from (0,0,10) toward -Z hits cylinder x^2+z^2=4 at z=2 i.e (0,0,2)
    Ray ray;
    ray.origin = SFVec3f{0.0f, 0.0f, 10.0f};
    ray.direction = bearingDir;
    // unclamped default min0 max-1
    CylinderDragResult r =
        cylinderDrag(frame, p0, bearingDir, ray, diskAngle, 0.0f, 0.0f, -1.0f);
    check(r.mode == CylinderMode::Cylinder, "cylinder mode selected (perp bearing)");
    // zero_vec +X, curr_vec at (0,0,2)->+Z. angle from +X to +Z about +Y:
    // atan2(cross2D(+X,+Z), dot) ; cross2D(ref,cur)=ref.x*cur.z-ref.z*cur.x=1*1-0*0=1
    // dot=0 -> atan2(1,0)=+pi/2
    check(feq(r.rotation.angle, static_cast<float>(std::numbers::pi) / 2.0f),
          "cylinder rotation +90deg about Y");
    check(feq(r.rotation.x, 0) && feq(r.rotation.y, 1) && feq(r.rotation.z, 0),
          "cylinder rotation axis +Y");
  }

  // --- DISK mode: bearing nearly parallel to Y (along -Y) so acute angle ~0 < diskAngle.
  {
    SFVec3f p0{1.0f, 0.0f, 0.0f}; // hit on Y=0 plane
    SFVec3f bearingDir{0.0f, -1.0f, 0.0f}; // parallel to Y -> DISK
    // current ray from (0,0,1) straight down -Y hits Y=0 plane at (0,0,1)
    Ray ray;
    ray.origin = SFVec3f{0.0f, 5.0f, 1.0f};
    ray.direction = SFVec3f{0.0f, -1.0f, 0.0f};
    CylinderDragResult r =
        cylinderDrag(frame, p0, bearingDir, ray, diskAngle, 0.0f, 0.0f, -1.0f);
    check(r.mode == CylinderMode::Disk, "disk mode selected (parallel bearing)");
    // ref=+X(1,0,0), cur at (0,0,1)->+Z. angle +pi/2 about Y (same as above)
    check(feq(r.rotation.angle, static_cast<float>(std::numbers::pi) / 2.0f),
          "disk rotation +90deg about Y");
  }

  // --- DS-1: the disk lies in the Y=0 plane of the local sensor frame, NOT a
  //     plane through the activation hit's Y. Activation hit has local Y=3; the
  //     tracked intersection must still land on Y=0.
  {
    SFVec3f p0{1.0f, 3.0f, 0.0f};          // activation hit with non-zero local Y
    SFVec3f bearingDir{0.0f, -1.0f, 0.0f}; // parallel to Y -> DISK
    Ray ray;
    ray.origin = SFVec3f{4.0f, 5.0f, 2.0f};
    ray.direction = SFVec3f{0.0f, -1.0f, 0.0f};
    CylinderDragResult r =
        cylinderDrag(frame, p0, bearingDir, ray, diskAngle, 0.0f, 0.0f, -1.0f);
    check(r.mode == CylinderMode::Disk, "ds1: disk mode selected");
    check(feq(r.trackPoint.y, 0.0f),
          "ds1: disk trackPoint on the Y=0 plane, not the activation-hit Y");
  }

  // --- Boundary: theta0 exactly == diskAngle -> CYLINDER (>= rule).
  {
    // bearing at exactly diskAngle from Y in the Y-Z plane.
    float a = diskAngle;
    SFVec3f bearingDir{0.0f, -std::cos(a), -std::sin(a)}; // acute angle to Y == a
    SFVec3f p0{2.0f, 0.0f, 0.0f};
    Ray ray;
    ray.origin = SFVec3f{0.0f, 0.0f, 10.0f};
    ray.direction = SFVec3f{0.0f, 0.0f, -1.0f};
    CylinderDragResult r =
        cylinderDrag(frame, p0, bearingDir, ray, diskAngle, 0.0f, 0.0f, -1.0f);
    check(r.mode == CylinderMode::Cylinder, "boundary theta0==diskAngle -> cylinder");
  }

  // --- min/max clamp in cylinder mode: clamp +90deg to maxAngle = +0.5 rad.
  {
    SFVec3f p0{2.0f, 0.0f, 0.0f};
    SFVec3f bearingDir{0.0f, 0.0f, -1.0f};
    Ray ray;
    ray.origin = SFVec3f{0.0f, 0.0f, 10.0f};
    ray.direction = SFVec3f{0.0f, 0.0f, -1.0f};
    // minAngle=-0.5, maxAngle=0.5 (min<max -> clamp). raw +pi/2 -> clamp to 0.5
    CylinderDragResult r =
        cylinderDrag(frame, p0, bearingDir, ray, diskAngle, 0.0f, -0.5f, 0.5f);
    check(feq(r.rotation.angle, 0.5f), "cylinder rotation clamped to maxAngle");
  }

  // --- offset add (cylinder): offset 0.1 added to +pi/2, unclamped default.
  {
    SFVec3f p0{2.0f, 0.0f, 0.0f};
    SFVec3f bearingDir{0.0f, 0.0f, -1.0f};
    Ray ray;
    ray.origin = SFVec3f{0.0f, 0.0f, 10.0f};
    ray.direction = SFVec3f{0.0f, 0.0f, -1.0f};
    CylinderDragResult r =
        cylinderDrag(frame, p0, bearingDir, ray, diskAngle, 0.1f, 0.0f, -1.0f);
    check(feq(r.rotation.angle, static_cast<float>(std::numbers::pi) / 2.0f + 0.1f),
          "cylinder offset added to angle");
  }
}

} // namespace

TEST_CASE("drag_math_test") {
  testPlane();
  testSphere();
  testCylinder();
  if (failures) {
    std::cerr << failures << " drag-math check(s) failed\n";
    CHECK(false); return;
  }
  std::cout << "all drag-math checks passed\n";
  return;
}
