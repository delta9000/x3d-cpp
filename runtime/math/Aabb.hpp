// Aabb.hpp — axis-aligned bounding box (float). Empty is the union identity.
// namespace x3d::runtime.
#ifndef X3D_RUNTIME_AABB_HPP
#define X3D_RUNTIME_AABB_HPP

#include "Mat4.hpp"      // Mat4, SFVec3f
#include <algorithm>

namespace x3d::runtime {

struct Aabb {
  SFVec3f min{0,0,0}, max{0,0,0};
  bool empty = true;

  void expand(const SFVec3f &p) {
    if (empty) { min = max = p; empty = false; return; }
    min.x = std::min(min.x, p.x); min.y = std::min(min.y, p.y); min.z = std::min(min.z, p.z);
    max.x = std::max(max.x, p.x); max.y = std::max(max.y, p.y); max.z = std::max(max.z, p.z);
  }
  void unionWith(const Aabb &o) {
    if (o.empty) return;
    expand(o.min); expand(o.max);
  }
  Aabb transformed(const Mat4 &m) const {
    if (empty) return *this;
    Aabb r;
    for (int i = 0; i < 8; ++i) {
      SFVec3f c{ (i & 1) ? max.x : min.x, (i & 2) ? max.y : min.y, (i & 4) ? max.z : min.z };
      r.expand(m.transformPoint(c));
    }
    return r;
  }
  SFVec3f center() const {
    return empty ? SFVec3f{0,0,0}
                 : SFVec3f{(min.x+max.x)*0.5f, (min.y+max.y)*0.5f, (min.z+max.z)*0.5f};
  }
  SFVec3f size() const {
    return empty ? SFVec3f{0,0,0} : SFVec3f{max.x-min.x, max.y-min.y, max.z-min.z};
  }
  static Aabb fromCenterSize(const SFVec3f &c, const SFVec3f &s) {
    Aabb r;
    SFVec3f h{s.x*0.5f, s.y*0.5f, s.z*0.5f};
    r.min = {c.x-h.x, c.y-h.y, c.z-h.z};
    r.max = {c.x+h.x, c.y+h.y, c.z+h.z};
    r.empty = false;
    return r;
  }
};

} // namespace x3d::runtime
#endif // X3D_RUNTIME_AABB_HPP
