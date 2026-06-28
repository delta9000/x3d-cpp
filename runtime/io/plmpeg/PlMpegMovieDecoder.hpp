// runtime/io/plmpeg/PlMpegMovieDecoder.hpp — Backend A (pl_mpeg, MPEG-1) for the
// MovieDecoder seam (ADR-0041). Part of the flag-gated isolated x3d_plmpeg target
// (the x3d_stbtt pattern): pl_mpeg.h is included PRIVATE in the single
// implementation TU and never leaks into this decoder-free public header.
//
// MPEG-1 is royalty-free (patents expired) and pl_mpeg is MIT, so this backend is
// safe to ship default-OFF; patent-encumbered codecs (H.264/265, FFmpeg,
// GStreamer) plug into the SAME seam downstream — see ADR-0041.
#ifndef X3D_RUNTIME_IO_PLMPEG_PLMPEG_MOVIE_DECODER_HPP
#define X3D_RUNTIME_IO_PLMPEG_PLMPEG_MOVIE_DECODER_HPP

#include "AssetResolver.hpp" // x3d::runtime::extract::AssetResolver
#include "MovieDecoder.hpp"  // x3d::runtime::extract::MovieDecoder

namespace x3d::runtime::io::plmpeg {

/// Returns a MovieDecoder that decodes MPEG-1 video via pl_mpeg.
///
/// `resolver` fetches the movie's encoded bytes for a URL (AssetKind::Movie);
/// the returned decoder caches one pl_mpeg context per URL. Resolver `Pending`
/// surfaces as a `Pending` frame (retry next frame); `Failed`/empty caches a
/// permanent failure for that URL. Each call seeks the cached context to the
/// requested media time (clamped to [0, duration) so an EOF holds the last frame)
/// and returns a BOTTOM-LEFT-origin RGBA8 frame per the seam contract.
x3d::runtime::extract::MovieDecoder
makePlMpegMovieDecoder(x3d::runtime::extract::AssetResolver resolver);

} // namespace x3d::runtime::io::plmpeg

#endif // X3D_RUNTIME_IO_PLMPEG_PLMPEG_MOVIE_DECODER_HPP
