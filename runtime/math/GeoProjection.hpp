// GeoProjection.hpp — the geospatial projection seam (ISO/IEC 19775-1 §25.2,
// ADR-0053).
//
// X3D Geospatial nodes author coordinates in a spatial reference frame named
// by their `geoSystem` field (§25.2.3): geodetic (GD, degrees + elevation),
// UTM (easting/northing + elevation), geocentric (GC, metres) or Web Mercator
// (WM). Internally everything becomes earth-fixed geocentric metres; nodes
// with a GeoOrigin are then expressed relative to it (GeoFrame.hpp).
//
// This header holds the frame description (parsed from `geoSystem`), the §25
// ellipsoid table, and the backend interface. A backend implements only the
// ellipsoid mathematics (geodetic <-> geocentric, geodetic <-> UTM) and an
// optional geoid; axis ordering, units, Web Mercator and the local frames are
// SDK-side and identical for every backend. The default backend is the
// first-party BuiltinGeoProjection (GeoBuiltinProjection.hpp); a PROJ-backed
// one is optional (runtime/io/proj/) and proves the seam in a swap-test.
//
// No datum transformation is performed: a GD coordinate on (say) Clarke 1866
// is converted with that ellipsoid's geometry into the same earth-fixed frame
// (§25.2.3 names ellipsoids only).
#ifndef X3D_RUNTIME_MATH_GEO_PROJECTION_HPP
#define X3D_RUNTIME_MATH_GEO_PROJECTION_HPP

#include "x3d/core/X3Dtypes.hpp"

#include <cstdlib>
#include <string>
#include <vector>

namespace x3d::runtime::geo {

using x3d::core::SFVec3d;

/// A reference ellipsoid: semi-major axis (metres) and inverse flattening.
struct Ellipsoid {
  double a = 6378137.0;
  double invF = 298.257223563;

  double f() const { return 1.0 / invF; }
  double e2() const { const double fl = f(); return fl * (2.0 - fl); }  // first eccentricity²
  double b() const { return a * (1.0 - f()); }
};

/// §25.2.3 Table 25.3 — the supported earth ellipsoids. Returns false for an
/// unknown code (the caller keeps WGS84 and the validator reports the token).
inline bool ellipsoidByCode(const std::string &code, Ellipsoid &out) {
  struct Row { const char *code; double a, invF; };
  static const Row kRows[] = {
      {"AA", 6377563.396, 299.3249646},   {"AM", 6377340.189, 299.3249646},
      {"AN", 6378160.0, 298.25},          {"BN", 6377483.865, 299.1528128},
      {"BR", 6377397.155, 299.1528128},   {"CC", 6378206.4, 294.9786982},
      {"CD", 6378249.145, 293.465},       {"EA", 6377276.345, 300.8017},
      {"EB", 6377298.556, 300.8017},      {"EC", 6377301.243, 300.8017},
      {"ED", 6377295.664, 300.8017},      {"EE", 6377304.063, 300.8017},
      {"EF", 6377309.613, 300.8017},      {"FA", 6378155.0, 298.3},
      {"HE", 6378200.0, 298.3},           {"HO", 6378270.0, 297.0},
      {"ID", 6378160.0, 298.247},         {"IN", 6378388.0, 297.0},
      {"KA", 6378245.0, 298.3},           {"RF", 6378137.0, 298.257222101},
      {"SA", 6378160.0, 298.25},          {"WD", 6378135.0, 298.26},
      {"WE", 6378137.0, 298.257223563},
  };
  for (const Row &r : kRows)
    if (code == r.code) { out = Ellipsoid{r.a, r.invF}; return true; }
  return false;
}

/// A parsed `geoSystem` (§25.2.3). Parsing is lenient: unknown tokens are
/// ignored here (X3DRangeValidate reports them) and defaults apply.
struct GeoSystem {
  enum class Frame { GD, UTM, GC, WM };
  Frame frame = Frame::GD;
  Ellipsoid ellipsoid{};          // GD/UTM; GC and WM are always WGS84
  bool geoidHeights = false;      // "WGS84": elevations above the WGS84 geoid
  bool longitudeFirst = false;    // GD: "longitude_first"
  bool eastingFirst = false;      // UTM: "easting_first"
  int zone = 0;                   // UTM "Z<n>", 1..60 (0 = missing)
  bool south = false;             // UTM "S" (the "N" alias is a no-op)
};

inline GeoSystem parseGeoSystem(const std::vector<std::string> &tokens) {
  GeoSystem s;
  if (tokens.empty()) return s;  // default [ "GD", "WE" ]
  const std::string &f = tokens.front();
  if (f == "UTM") s.frame = GeoSystem::Frame::UTM;
  else if (f == "GC" || f == "GCC") s.frame = GeoSystem::Frame::GC;
  else if (f == "WM") s.frame = GeoSystem::Frame::WM;
  // "GD", "GDC" and anything unrecognised: geodetic.
  for (std::size_t i = 1; i < tokens.size(); ++i) {
    const std::string &t = tokens[i];
    Ellipsoid e;
    if (t == "WGS84") s.geoidHeights = true;
    else if (t == "latitude_first") s.longitudeFirst = false;
    else if (t == "longitude_first") s.longitudeFirst = true;
    else if (t == "northing_first") s.eastingFirst = false;
    else if (t == "easting_first") s.eastingFirst = true;
    else if (t == "S") s.south = true;
    else if (t.size() > 1 && t[0] == 'Z') s.zone = std::atoi(t.c_str() + 1);
    else if (ellipsoidByCode(t, e)) s.ellipsoid = e;
  }
  if (s.frame == GeoSystem::Frame::GC || s.frame == GeoSystem::Frame::WM)
    s.ellipsoid = Ellipsoid{};  // §25.2.3: both are defined on WGS84
  return s;
}

/// The backend seam: ellipsoid mathematics only. Angles are radians, lengths
/// metres; UTM eastings/northings include the false easting (500 km) and the
/// southern false northing (10 000 km). Every call returns false (and leaves
/// outputs unspecified) when the input cannot be converted.
class GeoProjection {
public:
  virtual ~GeoProjection() = default;

  virtual bool geodeticToGeocentric(const Ellipsoid &e, double lat, double lon, double h,
                                    SFVec3d &gc) const = 0;
  virtual bool geocentricToGeodetic(const Ellipsoid &e, const SFVec3d &gc, double &lat,
                                    double &lon, double &h) const = 0;
  virtual bool utmToGeodetic(const Ellipsoid &e, int zone, bool south, double easting,
                             double northing, double &lat, double &lon) const = 0;
  virtual bool geodeticToUtm(const Ellipsoid &e, int zone, bool south, double lat, double lon,
                             double &easting, double &northing) const = 0;

  /// WGS84 geoid undulation N (metres) at a geodetic position, for the
  /// "WGS84" geoSystem option (h_ellipsoid = h_geoid + N). A backend without a
  /// geoid model returns false; heights are then treated as ellipsoidal.
  virtual bool geoidUndulation(double /*lat*/, double /*lon*/, double & /*n*/) const {
    return false;
  }
};

} // namespace x3d::runtime::geo

#endif // X3D_RUNTIME_MATH_GEO_PROJECTION_HPP
