// Lazy ExternalGeometry materialization. The extension depends on core.
// The resolver closure owns extension types and a URL-keyed mesh cache, while
// the extractor receives only core ExternalGeometryResult values. Nodes other
// than ExternalGeometry and urls other than .stl are Unsupported; a missing
// resolver or a Pending fetch is Pending; a failed fetch or malformed STL is
// Failed. Only Pending retries on later ticks.
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

inline std::function<x3d::runtime::extract::ExternalGeometryResult(
    const X3DNode*, x3d::runtime::extract::AssetResolver)>
makeExternalGeometryResolver(x3d::runtime::extract::AssetResolver bytesResolver) {
    using x3d::runtime::extract::AssetKind;
    using x3d::runtime::extract::AssetResolver;
    using x3d::runtime::extract::AssetResult;
    using x3d::runtime::extract::ExternalGeometryResult;
    using x3d::runtime::extract::PackedMesh;

    // Owned, closure-local cache — shared across copies of the std::function.
    auto cache = std::make_shared<std::unordered_map<std::string, PackedMesh>>();

    return [cache, bytesResolver](const X3DNode* node,
                                  AssetResolver /*siteResolver*/) -> ExternalGeometryResult {
        // (1) Confirm the node is an ExternalGeometry. Anything else is unhandled.
        if (!node) return ExternalGeometryResult::unsupported();
        if (node->nodeTypeName() != "ExternalGeometry") return ExternalGeometryResult::unsupported();
        const auto* ext = dynamic_cast<const ExternalGeometry*>(node);
        if (!ext) return ExternalGeometryResult::unsupported();

        // Read the url list (MFString = vector<string>). Use the first non-empty.
        const MFString urls = ext->getUrl();
        std::string url;
        for (const auto& u : urls) {
            if (!u.empty()) { url = u; break; }
        }
        if (url.empty()) return ExternalGeometryResult::failed(); // nothing to resolve.

        // (5) Cache hit — reuse without re-fetch/re-parse.
        if (auto it = cache->find(url); it != cache->end()) {
            return it->second;
        }

        // Only binary STL is supported.
        if (url.size() < 4 || url[url.size() - 4] != '.' ||
            std::tolower(static_cast<unsigned char>(url[url.size() - 3])) != 's' ||
            std::tolower(static_cast<unsigned char>(url[url.size() - 2])) != 't' ||
            std::tolower(static_cast<unsigned char>(url[url.size() - 1])) != 'l') {
            // Not cached: a url edit re-asks.
            return ExternalGeometryResult::unsupported();
        }

        // (3) Fetch bytes via the embedder's byte-level resolver. No resolver
        // wired => cannot fetch => Pending.
        if (!bytesResolver) return ExternalGeometryResult::pending();
        AssetResult res = bytesResolver(url, AssetKind::ExternalGeometry);
        if (res.failed()) return ExternalGeometryResult::failed();
        if (!res.ready() || res.bytes.empty()) {
            // Pending — not cached so a later extract can retry once the
            // asset becomes ready.
            return ExternalGeometryResult::pending();
        }

        // (4) Parse the bytes into a PackedMesh.
        PackedMesh mesh = parseStlBinary(res.bytes);
        if (mesh.empty()) {
            // Malformed STL. Not cached: a url edit re-asks.
            return ExternalGeometryResult::failed();
        }

        // (5) Cache and return.
        (*cache)[url] = mesh;
        return mesh;
    };
}

} // namespace x3d::runtime::ext
#endif // X3D_RUNTIME_EXT_EXTERNAL_GEOMETRY_RESOLVER_HPP
