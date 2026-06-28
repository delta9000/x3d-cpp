// draw_math.hpp — small renderer-side math helpers for hot draw paths.
#ifndef X3D_POC_RENDERER_DRAW_MATH_HPP
#define X3D_POC_RENDERER_DRAW_MATH_HPP

#include "Mat4.hpp"
#include <array>
#include <cmath>

namespace poc {

inline std::array<float, 9> identityNormalMatrix3() {
  return {1.0f, 0.0f, 0.0f,
          0.0f, 1.0f, 0.0f,
          0.0f, 0.0f, 1.0f};
}

// 3x3 normal matrix = inverse-transpose of (view * model)'s upper-left 3x3,
// packed column-major for glUniformMatrix3fv. This avoids materializing the full
// 4x4 view-model product and avoids a general 4x4 inverse in the per-draw path.
inline std::array<float, 9> normalMatrix3(const x3d::runtime::Mat4 &view,
                                          const x3d::runtime::Mat4 &model) {
  float a[9] = {};
  for (int col = 0; col < 3; ++col) {
    for (int row = 0; row < 3; ++row) {
      float s = 0.0f;
      for (int k = 0; k < 3; ++k)
        s += view.m[k * 4 + row] * model.m[col * 4 + k];
      a[col * 3 + row] = s;
    }
  }

  const float a00 = a[0], a10 = a[1], a20 = a[2];
  const float a01 = a[3], a11 = a[4], a21 = a[5];
  const float a02 = a[6], a12 = a[7], a22 = a[8];

  const float c00 = a11 * a22 - a12 * a21;
  const float c01 = a12 * a20 - a10 * a22;
  const float c02 = a10 * a21 - a11 * a20;
  const float c10 = a02 * a21 - a01 * a22;
  const float c11 = a00 * a22 - a02 * a20;
  const float c12 = a01 * a20 - a00 * a21;
  const float c20 = a01 * a12 - a02 * a11;
  const float c21 = a02 * a10 - a00 * a12;
  const float c22 = a00 * a11 - a01 * a10;

  const float det = a00 * c00 + a01 * c01 + a02 * c02;
  if (std::fabs(det) < 1e-20f) return identityNormalMatrix3();
  const float invDet = 1.0f / det;

  // inverse(A)^T == cofactor(A) / det. Return column-major.
  return {c00 * invDet, c10 * invDet, c20 * invDet,
          c01 * invDet, c11 * invDet, c21 * invDet,
          c02 * invDet, c12 * invDet, c22 * invDet};
}

} // namespace poc

#endif // X3D_POC_RENDERER_DRAW_MATH_HPP
