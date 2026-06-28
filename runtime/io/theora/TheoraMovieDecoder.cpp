// runtime/io/theora/TheoraMovieDecoder.cpp — the ONE translation unit that meets
// libogg + libtheora. The codec headers are PRIVATE to this TU (FreeType backend-B
// pattern); the factory exchanges only std types so nothing leaks to consumers.
//
// Ogg/Theora decode: feed all encoded bytes into an ogg_sync, demux pages by
// serial number to find the Theora logical stream, parse its 3 setup headers, then
// decode data packets to YCbCr and convert to RGBA. Seeking is done by sequential
// decode (rewind = tear down + re-init from the same bytes), exactly the seam
// behavior the pl_mpeg raw-elementary path uses, so both backends share the
// contract (ADR-0041): Pending/Failed parity, EOF-holds-last, backward-seek rewind.
#include "TheoraMovieDecoder.hpp"

#include <algorithm> // std::swap_ranges
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <ogg/ogg.h>
#include <theora/theoradec.h>

namespace x3d::runtime::io::theora {
namespace {

using x3d::runtime::extract::AssetKind;
using x3d::runtime::extract::AssetResolver;
using x3d::runtime::extract::FrameResult;
using x3d::runtime::extract::MovieDecoder;
using x3d::runtime::extract::VideoFrame;

inline std::uint8_t clamp8(int v) {
  return static_cast<std::uint8_t>(v < 0 ? 0 : (v > 255 ? 255 : v));
}

// A live Theora decode session over an in-memory Ogg stream. Holds all libogg /
// libtheora state; reinit() rebuilds it from the (retained) encoded bytes so a
// backward seek can replay from the start.
struct Session {
  std::vector<std::uint8_t> bytes;
  ogg_sync_state oy{};
  ogg_stream_state to{};
  th_info ti{};
  th_comment tc{};
  th_setup_info *ts = nullptr;
  th_dec_ctx *dec = nullptr;
  int serialno = 0;
  bool fedAll = false;
  bool valid = false;

  ~Session() { teardown(); }

  void teardown() {
    if (dec) { th_decode_free(dec); dec = nullptr; }
    if (ts) { th_setup_free(ts); ts = nullptr; }
    if (valid) { ogg_stream_clear(&to); }
    th_comment_clear(&tc);
    th_info_clear(&ti);
    ogg_sync_clear(&oy);
    valid = false;
    fedAll = false;
  }

  // Push ALL retained bytes into the ogg_sync (one shot; clips are small enough).
  void feedAll() {
    if (fedAll) return;
    const long n = static_cast<long>(bytes.size());
    char *buf = ogg_sync_buffer(&oy, n);
    if (buf && n > 0) {
      std::copy(bytes.begin(), bytes.end(), reinterpret_cast<std::uint8_t *>(buf));
      ogg_sync_wrote(&oy, n);
    }
    fedAll = true;
  }

  // Parse headers + allocate the decoder. Returns true on a usable Theora stream.
  bool init() {
    ogg_sync_init(&oy);
    th_info_init(&ti);
    th_comment_init(&tc);
    feedAll();

    int theoraHeaders = 0;
    bool found = false;
    ogg_page og;

    // Phase 1: walk the beginning-of-stream pages, adopt the Theora logical
    // stream, discard the rest (vorbis / skeleton).
    while (ogg_sync_pageout(&oy, &og) > 0) {
      if (!ogg_page_bos(&og)) {
        if (found) ogg_stream_pagein(&to, &og); // first data/secondary page
        break;
      }
      ogg_stream_state test;
      ogg_stream_init(&test, ogg_page_serialno(&og));
      ogg_stream_pagein(&test, &og);
      ogg_packet op;
      if (ogg_stream_packetout(&test, &op) > 0 && !found &&
          th_decode_headerin(&ti, &tc, &ts, &op) >= 0) {
        to = test; // adopt (transfers libogg's internal buffers — canonical idiom)
        serialno = ogg_page_serialno(&og);
        found = true;
        theoraHeaders = 1;
      } else {
        ogg_stream_clear(&test);
      }
    }
    if (!found) return false;

    // Phase 2: the remaining two Theora setup headers.
    while (theoraHeaders < 3) {
      ogg_packet op;
      int r = ogg_stream_packetout(&to, &op);
      if (r > 0) {
        if (th_decode_headerin(&ti, &tc, &ts, &op) < 0) return false;
        ++theoraHeaders;
        continue;
      }
      if (ogg_sync_pageout(&oy, &og) > 0) {
        if (ogg_page_serialno(&og) == serialno) ogg_stream_pagein(&to, &og);
        continue;
      }
      return false; // ran out of data before 3 headers.
    }

    dec = th_decode_alloc(&ti, ts);
    if (!dec) return false;
    valid = true;
    return true;
  }

  // Pull the next Theora data packet, decode it, and (unless a duplicate frame)
  // refresh `out`. Returns true while frames remain, false at end of stream.
  // `endTime` is the decoded frame's presentation end-time in seconds.
  bool decodeNext(VideoFrame &out, double &endTime, bool &refreshed) {
    refreshed = false;
    ogg_page og;
    ogg_packet op;
    for (;;) {
      int r = ogg_stream_packetout(&to, &op);
      if (r > 0) {
        ogg_int64_t granulepos = -1;
        int dr = th_decode_packetin(dec, &op, &granulepos);
        if (dr != 0 && dr != TH_DUPFRAME) continue; // header/garbage packet.
        endTime = (granulepos >= 0) ? th_granule_time(dec, granulepos) : endTime;
        if (dr == TH_DUPFRAME) { refreshed = false; return true; } // reuse last.
        th_ycbcr_buffer ycbcr;
        if (th_decode_ycbcr_out(dec, ycbcr) != 0) continue;
        toRgba(ycbcr, out);
        refreshed = true;
        return true;
      }
      // Need another page for this stream.
      if (ogg_sync_pageout(&oy, &og) > 0) {
        if (ogg_page_serialno(&og) == serialno) ogg_stream_pagein(&to, &og);
        continue;
      }
      return false; // end of stream.
    }
  }

  // th_ycbcr_buffer -> bottom-left-origin opaque RGBA8 over the picture region.
  void toRgba(const th_ycbcr_buffer &yc, VideoFrame &out) {
    const int pw = ti.pic_width, ph = ti.pic_height;
    const int px = ti.pic_x, py = ti.pic_y;
    out.width = static_cast<std::uint32_t>(pw);
    out.height = static_cast<std::uint32_t>(ph);
    out.rgba.assign(static_cast<std::size_t>(pw) * ph * 4, 0xFF);

    const int hdec = (ti.pixel_fmt == TH_PF_420 || ti.pixel_fmt == TH_PF_422) ? 1 : 0;
    const int vdec = (ti.pixel_fmt == TH_PF_420) ? 1 : 0;
    const th_img_plane &Y = yc[0];
    const th_img_plane &Cb = yc[1];
    const th_img_plane &Cr = yc[2];

    for (int y = 0; y < ph; ++y) {
      // Bottom-left origin: row 0 of output is the BOTTOM image row.
      const int srcY = py + (ph - 1 - y);
      std::uint8_t *dst = out.rgba.data() + static_cast<std::size_t>(y) * pw * 4;
      const std::uint8_t *yrow = Y.data + static_cast<std::ptrdiff_t>(srcY) * Y.stride;
      const int cYrow = (py + (ph - 1 - y)) >> vdec;
      const std::uint8_t *cbrow = Cb.data + static_cast<std::ptrdiff_t>(cYrow) * Cb.stride;
      const std::uint8_t *crrow = Cr.data + static_cast<std::ptrdiff_t>(cYrow) * Cr.stride;
      for (int x = 0; x < pw; ++x) {
        const int sx = px + x;
        const int cx = sx >> hdec;
        const int yy = yrow[sx];
        const int u = cbrow[cx] - 128;
        const int v = crrow[cx] - 128;
        // BT.601 full-range (matches the pl_mpeg backend so the two agree visually).
        dst[x * 4 + 0] = clamp8(yy + ((91881 * v) >> 16));
        dst[x * 4 + 1] = clamp8(yy - ((22554 * u + 46802 * v) >> 16));
        dst[x * 4 + 2] = clamp8(yy + ((116130 * u) >> 16));
        // alpha left 0xFF.
      }
    }
  }
};

struct Context {
  std::unique_ptr<Session> session; // null => failed.
  double lastEndTime = -1.0;
  bool haveFrame = false;
  VideoFrame lastFrame;
  bool failed = false;
};

} // namespace

MovieDecoder makeTheoraMovieDecoder(AssetResolver resolver) {
  auto cache = std::make_shared<std::map<std::string, Context>>();

  return [cache, resolver](const std::string &url,
                           double mediaTimeSeconds) -> FrameResult {
    auto it = cache->find(url);
    if (it == cache->end()) {
      if (!resolver) {
        (*cache)[url].failed = true;
        return FrameResult::makeFailed();
      }
      AssetResolver res = resolver;
      auto asset = res(url, AssetKind::Movie);
      if (asset.pending()) return FrameResult::makePending();
      if (!asset.ready() || asset.bytes.empty()) {
        (*cache)[url].failed = true;
        return FrameResult::makeFailed();
      }
      auto sess = std::make_unique<Session>();
      sess->bytes = std::move(asset.bytes);
      Context ctx;
      if (!sess->init()) {
        ctx.failed = true;
        (*cache)[url] = std::move(ctx);
        return FrameResult::makeFailed();
      }
      ctx.session = std::move(sess);
      it = cache->emplace(url, std::move(ctx)).first;
    }

    Context &ctx = it->second;
    if (ctx.failed || !ctx.session) return FrameResult::makeFailed();
    double t = mediaTimeSeconds < 0.0 ? 0.0 : mediaTimeSeconds;

    // Seeking backwards: rebuild the session from the retained bytes.
    if (t + 1e-9 < ctx.lastEndTime) {
      std::vector<std::uint8_t> bytes = std::move(ctx.session->bytes);
      ctx.session = std::make_unique<Session>();
      ctx.session->bytes = std::move(bytes);
      ctx.lastEndTime = -1.0;
      ctx.haveFrame = false;
      if (!ctx.session->init()) {
        ctx.failed = true;
        return FrameResult::makeFailed();
      }
    }

    // Decode forward until a frame's end-time passes t (greatest frame <= t), or
    // the stream ends (then `lastFrame` holds the final frame — EOF hold).
    while (!ctx.haveFrame || ctx.lastEndTime <= t + 1e-9) {
      double endTime = ctx.lastEndTime;
      bool refreshed = false;
      if (!ctx.session->decodeNext(ctx.lastFrame, endTime, refreshed)) break;
      ctx.lastEndTime = endTime;
      ctx.haveFrame = true;
      if (refreshed) { /* lastFrame updated */ }
    }

    if (!ctx.haveFrame) return FrameResult::makeFailed();
    return FrameResult::makeReady(ctx.lastFrame);
  };
}

} // namespace x3d::runtime::io::theora
