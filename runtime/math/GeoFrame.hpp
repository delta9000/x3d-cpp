// GeoFrame.hpp — SDK-side geospatial conversions and local frames (ISO/IEC
// 19775-1 §25.2, ADR-0053).
//
// Everything here is identical for every GeoProjection backend: axis order
// ("latitude_first"/"longitude_first", "northing_first"/"easting_first"),
// units (GD latitude/longitude in degrees, lengths in metres), the "WGS84"
// geoid-height option, Web Mercator, the local east/up/south basis, and the
// GeoOrigin frame that turns geocentric metres into the X3D world.
//
// Precision: geocentric values are ~6.4e6 m, so all arithmetic stays in double
// until a point is made relative to its GeoOrigin; only then is it narrowed to
// the float SFVec3f the rest of the runtime uses (§25.2.5).
//
// The process-wide backend defaults to the first-party BuiltinGeoProjection.
// An application may install another (for example the PROJ backend) with
// setProjection() before building scenes; conversions are otherwise pure.
#ifndef X3D_RUNTIME_MATH_GEO_FRAME_HPP
#define X3D_RUNTIME_MATH_GEO_FRAME_HPP

#include "GeoBuiltinProjection.hpp"
#include "GeoProjection.hpp"
#include "Mat4.hpp"

#include <cmath>
#include <memory>

namespace x3d::runtime::geo {

inline constexpr double kDegToRad = 3.14159265358979323846 / 180.0;
inline constexpr double kRadToDeg = 180.0 / 3.14159265358979323846;
inline constexpr double kWebMercatorRadius = 6378137.0;  // WGS84 a (EPSG:3857 sphere)

namespace detail {
inline std::shared_ptr<const GeoProjection> &projectionSlot() {
  static std::shared_ptr<const GeoProjection> slot = std::make_shared<BuiltinGeoProjection>();
  return slot;
}
} // namespace detail

/// The process-wide projection backend (never null).
inline const GeoProjection &projection() { return *detail::projectionSlot(); }

/// Install a backend for subsequent conversions; null restores the built-in one.
/// Call before building scenes; the setter is not synchronised.
inline void setProjection(std::shared_ptr<const GeoProjection> p) {
  detail::projectionSlot() = p ? std::move(p) : std::make_shared<BuiltinGeoProjection>();
}

/// Authored coordinate -> geodetic (radians, metres above the ellipsoid).
inline bool toGeodetic(const GeoSystem &s, const SFVec3d &c, double &lat, double &lon, double &h,
                       const GeoProjection &p = projection()) {
  using F = GeoSystem::Frame;
  switch (s.frame) {
  case F::GD:
    lat = (s.longitudeFirst ? c.y : c.x) * kDegToRad;
    lon = (s.longitudeFirst ? c.x : c.y) * kDegToRad;
    h = c.z;
    break;
  case F::UTM: {
    const double easting = s.eastingFirst ? c.x : c.y;
    const double northing = s.eastingFirst ? c.y : c.x;
    if (!p.utmToGeodetic(s.ellipsoid, s.zone, s.south, easting, northing, lat, lon)) return false;
    h = c.z;
    break;
  }
  case F::WM:
    // EPSG:3857: spherical Mercator of WGS84 geodetic coordinates.
    lon = c.x / kWebMercatorRadius;
    lat = 2.0 * std::atan(std::exp(c.y / kWebMercatorRadius)) - 0.5 * 3.14159265358979323846;
    h = c.z;
    break;
  case F::GC:
    return p.geocentricToGeodetic(Ellipsoid{}, c, lat, lon, h);
  }
  if (s.geoidHeights && s.frame != F::GC) {
    double n = 0;  // §25.2.3 "WGS84": elevation above the geoid; N = 0 without a model
    if (p.geoidUndulation(lat, lon, n)) h += n;
  }
  return std::isfinite(lat) && std::isfinite(lon) && std::isfinite(h);
}

/// Authored coordinate -> earth-fixed geocentric metres.
inline bool toGeocentric(const GeoSystem &s, const SFVec3d &c, SFVec3d &gc,
                         const GeoProjection &p = projection()) {
  if (s.frame == GeoSystem::Frame::GC) { gc = c; return true; }
  double lat, lon, h;
  if (!toGeodetic(s, c, lat, lon, h, p)) return false;
  return p.geodeticToGeocentric(s.ellipsoid, lat, lon, h, gc);
}

/// Earth-fixed geocentric metres -> a coordinate authored in `s`.
inline bool fromGeocentric(const GeoSystem &s, const SFVec3d &gc, SFVec3d &c,
                           const GeoProjection &p = projection()) {
  using F = GeoSystem::Frame;
  if (s.frame == F::GC) { c = gc; return true; }
  double lat, lon, h;
  if (!p.geocentricToGeodetic(s.ellipsoid, gc, lat, lon, h)) return false;
  if (s.geoidHeights) {
    double n = 0;
    if (p.geoidUndulation(lat, lon, n)) h -= n;
  }
  switch (s.frame) {
  case F::GD:
    c = s.longitudeFirst ? SFVec3d{lon * kRadToDeg, lat * kRadToDeg, h}
                         : SFVec3d{lat * kRadToDeg, lon * kRadToDeg, h};
    return true;
  case F::UTM: {
    double e, n;
    if (!p.geodeticToUtm(s.ellipsoid, s.zone, s.south, lat, lon, e, n)) return false;
    c = s.eastingFirst ? SFVec3d{e, n, h} : SFVec3d{n, e, h};
    return true;
  }
  case F::WM:
    c = SFVec3d{kWebMercatorRadius * lon,
                kWebMercatorRadius * std::log(std::tan(0.25 * 3.14159265358979323846 + 0.5 * lat)),
                h};
    return std::isfinite(c.y);
  case F::GC:
    break;
  }
  return false;
}

/// The local tangent basis at a geodetic position (§25.3.3/§25.3.11): X3D +X
/// is east, +Y is up (the ellipsoid normal) and +Z points south (−Z north).
struct LocalBasis {
  SFVec3d east{1, 0, 0}, up{0, 1, 0}, south{0, 0, 1};
};

inline LocalBasis localBasis(double lat, double lon) {
  const double sl = std::sin(lat), cl = std::cos(lat), so = std::sin(lon), co = std::cos(lon);
  LocalBasis b;
  b.east = SFVec3d{-so, co, 0.0};
  b.up = SFVec3d{cl * co, cl * so, sl};
  b.south = SFVec3d{sl * co, sl * so, -cl};  // = −north
  return b;
}

/// Maps earth-fixed geocentric metres into the X3D world of one node: relative
/// to its GeoOrigin (if any) and, with rotateYUp, rotated so the origin's up is
/// +Y (§25.3.6). Without a GeoOrigin the world is geocentric itself.
struct OriginFrame {
  SFVec3d origin{0, 0, 0};
  bool rotateYUp = false;
  LocalBasis basis{};  // at the origin; used only when rotateYUp

  SFVec3d rotate(const SFVec3d &d) const {
    if (!rotateYUp) return d;
    // World = basisᵀ · geocentric: components along east, up, south.
    auto dot = [](const SFVec3d &a, const SFVec3d &b) { return a.x * b.x + a.y * b.y + a.z * b.z; };
    return SFVec3d{dot(d, basis.east), dot(d, basis.up), dot(d, basis.south)};
  }
  SFVec3d toWorldD(const SFVec3d &gc) const {
    return rotate(SFVec3d{gc.x - origin.x, gc.y - origin.y, gc.z - origin.z});
  }
  SFVec3f toWorld(const SFVec3d &gc) const {
    const SFVec3d w = toWorldD(gc);
    return SFVec3f{static_cast<float>(w.x), static_cast<float>(w.y), static_cast<float>(w.z)};
  }
  SFVec3f directionToWorld(const SFVec3d &d) const {
    const SFVec3d w = rotate(d);
    return SFVec3f{static_cast<float>(w.x), static_cast<float>(w.y), static_cast<float>(w.z)};
  }
};

/// Build the frame of a GeoOrigin authored at `coords` in `sys`.
inline bool makeOriginFrame(const GeoSystem &sys, const SFVec3d &coords, bool rotateYUp,
                            OriginFrame &out, const GeoProjection &p = projection()) {
  SFVec3d gc;
  if (!toGeocentric(sys, coords, gc, p)) return false;
  out.origin = gc;
  out.rotateYUp = rotateYUp;
  if (rotateYUp) {
    double lat, lon, h;
    if (!p.geocentricToGeodetic(Ellipsoid{}, gc, lat, lon, h)) return false;
    out.basis = localBasis(lat, lon);
  }
  return true;
}

/// The world matrix of a local tangent frame anchored at an authored
/// coordinate (GeoLocation, GeoTransform's geoCenter, GeoViewpoint): columns
/// are the world-space east / up / south axes, translation is the anchor.
inline bool tangentFrame(const GeoSystem &sys, const SFVec3d &coords, const OriginFrame &origin,
                         Mat4 &out, const GeoProjection &p = projection()) {
  double lat, lon, h;
  if (!toGeodetic(sys, coords, lat, lon, h, p)) return false;
  SFVec3d gc;
  if (!p.geodeticToGeocentric(sys.ellipsoid, lat, lon, h, gc)) return false;
  const LocalBasis b = localBasis(lat, lon);
  const SFVec3f x = origin.directionToWorld(b.east), y = origin.directionToWorld(b.up),
                z = origin.directionToWorld(b.south), t = origin.toWorld(gc);
  out = Mat4::identity();
  out.m[0] = x.x; out.m[1] = x.y; out.m[2] = x.z;
  out.m[4] = y.x; out.m[5] = y.y; out.m[6] = y.z;
  out.m[8] = z.x; out.m[9] = z.y; out.m[10] = z.z;
  out.m[12] = t.x; out.m[13] = t.y; out.m[14] = t.z;
  return true;
}

} // namespace x3d::runtime::geo

#endif // X3D_RUNTIME_MATH_GEO_FRAME_HPP
