// runtime/io/s3/tests/asset_resolver_backend_b_test.cpp
// U2 per-backend test: verify S3Resolver wiring + URL prefix check +
// URL parse + SDK init. The success path is tested in U3 (swap-test)
// against a docker minio fixture, keeping this test fast + hermetic + offline.
#include "S3Resolver.hpp"

#include "AssetResolver.hpp"

#include <cassert>
#include <string>

using namespace x3d::runtime;
using namespace x3d::runtime::extract;
using namespace x3d::runtime::io::s3;

int main() {
  AssetResolver r = makeS3Resolver("http://localhost:9000");
  assert(static_cast<bool>(r));

  // 1. URL prefix check: rejects non-s3 schemes with Failed, no network call.
  assert(r("http://example.com/x", AssetKind::Texture).failed());
  assert(r("https://example.com/x", AssetKind::Texture).failed());
  assert(r("file:///etc/hosts", AssetKind::Texture).failed());
  assert(r("urn:x3d:foo", AssetKind::Texture).failed());
  assert(r("plain/path.txt", AssetKind::Texture).failed());
  assert(r("", AssetKind::Texture).failed());

  // 2. URL parse: malformed s3:// (no slash / empty key) returns Failed.
  assert(r("s3://bucket-only", AssetKind::Texture).failed());
  assert(r("s3:///empty-key", AssetKind::Texture).failed());

  // 3. SDK init path: a well-formed s3 URL with no bucket reachable returns
  //    Failed (the S3 client tries to connect and reports an error).
  //    127.0.0.1:1 is reserved + unused; AWS SDK will fail to connect.
  assert(r("s3://bucket/key", AssetKind::Texture).failed());

  return 0;
}
