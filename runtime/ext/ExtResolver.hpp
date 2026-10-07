// runtime/ext/ExtResolver.hpp
// Factory for the ExternalGeometry EXTERNPROTO ProtoDeclaration + the install()
// resolver seam. Namespace: x3d::runtime::ext.
// Part of the runtime/ext/ quarantine (x3d_cpp_ext target, default OFF).
// Core (x3d_cpp, sdk.hpp) MUST NEVER include this file.
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

/// Build the ProtoDeclaration that backs ExternalGeometry instances.
/// A fresh declaration for one resolver. Structure:
///
///   PROTO ExternalGeometry [
///     initializeOnly MFString  url         []
///     initializeOnly SFVec3f   bboxCenter  0 0 0
///     initializeOnly SFVec3f   bboxSize    -1 -1 -1
///     initializeOnly SFString  contentType ""
///   ] {
///     ExternalGeometry { url IS url; bboxCenter IS bboxCenter; ... }
///   }
///
/// The IS connections are represented as entries in body.isConnections:
///   { node=body_node, nodeField="url",         protoField="url" }
///   { node=body_node, nodeField="bboxCenter",  protoField="bboxCenter" }
///   { node=body_node, nodeField="bboxSize",    protoField="bboxSize" }
///   { node=body_node, nodeField="contentType", protoField="contentType" }
///
/// expandInstance uses cloneMap[is.node.get()] to find the clone and forwards
/// each fieldValue override through findField(clone, is.nodeField)->set().
inline std::shared_ptr<x3d::runtime::ProtoDeclaration>
makeExternalGeometryProto() {
    auto decl = std::make_shared<x3d::runtime::ProtoDeclaration>();
    decl->name = "ExternalGeometry";
    decl->createNode = [](const std::string &type) -> std::shared_ptr<X3DNode> {
        return type == "ExternalGeometry" ? std::make_shared<ExternalGeometry>() : nullptr;
    };

    // ── Interface fields ─────────────────────────────────────────────────
    {
        x3d::runtime::ProtoField f;
        f.name   = "url";
        f.type   = X3DFieldType::MFString;
        f.access = AccessType::InitializeOnly;
        f.value  = std::any(MFString{});
        decl->interface.push_back(std::move(f));
    }
    {
        x3d::runtime::ProtoField f;
        f.name   = "bboxCenter";
        f.type   = X3DFieldType::SFVec3f;
        f.access = AccessType::InitializeOnly;
        f.value  = std::any(SFVec3f{0.0f, 0.0f, 0.0f});
        decl->interface.push_back(std::move(f));
    }
    {
        x3d::runtime::ProtoField f;
        f.name   = "bboxSize";
        f.type   = X3DFieldType::SFVec3f;
        f.access = AccessType::InitializeOnly;
        f.value  = std::any(SFVec3f{-1.0f, -1.0f, -1.0f});
        decl->interface.push_back(std::move(f));
    }
    {
        x3d::runtime::ProtoField f;
        f.name   = "contentType";
        f.type   = X3DFieldType::SFString;
        f.access = AccessType::InitializeOnly;
        f.value  = std::any(SFString{});
        decl->interface.push_back(std::move(f));
    }

    // ── Body: one ExternalGeometry node ─────────────────────────────────
    auto body_node = std::make_shared<ExternalGeometry>();
    decl->body.nodes.push_back(body_node);

    // ── IS connections: each interface field → the matching body field ──
    // expandInstance uses body.isConnections to forward fieldValues from the
    // ProtoInstance onto the cloned body node (cloneMap[body_node.get()]).
    auto add_is = [&](const std::string& field) {
        x3d::runtime::IsConnection is;
        is.node       = body_node;  // the original body node (cloneMap key)
        is.nodeField  = field;
        is.protoField = field;
        decl->body.isConnections.push_back(std::move(is));
    };
    add_is("url");
    add_is("bboxCenter");
    add_is("bboxSize");
    add_is("contentType");

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
