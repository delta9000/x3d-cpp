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
// The factory ProtoDeclaration is cached in a local static (thread-safe under C++11+
// static init rules); it is immutable after construction so the shared_ptr is safe
// to return to concurrent callers.
//
// fallbackNodeCreator() hook contract:
//   - Process-global, sticky (no uninstall), set-once opt-in slot.
//   - Intended to be set once at single-threaded setup time before any parsing.
//   - Concurrent install()-during-deepClone is NOT synchronized and NOT supported.
#ifndef X3D_RUNTIME_EXT_RESOLVER_HPP
#define X3D_RUNTIME_EXT_RESOLVER_HPP

#include "ExternalGeometry.hpp"     // x3d::runtime::ext::ExternalGeometry
#include "X3DProto.hpp"             // ProtoDeclaration, ProtoField, IsConnection, ProtoBody
#include "X3DParse.hpp"             // x3d::codec::localFileProtoResolver, ProtoDeclarationResolver
#include "X3DProtoClone.hpp"        // x3d::runtime::fallbackNodeCreator()

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace x3d::runtime::ext {

/// The URN that identifies the ExternalGeometry native implementation.
inline constexpr const char* kExternalGeometryUrn =
    "urn:x3d-cpp-gen:ext:ExternalGeometry";

/// Build the ProtoDeclaration that backs ExternalGeometry instances.
/// Called once; the result is cached in install(). Structure:
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
/// The factory decl is constructed once via a local static (C++11 thread-safe
/// static init) and then shared read-only. The fallbackNodeCreator() hook is
/// also set exactly once across all install() calls (guarded by std::once_flag).
/// `base` is captured by value; no other global state is written after setup.
///
/// Cached decl + body node immutability contract: decl->body.nodes[0] and the
/// decl itself are immutable after construction — deepClone copies per-expansion.
/// Never mutate them directly after makeExternalGeometryProto() returns.
inline x3d::codec::ProtoDeclarationResolver
install(x3d::codec::ProtoDeclarationResolver base =
            x3d::codec::localFileProtoResolver) {
    // Cache the decl once per process. The static is initialized on first call
    // (C++11 guarantees thread-safe static init).
    static const std::shared_ptr<x3d::runtime::ProtoDeclaration> s_decl =
        makeExternalGeometryProto();
    // Copy the shared_ptr by value into the lambda so the lambda captures it by value
    // (a local copy of the shared_ptr, not a ref to the static — suppresses the
    // "capture of variable with non-automatic storage duration" warning).
    std::shared_ptr<x3d::runtime::ProtoDeclaration> decl = s_decl;

    // Register the process-global fallback creator exactly once across all
    // install() calls. The hook is a sticky, set-once, opt-in slot — intended
    // to be configured at single-threaded setup time before parsing begins.
    // Concurrent install()-during-deepClone is not synchronized and not supported.
    static std::once_flag s_creator_flag;
    std::call_once(s_creator_flag, []() {
        x3d::runtime::fallbackNodeCreator() =
            [](const std::string& t) -> std::shared_ptr<X3DNode> {
                if (t == "ExternalGeometry")
                    return std::make_shared<ExternalGeometry>();
                return nullptr;
            };
    });

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
