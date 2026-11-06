// S3 fixture provisioning. Keep AWS headers out of fixture consumers to avoid
// SDK header/ABI mismatches; see ADR-0023.
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
