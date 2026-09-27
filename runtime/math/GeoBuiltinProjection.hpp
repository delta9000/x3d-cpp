// GeoBuiltinProjection.hpp — the first-party GeoProjection backend (ADR-0053).
//
// Closed-form ellipsoid mathematics with no dependencies and no IO, so the
// default build handles every Geospatial frame:
//   * geodetic -> geocentric: the standard prime-vertical-radius formula;
//   * geocentric -> geodetic: fixed-point iteration on latitude with the
//     height form h = p·cosφ + z·sinφ − a²/N, which stays well conditioned at
//     the poles (converges to ~1e-15 rad in a few steps for terrestrial
//     points);
//   * UTM: Krüger's transverse Mercator series to sixth order in the third
//     flattening n (Karney 2011, "Transverse Mercator with an accuracy of a
//     few nanometers"), sub-millimetre within a UTM zone, with Newton's
//     method for the conformal-to-geodetic latitude on the inverse.
// No geoid: the "WGS84" geoSystem option falls back to ellipsoidal heights
// (geoidUndulation returns false) unless a geoid function is supplied.
#ifndef X3D_RUNTIME_MATH_GEO_BUILTIN_PROJECTION_HPP
#define X3D_RUNTIME_MATH_GEO_BUILTIN_PROJECTION_HPP

#include "GeoProjection.hpp"

#include <array>
#include <cmath>
#include <functional>
#include <utility>

namespace x3d::runtime::geo {

class BuiltinGeoProjection : public GeoProjection {
public:
  /// Optional WGS84 geoid: N(lat, lon) in metres, radians in. Lets an
  /// application plug in a grid (e.g. an EGM2008 raster) without a new backend.
  using GeoidFunction = std::function<bool(double lat, double lon, double &n)>;

  BuiltinGeoProjection() = default;
  explicit BuiltinGeoProjection(GeoidFunction geoid) : geoid_(std::move(geoid)) {}

  bool geodeticToGeocentric(const Ellipsoid &e, double lat, double lon, double h,
                            SFVec3d &gc) const override {
    if (!std::isfinite(lat) || !std::isfinite(lon) || !std::isfinite(h)) return false;
    const double e2 = e.e2();
    const double s = std::sin(lat), c = std::cos(lat);
    const double n = e.a / std::sqrt(1.0 - e2 * s * s);
    gc = SFVec3d{(n + h) * c * std::cos(lon), (n + h) * c * std::sin(lon),
                 (n * (1.0 - e2) + h) * s};
    return true;
  }

  bool geocentricToGeodetic(const Ellipsoid &e, const SFVec3d &gc, double &lat, double &lon,
                            double &h) const override {
    if (!std::isfinite(gc.x) || !std::isfinite(gc.y) || !std::isfinite(gc.z)) return false;
    const double e2 = e.e2();
    const double p = std::hypot(gc.x, gc.y);
    if (p < 1e-9 && std::fabs(gc.z) < 1e-9) return false;  // Earth's centre: undefined
    lon = (p < 1e-9) ? 0.0 : std::atan2(gc.y, gc.x);
    // Start from the latitude of the point projected along the ellipsoid normal
    // at the geocentric latitude; iterate φ = atan2(z, p·(1 − e²·N/(N + h))).
    double phi = std::atan2(gc.z, p * (1.0 - e2));
    for (int i = 0; i < 16; ++i) {
      const double s = std::sin(phi), c = std::cos(phi);
      const double n = e.a / std::sqrt(1.0 - e2 * s * s);
      h = p * c + gc.z * s - e.a * e.a / n;
      const double next = std::atan2(gc.z, p * (1.0 - e2 * n / (n + h)));
      const bool done = std::fabs(next - phi) < 1e-15;
      phi = next;
      if (done) break;
    }
    const double s = std::sin(phi), c = std::cos(phi);
    const double n = e.a / std::sqrt(1.0 - e2 * s * s);
    h = p * c + gc.z * s - e.a * e.a / n;
    lat = phi;
    return std::isfinite(lat) && std::isfinite(h);
  }

  bool utmToGeodetic(const Ellipsoid &e, int zone, bool south, double easting,
                     double northing, double &lat, double &lon) const override {
    if (zone < 1 || zone > 60 || !std::isfinite(easting) || !std::isfinite(northing))
      return false;
    const Krueger k(e);
    const double xi = (northing - (south ? kFalseNorthingSouth : 0.0)) / (kK0 * k.A);
    const double eta = (easting - kFalseEasting) / (kK0 * k.A);
    double xip = xi, etap = eta;
    for (int j = 1; j <= 6; ++j) {
      xip -= k.beta[j - 1] * std::sin(2 * j * xi) * std::cosh(2 * j * eta);
      etap -= k.beta[j - 1] * std::cos(2 * j * xi) * std::sinh(2 * j * eta);
    }
    // Conformal latitude τ' = tan χ, then solve for τ = tan φ (Karney eq. 19-21).
    const double sinhEta = std::sinh(etap), cosXi = std::cos(xip);
    const double taup = std::sin(xip) / std::hypot(sinhEta, cosXi);
    const double tau = tauFromTaup(taup, e.e2());
    lat = std::atan(tau);
    lon = centralMeridian(zone) + std::atan2(sinhEta, cosXi);
    return std::isfinite(lat) && std::isfinite(lon);
  }

  bool geodeticToUtm(const Ellipsoid &e, int zone, bool south, double lat, double lon,
                     double &easting, double &northing) const override {
    if (zone < 1 || zone > 60 || !std::isfinite(lat) || !std::isfinite(lon)) return false;
    const Krueger k(e);
    const double dlon = remainder(lon - centralMeridian(zone), 2.0 * kPi);
    const double taup = taupFromTau(std::tan(lat), e.e2());
    const double xip = std::atan2(taup, std::cos(dlon));
    const double etap = std::asinh(std::sin(dlon) / std::hypot(taup, std::cos(dlon)));
    double xi = xip, eta = etap;
    for (int j = 1; j <= 6; ++j) {
      xi += k.alpha[j - 1] * std::sin(2 * j * xip) * std::cosh(2 * j * etap);
      eta += k.alpha[j - 1] * std::cos(2 * j * xip) * std::sinh(2 * j * etap);
    }
    easting = kFalseEasting + kK0 * k.A * eta;
    northing = (south ? kFalseNorthingSouth : 0.0) + kK0 * k.A * xi;
    return std::isfinite(easting) && std::isfinite(northing);
  }

  bool geoidUndulation(double lat, double lon, double &n) const override {
    return geoid_ && geoid_(lat, lon, n);
  }

private:
  static constexpr double kPi = 3.14159265358979323846;
  static constexpr double kK0 = 0.9996;               // UTM scale on the central meridian
  static constexpr double kFalseEasting = 500000.0;
  static constexpr double kFalseNorthingSouth = 10000000.0;

  static double centralMeridian(int zone) { return (zone * 6.0 - 183.0) * kPi / 180.0; }
  static double remainder(double x, double y) { return std::remainder(x, y); }

  // Krüger series in the third flattening n (Karney 2011, eqs. 14, 35, 36).
  struct Krueger {
    double A = 0;
    std::array<double, 6> alpha{}, beta{};
    explicit Krueger(const Ellipsoid &e) {
      const double n = e.f() / (2.0 - e.f());
      const double n2 = n * n, n3 = n2 * n, n4 = n3 * n, n5 = n4 * n, n6 = n5 * n;
      A = e.a / (1.0 + n) * (1.0 + n2 / 4.0 + n4 / 64.0 + n6 / 256.0);
      alpha = {n / 2 - 2 * n2 / 3 + 5 * n3 / 16 + 41 * n4 / 180 - 127 * n5 / 288 +
                   7891 * n6 / 37800,
               13 * n2 / 48 - 3 * n3 / 5 + 557 * n4 / 1440 + 281 * n5 / 630 -
                   1983433 * n6 / 1935360,
               61 * n3 / 240 - 103 * n4 / 140 + 15061 * n5 / 26880 + 167603 * n6 / 181440,
               49561 * n4 / 161280 - 179 * n5 / 168 + 6601661 * n6 / 7257600,
               34729 * n5 / 80640 - 3418889 * n6 / 1995840,
               212378941 * n6 / 319334400};
      beta = {n / 2 - 2 * n2 / 3 + 37 * n3 / 96 - n4 / 360 - 81 * n5 / 512 +
                  96199 * n6 / 604800,
              n2 / 48 + n3 / 15 - 437 * n4 / 1440 + 46 * n5 / 105 - 1118711 * n6 / 3870720,
              17 * n3 / 480 - 37 * n4 / 840 - 209 * n5 / 4480 + 5569 * n6 / 90720,
              4397 * n4 / 161280 - 11 * n5 / 504 - 830251 * n6 / 7257600,
              4583 * n5 / 161280 - 108847 * n6 / 3991680,
              20648693 * n6 / 638668800};
    }
  };

  // τ' (tan of conformal latitude) from τ (tan of geodetic latitude).
  static double taupFromTau(double tau, double e2) {
    const double e = std::sqrt(e2);
    const double tau1 = std::hypot(1.0, tau);
    const double sig = std::sinh(e * std::atanh(e * tau / tau1));
    return tau * std::hypot(1.0, sig) - sig * tau1;
  }

  // Newton iteration for τ given τ' (Karney eqs. 19-21).
  static double tauFromTaup(double taup, double e2) {
    double tau = taup / (1.0 - e2);  // good first guess
    for (int i = 0; i < 12; ++i) {
      const double taupi = taupFromTau(tau, e2);
      const double dtau = (taup - taupi) / std::hypot(1.0, taupi) *
                          (1.0 + (1.0 - e2) * tau * tau) /
                          ((1.0 - e2) * std::hypot(1.0, tau));
      tau += dtau;
      if (std::fabs(dtau) < 1e-14 * std::fmax(1.0, std::fabs(tau))) break;
    }
    return tau;
  }

  GeoidFunction geoid_;
};

} // namespace x3d::runtime::geo

#endif // X3D_RUNTIME_MATH_GEO_BUILTIN_PROJECTION_HPP
