// MPEG-1 MovieDecoder adapter. Codec headers stay private.
// See ADR-0041.
#ifndef X3D_RUNTIME_IO_PLMPEG_PLMPEG_MOVIE_DECODER_HPP
#define X3D_RUNTIME_IO_PLMPEG_PLMPEG_MOVIE_DECODER_HPP

#include "AssetResolver.hpp" // x3d::runtime::extract::AssetResolver
#include "MovieDecoder.hpp"  // x3d::runtime::extract::MovieDecoder
#include "AudioDecoder.hpp" // x3d::runtime::AudioDecoder

namespace x3d::runtime::io::plmpeg {

/// Returns a MovieDecoder that decodes MPEG-1 video via pl_mpeg.
///
/// `resolver` fetches the movie's encoded bytes for a URL (AssetKind::Movie);
/// the returned decoder caches one pl_mpeg context per URL. Resolver `Pending`
/// surfaces as a `Pending` frame (retry next frame); `Failed`/empty caches a
/// permanent failure for that URL. Each call seeks the cached context to the
/// requested media time (clamped to [0, duration) so an EOF holds the last frame)
/// and returns a bottom-left-origin RGBA8 frame per the seam contract.
x3d::runtime::extract::MovieDecoder
makePlMpegMovieDecoder(x3d::runtime::extract::AssetResolver resolver);

/// Decode the first MPEG-1 program-stream MP2 audio track to mono PCM.
/// Raw elementary video and files without audio return an unsuccessful result.
x3d::runtime::AudioDecoder makePlMpegMovieAudioDecoder();

} // namespace x3d::runtime::io::plmpeg

#endif // X3D_RUNTIME_IO_PLMPEG_PLMPEG_MOVIE_DECODER_HPP
