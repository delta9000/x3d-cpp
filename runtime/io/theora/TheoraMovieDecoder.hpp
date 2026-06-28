// runtime/io/theora/TheoraMovieDecoder.hpp — Backend B (libtheora/libogg) for the
// MovieDecoder seam (ADR-0041). The seam's SECOND independent backend: it proves
// genericity (one frozen interface, two format-partitioned codecs) via the shared
// semantics-contract test — Theora and MPEG-1 cannot decode the same input, so the
// proof is contract-parity, not bit-identical swap (see ADR-0041).
//
// libtheora + libogg are NOT single-header; they are system/vcpkg libraries linked
// PRIVATE to the one implementation TU (the FreeType backend-B pattern, ADR-0025),
// so no codec header leaks into this decoder-free public header. Theora is a Xiph
// BSD codec and royalty-free, so it joins the default-shippable tier (flag-gated
// OFF: -DX3D_CPP_BUILD_THEORA=ON).
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
/// BOTTOM-LEFT-origin opaque RGBA8 frame. Pending/Failed mirror the resolver.
x3d::runtime::extract::MovieDecoder
makeTheoraMovieDecoder(x3d::runtime::extract::AssetResolver resolver);

} // namespace x3d::runtime::io::theora

#endif // X3D_RUNTIME_IO_THEORA_THEORA_MOVIE_DECODER_HPP
