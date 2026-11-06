// stb_image TextureResolver adapter. Decoder headers stay private.
// See ADR-0024.
#ifndef X3D_RUNTIME_IO_STB_STB_TEXTURE_RESOLVER_HPP
#define X3D_RUNTIME_IO_STB_STB_TEXTURE_RESOLVER_HPP

#include "AssetResolver.hpp" // x3d::runtime::extract::TextureResolver
#include "TextureResolver.hpp"

namespace x3d::runtime::io::stb {

/// Returns a TextureResolver that decodes image bytes via stb_image.
///
/// The `url` is treated as a local filesystem path: the resolver reads the file
/// and decodes it from memory to RGBA8 (4 channels, straight/non-premultiplied
/// alpha), tightly packed, bottom-left origin (GL convention — stb is run
/// with stbi_set_flip_vertically_on_load, matching the seam contract).
///
/// Returns Failed when the path cannot be opened or stb_image cannot decode the
/// bytes (unknown/corrupt format). Pending is never returned: decode is
/// synchronous and the bytes are local. Non-file URLs (http://, urn:) cannot be
/// opened as a path and therefore return Failed — fetching is AssetResolver's
/// job; this seam only turns bytes into pixels.
x3d::runtime::extract::TextureResolver makeStbTextureResolver();

/// Fetch bytes through the supplied oracle, then decode from memory.
/// Pending and Failed are preserved; URLs and fallback order belong to the
/// caller.
x3d::runtime::extract::TextureResolver
makeStbTextureResolver(x3d::runtime::extract::AssetResolver fetch);

}  // namespace x3d::runtime::io::stb

#endif  // X3D_RUNTIME_IO_STB_STB_TEXTURE_RESOLVER_HPP
