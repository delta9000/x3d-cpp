#include "Billboard.hpp"
#include "doctest/doctest.h"

#include <array>
#include <cmath>

using namespace x3d::runtime;

namespace {
void checkRotation(const Mat4 &m, const SFVec3f &viewDirection) {
  using namespace vec3;
  const auto x = m.transformDirection({1, 0, 0});
  const auto y = m.transformDirection({0, 1, 0});
  const auto z = m.transformDirection({0, 0, 1});
  for (const auto value : m.m) CHECK(std::isfinite(value));
  CHECK(dot(x, x) == doctest::Approx(1.0f));
  CHECK(dot(y, y) == doctest::Approx(1.0f));
  CHECK(dot(z, z) == doctest::Approx(1.0f));
  CHECK(dot(x, y) == doctest::Approx(0.0f).epsilon(1e-5));
  CHECK(dot(x, z) == doctest::Approx(0.0f).epsilon(1e-5));
  CHECK(dot(y, z) == doctest::Approx(0.0f).epsilon(1e-5));
  CHECK(dot(cross(x, y), z) == doctest::Approx(1.0f));
  CHECK(dot(z, normalize(viewDirection)) == doctest::Approx(1.0f));
}
}

TEST_CASE("billboard viewer alignment remains a rotation for parallel or zero up") {
  const std::array<SFVec3f, 8> directions{{
      {0, 1, 0}, {0, -1, 0}, {1, 0, 0}, {-1, 0, 0},
      {0, 0, 1}, {0, 0, -1}, {1, 2, 3}, {-2, 3, -1}}};
  for (const auto &direction : directions) {
    CAPTURE(direction.x); CAPTURE(direction.y); CAPTURE(direction.z);
    for (const float sign : {1.0f, -1.0f, 0.0f}) {
      const SFVec3f up{sign * direction.x, sign * direction.y, sign * direction.z};
      checkRotation(billboardLocalMatrix(Mat4::identity(), direction, up, {0, 0, 0}), direction);
    }
  }
}

TEST_CASE("billboard nearly parallel up and transformed parent preserve basis") {
  checkRotation(billboardLocalMatrix(Mat4::identity(), {0, 1, 0},
                                    {1e-12f, 1, 0}, {0, 0, 0}), {0, 1, 0});
  const auto parent = Mat4::translation({3, 4, 5}) * Mat4::rotation({0, 0, 1, 0.7f});
  const SFVec3f localView{0, 1, 0};
  checkRotation(billboardLocalMatrix(parent, parent.transformPoint(localView),
                                    parent.transformDirection(localView), {0, 0, 0}), localView);
}

TEST_CASE("billboard ordinary up alignment and coincident viewer stay unchanged") {
  const auto m = billboardLocalMatrix(Mat4::identity(), {0, 0, 1}, {0, 1, 0}, {0, 0, 0});
  CHECK(m.m == Mat4::identity().m);
  CHECK(billboardLocalMatrix(Mat4::identity(), {0, 0, 0}, {0, 1, 0}, {0, 0, 0}).m ==
        Mat4::identity().m);
}
