// runtime/parse/X3DProtoResolver.hpp
#ifndef X3D_PARSE_PROTO_RESOLVER_HPP
#define X3D_PARSE_PROTO_RESOLVER_HPP

#include "X3DProto.hpp"   // ProtoDeclaration

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace x3d::codec {

/// Resolve an EXTERNPROTO declaration from its `url` list (relative to
/// `baseUrl`). Returns the matched ProtoDeclaration, or null if no candidate
/// resolves (e.g. http/urn with no embedder override). Implementations must
/// not throw on a missing/unreachable url — return null and let the caller
/// record a ProtoWarning (lenient-read policy).
using ProtoDeclarationResolver =
    std::function<std::shared_ptr<x3d::runtime::ProtoDeclaration>(
        const std::vector<std::string> &urls, const std::string &baseUrl)>;

/// Default no-op resolver: resolves nothing. Used when expansion runs without a
/// configured resolver (local PROTOs still expand; EXTERN instances warn).
inline std::shared_ptr<x3d::runtime::ProtoDeclaration>
noopProtoResolver(const std::vector<std::string> &, const std::string &) {
  return nullptr;
}

} // namespace x3d::codec

#endif // X3D_PARSE_PROTO_RESOLVER_HPP
