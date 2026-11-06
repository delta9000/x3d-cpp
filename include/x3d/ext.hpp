// Opt-in add-on façade for the x3d-cpp-gen extension layer (x3d_cpp::ext).
//
// Include this header alongside x3d/sdk.hpp.
// Link the CMake target x3d_cpp::ext (requires -DX3D_CPP_BUILD_EXT=ON).
//
// The core SDK does not include or depend on this extension layer.
//
// Canonical usage:
//   #include "x3d/sdk.hpp"        // core surface
//   #include "x3d/ext.hpp"        // ext add-on (requires x3d_cpp::ext link)
//   auto resolver = x3d::ext::install();
//   auto doc = x3d::sdk::parseDocument(text, x3d::sdk::Encoding::XML, "", resolver);
#ifndef X3D_EXT_HPP
#define X3D_EXT_HPP

#include "ExternalGeometry.hpp"
#include "ExternalGeometryResolver.hpp"
#include "ExtResolver.hpp"

namespace x3d::ext {

using x3d::runtime::ext::ExternalGeometry;
using x3d::runtime::ext::install;
using x3d::runtime::ext::kExternalGeometryUrn;
using x3d::runtime::ext::makeExternalGeometryProto;
using x3d::runtime::ext::makeExternalGeometryResolver;

} // namespace x3d::ext

#endif // X3D_EXT_HPP
