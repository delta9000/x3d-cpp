// runtime/io/plmpeg/tests/movie_decoder_tests.cpp — Backend A (pl_mpeg) semantics
// contract for the MovieDecoder seam (ADR-0041).
//
// The seam's genericity proof is a PER-BACKEND semantics-contract test, not a
// bit-identical swap (video codecs partition by format — no two backends decode
// the same input). This asserts the observable seam behavior every backend must
// share: Pending-before-bytes, Ready frame dimensions + tight RGBA, EOF-holds-
// last-frame, backward-seek (rewind) parity, and Failed on bad/missing input.
//
// ABI ISOLATION: this TU talks ONLY through the seam factory (decoder-free header)
// and includes NO pl_mpeg header, so the codec cannot leak in.
#include "PlMpegMovieDecoder.hpp"
#include "doctest/doctest.h"

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

using namespace x3d::runtime::extract;
using x3d::runtime::io::plmpeg::makePlMpegMovieDecoder;

namespace {

std::vector<std::uint8_t> readFixture(const char *name) {
  std::string path = std::string(FIXTURES_DIR) + "/" + name;
  std::ifstream f(path, std::ios::binary);
  return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(f)),
                                   std::istreambuf_iterator<char>());
}

// A resolver that always returns the same Ready bytes.
AssetResolver readyResolver(std::vector<std::uint8_t> bytes) {
  return [bytes = std::move(bytes)](const std::string &, AssetKind) {
    return AssetResult::makeReady(bytes);
  };
}

} // namespace

TEST_CASE("plmpeg: raw elementary MPEG-1 decodes to a tight RGBA frame") {
  auto bytes = readFixture("redsquare.m1v");
  REQUIRE(!bytes.empty()); // fixture present
  auto dec = makePlMpegMovieDecoder(readyResolver(bytes));

  FrameResult r = dec("redsquare.m1v", 0.0);
  REQUIRE(r.ready());
  CHECK(r.frame.width == 32);
  CHECK(r.frame.height == 32);
  // Tightly packed RGBA8: exactly width*height*4 bytes.
  CHECK(r.frame.rgba.size() == static_cast<std::size_t>(32) * 32 * 4);

  // Content sanity: the clip is solid red, so the centre pixel's red channel
  // dominates green/blue (MPEG-1 is lossy YUV, so this is a loose check).
  const std::size_t centre = (static_cast<std::size_t>(16) * 32 + 16) * 4;
  CHECK(r.frame.rgba[centre + 0] > r.frame.rgba[centre + 1]);
  CHECK(r.frame.rgba[centre + 0] > r.frame.rgba[centre + 2]);
  CHECK(r.frame.rgba[centre + 3] == 255); // opaque alpha.
}

TEST_CASE("plmpeg: a media time past the end HOLDS the last frame") {
  auto bytes = readFixture("redsquare.m1v");
  REQUIRE(!bytes.empty());
  auto dec = makePlMpegMovieDecoder(readyResolver(bytes));

  CHECK(dec("redsquare.m1v", 0.0).ready());
  FrameResult eof = dec("redsquare.m1v", 999.0); // far past the 1s clip.
  REQUIRE(eof.ready());                          // holds, does not blank.
  CHECK(eof.frame.width == 32);
  CHECK(eof.frame.height == 32);
}

TEST_CASE("plmpeg: seeking backwards rewinds and still yields a frame") {
  auto bytes = readFixture("redsquare.m1v");
  REQUIRE(!bytes.empty());
  auto dec = makePlMpegMovieDecoder(readyResolver(bytes));

  CHECK(dec("redsquare.m1v", 0.8).ready()); // decode forward
  FrameResult back = dec("redsquare.m1v", 0.0); // then backwards -> rewind path
  REQUIRE(back.ready());
  CHECK(back.frame.width == 32);
}

TEST_CASE("plmpeg: Pending resolver surfaces Pending (retry next frame)") {
  auto dec = makePlMpegMovieDecoder(
      [](const std::string &, AssetKind) { return AssetResult::makePending(); });
  CHECK(dec("later.m1v", 0.0).pending());
}

TEST_CASE("plmpeg: Failed / missing bytes surface Failed") {
  auto dec = makePlMpegMovieDecoder(
      [](const std::string &, AssetKind) { return AssetResult::makeFailed(); });
  CHECK(dec("missing.m1v", 0.0).failed());

  // Garbage bytes are not a valid stream -> Failed (and cached, no crash on retry).
  auto garbage = makePlMpegMovieDecoder(readyResolver({1, 2, 3, 4, 5, 6, 7, 8}));
  CHECK(garbage("junk.m1v", 0.0).failed());
  CHECK(garbage("junk.m1v", 0.1).failed());
}

TEST_CASE("plmpeg: a null resolver never crashes, always Failed") {
  auto dec = makePlMpegMovieDecoder(AssetResolver{});
  CHECK(dec("anything.m1v", 0.0).failed());
}

TEST_CASE("MovieDecoder: the default null decoder always fails safe") {
  MovieDecoder d = makeNullMovieDecoder();
  CHECK(d("anything", 0.0).failed());
}
