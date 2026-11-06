// runtime/ext/ExtResolver.hpp
// Factory for the ExternalGeometry EXTERNPROTO ProtoDeclaration + the install()
// resolver seam. Namespace: x3d::runtime::ext.
// Part of the runtime/ext/ quarantine (x3d_cpp_ext target, default OFF).
// Core (x3d_cpp, sdk.hpp) must never include this file.
//
// Usage:
//   auto resolver = x3d::runtime::ext::install();  // or install(myBaseResolver)
//   auto doc = x3d::codec::parseDocument(text, Encoding::XML, "", resolver);
//
// The returned resolver intercepts "urn:x3d-cpp-gen:ext:ExternalGeometry" in the
// url list and returns a factory ProtoDeclaration whose body is an ExternalGeometry
// node with IS-wired interface fields. All other urls are delegated to `base`.
//
// Each returned resolver owns a fresh declaration and its explicit clone factory.
// Opting one resolver into this extension never changes another world's cloning.
#ifndef X3D_RUNTIME_EXT_RESOLVER_HPP
#define X3D_RUNTIME_EXT_RESOLVER_HPP

#include "ExternalGeometry.hpp"     // x3d::runtime::ext::ExternalGeometry
#include "X3DProto.hpp"             // ProtoDeclaration, ProtoField, IsConnection, ProtoBody
#include "X3DParse.hpp"             // x3d::codec::localFileProtoResolver, ProtoDeclarationResolver
#include "X3DProtoClone.hpp"        // explicit CloneContext

#include <memory>
#include <string>
#include <vector>

namespace x3d::runtime::ext {

/// The URN that identifies the ExternalGeometry native implementation.
inline constexpr const char* kExternalGeometryUrn =
    "urn:x3d-cpp-gen:ext:ExternalGeometry";

/// Return a fresh declaration for one resolver. The body wires interface fields
/// through IS connections; its native node is cloned with resolver-local services.
inline std::shared_ptr<x3d::runtime::ProtoDeclaration>
makeExternalGeometryProto() {
    auto decl = std::make_shared<x3d::runtime::ProtoDeclaration>();
    decl->name = "ExternalGeometry";
    decl->createNode = [](const std::string &type) -> std::shared_ptr<X3DNode> {
        return type == "ExternalGeometry" ? std::make_shared<ExternalGeometry>() : nullptr;
    };

    auto body_node = std::make_shared<ExternalGeometry>();
    decl->body.nodes.push_back(body_node);
    for (const auto& info : body_node->fields()) {
        if (info.access != AccessType::InitializeOnly) continue;
        x3d::runtime::ProtoField f;
        f.name   = info.x3dName;
        f.type   = info.type;
        f.access = info.access;
        f.value  = info.get(*body_node);
        decl->interface.push_back(std::move(f));

        x3d::runtime::IsConnection is;
        is.node       = body_node;
        is.nodeField  = info.x3dName;
        is.protoField = info.x3dName;
        decl->body.isConnections.push_back(std::move(is));
    }

    return decl;
}

/// Returns a ProtoDeclarationResolver that intercepts the ExternalGeometry URN
/// and returns the factory ProtoDeclaration; delegates everything else to `base`.
///
/// Resolver-local declaration and clone services; there is no global installation.
inline x3d::codec::ProtoDeclarationResolver
install(x3d::codec::ProtoDeclarationResolver base =
            x3d::codec::localFileProtoResolver) {
    auto decl = makeExternalGeometryProto();

    return [base, decl](const std::vector<std::string>& urls,
                        const std::string& baseUrl)
        -> std::shared_ptr<x3d::runtime::ProtoDeclaration> {
        // Check if any url matches our URN.
        for (const auto& u : urls) {
            if (u == kExternalGeometryUrn) return decl;
        }
        // Otherwise delegate to the base resolver (file-local or embedder override).
        if (base) return base(urls, baseUrl);
        return nullptr;
    };
}

} // namespace x3d::runtime::ext
#endif // X3D_RUNTIME_EXT_RESOLVER_HPP
