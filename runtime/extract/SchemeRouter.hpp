// runtime/extract/SchemeRouter.hpp
//
// makeSchemeRouter — compose per-scheme AssetResolver backends into one seam.
//
// THE RULE (mirroring makeMultiFormatTextureResolver, ADR-0024 §7): route by a
// cheap key BEFORE calling a backend, never use Failed to route. AssetResolver
// backends self-identify by URL scheme — HttpResolver owns http(s)://,
// S3Resolver owns s3://, a local-file resolver owns relative/path-less urls —
// and each returns Failed for everything else. Without a router, "wrong route"
// and "right route, genuinely failed" are both Failed and a consumer stacking
// backends cannot tell them apart. This composer dispatches by the URL's scheme
// (RFC 3986 §3.1, case-insensitive) so each backend only ever sees urls it
// owns and its Failed stays unambiguous. The frozen Ready/Pending/Failed seam
// type is unchanged.
//
// std-only and IO-free: this header never opens a socket, file or stream — it
// only compares a lexical prefix and calls a std::function. Unknown scheme ->
// Failed WITHOUT calling any backend (the router never guesses). A url with no
// scheme (relative / scheme-less) goes to the optional `fallback`, or Failed
// when none was supplied. A Windows drive path ("C:\models\a.x3d") is treated as
// scheme-less (its one-letter "scheme" is a drive designator) and so takes the
// fallback — see urlScheme.
#ifndef X3D_RUNTIME_EXTRACT_SCHEME_ROUTER_HPP
#define X3D_RUNTIME_EXTRACT_SCHEME_ROUTER_HPP

#include "AssetResolver.hpp"

#include <cctype>
#include <map>
#include <optional>
#include <string>

namespace x3d::runtime::extract {

namespace scheme_router_detail {

// ASCII lowercase copy (scheme matching is case-insensitive per RFC 3986).
inline std::string lowerAscii(std::string s) {
  for (char &c : s)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

// RFC 3986 §3.1: scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." ) ":". If
// `url` opens with a scheme, return it lowercased; otherwise nullopt (a
// scheme-less / relative url).
//
// Windows drive designators ("C:\models\a.x3d", "C:/models/a.x3d", "c:a.x3d")
// lexically parse as a one-letter scheme. No routed backend owns a one-letter
// scheme, so any single ASCII letter before ':' is treated as a drive path, not
// a scheme, and flows to the scheme-less fallback (the local-file resolver);
// otherwise it would be misrouted to Failed. Conservative by design: a
// hypothetical one-letter scheme cannot be routed through this composer.
inline std::optional<std::string> urlScheme(const std::string &url) {
  if (url.empty() || std::isalpha(static_cast<unsigned char>(url[0])) == 0)
    return std::nullopt;
  for (std::size_t i = 1; i < url.size(); ++i) {
    const unsigned char c = static_cast<unsigned char>(url[i]);
    if (c == ':') {
      if (i == 1)
        return std::nullopt; // single letter + ':' -> Windows drive, not a scheme
      return lowerAscii(url.substr(0, i));
    }
    if (std::isalnum(c) != 0 || c == '+' || c == '-' || c == '.')
      continue;
    return std::nullopt; // illegal scheme char before ':' -> not a scheme
  }
  return std::nullopt; // no ':' at all -> scheme-less
}

// Normalize a routing key so "http", "http:", and "http://" all match "http"
// (case-insensitively).
inline std::string normalizeSchemeKey(std::string key) {
  key = lowerAscii(std::move(key));
  if (key.size() >= 3 && key.compare(key.size() - 3, 3, "://") == 0)
    key.resize(key.size() - 3);
  else if (!key.empty() && key.back() == ':')
    key.pop_back();
  return key;
}

} // namespace scheme_router_detail

// Build one AssetResolver from `backends`, keyed by scheme name ("http",
// "https", "s3", "file", "urn", ...; a trailing ':'/'://' is tolerated and the
// key is matched case-insensitively). A url whose scheme matches a registered
// backend is handed to that backend verbatim; any other scheme yields Failed
// without calling anything. A scheme-less / relative url goes to `fallback`
// when one was supplied (the local-file resolver is the usual choice), else
// Failed.
inline AssetResolver makeSchemeRouter(std::map<std::string, AssetResolver> backends,
                                      AssetResolver fallback = nullptr) {
  std::map<std::string, AssetResolver> routes;
  for (auto &entry : backends)
    routes[scheme_router_detail::normalizeSchemeKey(entry.first)] =
        std::move(entry.second);

  return [routes = std::move(routes), fallback = std::move(fallback)](
             const std::string &url, AssetKind kind) -> AssetResult {
    const std::optional<std::string> scheme =
        scheme_router_detail::urlScheme(url);
    if (!scheme) {
      if (fallback)
        return fallback(url, kind);
      return AssetResult::makeFailed();
    }
    const auto it = routes.find(*scheme);
    if (it == routes.end() || !it->second)
      return AssetResult::makeFailed();
    return it->second(url, kind);
  };
}

} // namespace x3d::runtime::extract

#endif // X3D_RUNTIME_EXTRACT_SCHEME_ROUTER_HPP
