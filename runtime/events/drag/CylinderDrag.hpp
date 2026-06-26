// CylinderDrag.hpp — PURE CylinderSensor drag math (ISO/IEC 19775-1:2023
// §20.4.1 + drag-sensor common §20.2.2). No node/System coupling.
//
// Model (all math in the sensor-LOCAL frame; M_sensor = M_local * R(axisRotation),
// built by the caller and passed as `sensorFrame`; p0Local already in that frame):
//   Mode (frozen at activation):
//     theta0 = acos(|dot(normalize(bearingDirLocal), +Y)|)   (acute, [0,pi/2])
//     theta0 <  diskAngle -> DISK ; theta0 >= diskAngle -> CYLINDER.
//   DISK: virtual surface = plane y = p0Local.y; intersect ray; angle about +Y
//     from ref=normalize(p0Local.xz) to cur=normalize(Q.xz).
//   CYLINDER: radius r = sqrt(p0Local.x^2 + p0Local.z^2); intersect the infinite
//     cylinder x^2+z^2=r^2; same XZ angle measure.
//   raw = dAngle + offset; clamp: minAngle<=maxAngle -> clamp; else unclamped.
//   rotation_changed = SFRotation((0,1,0), clamped); trackPoint = Q (UNCLAMPED).
//
// Caller passes the ACTIVATION bearing direction in sensor-local space
// (`bearingDirLocal`) for the once-only mode decision, AND the current world
// `ray` for the per-motion intersection. The function transforms the world ray
// into the local frame via `sensorFrame`.
#ifndef X3D_RUNTIME_DRAG_CYLINDER_DRAG_HPP
#define X3D_RUNTIME_DRAG_CYLINDER_DRAG_HPP

#include "Mat4.hpp" // Mat4, SFVec3f, SFRotation
#include "Ray.hpp"

#include <algorithm>
#include <cmath>

namespace x3d::runtime {

enum class CylinderMode { Disk, Cylinder };

struct CylinderDragResult {
  SFVec3f trackPoint{0, 0, 0};     // unclamped surface intersection, sensor-local
  SFRotation rotation{0, 1, 0, 0}; // rotation_changed: axis +Y, clamped angle
  CylinderMode mode = CylinderMode::Cylinder;
  bool valid = true;               // false if the bearing missed the surface
};

// Decide disk vs cylinder from the activation bearing direction (sensor-local).
inline CylinderMode cylinderModeFor(const SFVec3f &bearingDirLocal,
                                    float diskAngle) {
  float len = std::sqrt(bearingDirLocal.x * bearingDirLocal.x +
                        bearingDirLocal.y * bearingDirLocal.y +
                        bearingDirLocal.z * bearingDirLocal.z);
  float ny = (len > 1e-12f) ? bearingDirLocal.y / len : 0.0f;
  float theta0 = std::acos(std::min(1.0f, std::fabs(ny))); // acute, [0,pi/2]
  // Spec rule: theta0 < diskAngle -> DISK; theta0 >= diskAngle -> CYLINDER.
  // A small angular epsilon makes the boundary deterministic: an input AT
  // diskAngle (whose acute angle round-trips through normalize()+acos() with
  // ~1e-7 float drift) selects CYLINDER, honoring the ">=" side of the rule.
  constexpr float kAngleEps = 1e-5f;
  return (theta0 < diskAngle - kAngleEps) ? CylinderMode::Disk
                                          : CylinderMode::Cylinder;
}

// cylinderDrag — one pointer-motion evaluation.
//   sensorFrame     : M_sensor (M_local * R(axisRotation)).
//   p0Local         : activation hit in sensor-local coords.
//   bearingDirLocal : activation bearing direction (sensor-local) — mode decision.
//   worldRay        : current bearing in WORLD space.
//   diskAngle       : disk/cylinder threshold (rad).
//   offset          : SFFloat angle accumulator (added to the relative angle).
//   minAngle/maxAngle : clamp bounds (minAngle>maxAngle => unclamped).
inline CylinderDragResult
cylinderDrag(const Mat4 &sensorFrame, const SFVec3f &p0Local,
             const SFVec3f &bearingDirLocal, const Ray &worldRay,
             float diskAngle, float offset, float minAngle, float maxAngle) {
  CylinderDragResult out;
  out.mode = cylinderModeFor(bearingDirLocal, diskAngle);

  const Mat4 inv = sensorFrame.inverse();
  const SFVec3f O = inv.transformPoint(worldRay.origin);
  const SFVec3f D = inv.transformDirection(worldRay.direction);

  // zero/reference vector: p0 projected onto XZ. Guard r=0 degenerate.
  const float refLen = std::sqrt(p0Local.x * p0Local.x + p0Local.z * p0Local.z);
  SFVec3f ref = (refLen > 1e-9f)
                    ? SFVec3f{p0Local.x / refLen, 0.0f, p0Local.z / refLen}
                    : SFVec3f{1.0f, 0.0f, 0.0f}; // fallback direction

  SFVec3f Q{0, 0, 0};
  bool hit = false;

  if (out.mode == CylinderMode::Disk) {
    // §20.4.1: in disk mode the geometry is treated as an infinitely large disk
    // lying in the Y=0 plane of the local sensor coordinate system — NOT a plane
    // through the activation hit's Y. (DS-1)
    if (std::fabs(D.y) > 1e-9f) {
      float t = (0.0f - O.y) / D.y;
      if (t >= 0.0f) {
        Q = SFVec3f{O.x + t * D.x, O.y + t * D.y, O.z + t * D.z};
        hit = true;
      }
    }
  } else {
    // Infinite cylinder x^2 + z^2 = r^2, r = refLen.
    const float r = refLen;
    const float a = D.x * D.x + D.z * D.z;
    if (a > 1e-12f && r > 1e-9f) {
      const float b = 2.0f * (O.x * D.x + O.z * D.z);
      const float c = O.x * O.x + O.z * O.z - r * r;
      const float disc = b * b - 4.0f * a * c;
      if (disc >= 0.0f) {
        const float sq = std::sqrt(disc);
        const float t1 = (-b - sq) / (2.0f * a);
        const float t2 = (-b + sq) / (2.0f * a);
        // Smallest non-negative t.
        float t = -1.0f;
        if (t1 >= 0.0f) t = t1;
        else if (t2 >= 0.0f) t = t2;
        if (t >= 0.0f) {
          Q = SFVec3f{O.x + t * D.x, O.y + t * D.y, O.z + t * D.z};
          hit = true;
        }
      }
    }
  }

  if (!hit) {
    // Bearing missed the virtual surface: spec permits implementation-defined
    // handling; flag invalid + hold offset so the System can decide.
    out.valid = false;
    out.trackPoint = p0Local;
    out.rotation = SFRotation{0, 1, 0, offset};
    return out;
  }

  out.trackPoint = Q; // unclamped

  // Current vector projected onto XZ.
  const float curLen = std::sqrt(Q.x * Q.x + Q.z * Q.z);
  SFVec3f cur = (curLen > 1e-9f) ? SFVec3f{Q.x / curLen, 0.0f, Q.z / curLen}
                                 : ref;

  // Right-handed angle about +Y from ref to cur:
  //   cross2D(ref,cur) about +Y = ref.x*cur.z - ref.z*cur.x.
  const float cross2d = ref.x * cur.z - ref.z * cur.x;
  const float dot2d = ref.x * cur.x + ref.z * cur.z;
  const float dAngle = std::atan2(cross2d, dot2d);

  float raw = dAngle + offset;
  float clamped = (minAngle <= maxAngle) ? std::clamp(raw, minAngle, maxAngle)
                                         : raw;
  out.rotation = SFRotation{0.0f, 1.0f, 0.0f, clamped};
  return out;
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_DRAG_CYLINDER_DRAG_HPP
