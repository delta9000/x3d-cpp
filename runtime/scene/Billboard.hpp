// Billboard.hpp — M2e: §23.4.1 Billboard view-facing rotation (pure math).
// Standalone so the per-path walkers that need it (SceneExtractor, PickSystem)
// can include it WITHOUT pulling in X3DExecutionContext. ViewDependentSystem.hpp
// includes X3DExecutionContext.hpp, which (transitively, via PickSystem.hpp)
// would form an include cycle if PickSystem reached billboardLocalMatrix through
// ViewDependentSystem.hpp — so the math lives here, depending only on Mat4.hpp.
#ifndef X3D_RUNTIME_BILLBOARD_HPP
#define X3D_RUNTIME_BILLBOARD_HPP

#include "Mat4.hpp"

#include <cmath>

namespace x3d::runtime {

namespace viewdep {
inline SFVec3f sub(const SFVec3f &a, const SFVec3f &b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
inline float dot(const SFVec3f &a, const SFVec3f &b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline SFVec3f cross(const SFVec3f &a, const SFVec3f &b) {
  return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
}
inline float len(const SFVec3f &a) { return std::sqrt(dot(a, a)); }
inline SFVec3f norm(const SFVec3f &a) { float l = len(a); return l < 1e-9f ? SFVec3f{0,0,0} : SFVec3f{a.x/l, a.y/l, a.z/l}; }
} // namespace viewdep

// §23.4.1 Billboard. parentWorldM = the billboard's frame (world of its parent);
// returns the billboard's LOCAL rotation matrix (applied as `worldM * R`).
inline Mat4 billboardLocalMatrix(const Mat4 &parentWorldM, const SFVec3f &cameraWorldPos,
                                 const SFVec3f &viewerUpWorld, const SFVec3f &axisOfRotation) {
  using namespace viewdep;
  const Mat4 inv = parentWorldM.inverse();
  const SFVec3f camLocal = inv.transformPoint(cameraWorldPos);   // viewer in local frame
  const SFVec3f b2v = norm(camLocal);                            // billboard-origin -> viewer
  if (len(b2v) < 1e-9f) return Mat4::identity();                 // viewer at origin: undefined

  const bool viewerAlign = (axisOfRotation.x == 0.0f && axisOfRotation.y == 0.0f && axisOfRotation.z == 0.0f);
  if (!viewerAlign) {
    // Rotate local +Z about `axis` into the plane(axis, b2v), as close to b2v as possible.
    const SFVec3f a = norm(axisOfRotation);
    const SFVec3f zL{0, 0, 1};
    SFVec3f zProj = norm(sub(zL, SFVec3f{a.x*dot(zL,a), a.y*dot(zL,a), a.z*dot(zL,a)}));
    SFVec3f bProj = norm(sub(b2v, SFVec3f{a.x*dot(b2v,a), a.y*dot(b2v,a), a.z*dot(b2v,a)}));
    if (len(zProj) < 1e-9f || len(bProj) < 1e-9f) return Mat4::identity(); // axis ∥ b2v: undefined
    const float angle = std::atan2(dot(cross(zProj, bProj), a), dot(zProj, bProj));
    return Mat4::rotation(SFRotation{a.x, a.y, a.z, angle});
  }
  // Viewer-alignment: +Z -> b2v, +Y -> viewer up.
  const SFVec3f upL = norm(inv.transformDirection(viewerUpWorld));
  const SFVec3f newZ = b2v;
  SFVec3f newX = cross(upL, newZ);
  if (len(newX) < 1e-9f) newX = cross(SFVec3f{0, 1, 0}, newZ); // up ∥ b2v fallback
  newX = norm(newX);
  const SFVec3f newY = cross(newZ, newX);
  // Column-major basis: columns are images of local X,Y,Z. Build via Mat4 element layout.
  Mat4 m = Mat4::identity();
  m.m[0]=newX.x; m.m[1]=newX.y; m.m[2]=newX.z;   // column 0 = newX
  m.m[4]=newY.x; m.m[5]=newY.y; m.m[6]=newY.z;   // column 1 = newY
  m.m[8]=newZ.x; m.m[9]=newZ.y; m.m[10]=newZ.z;  // column 2 = newZ
  return m;
}

} // namespace x3d::runtime
#endif
