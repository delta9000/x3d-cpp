// AssetInlineResolver.hpp — Inline loading through an explicit asset policy.
#ifndef X3D_PARSE_ASSET_INLINE_RESOLVER_HPP
#define X3D_PARSE_ASSET_INLINE_RESOLVER_HPP

#include "AssetDocumentResolvers.hpp"

namespace x3d::codec {

/// Build an Inline resolver using the same asset policy for nested Inline and
/// EXTERNPROTO documents, without any implicit local-file fallback.
inline runtime::InlineResolver inlineResolverFrom(
    runtime::extract::AssetResolver resolver, Encoding hint = Encoding::Unknown) {
  return assetResolversFrom(std::move(resolver), hint).inlineScene;
}

/// Preserve a separate EXTERNPROTO policy inside all fetched Inline documents.
/// An empty callback rejects EXTERNPROTO loads rather than selecting a default.
inline runtime::InlineResolver inlineResolverFrom(
    runtime::extract::AssetResolver resolver, Encoding hint,
    ProtoDeclarationResolver protoPolicy) {
  if (!protoPolicy) protoPolicy = noopProtoResolver;
  return asset_document_detail::makeResolvers(
      {std::move(resolver), hint, std::move(protoPolicy), {}}).inlineScene;
}

} // namespace x3d::codec
#endif // X3D_PARSE_ASSET_INLINE_RESOLVER_HPP
