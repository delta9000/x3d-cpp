// Sever containment back-edges while preserving shared DAG paths.
// Path-based walks cannot use a global visited set.
// See docs/wiki/explanation/decisions/0016-cycle-breaker.md.
#ifndef X3D_RUNTIME_CYCLE_BREAKER_HPP
#define X3D_RUNTIME_CYCLE_BREAKER_HPP

#include "x3d/nodes/X3DNode.hpp"
#include "x3d/core/X3DReflection.hpp"
#include "X3DScene.hpp"

#include <any>
#include <functional>
#include <memory>
#include <unordered_set>
#include <vector>

namespace x3d::runtime {
using namespace x3d::core;

// Sever every containment back-edge so `scene` becomes a DAG. Returns the number
// of edges removed (0 for any well-formed scene -> a no-op on valid content).
inline int breakContainmentCycles(Scene &scene) {
  std::unordered_set<const X3DNode *> onStack;  // gray: on the current DFS path
  std::unordered_set<const X3DNode *> done;     // black: subtree fully validated
  int severed = 0;

  std::function<void(X3DNode *)> dfs = [&](X3DNode *n) {
    if (!n) return;
    onStack.insert(n);
    for (const auto &f : n->fields()) {
      if (!f.get || !f.set) continue;  // need read and write to re-seat the field
      if (f.type == X3DFieldType::SFNode) {
        auto c = std::any_cast<std::shared_ptr<X3DNode>>(f.get(*n));
        if (!c) continue;
        if (onStack.count(c.get())) {        // back-edge -> sever
          f.set(*n, std::any(std::shared_ptr<X3DNode>{}));
          ++severed;
        } else if (!done.count(c.get())) {
          dfs(c.get());
        }
      } else if (f.type == X3DFieldType::MFNode) {
        auto v = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*n));
        std::vector<std::shared_ptr<X3DNode>> kept;
        kept.reserve(v.size());
        bool changed = false;
        for (const auto &c : v) {
          if (c && onStack.count(c.get())) {  // back-edge -> drop this child
            ++severed;
            changed = true;
            continue;
          }
          kept.push_back(c);
        }
        if (changed) f.set(*n, std::any(kept));
        for (const auto &c : kept)
          if (c && !done.count(c.get())) dfs(c.get());
      }
    }
    onStack.erase(n);
    done.insert(n);
  };

  for (const auto &root : scene.rootNodes)
    if (root) dfs(root.get());
  return severed;
}

}  // namespace x3d::runtime
#endif  // X3D_RUNTIME_CYCLE_BREAKER_HPP
