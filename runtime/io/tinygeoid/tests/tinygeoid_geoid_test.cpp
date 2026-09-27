// tinygeoid_geoid_test.cpp — the tinygeoid geoid adapter for the built-in
// GeoProjection backend (ADR-0053, GEO-GEOID-DEFAULT).
#include "GeoFrame.hpp"
#include "TinygeoidGeoid.hpp"

#include "doctest/doctest.h"

#include <cmath>
#include <filesystem>
#include <limits>
#include <memory>
#include <vector>

using namespace x3d::runtime;
using x3d::core::SFVec3d;

namespace {
// A 3x3 grid over lat/lon [-1, 1] degrees with N = 10*lat + lon, which
// bilinear interpolation reproduces exactly between samples.
::tinygeoid::GeoidGrid planeGrid(float noDataAt = 0, bool withNoData = false) {
  ::tinygeoid::GeoidMetadata meta;
  meta.rows = 3;
  meta.cols = 3;
  meta.origin_lat_deg = -1.0;
  meta.origin_lon_deg = -1.0;
  meta.delta_lat_deg = 1.0;
  meta.delta_lon_deg = 1.0;
  meta.no_data_value = -9999.0f;
  std::vector<float> samples;
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c)
      samples.push_back(10.0f * static_cast<float>(r - 1) + static_cast<float>(c - 1));
  if (withNoData) samples[static_cast<std::size_t>(noDataAt)] = -9999.0f;
  return ::tinygeoid::GeoidGrid(meta, samples);
}
} // namespace

TEST_CASE("geoid: tinygeoid supplies the WGS84 geoid to the built-in backend") {
  auto grid = std::make_shared<const ::tinygeoid::GeoidGrid>(planeGrid());
  const geo::BuiltinGeoProjection proj(io::geoid::makeGeoidFunction(grid));

  double n = 0;
  REQUIRE(proj.geoidUndulation(0.5 * geo::kDegToRad, 0.25 * geo::kDegToRad, n));
  CHECK(n == doctest::Approx(5.25));

  // §25.2.3: with "WGS84", an elevation of 100 above the geoid is 100 + N
  // above the ellipsoid; without the option the height is ellipsoidal.
  double lat, lon, h;
  REQUIRE(geo::toGeodetic(geo::parseGeoSystem({"GD", "WGS84"}), SFVec3d{0.5, 0.25, 100}, lat, lon, h, proj));
  CHECK(h == doctest::Approx(105.25));
  REQUIRE(geo::toGeodetic(geo::parseGeoSystem({"GD"}), SFVec3d{0.5, 0.25, 100}, lat, lon, h, proj));
  CHECK(h == doctest::Approx(100.0));

  // Round trip through geocentric keeps the geoid-relative height.
  const geo::GeoSystem sys = geo::parseGeoSystem({"GD", "WGS84"});
  SFVec3d gc, back;
  REQUIRE(geo::toGeocentric(sys, SFVec3d{0.5, 0.25, 100}, gc, proj));
  REQUIRE(geo::fromGeocentric(sys, gc, back, proj));
  CHECK(back.z == doctest::Approx(100.0).epsilon(1e-9));
}

TEST_CASE("geoid: a .tng file loads, and failures fall back to ellipsoidal heights") {
  const auto path = std::filesystem::temp_directory_path() / "x3d_tinygeoid_test.tng";
  const ::tinygeoid::GeoidGrid grid = planeGrid();
  std::vector<float> samples;
  for (std::size_t r = 0; r < 3; ++r)
    for (std::size_t c = 0; c < 3; ++c) samples.push_back(grid.value_unchecked(r, c));
  ::tinygeoid::save_geoid_grid(path.string(), grid.metadata(), samples, /*include_checksum=*/true);
  auto loaded = io::geoid::makeBuiltinWithGeoid(path.string());
  REQUIRE(loaded);
  double n = 0;
  REQUIRE(loaded->geoidUndulation(-0.5 * geo::kDegToRad, 0.0, n));
  CHECK(n == doctest::Approx(-5.0));
  std::filesystem::remove(path);

  CHECK_FALSE(io::geoid::makeBuiltinWithGeoid("/nonexistent/grid.tng"));

  // A no-data sample under the query position: no geoid there, not a throw.
  auto holes = std::make_shared<const ::tinygeoid::GeoidGrid>(planeGrid(4, true));  // centre sample
  const geo::BuiltinGeoProjection proj(io::geoid::makeGeoidFunction(holes));
  CHECK_FALSE(proj.geoidUndulation(0.1 * geo::kDegToRad, 0.1 * geo::kDegToRad, n));
  double lat, lon, h;
  REQUIRE(geo::toGeodetic(geo::parseGeoSystem({"GD", "WGS84"}), SFVec3d{0.1, 0.1, 100}, lat, lon, h, proj));
  CHECK(h == doctest::Approx(100.0));
}
