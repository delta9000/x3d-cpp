// ─── include/x3d/ext.hpp ──────────────────────────────────────────────────────
// Opt-in add-on façade for the x3d-cpp-gen extension layer (x3d_cpp::ext).
//
// Include this header IN ADDITION TO (never instead of) x3d/sdk.hpp.
// Link the CMake target x3d_cpp::ext (requires -DX3D_CPP_BUILD_EXT=ON).
//
// This header is NOT included by x3d/sdk.hpp. The core SDK has no knowledge of
// the extension layer — the firewall is absolute. Only embedders that explicitly
// opt in get the ext surface.
//
// Canonical usage:
//   #include "x3d/sdk.hpp"        // core surface
//   #include "x3d/ext.hpp"        // ext add-on (requires x3d_cpp::ext link)
//   auto resolver = x3d::ext::install();
//   auto doc = x3d::sdk::parseDocument(text, x3d::sdk::Encoding::XML, "", resolver);
// ─────────────────────────────────────────────────────────────────────────────
#ifndef X3D_EXT_HPP
#define X3D_EXT_HPP

#include "ExternalGeometry.hpp"          // x3d::runtime::ext::ExternalGeometry
#include "ExternalGeometryResolver.hpp"  // x3d::runtime::ext::makeExternalGeometryResolver()
#include "ExtResolver.hpp"               // x3d::runtime::ext::install()

/// The curated ext embedder surface.
namespace x3d::ext {

using x3d::runtime::ext::ExternalGeometry;             ///< the ExternalGeometry node class
using x3d::runtime::ext::install;                      ///< install(base) → ProtoDeclarationResolver
using x3d::runtime::ext::kExternalGeometryUrn;         ///< "urn:x3d-cpp-gen:ext:ExternalGeometry"
using x3d::runtime::ext::makeExternalGeometryProto;    ///< factory (advanced use)
using x3d::runtime::ext::makeExternalGeometryResolver; ///< (bytesResolver) → MeshBuildOptions seam (Round 2)

} // namespace x3d::ext

#endif // X3D_EXT_HPP
