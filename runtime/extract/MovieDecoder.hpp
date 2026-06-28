// runtime/extract/MovieDecoder.hpp — the MovieDecoder seam (ADR-0041).
//
// MovieTexture frame decode is a CONSUMER-side seam, exactly like TextureResolver
// (ADR-0024) and FontMetrics (ADR-0025): the SDK core NEVER links a video codec.
// The extractor surfaces a MovieTexture as TextureRef::Source::Movie (url +
// sampler descriptor) and the X3DTimeDependentSystem drives its time-dependent
// fields (startTime/stopTime/loop/speed -> isActive/elapsedTime). What this seam
// adds is the frame-decode path: given a movie URL and the media time the consumer
// already computed, return the RGB(A) frame to upload as a texture.
//
// THREADING / OWNERSHIP CONTRACT (mirrors the other seams):
//   * The callback is owned by the CONSUMER; the SDK never invokes it and never
//     opens a media file or links a codec.
//   * It is called once per ACTIVE MovieTexture per frame with the media time
//     (seconds into the clip) the consumer derived from the time-dependent
//     lifecycle. The backend owns a per-URL decoder context internally — sequential
//     decode is cheap; seeking is the expensive path.
//   * Status mirrors AssetResolver/TextureResolver:
//       - Ready   -> upload `frame` (tightly packed RGBA8, BOTTOM-LEFT origin / GL).
//       - Pending -> not decoded yet; keep the previously uploaded frame; the
//                    render loop MUST NOT block.
//       - Failed  -> no data; white / last-frame fallback.
//   * EOF with loop=FALSE holds the last frame; loop=TRUE wraps. The media time
//     the consumer passes already reflects any loop wrap, so the backend just
//     decodes the requested time.
//
// Backend A (pl_mpeg, MPEG-1) lives in runtime/io/plmpeg/ as a flag-gated isolated
// target (the x3d_stbtt pattern); the core depends on NONE of it.
#ifndef X3D_RUNTIME_EXTRACT_MOVIE_DECODER_HPP
#define X3D_RUNTIME_EXTRACT_MOVIE_DECODER_HPP

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace x3d::runtime::extract {

// One decoded video frame. `rgba` is tightly packed RGBA8 (4 bytes/pixel,
// width*height*4 bytes) with a BOTTOM-LEFT origin so it uploads directly as a GL
// texture with no flip — the same convention as TexturePixelResult.
struct VideoFrame {
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  std::vector<std::uint8_t> rgba;
};

// Lifecycle of a single decode request. Pending is the "not yet" state that keeps
// the render loop non-blocking.
enum class FrameStatus { Ready, Pending, Failed };

// status + frame. For Pending/Failed `frame` is empty.
struct FrameResult {
  FrameStatus status = FrameStatus::Failed;
  VideoFrame frame;

  bool ready() const { return status == FrameStatus::Ready; }
  bool pending() const { return status == FrameStatus::Pending; }
  bool failed() const { return status == FrameStatus::Failed; }

  static FrameResult makeReady(VideoFrame f) {
    return FrameResult{FrameStatus::Ready, std::move(f)};
  }
  static FrameResult makePending() {
    return FrameResult{FrameStatus::Pending, {}};
  }
  static FrameResult makeFailed() { return FrameResult{FrameStatus::Failed, {}}; }
};

// THE callback type. (url, mediaTimeSeconds) -> FrameResult. Copyable value type
// so it threads through the consumer's render state identically to AssetResolver /
// FontMetrics. Owned by the consumer; the SDK never calls it.
using MovieDecoder =
    std::function<FrameResult(const std::string &url, double mediaTimeSeconds)>;

// Default seam value: a decoder that never produces a frame (always Failed). Used
// when no backend is wired — movie-textured surfaces fall back to white/last, the
// same always-safe behaviour as an unwired TextureResolver.
inline MovieDecoder makeNullMovieDecoder() {
  return [](const std::string &, double) { return FrameResult::makeFailed(); };
}

} // namespace x3d::runtime::extract

#endif // X3D_RUNTIME_EXTRACT_MOVIE_DECODER_HPP
