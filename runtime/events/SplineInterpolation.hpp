// SplineInterpolation.hpp
// Non-linear interpolation math for the spline interpolator family (ISO/IEC
// 19775-1 §19.2.4 Hermite spline, §19.4.13 Squad) and the §19.4.4 EaseInEaseOut
// fraction modifier. Factored out of the Systems so the math is unit-testable.
//
// Spec-correctness notes:
//   - hermiteSpline: the §19.2.4 algorithm v_s = S^T H C with the standard
//     Hermite basis (h00,h01,h10,h11), per-segment outgoing/incoming tangents
//     scaled by the F+/F- non-uniform-interval factors, author keyVelocity
//     (size 2 = endpoints, size N = per-key) or central-difference tangents,
//     normalizeVelocity scaling, closed-loop key/value wrap, and the open-curve
//     endpoint overrides (zero tangents when no author velocity).
//   - squadOrientation: Shoemake Squad in quaternion space (§19.4.13, [SHOE]);
//     intermediate quaternions auto-computed, clamped at the ends. N=2 reduces
//     to plain SLERP (== OrientationInterpolator).
//   - easeInEaseOut: the §19.4.4 ten-step algorithm; the eased *local* fraction
//     is remapped to the global key domain (matching shipping browsers, so the
//     output is directly usable as a downstream interpolator's set_fraction).
#ifndef X3D_RUNTIME_SPLINE_INTERPOLATION_HPP
#define X3D_RUNTIME_SPLINE_INTERPOLATION_HPP

#include "Interpolation.hpp" // Quat, slerp, quatFromRotation, rotationFromQuat

#include "x3d/core/X3Dtypes.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace x3d::runtime {

using namespace x3d::core;

// --- value-type vector ops for the spline value types -----------------------
inline float vadd(float a, float b) { return a + b; }
inline float vsub(float a, float b) { return a - b; }
inline float vscale(float a, float s) { return a * s; }
inline float vlen(float a) { return std::fabs(a); }

inline SFVec2f vadd(const SFVec2f &a, const SFVec2f &b) { return {a.x + b.x, a.y + b.y}; }
inline SFVec2f vsub(const SFVec2f &a, const SFVec2f &b) { return {a.x - b.x, a.y - b.y}; }
inline SFVec2f vscale(const SFVec2f &a, float s) { return {a.x * s, a.y * s}; }
inline float vlen(const SFVec2f &a) { return std::sqrt(a.x * a.x + a.y * a.y); }

inline SFVec3f vadd(const SFVec3f &a, const SFVec3f &b) {
  return {a.x + b.x, a.y + b.y, a.z + b.z};
}
inline SFVec3f vsub(const SFVec3f &a, const SFVec3f &b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}
inline SFVec3f vscale(const SFVec3f &a, float s) { return {a.x * s, a.y * s, a.z * s}; }
inline float vlen(const SFVec3f &a) {
  return std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

/**
 * @brief §19.2.4 Hermite spline interpolation for Spline{Scalar,Position,
 *        Position2D}Interpolator.
 * @param key monotonic key times (size N).
 * @param keyValue one value per key (size >= N).
 * @param keyVelocity empty (auto tangents), size 2 (first/last key velocity), or
 *        size N (per-key velocity); any other size is ignored.
 * @param closed wrap key/keyValue for continuous-velocity closed loops.
 * @param normalizeVelocity scale author velocities to chord-length tangents.
 * @param fraction the set_fraction input.
 */
template <typename T>
T hermiteSpline(const MFFloat &key, const std::vector<T> &keyValue,
                const std::vector<T> &keyVelocity, bool closed,
                bool normalizeVelocity, float fraction) {
  const std::size_t N = key.size();
  if (N == 0 || keyValue.size() < N) return T{};
  if (N == 1) return keyValue[0];
  if (fraction <= key.front()) return keyValue[0];
  if (fraction >= key.back()) return keyValue[N - 1];

  std::size_t i = 0;
  for (std::size_t k = 1; k < N; ++k) {
    if (fraction < key[k]) {
      i = k - 1;
      break;
    }
  }
  const float seg = key[i + 1] - key[i];
  const float s = seg > 0.0f ? (fraction - key[i]) / seg : 0.0f;

  const std::size_t kvN = keyVelocity.size();
  const bool perKey = (kvN == N);
  const bool endOnly = (kvN == 2);
  const bool hasVel = perKey || endOnly;

  float dtot = 0.0f;
  if (hasVel && normalizeVelocity) {
    for (std::size_t k = 0; k + 1 < N; ++k)
      dtot += vlen(vsub(keyValue[k], keyValue[k + 1]));
  }

  // Raw base tangent T_j: from keyVelocity (author, optionally normalized) or a
  // central difference (closed-wrapped at the ends).
  auto rawT = [&](long j) -> T {
    const bool author = perKey || (endOnly && (j == 0 || j == (long)N - 1));
    T base{};
    if (perKey) {
      base = keyVelocity[(std::size_t)j];
    } else if (endOnly && j == 0) {
      base = keyVelocity[0];
    } else if (endOnly && j == (long)N - 1) {
      base = keyVelocity[1];
    } else {
      T vprev = (j == 0) ? (closed ? keyValue[N - 2] : keyValue[0])
                         : keyValue[(std::size_t)(j - 1)];
      T vnext = (j == (long)N - 1) ? (closed ? keyValue[1] : keyValue[N - 1])
                                   : keyValue[(std::size_t)(j + 1)];
      base = vscale(vsub(vnext, vprev), 0.5f);
    }
    if (author && normalizeVelocity) {
      const float len = vlen(base);
      if (len > 1e-12f) base = vscale(base, dtot / len);
    }
    return base;
  };

  // Key time with closed wrap (t_{-1}=t_{N-2}, t_N=t_1 per §19.2.4).
  auto tt = [&](long j) -> float {
    if (j < 0) return key[N - 2];
    if (j >= (long)N) return key[1];
    return key[(std::size_t)j];
  };
  auto fplus = [&](long j) -> float {
    const float den = tt(j + 1) - tt(j - 1);
    return den != 0.0f ? 2.0f * (tt(j) - tt(j - 1)) / den : 1.0f;
  };
  auto fminus = [&](long j) -> float {
    const float den = tt(j + 1) - tt(j - 1);
    return den != 0.0f ? 2.0f * (tt(j + 1) - tt(j)) / den : 1.0f;
  };

  // Outgoing tangent T0_j and incoming tangent T1_j, with the open-curve
  // endpoint overrides (zero when no author velocity; raw author velocity at the
  // very ends).
  auto controlOut = [&](long j) -> T { // T0_j
    if (!closed && j == 0) return hasVel ? rawT(0) : T{};
    return vscale(rawT(j), fplus(j));
  };
  auto controlIn = [&](long j) -> T { // T1_j
    if (!closed && j == (long)N - 1) return hasVel ? rawT((long)N - 1) : T{};
    return vscale(rawT(j), fminus(j));
  };

  const T m0 = controlOut((long)i);     // T0_i
  const T m1 = controlIn((long)i + 1);  // T1_{i+1}

  const float s2 = s * s, s3 = s2 * s;
  const float h00 = 2.0f * s3 - 3.0f * s2 + 1.0f;
  const float h01 = -2.0f * s3 + 3.0f * s2;
  const float h10 = s3 - 2.0f * s2 + s;
  const float h11 = s3 - s2;

  return vadd(vadd(vscale(keyValue[i], h00), vscale(keyValue[i + 1], h01)),
              vadd(vscale(m0, h10), vscale(m1, h11)));
}

// --- Quaternion algebra for Squad ------------------------------------------
inline Quat quatMul(const Quat &a, const Quat &b) {
  return Quat{a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
              a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
              a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
              a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}
inline Quat quatConj(const Quat &q) { return Quat{-q.x, -q.y, -q.z, q.w}; } // unit inverse

/** @brief log of a unit quaternion -> pure quaternion (w=0). */
inline Quat quatLog(const Quat &q) {
  const double v = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z);
  if (v < 1e-12) return Quat{0.0, 0.0, 0.0, 0.0};
  const double w = std::clamp(q.w, -1.0, 1.0);
  const double f = std::atan2(v, w) / v;
  return Quat{q.x * f, q.y * f, q.z * f, 0.0};
}
/** @brief exp of a pure quaternion (w=0) -> unit quaternion. */
inline Quat quatExp(const Quat &q) {
  const double v = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z);
  if (v < 1e-12) return Quat{0.0, 0.0, 0.0, 1.0};
  const double s = std::sin(v) / v;
  return Quat{q.x * s, q.y * s, q.z * s, std::cos(v)};
}
/** @brief Shoemake intermediate (control) quaternion at key i. */
inline Quat squadIntermediate(const Quat &qprev, const Quat &qi, const Quat &qnext) {
  const Quat qinv = quatConj(qi);
  const Quat l1 = quatLog(quatMul(qinv, qnext));
  const Quat l2 = quatLog(quatMul(qinv, qprev));
  const Quat e{-(l1.x + l2.x) / 4.0, -(l1.y + l2.y) / 4.0, -(l1.z + l2.z) / 4.0, 0.0};
  return quatMul(qi, quatExp(e));
}

/**
 * @brief §19.4.13 SquadOrientationInterpolator: spherical cubic (Squad)
 *        interpolation of a rotation list. N=2 reduces to SLERP.
 */
inline SFRotation squadOrientation(const MFFloat &key,
                                   const std::vector<SFRotation> &keyValue,
                                   float fraction) {
  const std::size_t N = key.size();
  if (N == 0 || keyValue.size() < N) return SFRotation{0.0f, 0.0f, 1.0f, 0.0f};
  if (N == 1) return keyValue[0];
  if (fraction <= key.front()) return keyValue[0];
  if (fraction >= key.back()) return keyValue[N - 1];

  std::size_t i = 0;
  for (std::size_t k = 1; k < N; ++k) {
    if (fraction < key[k]) {
      i = k - 1;
      break;
    }
  }
  const float seg = key[i + 1] - key[i];
  const float s = seg > 0.0f ? (fraction - key[i]) / seg : 0.0f;

  std::vector<Quat> q(N);
  for (std::size_t k = 0; k < N; ++k) q[k] = quatFromRotation(keyValue[k]);

  auto inter = [&](std::size_t j) -> Quat {
    if (j == 0 || j == N - 1) return q[j]; // clamp at the ends
    return squadIntermediate(q[j - 1], q[j], q[j + 1]);
  };
  const Quat a0 = inter(i), a1 = inter(i + 1);
  const Quat r = slerp(slerp(q[i], q[i + 1], s), slerp(a0, a1, s),
                       2.0 * s * (1.0 - s));
  return rotationFromQuat(r);
}

/**
 * @brief §19.4.4 EaseInEaseOut: modify a time fraction with ease-in/ease-out.
 * @details easeInEaseOut pairs are (easeIn, easeOut); e_out = eieo[i].y (key i),
 *          e_in = eieo[i+1].x (key i+1). Returns the eased fraction remapped to
 *          the global key domain. Falls back to the identity (pass-through) when
 *          the key/easeInEaseOut data is insufficient.
 */
inline float easeInEaseOut(const MFFloat &key, const std::vector<SFVec2f> &eieo,
                           float fraction) {
  const std::size_t N = key.size();
  if (N < 2 || eieo.size() < N) return fraction;
  if (fraction <= key.front()) return key.front();
  if (fraction >= key.back()) return key.back();

  std::size_t i = 0;
  for (std::size_t k = 1; k < N; ++k) {
    if (fraction < key[k]) {
      i = k - 1;
      break;
    }
  }
  const float span = key[i + 1] - key[i];
  const float u = span > 0.0f ? (fraction - key[i]) / span : 0.0f; // local fraction
  float eout = eieo[i].y;     // easeOut for key i
  float ein = eieo[i + 1].x;  // easeIn for key i+1
  const float S = ein + eout;

  float modified;
  if (S < 0.0f) {
    modified = u;
  } else {
    if (S > 1.0f) {
      ein /= S;
      eout /= S;
    }
    const float t = 1.0f / (2.0f - eout - ein);
    if (u < eout)
      modified = (t / eout) * u * u;
    else if (u < 1.0f - ein)
      modified = t * (2.0f * u - eout);
    else
      modified = 1.0f - (t * (1.0f - u) * (1.0f - u)) / ein;
  }
  return key[i] + modified * span; // remap local -> global
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_SPLINE_INTERPOLATION_HPP
