// runtime/io/plmpeg/tests/movie_decoder_tests.cpp — the MovieDecoder seam
// genericity + pluggability proof (ADR-0041).
//
// Video codecs PARTITION by format — no two backends decode the same input — so
// the proof is NOT a bit-identical swap (the font/texture pattern) but a SHARED
// per-backend semantics-CONTRACT: each backend, given its OWN reference clip, must
// agree on the observable seam behavior. runContract() encodes that contract and
// is run against EVERY plug:
//   * pl_mpeg / MPEG-1                (Backend A, always)
//   * libtheora / Ogg-Theora         (Backend B, when built)
//   * a FROM-SCRATCH std-only backend (proves the interface needs nothing but the
//                                      public header + std to implement)
//   * the multi-format composer        (proves backends COMPOSE behind one seam)
// Plus pluggability stress: each backend rejects a FOREIGN container cleanly, and
// truncated input never crashes — so a dispatcher/consumer can trust Failed.
//
// ABI ISOLATION: this TU talks ONLY through the seam factories (decoder-free
// headers); it includes NO pl_mpeg / theora / ogg header.
#include "MovieDecoder.hpp"
#include "MultiFormatMovieDecoder.hpp"
#include "PlMpegMovieDecoder.hpp"
#include "doctest/doctest.h"

#ifdef X3D_MOVIE_HAVE_THEORA
#include "TheoraMovieDecoder.hpp"
#endif

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
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
// across backends; only the reference bytes + their decoded dimensions differ.
using Backend = std::function<MovieDecoder(AssetResolver)>;

void runContract(const Backend &make, const std::vector<std::uint8_t> &validBytes,
                 unsigned w, unsigned h) {
  REQUIRE(!validBytes.empty());

  // --- Ready: a tight, opaque, correctly-sized frame. ---
  auto dec = make(readyResolver(validBytes));
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

  // --- Backward seek (incl. straight after EOF) rewinds and still yields a frame. ---
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

// A MovieDecoder written FROM SCRATCH against ONLY the public header + std — no
// codec, no external dependency. "Valid" input begins with the ASCII magic
// "FAKEMOV1"; anything else (garbage / wrong format) -> Failed. It honors the
// resolver tri-state and produces a solid-red w×h opaque frame. If this passes the
// same runContract as the real codecs, the seam carries zero codec coupling.
MovieDecoder makeFakeMovieDecoder(AssetResolver resolver, unsigned w, unsigned h) {
  struct State { bool decided = false; bool ok = false; };
  auto states = std::make_shared<std::map<std::string, State>>();
  static const std::uint8_t kMagic[8] = {'F', 'A', 'K', 'E', 'M', 'O', 'V', '1'};
  return [resolver, states, w, h](const std::string &url,
                                  double) -> FrameResult {
    State &s = (*states)[url];
    if (!s.decided) {
      if (!resolver) { s.decided = true; s.ok = false; return FrameResult::makeFailed(); }
      AssetResolver r = resolver;
      AssetResult a = r(url, AssetKind::Movie);
      if (a.pending()) return FrameResult::makePending(); // not decided: retry.
      s.ok = a.ready() && a.bytes.size() >= 8 &&
             std::equal(kMagic, kMagic + 8, a.bytes.begin());
      s.decided = true;
    }
    if (!s.ok) return FrameResult::makeFailed();
    VideoFrame f;
    f.width = w;
    f.height = h;
    f.rgba.assign(static_cast<std::size_t>(w) * h * 4, 0);
    for (std::size_t i = 0; i < static_cast<std::size_t>(w) * h; ++i) {
      f.rgba[i * 4 + 0] = 200; // red dominant
      f.rgba[i * 4 + 3] = 255; // opaque
    }
    return FrameResult::makeReady(std::move(f));
  };
}

std::vector<std::uint8_t> fakeBytes() {
  std::vector<std::uint8_t> b = {'F', 'A', 'K', 'E', 'M', 'O', 'V', '1'};
  b.resize(64, 0); // magic + padding
  return b;
}

} // namespace

TEST_CASE("MovieDecoder contract: pl_mpeg / MPEG-1 (Backend A)") {
  runContract(
      [](AssetResolver r) {
        return x3d::runtime::io::plmpeg::makePlMpegMovieDecoder(std::move(r));
      },
      readFixture(PLMPEG_FIXTURES_DIR, "redsquare.m1v"), 32, 32);
}

#ifdef X3D_MOVIE_HAVE_THEORA
TEST_CASE("MovieDecoder contract: libtheora / Ogg-Theora (Backend B)") {
  runContract(
      [](AssetResolver r) {
        return x3d::runtime::io::theora::makeTheoraMovieDecoder(std::move(r));
      },
      readFixture(THEORA_FIXTURES_DIR, "redsquare.ogv"), 32, 32);
}
#endif

TEST_CASE("MovieDecoder contract: a FROM-SCRATCH std-only backend") {
  // The seam is implementable by any downstream with only the header + std.
  runContract([](AssetResolver r) { return makeFakeMovieDecoder(std::move(r), 8, 8); },
              fakeBytes(), 8, 8);
}

TEST_CASE("MovieDecoder: the multi-format composer is itself a conforming backend") {
  // The composer (sniff -> dispatch) must satisfy the SAME contract end-to-end.
  auto m1v = readFixture(PLMPEG_FIXTURES_DIR, "redsquare.m1v");
  Backend composed = [](AssetResolver r) {
    std::map<MovieFormat, MovieDecoder> backends;
    backends[MovieFormat::Mpeg1] =
        x3d::runtime::io::plmpeg::makePlMpegMovieDecoder(r);
    return makeMultiFormatMovieDecoder(r, std::move(backends));
  };
  runContract(composed, m1v, 32, 32);
}

TEST_CASE("MovieDecoder: composer routes each container to its backend by magic") {
  auto m1v = readFixture(PLMPEG_FIXTURES_DIR, "redsquare.m1v");
  std::map<std::string, std::vector<std::uint8_t>> files;
  files["a.m1v"] = m1v;
#ifdef X3D_MOVIE_HAVE_THEORA
  files["b.ogv"] = readFixture(THEORA_FIXTURES_DIR, "redsquare.ogv");
#endif
  // One resolver serves every url/format (keyed by url).
  AssetResolver multi = [files](const std::string &u, AssetKind) {
    auto it = files.find(u);
    return it != files.end() ? AssetResult::makeReady(it->second)
                             : AssetResult::makeFailed();
  };
  std::map<MovieFormat, MovieDecoder> backends;
  backends[MovieFormat::Mpeg1] =
      x3d::runtime::io::plmpeg::makePlMpegMovieDecoder(multi);
#ifdef X3D_MOVIE_HAVE_THEORA
  backends[MovieFormat::OggTheora] =
      x3d::runtime::io::theora::makeTheoraMovieDecoder(multi);
#endif
  auto dec = makeMultiFormatMovieDecoder(multi, std::move(backends));

  FrameResult a = dec("a.m1v", 0.0); // MPEG magic -> pl_mpeg
  REQUIRE(a.ready());
  CHECK(a.frame.width == 32);
#ifdef X3D_MOVIE_HAVE_THEORA
  FrameResult b = dec("b.ogv", 0.0); // Ogg magic -> theora
  REQUIRE(b.ready());
  CHECK(b.frame.width == 32);
#endif
  CHECK(dec("nope", 0.0).failed()); // unreadable -> Failed (not a routing crash)
}

TEST_CASE("MovieDecoder: each backend rejects a FOREIGN container cleanly") {
  // A backend handed someone else's format must Failed, not crash or mis-decode —
  // this is what lets the magic-sniff composer (and any consumer) trust Failed.
#ifdef X3D_MOVIE_HAVE_THEORA
  auto m1v = readFixture(PLMPEG_FIXTURES_DIR, "redsquare.m1v");
  auto ogv = readFixture(THEORA_FIXTURES_DIR, "redsquare.ogv");
  CHECK(x3d::runtime::io::plmpeg::makePlMpegMovieDecoder(readyResolver(ogv))("x", 0.0)
            .failed());
  CHECK(x3d::runtime::io::theora::makeTheoraMovieDecoder(readyResolver(m1v))("x", 0.0)
            .failed());
#endif
}

TEST_CASE("MovieDecoder: truncated input fails without crashing") {
  auto m1v = readFixture(PLMPEG_FIXTURES_DIR, "redsquare.m1v");
  for (std::size_t k : {std::size_t{1}, std::size_t{4}, std::size_t{16},
                        std::size_t{64}, m1v.size() / 2}) {
    std::vector<std::uint8_t> trunc(m1v.begin(),
                                    m1v.begin() + std::min(k, m1v.size()));
    FrameResult r =
        x3d::runtime::io::plmpeg::makePlMpegMovieDecoder(readyResolver(trunc))("x",
                                                                               0.0);
    CHECK((r.failed() || r.ready())); // defined result, never a crash/UB.
  }
}

TEST_CASE("MovieDecoder: the default null decoder always fails safe") {
  MovieDecoder d = makeNullMovieDecoder();
  CHECK(d("anything", 0.0).failed());
}
