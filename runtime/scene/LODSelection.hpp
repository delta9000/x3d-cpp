#ifndef X3D_RUNTIME_LOD_SELECTION_HPP
#define X3D_RUNTIME_LOD_SELECTION_HPP

#include "GeometryBounds.hpp"
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

} // namespace x3d::runtime

#endif
