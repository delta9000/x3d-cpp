// Mat4.hpp — minimal column-major 4x4 float matrix for world-transform math.
// Storage: std::array<float,16>, COLUMN-MAJOR. Element (row r, col c) is m[c*4+r].
// A column vector p (w=1) transforms as out = M * p. Builders + X3D Transform
// composition. No external dependency. namespace x3d::runtime.
#ifndef X3D_RUNTIME_MAT4_HPP
#define X3D_RUNTIME_MAT4_HPP

#include "Interpolation.hpp" // Quat, quatFromRotation
#include "x3d/core/X3Dtypes.hpp"      // SFVec3f, SFRotation

#include <array>
#include <cmath>

namespace x3d::runtime {

struct Mat4 {
  std::array<float, 16> m{}; // column-major; default zero

  static Mat4 identity() {
    Mat4 r;
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
  }

  // C = (*this) * rhs, column-major: C[c*4+r] = sum_k A[k*4+r]*B[c*4+k].
  Mat4 operator*(const Mat4 &b) const {
    Mat4 c; // zero
    for (int col = 0; col < 4; ++col)
      for (int row = 0; row < 4; ++row) {
        float s = 0.0f;
        for (int k = 0; k < 4; ++k)
          s += m[k * 4 + row] * b.m[col * 4 + k];
        c.m[col * 4 + row] = s;
      }
    return c;
  }

  // Transform a point (w=1).
  SFVec3f transformPoint(const SFVec3f &p) const {
    return SFVec3f{
        m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12],
        m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13],
        m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14]};
  }

  // Transform a direction (w=0): rotation/scale only, no translation.
  SFVec3f transformDirection(const SFVec3f &d) const {
    return SFVec3f{
        m[0]*d.x + m[4]*d.y + m[8]*d.z,
        m[1]*d.x + m[5]*d.y + m[9]*d.z,
        m[2]*d.x + m[6]*d.y + m[10]*d.z};
  }

  // General 4x4 inverse (column-major == OpenGL layout, standard adjugate form).
  // Returns identity() if (near-)singular.
  Mat4 inverse() const {
    std::array<float,16> inv;
    inv[0]  =  m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15] + m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
    inv[4]  = -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15] - m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
    inv[8]  =  m[4]*m[9]*m[15]  - m[4]*m[11]*m[13] - m[8]*m[5]*m[15] + m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
    inv[12] = -m[4]*m[9]*m[14]  + m[4]*m[10]*m[13] + m[8]*m[5]*m[14] - m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
    inv[1]  = -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15] - m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
    inv[5]  =  m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15] + m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
    inv[9]  = -m[0]*m[9]*m[15]  + m[0]*m[11]*m[13] + m[8]*m[1]*m[15] - m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
    inv[13] =  m[0]*m[9]*m[14]  - m[0]*m[10]*m[13] - m[8]*m[1]*m[14] + m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
    inv[2]  =  m[1]*m[6]*m[15]  - m[1]*m[7]*m[14]  - m[5]*m[2]*m[15] + m[5]*m[3]*m[14] + m[13]*m[2]*m[7]  - m[13]*m[3]*m[6];
    inv[6]  = -m[0]*m[6]*m[15]  + m[0]*m[7]*m[14]  + m[4]*m[2]*m[15] - m[4]*m[3]*m[14] - m[12]*m[2]*m[7]  + m[12]*m[3]*m[6];
    inv[10] =  m[0]*m[5]*m[15]  - m[0]*m[7]*m[13]  - m[4]*m[1]*m[15] + m[4]*m[3]*m[13] + m[12]*m[1]*m[7]  - m[12]*m[3]*m[5];
    inv[14] = -m[0]*m[5]*m[14]  + m[0]*m[6]*m[13]  + m[4]*m[1]*m[14] - m[4]*m[2]*m[13] - m[12]*m[1]*m[6]  + m[12]*m[2]*m[5];
    inv[3]  = -m[1]*m[6]*m[11]  + m[1]*m[7]*m[10]  + m[5]*m[2]*m[11] - m[5]*m[3]*m[10] - m[9]*m[2]*m[7]   + m[9]*m[3]*m[6];
    inv[7]  =  m[0]*m[6]*m[11]  - m[0]*m[7]*m[10]  - m[4]*m[2]*m[11] + m[4]*m[3]*m[10] + m[8]*m[2]*m[7]   - m[8]*m[3]*m[6];
    inv[11] = -m[0]*m[5]*m[11]  + m[0]*m[7]*m[9]   + m[4]*m[1]*m[11] - m[4]*m[3]*m[9]  - m[8]*m[1]*m[7]   + m[8]*m[3]*m[5];
    inv[15] =  m[0]*m[5]*m[10]  - m[0]*m[6]*m[9]   - m[4]*m[1]*m[10] + m[4]*m[2]*m[9]  + m[8]*m[1]*m[6]   - m[8]*m[2]*m[5];
    float det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
    if (std::fabs(det) < 1e-20f) return identity();
    float idet = 1.0f / det;
    Mat4 r;
    for (int i = 0; i < 16; ++i) r.m[i] = inv[i] * idet;
    return r;
  }

  static Mat4 translation(const SFVec3f &t) {
    Mat4 r = identity();
    r.m[12] = t.x; r.m[13] = t.y; r.m[14] = t.z;
    return r;
  }

  static Mat4 scale(const SFVec3f &s) {
    Mat4 r; // zero
    r.m[0] = s.x; r.m[5] = s.y; r.m[10] = s.z; r.m[15] = 1.0f;
    return r;
  }

  // Axis-angle SFRotation -> rotation matrix, via the existing quaternion path.
  static Mat4 rotation(const SFRotation &rot) {
    Quat q = quatFromRotation(rot); // unit quaternion (x,y,z,w)
    const float x = static_cast<float>(q.x), y = static_cast<float>(q.y),
                z = static_cast<float>(q.z), w = static_cast<float>(q.w);
    Mat4 r;
    r.m[0] = 1 - 2 * (y * y + z * z); r.m[1] = 2 * (x * y + w * z); r.m[2] = 2 * (x * z - w * y); r.m[3] = 0;
    r.m[4] = 2 * (x * y - w * z);     r.m[5] = 1 - 2 * (x * x + z * z); r.m[6] = 2 * (y * z + w * x); r.m[7] = 0;
    r.m[8] = 2 * (x * z + w * y);     r.m[9] = 2 * (y * z - w * x);     r.m[10] = 1 - 2 * (x * x + y * y); r.m[11] = 0;
    r.m[12] = 0; r.m[13] = 0; r.m[14] = 0; r.m[15] = 1;
    return r;
  }
};

// X3D Transform local matrix: M = T * C * R * SR * S * SR^-1 * C^-1
// (ISO/IEC 19775-1 Transform). Applied to a column vector right-to-left.
inline Mat4 transformMatrix(const SFVec3f &translation, const SFRotation &rotation,
                            const SFVec3f &scale, const SFVec3f &center,
                            const SFRotation &scaleOrientation) {
  Mat4 T = Mat4::translation(translation);
  Mat4 C = Mat4::translation(center);
  Mat4 Cinv = Mat4::translation(SFVec3f{-center.x, -center.y, -center.z});
  Mat4 R = Mat4::rotation(rotation);
  Mat4 SR = Mat4::rotation(scaleOrientation);
  Mat4 SRinv = Mat4::rotation(
      SFRotation{scaleOrientation.x, scaleOrientation.y, scaleOrientation.z,
                 -scaleOrientation.angle});
  Mat4 S = Mat4::scale(scale);
  return T * C * R * SR * S * SRinv * Cinv;
}

// Extract the rotation of a rigid matrix as an axis-angle SFRotation (Shepperd's
// quaternion-from-matrix; column-major element (row r,col c) = m[c*4+r]). Assumes
// the upper-left 3x3 is a pure rotation (no scale).
inline SFRotation rotationFromMatrix(const Mat4 &mat) {
  const double m00 = mat.m[0], m10 = mat.m[1], m20 = mat.m[2];
  const double m01 = mat.m[4], m11 = mat.m[5], m21 = mat.m[6];
  const double m02 = mat.m[8], m12 = mat.m[9], m22 = mat.m[10];
  const double tr = m00 + m11 + m22;
  Quat q{0, 0, 0, 1};
  if (tr > 0.0) {
    double s = std::sqrt(tr + 1.0) * 2.0;
    q.w = 0.25 * s; q.x = (m21 - m12) / s; q.y = (m02 - m20) / s; q.z = (m10 - m01) / s;
  } else if (m00 > m11 && m00 > m22) {
    double s = std::sqrt(1.0 + m00 - m11 - m22) * 2.0;
    q.w = (m21 - m12) / s; q.x = 0.25 * s; q.y = (m01 + m10) / s; q.z = (m02 + m20) / s;
  } else if (m11 > m22) {
    double s = std::sqrt(1.0 + m11 - m00 - m22) * 2.0;
    q.w = (m02 - m20) / s; q.x = (m01 + m10) / s; q.y = 0.25 * s; q.z = (m12 + m21) / s;
  } else {
    double s = std::sqrt(1.0 + m22 - m00 - m11) * 2.0;
    q.w = (m10 - m01) / s; q.x = (m02 + m20) / s; q.y = (m12 + m21) / s; q.z = 0.25 * s;
  }
  return rotationFromQuat(q);
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_MAT4_HPP
