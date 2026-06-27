// X3DRangeValidate.hpp — collect out-of-range values kept by the lenient read
// path. Node-agnostic: walks the graph via reflection and aggregates each
// node's validateRanges(). Range constraints ONLY (SFColor/SFColorRGBA [0,1],
// numeric minInclusive/maxInclusive) — enum/required/type/structural
// validation is out of scope here.
#ifndef X3D_RANGE_VALIDATE_HPP
#define X3D_RANGE_VALIDATE_HPP

#include "x3d/nodes/X3DNode.hpp"

#include <any>
#include <memory>
#include <unordered_set>
#include <vector>

namespace range_detail {

/// Depth-first range-diagnostic walk guarded against graph cycles. `onPath`
/// holds the nodes on the current recursion stack: a child already on the path
/// is a USE referencing an ancestor (a true cycle) and is skipped, so the walk
/// terminates. A node NOT on the current path is still descended each time it is
/// reached, preserving the per-usage-site semantics for shared (DAG) subtrees.
inline void collect(const X3DNode &node, std::vector<RangeDiagnostic> &out,
                    std::unordered_set<const X3DNode *> &onPath) {
  if (!onPath.insert(&node).second)
    return; // cycle: node is an ancestor of itself on this path
  node.validateRanges(out);
  for (const FieldInfo &f : node.fields()) {
    if (!f.isNode() || !f.isReadable())
      continue;
    std::any v = f.get(node);
    if (f.type == X3DFieldType::SFNode) {
      auto child = std::any_cast<std::shared_ptr<X3DNode>>(v);
      if (child)
        collect(*child, out, onPath);
    } else { // MFNode
      auto vec = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(v);
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
inline void collectRangeWarnings(const X3DNode &node,
                                 std::vector<RangeDiagnostic> &out) {
  std::unordered_set<const X3DNode *> onPath;
  range_detail::collect(node, out, onPath);
}

/// Convenience overload returning a fresh vector.
inline std::vector<RangeDiagnostic> collectRangeWarnings(const X3DNode &root) {
  std::vector<RangeDiagnostic> out;
  collectRangeWarnings(root, out);
  return out;
}

#endif // X3D_RANGE_VALIDATE_HPP
