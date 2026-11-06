// S3 AssetResolver adapter. AWS headers stay private to the backend.
// See ADR-0023.
#ifndef X3D_RUNTIME_IO_S3_S3_RESOLVER_HPP
#define X3D_RUNTIME_IO_S3_S3_RESOLVER_HPP

#include "AssetResolver.hpp"   // x3d::runtime::extract::AssetResolver

#include <string>

namespace x3d::runtime::io::s3 {

/// Returns an AssetResolver that fetches bytes via the AWS C++ SDK S3
/// client. URL form: s3://<bucket>/<key>. Honors only the s3:// scheme;
/// returns Failed for everything else.
///
/// `endpoint` is the S3-compatible endpoint (default localhost:9000 for the
/// docker minio fixture). AWS SDK init/shutdown is handled lazily on first
/// call via std::once_flag — embedder does not need to call
/// Aws::InitAPI/ShutdownAPI explicitly (but may; idempotent).
///
/// Synchronous: blocks on GetObject until the body is fully received, then
/// returns AssetResult::makeReady(bytes) on success, Failed otherwise.
/// Pending is not supported.
x3d::runtime::extract::AssetResolver makeS3Resolver(
    const std::string &endpoint = "http://localhost:9000");

}  // namespace x3d::runtime::io::s3

#endif  // X3D_RUNTIME_IO_S3_S3_RESOLVER_HPP
