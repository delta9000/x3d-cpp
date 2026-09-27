// §25.2.3 GD/UTM ellipsoid conversions and optional WGS84 geoid, ADR-0053.
#include "GeoFrame.hpp"
#include "ProjGeoProjection.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>

using namespace x3d::runtime::geo;
using x3d::runtime::io::proj::ProjGeoProjection;

namespace {
int failures = 0;
double worstGc = 0, worstUtm = 0, worstLat = 0, worstLon = 0, worstHeight = 0;

void check(bool good, const std::string &where) {
  if (!good) {
    std::cerr << "FAIL: " << where << '\n';
    ++failures;
  }
}

double angularDifference(double a, double b) {
  return std::fabs(std::remainder((a - b) * kRadToDeg, 360.0));
}

void compareGc(const BuiltinGeoProjection &builtin,
               const ProjGeoProjection &proj, const Ellipsoid &ellipsoid,
               const std::string &code, double latDeg, double lonDeg,
               double h) {
  const double lat = latDeg * kDegToRad, lon = lonDeg * kDegToRad;
  SFVec3d a, b;
  const std::string where = code + " GD " + std::to_string(latDeg) + "," +
                            std::to_string(lonDeg) + "," + std::to_string(h);
  if (!builtin.geodeticToGeocentric(ellipsoid, lat, lon, h, a) ||
      !proj.geodeticToGeocentric(ellipsoid, lat, lon, h, b)) {
    check(false, where + " forward");
    return;
  }
  const double gc = std::max(
      {std::fabs(a.x - b.x), std::fabs(a.y - b.y), std::fabs(a.z - b.z)});
  worstGc = std::max(worstGc, gc);
  check(gc <= 1e-6, where + " geocentric");
  double alat, alon, ah, blat, blon, bh;
  if (!builtin.geocentricToGeodetic(ellipsoid, a, alat, alon, ah) ||
      !proj.geocentricToGeodetic(ellipsoid, a, blat, blon, bh)) {
    check(false, where + " inverse");
    return;
  }
  const double latDiff = std::fabs(alat - blat) * kRadToDeg;
  const double lonDiff = angularDifference(alon, blon);
  const double hDiff = std::fabs(ah - bh);
  worstLat = std::max(worstLat, latDiff);
  worstLon = std::max(worstLon, lonDiff);
  worstHeight = std::max(worstHeight, hDiff);
  // PROJ's inverse +proj=cart is not iterated: at 20 km altitude its height is
  // ~3 µm off (PROJ 9.8: 0 -70 20000 -> fwd -> inv gives 20000.0000026), while
  // the built-in inverse converges to ~1e-9 m. 1e-5 m bounds PROJ's residual.
  check(latDiff <= 1e-9 && lonDiff <= 1e-9 && hDiff <= 1e-5,
        where + " inverse geodetic");
}

void compareUtm(const BuiltinGeoProjection &builtin,
                const ProjGeoProjection &proj, const Ellipsoid &ellipsoid,
                const std::string &code, int zone, bool south, double latDeg,
                double deltaLonDeg) {
  const double lat = latDeg * kDegToRad;
  const double lon = (zone * 6.0 - 183.0 + deltaLonDeg) * kDegToRad;
  const std::string where = code + " UTM " + std::to_string(zone) +
                            (south ? "S" : "N") + " " + std::to_string(latDeg) +
                            "," + std::to_string(deltaLonDeg);
  double ae, an, be, bn;
  if (!builtin.geodeticToUtm(ellipsoid, zone, south, lat, lon, ae, an) ||
      !proj.geodeticToUtm(ellipsoid, zone, south, lat, lon, be, bn)) {
    check(false, where + " forward");
    return;
  }
  const double diff = std::max(std::fabs(ae - be), std::fabs(an - bn));
  worstUtm = std::max(worstUtm, diff);
  check(diff <= 1e-4, where + " easting/northing");
  double alat, alon, blat, blon;
  if (!builtin.utmToGeodetic(ellipsoid, zone, south, ae, an, alat, alon) ||
      !proj.utmToGeodetic(ellipsoid, zone, south, ae, an, blat, blon)) {
    check(false, where + " inverse");
    return;
  }
  const double latDiff = std::fabs(alat - blat) * kRadToDeg;
  const double lonDiff = angularDifference(alon, blon);
  worstLat = std::max(worstLat, latDiff);
  worstLon = std::max(worstLon, lonDiff);
  check(latDiff <= 1e-9 && lonDiff <= 1e-9, where + " inverse geodetic");
}
} // namespace

int main() {
  const BuiltinGeoProjection builtin;
  const ProjGeoProjection proj;
  const std::string codes[] = {"WE", "CC", "AA", "IN", "KA", "BR"};
  const double latitudes[] = {-89.999, -89.991, -70, -45,    -0.1,  0,
                              0.1,     45,      70,  89.991, 89.999};
  const double longitudes[] = {-179.999, -179.991, -123, -71.5,   -3,
                               0,        45,       150,  179.991, 179.999};
  const double heights[] = {-500, 0, 100, 20000};
  for (const std::string &code : codes) {
    Ellipsoid e;
    check(ellipsoidByCode(code, e), code + " table");
    for (double lat : latitudes)
      for (double lon : longitudes)
        for (double h : heights)
          compareGc(builtin, proj, e, code, lat, lon, h);
    for (int zone : {1, 10, 31, 56, 60})
      for (double lat : {-75.0, -45.0, -0.1, 0.1, 45.0, 75.0})
        for (double dLon : {-3.5, -3.0, 0.0, 3.0, 3.5})
          compareUtm(builtin, proj, e, code, zone, lat < 0, lat, dLon);
  }
  Ellipsoid we;
  ellipsoidByCode("WE", we);
  double lat, lon, h, east, north, n = 0;
  check(!proj.geocentricToGeodetic(we, SFVec3d{0, 0, 0}, lat, lon, h),
        "earth centre");
  check(!proj.geodeticToUtm(we, 0, false, 0, 0, east, north), "invalid zone");
  check(!proj.geoidUndulation(0, 0, n), "unconfigured geoid");
  // A 2x2 test grid isolates §25.2.3's WGS84 height rule from downloaded data.
  ProjGeoProjection sampleGrid(X3D_PROJ_TEST_GRID);
  check(sampleGrid.geoidUndulation(40 * kDegToRad, -75 * kDegToRad, n),
        "sample geoid lookup");
  check(std::fabs(n - 25.0) <= 1e-9, "sample geoid interpolation and sign");
  check(!sampleGrid.geoidUndulation(0, 0, n), "outside geoid coverage");
  SFVec3d fromGeoid, fromEllipsoid;
  check(toGeocentric(parseGeoSystem({"GD", "WE", "WGS84"}), {40, -75, 100},
                     fromGeoid, sampleGrid),
        "sample geoid GeoFrame conversion");
  check(toGeocentric(parseGeoSystem({"GD", "WE"}), {40, -75, 125},
                     fromEllipsoid, sampleGrid),
        "sample ellipsoid GeoFrame conversion");
  check(std::max({std::fabs(fromGeoid.x - fromEllipsoid.x),
                  std::fabs(fromGeoid.y - fromEllipsoid.y),
                  std::fabs(fromGeoid.z - fromEllipsoid.z)}) <= 1e-9,
        "sample geoid height application");
  const char *gridEnv = std::getenv("X3D_GEOID_GRID");
  const std::string grid = gridEnv ? gridEnv : "";
  if (!grid.empty() && std::filesystem::exists(grid)) {
    ProjGeoProjection withGrid(grid);
    check(withGrid.geoidUndulation(40 * kDegToRad, -75 * kDegToRad, n),
          "geoid grid sample");
    if (withGrid.geoidUndulation(40 * kDegToRad, -75 * kDegToRad, n)) {
      // §25.2.3 WGS84: GeoFrame adds N to geoid-relative authored height.
      SFVec3d fromGeoid, fromEllipsoid;
      check(toGeocentric(parseGeoSystem({"GD", "WE", "WGS84"}), {40, -75, 100},
                         fromGeoid, withGrid),
            "geoid GeoFrame conversion");
      check(toGeocentric(parseGeoSystem({"GD", "WE"}), {40, -75, 100 + n},
                         fromEllipsoid, withGrid),
            "ellipsoid GeoFrame conversion");
      const double diff = std::max({std::fabs(fromGeoid.x - fromEllipsoid.x),
                                    std::fabs(fromGeoid.y - fromEllipsoid.y),
                                    std::fabs(fromGeoid.z - fromEllipsoid.z)});
      check(diff <= 1e-9, "geoid height sign");
      std::cout << "Geoid N at 40,-75: " << n << " m\n";
    }
  } else {
    std::cout
        << "Geoid grid case skipped (set X3D_GEOID_GRID to a GTX/GTG path)\n";
  }
  std::cout << std::setprecision(12) << "Worst differences: geocentric "
            << worstGc << " m; UTM " << worstUtm << " m; inverse lat "
            << worstLat << " deg, lon " << worstLon << " deg, height "
            << worstHeight << " m\n";
  return failures ? 1 : 0;
}
