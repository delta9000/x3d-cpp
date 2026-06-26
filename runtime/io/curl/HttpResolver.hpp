// runtime/io/curl/HttpResolver.hpp — Backend A (libcurl HTTP) for the
// AssetResolver seam (Phase-1 genericity pilot). Part of the runtime/io/curl
// quarantine (x3d_curl target, default OFF). Core (x3d_cpp, sdk.hpp) MUST
// NEVER include this file.
#ifndef X3D_RUNTIME_IO_CURL_HTTP_RESOLVER_HPP
#define X3D_RUNTIME_IO_CURL_HTTP_RESOLVER_HPP

#include "AssetResolver.hpp"   // x3d::runtime::extract::AssetResolver

namespace x3d::runtime::io::curl {

/// Returns an AssetResolver that fetches bytes via libcurl over http(s).
///
/// Synchronous: blocks on curl_easy_perform until response headers + body
/// arrive, then returns AssetResult::makeReady(bytes) on a 2xx, Failed
/// otherwise. Pending is NOT supported (deferred-bytes is a follow-up).
///
/// Honors only http:// and https:// URL schemes; returns Failed for everything
/// else (urn:, file:, missing scheme) — embedder override territory.
///
/// Thread-safe per-call (curl_easy_init is reentrant); callers that need
/// throughput should construct one and reuse it on the calling thread, not
/// share across threads.
x3d::runtime::extract::AssetResolver makeHttpResolver();

}  // namespace x3d::runtime::io::curl

#endif  // X3D_RUNTIME_IO_CURL_HTTP_RESOLVER_HPP
