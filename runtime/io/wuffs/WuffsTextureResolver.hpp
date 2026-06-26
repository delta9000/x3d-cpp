// runtime/io/wuffs/WuffsTextureResolver.hpp — Backend B (wuffs) for the
// TextureResolver decode seam (T-TEX genericity proof, ADR-0024). Part of the
// runtime/io/wuffs quarantine (x3d_wuffs target, default OFF). Core (x3d_cpp,
// sdk.hpp) MUST NEVER include this file; it is decoder-free (no wuffs header),
// so consumers linking x3d_wuffs do not inherit the wuffs amalgamation.
//
// wuffs decodes from a memory-safe, transpiled-from-DSL codec — an independent
// implementation from stb_image's hand-written C, so a byte-equal parity
// failure in the swap-test is a real correctness leak, not shared-bug masking.
#ifndef X3D_RUNTIME_IO_WUFFS_WUFFS_TEXTURE_RESOLVER_HPP
#define X3D_RUNTIME_IO_WUFFS_WUFFS_TEXTURE_RESOLVER_HPP

#include "TextureResolver.hpp"  // x3d::runtime::extract::TextureResolver

namespace x3d::runtime::io::wuffs {

/// Returns a TextureResolver that decodes image bytes via wuffs.
///
/// The `url` is treated as a local filesystem path: the resolver reads the file
/// and decodes it from memory to RGBA8 (4 channels, straight/non-premultiplied
/// alpha), tightly packed, **bottom-left origin** (GL convention). wuffs decodes
/// top-left, so the rows are flipped here to match the seam contract and the stb
/// backend.
///
/// Returns Failed when the path cannot be opened or wuffs cannot decode the
/// bytes. Pending is never returned. Non-file URLs return Failed (decode-only
/// seam; fetching is AssetResolver's job).
x3d::runtime::extract::TextureResolver makeWuffsTextureResolver();

}  // namespace x3d::runtime::io::wuffs

#endif  // X3D_RUNTIME_IO_WUFFS_WUFFS_TEXTURE_RESOLVER_HPP
