// SphereDrag.hpp — PURE SphereSensor drag math (ISO/IEC 19775-1:2023 §20.4.3 +
// drag-sensor common §20.2.2). No node/System coupling.
//
// Model (all math in the sensor-LOCAL frame; SphereSensor has NO axisRotation,
// so the sensor frame IS the parent's local frame):
//   Activation (frozen): p0Local = inv(sensorFrame)*worldHit; r = |p0Local|;
//                        p0_hat = p0Local / r.
//   Per motion:
//     ray_local = inv(sensorFrame) applied to the world ray.
//     Intersect ray_local with the virtual sphere (center origin, radius r) ->
//       p_local (nearest positive-t hit); trackPoint_changed = p_local (unclamped).
//     p_hat = p_local / r.
//     R_rel = rotation taking p0_hat -> p_hat:
//       axis = normalize(cross(p0_hat, p_hat)); angle = acos(clamp(dot,-1,1)).
//       (anti)parallel: parallel -> identity; antiparallel -> (any perp, pi).
//     rotation_changed = toAxisAngle( q_rel * q_offset )   (offset is the base).
#ifndef X3D_RUNTIME_DRAG_SPHERE_DRAG_HPP
#define X3D_RUNTIME_DRAG_SPHERE_DRAG_HPP

#include "Interpolation.hpp" // Quat, quatFromRotation, rotationFromQuat
#include "Intersect.hpp"     // raySphere
#include "Mat4.hpp"          // Mat4, SFVec3f, SFRotation
#include "Ray.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace x3d::runtime {

struct SphereDragResult {
  SFVec3f trackPoint{0, 0, 0};       // point on the virtual sphere, sensor-local
  SFRotation rotation{0, 1, 0, 0};   // rotation_changed (relative + offset)
  bool valid = true;                 // false if the bearing missed the sphere
};

namespace detail {

inline float sdot(const SFVec3f &a, const SFVec3f &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}
inline SFVec3f scross(const SFVec3f &a, const SFVec3f &b) {
  return SFVec3f{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                 a.x * b.y - a.y * b.x};
}
inline float slen(const SFVec3f &a) { return std::sqrt(sdot(a, a)); }

// Hamilton product of two quaternions (x,y,z,w): result = a * b.
inline Quat qmul(const Quat &a, const Quat &b) {
  return Quat{a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
              a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
              a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
              a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}

} // namespace detail

// sphereDrag — one pointer-motion evaluation.
//   sensorFrame : M_sensor (world matrix of the sensor's parent group).
//   p0Local     : activation hit in sensor-local coords (defines r and p0_hat).
//   worldRay    : current bearing in WORLD space.
//   offset      : SFRotation accumulator composed as the base (q_rel * q_offset).
inline SphereDragResult sphereDrag(const Mat4 &sensorFrame,
                                   const SFVec3f &p0Local, const Ray &worldRay,
                                   const SFRotation &offset) {
  using namespace detail;
  SphereDragResult out;

  const float r = slen(p0Local);
  if (r < 1e-9f) {
    out.valid = false;
    out.rotation = offset;
    out.trackPoint = p0Local;
    return out;
  }
  const SFVec3f p0_hat{p0Local.x / r, p0Local.y / r, p0Local.z / r};

  // Transform the world ray into sensor-local space.
  const Mat4 inv = sensorFrame.inverse();
  Ray local;
  local.origin = inv.transformPoint(worldRay.origin);
  local.direction = inv.transformDirection(worldRay.direction);

  auto hit = raySphere(local, r);
  if (!hit) {
    // Bearing missed the sphere: spec permits browser-defined behavior; flag
    // invalid + hold p0 / offset so the System can decide.
    out.valid = false;
    out.trackPoint = p0Local;
    out.rotation = offset;
    return out;
  }
  const SFVec3f p_local = local.pointAt(*hit);
  out.trackPoint = p_local; // unclamped

  const float pl = slen(p_local);
  const SFVec3f p_hat = (pl > 1e-9f)
                            ? SFVec3f{p_local.x / pl, p_local.y / pl, p_local.z / pl}
                            : p0_hat;

  // Relative rotation p0_hat -> p_hat.
  SFVec3f axis = scross(p0_hat, p_hat);
  const float axisLen = slen(axis);
  const float c = std::clamp(sdot(p0_hat, p_hat), -1.0f, 1.0f);
  SFRotation rRel{0, 1, 0, 0};
  if (axisLen < 1e-6f) {
    if (c >= 0.0f) {
      rRel = SFRotation{0, 1, 0, 0}; // parallel -> identity
    } else {
      // antiparallel: pick any axis perpendicular to p0_hat, rotate by pi.
      SFVec3f t = (std::fabs(p0_hat.x) < 0.9f) ? SFVec3f{1, 0, 0}
                                               : SFVec3f{0, 1, 0};
      SFVec3f perp = scross(p0_hat, t);
      float pn = slen(perp);
      if (pn > 1e-9f) perp = SFVec3f{perp.x / pn, perp.y / pn, perp.z / pn};
      rRel = SFRotation{perp.x, perp.y, perp.z, static_cast<float>(std::numbers::pi)};
    }
  } else {
    axis = SFVec3f{axis.x / axisLen, axis.y / axisLen, axis.z / axisLen};
    rRel = SFRotation{axis.x, axis.y, axis.z, std::acos(c)};
  }

  // rotation_changed = q_rel * q_offset (offset is the base, drag on top).
  const Quat qOut = qmul(quatFromRotation(rRel), quatFromRotation(offset));
  out.rotation = rotationFromQuat(qOut);
  return out;
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_DRAG_SPHERE_DRAG_HPP
