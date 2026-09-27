// GeoNodes.hpp — reads Geospatial node fields into the GeoFrame conversions
// (ISO/IEC 19775-1 §25, ADR-0053).
//
// Every node that carries geographic coordinates (GeoCoordinate,
// GeoElevationGrid, GeoLocation, GeoTransform, GeoViewpoint,
// GeoPositionInterpolator, GeoProximitySensor, GeoTouchSensor, GeoLOD,
// GeoOrigin) has a `geoSystem` MFString and, except GeoOrigin, an optional
// `geoOrigin` SFNode. These helpers turn one of that node's authored
// coordinates into the float X3D world (relative to its GeoOrigin), so every
// consumer — mesh building, bounds, transforms, the camera, sensors — converts
// the same way.
#ifndef X3D_RUNTIME_SCENE_GEO_NODES_HPP
#define X3D_RUNTIME_SCENE_GEO_NODES_HPP

#include "GeoFrame.hpp"
#include "GeometryBounds.hpp"

#include <string>
#include <vector>

namespace x3d::runtime::geo {

using x3d::nodes::X3DNode;

/// The node's parsed `geoSystem` (default [ "GD", "WE" ]).
inline GeoSystem systemOf(const X3DNode &n) {
  return parseGeoSystem(
      geombounds::getField<std::vector<std::string>>(n, "geoSystem", {}));
}

/// The frame of the node's `geoOrigin` (§25.3.6). Without one — or if its
/// coordinates cannot be converted — the world is geocentric (origin 0, no
/// rotation).
inline OriginFrame originOf(const X3DNode &n, const GeoProjection &p = projection()) {
  OriginFrame frame;
  auto o = geombounds::getNode(n, "geoOrigin");
  if (!o) return frame;
  const SFVec3d coords = geombounds::getField<SFVec3d>(*o, "geoCoords", SFVec3d{0, 0, 0});
  const bool rotateYUp = geombounds::getField<bool>(*o, "rotateYUp", false);
  OriginFrame made;
  if (makeOriginFrame(systemOf(*o), coords, rotateYUp, made, p)) frame = made;
  return frame;
}

/// Convert one coordinate authored on `n` into the X3D world. False if the
/// coordinate cannot be converted (e.g. UTM without a zone).
inline bool toWorld(const X3DNode &n, const SFVec3d &coords, SFVec3f &out,
                    const GeoProjection &p = projection()) {
  SFVec3d gc;
  if (!toGeocentric(systemOf(n), coords, gc, p)) return false;
  out = originOf(n, p).toWorld(gc);
  return true;
}

/// Convert every coordinate of a list (e.g. GeoCoordinate.point) with one
/// parse of the node's geoSystem/geoOrigin. Unconvertible entries become
/// (0,0,0) and are counted in `failures`.
inline std::vector<SFVec3f> toWorld(const X3DNode &n, const std::vector<SFVec3d> &coords,
                                    std::size_t *failures = nullptr,
                                    const GeoProjection &p = projection()) {
  const GeoSystem sys = systemOf(n);
  const OriginFrame origin = originOf(n, p);
  std::vector<SFVec3f> out;
  out.reserve(coords.size());
  std::size_t bad = 0;
  for (const SFVec3d &c : coords) {
    SFVec3d gc;
    if (toGeocentric(sys, c, gc, p)) out.push_back(origin.toWorld(gc));
    else { out.push_back(SFVec3f{0, 0, 0}); ++bad; }
  }
  if (failures) *failures = bad;
  return out;
}

/// The world matrix of the local tangent frame at a coordinate authored on
/// `n` (+X east, +Y up, −Z north; §25.3.3): GeoLocation's placement,
/// GeoTransform's geoCenter frame, GeoViewpoint's orientation frame.
inline bool tangentFrameOf(const X3DNode &n, const SFVec3d &coords, Mat4 &out,
                           const GeoProjection &p = projection()) {
  return tangentFrame(systemOf(n), coords, originOf(n, p), out, p);
}

/// Inverse of toWorld for outputs such as geoCoord_changed / hitGeoCoord_changed.
inline bool fromWorld(const X3DNode &n, const SFVec3f &world, SFVec3d &coords,
                      const GeoProjection &p = projection()) {
  const OriginFrame origin = originOf(n, p);
  SFVec3d d{world.x, world.y, world.z};
  if (origin.rotateYUp) {
    // Undo World = basisᵀ·(gc − origin): gc = origin + east·x + up·y + south·z.
    const LocalBasis &b = origin.basis;
    d = SFVec3d{b.east.x * d.x + b.up.x * d.y + b.south.x * d.z,
                b.east.y * d.x + b.up.y * d.y + b.south.y * d.z,
                b.east.z * d.x + b.up.z * d.y + b.south.z * d.z};
  }
  const SFVec3d gc{origin.origin.x + d.x, origin.origin.y + d.y, origin.origin.z + d.z};
  return fromGeocentric(systemOf(n), gc, coords, p);
}

} // namespace x3d::runtime::geo

#endif // X3D_RUNTIME_SCENE_GEO_NODES_HPP
