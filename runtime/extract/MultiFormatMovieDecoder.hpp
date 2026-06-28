// MultiFormatMovieDecoder.hpp — std-only composer over the MovieDecoder seam.
// namespace x3d::runtime::extract. Header-only, codec-free, leaf.
//
// A single video codec never covers every container an embedder needs (pl_mpeg is
// MPEG-1; libtheora is Ogg/Theora; a downstream might add WebM/VP9 or an OS-native
// H.264). So a real MovieDecoder is an ARRAY of per-format backends behind ONE
// seam — the movie analog of MultiFormatTextureResolver.
//
// THE RULE (ADR-0024 §7, shared by every decode seam): route by a cheap key BEFORE
// calling a backend, never use Failed to route. Decode routes by SNIFFED MAGIC
// BYTES (the URL extension can lie). Because dispatch picks the one correct backend
// up front — sniffed ONCE per URL and cached — each backend only ever sees input it
// owns, so its Failed keeps meaning "real decode failure" and the frozen
// Ready/Pending/Failed seam type is unchanged. This composer is that thin std-only
// array; it is NOT a change to the seam, and any conforming MovieDecoder (the two
// shipped backends, or a downstream's own) drops straight in.
#ifndef X3D_RUNTIME_EXTRACT_MULTI_FORMAT_MOVIE_DECODER_HPP
#define X3D_RUNTIME_EXTRACT_MULTI_FORMAT_MOVIE_DECODER_HPP

#include "AssetResolver.hpp"
#include "MovieDecoder.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace x3d::runtime::extract {

// ---------------------------------------------------------------------------
// MovieFormat — the cheap routing key. One entry per decodable container. The set
// is open: a downstream adds WebM / Mp4 here and registers a backend for it.
// ---------------------------------------------------------------------------
enum class MovieFormat { Mpeg1, OggTheora, WebM };

// ---------------------------------------------------------------------------
// sniffMovieFormat — classify a byte prefix by signature, before any backend runs.
//   * Ogg (Theora/Vorbis):     "OggS"            (4F 67 67 53)
//   * Matroska/WebM (EBML):    1A 45 DF A3
//   * MPEG-1 (program OR raw):  00 00 01 then a pack (BA) or sequence (B3) start
//     code — covers both MPEG-PS and raw elementary video streams.
// Returns nullopt for an unrecognizable blob (the composer then returns Failed).
// ---------------------------------------------------------------------------
inline std::optional<MovieFormat> sniffMovieFormat(
    const std::vector<std::uint8_t>& b) {
  const std::size_t n = b.size();
  if (n >= 4 && b[0] == 'O' && b[1] == 'g' && b[2] == 'g' && b[3] == 'S') {
    return MovieFormat::OggTheora;
  }
  if (n >= 4 && b[0] == 0x1A && b[1] == 0x45 && b[2] == 0xDF && b[3] == 0xA3) {
    return MovieFormat::WebM;
  }
  if (n >= 4 && b[0] == 0x00 && b[1] == 0x00 && b[2] == 0x01 &&
      (b[3] == 0xBA || b[3] == 0xB3)) {
    return MovieFormat::Mpeg1;
  }
  return std::nullopt;
}

// ---------------------------------------------------------------------------
// makeMultiFormatMovieDecoder — compose per-format backends into one seam.
//
// On the FIRST sight of a URL, `sniffer` fetches its bytes (AssetKind::Movie), the
// magic is classified, and the chosen backend is cached for that URL; every later
// frame skips the sniff and delegates straight to it. The chosen backend is handed
// the same `url` (it fetches via its OWN resolver) and only ever sees input it
// owns. Returns Pending while the sniff fetch is not ready (retry next frame, not
// cached); Failed when the bytes are unreadable, the format is unrecognized, or no
// backend is registered for it. Any conforming MovieDecoder may be registered.
// ---------------------------------------------------------------------------
inline MovieDecoder makeMultiFormatMovieDecoder(
    AssetResolver sniffer, std::map<MovieFormat, MovieDecoder> backends) {
  auto store = std::make_shared<std::map<MovieFormat, MovieDecoder>>(
      std::move(backends));
  // Per-URL routing decision (map nodes are address-stable, so the cached pointer
  // stays valid for the life of `store`). A cached null = "tried, unroutable".
  auto chosen = std::make_shared<std::map<std::string, const MovieDecoder*>>();

  return [sniffer, store, chosen](const std::string& url,
                                  double mediaTimeSeconds) -> FrameResult {
    auto ci = chosen->find(url);
    if (ci == chosen->end()) {
      if (!sniffer) {
        (*chosen)[url] = nullptr;
        return FrameResult::makeFailed();
      }
      AssetResolver s = sniffer;
      AssetResult a = s(url, AssetKind::Movie);
      if (a.pending()) return FrameResult::makePending(); // retry; do not cache.
      std::optional<MovieFormat> fmt;
      if (a.ready() && !a.bytes.empty()) fmt = sniffMovieFormat(a.bytes);
      const MovieDecoder* dec = nullptr;
      if (fmt.has_value()) {
        auto di = store->find(*fmt);
        if (di != store->end() && di->second) dec = &di->second;
      }
      ci = chosen->emplace(url, dec).first;
    }
    if (!ci->second) return FrameResult::makeFailed();
    return (*ci->second)(url, mediaTimeSeconds);
  };
}

} // namespace x3d::runtime::extract

#endif // X3D_RUNTIME_EXTRACT_MULTI_FORMAT_MOVIE_DECODER_HPP
