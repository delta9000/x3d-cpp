#ifndef X3D_RUNTIME_LOD_SELECTION_HPP
#define X3D_RUNTIME_LOD_SELECTION_HPP

#include "GeometryBounds.hpp"
#include "Mat4.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace x3d::runtime {

// §23.4.3: select the level from distance in the LOD's local frame.
inline int lodSelectLevel(const x3d::nodes::X3DNode &lod, float distToCenter) {
  const auto range = geombounds::getField<std::vector<float>>(lod, "range", {});
  if (range.empty()) return 0;
  int level = 0;
  for (float r : range) { if (distToCenter >= r) ++level; else break; }
  return level;
}

// §10.4.3 / §23.4.3: rendering traverses only the selected Switch child or
// LOD level (world = the node's world matrix; eyeWorld = viewer position).
inline std::shared_ptr<x3d::nodes::X3DNode> traversedChild(const x3d::nodes::X3DNode &node,
                                                          const Mat4 &world,
                                                          const x3d::core::SFVec3f &eyeWorld) {
  const auto kids = geombounds::getField<std::vector<std::shared_ptr<x3d::nodes::X3DNode>>>(
      node, "children", {});
  if (kids.empty()) return {};
  int choice = -1;
  if (node.nodeTypeName() == "Switch") {
    choice = geombounds::getField<int>(node, "whichChoice", -1);
  } else if (node.nodeTypeName() == "LOD") {
    const auto center = geombounds::getField<x3d::core::SFVec3f>(node, "center", {0, 0, 0});
    const auto eye = world.inverse().transformPoint(eyeWorld);
    const float dx = eye.x - center.x, dy = eye.y - center.y, dz = eye.z - center.z;
    choice = std::min(lodSelectLevel(node, std::sqrt(dx * dx + dy * dy + dz * dz)),
                      static_cast<int>(kids.size()) - 1);
  }
  return choice >= 0 && choice < static_cast<int>(kids.size()) ? kids[choice] : nullptr;
}

} // namespace x3d::runtime

#endif
