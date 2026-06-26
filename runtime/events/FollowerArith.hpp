#ifndef X3D_RUNTIME_FOLLOWER_ARITH_HPP
#define X3D_RUNTIME_FOLLOWER_ARITH_HPP
#include "Interpolation.hpp" // slerpRotation, Quat, quatFromRotation
#include "X3Dtypes.hpp"
#include <algorithm>
#include <cmath>
#include <vector>
namespace x3d::runtime {
template <typename T> struct FollowerArith;

template <> struct FollowerArith<float> {
  static float lerp(const float &a, const float &b, float t){ return a + (b-a)*t; }
  static float dist(const float &a, const float &b){ return std::fabs(a-b); }
  // SF/scalar types: no reshape needed, return v unchanged.
  static float reshapeLike(const float &v, const float &){ return v; }
};
template <> struct FollowerArith<SFVec2f> {
  static SFVec2f lerp(const SFVec2f &a, const SFVec2f &b, float t){ return {a.x+(b.x-a.x)*t, a.y+(b.y-a.y)*t}; }
  static float dist(const SFVec2f &a, const SFVec2f &b){ return std::hypot(a.x-b.x, a.y-b.y); }
  static SFVec2f reshapeLike(const SFVec2f &v, const SFVec2f &){ return v; }
};
template <> struct FollowerArith<SFVec3f> {
  static SFVec3f lerp(const SFVec3f &a, const SFVec3f &b, float t){ return {a.x+(b.x-a.x)*t, a.y+(b.y-a.y)*t, a.z+(b.z-a.z)*t}; }
  static float dist(const SFVec3f &a, const SFVec3f &b){ float dx=a.x-b.x,dy=a.y-b.y,dz=a.z-b.z; return std::sqrt(dx*dx+dy*dy+dz*dz); }
  static SFVec3f reshapeLike(const SFVec3f &v, const SFVec3f &){ return v; }
};
template <> struct FollowerArith<SFColor> {
  static SFColor lerp(const SFColor &a, const SFColor &b, float t){ return {a.r+(b.r-a.r)*t, a.g+(b.g-a.g)*t, a.b+(b.b-a.b)*t}; }
  static float dist(const SFColor &a, const SFColor &b){ float dr=a.r-b.r,dg=a.g-b.g,db=a.b-b.b; return std::sqrt(dr*dr+dg*dg+db*db); }
  static SFColor reshapeLike(const SFColor &v, const SFColor &){ return v; }
};
template <> struct FollowerArith<SFRotation> {
  static SFRotation lerp(const SFRotation &a, const SFRotation &b, float t){ return slerpRotation(a, b, t); }
  static float dist(const SFRotation &a, const SFRotation &b){
    // angle between orientations via quaternion dot.
    Quat qa = quatFromRotation(a), qb = quatFromRotation(b);
    double d = std::fabs(qa.x*qb.x + qa.y*qb.y + qa.z*qb.z + qa.w*qb.w);
    d = std::min(1.0, d);
    return static_cast<float>(2.0 * std::acos(d));
  }
  static SFRotation reshapeLike(const SFRotation &v, const SFRotation &){ return v; }
};
// MF: element-wise, output adopts b's length (the destination's shape).
template <typename Elem> struct MFFollowerArith {
  using V = std::vector<Elem>;
  static V lerp(const V &a, const V &b, float t){
    V out(b.size());
    for(size_t i=0;i<b.size();++i)
      out[i] = (i<a.size()) ? FollowerArith<Elem>::lerp(a[i], b[i], t) : b[i];
    return out;
  }
  static float dist(const V &a, const V &b){
    float m=0.f; size_t n=std::min(a.size(), b.size());
    for(size_t i=0;i<n;++i) m=std::max(m, FollowerArith<Elem>::dist(a[i], b[i]));
    if(a.size()!=b.size()) m=std::max(m, 1e9f); // length mismatch => not converged
    return m;
  }
  // Broadcast v to match like.size(): element i = v[i] if in-range, else v.back().
  // If v is empty, fall back to like[i] for each position.
  static V reshapeLike(const V &v, const V &like){
    if(v.size() == like.size()) return v;
    V out(like.size());
    for(size_t i=0;i<like.size();++i){
      if(i < v.size())      out[i] = v[i];
      else if(!v.empty())   out[i] = v.back();
      else                  out[i] = like[i];
    }
    return out;
  }
};
template <> struct FollowerArith<MFVec3f> : MFFollowerArith<SFVec3f> {};
template <> struct FollowerArith<MFVec2f> : MFFollowerArith<SFVec2f> {};
} // namespace x3d::runtime
#endif
