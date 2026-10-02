// BoundsSystem.hpp — local-frame AABB per node + bottom-up bounds propagation.
// World bounds are a lazy query composing TransformSystem::worldTransform.
// Side table keyed by const X3DNode*. namespace x3d::runtime.
#ifndef X3D_RUNTIME_BOUNDS_SYSTEM_HPP
#define X3D_RUNTIME_BOUNDS_SYSTEM_HPP

#include "FieldRead.hpp"
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
#include <utility>
#include <vector>

namespace x3d::runtime {
using namespace x3d::core;

class BoundsSystem {
public:
  // Optional FontMetrics seam for exact Text glyph bounds. When unset (the
  // default — the SDK is IO-free), Text uses its conservative heuristic.
  void setFontMetrics(extract::FontMetrics fm) {
    fontMetrics_ = std::move(fm);
  }

  void buildBounds(const Scene &scene, const TransformSystem &ts) {
    roots_.clear();
    for (const auto &root : scene.rootNodes)
      if (root) roots_.push_back(root);
    rebuild(ts);
    ++revision_;
  }

  /// Monotonic revision — bumps whenever any node's local bound is (re)computed
  /// to a different value (buildBounds, or a propagate that actually changed a
  /// bound). A no-op tick leaves it unchanged. Cheap cache key for bound-derived
  /// consumer state (e.g. a pick index's world AABBs).
  std::uint64_t revision() const { return revision_; }

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
    // A node-reference write can destroy the old child before this pass. Compare
    // identities without dereferencing cached pointers; rebuild only on an actual
    // edge change (including an expired old node at a reused address).
    bool topologyChanged = false;
    for (const X3DNode *n : seed) {
      auto it = lifetime_.find(n);
      if (it == lifetime_.end()) continue;
      auto live = it->second.lock();
      if (!live || childrenChanged(*live)) { topologyChanged = true; break; }
    }
    if (topologyChanged) {
      auto before = std::move(local_);
      rebuild(ts);
      bool changed = before.size() != local_.size();
      for (const auto &[n, now] : local_) {
        auto it = before.find(n);
        if (it == before.end() || !equalish(it->second, now)) {
          dirty.markDirty(n, DirtyBounds);
          changed = true;
        }
      }
      if (changed) ++revision_;
      return;
    }

    // Invalidate the complete ancestor closure BEFORE recomputing. A single
    // parent pointer misses USE placements; recursive recompute-up can process a
    // diamond's common ancestor before its second branch has refreshed. Memoized
    // post-order computation visits each affected node once, even on dense DAGs.
    // Include ancestors even when the seed's local box is unchanged: changing a
    // Transform's local matrix changes its contribution in its parent's frame.
    std::unordered_set<const X3DNode *> affected;
    std::vector<const X3DNode *> pending = seed;
    while (!pending.empty()) {
      const X3DNode *n = pending.back();
      pending.pop_back();
      if (!local_.count(n) || !affected.insert(n).second) continue;
      auto it = parents_.find(n);
      if (it != parents_.end())
        pending.insert(pending.end(), it->second.begin(), it->second.end());
    }
    std::unordered_map<const X3DNode *, Aabb> before;
    for (const X3DNode *n : affected) {
      before.emplace(n, local_.at(n));
      local_.erase(n);
    }
    bool changed = false;
    for (const X3DNode *n : affected) {
      Aabb now = compute(n, ts);
      dirty.markDirty(n, DirtyBounds);
      if (!equalish(before.at(n), now)) changed = true;
    }
    if (changed) ++revision_;
  }

private:
  // Delegates to TransformSystem so all transform-bearing types stay in sync.
  // Billboard is view-dependent (active Viewpoint) — deferred to M2c/M2d.
  bool isTransform(const X3DNode *n) const { return TransformSystem::isTransform(n); }

  bool childrenChanged(const X3DNode &n) const {
    std::unordered_set<const X3DNode *> now;
    bool expired = false;
    forEachChildNode(n, [&](const FieldInfo &, const std::shared_ptr<X3DNode> &c) {
      now.insert(c.get());
      auto it = lifetime_.find(c.get());
      if (it == lifetime_.end() || it->second.expired()) expired = true;
    });
    auto it = children_.find(&n);
    if (expired) return true;
    if (it == children_.end()) return !now.empty();
    if (it->second.size() != now.size()) return true;
    for (const X3DNode *c : it->second)
      if (!now.count(c)) return true;
    return false;
  }

  void rebuild(const TransformSystem &ts) {
    parents_.clear(); children_.clear(); local_.clear(); indexed_.clear();
    computing_.clear(); lifetime_.clear();
    for (const auto &root : roots_)
      if (auto live = root.lock()) index(live, nullptr);
    for (const auto &root : roots_)
      if (auto live = root.lock()) compute(live.get(), ts);
  }

  // Record every distinct parent edge, but recurse each shared subtree once.
  // Weak identities keep the tables non-owning and let propagation recognize
  // nodes destroyed by a field replacement without reading the old object.
  void index(const std::shared_ptr<X3DNode> &node, const X3DNode *parent) {
    const X3DNode *n = node.get();
    if (parent && parents_[n].insert(parent).second)
      children_[parent].push_back(n);
    if (!indexed_.insert(n).second) return;
    lifetime_[n] = node;
    forEachChildNode(*n, [&](const FieldInfo &, const std::shared_ptr<X3DNode> &c) {
      index(c, n);
    });
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
      a = localGeometryBounds(n, fontMetrics_); // empty unless n is itself a geometry node
      a.unionWith(childUnion);
    }
    local_[n] = a;
    computing_.erase(n);   // finished: subsequent visits hit the local_ memo above
    return a;
  }

  static bool equalish(const Aabb &a, const Aabb &b) {
    if (a.empty != b.empty) return false;
    if (a.empty) return true;
    auto f = [](float x, float y) { return (x - y) * (x - y) < 1e-10f; };
    return f(a.min.x,b.min.x) && f(a.min.y,b.min.y) && f(a.min.z,b.min.z) &&
           f(a.max.x,b.max.x) && f(a.max.y,b.max.y) && f(a.max.z,b.max.z);
  }

  std::unordered_map<const X3DNode *, std::unordered_set<const X3DNode *>> parents_;
  std::unordered_map<const X3DNode *, std::weak_ptr<X3DNode>> lifetime_;
  std::vector<std::weak_ptr<X3DNode>> roots_;
  std::unordered_map<const X3DNode *, std::vector<const X3DNode *>> children_;
  std::unordered_map<const X3DNode *, Aabb> local_;
  std::unordered_set<const X3DNode *> indexed_;   // subtrees already recursed (cycle/sharing guard)
  std::unordered_set<const X3DNode *> computing_; // nodes in-progress in compute() (cycle break)
  static inline const Aabb kCycleEmpty{};         // back-edge contribution on a cycle
  extract::FontMetrics fontMetrics_{};            // empty => heuristic Text bounds
  std::uint64_t revision_ = 0;                    // monotonic local-bound revision
};

} // namespace x3d::runtime
#endif // X3D_RUNTIME_BOUNDS_SYSTEM_HPP
