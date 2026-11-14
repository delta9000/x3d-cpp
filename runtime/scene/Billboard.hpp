// §23.4.1 Billboard view-facing rotation. Shared by picking and extraction;
// keep independent of X3DExecutionContext to avoid an include cycle.
#ifndef X3D_RUNTIME_BILLBOARD_HPP
#define X3D_RUNTIME_BILLBOARD_HPP

#include "Mat4.hpp"
#include "Vec3.hpp"

#include <cmath>

namespace x3d::runtime {

// §23.4.1 Billboard. parentWorldM = the billboard's frame (world of its parent);
// returns the billboard's local rotation matrix (applied as `worldM * R`).
inline Mat4 billboardLocalMatrix(const Mat4 &parentWorldM, const SFVec3f &cameraWorldPos,
                                 const SFVec3f &viewerUpWorld, const SFVec3f &axisOfRotation) {
  const Mat4 inv = parentWorldM.inverse();
  const SFVec3f camLocal = inv.transformPoint(cameraWorldPos);   // viewer in local frame
  const SFVec3f b2v = vec3::normalizeOrZero(camLocal, 1e-9f); // billboard-origin -> viewer
  if (vec3::length(b2v) < 1e-9f) return Mat4::identity(); // viewer at origin: undefined

  const bool viewerAlign = (axisOfRotation.x == 0.0f && axisOfRotation.y == 0.0f && axisOfRotation.z == 0.0f);
  if (!viewerAlign) {
    // Rotate local +Z about `axis` into the plane(axis, b2v), as close to b2v as possible.
    const SFVec3f a = vec3::normalizeOrZero(axisOfRotation, 1e-9f);
    const SFVec3f zL{0, 0, 1};
    SFVec3f zProj = vec3::normalizeOrZero(vec3::sub(zL, SFVec3f{a.x*vec3::dot(zL,a), a.y*vec3::dot(zL,a), a.z*vec3::dot(zL,a)}), 1e-9f);
    SFVec3f bProj = vec3::normalizeOrZero(vec3::sub(b2v, SFVec3f{a.x*vec3::dot(b2v,a), a.y*vec3::dot(b2v,a), a.z*vec3::dot(b2v,a)}), 1e-9f);
    if (vec3::length(zProj) < 1e-9f || vec3::length(bProj) < 1e-9f) return Mat4::identity(); // axis ∥ b2v: undefined
    const float angle = std::atan2(vec3::dot(vec3::cross(zProj, bProj), a), vec3::dot(zProj, bProj));
    return Mat4::rotation(SFRotation{a.x, a.y, a.z, angle});
  }
  // Viewer-alignment: +Z -> b2v, +Y -> viewer up.
  const SFVec3f upL = vec3::normalizeOrZero(inv.transformDirection(viewerUpWorld), 1e-9f);
  const SFVec3f newZ = b2v;
  SFVec3f newX = vec3::cross(upL, newZ);
  if (vec3::length(newX) < 1e-9f) {
    // Roll is undefined when up is parallel to the viewing direction (or
    // zero). Choose the least-aligned canonical axis to avoid a singular
    // rotation.
    const float ax = std::fabs(newZ.x), ay = std::fabs(newZ.y),
                az = std::fabs(newZ.z);
    const SFVec3f fallbackUp = ax <= ay && ax <= az ? SFVec3f{1, 0, 0}
                               : ay <= az           ? SFVec3f{0, 1, 0}
                                                    : SFVec3f{0, 0, 1};
    newX = vec3::cross(fallbackUp, newZ);
  }
  newX = vec3::normalizeOrZero(newX, 1e-9f);
  const SFVec3f newY = vec3::cross(newZ, newX);
  // Column-major basis: columns are images of local X,Y,Z. Build via Mat4 element layout.
  Mat4 m = Mat4::identity();
  m.m[0]=newX.x; m.m[1]=newX.y; m.m[2]=newX.z;   // column 0 = newX
  m.m[4]=newY.x; m.m[5]=newY.y; m.m[6]=newY.z;   // column 1 = newY
  m.m[8]=newZ.x; m.m[9]=newZ.y; m.m[10]=newZ.z;  // column 2 = newZ
  return m;
}

} // namespace x3d::runtime
#endif
