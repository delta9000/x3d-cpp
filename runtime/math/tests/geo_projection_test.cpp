// geo_projection_test.cpp — the first-party GeoProjection backend and the
// SDK-side frames (ADR-0053) against reference values generated with PROJ 9.8
// (`cct +proj=cart`, `cs2cs ... +proj=utm`, `+proj=webmerc`), printed to 6
// decimals. The PROJ backend's swap-test re-checks the same numbers live.
#include "GeoFrame.hpp"

#include "doctest/doctest.h"

#include <cmath>
#include <string>
#include <vector>

using namespace x3d::runtime::geo;
using x3d::core::SFVec3d;
using x3d::core::SFVec3f;
using x3d::runtime::Mat4;

namespace {
const BuiltinGeoProjection kBuiltin;

struct GcCase { double lon, lat, h, x, y, z; };

// lon lat h -> X Y Z on four ellipsoids (PROJ: cct -d 6 +proj=cart <ellipsoid>).
void checkGeocentric(const Ellipsoid &e, const std::vector<GcCase> &cases) {
  for (const GcCase &c : cases) {
    SFVec3d gc;
    REQUIRE(kBuiltin.geodeticToGeocentric(e, c.lat * kDegToRad, c.lon * kDegToRad, c.h, gc));
    CHECK(std::fabs(gc.x - c.x) < 1e-5);
    CHECK(std::fabs(gc.y - c.y) < 1e-5);
    CHECK(std::fabs(gc.z - c.z) < 1e-5);
    // And back: geodetic within 1e-10 degree / 1e-6 m (longitude undefined at a pole).
    double lat, lon, h;
    REQUIRE(kBuiltin.geocentricToGeodetic(e, SFVec3d{c.x, c.y, c.z}, lat, lon, h));
    CHECK(std::fabs(lat * kRadToDeg - c.lat) < 1e-8);
    // The reference X/Y carry 6 decimals (±5e-7 m); near the polar axis that
    // rounding alone moves longitude by ~5e-7/p rad, so scale the tolerance.
    const double p = std::hypot(c.x, c.y);
    if (p > 1.0) CHECK(std::fabs(lon * kRadToDeg - c.lon) < 1e-8 + 2e-6 / p * kRadToDeg);
    CHECK(std::fabs(h - c.h) < 1e-4);
  }
}
} // namespace

TEST_CASE("geo: geodetic <-> geocentric matches PROJ on WGS84, Clarke 1866, Airy, International") {
  Ellipsoid we, cc, aa, in;
  REQUIRE(ellipsoidByCode("WE", we));
  REQUIRE(ellipsoidByCode("CC", cc));
  REQUIRE(ellipsoidByCode("AA", aa));
  REQUIRE(ellipsoidByCode("IN", in));
  checkGeocentric(we, {{-122.37896, 37.62131, 10.4, -2708753.465848, -4271779.008256, 3872243.452871},
                       {0, 0, 0, 6378137.0, 0, 0},
                       {0, 90, 0, 0, 0, 6356752.314245},
                       {151.2093, -33.8688, 58, -4646093.477288, 2553229.535817, -3534404.710910},
                       {-71.5, 41.5, 100, 1518002.282459, -4536830.594993, 4204238.520359},
                       {-0.0015, 51.4778, 45, 3980609.237098, -104.212106, 4966859.728504},
                       {179.9, -89.99, -50, -1116.929362, 1.949411, -6356702.216775},
                       {45, 0, 20000, 4524166.059661, 4524166.059661, 0}});
  checkGeocentric(cc, {{-122.37896, 37.62131, 10.4, -2708820.521816, -4271884.757370, 3872049.743714},
                       {-71.5, 41.5, 100, 1518043.626150, -4536954.158261, 4204038.634579},
                       {0, 90, 0, 0, 0, 6356583.799999}});
  checkGeocentric(aa, {{-0.0015, 51.4778, 45, 3980222.092419, -104.201971, 4966495.858920},
                       {151.2093, -33.8688, 58, -4645658.410539, 2552990.447801, -3534158.563612}});
  checkGeocentric(in, {{179.9, -89.99, -50, -1116.989224, 1.949516, -6356861.848652},
                       {45, 0, 20000, 4524343.543463, 4524343.543463, 0}});
}

TEST_CASE("geo: UTM forward matches PROJ and round-trips (north, south, zone edges, other ellipsoid)") {
  Ellipsoid we, in;
  REQUIRE(ellipsoidByCode("WE", we));
  REQUIRE(ellipsoidByCode("IN", in));
  struct U { Ellipsoid e; int zone; bool south; double lon, lat, east, north; };
  const std::vector<U> cases = {
      {we, 10, false, -122.37896, 37.62131, 554805.119002, 4163981.250122},
      {we, 10, false, -120.0, 45.0, 736446.026101, 4987329.504699},   // 3 degrees east of the CM
      {we, 10, false, -125.9, 45.0, 271435.514118, 4987042.306615},   // ~3 degrees west
      {we, 56, true, 151.2093, -33.8688, 334368.633648, 6250948.345385},
      {we, 56, true, 150.1, -0.5, 177176.037447, 9944663.622434},
      {in, 31, false, 2.3522, 48.8566, 452480.280088, 5411824.306950},
      {we, 19, false, -71.5, 41.5, 291334.882330, 4597281.734700},
      {we, 19, false, -69.1, 70.2, 496219.477725, 7788179.971678},
  };
  for (const U &u : cases) {
    double e, n;
    REQUIRE(kBuiltin.geodeticToUtm(u.e, u.zone, u.south, u.lat * kDegToRad, u.lon * kDegToRad, e, n));
    CHECK(std::fabs(e - u.east) < 1e-4);
    CHECK(std::fabs(n - u.north) < 1e-4);
    double lat, lon;
    REQUIRE(kBuiltin.utmToGeodetic(u.e, u.zone, u.south, u.east, u.north, lat, lon));
    CHECK(std::fabs(lat * kRadToDeg - u.lat) < 1e-9);
    CHECK(std::fabs(lon * kRadToDeg - u.lon) < 1e-9);
  }
  double lat, lon;
  CHECK_FALSE(kBuiltin.utmToGeodetic(we, 0, false, 500000, 0, lat, lon));  // no zone
}

TEST_CASE("geo: geoSystem parsing (§25.2.3)") {
  const GeoSystem d = parseGeoSystem({});
  CHECK(d.frame == GeoSystem::Frame::GD);
  CHECK(d.ellipsoid.a == 6378137.0);
  const GeoSystem u = parseGeoSystem({"UTM", "Z10", "S", "IN", "easting_first"});
  CHECK(u.frame == GeoSystem::Frame::UTM);
  CHECK(u.zone == 10);
  CHECK(u.south);
  CHECK(u.eastingFirst);
  CHECK(u.ellipsoid.a == 6378388.0);
  const GeoSystem g = parseGeoSystem({"GD", "CC", "WGS84", "longitude_first"});
  CHECK(g.geoidHeights);
  CHECK(g.longitudeFirst);
  CHECK(g.ellipsoid.invF == 294.9786982);
  CHECK(parseGeoSystem({"UTM", "Z17", "N"}).south == false);  // "N": no-op northern alias
  CHECK(parseGeoSystem({"GCC"}).frame == GeoSystem::Frame::GC);
  CHECK(parseGeoSystem({"GC", "CC"}).ellipsoid.a == 6378137.0);  // GC is always WGS84
}

TEST_CASE("geo: authored coordinates honour ordering, UTM and Web Mercator") {
  SFVec3d a, b;
  REQUIRE(toGeocentric(parseGeoSystem({"GD"}), SFVec3d{41.5, -71.5, 100}, a, kBuiltin));
  REQUIRE(toGeocentric(parseGeoSystem({"GD", "longitude_first"}), SFVec3d{-71.5, 41.5, 100}, b, kBuiltin));
  CHECK(std::fabs(a.x - 1518002.282459) < 1e-5);
  CHECK(std::fabs(a.x - b.x) + std::fabs(a.y - b.y) + std::fabs(a.z - b.z) < 1e-9);

  SFVec3d nf, ef;  // UTM default northing_first vs easting_first
  REQUIRE(toGeocentric(parseGeoSystem({"UTM", "Z19"}), SFVec3d{4597281.734700, 291334.882330, 100}, nf, kBuiltin));
  REQUIRE(toGeocentric(parseGeoSystem({"UTM", "Z19", "easting_first"}), SFVec3d{291334.882330, 4597281.734700, 100}, ef, kBuiltin));
  CHECK(std::fabs(nf.x - a.x) < 1e-3);
  CHECK(std::fabs(nf.y - a.y) < 1e-3);
  CHECK(std::fabs(nf.z - a.z) < 1e-3);
  CHECK(std::fabs(nf.x - ef.x) + std::fabs(nf.y - ef.y) + std::fabs(nf.z - ef.z) < 1e-9);

  // Web Mercator (PROJ +proj=webmerc): (-71.5, 41.5) -> (-7959343.591719, 5086373.649287).
  SFVec3d wm;
  REQUIRE(fromGeocentric(parseGeoSystem({"WM"}), a, wm, kBuiltin));
  CHECK(std::fabs(wm.x - -7959343.591719) < 1e-4);
  CHECK(std::fabs(wm.y - 5086373.649287) < 1e-4);
  CHECK(std::fabs(wm.z - 100.0) < 1e-6);
  SFVec3d back;
  REQUIRE(toGeocentric(parseGeoSystem({"WM"}), wm, back, kBuiltin));
  CHECK(std::fabs(back.x - a.x) + std::fabs(back.y - a.y) + std::fabs(back.z - a.z) < 1e-6);

  SFVec3d gd;  // geocentric -> GD in the authored order
  REQUIRE(fromGeocentric(parseGeoSystem({"GD", "longitude_first"}), a, gd, kBuiltin));
  CHECK(std::fabs(gd.x - -71.5) < 1e-10);
  CHECK(std::fabs(gd.y - 41.5) < 1e-10);
}

TEST_CASE("geo: the WGS84 geoid option adds the undulation when a model is present") {
  const BuiltinGeoProjection withGeoid([](double, double, double &n) { n = -30.0; return true; });
  const GeoSystem sys = parseGeoSystem({"GD", "WGS84"});
  double lat, lon, h;
  REQUIRE(toGeodetic(sys, SFVec3d{41.5, -71.5, 100}, lat, lon, h, withGeoid));
  CHECK(h == doctest::Approx(70.0));  // 100 m above the geoid, geoid 30 m below the ellipsoid
  REQUIRE(toGeodetic(sys, SFVec3d{41.5, -71.5, 100}, lat, lon, h, kBuiltin));
  CHECK(h == doctest::Approx(100.0));  // no model: ellipsoidal heights
  SFVec3d gc, c;
  REQUIRE(toGeocentric(sys, SFVec3d{41.5, -71.5, 100}, gc, withGeoid));
  REQUIRE(fromGeocentric(sys, gc, c, withGeoid));
  CHECK(c.z == doctest::Approx(100.0));
}

TEST_CASE("geo: GeoOrigin frame and tangent frames (§25.3.6, §25.3.3)") {
  const GeoSystem gd = parseGeoSystem({"GD"});
  OriginFrame origin;
  REQUIRE(makeOriginFrame(gd, SFVec3d{41.5, -71.5, 0}, /*rotateYUp=*/true, origin, kBuiltin));
  SFVec3d gc;
  REQUIRE(toGeocentric(gd, SFVec3d{41.5, -71.5, 0}, gc, kBuiltin));
  const SFVec3f o = origin.toWorld(gc);
  CHECK(std::fabs(o.x) + std::fabs(o.y) + std::fabs(o.z) < 1e-6f);
  REQUIRE(toGeocentric(gd, SFVec3d{41.5, -71.5, 100}, gc, kBuiltin));
  const SFVec3f up = origin.toWorld(gc);  // 100 m straight up -> +Y
  CHECK(std::fabs(up.x) < 1e-4f);
  CHECK(up.y == doctest::Approx(100.0f).epsilon(1e-6));
  CHECK(std::fabs(up.z) < 1e-4f);
  REQUIRE(toGeocentric(gd, SFVec3d{41.501, -71.5, 0}, gc, kBuiltin));
  const SFVec3f north = origin.toWorld(gc);  // ~111 m north -> −Z, slightly below the tangent plane
  CHECK(north.z < -110.0f);
  CHECK(std::fabs(north.x) < 1e-3f);
  CHECK(north.y < 0.0f);
  REQUIRE(toGeocentric(gd, SFVec3d{41.5, -71.499, 0}, gc, kBuiltin));
  CHECK(origin.toWorld(gc).x > 80.0f);  // east -> +X

  // A tangent frame at the origin is the identity rotation there.
  Mat4 m;
  REQUIRE(tangentFrame(gd, SFVec3d{41.5, -71.5, 0}, origin, m, kBuiltin));
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      CHECK(std::fabs(m.m[i * 4 + j] - (i == j ? 1.0f : 0.0f)) < 1e-6f);

  // Without rotateYUp the frame is the geocentric orientation of the local axes.
  OriginFrame plain;
  REQUIRE(makeOriginFrame(gd, SFVec3d{0, 0, 0}, false, plain, kBuiltin));
  REQUIRE(tangentFrame(gd, SFVec3d{0, 0, 0}, plain, m, kBuiltin));
  CHECK(m.m[4] == doctest::Approx(1.0f));   // up (column 1) = +X geocentric at lat 0, lon 0
  CHECK(m.m[1] == doctest::Approx(1.0f));   // east (column 0) = +Y geocentric
  CHECK(m.m[10] == doctest::Approx(-1.0f)); // south (column 2) = −Z geocentric
}

TEST_CASE("geo: invalid input is rejected, never NaN") {
  double lat, lon, h;
  CHECK_FALSE(kBuiltin.geocentricToGeodetic(Ellipsoid{}, SFVec3d{0, 0, 0}, lat, lon, h));
  SFVec3d gc;
  CHECK_FALSE(kBuiltin.geodeticToGeocentric(Ellipsoid{}, NAN, 0, 0, gc));
  CHECK_FALSE(toGeocentric(parseGeoSystem({"UTM"}), SFVec3d{0, 500000, 0}, gc, kBuiltin));  // no zone
}

#include "GeoNodes.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"

#include <any>
#include <memory>

namespace {
void setF(const std::shared_ptr<x3d::nodes::X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
} // namespace

TEST_CASE("geo: node glue converts through the node's geoSystem and GeoOrigin") {
  auto origin = x3d::nodes::createX3DNode("GeoOrigin");
  setF(origin, "geoCoords", std::any(SFVec3d{41.5, -71.5, 0}));
  setF(origin, "rotateYUp", std::any(true));
  auto coord = x3d::nodes::createX3DNode("GeoCoordinate");
  setF(coord, "geoOrigin", std::any(std::shared_ptr<x3d::nodes::X3DNode>(origin)));
  setF(coord, "geoSystem", std::any(std::vector<std::string>{"GD", "longitude_first"}));

  SFVec3f w;
  REQUIRE(toWorld(*coord, SFVec3d{-71.5, 41.5, 100}, w, builtinProjection()));  // longitude first
  CHECK(std::fabs(w.x) < 1e-4f);
  CHECK(w.y == doctest::Approx(100.0f).epsilon(1e-6));
  CHECK(std::fabs(w.z) < 1e-4f);

  SFVec3d back;
  REQUIRE(fromWorld(*coord, SFVec3f{30, 5, -40}, back, builtinProjection()));
  SFVec3f again;
  REQUIRE(toWorld(*coord, back, again, builtinProjection()));
  CHECK(std::fabs(again.x - 30) + std::fabs(again.y - 5) + std::fabs(again.z + 40) < 1e-3f);

  const auto pts = toWorld(*coord, std::vector<SFVec3d>{{-71.5, 41.5, 0}, {-71.5, 41.5, 10}}, nullptr, builtinProjection());
  REQUIRE(pts.size() == 2);
  CHECK(pts[1].y == doctest::Approx(10.0f).epsilon(1e-5));

  auto bare = x3d::nodes::createX3DNode("GeoCoordinate");  // no GeoOrigin: geocentric world
  REQUIRE(toWorld(*bare, SFVec3d{0, 0, 0}, w, builtinProjection()));
  CHECK(w.x == doctest::Approx(6378137.0f));
}
