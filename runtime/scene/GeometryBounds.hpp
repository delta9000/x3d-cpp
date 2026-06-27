// GeometryBounds.hpp — local-frame AABB of a geometry node, by type dispatch +
// reflection field reads. namespace x3d::runtime.
#ifndef X3D_RUNTIME_GEOMETRY_BOUNDS_HPP
#define X3D_RUNTIME_GEOMETRY_BOUNDS_HPP

#include "Aabb.hpp"
#include "x3d/nodes/X3DNode.hpp"
#include <algorithm>
#include <any>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace x3d::runtime {
using namespace x3d::core;
namespace geombounds {

// Outcome of a reflection field read. Distinguishes a genuinely absent field
// (returning a default is correct) from a field that is present but stored as a
// different type than requested (a caller-side contract violation — e.g. reading
// an SFVec3d `position` as SFVec3f — which must be surfaced, not silently hidden).
enum class FieldRead { Ok, Absent, TypeMismatch };

// Classify + read field `name` into `out`. `out` is assigned only on Ok.
template <class T>
FieldRead readField(const X3DNode &n, const char *name, T &out) {
  for (const auto &f : n.fields())
    if (f.x3dName == name) {
      if (!f.get) return FieldRead::Absent; // no getter -> treat as absent
      std::any v = f.get(n);
      if (const T *p = std::any_cast<T>(&v)) { out = *p; return FieldRead::Ok; }
      return FieldRead::TypeMismatch;
    }
  return FieldRead::Absent;
}

// Safe reflection read: returns dflt when the field is absent. A field that is
// present but of the wrong type is a programming error (the caller asked for the
// wrong T): assert in debug builds so tests fail loud; in release, degrade to the
// default so production stays graceful. See FieldRead above.
template <class T>
T getField(const X3DNode &n, const char *name, T dflt) {
  switch (readField(n, name, dflt)) {
    case FieldRead::Ok:           return dflt; // readField wrote the value into dflt
    case FieldRead::Absent:       return dflt;
    case FieldRead::TypeMismatch:
      assert(false && "getField: field present but stored type != requested type");
      return dflt;
  }
  return dflt;
}

// Reads an SFVec3f field, also accepting an SFVec3d-typed field (double-precision
// geospatial coordinates — GeoViewpoint.position, GeoOrigin, etc.) by narrowing to
// float. Without this, a plain getField<SFVec3f> on a double field reads the default
// (GEO-1: bound GeoViewpoint pinned to the origin).
inline SFVec3f getVec3fLenient(const X3DNode &n, const char *name, SFVec3f dflt) {
  SFVec3f f = dflt;
  if (readField(n, name, f) == FieldRead::Ok) return f;
  SFVec3d d{dflt.x, dflt.y, dflt.z};
  if (readField(n, name, d) == FieldRead::Ok)
    return SFVec3f{(float)d.x, (float)d.y, (float)d.z};
  return dflt;
}

// Reads an MFVec3f "point"-style field, also accepting MFVec3d (GeoCoordinate.point)
// by narrowing each element to float. Empty if absent or another type (GEO-2).
inline std::vector<SFVec3f> getPointsLenient(const X3DNode &n, const char *name) {
  std::vector<SFVec3f> f;
  if (readField(n, name, f) == FieldRead::Ok) return f;
  std::vector<SFVec3d> d;
  if (readField(n, name, d) == FieldRead::Ok) {
    std::vector<SFVec3f> out;
    out.reserve(d.size());
    for (const auto &p : d) out.push_back(SFVec3f{(float)p.x, (float)p.y, (float)p.z});
    return out;
  }
  return {};
}

// Reads an SFFloat field, also accepting SFDouble (GeoElevationGrid.xSpacing,
// creaseAngle, etc.) by narrowing to float.
inline float getFloatLenient(const X3DNode &n, const char *name, float dflt) {
  float f = dflt;
  if (readField(n, name, f) == FieldRead::Ok) return f;
  double d = dflt;
  if (readField(n, name, d) == FieldRead::Ok) return (float)d;
  return dflt;
}

// Reads an MFFloat field, also accepting MFDouble (GeoElevationGrid.height) by
// narrowing each element to float. Empty if absent or another type.
inline std::vector<float> getFloatsLenient(const X3DNode &n, const char *name) {
  std::vector<float> f;
  if (readField(n, name, f) == FieldRead::Ok) return f;
  std::vector<double> d;
  if (readField(n, name, d) == FieldRead::Ok)
    return std::vector<float>(d.begin(), d.end());
  return {};
}

inline std::shared_ptr<X3DNode> getNode(const X3DNode &n, const char *name) {
  return getField<std::shared_ptr<X3DNode>>(n, name, nullptr);
}

inline bool hasField(const X3DNode &n, const char *name) {
  for (const auto &f : n.fields()) if (f.x3dName == name) return true;
  return false;
}

// AABB over a Coordinate-like node's "point". Accepts MFVec3f and MFVec3d
// (GeoCoordinate / CoordinateDouble) via getPointsLenient. Empty if none.
inline Aabb pointsBounds(const std::shared_ptr<X3DNode> &coordNode) {
  Aabb r;
  if (!coordNode) return r;
  auto pts = getPointsLenient(*coordNode, "point");
  for (const auto &p : pts) r.expand(p);
  return r;
}

} // namespace geombounds

/// Local-frame AABB of `geom` (the node in a Shape's `geometry` slot). Empty for
/// unsupported / null / degenerate geometry.
inline Aabb localGeometryBounds(const X3DNode *geom) {
  using namespace geombounds;
  if (!geom) return {};
  const std::string t = geom->nodeTypeName();

  if (t == "Box")
    return Aabb::fromCenterSize({0,0,0}, getField<SFVec3f>(*geom, "size", {2,2,2}));
  if (t == "Sphere") {
    float r = getField<float>(*geom, "radius", 1.0f);
    return Aabb::fromCenterSize({0,0,0}, {2*r, 2*r, 2*r});
  }
  if (t == "Cone") {
    float br = getField<float>(*geom, "bottomRadius", 1.0f);
    float h  = getField<float>(*geom, "height", 2.0f);
    return Aabb::fromCenterSize({0,0,0}, {2*br, h, 2*br});
  }
  if (t == "Cylinder") {
    float r = getField<float>(*geom, "radius", 1.0f);
    float h = getField<float>(*geom, "height", 2.0f);
    return Aabb::fromCenterSize({0,0,0}, {2*r, h, 2*r});
  }
  if (t == "NurbsCurve" || t == "NurbsPatchSurface")
    return pointsBounds(getNode(*geom, "controlPoint"));
  // Generic mesh: any geometry carrying a Coordinate via "coord" or "controlPoint".
  if (hasField(*geom, "coord"))
    return pointsBounds(getNode(*geom, "coord"));
  if (hasField(*geom, "controlPoint"))
    return pointsBounds(getNode(*geom, "controlPoint"));

  if (t == "ElevationGrid" || t == "GeoElevationGrid") {
    // Best-effort grid bound in the raw local frame (Geo* is not geo-projected —
    // see backlog M2B-2). x/z from dimensions*spacing, y from the height range.
    int xd = getField<int>(*geom, "xDimension", 0);
    int zd = getField<int>(*geom, "zDimension", 0);
    // GeoElevationGrid spacing/height are SFDouble/MFDouble; read leniently.
    float xs = getFloatLenient(*geom, "xSpacing", 1.0f);
    float zs = getFloatLenient(*geom, "zSpacing", 1.0f);
    auto hs = getFloatsLenient(*geom, "height");
    Aabb r;
    if (xd > 0 && zd > 0) {
      float ymin = 0, ymax = 0;
      bool first = true;
      for (float h : hs) { if (first){ymin=ymax=h;first=false;} else {ymin=std::min(ymin,h);ymax=std::max(ymax,h);} }
      r.expand({0, ymin, 0});
      r.expand({(xd-1)*xs, ymax, (zd-1)*zs});
    }
    return r;
  }
  if (t == "Extrusion") {
    // Conservative: AABB over spine points, expanded by the max cross-section
    // radius (scaled). Over-bounds safely without the spine-aligned-frame math.
    auto spine = getField<std::vector<SFVec3f>>(*geom, "spine", {});
    auto sect  = getField<std::vector<SFVec2f>>(*geom, "crossSection", {});
    auto scale = getField<std::vector<SFVec2f>>(*geom, "scale", {});
    float maxS = 1.0f;
    for (const auto &s : scale) maxS = std::max(maxS, std::max(std::fabs(s.x), std::fabs(s.y)));
    float rad = 0.0f;
    for (const auto &p : sect) rad = std::max(rad, std::sqrt(p.x*p.x + p.y*p.y));
    rad *= maxS;
    Aabb r;
    for (const auto &p : spine) {
      r.expand({p.x - rad, p.y - rad, p.z - rad});
      r.expand({p.x + rad, p.y + rad, p.z + rad});
    }
    return r;
  }
  if (t == "Text") {
    // Conservative symmetric estimate. Exact glyph bounds need a font engine
    // (backlog M2B-1). width ~ longest-string * size * 0.6, capped by maxExtent;
    // height ~ lineCount * size * spacing. Centered box (justification-agnostic).
    auto strs = getField<std::vector<std::string>>(*geom, "string", {});
    float size = 1.0f, spacing = 1.0f;
    if (auto fs = getNode(*geom, "fontStyle")) {
      size = getField<float>(*fs, "size", 1.0f);
      spacing = getField<float>(*fs, "spacing", 1.0f);
    }
    std::size_t maxLen = 0;
    for (const auto &s : strs) maxLen = std::max(maxLen, s.size());
    float width = static_cast<float>(maxLen) * size * 0.6f;
    float maxExtent = getField<float>(*geom, "maxExtent", 0.0f);
    if (maxExtent > 0.0f) width = std::min(width, maxExtent);
    float height = static_cast<float>(strs.empty() ? 1 : strs.size()) * size * spacing;
    if (width <= 0.0f && height <= 0.0f) return {};
    Aabb r;
    r.expand({-width, -height, 0});
    r.expand({ width,  height, 0});
    return r;
  }

  // Long-tail types not yet handled return empty.
  return {};
}

} // namespace x3d::runtime
#endif // X3D_RUNTIME_GEOMETRY_BOUNDS_HPP
