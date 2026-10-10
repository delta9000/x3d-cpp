// AssetDocumentResolvers.hpp — synchronous, explicitly paired asset loaders.
#ifndef X3D_PARSE_ASSET_DOCUMENT_RESOLVERS_HPP
#define X3D_PARSE_ASSET_DOCUMENT_RESOLVERS_HPP

#include "X3DParse.hpp"
#include "../extract/AssetResolver.hpp"

#include <algorithm>
#include <cctype>
#include <exception>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace x3d::codec {

/// Pass both callbacks to parseDocument to keep one asset policy across Inline
/// and EXTERNPROTO boundaries. Neither callback falls back to local-file I/O.
struct AssetDocumentResolvers {
  ProtoDeclarationResolver proto;
  runtime::InlineResolver inlineScene;
};

namespace asset_document_detail {

inline std::size_t schemeEnd(const std::string &url) {
  if (url.empty() || !std::isalpha(static_cast<unsigned char>(url.front())))
    return std::string::npos;
  for (std::size_t i = 1; i < url.size(); ++i) {
    const unsigned char c = static_cast<unsigned char>(url[i]);
    if (c == ':') return i;
    if (!std::isalnum(c) && c != '+' && c != '-' && c != '.') break;
  }
  return std::string::npos;
}

// Normalize only URL path segments, never the authority, query or opaque URN.
// This is lexical URL handling; it performs no filesystem or network access.
inline std::string normalizeUrl(const std::string &url) {
  // A slash in a query/fragment is data, never the start of an authority's
  // path. Split those bytes before locating or normalizing path segments.
  const auto suffixStart = url.find_first_of("?#");
  const std::string address = url.substr(0, suffixStart);
  const auto scheme = schemeEnd(address);
  std::size_t pathStart = 0;
  if (scheme != std::string::npos) {
    if (address.compare(scheme + 1, 2, "//") != 0) return url;
    pathStart = address.find('/', scheme + 3);
    if (pathStart == std::string::npos) return url;
  } else if (address.rfind("//", 0) == 0) {
    pathStart = address.find('/', 2);
    if (pathStart == std::string::npos) return url;
  }
  const std::string path = address.substr(pathStart);
  const bool absolute = !path.empty() && path.front() == '/';
  const bool trailing = !path.empty() &&
      (path.back() == '/' || path == "." || path == ".." ||
       path.ends_with("/.") || path.ends_with("/.."));
  std::vector<std::string> parts;
  for (std::size_t i = 0; i <= path.size();) {
    const auto end = path.find('/', i);
    const std::string part = path.substr(i, end - i);
    if (part == "..") {
      if (!parts.empty() && parts.back() != "..") parts.pop_back();
      else if (!absolute) parts.push_back(part);
    } else if (part != "." && (!part.empty() || (i != 0 && end != std::string::npos))) {
      parts.push_back(part); // preserve significant interior empty URL segments
    }
    if (end == std::string::npos) break;
    i = end + 1;
  }
  std::string result = address.substr(0, pathStart);
  if (absolute) result += '/';
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i != 0) result += '/';
    result += parts[i];
  }
  if (trailing && !parts.empty()) result += '/';
  // A relative path such as child/.. still denotes the current directory.
  // Keeping ./ prevents a fetched directory from becoming a file-like base.
  if (trailing && result.empty()) result = "./";
  if (suffixStart != std::string::npos) result += url.substr(suffixStart);
  return result;
}

inline std::string resolveUrl(const std::string &url,
                              const std::string &baseDir) {
  if (schemeEnd(url) != std::string::npos || baseDir.empty())
    return normalizeUrl(url);
  const auto scheme = schemeEnd(baseDir);
  if (url.rfind("//", 0) == 0)
    return normalizeUrl(scheme == std::string::npos
                            ? url : baseDir.substr(0, scheme + 1) + url);
  if (!url.empty() && url.front() == '/') {
    const std::string baseAddress = baseDir.substr(0, baseDir.find_first_of("?#"));
    const auto authorityStart = scheme != std::string::npos &&
        baseAddress.compare(scheme + 1, 2, "//") == 0 ? scheme + 3
        : baseAddress.rfind("//", 0) == 0 ? std::size_t{2} : std::string::npos;
    if (authorityStart != std::string::npos) {
      const auto slash = baseAddress.find('/', authorityStart);
      return normalizeUrl(baseAddress.substr(0, slash) + url);
    }
    return normalizeUrl(url);
  }
  // An opaque source identifier has no relative directory hierarchy.
  if (scheme != std::string::npos &&
      baseDir.compare(scheme + 1, 2, "//") != 0) return normalizeUrl(url);
  return normalizeUrl(baseDir + (baseDir.back() == '/' ? "" : "/") + url);
}

inline std::string documentBaseDir(const std::string &url) {
  const std::string path = url.substr(0, url.find_first_of("?#"));
  const auto scheme = schemeEnd(path);
  if (scheme != std::string::npos) {
    if (path.compare(scheme + 1, 2, "//") != 0) return {};
    if (path.find('/', scheme + 3) == std::string::npos) return path;
  } else if (path.rfind("//", 0) == 0 && path.find('/', 2) == std::string::npos) {
    return path; // an authority-only network-path reference keeps its host
  }
  const auto slash = path.find_last_of('/');
  if (slash == std::string::npos) return {};
  return slash == 0 ? std::string("/") : path.substr(0, slash);
}

struct Policy {
  runtime::extract::AssetResolver assets;
  Encoding hint = Encoding::Unknown;
  // Optional opaque callbacks are copied unchanged through every nested parse.
  // Empty means use the corresponding asset-backed loader, never a file loader.
  ProtoDeclarationResolver protoOverride;
  runtime::InlineResolver inlineOverride;
};

struct Context {
  const Policy &policy;
  std::vector<std::string> active;
  bool pending = false;
};

struct ActiveGuard {
  std::vector<std::string> &active;
  ActiveGuard(std::vector<std::string> &a, const std::string &url) : active(a) {
    active.push_back(url);
  }
  ~ActiveGuard() { active.pop_back(); }
  ActiveGuard(const ActiveGuard &) = delete;
  ActiveGuard &operator=(const ActiveGuard &) = delete;
};

inline std::shared_ptr<runtime::ProtoDeclaration>
resolveProto(Context &, const std::vector<std::string> &, const std::string &);
inline std::shared_ptr<runtime::Scene>
resolveInline(Context &, const std::vector<std::string> &, const std::string &);

// All nested callbacks have an explicitly bounded synchronous lifetime. They
// refer to this invocation's context; no shared ownership cycle or hidden
// thread-local/global loader policy is needed.
inline AssetDocumentResolvers nestedResolvers(Context &context) {
  return {
      context.policy.protoOverride ? context.policy.protoOverride
          : ProtoDeclarationResolver{[&context](const auto &urls, const auto &base) {
              return resolveProto(context, urls, base);
            }},
      context.policy.inlineOverride ? context.policy.inlineOverride
          : runtime::InlineResolver{[&context](const auto &urls, const auto &base) {
              return resolveInline(context, urls, base);
            }}};
}

template <typename Result, typename Select>
std::shared_ptr<Result> resolveDocument(Context &context,
    const std::vector<std::string> &urls, const std::string &baseDir,
    runtime::extract::AssetKind kind, Select select) {
  if (context.pending || context.active.size() >= kMaxNestingDepth) return nullptr;
  for (const auto &raw : urls) {
    const auto hash = raw.find('#');
    const std::string fragment = hash == std::string::npos ? "" : raw.substr(hash + 1);
    const std::string candidate = raw.substr(0, hash);
    if (candidate.empty()) continue;
    const std::string url = resolveUrl(candidate, baseDir);
    if (std::find(context.active.begin(), context.active.end(), url) !=
        context.active.end()) continue;
    ActiveGuard guard(context.active, url);
    try {
      const auto result = context.policy.assets(url, kind);
      if (result.pending()) {
        context.pending = true; // hard failure also propagates through parent loads
        return nullptr;
      }
      if (!result.ready()) continue;
      const std::string body(result.bytes.begin(), result.bytes.end());
      const Encoding enc = context.policy.hint == Encoding::Unknown
          ? sniff(url, body) : context.policy.hint;
      if (enc == Encoding::Unknown) continue;
      const auto nested = nestedResolvers(context);
      auto doc = parseDocument(body, enc, documentBaseDir(url),
                               nested.proto, nested.inlineScene);
      if (context.pending) return nullptr;
      if (auto selected = select(std::move(doc), fragment)) return selected;
    } catch (const std::exception &) {
      // Missing/unparseable candidates and backend failures remain lenient.
      if (context.pending) return nullptr;
    }
  }
  return nullptr;
}

inline std::shared_ptr<runtime::ProtoDeclaration>
resolveProto(Context &context, const std::vector<std::string> &urls,
             const std::string &baseDir) {
  return resolveDocument<runtime::ProtoDeclaration>(context, urls, baseDir,
      runtime::extract::AssetKind::ExternProto,
      [](runtime::X3DDocument doc, const std::string &fragment) {
        if (!fragment.empty()) return doc.scene.findProto(fragment);
        return doc.scene.protoDeclarations.empty()
            ? std::shared_ptr<runtime::ProtoDeclaration>{}
            : doc.scene.protoDeclarations.front();
      });
}

inline std::shared_ptr<runtime::Scene>
resolveInline(Context &context, const std::vector<std::string> &urls,
              const std::string &baseDir) {
  return resolveDocument<runtime::Scene>(context, urls, baseDir,
      runtime::extract::AssetKind::Inline,
      [](runtime::X3DDocument doc, const std::string &) {
        return std::make_shared<runtime::Scene>(std::move(doc.scene));
      });
}

inline AssetDocumentResolvers makeResolvers(Policy policy) {
  if (!policy.assets) policy.assets = runtime::extract::makeNullAssetResolver();
  return {
      [policy](const auto &urls, const auto &base) {
        Context context{policy, {}, false};
        return resolveProto(context, urls, base);
      },
      [policy](const auto &urls, const auto &base) {
        Context context{policy, {}, false};
        return resolveInline(context, urls, base);
      }};
}

} // namespace asset_document_detail

/// Build both parse-time loaders from one synchronous asset policy. URLs sent
/// to the backend are fragment-free and resolved against the source directory.
/// Failed candidates are skipped; Pending stops the entire recursive load.
/// The policy is used for every nested Inline/EXTERNPROTO, with no I/O fallback.
inline AssetDocumentResolvers assetResolversFrom(
    runtime::extract::AssetResolver resolver, Encoding hint = Encoding::Unknown) {
  return asset_document_detail::makeResolvers({std::move(resolver), hint, {}, {}});
}

} // namespace x3d::codec
#endif // X3D_PARSE_ASSET_DOCUMENT_RESOLVERS_HPP
