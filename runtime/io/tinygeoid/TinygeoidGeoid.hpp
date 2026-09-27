// TinygeoidGeoid.hpp — a WGS84 geoid for the built-in GeoProjection backend
// from a tinygeoid `.tng` grid (ADR-0053; vendored tinygeoid, MIT).
//
// The §25.2.3 "WGS84" geoSystem option makes elevations relative to the WGS84
// geoid (mean sea level). The first-party backend has no geoid model of its
// own; this adapter supplies one from an application-provided grid (for
// example EGM2008 at 2.5′, converted with tinygeoid's tng_pack), without the
// optional PROJ backend. Loading reads a file, so this lives under
// runtime/io/ and is not part of the IO-free SDK headers.
//
//   geo::setProjection(io::geoid::makeBuiltinWithGeoid("egm2008_25.tng"));
#ifndef X3D_RUNTIME_IO_TINYGEOID_GEOID_HPP
#define X3D_RUNTIME_IO_TINYGEOID_GEOID_HPP

#include "GeoBuiltinProjection.hpp"
#include "vendor/tinygeoid.hpp"

#include <cmath>
#include <exception>
#include <memory>
#include <string>

namespace x3d::runtime::io::geoid {

/// A geoid function over a loaded grid: N(lat, lon) in metres from radians.
/// Positions the grid cannot answer (no-data samples) report false, so the
/// height stays ellipsoidal there.
inline geo::BuiltinGeoProjection::GeoidFunction
makeGeoidFunction(std::shared_ptr<const ::tinygeoid::GeoidGrid> grid) {
  return [grid = std::move(grid)](double lat, double lon, double &n) {
    if (!grid) return false;
    try {
      n = ::tinygeoid::undulation(*grid, lat * geo::kRadToDeg, lon * geo::kRadToDeg);
      return std::isfinite(n);
    } catch (const std::exception &) {
      return false;
    }
  };
}

/// Load a `.tng` grid and return the built-in backend with it as its geoid.
/// Null if the file cannot be read or parsed.
inline std::shared_ptr<const geo::GeoProjection> makeBuiltinWithGeoid(const std::string &tngPath) {
  try {
    auto grid = std::make_shared<const ::tinygeoid::GeoidGrid>(::tinygeoid::load_geoid_grid(tngPath));
    return std::make_shared<geo::BuiltinGeoProjection>(makeGeoidFunction(std::move(grid)));
  } catch (const std::exception &) {
    return nullptr;
  }
}

} // namespace x3d::runtime::io::geoid

#endif // X3D_RUNTIME_IO_TINYGEOID_GEOID_HPP
