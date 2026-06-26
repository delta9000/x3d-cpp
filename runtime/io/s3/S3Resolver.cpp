// runtime/io/s3/S3Resolver.cpp — AWS C++ SDK S3 backend for the
// AssetResolver seam. The single TU where the AWS SDK meets the seam
// (mirrors the QuickJsBackend.cpp / HttpResolver.cpp isolation discipline).
#include "S3Resolver.hpp"

#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>

#include <cstdint>
#include <istream>
#include <iterator>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace x3d::runtime::io::s3 {

namespace {

bool isS3Url(const std::string &url) { return url.rfind("s3://", 0) == 0; }

// Parse s3://bucket/key[?...] -> (bucket, key). Returns empty strings on
// malformed input.
std::pair<std::string, std::string> parseS3Url(const std::string &url) {
  std::string rest = url.substr(5);  // strip "s3://"
  // Strip query string if present (S3 signed URLs and similar).
  if (auto q = rest.find('?'); q != std::string::npos) rest = rest.substr(0, q);
  auto slash = rest.find('/');
  if (slash == std::string::npos) return {"", ""};
  return {rest.substr(0, slash), rest.substr(slash + 1)};
}

// Lazy AWS SDK init — Aws::InitAPI must run exactly once before any SDK
// client is constructed. We do it on the first resolver call and never
// shut down (process exit cleans up; matching the long-lived-app use case).
void ensureAwsInit() {
  static std::once_flag flag;
  std::call_once(flag, []() { Aws::InitAPI(Aws::SDKOptions{}); });
}

}  // namespace

x3d::runtime::extract::AssetResolver makeS3Resolver(
    const std::string &endpoint) {
  return [endpoint](const std::string &url,
                    x3d::runtime::extract::AssetKind /*kind*/)
      -> x3d::runtime::extract::AssetResult {
    if (!isS3Url(url)) {
      return x3d::runtime::extract::AssetResult::makeFailed();
    }

    auto [bucket, key] = parseS3Url(url);
    if (bucket.empty() || key.empty()) {
      return x3d::runtime::extract::AssetResult::makeFailed();
    }

    ensureAwsInit();

    Aws::S3::S3ClientConfiguration cfg;
    cfg.endpointOverride = endpoint;
    cfg.scheme = Aws::Http::Scheme::HTTP;
    cfg.useVirtualAddressing = false;
    // The minio fixture + local CI run accepts anonymous access; for real
    // AWS the embedder wires a credentials provider chain before constructing
    // the resolver. We don't force credentials here — the endpoint policy
    // dictates auth.
    cfg.requestTimeoutMs = 30000;

    Aws::S3::S3Client s3(cfg);
    Aws::S3::Model::GetObjectRequest req;
    req.SetBucket(bucket.c_str());
    req.SetKey(key.c_str());

    auto outcome = s3.GetObject(req);
    if (!outcome.IsSuccess()) {
      return x3d::runtime::extract::AssetResult::makeFailed();
    }

    auto &body_stream = outcome.GetResultWithOwnership().GetBody();
    std::vector<std::uint8_t> bytes(
        (std::istreambuf_iterator<char>(body_stream)),
        std::istreambuf_iterator<char>{});
    if (bytes.empty()) {
      return x3d::runtime::extract::AssetResult::makeFailed();
    }
    return x3d::runtime::extract::AssetResult::makeReady(std::move(bytes));
  };
}

}  // namespace x3d::runtime::io::s3
