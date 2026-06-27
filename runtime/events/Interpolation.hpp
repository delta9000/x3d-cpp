// Interpolation.hpp
// Shared interpolation math for the interpolator-system family. Factored out of
// the original PositionInterpolatorBehavior so every interpolator reuses one
// key-span lookup and the color/orientation conversions live in one place.
//
// Spec-correctness notes (X3D 19775-1 Interpolation component):
//   - locateKeySpan: piecewise lookup over a monotonic key[], clamping at both
//     ends. Each interpolator supplies only its per-component lerp.
//   - ColorInterpolator interpolates in HSV (not RGB), lerping hue along the
//     shorter arc. rgbToHsv/hsvToRgb are the Foley & van Dam conversions.
//   - OrientationInterpolator uses quaternion SLERP, choosing the shorter path
//     (negate one quat when dot < 0) and falling back to NLERP near the
//     degenerate (sin(omega) ~ 0) case.
#ifndef X3D_RUNTIME_INTERPOLATION_HPP
#define X3D_RUNTIME_INTERPOLATION_HPP

#include "x3d/core/X3Dtypes.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace x3d::runtime {

/**
 * @brief Bracketing indices and in-segment parameter for a fraction over key[].
 * @details `lo`/`hi` bracket `fraction` in the monotonic key array; `t` is the
 *          in-segment parameter in [0,1]. `clamped` is true when the fraction
 *          fell at or outside an end (then lo == hi and t == 0).
 */
struct KeySpan {
  std::size_t lo = 0;
  std::size_t hi = 0;
  float t = 0.0f;
  bool clamped = false;
};

/**
 * @brief Locate the key span bracketing `fraction`, clamping at both ends.
 * @details Empty/singleton keys are the caller's responsibility (returns a
 *          clamped span at index 0). For an interior fraction, returns the
 *          lower/upper key indices and the normalized parameter between them.
 */
inline KeySpan locateKeySpan(const MFFloat &key, float fraction) {
  KeySpan span;
  if (key.size() < 2) {
    span.lo = span.hi = 0;
    span.t = 0.0f;
    span.clamped = true;
    return span;
  }
  if (fraction <= key.front()) {
    span.lo = span.hi = 0;
    span.t = 0.0f;
    span.clamped = true;
    return span;
  }
  if (fraction >= key.back()) {
    span.lo = span.hi = key.size() - 1;
    span.t = 0.0f;
    span.clamped = true;
    return span;
  }
  for (std::size_t i = 1; i < key.size(); ++i) {
    if (fraction <= key[i]) {
      float seg = key[i] - key[i - 1];
      span.lo = i - 1;
      span.hi = i;
      span.t = seg > 0.0f ? (fraction - key[i - 1]) / seg : 0.0f;
      span.clamped = false;
      return span;
    }
  }
  // Fallthrough (non-monotonic key): clamp to the back.
  span.lo = span.hi = key.size() - 1;
  span.t = 0.0f;
  span.clamped = true;
  return span;
}

// --- Scalar / vector lerps -------------------------------------------------

inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }

inline SFVec2f lerpVec2(const SFVec2f &a, const SFVec2f &b, float t) {
  return SFVec2f{lerpf(a.x, b.x, t), lerpf(a.y, b.y, t)};
}

inline SFVec3f lerpVec3(const SFVec3f &a, const SFVec3f &b, float t) {
  return SFVec3f{lerpf(a.x, b.x, t), lerpf(a.y, b.y, t), lerpf(a.z, b.z, t)};
}

// --- HSV color (Foley & van Dam, per flux anmhsvcolor.cpp) ----------------

struct Hsv {
  float h; // degrees, [0,360)
  float s; // [0,1]
  float v; // [0,1]
};

/**
 * @brief Convert an RGB color to HSV (hue in degrees). Achromatic hue is 0.
 */
inline Hsv rgbToHsv(const SFColor &c) {
  float rgbmax = std::max(std::max(c.r, c.g), c.b);
  float rgbmin = std::min(std::min(c.r, c.g), c.b);
  float diff = rgbmax - rgbmin;

  Hsv out;
  out.v = rgbmax;
  out.s = (rgbmax != 0.0f) ? diff / rgbmax : 0.0f;

  if (out.s == 0.0f) {
    out.h = 0.0f;
  } else {
    float rc = (rgbmax - c.r) / diff;
    float gc = (rgbmax - c.g) / diff;
    float bc = (rgbmax - c.b) / diff;
    float h;
    if (c.r == rgbmax) {
      h = bc - gc;
    } else if (c.g == rgbmax) {
      h = 2.0f + rc - bc;
    } else {
      h = 4.0f + gc - rc;
    }
    h *= 60.0f;
    h = std::fmod(h, 360.0f);
    if (h < 0.0f) {
      h += 360.0f;
    }
    out.h = h;
  }
  return out;
}

/**
 * @brief Convert HSV (hue in degrees) back to an RGB color.
 */
inline SFColor hsvToRgb(const Hsv &in) {
  if (in.s == 0.0f) {
    return SFColor{in.v, in.v, in.v};
  }
  float hue = in.h / 60.0f;
  int i = static_cast<int>(std::floor(hue));
  float f = hue - static_cast<float>(i);
  float p = in.v * (1.0f - in.s);
  float q = in.v * (1.0f - in.s * f);
  float t = in.v * (1.0f - in.s * (1.0f - f));
  switch (((i % 6) + 6) % 6) {
  case 0:
    return SFColor{in.v, t, p};
  case 1:
    return SFColor{q, in.v, p};
  case 2:
    return SFColor{p, in.v, t};
  case 3:
    return SFColor{p, q, in.v};
  case 4:
    return SFColor{t, p, in.v};
  default:
    return SFColor{in.v, p, q};
  }
}

/**
 * @brief Interpolate two RGB colors in HSV, lerping hue along the shorter arc.
 */
inline SFColor lerpColorHsv(const SFColor &a, const SFColor &b, float t) {
  Hsv ha = rgbToHsv(a);
  Hsv hb = rgbToHsv(b);

  // Shorter hue arc.
  float dh = hb.h - ha.h;
  if (dh > 180.0f) {
    dh -= 360.0f;
  } else if (dh < -180.0f) {
    dh += 360.0f;
  }
  float h = ha.h + dh * t;
  h = std::fmod(h, 360.0f);
  if (h < 0.0f) {
    h += 360.0f;
  }
  Hsv out{h, lerpf(ha.s, hb.s, t), lerpf(ha.v, hb.v, t)};
  return hsvToRgb(out);
}

// --- Quaternion SLERP for orientation/normal (per flux anmquaternion.cpp) --

struct Quat {
  double x, y, z, w;
};

/** @brief Build a unit quaternion from an axis-angle SFRotation. */
inline Quat quatFromRotation(const SFRotation &r) {
  // Normalize the axis (degenerate axis -> identity-ish about +Y).
  double ax = r.x, ay = r.y, az = r.z;
  double len = std::sqrt(ax * ax + ay * ay + az * az);
  if (len < 1e-12) {
    ax = 0.0;
    ay = 1.0;
    az = 0.0;
  } else {
    ax /= len;
    ay /= len;
    az /= len;
  }
  double s = std::sin(r.angle * 0.5);
  return Quat{ax * s, ay * s, az * s, std::cos(r.angle * 0.5)};
}

/** @brief Convert a unit quaternion back to an axis-angle SFRotation. */
inline SFRotation rotationFromQuat(const Quat &q) {
  double vlen = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z);
  if (vlen < 1e-9) {
    return SFRotation{0.0f, 1.0f, 0.0f, 0.0f};
  }
  double w = std::clamp(q.w, -1.0, 1.0);
  float angle = static_cast<float>(std::acos(w) * 2.0);
  return SFRotation{static_cast<float>(q.x / vlen),
                    static_cast<float>(q.y / vlen),
                    static_cast<float>(q.z / vlen), angle};
}

/**
 * @brief Spherical linear interpolation of two unit quaternions (shorter path).
 * @details Negates q2 when the dot product is negative so the interpolation
 *          takes the shorter arc; falls back to NLERP-style linear blend when
 *          the quaternions are nearly parallel (sin(omega) ~ 0).
 */
inline Quat slerp(const Quat &q1, const Quat &q2, double ratio) {
  double cosomega = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
  double sign = 1.0;
  if (cosomega < 0.0) {
    cosomega = -cosomega;
    sign = -1.0;
  }

  double scale1, scale2;
  if ((1.0 - cosomega) > 1e-9) {
    double omega = std::acos(cosomega);
    double sinomega = std::sin(omega);
    scale1 = std::sin((1.0 - ratio) * omega) / sinomega;
    scale2 = std::sin(ratio * omega) / sinomega;
  } else {
    scale1 = 1.0 - ratio;
    scale2 = ratio;
  }
  scale2 *= sign;

  return Quat{scale1 * q1.x + scale2 * q2.x, scale1 * q1.y + scale2 * q2.y,
              scale1 * q1.z + scale2 * q2.z, scale1 * q1.w + scale2 * q2.w};
}

/**
 * @brief SLERP between two axis-angle rotations, returning an axis-angle
 * result.
 */
inline SFRotation slerpRotation(const SFRotation &a, const SFRotation &b,
                                float t) {
  return rotationFromQuat(
      slerp(quatFromRotation(a), quatFromRotation(b), static_cast<double>(t)));
}

/**
 * @brief Interpolate two unit normals on the sphere (SLERP), then renormalize.
 * @details Treats the vectors as quaternions with w=0 for the SLERP blend;
 *          near-parallel inputs fall back to a normalized linear blend.
 */
inline SFVec3f slerpNormal(const SFVec3f &a, const SFVec3f &b, float t) {
  double ax = a.x, ay = a.y, az = a.z;
  double bx = b.x, by = b.y, bz = b.z;
  double dot = ax * bx + ay * by + az * bz;
  dot = std::clamp(dot, -1.0, 1.0);

  double rx, ry, rz;
  if ((1.0 - std::fabs(dot)) > 1e-9) {
    double omega = std::acos(dot);
    double sinomega = std::sin(omega);
    double s1 = std::sin((1.0 - t) * omega) / sinomega;
    double s2 = std::sin(t * omega) / sinomega;
    rx = s1 * ax + s2 * bx;
    ry = s1 * ay + s2 * by;
    rz = s1 * az + s2 * bz;
  } else {
    rx = ax + (bx - ax) * t;
    ry = ay + (by - ay) * t;
    rz = az + (bz - az) * t;
  }
  double len = std::sqrt(rx * rx + ry * ry + rz * rz);
  if (len < 1e-12) {
    return a;
  }
  return SFVec3f{static_cast<float>(rx / len), static_cast<float>(ry / len),
                 static_cast<float>(rz / len)};
}

// Generic single-value interpolation: clamp at the ends, else lerp the
// bracketing keyValues with the supplied function. Covers Scalar/Position/
// Position2D/Color/Orientation (the only variation is the value type + lerp fn).
template <typename T, typename Lerp>
T interpolateValue(const MFFloat &key, const std::vector<T> &keyValue,
                   float fraction, Lerp lerp) {
  if (key.empty() || keyValue.empty()) return T{};
  KeySpan s = locateKeySpan(key, fraction);
  if (s.clamped || s.hi >= keyValue.size())
    return keyValue[std::min(s.lo, keyValue.size() - 1)];
  return lerp(keyValue[s.lo], keyValue[s.hi], s.t);
}

// Generic multi-value interpolation: the flat keyValue (numKeys * numPoints) is
// reshaped by numKeys; each output element blends the bracketing rows with the
// supplied per-element function. Covers Coordinate/Coordinate2D/Normal.
template <typename T, typename Lerp>
std::vector<T> interpolateMulti(const MFFloat &key,
                                const std::vector<T> &keyValue, float fraction,
                                Lerp lerp) {
  if (key.empty() || keyValue.empty()) return {};
  std::size_t numKeys = key.size();
  std::size_t numPoints = keyValue.size() / numKeys;
  if (numPoints == 0) return {};
  KeySpan s = locateKeySpan(key, fraction);
  std::vector<T> out;
  out.reserve(numPoints);
  if (s.clamped) {
    std::size_t base = s.lo * numPoints;
    for (std::size_t p = 0; p < numPoints; ++p) out.push_back(keyValue[base + p]);
    return out;
  }
  std::size_t baseLo = s.lo * numPoints, baseHi = s.hi * numPoints;
  for (std::size_t p = 0; p < numPoints; ++p)
    out.push_back(lerp(keyValue[baseLo + p], keyValue[baseHi + p], s.t));
  return out;
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_INTERPOLATION_HPP
