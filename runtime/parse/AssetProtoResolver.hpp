// runtime/parse/AssetProtoResolver.hpp
//
// protoResolverFrom — an EXTERNPROTO ProtoDeclarationResolver backed by the
// runtime/extract AssetResolver seam, so an embedder can plug the http (or s3,
// or any routed) backend into parse-time prototype expansion.
//
// The default localFileProtoResolver (X3DParse.hpp) is file-local and skips
// http(s):// and urn: as embedder-override territory. This adapter closes that
// gap: for each url in an EXTERNPROTO's url list (in declared order) it asks the
// AssetResolver for the document bytes, sniffs the encoding, parses the fetched
// document with the existing front door (parseDocument), and returns the
// matching ProtoDeclare — resolved by the '#ProtoName' fragment when present,
// else the document's first ProtoDeclare.
//
// Contract (B) parse-time (see AssetResolver.hpp): the fetched document must be
// Ready or Failed synchronously. Failed -> try the next url (lenient). Pending
// is INCOHERENT at parse time and is treated as a hard error: resolution stops
// and returns null. Never throws.
//
// urn: is neither special-cased nor skipped here — it is handed to the resolver
// like any other url, so it resolves only when the injected resolver (typically
// a makeSchemeRouter with a "urn" entry) owns that scheme; otherwise the
// resolver answers Failed and the candidate is skipped. This mirrors the
// default resolver, which skips urn:// when no embedder override exists.
//
// Cycle guard: a per-thread set of urls currently being resolved (the analogue
// of the default resolver's activeFiles) makes a self- or mutually-referencing
// EXTERNPROTO terminate — a re-entered url is skipped and answers no proto,
// rather than recursing without bound.
#ifndef X3D_PARSE_ASSET_PROTO_RESOLVER_HPP
#define X3D_PARSE_ASSET_PROTO_RESOLVER_HPP

#include "X3DParse.hpp"                  // parseDocument, sniff, Encoding
#include "X3DProtoResolver.hpp"          // ProtoDeclarationResolver
#include "../extract/AssetResolver.hpp"  // x3d::runtime::extract::AssetResolver

#include <algorithm>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace x3d::codec {

namespace asset_proto_detail {

// Bytes are still encoded text; copy into a std::string for the parser.
inline std::string bytesToText(const std::vector<std::uint8_t> &bytes) {
  return std::string(reinterpret_cast<const char *>(bytes.data()), bytes.size());
}

// A url carries an absolute scheme when it opens with "scheme://". (urn: has no
// "//", and urn documents do not carry relative references in practice.)
inline bool hasAuthorityScheme(const std::string &url) {
  return url.find("://") != std::string::npos;
}

// The base directory a fetched document's own relative references resolve
// against: an absolute url contributes its own directory; a relative url is
// first joined against the referencing document's `baseUrl`.
inline std::string documentBaseDir(const std::string &url,
                                   const std::string &baseUrl) {
  std::string full = url;
  if (!hasAuthorityScheme(url) && !baseUrl.empty() &&
      (url.empty() || url.front() != '/'))
    full = baseUrl + "/" + url;
  const std::size_t slash = full.find_last_of('/');
  return slash == std::string::npos ? std::string() : full.substr(0, slash);
}

// Cycle guard, mirroring the default localFileProtoResolver's thread_local
// activeFiles (X3DParse.cpp): a document whose own EXTERNPROTOs reference it,
// directly or through a chain, would otherwise re-fetch and re-parse itself
// without bound and overflow the stack. Keyed by the fragment-stripped url (the
// fetch identity), per-thread so concurrent parses do not interfere.
inline std::vector<std::string> &activeExternUrls() {
  static thread_local std::vector<std::string> active;
  return active;
}

struct ActiveUrlGuard {
  explicit ActiveUrlGuard(std::string url) : url_(std::move(url)) {
    activeExternUrls().push_back(url_);
  }
  ~ActiveUrlGuard() { activeExternUrls().pop_back(); }
  ActiveUrlGuard(const ActiveUrlGuard &) = delete;
  ActiveUrlGuard &operator=(const ActiveUrlGuard &) = delete;
  std::string url_;
};

// Recursive worker. A free function (not a self-capturing std::function) so the
// nested resolver can re-enter without a reference cycle.
inline std::shared_ptr<runtime::ProtoDeclaration>
resolveExternFromAsset(const runtime::extract::AssetResolver &resolver, Encoding hint,
                       const std::vector<std::string> &urls,
                       const std::string &baseUrl) {
  for (const std::string &raw : urls) {
    std::string url = raw;
    std::string fragment;
    if (const auto hash = url.find('#'); hash != std::string::npos) {
      fragment = url.substr(hash + 1);
      url.resize(hash);
    }

    const std::vector<std::string> &active = activeExternUrls();
    if (std::find(active.begin(), active.end(), url) != active.end())
      continue; // cycle: this url is already being resolved up the stack
    ActiveUrlGuard urlGuard(url);

    const runtime::extract::AssetResult result =
        resolver(url, runtime::extract::AssetKind::ExternProto);
    if (result.pending())
      return nullptr; // parse-time contract (B): Pending is a hard error
    if (!result.ready())
      continue; // Failed: try the next candidate url

    const std::string body = bytesToText(result.bytes);
    const Encoding enc = hint != Encoding::Unknown ? hint : sniff(url, body);
    if (enc == Encoding::Unknown)
      continue; // unclassifiable: skip this candidate

    const ProtoDeclarationResolver nested =
        [resolver, hint](const std::vector<std::string> &nestedUrls,
                         const std::string &nestedBase) {
          return resolveExternFromAsset(resolver, hint, nestedUrls, nestedBase);
        };

    std::shared_ptr<runtime::ProtoDeclaration> found;
    try {
      runtime::X3DDocument doc = parseDocument(
          body, enc, documentBaseDir(url, baseUrl), nested);
      if (!fragment.empty())
        found = doc.scene.findProto(fragment);
      else if (!doc.scene.protoDeclarations.empty())
        found = doc.scene.protoDeclarations.front();
    } catch (const std::exception &) {
      // lenient: fall through to the next candidate url
    }
    if (found)
      return found;
  }
  return nullptr;
}

} // namespace asset_proto_detail

/// Build a ProtoDeclarationResolver that fetches EXTERNPROTO documents through
/// `resolver` (contract B). `hint` forces an encoding for every fetched
/// document; the default Unknown sniffs each from its url + content.
inline ProtoDeclarationResolver protoResolverFrom(runtime::extract::AssetResolver resolver,
                                                  Encoding hint = Encoding::Unknown) {
  if (!resolver)
    resolver = runtime::extract::makeNullAssetResolver();
  return [resolver, hint](const std::vector<std::string> &urls,
                          const std::string &baseUrl) {
    return asset_proto_detail::resolveExternFromAsset(resolver, hint, urls,
                                                      baseUrl);
  };
}

} // namespace x3d::codec

#endif // X3D_PARSE_ASSET_PROTO_RESOLVER_HPP
