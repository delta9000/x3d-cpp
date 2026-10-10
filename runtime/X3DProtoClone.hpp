// runtime/X3DProtoClone.hpp
#ifndef X3D_RUNTIME_PROTO_CLONE_HPP
#define X3D_RUNTIME_PROTO_CLONE_HPP

#include "FieldRead.hpp"
#include "X3DProto.hpp"
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
using namespace x3d::core;

/// Explicit services for one clone transaction. With no stores, cloning is
/// generated-fields-only. Pass both stores to include author fields; their
/// lifetime is borrowed only for the synchronous call. No ambient factory.
struct CloneContext {
  const DynamicFieldStore *sourceFields = nullptr;
  DynamicFieldStore *destinationFields = nullptr;
  FallbackNodeCreator createNode;
};

/// Deep-clone a node tree. `cloneMap` (original ptr -> clone) preserves
/// intra-tree DEF/USE shared identity: a node referenced twice clones once.
inline std::shared_ptr<x3d::nodes::X3DNode>
deepClone(const std::shared_ptr<x3d::nodes::X3DNode> &src,
          std::unordered_map<const x3d::nodes::X3DNode *, std::shared_ptr<x3d::nodes::X3DNode>> &cloneMap,
          const CloneContext &context = {}) {
  if (!!context.sourceFields != !!context.destinationFields)
    throw std::invalid_argument("clone author fields require source and destination owners");
  if (!src) return nullptr;
  auto it = cloneMap.find(src.get());
  if (it != cloneMap.end()) return it->second;       // USE: same clone

  std::shared_ptr<x3d::nodes::X3DNode> dst = x3d::nodes::X3DNodeFactory::create(src->nodeTypeName());
  // A resolver may opt this declaration into extension cloning only.
  if (!dst && context.createNode)
    dst = context.createNode(src->nodeTypeName());
  if (!dst) return nullptr;                            // genuinely unknown type: drop
  cloneMap[src.get()] = dst;
  dst->setDEF(src->getDEF());

  for (const FieldInfo &f : src->fields()) {
    if (!f.get || !f.set) continue;                    // event-only/read-only
    if (f.type == X3DFieldType::SFNode) {
      FieldRef<std::shared_ptr<x3d::nodes::X3DNode>> child(*src, f);
      if (!child) continue;
      f.set(*dst, std::any(deepClone(*child, cloneMap, context)));
    } else if (f.type == X3DFieldType::MFNode) {
      // Borrowed: the walk never writes `src`, only the clones.
      FieldRef<std::vector<std::shared_ptr<x3d::nodes::X3DNode>>> kids(*src, f);
      if (!kids) continue;
      std::vector<std::shared_ptr<x3d::nodes::X3DNode>> out;
      out.reserve(kids->size());
      for (const auto &k : *kids) out.push_back(deepClone(k, cloneMap, context));
      f.set(*dst, std::any(std::move(out)));
    } else {
      f.set(*dst, f.get(*src));                         // scalar: copy boxed any
    }
  }
  if (context.sourceFields) {
    for (const FieldInfo &field : context.sourceFields->authorFields(*src)) {
      std::any value = context.sourceFields->getValue(*src, field.x3dName);
      if (value.has_value() && field.type == X3DFieldType::SFNode) {
        value = deepClone(std::any_cast<SFNode>(value), cloneMap, context);
      } else if (value.has_value() && field.type == X3DFieldType::MFNode) {
        MFNode copies;
        for (const auto &node : std::any_cast<const MFNode &>(value))
          copies.push_back(deepClone(node, cloneMap, context));
        value = std::move(copies);
      }
      context.destinationFields->addAuthorField(dst,
          {field.x3dName, field.type, field.access, value});
      // Preserve any already-stored event value, too. Registration seeds only
      // initializeOnly/inputOutput; direct store writes validate every type.
      if (value.has_value() && (field.access == AccessType::InputOnly ||
                                field.access == AccessType::OutputOnly))
        context.destinationFields->setValue(*dst, field.x3dName, std::move(value));
    }
  }
  return dst;
}

inline std::shared_ptr<x3d::nodes::X3DNode> deepClone(const std::shared_ptr<x3d::nodes::X3DNode> &src,
                                                               const CloneContext &context = {}) {
  std::unordered_map<const x3d::nodes::X3DNode *, std::shared_ptr<x3d::nodes::X3DNode>> m;
  return deepClone(src, m, context);
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_PROTO_CLONE_HPP
