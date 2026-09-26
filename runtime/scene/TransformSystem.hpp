// TransformSystem.hpp — transform-hierarchy index + world-transform side table
// + incremental world-transform propagation. M2C-4: covers all static
// transform-bearing types (Transform, HAnimHumanoid, HAnimJoint, CADPart).
// M2C-2: propagate() also structurally re-indexes a grouping node whose children
// changed (DirtyChildren) — add/remove only the affected subtree — and exposes a
// monotonic revision() cache key. Billboard is view-dependent (active Viewpoint)
// — deferred to M2c/M2d. World transforms are stored in a side table keyed by
// const X3DNode* (nothing on the node). namespace x3d::runtime.
#ifndef X3D_RUNTIME_TRANSFORM_SYSTEM_HPP
#define X3D_RUNTIME_TRANSFORM_SYSTEM_HPP

#include "FieldRead.hpp"
#include "DirtyTracker.hpp"
#include "Mat4.hpp"
#include "x3d/nodes/X3DNode.hpp"
#include "X3DScene.hpp"

#include <algorithm>
#include <any>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace x3d::runtime {
using namespace x3d::core;

class TransformSystem {
public:
  /// Diagnostic counter: number of localMatrix() TRS recompositions performed
  /// process-wide. localMatrix() is non-trivial — five reflective field scans
  /// plus a quaternion compose — so incremental consumers must memoize shared
  /// ancestors instead of recomposing them once per dependent render item. Tests
  /// snapshot this around delta() to assert the re-accumulation stays
  /// O(distinct transforms), not O(items * depth).
  static inline std::uint64_t localMatrixCalls_ = 0;
  static std::uint64_t localMatrixCallCount() { return localMatrixCalls_; }

  /// Build the Transform hierarchy index + initial world transforms from a Scene.
  void buildIndex(const Scene &scene) {
    parent_.clear(); children_.clear(); world_.clear(); walked_.clear();
    nearestTransform_.clear(); directChildren_.clear(); refCount_.clear();
    roots_.clear();
    for (const auto &root : scene.rootNodes)
      if (root) {
        roots_.push_back(root.get());
        walk(root.get(), /*parentNode=*/nullptr, /*parentTransform=*/nullptr);
      }
    ++revision_;
  }

  /// World matrix for a Transform node (identity if not indexed).
  Mat4 worldTransform(const X3DNode *n) const {
    auto it = world_.find(n);
    return it == world_.end() ? Mat4::identity() : it->second;
  }

  /// World matrix for ANY node — Transform nodes get their own world; non-Transform
  /// nodes inherit their nearest ancestor Transform's world. Identity if the node
  /// is unindexed or has no Transform ancestor. Used by TransformSensor (whose
  /// targetObject may be reachable via both a non-Transform reference like
  /// `targetObject` AND a Transform ancestor via `children`; the scene-graph
  /// walk from roots picks the wrong path, but walking UP via the parent index
  /// always finds the Transform ancestor).
  ///
  /// Reads `localMatrix` on every call (which reads CURRENT field values), so
  /// mid-tick writes via `setF` are reflected immediately — unlike the cached
  /// `worldTransform`, which only refreshes via `propagate()` at the END of a
  /// tick (after systems have run). Critical for TransformSensor, whose target
  /// can move between ticks.
  Mat4 worldTransformAny(const X3DNode *n) const {
    if (!n) return Mat4::identity();
    const X3DNode *root = nullptr;
    if (isTransform(n)) {
      root = n;
    } else {
      auto it = nearestTransform_.find(n);
      if (it == nearestTransform_.end()) return Mat4::identity();
      root = it->second;
    }
    // Accumulate from `root` UP through Transform ancestors via the parent
    // index. localMatrix() is a static read of current field values — always
    // fresh, no stale-cache hazard.
    Mat4 m = localMatrix(root);
    for (const X3DNode *a = parentOf(root); a; a = parentOf(a)) {
      if (!isTransform(a)) break; // parent_ only stores Transform parents
      m = localMatrix(a) * m;
    }
    return m;
  }

  /// World matrix of Transform node `n` as reached through the parent edge
  /// `parent` (a transform-bearing node, or nullptr for a root placement). A
  /// DEF/USE node shared under two parents has a DISTINCT world matrix per
  /// parent; this resolves the one for `parent` live as `worldTransform(parent)
  /// * localMatrix(n)` — always fresh, no stale cache. Returns identity when
  /// `parent` is not an indexed parent edge of `n`. For `n`'s canonical (first)
  /// parent it equals `worldTransform(n)`.
  Mat4 worldTransformUnder(const X3DNode *parent, const X3DNode *n) const {
    if (!n || !isTransform(n)) return Mat4::identity();
    if (!parent) {
      auto pit = parent_.find(n);
      return (pit != parent_.end() && pit->second == nullptr)
                 ? worldTransform(n)
                 : Mat4::identity();
    }
    auto it = children_.find(parent);
    if (it == children_.end()) return Mat4::identity();
    for (const X3DNode *c : it->second)
      if (c == n) return worldTransform(parent) * localMatrix(n);
    return Mat4::identity();
  }

  /// Monotonic revision counter — bumps whenever a world matrix or the
  /// hierarchy index changes (buildIndex, a transform re-accumulation, or a
  /// structural re-walk). A no-op tick leaves it unchanged. Cheap const
  /// accessor for caches keyed on the transform state (e.g. a pick index).
  std::uint64_t revision() const { return revision_; }

  /// Recompute world transforms for every dirtied subtree (marking each
  /// recomputed node DirtyWorldTransform), then structurally re-index any
  /// grouping node whose child set changed (DirtyChildren). Only subtrees under
  /// a dirtied local transform, or whose direct-child membership changed, are
  /// revisited.
  void propagate(DirtyTracker &dirty) {
    // Collect the dirtied Transform roots: nodes flagged DirtyLocalTransform with
    // NO ancestor also so flagged (the ancestor's subtree pass covers them).
    // Also collect the grouping nodes whose children changed. Snapshot both
    // before marking -- recompute/reindex append to changedNodes().
    std::vector<const X3DNode *> roots;
    std::vector<const X3DNode *> structural;
    for (const X3DNode *n : dirty.changedNodes()) {
      const unsigned f = dirty.flags(n);
      if (f & DirtyChildren) structural.push_back(n);
      if (!(f & DirtyLocalTransform)) continue;
      if (!parent_.count(n) && !world_.count(n)) continue; // not a known Transform
      bool ancestorDirty = false;
      for (const X3DNode *p = parentOf(n); p; p = parentOf(p))
        if (dirty.flags(p) & DirtyLocalTransform) { ancestorDirty = true; break; }
      if (!ancestorDirty) roots.push_back(n);
    }
    std::unordered_set<const X3DNode *> visited;
    for (const X3DNode *r : roots)
      recompute(r, parentOf(r), worldTransform(parentOf(r)), dirty, visited);

    bool changed = !roots.empty();
    staleCanonical_.clear();
    for (const X3DNode *g : structural)
      if (reindexChildren(g, dirty)) changed = true;
    // A Transform moved between two grouping nodes in the same tick can survive
    // its canonical parent edge being dropped while another parent still reaches
    // it (refCount_ > 1), leaving parent_/world_ pointing at the OLD frame. Once
    // every DirtyChildren change is applied, re-point any such node along the
    // first path (the same rule buildIndex uses) so the result is the same
    // regardless of the order the dirty groups were processed.
    if (repairCanonicalParents(dirty)) changed = true;
    if (changed) ++revision_;
  }

  // Read a transform-bearing node's local matrix from its TRS fields via
  // reflection. Public so BoundsSystem/PickSystem/LightSystem can reuse it.
  static Mat4 localMatrix(const X3DNode *n) {
    ++localMatrixCalls_;
    return transformMatrix(getVec(n, "translation"), getRot(n, "rotation"),
                           getVec(n, "scale"), getVec(n, "center"),
                           getRot(n, "scaleOrientation"));
  }

  // Returns true for any node type that carries a full TRS frame
  // (translation/rotation/scale/center/scaleOrientation) and participates in
  // the transform hierarchy. Public so callers can share a single definition.
  // Billboard is view-dependent (active Viewpoint) — deferred to M2c/M2d.
  static bool isTransform(const X3DNode *n) {
    if (!n) return false;
    const std::string t = n->nodeTypeName();
    return t == "Transform" || t == "HAnimHumanoid" || t == "HAnimJoint" ||
           t == "CADPart";
  }

private:
  const X3DNode *parentOf(const X3DNode *n) const {
    auto it = parent_.find(n);
    return it == parent_.end() ? nullptr : it->second;
  }

  static SFVec3f getVec(const X3DNode *n, const std::string &name) {
    for (const auto &f : n->fields())
      if (f.x3dName == name) {
        FieldRef<SFVec3f> v(*n, f);
        return v ? *v : SFVec3f{0, 0, 0};
      }
    return SFVec3f{0, 0, 0};
  }
  static SFRotation getRot(const X3DNode *n, const std::string &name) {
    for (const auto &f : n->fields())
      if (f.x3dName == name) {
        FieldRef<SFRotation> v(*n, f);
        return v ? *v : SFRotation{0, 0, 1, 0};
      }
    return SFRotation{0, 0, 1, 0};
  }

  // The transform-bearing node serving a containing node's children: the node
  // itself if Transform-bearing, else its nearest ancestor Transform (or null).
  const X3DNode *servingTransformOf(const X3DNode *p) const {
    if (!p) return nullptr;
    if (isTransform(p)) return p;
    auto it = nearestTransform_.find(p);
    return it == nearestTransform_.end() ? nullptr : it->second;
  }

  // Record a Transform node's edge to `parentTransform`: canonical parent/world
  // on the FIRST edge seen, and a children_ entry on EVERY edge so a USE-shared
  // node under two parents is reachable via both (worldTransformUnder()).
  void registerEdge(const X3DNode *n, const X3DNode *parentTransform) {
    auto pit = parent_.find(n);
    if (pit == parent_.end()) {
      parent_[n] = parentTransform;
      world_[n] = (parentTransform ? worldTransform(parentTransform)
                                   : Mat4::identity()) *
                  localMatrix(n);
    } else if (pit->second == parentTransform) {
      world_[n] = (parentTransform ? worldTransform(parentTransform)
                                   : Mat4::identity()) *
                  localMatrix(n);
    }
    if (parentTransform) {
      auto &kids = children_[parentTransform];
      if (std::find(kids.begin(), kids.end(), n) == kids.end()) kids.push_back(n);
    }
  }

  // DFS the scene graph over node-typed fields; index Transforms + seed worlds.
  void walk(const X3DNode *n, const X3DNode *parentNode,
            const X3DNode *parentTransform) {
    const X3DNode *nextParent = parentTransform;
    if (isTransform(n)) {
      registerEdge(n, parentTransform);
      nextParent = n;
    } else if (parentTransform) {
      // Non-Transform node inherits the nearest ancestor Transform; used by
      // worldTransformAny() to compute the world matrix of any node without
      // needing to walk down from roots (which can find a non-Transform path).
      nearestTransform_[n] = parentTransform;
    }
    // Record the direct-parent edge (all node types) for structural diffing; a
    // USE-shared node reached via several parents counts each distinct edge.
    if (parentNode && directChildren_[parentNode].insert(n).second) ++refCount_[n];
    // Recurse each subtree ONCE: a USE-shared node reachable by many paths must not
    // re-walk its subtree per path (multiplicative explosion / hang). world_ stays the
    // documented per-node (first-path) approximation; the extractor re-accumulates
    // per-path itself, so the render path is unaffected.
    if (!walked_.insert(n).second) return;
    forEachChildNode(*n, [&](const FieldInfo &, const std::shared_ptr<X3DNode> &c) {
      walk(c.get(), n, nextParent);
    });
  }

  // Recompute world = parentWorld * local for `n`, then recurse its canonical
  // child Transforms. A USE-shared node reached via a NON-canonical parent edge
  // is left to worldTransformUnder()'s live resolution, so the canonical world
  // (first path) is never clobbered by another path.
  void recompute(const X3DNode *n, const X3DNode *parentNode,
                 const Mat4 &parentWorld, DirtyTracker &dirty,
                 std::unordered_set<const X3DNode *> &visited) {
    if (!visited.insert(n).second) return;
    Mat4 w = parentWorld * localMatrix(n);
    if (parentOf(n) == parentNode) world_[n] = w; // canonical edge only
    dirty.markDirty(n, DirtyWorldTransform);
    auto it = children_.find(n);
    if (it == children_.end()) return;
    for (const X3DNode *c : it->second)
      if (parentOf(c) == n) recompute(c, n, w, dirty, visited);
  }

  // Remove ONE occurrence of `c` from children_[serving] (no-op if absent).
  void removeEdgeChild(const X3DNode *serving, const X3DNode *c) {
    auto it = children_.find(serving);
    if (it == children_.end()) return;
    auto &v = it->second;
    auto p = std::find(v.begin(), v.end(), c);
    if (p != v.end()) v.erase(p);
  }

  // Drop the parent edge g -> c. Erases c's whole subtree only when no other
  // indexed parent still reaches it (USE sharing).
  void dropEdge(const X3DNode *g, const X3DNode *c) {
    auto it = directChildren_.find(g);
    if (it == directChildren_.end() || !it->second.erase(c)) return;
    const X3DNode *serving = servingTransformOf(g);
    if (isTransform(c) && serving) removeEdgeChild(serving, c);
    auto rc = refCount_.find(c);
    if (rc != refCount_.end() && rc->second > 1) {
      --rc->second;
      // Still referenced elsewhere: its canonical parent edge may have just been
      // dropped, so re-derive it once all structural changes settle.
      if (isTransform(c)) staleCanonical_.push_back(c);
      return;
    }
    if (rc != refCount_.end()) refCount_.erase(rc);
    eraseSubtree(c, serving);
  }

  // Erase `c` (already detached from its parent edge) and every descendant edge
  // that loses its last reference. `parentServing` is the transform serving c's
  // siblings (inherited by non-Transform children).
  void eraseSubtree(const X3DNode *c, const X3DNode *parentServing) {
    const X3DNode *serving = isTransform(c) ? c : parentServing;
    auto it = directChildren_.find(c);
    if (it != directChildren_.end()) {
      std::vector<const X3DNode *> kids(it->second.begin(), it->second.end());
      directChildren_.erase(it);
      for (const X3DNode *k : kids) {
        auto rc = refCount_.find(k);
        if (rc != refCount_.end() && rc->second > 1) { --rc->second; continue; }
        if (rc != refCount_.end()) refCount_.erase(rc);
        if (isTransform(k) && serving) removeEdgeChild(serving, k);
        eraseSubtree(k, serving);
      }
    }
    parent_.erase(c); world_.erase(c);
    nearestTransform_.erase(c); walked_.erase(c); refCount_.erase(c);
  }

  // Re-walk ONLY the direct children of grouping node `g` whose set changed: new
  // children are indexed (their whole subtree), removed children are dropped (if
  // no other parent still references them), and unchanged children are left
  // untouched. Returns true when the index changed.
  bool reindexChildren(const X3DNode *g, DirtyTracker &dirty) {
    if (!walked_.count(g)) return false; // g is not part of the indexed graph
    std::unordered_set<const X3DNode *> now;
    forEachChildNode(*g, [&](const FieldInfo &, const std::shared_ptr<X3DNode> &c) {
      if (c) now.insert(c.get());
    });
    auto oldIt = directChildren_.find(g);
    std::vector<const X3DNode *> removed, added;
    if (oldIt != directChildren_.end())
      for (const X3DNode *c : oldIt->second)
        if (!now.count(c)) removed.push_back(c);
    for (const X3DNode *c : now)
      if (oldIt == directChildren_.end() || !oldIt->second.count(c)) added.push_back(c);
    if (removed.empty() && added.empty()) return false;

    const X3DNode *serving = servingTransformOf(g);
    for (const X3DNode *c : removed) dropEdge(g, c);
    for (const X3DNode *c : added) {
      walk(c, g, serving);
      dirty.markDirty(c, DirtyWorldTransform);
    }
    directChildren_[g] = std::move(now);
    return true;
  }

  // Re-point every Transform whose canonical parent edge was dropped this tick
  // but that another parent still reaches. The new canonical parent is chosen by
  // the same first-path DFS buildIndex uses (roots in scene order, then child
  // fields in declaration order), so the outcome does not depend on the order the
  // dirty grouping nodes were processed. Returns true when a world changed.
  bool repairCanonicalParents(DirtyTracker &dirty) {
    bool changed = false;
    for (const X3DNode *n : staleCanonical_) {
      if (!parent_.count(n)) continue; // erased by the drop (last reference)
      const X3DNode *serving = nullptr;
      if (!firstPathServing(n, serving)) continue; // unreachable; leave as-is
      if (parentOf(n) == serving) continue;        // canonical edge still live
      parent_[n] = serving;
      std::unordered_set<const X3DNode *> visited;
      recompute(n, serving, serving ? worldTransform(serving) : Mat4::identity(),
                dirty, visited);
      changed = true;
    }
    return changed;
  }

  // First DFS path from the buildIndex roots reaching `n`, in the same field
  // order walk() recurses. `servingOut` receives the nearest ancestor Transform
  // on that path (nullptr = n is a root or has no Transform ancestor).
  bool firstPathServing(const X3DNode *n, const X3DNode *&servingOut) const {
    std::unordered_set<const X3DNode *> seen;
    for (const X3DNode *r : roots_) {
      if (r == n) { servingOut = nullptr; return true; }
      if (firstPathWalk(r, nullptr, n, servingOut, seen)) return true;
    }
    return false;
  }

  bool firstPathWalk(const X3DNode *cur, const X3DNode *serving,
                     const X3DNode *target, const X3DNode *&servingOut,
                     std::unordered_set<const X3DNode *> &seen) const {
    const X3DNode *next = isTransform(cur) ? cur : serving;
    if (!seen.insert(cur).second) return false; // subtree already recursed
    bool found = false;
    forEachChildNode(*cur, [&](const FieldInfo &, const std::shared_ptr<X3DNode> &c) {
      if (found || !c) return;
      if (c.get() == target) { servingOut = next; found = true; return; }
      if (firstPathWalk(c.get(), next, target, servingOut, seen)) found = true;
    });
    return found;
  }

  std::unordered_map<const X3DNode *, const X3DNode *> parent_;
  std::unordered_map<const X3DNode *, std::vector<const X3DNode *>> children_;
  std::unordered_map<const X3DNode *, Mat4> world_;
  std::unordered_set<const X3DNode *> walked_;   // subtrees already recursed (cycle/sharing guard)
  // For non-Transform nodes visited during walk: the nearest ancestor Transform
  // (NULL = no ancestor). Powers worldTransformAny() so non-Transform targets
  // (TransformSensor.targetObject, navigation pick results) resolve via the
  // Transform ancestor rather than the first reachable root-path.
  std::unordered_map<const X3DNode *, const X3DNode *> nearestTransform_;
  // Structural index for DirtyChildren re-walking: each containing node's direct
  // node children (any type) as last walked, plus a per-node count of the
  // distinct parent edges reaching it (drop only when it reaches zero).
  std::unordered_map<const X3DNode *, std::unordered_set<const X3DNode *>> directChildren_;
  std::unordered_map<const X3DNode *, std::size_t> refCount_;
  // Scene roots in authored order — anchors the first-path DFS that re-derives a
  // stale canonical parent deterministically (see repairCanonicalParents).
  std::vector<const X3DNode *> roots_;
  // Scratch: Transforms whose canonical parent edge was dropped this tick while
  // another parent still reaches them.
  std::vector<const X3DNode *> staleCanonical_;
  std::uint64_t revision_ = 0;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_TRANSFORM_SYSTEM_HPP
