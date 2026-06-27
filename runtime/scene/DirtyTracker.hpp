// DirtyTracker.hpp — per-node dirty category bits + changed-node list.
// The HdChangeTracker analog: the cascade feeds it during a tick; the consumer
// pulls changedNodes() after the tick. Keyed by const X3DNode*. Side table —
// nothing is stored on the node. namespace x3d::runtime.
#ifndef X3D_RUNTIME_DIRTY_TRACKER_HPP
#define X3D_RUNTIME_DIRTY_TRACKER_HPP

#include <unordered_map>
#include <vector>

namespace x3d::nodes { class X3DNode; }

namespace x3d::runtime {
using x3d::nodes::X3DNode;

enum DirtyFlags : unsigned {
  DirtyNone           = 0,
  DirtyLocalTransform = 1u << 0, // a Transform's TRS field changed
  DirtyWorldTransform = 1u << 1, // world matrix recomputed this tick
  DirtyChildren       = 1u << 2, // children/addChildren/removeChildren changed
  DirtyField          = 1u << 3, // any other field changed
  DirtyBounds         = 1u << 4, // a node's local/world AABB needs recompute (M2b)
};

class DirtyTracker {
public:
  void markDirty(const X3DNode *n, unsigned flags) {
    if (!n) return;
    unsigned &cur = flags_[n];
    if (cur == DirtyNone) changed_.push_back(n); // first transition -> list it
    cur |= flags;
  }

  unsigned flags(const X3DNode *n) const {
    auto it = flags_.find(n);
    return it == flags_.end() ? DirtyNone : it->second;
  }

  const std::vector<const X3DNode *> &changedNodes() const { return changed_; }

  void clear() {
    flags_.clear();
    changed_.clear();
  }

private:
  std::unordered_map<const X3DNode *, unsigned> flags_;
  std::vector<const X3DNode *> changed_;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_DIRTY_TRACKER_HPP
