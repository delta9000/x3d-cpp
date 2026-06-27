// sfimage_overflow_test.cpp
// Regression for SEC-2 (#12): a hostile SFImage/PixelTexture token with an
// out-of-range numComponents.
//
// X3D §5.3.6 caps numComponents at [0,4]. The byte-unpacking loop in
// parseImageFrom() drove an inner `packed >> (8*b)` with attacker-controlled
// `b` AND pushed numComponents bytes per pixel, so a value like 2_000_000_000
// both overflowed the signed shift count (UBSan: `1999999999 * 8`) and tried to
// push multiple GB from a single one-pixel token.
//
// Post-fix: numComponents is clamped to the spec range [0,4] before the loop,
// so the emitted byte count is bounded no matter what the token claims.
#include "FieldValueIO.hpp"

#include "doctest/doctest.h"

using x3d::codec::parseImage;

TEST_CASE("sfimage_clamps_out_of_range_numComponents") {
  // 1x1 image claiming 5 components/pixel (spec max is 4). Pre-fix the loop
  // emitted 5 bytes; post-fix it is clamped to <= 4.
  auto img = parseImage("1 1 5 0xFFFFFFFFFF");
  CHECK(img.data.size() <= 4);
}

TEST_CASE("sfimage_hostile_numComponents_does_not_over_allocate") {
  // The reported payload: one pixel claiming ~2e9 components. Bounded output and
  // no multi-GB allocation / signed-overflow shift.
  auto img = parseImage("1 1 2000000000 0xFF");
  CHECK(img.data.size() <= 4);
}

TEST_CASE("sfimage_valid_rgba_pixel_still_unpacks") {
  // Regression guard: a well-formed 1x1 RGBA pixel still unpacks to 4 bytes,
  // most-significant component first (R,G,B,A).
  auto img = parseImage("1 1 4 0x11223344");
  REQUIRE(img.data.size() == 4);
  CHECK(img.data[0] == 0x11);
  CHECK(img.data[1] == 0x22);
  CHECK(img.data[2] == 0x33);
  CHECK(img.data[3] == 0x44);
}
