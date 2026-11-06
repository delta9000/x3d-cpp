// Ogg/Theora MovieDecoder adapter. Codec headers stay private.
// See ADR-0041.
#ifndef X3D_RUNTIME_IO_THEORA_THEORA_MOVIE_DECODER_HPP
#define X3D_RUNTIME_IO_THEORA_THEORA_MOVIE_DECODER_HPP

#include "AssetResolver.hpp" // x3d::runtime::extract::AssetResolver
#include "MovieDecoder.hpp"  // x3d::runtime::extract::MovieDecoder

namespace x3d::runtime::io::theora {

/// Returns a MovieDecoder that decodes Ogg/Theora video via libtheora + libogg.
///
/// Same observable seam contract as makePlMpegMovieDecoder: `resolver` fetches the
/// movie's encoded bytes for a URL (AssetKind::Movie); the returned decoder caches
/// one Theora context per URL, decodes sequentially to the requested media time
/// (rewinding when seeking backwards), holds the last frame at EOF, and returns a
/// bottom-left-origin opaque RGBA8 frame. Pending/Failed mirror the resolver.
x3d::runtime::extract::MovieDecoder
makeTheoraMovieDecoder(x3d::runtime::extract::AssetResolver resolver);

} // namespace x3d::runtime::io::theora

#endif // X3D_RUNTIME_IO_THEORA_THEORA_MOVIE_DECODER_HPP
