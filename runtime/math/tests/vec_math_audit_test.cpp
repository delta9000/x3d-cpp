// vec_math_audit_test.cpp — AUD-VEC-MATH regression tests.
// Covers edge cases in quaternion construction, matrix inversion, slerp,
// transform composition, and ray-intersection degeneracy.
#include "Mat4.hpp"
#include "Intersect.hpp"
#include "Interpolation.hpp"
#include "doctest/doctest.h"
#include <cmath>
#include <numbers>
using namespace x3d::runtime;

static bool feq(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) < eps; }
static bool veq(const SFVec3f& a, const SFVec3f& b, float eps = 1e-4f) {
  return feq(a.x,b.x,eps) && feq(a.y,b.y,eps) && feq(a.z,b.z,eps);
}

TEST_CASE("vec_math_audit_test") {
  // 1. quatFromRotation with zero axis → falls back to +Y (normalized)
  {
    Quat q = quatFromRotation(SFRotation{0,0,0,0});
    CHECK((feq(static_cast<float>(q.x), 0.0f)));
    CHECK((feq(static_cast<float>(q.y), 0.0f)));
    CHECK((feq(static_cast<float>(q.z), 0.0f)));
    CHECK((feq(static_cast<float>(q.w), 1.0f)));
  }
  {
    Quat q = quatFromRotation(SFRotation{0,0,0, static_cast<float>(std::numbers::pi)});
    CHECK((feq(static_cast<float>(q.x), 0.0f)));
    CHECK((feq(static_cast<float>(q.y), 1.0f))); // axis became +Y, sin(pi/2)=1
    CHECK((feq(static_cast<float>(q.z), 0.0f)));
    CHECK((feq(static_cast<float>(q.w), 0.0f))); // cos(pi/2)=0
  }

  // 2. rotationFromQuat with near-zero vector length → identity about +Y
  {
    SFRotation r = rotationFromQuat(Quat{0,0,0,1});
    CHECK((feq(r.x, 0.0f)));
    CHECK((feq(r.y, 1.0f)));
    CHECK((feq(r.z, 0.0f)));
    CHECK((feq(r.angle, 0.0f)));
  }

  // 3. Matrix inversion: singular matrices return identity (documented fallback)
  {
    Mat4 zero;
    Mat4 zi = zero.inverse();
    CHECK((feq(zi.m[0], 1) && feq(zi.m[5], 1) && feq(zi.m[10], 1) && feq(zi.m[15], 1)));

    // Scale with a zero component → det = 0
    Mat4 sz = Mat4::scale({1,0,2});
    Mat4 szi = sz.inverse();
    CHECK((feq(szi.m[0], 1) && feq(szi.m[5], 1) && feq(szi.m[10], 1) && feq(szi.m[15], 1)));

    // Pure translation has det=1, should invert correctly
    Mat4 t = Mat4::translation({3,-2,5});
    Mat4 ti = t.inverse();
    CHECK((veq(ti.transformPoint({3,-2,5}), {0,0,0})));
  }

  // 4. slerp antipodal quaternions → shorter path, valid unit result
  {
    Quat q1{0, 1, 0, 0};   // 180° about Y
    Quat q2{0, -1, 0, 0};  // antipodal representation of same rotation
    Quat r = slerp(q1, q2, 0.5);
    double len = std::sqrt(r.x*r.x + r.y*r.y + r.z*r.z + r.w*r.w);
    CHECK((feq(static_cast<float>(len), 1.0f, 1e-3f)));
  }

  // 5. transformMatrix precision: TRS composition round-trip on points
  {
    SFVec3f t{1.0f, 2.0f, 3.0f};
    SFRotation rot{0,0,1, static_cast<float>(std::numbers::pi/4.0)};
    SFVec3f s{2.0f, 2.0f, 2.0f};
    Mat4 m = transformMatrix(t, rot, s, {0,0,0}, {0,0,1,0});

    // Origin maps to translation
    CHECK((veq(m.transformPoint({0,0,0}), t)));

    // Directions omit translation
    CHECK((veq(m.transformDirection({0,0,0}), {0,0,0})));

    // Identity transform round-trip
    Mat4 id = transformMatrix({0,0,0}, {0,0,1,0}, {1,1,1}, {0,0,0}, {0,0,1,0});
    CHECK((veq(id.transformPoint({1,2,3}), {1,2,3})));
  }

  // 6. rayTriangle/rayTriangleBary with degenerate triangles → nullopt
  {
    // Collinear triangle
    SFVec3f v0{0,0,0}, v1{1,0,0}, v2{2,0,0};
    CHECK((!rayTriangle(Ray{{0.5f,0.5f,5},{0,0,-1}}, v0,v1,v2)));

    // Collapsed edge (two identical vertices)
    SFVec3f v0b{0,0,0}, v1b{1,0,0}, v2b{1,0,0};
    CHECK((!rayTriangle(Ray{{0.5f,0.5f,5},{0,0,-1}}, v0b,v1b,v2b)));

    // Ray parallel to triangle plane
    SFVec3f v0c{0,0,0}, v1c{1,0,0}, v2c{0,1,0};
    CHECK((!rayTriangle(Ray{{0.5f,0.5f,5},{1,0,0}}, v0c,v1c,v2c)));

    // Barycentric version with degenerate triangle
    float u=0,v=0;
    CHECK((!rayTriangleBary(Ray{{0.5f,0.5f,5},{0,0,-1}}, v0,v1,v2, u, v)));
  }

  // 7. slerpNormal with parallel and antipodal vectors
  {
    // Parallel
    SFVec3f a{1,0,0};
    SFVec3f b{1,0,0};
    SFVec3f r = slerpNormal(a, b, 0.5f);
    CHECK((veq(r, a, 1e-3f)));

    // Antipodal (fallback to NLERP then return a on zero-length mid-vector)
    SFVec3f c{0,1,0};
    SFVec3f d{0,-1,0};
    SFVec3f ra = slerpNormal(c, d, 0.5f);
    float len = std::sqrt(ra.x*ra.x + ra.y*ra.y + ra.z*ra.z);
    CHECK((feq(len, 1.0f, 1e-3f)));
  }

  return;
}
