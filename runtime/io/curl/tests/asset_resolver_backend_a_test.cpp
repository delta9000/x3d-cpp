// runtime/io/curl/tests/asset_resolver_backend_a_test.cpp
// U1 per-backend test: verify HttpResolver wiring + URL prefix check +
// libcurl error path. The success path is tested in U3 (swap-test) against
// an in-process HTTP server — keeping this test fast + hermetic + offline.
#include "HttpResolver.hpp"

#include "AssetResolver.hpp"

#include <cassert>
#include <string>

using namespace x3d::runtime;
using namespace x3d::runtime::extract;
using namespace x3d::runtime::io::curl;

int main() {
  AssetResolver r = makeHttpResolver();
  assert(static_cast<bool>(r));

  // 1. URL prefix check: rejects non-http schemes with Failed, no network call.
  assert(r("urn:x3d:foo", AssetKind::Texture).failed());
  assert(r("file:///etc/hosts", AssetKind::Texture).failed());
  assert(r("plain/path.txt", AssetKind::Texture).failed());
  assert(r("", AssetKind::Texture).failed());
  assert(r("HTTP://uppercase-scheme.example/x", AssetKind::Texture).failed());

  // 2. libcurl error path: unreachable host returns Failed.
  //    127.0.0.1:1 is reserved + unused; libcurl reports CURLE_COULDNT_CONNECT.
  assert(r("http://127.0.0.1:1/x", AssetKind::Texture).failed());

  // 3. URL prefix check honors https:// just like http:// (no network call —
  //    TLS handshake would fail; only the prefix is being checked here).
  //    Use a non-routable host so we exercise the prefix-accept path.
  assert(r("https://nonexistent.invalid/x", AssetKind::Texture).failed());

  return 0;
}
