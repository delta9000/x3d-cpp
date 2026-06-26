// mat4_inverse_test.cpp
#include "Mat4.hpp"
#include "Ray.hpp"
#include "doctest/doctest.h"
#include <cmath>
using namespace x3d::runtime;
static bool feq(float a, float b) { return std::fabs(a - b) < 1e-3f; }

TEST_CASE("mat4_inverse_test") {
  // M * inverse(M) ~= I for a non-trivial translate*rotate*scale.
  Mat4 M = Mat4::translation({3,-2,5})
         * Mat4::rotation(SFRotation{0,1,0, static_cast<float>(M_PI/3)})
         * Mat4::scale({2,4,0.5f});
  Mat4 I = M * M.inverse();
  for (int c=0;c<4;++c) for (int r=0;r<4;++r)
    CHECK((feq(I.m[c*4+r], c==r ? 1.0f : 0.0f)));

  // transformDirection ignores translation; transformPoint does not.
  Mat4 T = Mat4::translation({10,20,30});
  CHECK((feq(T.transformDirection({1,0,0}).x, 1) && feq(T.transformDirection({1,0,0}).y, 0)));
  CHECK((feq(T.transformPoint({0,0,0}).x, 10)));

  // singular -> identity (documented)
  Mat4 zero; // all-zero (empty Mat4)
  Mat4 zi = zero.inverse();
  CHECK((feq(zi.m[0],1) && feq(zi.m[5],1) && feq(zi.m[15],1)));

  // Ray::pointAt
  Ray ray{{0,0,0},{0,0,-1}};
  CHECK((feq(ray.pointAt(5).z, -5)));
  return;
}
