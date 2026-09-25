// NodeHandleTable.hpp
// Per-script table mapping the opaque SFNode handles a JS engine sees to the
// nodes they name.
//
// A script never holds a C++ pointer. pushNode interns the node here and gives
// the engine a small integer id, stored where script code cannot forge or
// rewrite it (a QuickJS class opaque / a Duktape hidden symbol). extractNode
// resolves the id back through the table:
//
//   - an unknown id (forged, or from another script) resolves to null, so
//     hostile content can never aim the runtime at an arbitrary address;
//   - the table holds a weak_ptr, so a node the scene has since dropped
//     resolves to null instead of dangling;
//   - a live node resolves to its REAL owning shared_ptr. An SFNode a script
//     writes into a field (e.g. `children`) therefore shares ownership exactly
//     like a DEF/USE reference — the ECMAScript binding's SFNode is a node
//     reference (ISO/IEC 19777-1), not a borrowed pointer.
#ifndef X3D_RUNTIME_NODE_HANDLE_TABLE_HPP
#define X3D_RUNTIME_NODE_HANDLE_TABLE_HPP

#include "x3d/nodes/X3DNode.hpp"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace x3d::runtime {

class NodeHandleTable {
public:
  using Id = std::uint32_t;
  static constexpr Id kNull = 0;

  /// Intern `node`, returning its id (kNull for a null node). The same live
  /// node keeps the same id for the life of the table.
  Id intern(const std::shared_ptr<x3d::nodes::X3DNode> &node) {
    if (!node) return kNull;
    auto it = ids_.find(node.get());
    if (it != ids_.end()) {
      std::weak_ptr<x3d::nodes::X3DNode> &slot = slots_[it->second - 1];
      // Same live object: reuse. A dead slot at a recycled address is stale;
      // mint a fresh id so the old handle can never alias the new node.
      if (!slot.expired()) return it->second;
      ids_.erase(it);
    }
    slots_.push_back(node);
    const Id id = static_cast<Id>(slots_.size());
    ids_.emplace(node.get(), id);
    return id;
  }

  /// The owning reference for `id`, or null if the id is unknown or the node
  /// has been destroyed.
  std::shared_ptr<x3d::nodes::X3DNode> resolve(Id id) const {
    if (id == kNull || id > slots_.size()) return nullptr;
    return slots_[id - 1].lock();
  }

private:
  std::vector<std::weak_ptr<x3d::nodes::X3DNode>> slots_; ///< id-1 -> node
  std::unordered_map<const x3d::nodes::X3DNode *, Id> ids_;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_NODE_HANDLE_TABLE_HPP
