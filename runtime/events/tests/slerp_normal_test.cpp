#include "doctest/doctest.h"
// slerp_normal_test.cpp
// Regression test for RTC-8: NormalInterpolator slerpNormal must not produce NaN
// for antipodal inputs (dot ~ -1), where acos(-1)=PI and sin(PI)~0 previously
// divided to NaN. Also checks the well-behaved 90-degree arc still interpolates.
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "Interpolation.hpp"

#include <cmath>
#include <iostream>
#include <string>

using namespace x3d;
using namespace x3d::runtime;

namespace {

int failures = 0;

void check(bool cond, const std::string &what) {
  if (!cond) {
    std::cerr << "FAIL: " << what << "\n";
    ++failures;
  } else {
    std::cout << "ok: " << what << "\n";
  }
}

bool finite3(const SFVec3f &v) {
  return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
bool isUnit(const SFVec3f &v) {
  double len = std::sqrt((double)v.x * v.x + (double)v.y * v.y + (double)v.z * v.z);
  return std::fabs(len - 1.0) < 1e-5;
}
bool near(float a, float b) { return std::fabs(a - b) < 1e-4f; }

// Antipodal normals (dot == -1 exactly): must return a finite unit vector.
void test_antipodal_no_nan() {
  SFVec3f r = slerpNormal({0, 0, 1}, {0, 0, -1}, 0.5f);
  check(finite3(r), "antipodal slerpNormal returns finite (no NaN)");
  check(isUnit(r), "antipodal slerpNormal returns a unit vector");
}

// Near-antipodal: dot just above -1 must also stay finite/unit.
void test_near_antipodal_no_nan() {
  // Slightly perturbed antipode.
  double e = 1e-7;
  double len = std::sqrt(e * e + 1.0);
  SFVec3f b{(float)(e / len), 0, (float)(-1.0 / len)};
  SFVec3f r = slerpNormal({0, 0, 1}, b, 0.5f);
  check(finite3(r), "near-antipodal slerpNormal returns finite (no NaN)");
  check(isUnit(r), "near-antipodal slerpNormal returns a unit vector");
}

// Endpoints (t=0, t=1) are exact.
void test_endpoints() {
  SFVec3f a{1, 0, 0}, b{0, 1, 0};
  SFVec3f r0 = slerpNormal(a, b, 0.0f);
  SFVec3f r1 = slerpNormal(a, b, 1.0f);
  check(near(r0.x, 1) && near(r0.y, 0) && near(r0.z, 0), "t=0 returns a");
  check(near(r1.x, 0) && near(r1.y, 1) && near(r1.z, 0), "t=1 returns b");
}

// 90-degree apart: midpoint lies on the great-circle arc at 45 degrees.
void test_ninety_degrees() {
  SFVec3f a{1, 0, 0}, b{0, 1, 0};
  SFVec3f m = slerpNormal(a, b, 0.5f);
  check(finite3(m) && isUnit(m), "90-degree midpoint finite unit");
  float c = (float)(std::sqrt(2.0) / 2.0); // cos 45
  check(near(m.x, c) && near(m.y, c) && near(m.z, 0),
        "90-degree midpoint is the 45-degree point on the arc");
}

// Identical normals (dot == +1): finite unit, equals input.
void test_identical() {
  SFVec3f r = slerpNormal({0, 1, 0}, {0, 1, 0}, 0.5f);
  check(finite3(r) && isUnit(r), "identical normals finite unit");
  check(near(r.y, 1), "identical normals return the input direction");
}

} // namespace

TEST_CASE("slerp_normal_test") {
  test_antipodal_no_nan();
  test_near_antipodal_no_nan();
  test_endpoints();
  test_ninety_degrees();
  test_identical();

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    CHECK(false); return;
  }
  std::cout << "all slerpNormal tests passed\n";
  return;
}
