// TransformSystem.hpp — transform-hierarchy index + world-transform side table
// + incremental world-transform propagation. M2C-4: covers all static
// transform-bearing types (Transform, HAnimHumanoid, HAnimJoint, CADPart).
// Billboard is view-dependent (active Viewpoint) — deferred to M2c/M2d.
// World transforms are stored in a side table keyed by const X3DNode* (nothing
// on the node). namespace x3d::runtime.
#ifndef X3D_RUNTIME_TRANSFORM_SYSTEM_HPP
#define X3D_RUNTIME_TRANSFORM_SYSTEM_HPP

#include "DirtyTracker.hpp"
#include "Mat4.hpp"
#include "X3DNode.hpp"
#include "X3DScene.hpp"

#include <any>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace x3d::runtime {

class TransformSystem {
public:
  /// Build the Transform hierarchy index + initial world transforms from a Scene.
  void buildIndex(const Scene &scene) {
    parent_.clear(); children_.clear(); world_.clear(); walked_.clear();
    nearestTransform_.clear();
    for (const auto &root : scene.rootNodes)
      if (root) walk(root.get(), /*parentTransform=*/nullptr);
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

  /// Recompute world transforms for every dirtied subtree, marking each
  /// recomputed node DirtyWorldTransform. Only subtrees under a dirtied local
  /// transform are revisited (incremental).
  void propagate(DirtyTracker &dirty) {
    // Collect the dirtied Transform roots: nodes flagged DirtyLocalTransform with
    // NO ancestor also so flagged (the ancestor's subtree pass covers them).
    std::vector<const X3DNode *> roots;
    for (const X3DNode *n : dirty.changedNodes()) {
      if (!(dirty.flags(n) & DirtyLocalTransform)) continue;
      if (!parent_.count(n) && !world_.count(n)) continue; // not a known Transform
      bool ancestorDirty = false;
      for (const X3DNode *p = parentOf(n); p; p = parentOf(p))
        if (dirty.flags(p) & DirtyLocalTransform) { ancestorDirty = true; break; }
      if (!ancestorDirty) roots.push_back(n);
    }
    std::unordered_set<const X3DNode *> visited;
    for (const X3DNode *r : roots)
      recompute(r, worldTransform(parentOf(r)), dirty, visited);
  }

  // Read a transform-bearing node's local matrix from its TRS fields via
  // reflection. Public so BoundsSystem/PickSystem/LightSystem can reuse it.
  static Mat4 localMatrix(const X3DNode *n) {
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
      if (f.x3dName == name) return std::any_cast<SFVec3f>(f.get(*n));
    return SFVec3f{0, 0, 0};
  }
  static SFRotation getRot(const X3DNode *n, const std::string &name) {
    for (const auto &f : n->fields())
      if (f.x3dName == name) return std::any_cast<SFRotation>(f.get(*n));
    return SFRotation{0, 0, 1, 0};
  }

  // DFS the scene graph over node-typed fields; index Transforms + seed worlds.
  void walk(const X3DNode *n, const X3DNode *parentTransform) {
    const X3DNode *nextParent = parentTransform;
    if (isTransform(n)) {
      parent_[n] = parentTransform;
      if (parentTransform) children_[parentTransform].push_back(n);
      world_[n] = (parentTransform ? worldTransform(parentTransform) : Mat4::identity())
                  * localMatrix(n);
      nextParent = n;
    } else if (parentTransform) {
      // Non-Transform node inherits the nearest ancestor Transform; used by
      // worldTransformAny() to compute the world matrix of any node without
      // needing to walk down from roots (which can find a non-Transform path).
      nearestTransform_[n] = parentTransform;
    }
    // Recurse each subtree ONCE: a USE-shared node reachable by many paths must not
    // re-walk its subtree per path (multiplicative explosion / hang). world_ stays the
    // documented per-node (first-path) approximation; the extractor re-accumulates
    // per-path itself, so the render path is unaffected.
    if (!walked_.insert(n).second) return;
    for (const auto &f : n->fields()) {
      if (!f.get) continue; // inputOnly node fields (addChildren/...) have no getter
      if (f.type == X3DFieldType::SFNode) {
        auto c = std::any_cast<std::shared_ptr<X3DNode>>(f.get(*n));
        if (c) walk(c.get(), nextParent);
      } else if (f.type == X3DFieldType::MFNode) {
        for (const auto &c :
             std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*n)))
          if (c) walk(c.get(), nextParent);
      }
    }
  }

  // Recompute world = parentWorld * local for `n`, then recurse child Transforms.
  void recompute(const X3DNode *n, const Mat4 &parentWorld, DirtyTracker &dirty,
                 std::unordered_set<const X3DNode *> &visited) {
    if (!visited.insert(n).second) return;
    Mat4 w = parentWorld * localMatrix(n);
    world_[n] = w;
    dirty.markDirty(n, DirtyWorldTransform);
    auto it = children_.find(n);
    if (it == children_.end()) return;
    for (const X3DNode *c : it->second)
      recompute(c, w, dirty, visited);
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
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_TRANSFORM_SYSTEM_HPP
