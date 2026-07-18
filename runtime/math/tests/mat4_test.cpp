// mat4_test.cpp — column-major 4x4 math + X3D Transform composition.
#include "Mat4.hpp"
#include "doctest/doctest.h"
#include <cmath>
#include <numbers>
using namespace x3d::runtime;

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }
static bool veq(const SFVec3f& a, const SFVec3f& b) {
  return feq(a.x,b.x) && feq(a.y,b.y) && feq(a.z,b.z);
}

TEST_CASE("mat4_test") {
  // identity transforms a point to itself
  CHECK((veq(Mat4::identity().transformPoint({1,2,3}), {1,2,3})));

  // pure translation
  Mat4 t = Mat4::translation({10,20,30});
  CHECK((veq(t.transformPoint({1,2,3}), {11,22,33})));

  // pure scale
  Mat4 s = Mat4::scale({2,3,4});
  CHECK((veq(s.transformPoint({1,1,1}), {2,3,4})));

  // 90deg rotation about +Z maps +X -> +Y
  Mat4 r = Mat4::rotation(SFRotation{0,0,1, static_cast<float>(std::numbers::pi/2.0)});
  CHECK((veq(r.transformPoint({1,0,0}), {0,1,0})));

  // multiply order: (T * S) applied to p scales THEN translates
  Mat4 ts = t * s;
  CHECK((veq(ts.transformPoint({1,1,1}), {12,23,34}))); // (2,3,4) then +(10,20,30)

  // X3D Transform composition: translation + scale, no rotation/center.
  Mat4 m = transformMatrix(/*translation*/{5,0,0}, /*rotation*/{0,0,1,0},
                           /*scale*/{2,2,2}, /*center*/{0,0,0},
                           /*scaleOrientation*/{0,0,1,0});
  CHECK((veq(m.transformPoint({1,0,0}), {7,0,0}))); // 1*2 + 5

  // center offset: scale about center (1,0,0) leaves that point fixed.
  Mat4 mc = transformMatrix({0,0,0}, {0,0,1,0}, {3,3,3}, {1,0,0}, {0,0,1,0});
  CHECK((veq(mc.transformPoint({1,0,0}), {1,0,0})));   // center fixed point
  CHECK((veq(mc.transformPoint({2,0,0}), {4,0,0})));   // (2-1)*3 + 1 = 4
  return;
}
