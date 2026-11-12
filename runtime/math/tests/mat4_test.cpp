// mat4_test.cpp — column-major 4x4 math + X3D Transform composition.
#include "Mat4.hpp"
#include "doctest/doctest.h"
#include "scene_test_helpers.hpp"
#include <cmath>
#include <numbers>
using namespace x3d::runtime;
using x3d::test::feq;

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

  // multiply order: (T * S) applied to p scales then translates
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

TEST_CASE("rotationFromMatrix preserves the transformed basis") {
  for (const SFVec3f axis : {SFVec3f{1,0,0}, SFVec3f{0,1,0},
                            SFVec3f{0,0,1}, SFVec3f{1,2,3}}) {
    float maxDelta = 0.0f;
    for (int i = -10000; i <= 10004; ++i) {
      // Sweep small angles, then exercise each branch at ordinary rotations.
      const float angle = i <= 10000 ? i * 1e-7f :
          (i - 10000) * static_cast<float>(std::numbers::pi / 2.0);
      const Mat4 input = Mat4::rotation({axis.x, axis.y, axis.z, angle});
      const Mat4 result = Mat4::rotation(rotationFromMatrix(input));
      for (const SFVec3f basis : {SFVec3f{1,0,0}, SFVec3f{0,1,0}, SFVec3f{0,0,1}}) {
        const SFVec3f expected = input.transformDirection(basis);
        const SFVec3f actual = result.transformDirection(basis);
        const float dx = actual.x - expected.x, dy = actual.y - expected.y,
                    dz = actual.z - expected.z;
        maxDelta = std::max(maxDelta, std::sqrt(dx*dx + dy*dy + dz*dz));
      }
    }
    // The inputs have unit-length basis vectors; 1e-6 allows float rounding.
    CHECK_MESSAGE(maxDelta <= 1e-6f, "axis: ", axis.x, ", ", axis.y, ", ", axis.z);
  }
}
