// SFImage component counts are clamped to [0,4] before pixel unpacking (§5.3.6).

#include "FieldValueIO.hpp"

#include "doctest/doctest.h"

using x3d::codec::parseImage;

TEST_CASE("sfimage_clamps_out_of_range_numComponents") {
  // The §5.3.6 limit of four components bounds a one-pixel image to four bytes.
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
