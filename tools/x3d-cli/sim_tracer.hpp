// tools/x3d-cli/sim_tracer.hpp
// ─────────────────────────────────────────────────────────────────────────────
// Field-change tracer for `x3d sim` (design Unit 3).
//
// TRACER MECHANISM — snapshot-diff (NOT a cascade observer).
//   The X3DExecutionContext exposes exactly one per-field delivery observer slot
//   (cascade_.setFieldObserver), and buildSceneGraph() already binds it to
//   classifyDirty(), which feeds the DirtyTracker → transform/bounds propagation.
//   Hooking it for tracing would either evict classifyDirty (breaking the
//   dirty-driven world-transform/bounds update) or require chaining that is not
//   exposed. So this tracer takes the design's documented FALLBACK: a robust
//   snapshot-diff. Each tick it snapshots every readable, NON-node field value
//   (via reflection fields()), diffs against the previous tick's snapshot, and
//   emits the (node, field, newValue) deltas. Sensor/interpolator outputs
//   (value_changed, fraction_changed, enterTime, isActive, …) are inputOutput or
//   outputOnly fields whose stored value the cascade writes, so they surface
//   naturally — no special-casing. This is clean (touches no core state) and
//   deterministic (iteration order is fixed; see below).
//
// NODE NAMING — deterministic.
//   • DEF-named nodes use their DEF name (from Scene::defs).
//   • Un-DEF'd nodes get a synthesized stable id "<Type>#<index>", where <index>
//     is the node's position in a FIXED document-order DFS over rootNodes (the
//     same blind SFNode/MFNode descent the bridge uses), counting per-type. The
//     walk is order-stable across runs, so ids are reproducible.
// ─────────────────────────────────────────────────────────────────────────────
#ifndef X3D_SIM_TRACER_HPP
#define X3D_SIM_TRACER_HPP

#include "FieldValueIO.hpp"      // x3d::codec::formatValue
#include "x3d/nodes/X3DNode.hpp"
#include "x3d/core/X3DReflection.hpp"
#include "X3DScene.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace x3d::sim {

/// One traced field change in a tick.
struct FieldChange {
  std::string node;   ///< DEF name or "<Type>#<index>"
  std::string field;  ///< X3D field name
  std::string value;  ///< formatValue() of the new value
};

/// All changes observed in one tick.
struct TickTrace {
  int tick = 0;
  double t = 0.0;
  std::vector<FieldChange> changes;
};

/// Captures per-tick field-value snapshots and diffs them into FieldChanges.
/// Construct over a Scene (fixes the node set + names), then call snapshot()
/// once before ticking (baseline) and diff(tick, t) after each ctx.tick().
class FieldTracer {
public:
  /// `watch`: if non-empty, restrict tracing to these "Node.field" specs;
  /// otherwise trace every readable non-node field (broad mode).
  explicit FieldTracer(const x3d::runtime::Scene &scene,
                       std::vector<std::string> watch = {}) {
    buildNodeIndex(scene);
    for (auto &w : watch) watch_.insert(std::move(w));
  }

  /// Take the baseline snapshot (call once, before the first tick).
  void baseline() { capture(prev_); }

  /// Diff the current field values against the previous snapshot; returns the
  /// changes and advances the baseline. Deterministic ordering: nodes in
  /// document order, fields in alphabetical order (std::map key order).
  TickTrace diff(int tick, double t) {
    TickTrace tr;
    tr.tick = tick;
    tr.t = t;
    Snapshot cur;
    capture(cur);
    for (const NodeRec &rec : nodes_) {
      auto curIt = cur.find(rec.node);
      if (curIt == cur.end()) continue;
      auto prevIt = prev_.find(rec.node);
      for (const auto &fv : curIt->second) {
        if (prevIt != prev_.end()) {
          auto pf = prevIt->second.find(fv.first);
          if (pf != prevIt->second.end() && pf->second == fv.second)
            continue;  // unchanged
        }
        if (!watch_.empty() &&
            watch_.find(rec.name + "." + fv.first) == watch_.end())
          continue;
        tr.changes.push_back({rec.name, fv.first, fv.second});
      }
    }
    prev_ = std::move(cur);
    return tr;
  }

private:
  struct NodeRec {
    const X3DNode *node;
    std::string name;
  };
  // node ptr -> (fieldName -> formatted value)
  using Snapshot =
      std::unordered_map<const X3DNode *,
                         std::map<std::string, std::string>>;

  void capture(Snapshot &snap) const {
    snap.clear();
    for (const NodeRec &rec : nodes_) {
      auto &m = snap[rec.node];
      for (const auto &f : rec.node->fields()) {
        if (f.isNode() || !f.isReadable()) continue;
        if (f.isEnum()) {
          m[f.x3dName] = f.getEnumString ? f.getEnumString(*rec.node) : "";
          continue;
        }
        m[f.x3dName] = safeFormat(f, *rec.node);
      }
    }
  }

  static std::string safeFormat(const FieldInfo &f,
                                const X3DNode &n) {
    try {
      return x3d::codec::formatValue(f.type, f.get(n));
    } catch (...) {
      return "";  // never let a bad-any-cast crash the tracer
    }
  }

  // Document-order DFS over the scene, assigning DEF names or "<Type>#<idx>".
  void buildNodeIndex(const x3d::runtime::Scene &scene) {
    // node ptr -> DEF name: iterate defs in sorted key order so the
    // lexicographically smallest DEF name wins for nodes with multiple names.
    // (scene.defs is an unordered_map; iteration order is nondeterministic, so
    // we must not use emplace/first-insert-wins directly.)
    std::unordered_map<const X3DNode *, std::string> defOf;
    {
      std::vector<std::pair<std::string, const X3DNode *>> sorted;
      sorted.reserve(scene.defs.size());
      for (const auto &kv : scene.defs)
        if (kv.second) sorted.emplace_back(kv.first, kv.second.get());
      // Sort ascending then emplace (first-insert-wins) → the lexicographically
      // smallest DEF name wins for nodes with multiple DEF aliases.
      std::sort(sorted.begin(), sorted.end(),
                [](const auto &a, const auto &b) { return a.first < b.first; });
      for (const auto &kv : sorted)
        defOf.emplace(kv.second, kv.first);  // smallest key wins
    }

    std::unordered_map<std::string, int> typeCounter;
    std::unordered_set<const X3DNode *> seen;
    std::function<void(const X3DNode *)> rec =
        [&](const X3DNode *n) {
          if (!n || !seen.insert(n).second) return;
          NodeRec r;
          r.node = n;
          auto dIt = defOf.find(n);
          if (dIt != defOf.end()) {
            r.name = dIt->second;
          } else {
            const std::string ty = n->nodeTypeName();
            r.name = ty + "#" + std::to_string(typeCounter[ty]++);
          }
          nodes_.push_back(std::move(r));
          for (const auto &f : n->fields()) {
            if (!f.get) continue;
            if (f.type == X3DFieldType::SFNode) {
              // Use pointer-form any_cast: returns nullptr on type mismatch
              // instead of throwing, so a malformed/unexpected field is silently
              // skipped rather than crashing the tracer.
              std::any v = f.get(*n);
              const auto *p =
                  std::any_cast<std::shared_ptr<X3DNode>>(&v);
              if (p && *p) rec(p->get());
            } else if (f.type == X3DFieldType::MFNode) {
              std::any v = f.get(*n);
              const auto *p =
                  std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(&v);
              if (p)
                for (const auto &c : *p)
                  if (c) rec(c.get());
            }
          }
        };
    for (const auto &root : scene.rootNodes) rec(root.get());
  }

  std::vector<NodeRec> nodes_;
  Snapshot prev_;
  std::unordered_set<std::string> watch_;
};

}  // namespace x3d::sim

#endif  // X3D_SIM_TRACER_HPP
