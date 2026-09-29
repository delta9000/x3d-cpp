#ifndef X3D_RUNTIME_PROTO_FIELD_ORDER_HPP
#define X3D_RUNTIME_PROTO_FIELD_ORDER_HPP

#include "X3DProto.hpp"
#include "x3d/nodes/X3DNode.hpp"

#include <unordered_map>

namespace x3d::runtime {

inline std::string canonicalNodeSlot(const X3DNode &parent,
                                     const std::string &slot) {
  for (const auto &field : parent.fields())
    if (field.isNode() &&
        (field.x3dName == slot || field.containerField == slot))
      return field.x3dName;
  return slot;
}

// Reconcile the authored sequence with the current reflected child fields.
// Node/instance values are authoritative; declarations have no field value and
// remain in their authored positions even when nobody refers to them.
inline std::vector<ProtoBodyStatement>
orderedNodeStatements(const ProtoBody &body,
                      const std::shared_ptr<X3DNode> &parent) {
  std::vector<ProtoBodyStatement> live;
  if (!parent) return live;
  for (const auto &field : parent->fields()) {
    if (!field.isReadable() || !field.isNode() || !field.get) continue;
    const std::string slot = field.x3dName;
    std::any value = field.get(*parent);
    if (field.type == X3DFieldType::SFNode) {
      if (auto child = std::any_cast<std::shared_ptr<X3DNode>>(value))
        live.push_back({ProtoBodyStatement::Kind::Node, child, 0, {}, {}, slot});
    } else if (field.type == X3DFieldType::MFNode) {
      for (const auto &child :
           std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(value))
        if (child)
          live.push_back({ProtoBodyStatement::Kind::Node, child, 0, {}, {}, slot});
    }
  }
  for (std::size_t i = 0; i < body.nestedInstances.size(); ++i) {
    const auto &instance = body.nestedInstances[i];
    if (instance.parent.lock() != parent) continue;
    const std::string authoredSlot = instance.parentField.empty()
                                 ? instance.containerField : instance.parentField;
    const std::string slot = canonicalNodeSlot(*parent, authoredSlot);
    live.push_back({ProtoBodyStatement::Kind::Instance, {}, i, {}, {}, slot});
  }

  auto it = body.nodeStatements.find(std::weak_ptr<X3DNode>(parent));
  if (it == body.nodeStatements.end()) return live;
  std::vector<ProtoBodyStatement> result;
  std::vector<bool> used(live.size(), false);
  for (const auto &statement : it->second) {
    if (statement.kind == ProtoBodyStatement::Kind::Proto) {
      if (statement.proto) result.push_back(statement);
      continue;
    }
    if (statement.kind == ProtoBodyStatement::Kind::ExternProto) {
      if (statement.externProto) result.push_back(statement);
      continue;
    }
    const std::string slot = canonicalNodeSlot(*parent, statement.field);
    for (std::size_t i = 0; i < live.size(); ++i) {
      if (used[i] || live[i].kind != statement.kind ||
          live[i].field != slot) continue;
      if (statement.kind == ProtoBodyStatement::Kind::Node &&
          live[i].node != statement.node) continue;
      if (statement.kind == ProtoBodyStatement::Kind::Instance &&
          live[i].instanceIndex != statement.instanceIndex) continue;
      used[i] = true;
      result.push_back(live[i]);
      break;
    }
  }
  for (std::size_t i = 0; i < live.size(); ++i)
    if (!used[i]) result.push_back(live[i]);

  // The ledger fixes statement positions, but the reflected field owns the
  // current order of node values within each slot. Refill only node positions
  // so declarations and nested instances keep their authored anchors.
  std::unordered_map<std::string, std::size_t> nextNode;
  for (auto &statement : result) {
    if (statement.kind != ProtoBodyStatement::Kind::Node) continue;
    auto &next = nextNode[statement.field];
    for (; next < live.size(); ++next) {
      if (live[next].kind != ProtoBodyStatement::Kind::Node ||
          live[next].field != statement.field) continue;
      statement = live[next++];
      break;
    }
  }
  return result;
}

} // namespace x3d::runtime
#endif
