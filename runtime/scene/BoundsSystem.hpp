// BoundsSystem.hpp — local-frame AABB per node + bottom-up bounds propagation.
// World bounds are a lazy query composing TransformSystem::worldTransform.
// Side table keyed by const X3DNode*. namespace x3d::runtime.
#ifndef X3D_RUNTIME_BOUNDS_SYSTEM_HPP
#define X3D_RUNTIME_BOUNDS_SYSTEM_HPP

#include "Aabb.hpp"
#include "DirtyTracker.hpp"
#include "GeometryBounds.hpp"
#include "TransformSystem.hpp"
#include "x3d/nodes/X3DNode.hpp"
#include "X3DScene.hpp"

#include <any>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace x3d::runtime {
using namespace x3d::core;

class BoundsSystem {
public:
  void buildBounds(const Scene &scene, const TransformSystem &ts) {
    parent_.clear(); children_.clear(); local_.clear(); indexed_.clear();
    computing_.clear();
    for (const auto &root : scene.rootNodes)
      if (root) index(root.get(), nullptr);
    for (const auto &root : scene.rootNodes)
      if (root) compute(root.get(), ts);
  }

  const Aabb &localBounds(const X3DNode *n) const {
    static const Aabb kEmpty{};
    auto it = local_.find(n);
    return it == local_.end() ? kEmpty : it->second;
  }

  Aabb worldBounds(const X3DNode *n, const TransformSystem &ts) const {
    return localBounds(n).transformed(ts.worldTransform(n));
  }

  // Recompute dirtied subtrees bottom-up; mark each recomputed node DirtyBounds.
  void propagate(DirtyTracker &dirty, const TransformSystem &ts) {
    // Snapshot the changed nodes (markDirty during the walk would mutate the list).
    std::vector<const X3DNode *> seed(dirty.changedNodes().begin(),
                                      dirty.changedNodes().end());
    for (const X3DNode *n : seed) {
      if (!local_.count(n)) continue;            // not bounds-participating
      recomputeUp(n, ts, dirty);
    }
  }

private:
  // Delegates to TransformSystem so all transform-bearing types stay in sync.
  // Billboard is view-dependent (active Viewpoint) — deferred to M2c/M2d.
  bool isTransform(const X3DNode *n) const { return TransformSystem::isTransform(n); }

  // DFS index: parent + children over node-typed fields (guard null getters).
  void index(const X3DNode *n, const X3DNode *parent) {
    parent_[n] = parent;
    if (parent) children_[parent].push_back(n);   // record EVERY parent edge (bounds union needs all)
    // Recurse into n's subtree ONCE: a USE-shared node reachable by many paths must
    // not re-walk its subtree per path (multiplicative explosion / hang). Its parent
    // edge is still recorded above on every reference, so the bounds union stays exact.
    if (!indexed_.insert(n).second) return;
    for (const auto &f : n->fields()) {
      if (!f.get) continue;
      if (f.type == X3DFieldType::SFNode) {
        auto c = std::any_cast<std::shared_ptr<X3DNode>>(f.get(*n));
        if (c) index(c.get(), n);
      } else if (f.type == X3DFieldType::MFNode) {
        for (const auto &c : std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*n)))
          if (c) index(c.get(), n);
      }
    }
  }

  // Author bbox override iff every component of bboxSize >= 0.
  bool authorBounds(const X3DNode *n, Aabb &out) const {
    if (!geombounds::hasField(*n, "bboxSize")) return false;
    SFVec3f sz = geombounds::getField<SFVec3f>(*n, "bboxSize", {-1,-1,-1});
    if (sz.x < 0 || sz.y < 0 || sz.z < 0) return false;
    SFVec3f c = geombounds::getField<SFVec3f>(*n, "bboxCenter", {0,0,0});
    out = Aabb::fromCenterSize(c, sz);
    return true;
  }

  // Compute a node's local AABB in its own frame (post-order build). EVERY node
  // gets a local_ entry — including geometry leaves (Box/IFS/...), reached as graph
  // children via the Shape's "geometry" SFNode field — so a later change to a leaf
  // is found by propagate. A node's bounds = its own geometry (non-empty only if it
  // IS a geometry node) unioned with its children (child-Transform frames mapped in
  // via their local matrix). An author bbox overrides (children still get computed
  // for their own entries, but are not unioned into this node).
  Aabb compute(const X3DNode *n, const TransformSystem &ts) {
    // Memo: a node's local bounds are path-independent (its own frame; the parent
    // applies the child Transform at the union site below), so a USE-shared node is
    // computed ONCE, not once per root-to-node path. This is the load-bearing fix for
    // the multiplicative recompute that hung on heavy USE/DEF scenes.
    if (auto it = local_.find(n); it != local_.end()) return it->second;
    // Cycle guard (white/gray/black DFS): index() records EVERY parent edge, so a
    // containment cycle (a node USE'ing its own DEF, e.g. <X DEF='a' USE='a'/>)
    // puts a back-edge into children_. The post-order memo above only catches
    // FINISHED nodes; an in-progress node on the cycle is not yet in local_, so
    // without this guard compute() recurses the back-edge forever (stack overflow).
    // On re-entry of an in-progress node, contribute nothing — the back-reference
    // adds no geometry beyond what is already being unioned up the stack.
    if (!computing_.insert(n).second) return kCycleEmpty;
    Aabb childUnion;
    auto it = children_.find(n);
    if (it != children_.end())
      for (const X3DNode *c : it->second) {
        Aabb cb = compute(c, ts); // post-order: child entry set first
        if (isTransform(c)) cb = cb.transformed(TransformSystem::localMatrix(c));
        childUnion.unionWith(cb);
      }
    Aabb a;
    if (!authorBounds(n, a)) {       // author bbox is authoritative; else compute
      a = localGeometryBounds(n);    // empty unless n is itself a geometry node
      a.unionWith(childUnion);
    }
    local_[n] = a;
    computing_.erase(n);   // finished: subsequent visits hit the local_ memo above
    return a;
  }

  // Recompute n's local AABB and re-union ancestors, stopping when unchanged.
  void recomputeUp(const X3DNode *n, const TransformSystem &ts, DirtyTracker &dirty) {
    Aabb before = localBounds(n);
    Aabb now = recomputeLocal(n, ts);
    local_[n] = now;
    dirty.markDirty(n, DirtyBounds);
    if (equalish(before, now)) return; // no change to propagate further? still update self
    const X3DNode *p = parentOf(n);
    if (p) recomputeUp(p, ts, dirty);
  }

  // Recompute one node's local AABB from its CURRENT geometry + its children's
  // already-stored local bounds (mirrors compute() but non-recursive: children's
  // local_ entries are reused, since propagate walks bottom-up from the changed leaf).
  Aabb recomputeLocal(const X3DNode *n, const TransformSystem &ts) {
    (void)ts;
    Aabb a;
    if (authorBounds(n, a)) return a;
    a = localGeometryBounds(n);                 // empty unless n is a geometry node
    auto it = children_.find(n);
    if (it != children_.end())
      for (const X3DNode *c : it->second) {
        Aabb cb = localBounds(c);
        if (isTransform(c)) cb = cb.transformed(TransformSystem::localMatrix(c));
        a.unionWith(cb);
      }
    return a;
  }

  const X3DNode *parentOf(const X3DNode *n) const {
    auto it = parent_.find(n);
    return it == parent_.end() ? nullptr : it->second;
  }
  static bool equalish(const Aabb &a, const Aabb &b) {
    if (a.empty != b.empty) return false;
    if (a.empty) return true;
    auto f = [](float x, float y) { return (x - y) * (x - y) < 1e-10f; };
    return f(a.min.x,b.min.x) && f(a.min.y,b.min.y) && f(a.min.z,b.min.z) &&
           f(a.max.x,b.max.x) && f(a.max.y,b.max.y) && f(a.max.z,b.max.z);
  }

  std::unordered_map<const X3DNode *, const X3DNode *> parent_;
  std::unordered_map<const X3DNode *, std::vector<const X3DNode *>> children_;
  std::unordered_map<const X3DNode *, Aabb> local_;
  std::unordered_set<const X3DNode *> indexed_;   // subtrees already recursed (cycle/sharing guard)
  std::unordered_set<const X3DNode *> computing_; // nodes in-progress in compute() (cycle break)
  static inline const Aabb kCycleEmpty{};         // back-edge contribution on a cycle
};

} // namespace x3d::runtime
#endif // X3D_RUNTIME_BOUNDS_SYSTEM_HPP
