// runtime/io/plmpeg/PlMpegMovieDecoder.cpp — the ONE translation unit that meets
// pl_mpeg. PL_MPEG_IMPLEMENTATION is defined exactly here so the codec lives in a
// single isolated object file and never leaks past the x3d_plmpeg target.
//
// Two MPEG-1 container shapes occur in the wild and both must work:
//   * PROGRAM STREAM (.mpg with a pack header 0x000001BA) — the high-level plm_t
//     demuxer handles it; we seek by media time (plm_seek_frame).
//   * RAW ELEMENTARY VIDEO (.mpg/.m1v starting with a sequence header 0x000001B3,
//     no system layer — the entire Web3D/NIST conformance corpus) — the plm_t
//     demuxer reports zero streams for these, so we drive the low-level plm_video_t
//     directly and reach a media time by sequential decode (rewind when seeking
//     backwards), exactly the "sequential decode is cheap" model in ADR-0041.
#include "PlMpegMovieDecoder.hpp"

#include <algorithm> // std::swap_ranges
#include <cstddef>
#include <cstdint>
#include <cstdlib> // std::malloc / std::free
#include <cstring> // std::memcpy
#include <map>
#include <memory>
#include <string>
#include <utility>

#define PL_MPEG_IMPLEMENTATION
#include "vendor/pl_mpeg.h"

namespace x3d::runtime::io::plmpeg {
namespace {

using x3d::runtime::extract::AssetKind;
using x3d::runtime::extract::AssetResolver;
using x3d::runtime::extract::FrameResult;
using x3d::runtime::extract::MovieDecoder;
using x3d::runtime::extract::VideoFrame;

struct PlmDeleter {
  void operator()(plm_t *p) const {
    if (p) plm_destroy(p);
  }
};
struct PlmVideoDeleter {
  void operator()(plm_video_t *p) const {
    if (p) plm_video_destroy(p); // owns its plm_buffer (destroy_when_done)
  }
};
using PlmPtr = std::unique_ptr<plm_t, PlmDeleter>;
using PlmVideoPtr = std::unique_ptr<plm_video_t, PlmVideoDeleter>;

// Per-URL decoder context. Exactly one of {plm, video} is set; `failed` marks a
// URL we already tried and could not open so we never re-resolve it every frame.
struct Context {
  PlmPtr plm;            // program-stream path
  PlmVideoPtr video;     // raw-elementary-video path
  double lastDecodedTime = -1.0; // media time of `lastFrame` (raw path)
  bool haveFrame = false;        // raw path has produced at least one frame
  VideoFrame lastFrame;          // cached last frame (raw path repeat / EOF hold)
  bool failed = false;
};

// First start code in the first few KB: 0xBA => program stream, 0xB3 => raw video
// elementary stream. A program stream's first start code is the pack header, which
// precedes any sequence header, so returning on the first match disambiguates.
char detectStreamKind(const std::vector<std::uint8_t> &b) {
  const std::size_t limit = b.size() < 8192 ? b.size() : 8192;
  for (std::size_t i = 0; i + 3 < limit; ++i) {
    if (b[i] == 0 && b[i + 1] == 0 && b[i + 2] == 1) {
      if (b[i + 3] == 0xBA) return 'P'; // pack header -> program stream
      if (b[i + 3] == 0xB3) return 'V'; // sequence header -> raw video
    }
  }
  return 0;
}

// frame -> bottom-left-origin RGBA8 (the seam contract); pl_mpeg writes top-left.
VideoFrame toBottomLeftRgba(plm_frame_t *frame) {
  VideoFrame out;
  out.width = frame->width;
  out.height = frame->height;
  const std::size_t stride = static_cast<std::size_t>(frame->width) * 4;
  // Pre-fill 0xFF so the alpha byte ends up 255 (opaque): plm_frame_to_rgba writes
  // R,G,B and leaves the alpha component UNTOUCHED, so a zero-filled buffer would
  // yield a fully transparent frame.
  out.rgba.assign(stride * frame->height, 0xFF);
  plm_frame_to_rgba(frame, out.rgba.data(), static_cast<int>(stride));
  for (std::uint32_t y = 0; y < frame->height / 2; ++y) {
    std::uint8_t *a = out.rgba.data() + static_cast<std::size_t>(y) * stride;
    std::uint8_t *b = out.rgba.data() +
                      static_cast<std::size_t>(frame->height - 1 - y) * stride;
    std::swap_ranges(a, a + stride, b);
  }
  return out;
}

} // namespace

MovieDecoder makePlMpegMovieDecoder(AssetResolver resolver) {
  // Shared so the returned std::function stays copyable while every copy talks to
  // the SAME per-URL context cache (sequential decode needs a persistent decoder).
  auto cache = std::make_shared<std::map<std::string, Context>>();

  return [cache, resolver](const std::string &url,
                           double mediaTimeSeconds) -> FrameResult {
    auto it = cache->find(url);
    if (it == cache->end()) {
      // First sight of this URL: fetch the encoded bytes and open a context.
      if (!resolver) {
        (*cache)[url].failed = true;
        return FrameResult::makeFailed();
      }
      AssetResolver res = resolver;
      auto asset = res(url, AssetKind::Movie);
      if (asset.pending())
        return FrameResult::makePending(); // not cached: retry next frame.
      if (!asset.ready() || asset.bytes.empty()) {
        (*cache)[url].failed = true;
        return FrameResult::makeFailed();
      }

      const char kind = detectStreamKind(asset.bytes);
      // pl_mpeg takes ownership of the buffer (free_when_done=1): hand it a
      // malloc'd copy it can free() on destroy.
      const std::size_t n = asset.bytes.size();
      auto *buf = static_cast<std::uint8_t *>(std::malloc(n));
      if (!buf) {
        (*cache)[url].failed = true;
        return FrameResult::makeFailed();
      }
      std::memcpy(buf, asset.bytes.data(), n);

      Context ctx;
      if (kind == 'V') {
        plm_buffer_t *pbuf =
            plm_buffer_create_with_memory(buf, n, /*free_when_done=*/1);
        plm_video_t *vid =
            pbuf ? plm_video_create_with_buffer(pbuf, /*destroy_when_done=*/1)
                 : nullptr;
        if (!vid || plm_video_get_width(vid) <= 0 ||
            plm_video_get_height(vid) <= 0) {
          if (vid)
            plm_video_destroy(vid);
          else if (pbuf)
            plm_buffer_destroy(pbuf); // frees buf
          else
            std::free(buf);
          ctx.failed = true;
          (*cache)[url] = std::move(ctx);
          return FrameResult::makeFailed();
        }
        ctx.video.reset(vid);
      } else {
        // Program stream (or unknown: let the demuxer try).
        plm_t *plm = plm_create_with_memory(buf, n, /*free_when_done=*/1);
        if (!plm || plm_get_width(plm) <= 0 || plm_get_height(plm) <= 0) {
          if (plm)
            plm_destroy(plm); // frees buf
          else
            std::free(buf);
          ctx.failed = true;
          (*cache)[url] = std::move(ctx);
          return FrameResult::makeFailed();
        }
        plm_set_audio_enabled(plm, 0); // texture path: video only.
        ctx.plm.reset(plm);
      }
      it = cache->emplace(url, std::move(ctx)).first;
    }

    Context &ctx = it->second;
    if (ctx.failed) return FrameResult::makeFailed();

    // Clamp to [0, duration): the consumer passes loop-wrapped time, but an EOF
    // (loop=FALSE past the end) should HOLD the last frame, not go blank.
    double t = mediaTimeSeconds < 0.0 ? 0.0 : mediaTimeSeconds;

    // ---- Program-stream path: seek by media time. ----
    if (ctx.plm) {
      plm_t *plm = ctx.plm.get();
      const double duration = plm_get_duration(plm);
      const double fps = plm_get_framerate(plm);
      const double framePeriod = (fps > 0.0) ? 1.0 / fps : 1e-3;
      if (duration > 0.0 && t >= duration) t = duration - framePeriod;
      if (t < 0.0) t = 0.0;
      plm_frame_t *frame = plm_seek_frame(plm, t, /*seek_exact=*/1);
      if (!frame) return FrameResult::makeFailed();
      return FrameResult::makeReady(toBottomLeftRgba(frame));
    }

    // ---- Raw-elementary path: sequential decode to media time. ----
    plm_video_t *vid = ctx.video.get();
    if (!vid) return FrameResult::makeFailed();

    // Seeking backwards (e.g. a loop wrap or a re-render): rewind and replay.
    if (t + 1e-9 < ctx.lastDecodedTime) {
      plm_video_rewind(vid);
      ctx.lastDecodedTime = -1.0;
      ctx.haveFrame = false;
    }

    // Decode forward while the NEXT frame's time is still <= t; the decoder's
    // internal time is the time of the frame about to be decoded. Convert only the
    // final frame. plm_video_get_time advances by 1/fps per decode, so this lands
    // on the greatest frame time <= t (and on the last frame at/after EOF).
    plm_frame_t *cur = nullptr;
    while (!plm_video_has_ended(vid) &&
           (!ctx.haveFrame || plm_video_get_time(vid) <= t + 1e-9)) {
      plm_frame_t *f = plm_video_decode(vid);
      if (!f) break;
      cur = f;
      ctx.haveFrame = true;
      ctx.lastDecodedTime = f->time;
    }
    if (cur) ctx.lastFrame = toBottomLeftRgba(cur);
    if (!ctx.haveFrame) return FrameResult::makeFailed();
    return FrameResult::makeReady(ctx.lastFrame);
  };
}

} // namespace x3d::runtime::io::plmpeg
