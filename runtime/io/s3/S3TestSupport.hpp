// runtime/io/s3/S3TestSupport.hpp — test-only S3 provisioning helper for the
// AssetResolver swap-test (U3 / ADR-0023). AWS-SDK-free signatures.
//
// WHY THIS EXISTS: the swap-test must seed a fixture bucket so Backend B
// (S3Resolver) has something to fetch. Seeding needs the AWS SDK — but if the
// swap-test TU calls the SDK directly it becomes a SECOND translation unit
// where the AWS SDK appears, and two AWS TUs can disagree on headers (e.g. a
// system-installed SDK vs the linked one), which corrupts ClientConfiguration
// at runtime. So the SDK stays confined to S3TestSupport.cpp (built by
// x3d_s3_testsupport with the AWS SDK linked PRIVATE), exactly as the
// production read path is confined to S3Resolver.cpp. The swap-test sees only
// the std-typed signatures below and never includes <aws/...> — mirroring how
// Backend A's in-process HTTP fixture server is libcurl-free.
#ifndef X3D_RUNTIME_IO_S3_S3_TEST_SUPPORT_HPP
#define X3D_RUNTIME_IO_S3_S3_TEST_SUPPORT_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace x3d::runtime::io::s3 {

/// Ensure `bucket` exists at the S3-compatible `endpoint` (idempotent: a
/// bucket that already exists is treated as success). Credentials come from
/// the standard AWS environment variables. Returns false only on a real
/// failure (e.g. the endpoint is unreachable).
bool s3EnsureBucket(const std::string &endpoint, const std::string &bucket);

/// PUT one object (`bytes` under `key`) into `bucket` at `endpoint`. Returns
/// true on success. The bucket must already exist (see s3EnsureBucket).
bool s3PutObject(const std::string &endpoint, const std::string &bucket,
                 const std::string &key,
                 const std::vector<std::uint8_t> &bytes);

}  // namespace x3d::runtime::io::s3

#endif  // X3D_RUNTIME_IO_S3_S3_TEST_SUPPORT_HPP
