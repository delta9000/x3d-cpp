// runtime/io/s3/S3TestSupport.cpp — the single test-support TU where the AWS
// SDK meets the swap-test's fixture-seeding need. Linked into x3d_s3_testsupport
// with the AWS SDK PRIVATE, so the AWS SDK is confined here (mirrors the
// S3Resolver.cpp / QuickJsBackend.cpp isolation discipline). See
// S3TestSupport.hpp for why the swap-test must not include <aws/...> itself.
#include "S3TestSupport.hpp"

#include <aws/core/Aws.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/PutObjectRequest.h>

#include <mutex>
#include <string>

namespace x3d::runtime::io::s3 {

namespace {

// Aws::InitAPI must run once before any client is constructed. Done lazily on
// first use; never shut down (process exit cleans up). This is a separate
// once_flag from S3Resolver.cpp's — InitAPI tolerates being called once from
// each, matching the long-lived-process use case.
void ensureAwsInit() {
  static std::once_flag flag;
  std::call_once(flag, []() { Aws::InitAPI(Aws::SDKOptions{}); });
}

Aws::S3::S3Client makeClient(const std::string &endpoint) {
  Aws::S3::S3ClientConfiguration cfg;
  cfg.endpointOverride = endpoint;
  cfg.scheme = Aws::Http::Scheme::HTTP;
  cfg.useVirtualAddressing = false;  // path-style for minio
  cfg.requestTimeoutMs = 30000;
  return Aws::S3::S3Client(cfg);
}

}  // namespace

bool s3EnsureBucket(const std::string &endpoint, const std::string &bucket) {
  ensureAwsInit();
  auto s3 = makeClient(endpoint);
  Aws::S3::Model::CreateBucketRequest req;
  req.SetBucket(bucket.c_str());
  auto out = s3.CreateBucket(req);
  if (out.IsSuccess()) return true;
  // A bucket that already exists is success for our idempotent intent; any
  // other error (e.g. endpoint unreachable) is a real failure.
  const auto name = out.GetError().GetExceptionName();
  return name == "BucketAlreadyOwnedByYou" || name == "BucketAlreadyExists";
}

bool s3PutObject(const std::string &endpoint, const std::string &bucket,
                 const std::string &key,
                 const std::vector<std::uint8_t> &bytes) {
  ensureAwsInit();
  auto s3 = makeClient(endpoint);
  Aws::S3::Model::PutObjectRequest req;
  req.SetBucket(bucket.c_str());
  req.SetKey(key.c_str());
  auto data = Aws::MakeShared<Aws::StringStream>("x3d-s3-testsupport");
  data->write(reinterpret_cast<const char *>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
  req.SetBody(data);
  return s3.PutObject(req).IsSuccess();
}

}  // namespace x3d::runtime::io::s3
