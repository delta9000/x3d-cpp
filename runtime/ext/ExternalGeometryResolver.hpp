// Lazy ExternalGeometry materialization. The extension depends on core.
// The resolver closure owns extension types and a URL-keyed mesh cache, while
// the extractor receives only core PackedMesh values. Failed, Pending, empty,
// or unsupported inputs return an empty mesh; extraction retries on later ticks.
#ifndef X3D_RUNTIME_EXT_EXTERNAL_GEOMETRY_RESOLVER_HPP
#define X3D_RUNTIME_EXT_EXTERNAL_GEOMETRY_RESOLVER_HPP

#include "ExternalGeometry.hpp"      // x3d::runtime::ext::ExternalGeometry
#include "codecs/StlReader.hpp"      // x3d::runtime::ext::parseStlBinary
#include "AssetResolver.hpp"         // x3d::runtime::extract::{AssetResolver, AssetKind, AssetResult}
#include "PackedMesh.hpp"            // x3d::runtime::extract::PackedMesh

#include <cctype>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace x3d::runtime::ext {

// Fetch through the captured bytesResolver, rather than the call-site resolver.
// Successful meshes are cached across copies of the closure; Pending results
// are not cached so extraction can retry.

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

        // Only binary STL is supported.
        if (url.size() < 4 || url[url.size() - 4] != '.' ||
            std::tolower(static_cast<unsigned char>(url[url.size() - 3])) != 's' ||
            std::tolower(static_cast<unsigned char>(url[url.size() - 2])) != 't' ||
            std::tolower(static_cast<unsigned char>(url[url.size() - 1])) != 'l') {
            // Unsupported formats return an empty mesh without caching.
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
