#include "ProjGeoProjection.hpp"

#include <proj.h>

#include <cmath>
#include <iomanip>
#include <map>
#include <mutex>
#include <memory>
#include <sstream>
#include <string>

namespace x3d::runtime::io::proj {
namespace {
using Operation = std::unique_ptr<PJ, decltype(&proj_destroy)>;

std::string ellipsoidArgs(const geo::Ellipsoid &e) {
  std::ostringstream args;
  args << std::setprecision(17) << " +a=" << e.a << " +rf=" << e.invF;
  return args.str();
}

std::string utmDefinition(const geo::Ellipsoid &e, int zone, bool south) {
  return "+proj=utm +zone=" + std::to_string(zone) + (south ? " +south" : "") +
         ellipsoidArgs(e);
}
} // namespace

// One private PROJ context per backend, with parsed operations cached by
// definition: converting a large grid must not re-parse the definition string
// per point. The mutex serialises use of the context and the cache.
struct ProjGeoProjection::Impl {
  explicit Impl(std::string path)
      : geoidGridPath(std::move(path)), context(proj_context_create(), proj_context_destroy) {}
  std::string geoidGridPath;
  std::unique_ptr<PJ_CONTEXT, decltype(&proj_context_destroy)> context;
  std::map<std::string, Operation> operations;
  std::mutex mutex;

  bool transform(const std::string &definition, PJ_DIRECTION direction, PJ_COORD input,
                 PJ_COORD &output) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = operations.find(definition);
    if (it == operations.end()) {
      Operation op(proj_create(context.get(), definition.c_str()), proj_destroy);
      if (!op) return false;
      it = operations.emplace(definition, std::move(op)).first;
    }
    PJ *op = it->second.get();
    proj_errno_reset(op);
    output = proj_trans(op, direction, input);
    return proj_errno(op) == 0 && std::isfinite(output.xyzt.x) &&
           std::isfinite(output.xyzt.y) && std::isfinite(output.xyzt.z);
  }
};

ProjGeoProjection::ProjGeoProjection(std::string geoidGridPath)
    : impl_(std::make_unique<Impl>(std::move(geoidGridPath))) {}
ProjGeoProjection::~ProjGeoProjection() = default;

bool ProjGeoProjection::geodeticToGeocentric(const geo::Ellipsoid &e,
                                             double lat, double lon, double h,
                                             geo::SFVec3d &gc) const {
  if (!std::isfinite(lat) || !std::isfinite(lon) || !std::isfinite(h))
    return false;
  PJ_COORD result;
  if (!impl_->transform("+proj=cart" + ellipsoidArgs(e), PJ_FWD,
                 proj_coord(lon, lat, h, 0), result))
    return false;
  gc = {result.xyz.x, result.xyz.y, result.xyz.z};
  return true;
}

bool ProjGeoProjection::geocentricToGeodetic(const geo::Ellipsoid &e,
                                             const geo::SFVec3d &gc,
                                             double &lat, double &lon,
                                             double &h) const {
  if (!std::isfinite(gc.x) || !std::isfinite(gc.y) || !std::isfinite(gc.z) ||
      (std::hypot(gc.x, gc.y) < 1e-9 && std::fabs(gc.z) < 1e-9))
    return false;
  const std::string cart = "+proj=cart" + ellipsoidArgs(e);
  PJ_COORD result;
  if (!impl_->transform(cart, PJ_INV, proj_coord(gc.x, gc.y, gc.z, 0), result))
    return false;
  lon = result.lpzt.lam;
  lat = result.lpzt.phi;
  h = result.lpzt.z;
  return true;
}

bool ProjGeoProjection::utmToGeodetic(const geo::Ellipsoid &e, int zone,
                                      bool south, double easting,
                                      double northing, double &lat,
                                      double &lon) const {
  if (zone < 1 || zone > 60 || !std::isfinite(easting) ||
      !std::isfinite(northing))
    return false;
  PJ_COORD result;
  if (!impl_->transform(utmDefinition(e, zone, south), PJ_INV,
                 proj_coord(easting, northing, 0, 0), result))
    return false;
  lon = result.lp.lam;
  lat = result.lp.phi;
  return true;
}

bool ProjGeoProjection::geodeticToUtm(const geo::Ellipsoid &e, int zone,
                                      bool south, double lat, double lon,
                                      double &easting, double &northing) const {
  if (zone < 1 || zone > 60 || !std::isfinite(lat) || !std::isfinite(lon))
    return false;
  PJ_COORD result;
  if (!impl_->transform(utmDefinition(e, zone, south), PJ_FWD,
                 proj_coord(lon, lat, 0, 0), result))
    return false;
  easting = result.xy.x;
  northing = result.xy.y;
  return true;
}

bool ProjGeoProjection::geoidUndulation(double lat, double lon,
                                        double &n) const {
  if (impl_->geoidGridPath.empty() || !std::isfinite(lat) ||
      !std::isfinite(lon))
    return false;
  PJ_COORD result;
  // §25.2.3: WGS84 heights are relative to the geoid. Ask PROJ for N so
  // GeoFrame can compute h_ellipsoid = h_geoid + N; its default sign is -1.
  if (!impl_->transform("+proj=vgridshift +grids=" + impl_->geoidGridPath +
                     " +multiplier=1",
                 PJ_FWD, proj_coord(lon, lat, 0, 0), result))
    return false;
  n = result.xyz.z;
  return true;
}

} // namespace x3d::runtime::io::proj
