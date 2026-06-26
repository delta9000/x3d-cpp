// PlaneDrag.hpp — PURE PlaneSensor drag math (ISO/IEC 19775-1:2023 §20.4.2 +
// drag-sensor common §20.2.2). No node/System coupling: a single free function
// over plain SF* values, unit-testable in isolation.
//
// Model (all math in the sensor-LOCAL frame):
//   M_sensor = M_world_of_parent * R(axisRotation)   (built by the caller and
//              passed in as `sensorFrame`; the activation hit `p0Local` is
//              already in this local frame).
//   Tracking plane: z = p0Local.z, parallel to the Z=0 plane of the sensor
//   frame and coincident with the initial hit. Frozen at activation.
//
//   Per motion (world ray in `ray`):
//     O_s = inv(M_sensor) * ray.origin
//     D_s = inv(M_sensor).upper3x3 * ray.direction
//     t   = (p0Local.z - O_s.z) / D_s.z   (|D_s.z|~0 -> hold last: caller's job;
//           here we still compute, producing a far point — see note)
//     Q_s = O_s + t * D_s                 (trackPoint_changed, UNCLAMPED)
//     raw = (Q_s.x - p0Local.x, Q_s.y - p0Local.y, 0)
//     unclamped = raw + offset            (offset.z passes straight through)
//     translation = (clampDim(unclamped.x, min.x, max.x),
//                    clampDim(unclamped.y, min.y, max.y),
//                    unclamped.z)
//   clampDim: min>max -> unclamped; min==max -> locked to min; else std::clamp.
#ifndef X3D_RUNTIME_DRAG_PLANE_DRAG_HPP
#define X3D_RUNTIME_DRAG_PLANE_DRAG_HPP

#include "Mat4.hpp" // Mat4, SFVec3f, SFVec2f
#include "Ray.hpp"

#include <algorithm>
#include <cmath>

namespace x3d::runtime {

struct PlaneDragResult {
  SFVec3f trackPoint{0, 0, 0};  // unclamped plane intersection, sensor-local
  SFVec3f translation{0, 0, 0}; // clamped translation_changed, sensor-local
  bool valid = true;            // false if the ray was parallel to the plane
};

// Per-component clamp: min>max => unclamped; min==max => locked to min; else clamp.
inline float planeClampDim(float v, float mn, float mx) {
  if (mn > mx) return v;       // unclamped
  if (mn == mx) return mn;     // locked (line sensor)
  return std::clamp(v, mn, mx);
}

// planeDrag — one pointer-motion evaluation.
//   sensorFrame : M_sensor (world matrix of parent * R(axisRotation)).
//   p0Local     : activation hit in sensor-local coords (defines plane z).
//   worldRay    : current bearing in WORLD space.
//   offset      : SFVec3f accumulator (added; offset.z passes through).
//   minPosition/maxPosition : per-component (X,Y) clamp bounds.
inline PlaneDragResult planeDrag(const Mat4 &sensorFrame, const SFVec3f &p0Local,
                                 const Ray &worldRay, const SFVec3f &offset,
                                 const SFVec2f &minPosition,
                                 const SFVec2f &maxPosition) {
  PlaneDragResult out;
  const Mat4 inv = sensorFrame.inverse();

  const SFVec3f Os = inv.transformPoint(worldRay.origin);
  const SFVec3f Ds = inv.transformDirection(worldRay.direction);

  const float planeZ = p0Local.z;
  if (std::fabs(Ds.z) < 1e-9f) {
    // Ray (near-)parallel to the tracking plane: no finite intersection.
    // Spec allows implementation-defined handling; we flag invalid so the
    // caller (System) can hold the last valid value.
    out.valid = false;
    out.trackPoint = p0Local;
    out.translation = SFVec3f{offset.x, offset.y, offset.z};
    return out;
  }

  const float t = (planeZ - Os.z) / Ds.z;
  const SFVec3f Qs{Os.x + t * Ds.x, Os.y + t * Ds.y, Os.z + t * Ds.z};
  out.trackPoint = Qs; // unclamped

  const float dx = Qs.x - p0Local.x;
  const float dy = Qs.y - p0Local.y;
  const SFVec3f unclamped{dx + offset.x, dy + offset.y, offset.z};

  out.translation = SFVec3f{
      planeClampDim(unclamped.x, minPosition.x, maxPosition.x),
      planeClampDim(unclamped.y, minPosition.y, maxPosition.y), unclamped.z};
  return out;
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_DRAG_PLANE_DRAG_HPP
