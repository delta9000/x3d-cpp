#pragma once

#include "x3d/sdk.hpp"

#include <unordered_set>
#include <vector>

namespace x3d::cli_detail {

struct ProtoUse {
    std::unordered_set<const runtime::ProtoDeclaration *> local;
    std::unordered_set<const runtime::ExternProtoDeclaration *> external;
};

// Inspect authored instances, including templates in locally authored PROTO
// bodies. An EXTERNPROTO instance can also carry its resolved implementation;
// its extern declaration is the declaration used in this document.
inline ProtoUse collectProtoUse(const sdk::X3DDocument &doc) {
    ProtoUse used;
    const auto &scene = doc.scene;
    std::vector<const runtime::ProtoDeclaration *> pending;
    const auto recordInstance = [&](const runtime::ProtoInstance &instance) {
        if (instance.externDeclaration) {
            used.external.insert(instance.externDeclaration.get());
        } else if (instance.declaration) {
            used.local.insert(instance.declaration.get());
            pending.push_back(instance.declaration.get());
        }
    };
    for (const auto &instance : scene.protoInstances)
        recordInstance(instance);

    std::unordered_set<const x3d::nodes::X3DNode *> visitedNodes;
    const auto visitNode = [&](auto &&self,
                               const std::shared_ptr<x3d::nodes::X3DNode> &node) -> void {
        if (!node || !visitedNodes.insert(node.get()).second) return;
        if (auto wrapper = std::dynamic_pointer_cast<runtime::ProtoInstanceTemplate>(node)) {
            recordInstance(wrapper->instance);
            for (const auto &value : wrapper->instance.fieldValues)
                for (const auto &child : value.nodeValue) self(self, child);
        }
        for (const auto &field : node->fields()) {
            if (!field.isReadable() || !field.isNode() || !field.get) continue;
            auto value = field.get(*node);
            if (field.type == core::X3DFieldType::SFNode) {
                if (auto child = std::any_cast<std::shared_ptr<x3d::nodes::X3DNode>>(value))
                    self(self, child);
            } else if (field.type == core::X3DFieldType::MFNode) {
                for (const auto &child :
                     std::any_cast<std::vector<std::shared_ptr<x3d::nodes::X3DNode>>>(value))
                    self(self, child);
            }
        }
    };
    for (const auto &instance : scene.protoInstances)
        for (const auto &value : instance.fieldValues)
            for (const auto &node : value.nodeValue) visitNode(visitNode, node);

    std::unordered_set<const runtime::ProtoDeclaration *> visited;
    for (const auto &declaration : scene.protoDeclarations)
        if (declaration) pending.push_back(declaration.get());
    while (!pending.empty()) {
        const auto *declaration = pending.back();
        pending.pop_back();
        if (!visited.insert(declaration).second) continue;
        for (const auto &field : declaration->interface)
            for (const auto &node : field.nodeDefault) visitNode(visitNode, node);
        for (const auto &node : declaration->body.nodes) visitNode(visitNode, node);
        for (const auto &instance : declaration->body.nestedInstances)
        {
            recordInstance(instance);
            for (const auto &value : instance.fieldValues)
                for (const auto &node : value.nodeValue) visitNode(visitNode, node);
        }
        for (const auto &statement : declaration->body.statements)
            if (statement.kind == runtime::ProtoBodyStatement::Kind::Proto && statement.proto)
                pending.push_back(statement.proto.get());
        for (const auto &[parent, statements] : declaration->body.nodeStatements)
            if (!parent.expired())
                for (const auto &statement : statements)
                    if (statement.kind == runtime::ProtoBodyStatement::Kind::Proto && statement.proto)
                        pending.push_back(statement.proto.get());
    }
    return used;
}

} // namespace x3d::cli_detail
