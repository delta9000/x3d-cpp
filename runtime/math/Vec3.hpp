// Vec3.hpp — the SFVec3f arithmetic the runtime systems share. namespace x3d::runtime::vec3.
#ifndef X3D_RUNTIME_VEC3_HPP
#define X3D_RUNTIME_VEC3_HPP

#include "x3d/core/X3Dtypes.hpp" // SFVec3f

#include <cmath>

namespace x3d::runtime::vec3 {

using x3d::core::SFVec3f;

inline SFVec3f add(const SFVec3f &a, const SFVec3f &b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline SFVec3f sub(const SFVec3f &a, const SFVec3f &b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline SFVec3f scale(const SFVec3f &a, float s) { return {a.x * s, a.y * s, a.z * s}; }
inline float dot(const SFVec3f &a, const SFVec3f &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline SFVec3f cross(const SFVec3f &a, const SFVec3f &b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline float length(const SFVec3f &a) { return std::sqrt(dot(a, a)); }

/// Unit vector along `a`; a vector no longer than `eps` comes back unchanged.
inline SFVec3f normalize(const SFVec3f &a, float eps = 1e-12f) {
  const float l = length(a);
  return l > eps ? SFVec3f{a.x / l, a.y / l, a.z / l} : a;
}

/// Unit vector along `a`; a vector no longer than `eps` becomes zero.
inline SFVec3f normalizeOrZero(const SFVec3f &a, float eps = 0.0f) {
  const float l = length(a);
  return l > eps ? SFVec3f{a.x / l, a.y / l, a.z / l} : SFVec3f{0, 0, 0};
}

} // namespace x3d::runtime::vec3

#endif
