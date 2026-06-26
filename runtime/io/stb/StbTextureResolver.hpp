// runtime/io/stb/StbTextureResolver.hpp — Backend A (stb_image) for the
// TextureResolver decode seam (T-TEX genericity proof, ADR-0024). Part of the
// runtime/io/stb quarantine (x3d_stb target, default OFF). Core (x3d_cpp,
// sdk.hpp) MUST NEVER include this file; it is decoder-free (no stb_image.h),
// so consumers linking x3d_stb do not inherit stb's translation unit.
#ifndef X3D_RUNTIME_IO_STB_STB_TEXTURE_RESOLVER_HPP
#define X3D_RUNTIME_IO_STB_STB_TEXTURE_RESOLVER_HPP

#include "TextureResolver.hpp"  // x3d::runtime::extract::TextureResolver

namespace x3d::runtime::io::stb {

/// Returns a TextureResolver that decodes image bytes via stb_image.
///
/// The `url` is treated as a local filesystem path: the resolver reads the file
/// and decodes it from memory to RGBA8 (4 channels, straight/non-premultiplied
/// alpha), tightly packed, **bottom-left origin** (GL convention — stb is run
/// with stbi_set_flip_vertically_on_load, matching the seam contract).
///
/// Returns Failed when the path cannot be opened or stb_image cannot decode the
/// bytes (unknown/corrupt format). Pending is never returned: decode is
/// synchronous and the bytes are local. Non-file URLs (http://, urn:) cannot be
/// opened as a path and therefore return Failed — fetching is AssetResolver's
/// job; this seam only turns bytes into pixels.
x3d::runtime::extract::TextureResolver makeStbTextureResolver();

}  // namespace x3d::runtime::io::stb

#endif  // X3D_RUNTIME_IO_STB_STB_TEXTURE_RESOLVER_HPP
