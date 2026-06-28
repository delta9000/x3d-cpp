// runtime/io/plmpeg/tests/movie_decoder_tests.cpp — the MovieDecoder seam
// genericity proof (ADR-0041).
//
// Video codecs PARTITION by format — no two backends decode the same input — so
// the proof is NOT a bit-identical swap (the font/texture pattern) but a SHARED
// per-backend semantics-CONTRACT: each backend, given its OWN reference clip, must
// agree on the observable seam behavior — Pending-before-bytes, Ready frame dims +
// tight opaque RGBA, EOF-holds-last-frame, backward-seek (rewind) parity, and
// Failed on bad/missing/garbage input. runContract() encodes that contract; it is
// run against pl_mpeg (Backend A, always) and libtheora (Backend B, when built).
// Two backends passing the identical contract is what flips the seam to GREEN.
//
// ABI ISOLATION: this TU talks ONLY through the seam factories (decoder-free
// headers); it includes NO pl_mpeg / theora / ogg header.
#include "PlMpegMovieDecoder.hpp"
#include "doctest/doctest.h"

#ifdef X3D_MOVIE_HAVE_THEORA
#include "TheoraMovieDecoder.hpp"
#endif

#include <cstdint>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

using namespace x3d::runtime::extract;

namespace {

std::vector<std::uint8_t> readFixture(const char *dir, const char *name) {
  std::string path = std::string(dir) + "/" + name;
  std::ifstream f(path, std::ios::binary);
  return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(f)),
                                   std::istreambuf_iterator<char>());
}

AssetResolver readyResolver(std::vector<std::uint8_t> bytes) {
  return [bytes = std::move(bytes)](const std::string &, AssetKind) {
    return AssetResult::makeReady(bytes);
  };
}

// A backend is a factory: (resolver) -> MovieDecoder. The contract is identical
// across backends; only the reference clip + its dimensions differ.
using Backend = std::function<MovieDecoder(AssetResolver)>;

void runContract(const Backend &make, const char *dir, const char *fixture,
                 unsigned w, unsigned h) {
  auto bytes = readFixture(dir, fixture);
  REQUIRE_MESSAGE(!bytes.empty(), fixture); // fixture present

  // --- Ready: a tight, opaque, correctly-sized frame. ---
  auto dec = make(readyResolver(bytes));
  FrameResult r = dec("clip", 0.0);
  REQUIRE(r.ready());
  CHECK(r.frame.width == w);
  CHECK(r.frame.height == h);
  CHECK(r.frame.rgba.size() == static_cast<std::size_t>(w) * h * 4);
  // Solid-red clip: centre pixel's red dominates (loose — lossy YUV), opaque alpha.
  const std::size_t c = (static_cast<std::size_t>(h / 2) * w + w / 2) * 4;
  CHECK(r.frame.rgba[c + 0] > r.frame.rgba[c + 1]);
  CHECK(r.frame.rgba[c + 0] > r.frame.rgba[c + 2]);
  CHECK(r.frame.rgba[c + 3] == 255);

  // --- EOF holds the last frame (does not blank). ---
  FrameResult eof = dec("clip", 999.0);
  REQUIRE(eof.ready());
  CHECK(eof.frame.width == w);

  // --- Backward seek rewinds and still yields a frame. ---
  CHECK(dec("clip", 0.5).ready());
  FrameResult back = dec("clip", 0.0);
  REQUIRE(back.ready());
  CHECK(back.frame.width == w);

  // --- Pending resolver -> Pending (retry next frame). ---
  CHECK(make([](const std::string &, AssetKind) {
            return AssetResult::makePending();
          })("later", 0.0)
            .pending());

  // --- Failed / missing -> Failed. ---
  CHECK(make([](const std::string &, AssetKind) {
            return AssetResult::makeFailed();
          })("missing", 0.0)
            .failed());

  // --- Garbage bytes -> Failed, and cached (no crash on retry). ---
  auto garbage = make(readyResolver({1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));
  CHECK(garbage("junk", 0.0).failed());
  CHECK(garbage("junk", 0.1).failed());

  // --- Null resolver never crashes, always Failed. ---
  CHECK(make(AssetResolver{})("x", 0.0).failed());
}

} // namespace

TEST_CASE("MovieDecoder contract: pl_mpeg / MPEG-1 (Backend A)") {
  runContract(
      [](AssetResolver r) {
        return x3d::runtime::io::plmpeg::makePlMpegMovieDecoder(std::move(r));
      },
      PLMPEG_FIXTURES_DIR, "redsquare.m1v", 32, 32);
}

#ifdef X3D_MOVIE_HAVE_THEORA
TEST_CASE("MovieDecoder contract: libtheora / Ogg-Theora (Backend B)") {
  runContract(
      [](AssetResolver r) {
        return x3d::runtime::io::theora::makeTheoraMovieDecoder(std::move(r));
      },
      THEORA_FIXTURES_DIR, "redsquare.ogv", 32, 32);
}
#endif

TEST_CASE("MovieDecoder: the default null decoder always fails safe") {
  MovieDecoder d = makeNullMovieDecoder();
  CHECK(d("anything", 0.0).failed());
}
