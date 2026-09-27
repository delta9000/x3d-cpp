// X3DRangeValidate.hpp — collect out-of-range values kept by the lenient read
// path. Node-agnostic: walks the graph via reflection and aggregates each
// node's validateRanges(). Range constraints ONLY (SFColor/SFColorRGBA [0,1],
// numeric minInclusive/maxInclusive) — enum/required/type/structural
// validation is out of scope here.
#ifndef X3D_RANGE_VALIDATE_HPP
#define X3D_RANGE_VALIDATE_HPP

#include "x3d/nodes/X3DNode.hpp"
#include "x3d/nodes/OrthoViewpoint.hpp"
#include "x3d/nodes/X3DBackgroundNode.hpp"
#include "GeoProjection.hpp"

#include <any>
#include <algorithm>
#include <cctype>
#include <charconv>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace range_detail {

/// Fixed-tuple view of the legacy MFFloat OrthoViewpoint field; absent if malformed.
inline std::optional<x3d::core::SFVec4f> orthoFieldOfView4(
    const x3d::nodes::OrthoViewpoint &node) {
  const auto &v = node.getFieldOfView();
  if (v.size() != 4) return std::nullopt;
  return x3d::core::SFVec4f{v[0], v[1], v[2], v[3]};
}

inline void add(const x3d::nodes::X3DNode &node, const std::string &field,
                const std::string &code, const std::string &detail,
                std::vector<x3d::core::RangeDiagnostic> &out) {
  out.push_back({node.nodeTypeName(), {}, field, code + ": " + detail});
}

inline void validateSpecial(const x3d::nodes::X3DNode &node,
                            std::vector<x3d::core::RangeDiagnostic> &out) {
  using namespace x3d;
  if (const auto *bg = dynamic_cast<const nodes::X3DBackgroundNode *>(&node)) {
    // §24.2.1: sky and ground angles must be non-decreasing.
    if (!std::is_sorted(bg->getSkyAngle().begin(), bg->getSkyAngle().end()))
      add(node, "skyAngle", "BACKGROUND_ANGLE_ORDER", "angles must be non-decreasing", out);
    if (!std::is_sorted(bg->getGroundAngle().begin(), bg->getGroundAngle().end()))
      add(node, "groundAngle", "BACKGROUND_ANGLE_ORDER", "angles must be non-decreasing", out);
  }
  if (node.nodeTypeName() == "OrthoViewpoint" ||
      node.nodeTypeName() == "TextureProjectorParallel") {
    for (const core::FieldInfo &f : node.fields()) {
      if (f.x3dName != "fieldOfView") continue;
      auto v = f.get(node);
      std::vector<float> xy;
      if (f.type == core::X3DFieldType::MFFloat) xy = std::any_cast<std::vector<float>>(v);
      else {
        auto q = std::any_cast<core::SFVec4f>(v);
        xy = {q.x, q.y, q.z, q.w};
      }
      if (xy.size() != 4) {
        add(node, "fieldOfView", "FOV_TUPLE_ARITY", "expected exactly 4 values", out);
      } else if (!(xy[0] < xy[2]) || !(xy[1] < xy[3])) {
        add(node, "fieldOfView", "FOV_EXTENT_ORDER", "minimum extents must be less than maximum extents", out);
      }
    }
  }
  for (const core::FieldInfo &f : node.fields()) {
    if (f.x3dName != "geoSystem") continue;
    const auto tokens = std::any_cast<std::vector<std::string>>(f.get(node));
    auto bad = [&](const std::string &why) { add(node, "geoSystem", "GEOSYSTEM_TOKEN", why, out); };
    if (tokens.empty()) { bad("missing spatial reference frame"); continue; }
    const auto &frame = tokens.front();
    if (frame != "GD" && frame != "GDC" && frame != "GC" && frame != "GCC" &&
        frame != "UTM" && frame != "WM") {
      bad("unknown spatial reference frame '" + frame + "'");
      continue;
    }
    const auto system = runtime::geo::parseGeoSystem(tokens);
    const bool utm = system.frame == runtime::geo::GeoSystem::Frame::UTM;
    const bool gd = system.frame == runtime::geo::GeoSystem::Frame::GD;
    bool zone = false, south = false, ellipsoid = false, geoid = false;
    for (std::size_t i = 1; i < tokens.size(); ++i) {
      const auto &t = tokens[i];
      runtime::geo::Ellipsoid e;
      if (t == "N") {
        bad("'N' is undefined; northern hemisphere is the default");
      } else if (utm && t.size() > 1 && t[0] == 'Z') {
        unsigned n = 0;
        const auto [end, ec] = std::from_chars(t.data() + 1, t.data() + t.size(), n);
        if (zone || ec != std::errc{} || end != t.data() + t.size() ||
            n < 1 || n > 60 || system.zone != static_cast<int>(n))
          bad("invalid or repeated UTM zone '" + t + "'");
        else zone = true;
      } else if (utm && t == "S") {
        if (south) bad("repeated UTM hemisphere token 'S'");
        else south = true;
      } else if ((gd && (t == "latitude_first" || t == "longitude_first")) ||
                 (utm && (t == "northing_first" || t == "easting_first"))) {
        // Ordering is interpreted by parseGeoSystem above.
      } else if ((gd || utm) && t == "WGS84") {
        if (geoid) bad("repeated geoid token 'WGS84'");
        else geoid = true;
      } else if ((gd || utm) && runtime::geo::ellipsoidByCode(t, e)) {
        if (ellipsoid) bad("repeated ellipsoid token '" + t + "'");
        else ellipsoid = true;
      } else {
        bad("unsupported token '" + t + "'");
      }
    }
    if (utm && !zone) bad("UTM requires a Z1 through Z60 zone token");
  }
}

/// Depth-first range-diagnostic walk guarded against graph cycles. `onPath`
/// holds the nodes on the current recursion stack: a child already on the path
/// is a USE referencing an ancestor (a true cycle) and is skipped, so the walk
/// terminates. A node NOT on the current path is still descended each time it is
/// reached, preserving the per-usage-site semantics for shared (DAG) subtrees.
inline void collect(const x3d::nodes::X3DNode &node, std::vector<x3d::core::RangeDiagnostic> &out,
                    std::unordered_set<const x3d::nodes::X3DNode *> &onPath) {
  if (!onPath.insert(&node).second)
    return; // cycle: node is an ancestor of itself on this path
  node.validateRanges(out);
  validateSpecial(node, out);
  for (const x3d::core::FieldInfo &f : node.fields()) {
    if (!f.isNode() || !f.isReadable())
      continue;
    std::any v = f.get(node);
    if (f.type == x3d::core::X3DFieldType::SFNode) {
      auto child = std::any_cast<std::shared_ptr<x3d::nodes::X3DNode>>(v);
      if (child)
        collect(*child, out, onPath);
    } else { // MFNode
      auto vec = std::any_cast<std::vector<std::shared_ptr<x3d::nodes::X3DNode>>>(v);
      for (const auto &child : vec)
        if (child)
          collect(*child, out, onPath);
    }
  }
  onPath.erase(&node);
}

} // namespace range_detail

/// Append every node's range diagnostics, depth-first over SFNode/MFNode
/// fields. A DEF/USE-shared node is visited once per reference, so its
/// diagnostics repeat per usage site; a cyclic graph (USE of an ancestor) is
/// walked without infinite recursion (see range_detail::collect).
inline void collectRangeWarnings(const x3d::nodes::X3DNode &node,
                                 std::vector<x3d::core::RangeDiagnostic> &out) {
  std::unordered_set<const x3d::nodes::X3DNode *> onPath;
  range_detail::collect(node, out, onPath);
}

/// Convenience overload returning a fresh vector.
inline std::vector<x3d::core::RangeDiagnostic> collectRangeWarnings(const x3d::nodes::X3DNode &root) {
  std::vector<x3d::core::RangeDiagnostic> out;
  collectRangeWarnings(root, out);
  return out;
}

#endif // X3D_RANGE_VALIDATE_HPP
