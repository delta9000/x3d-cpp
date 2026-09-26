// runtime/io/curl/HttpResolver.hpp — Backend A (libcurl HTTP) for the
// AssetResolver seam (Phase-1 genericity pilot). Part of the runtime/io/curl
// quarantine (x3d_curl target, default OFF). Core (x3d_cpp, sdk.hpp) MUST
// NEVER include this file.
#ifndef X3D_RUNTIME_IO_CURL_HTTP_RESOLVER_HPP
#define X3D_RUNTIME_IO_CURL_HTTP_RESOLVER_HPP

#include "AssetResolver.hpp"   // x3d::runtime::extract::AssetResolver

#include <cstdint>

namespace x3d::runtime::io::curl {

/// Configuration for makeHttpResolver. The defaults are the **hardened** ones
/// (SEC-6): private/loopback/link-local destinations are refused and a response
/// body cannot exceed maxBytes. Pass an instance only to widen for local dev.
struct HttpResolverOptions {
  /// When false (default) the resolver refuses to connect to a **resolved**
  /// loopback, link-local (169.254/16, fe80::/10), RFC1918, CGNAT (100.64/10),
  /// unique-local (fc00::/7) or unspecified (0.0.0.0/::) address, and also
  /// classifies IPv4 addresses embedded in IPv4-mapped (::ffff:a.b.c.d),
  /// IPv4-compatible (::a.b.c.d) and NAT64 (64:ff9b::/96) IPv6 forms. The check
  /// runs on the resolved sockaddr (POST-DNS, per connection), so a hostname
  /// or redirect cannot smuggle a private address past it. Environment proxies
  /// (http_proxy/https_proxy/all_proxy) are disabled while this is false, so the
  /// check cannot be defeated by routing the request through a proxy that
  /// libcurl—not the guard—connects to. Set true to talk to a local server
  /// (tests, local dev only); this also re-enables environment proxies.
  bool allowPrivateNetworks = false;
  /// Maximum number of redirects to follow (CURLOPT_MAXREDIRS).
  long maxRedirects = 5;
  /// Hard ceiling on response-body bytes; a transfer that exceeds it is aborted
  /// and reported Failed. Also set as a CURLOPT_MAXFILESIZE_LARGE hint.
  std::uint64_t maxBytes = 256ull * 1024 * 1024;  // 256 MiB
  /// Whole-transfer timeout in seconds (CURLOPT_TIMEOUT).
  long timeoutSeconds = 30;
};

/// Returns an AssetResolver that fetches bytes via libcurl over http(s).
///
/// Synchronous: blocks on curl_easy_perform until response headers + body
/// arrive, then returns AssetResult::makeReady(bytes) on a 2xx, Failed
/// otherwise. Pending is NOT supported (deferred-bytes is a follow-up).
///
/// Honors only http:// and https:// URL schemes (both for the request and for
/// redirects); returns Failed for everything else (urn:, file:, missing
/// scheme) — embedder override territory.
///
/// SEC-6 hardening, on by default: protocol-restricted redirects, a redirect
/// cap, a response-size cap, and a resolved-address guard that blocks
/// private/loopback/link-local destinations. See HttpResolverOptions.
///
/// Thread-safe per-call (curl_easy_init is reentrant); callers that need
/// throughput should construct one and reuse it on the calling thread, not
/// share across threads.
x3d::runtime::extract::AssetResolver makeHttpResolver(
    HttpResolverOptions options = {});

}  // namespace x3d::runtime::io::curl

#endif  // X3D_RUNTIME_IO_CURL_HTTP_RESOLVER_HPP
