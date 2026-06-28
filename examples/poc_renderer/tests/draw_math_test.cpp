// draw_math_test.cpp — pure renderer math tests. No GL context required.
#include "../draw_math.hpp"
#include "Mat4.hpp"
#include <array>
#include <cmath>
#include <iostream>

using x3d::core::SFRotation;
using x3d::core::SFVec3f;
using x3d::runtime::Mat4;

static int failures = 0;
static void check(bool c, const char *what) {
  if (!c) { std::cerr << "FAIL: " << what << "\n"; ++failures; }
  else    { std::cout << "ok: " << what << "\n"; }
}

static std::array<float, 9> referenceNormalMatrix3(const Mat4 &view,
                                                   const Mat4 &model) {
  const Mat4 viewModel = view * model;
  const Mat4 inv = viewModel.inverse();
  std::array<float, 9> out{};
  for (int c = 0; c < 3; ++c)
    for (int r = 0; r < 3; ++r)
      out[c * 3 + r] = inv.m[r * 4 + c];
  return out;
}

int main() {
  Mat4 view = Mat4::translation(SFVec3f{3.0f, -2.0f, 7.0f}) *
              Mat4::rotation(SFRotation{0.0f, 1.0f, 0.0f, 0.7f});
  Mat4 model = Mat4::translation(SFVec3f{-4.0f, 5.0f, 2.0f}) *
               Mat4::rotation(SFRotation{1.0f, 0.0f, 0.0f, 0.4f}) *
               Mat4::scale(SFVec3f{2.0f, 3.0f, 0.5f});

  const auto ref = referenceNormalMatrix3(view, model);
  const auto fast = poc::normalMatrix3(view, model);
  for (std::size_t i = 0; i < ref.size(); ++i)
    check(std::fabs(ref[i] - fast[i]) < 1e-4f,
          "fast normal matrix matches full inverse-transpose");

  const auto singular =
      poc::normalMatrix3(Mat4::identity(), Mat4::scale(SFVec3f{1.0f, 0.0f, 1.0f}));
  const auto identity = poc::normalMatrix3(Mat4::identity(), Mat4::identity());
  check(singular == identity, "singular normal matrix falls back to identity");

  return failures ? 1 : 0;
}
