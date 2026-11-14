// Small scene-building helpers shared by the runtime test suites.
#ifndef X3D_TEST_SUPPORT_SCENE_TEST_HELPERS_HPP
#define X3D_TEST_SUPPORT_SCENE_TEST_HELPERS_HPP

#include "x3d/nodes/X3DNode.hpp"

#include <any>
#include <cmath>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace x3d::test {

/// Set a field by its X3D name through reflection; a no-op if there is no such settable field.
inline void setF(const std::shared_ptr<x3d::nodes::X3DNode> &n, std::string_view name, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == name && f.set) {
      f.set(*n, std::move(v));
      return;
    }
}

/// Append `c` to `p.children`.
inline void addChild(const std::shared_ptr<x3d::nodes::X3DNode> &p,
                     const std::shared_ptr<x3d::nodes::X3DNode> &c) {
  using Kids = std::vector<std::shared_ptr<x3d::nodes::X3DNode>>;
  for (auto &f : p->fields())
    if (f.x3dName == "children" && f.set) {
      auto k = std::any_cast<Kids>(f.get(*p));
      k.push_back(c);
      f.set(*p, std::any(std::move(k)));
      return;
    }
}

inline bool feq(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) < eps; }

} // namespace x3d::test

#endif
