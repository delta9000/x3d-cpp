// runtime/ext/ExternalGeometryResolver.hpp
// Lazy materialization resolver for the ExternalGeometry extension node (Round 2).
// Namespace: x3d::runtime::ext. Header-only. Part of the runtime/ext/ quarantine
// (x3d_cpp_ext target, default OFF, gated by X3D_CPP_BUILD_EXT).
//
// One-way dep: ext->core. Core (x3d_cpp, sdk.hpp) MUST NEVER include this file.
//
// This file builds the higher-level (X3DNode -> PackedMesh) seam value that an
// embedder assigns to MeshBuildOptions::externalGeometryResolver. It bridges the
// FIREWALL: the extractor only ever sees the core-typed
//   std::function<PackedMesh(const X3DNode*, AssetResolver)>
// callback; the ext-specific knowledge (the ExternalGeometry node, the STL codec)
// lives entirely inside the closure built here.
//
// Lazy contract (mirrors the texture AssetResolver / Pending model):
//   - reads the node's url via the typed ExternalGeometry::getUrl() accessor
//     (after a dynamic_cast confirms the node is an ExternalGeometry leaf);
//   - sniffs format by extension (.stl -> binary STL; else unhandled -> empty);
//   - fetches bytes through the embedder's byte-level AssetResolver with
//     AssetKind::ExternalGeometry;
//   - not-ready / empty bytes -> empty PackedMesh => SceneExtractor treats it as
//     Pending and skips this tick (it will retry on the next extract);
//   - on ready bytes -> parseStlBinary -> PackedMesh;
//   - CACHES the resulting PackedMesh keyed by url (owned in the closure via a
//     shared_ptr<map>), so a second extract of the same node reuses the mesh
//     without re-fetching bytes or re-parsing.
//
// Empty PackedMesh (vertex_count == 0) is the universal Pending/skip/unhandled
// signal back to the extractor.
#ifndef X3D_RUNTIME_EXT_EXTERNAL_GEOMETRY_RESOLVER_HPP
#define X3D_RUNTIME_EXT_EXTERNAL_GEOMETRY_RESOLVER_HPP

#include "ExternalGeometry.hpp"      // x3d::runtime::ext::ExternalGeometry
#include "codecs/StlReader.hpp"      // x3d::runtime::ext::parseStlBinary
#include "AssetResolver.hpp"         // x3d::runtime::extract::{AssetResolver, AssetKind, AssetResult}
#include "PackedMesh.hpp"            // x3d::runtime::extract::PackedMesh

#include <algorithm>
#include <cctype>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace x3d::runtime::ext {

// ---------------------------------------------------------------------------
// detail — small helpers (lower-case extension test).
// ---------------------------------------------------------------------------
namespace egr_detail {

inline std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// Returns true if `url` ends (case-insensitively) in `ext` (e.g. ".stl").
inline bool hasExtension(const std::string& url, const std::string& ext) {
    if (url.size() < ext.size()) return false;
    return toLower(url.substr(url.size() - ext.size())) == ext;
}

} // namespace egr_detail

// ---------------------------------------------------------------------------
// makeExternalGeometryResolver — factory.
//
// Returns a std::function<PackedMesh(const X3DNode*, AssetResolver)> suitable for
// MeshBuildOptions::externalGeometryResolver. `bytesResolver` is the embedder's
// byte-level resolver (url, kind) -> AssetResult; it is captured and used to
// fetch the raw mesh bytes. (The AssetResolver the extractor passes at the call
// site is currently a default-constructed placeholder, so the resolver fetched
// here is the captured `bytesResolver`.)
//
// CACHE: the returned closure owns a shared_ptr to a url->PackedMesh map. A
// successful parse is cached; a subsequent extract of a node with the same url
// returns the cached copy WITHOUT calling `bytesResolver` again. Pending results
// (empty bytes) are NOT cached (so a later tick can still resolve them).
// ---------------------------------------------------------------------------
inline std::function<x3d::runtime::extract::PackedMesh(
    const X3DNode*, x3d::runtime::extract::AssetResolver)>
makeExternalGeometryResolver(x3d::runtime::extract::AssetResolver bytesResolver) {
    using x3d::runtime::extract::AssetKind;
    using x3d::runtime::extract::AssetResolver;
    using x3d::runtime::extract::AssetResult;
    using x3d::runtime::extract::PackedMesh;

    // Owned, closure-local cache — shared across copies of the std::function.
    auto cache = std::make_shared<std::unordered_map<std::string, PackedMesh>>();

    return [cache, bytesResolver](const X3DNode* node,
                                  AssetResolver /*siteResolver*/) -> PackedMesh {
        // (1) Confirm the node is an ExternalGeometry. Anything else is unhandled.
        if (!node) return PackedMesh{};
        if (node->nodeTypeName() != "ExternalGeometry") return PackedMesh{};
        const auto* ext = dynamic_cast<const ExternalGeometry*>(node);
        if (!ext) return PackedMesh{};

        // Read the url list (MFString = vector<string>). Use the first non-empty.
        const MFString urls = ext->getUrl();
        std::string url;
        for (const auto& u : urls) {
            if (!u.empty()) { url = u; break; }
        }
        if (url.empty()) return PackedMesh{}; // nothing to resolve.

        // (5) Cache hit — reuse without re-fetch/re-parse.
        if (auto it = cache->find(url); it != cache->end()) {
            return it->second;
        }

        // (2) Sniff format by extension. Only binary STL is handled at Round 2.
        if (!egr_detail::hasExtension(url, ".stl")) {
            // Unhandled format — return empty (skip), do not cache (a future
            // codec might handle it once added).
            return PackedMesh{};
        }

        // (3) Fetch bytes via the embedder's byte-level resolver. No resolver
        // wired => cannot fetch => Pending.
        if (!bytesResolver) return PackedMesh{};
        AssetResult res = bytesResolver(url, AssetKind::ExternalGeometry);
        if (!res.ready() || res.bytes.empty()) {
            // Pending or Failed — return empty (skip this tick). Not cached so a
            // later extract can retry once the asset becomes ready.
            return PackedMesh{};
        }

        // (4) Parse the bytes into a PackedMesh.
        PackedMesh mesh = parseStlBinary(res.bytes);
        if (mesh.empty()) {
            // Malformed STL — empty mesh. Treat as unhandled/skip; do not cache.
            return PackedMesh{};
        }

        // (5) Cache and return.
        (*cache)[url] = mesh;
        return mesh;
    };
}

} // namespace x3d::runtime::ext
#endif // X3D_RUNTIME_EXT_EXTERNAL_GEOMETRY_RESOLVER_HPP
