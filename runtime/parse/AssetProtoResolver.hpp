// AssetProtoResolver.hpp — EXTERNPROTO loading through an explicit asset policy.
#ifndef X3D_PARSE_ASSET_PROTO_RESOLVER_HPP
#define X3D_PARSE_ASSET_PROTO_RESOLVER_HPP

#include "AssetDocumentResolvers.hpp"

namespace x3d::codec {

/// Build an EXTERNPROTO resolver using the same asset policy for nested
/// EXTERNPROTO and Inline documents. No nested parse enables local-file I/O.
/// `hint` forces the encoding; Unknown sniffs each fetched document separately.
inline ProtoDeclarationResolver protoResolverFrom(
    runtime::extract::AssetResolver resolver, Encoding hint = Encoding::Unknown) {
  return assetResolversFrom(std::move(resolver), hint).proto;
}

/// Explicitly retain an independent Inline policy in every fetched document.
/// An empty callback rejects Inline loads; it never selects the file default.
/// Callers passing a custom Inline callback to parseDocument should pass the
/// same callback here, or use assetResolversFrom when one asset policy suffices.
inline ProtoDeclarationResolver protoResolverFrom(
    runtime::extract::AssetResolver resolver, Encoding hint,
    runtime::InlineResolver inlinePolicy) {
  if (!inlinePolicy)
    inlinePolicy = [](const auto &, const auto &) -> std::shared_ptr<runtime::Scene> {
      return nullptr;
    };
  return asset_document_detail::makeResolvers(
      {std::move(resolver), hint, {}, std::move(inlinePolicy)}).proto;
}

} // namespace x3d::codec
#endif // X3D_PARSE_ASSET_PROTO_RESOLVER_HPP
