// runtime/X3DProtoClone.hpp
#ifndef X3D_RUNTIME_PROTO_CLONE_HPP
#define X3D_RUNTIME_PROTO_CLONE_HPP

#include "x3d/nodes/X3DNode.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "x3d/core/X3DReflection.hpp"

#include <any>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace x3d::runtime {

/// A fallback creator for node types not registered in the generated
/// X3DNodeFactory (e.g. hand-written x3d::runtime::ext extension nodes).
/// Default: empty (no fallback). An opt-in extension (such as
/// x3d::runtime::ext::install()) sets this so deepClone() can clone its nodes
/// without any generated-layer edits.
///
/// Contract: process-global, sticky (no uninstall), set-once opt-in slot.
/// Must be set at single-threaded setup time before parsing begins.
/// Concurrent writes (install()-during-deepClone) are not synchronized.
using FallbackNodeCreator =
    std::function<std::shared_ptr<X3DNode>(const std::string &)>;

inline FallbackNodeCreator &fallbackNodeCreator() {
  static FallbackNodeCreator f;
  return f;
}

/// Deep-clone a node tree. `cloneMap` (original ptr -> clone) preserves
/// intra-tree DEF/USE shared identity: a node referenced twice clones once.
inline std::shared_ptr<X3DNode>
deepClone(const std::shared_ptr<X3DNode> &src,
          std::unordered_map<const X3DNode *, std::shared_ptr<X3DNode>> &cloneMap) {
  if (!src) return nullptr;
  auto it = cloneMap.find(src.get());
  if (it != cloneMap.end()) return it->second;       // USE: same clone

  std::shared_ptr<X3DNode> dst = X3DNodeFactory::create(src->nodeTypeName());
  // On factory miss, consult the opt-in ext-populated hook (only set when the
  // ext module is installed; absent by default — degrades gracefully to nullptr).
  if (!dst && fallbackNodeCreator())
    dst = fallbackNodeCreator()(src->nodeTypeName());
  if (!dst) return nullptr;                            // genuinely unknown type: drop
  cloneMap[src.get()] = dst;
  dst->setDEF(src->getDEF());

  for (const FieldInfo &f : src->fields()) {
    if (!f.get || !f.set) continue;                    // event-only/read-only
    if (f.type == X3DFieldType::SFNode) {
      auto child = std::any_cast<std::shared_ptr<X3DNode>>(f.get(*src));
      f.set(*dst, std::any(deepClone(child, cloneMap)));
    } else if (f.type == X3DFieldType::MFNode) {
      auto kids =
          std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*src));
      std::vector<std::shared_ptr<X3DNode>> out;
      out.reserve(kids.size());
      for (auto &k : kids) out.push_back(deepClone(k, cloneMap));
      f.set(*dst, std::any(std::move(out)));
    } else {
      f.set(*dst, f.get(*src));                         // scalar: copy boxed any
    }
  }
  return dst;
}

inline std::shared_ptr<X3DNode> deepClone(const std::shared_ptr<X3DNode> &src) {
  std::unordered_map<const X3DNode *, std::shared_ptr<X3DNode>> m;
  return deepClone(src, m);
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_PROTO_CLONE_HPP
