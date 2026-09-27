// Optional PROJ implementation of the §25.2.3 GeoProjection seam (ADR-0053).
#ifndef X3D_RUNTIME_IO_PROJ_GEO_PROJECTION_HPP
#define X3D_RUNTIME_IO_PROJ_GEO_PROJECTION_HPP

#include "GeoProjection.hpp"

#include <memory>
#include <string>

namespace x3d::runtime::io::proj {

class ProjGeoProjection final : public geo::GeoProjection {
public:
  explicit ProjGeoProjection(std::string geoidGridPath = {});
  ~ProjGeoProjection() override;
  ProjGeoProjection(const ProjGeoProjection &) = delete;
  ProjGeoProjection &operator=(const ProjGeoProjection &) = delete;

  bool geodeticToGeocentric(const geo::Ellipsoid &e, double lat, double lon,
                            double h, geo::SFVec3d &gc) const override;
  bool geocentricToGeodetic(const geo::Ellipsoid &e, const geo::SFVec3d &gc,
                            double &lat, double &lon, double &h) const override;
  bool utmToGeodetic(const geo::Ellipsoid &e, int zone, bool south,
                     double easting, double northing, double &lat,
                     double &lon) const override;
  bool geodeticToUtm(const geo::Ellipsoid &e, int zone, bool south, double lat,
                     double lon, double &easting,
                     double &northing) const override;
  bool geoidUndulation(double lat, double lon, double &n) const override;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace x3d::runtime::io::proj

#endif
