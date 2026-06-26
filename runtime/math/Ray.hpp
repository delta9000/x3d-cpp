// Ray.hpp — a ray (origin + direction). namespace x3d::runtime.
#ifndef X3D_RUNTIME_RAY_HPP
#define X3D_RUNTIME_RAY_HPP
#include "Mat4.hpp" // SFVec3f
namespace x3d::runtime {
struct Ray {
  SFVec3f origin{0,0,0};
  SFVec3f direction{0,0,-1};
  SFVec3f pointAt(float t) const {
    return SFVec3f{origin.x + direction.x*t, origin.y + direction.y*t, origin.z + direction.z*t};
  }
};
} // namespace x3d::runtime
#endif
