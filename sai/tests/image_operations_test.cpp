#include "x3d/sai/experimental/image.hpp"
#include "doctest/doctest.h"

#include <array>
#include <limits>

namespace sai = x3d::sai::experimental;

TEST_CASE("packed image pixels preserve channel order and unsigned depth") {
  const std::array<std::uint32_t, 2> values{0x12345678u, 0xff000080u};
  for (int components = 1; components <= 4; ++components) {
    const auto mask = components == 4 ? 0xffffffffu :
                       (std::uint32_t{1} << (8 * components)) - 1;
    const std::array<std::uint32_t, 2> pixels{values[0] & mask, values[1] & mask};
    auto image = sai::image_from_pixels(2, 1, components, pixels);
    REQUIRE(image);
    REQUIRE(image->data.size() == 2u * components);
    CHECK(image->data[0] == ((pixels[0] >> (8 * (components - 1))) & 255));
    CHECK(image->data[components - 1] == (pixels[0] & 255));
    CHECK(sai::image_pixels(*image).value() ==
          std::vector<std::uint32_t>{pixels.begin(), pixels.end()});
  }
  const std::array<std::uint32_t, 1> too_large{256};
  CHECK(sai::image_from_pixels(1, 1, 1, too_large).error().code ==
        sai::error_code::invalid_value);
  CHECK(sai::image_from_pixels(-1, 1, 1, {}).error().code ==
        sai::error_code::invalid_value);
  CHECK_FALSE(sai::image_from_pixels(1, 1, 0, too_large));
  CHECK_FALSE(sai::image_from_pixels(1, 1, 5, too_large));
  CHECK_FALSE(sai::image_from_pixels(1, 2, 1, too_large));
  CHECK_FALSE(sai::image_from_pixels(INT32_MAX, INT32_MAX, 4, {}));
  const auto empty = sai::image_from_pixels(0, 0, 0, {});
  REQUIRE(empty);
  CHECK(sai::image_pixels(*empty)->empty());
  CHECK_FALSE(sai::image_pixels(sai::image{1, 1, 0, {}}));
  CHECK_FALSE(sai::image_pixels(sai::image{1, 1, 3, {1, 2}}));
}

TEST_CASE("subimage clipping matches a pixel-coordinate oracle") {
  const std::array<std::uint32_t, 6> source_pixels{1, 2, 3, 4, 5, 6};
  const std::array<std::uint32_t, 6> destination_pixels{11, 12, 13, 14, 15, 16};
  for (int components = 1; components <= 4; ++components) {
    const auto source = sai::image_from_pixels(3, 2, components, source_pixels).value();
    const auto destination = sai::image_from_pixels(2, 3, components, destination_pixels).value();
    for (int sx = -2; sx <= 3; ++sx)
      for (int sy = -2; sy <= 2; ++sy)
        for (int dx = -2; dx <= 2; ++dx)
          for (int dy = -2; dy <= 3; ++dy) {
            auto expected = std::vector<std::uint32_t>{destination_pixels.begin(), destination_pixels.end()};
            for (int y = 0; y < 4; ++y)
              for (int x = 0; x < 4; ++x)
                if (sx + x >= 0 && sx + x < 3 && sy + y >= 0 && sy + y < 2 &&
                    dx + x >= 0 && dx + x < 2 && dy + y >= 0 && dy + y < 3)
                  expected[(dy + y) * 2 + dx + x] = source_pixels[(sy + y) * 3 + sx + x];
            const auto copied = sai::with_sub_image(destination, source, 4, 4, sx, sy, dx, dy);
            REQUIRE(copied);
            CHECK(copied->width == 2);
            CHECK(copied->height == 3);
            CHECK(copied->components == components);
            CHECK(sai::image_pixels(*copied).value() == expected);
            CHECK(sai::image_pixels(destination).value() ==
                  std::vector<std::uint32_t>{destination_pixels.begin(), destination_pixels.end()});
          }
  }
}

TEST_CASE("subimage self-copy, boundaries and errors preserve owning inputs") {
  const std::array<std::uint32_t, 6> pixels{1, 2, 3, 4, 5, 6};
  const auto image = sai::image_from_pixels(3, 2, 1, pixels).value();
  const auto copied = sai::with_sub_image(image, image, 3, 2, 0, 0, 1, 0).value();
  CHECK(sai::image_pixels(copied).value() == std::vector<std::uint32_t>{1, 1, 2, 4, 4, 5});
  CHECK(sai::image_pixels(image).value() == std::vector<std::uint32_t>{pixels.begin(), pixels.end()});
  CHECK(sai::with_sub_image(image, image, 0, 0, 0, 0, 0, 0)->data == image.data);
  CHECK(sai::with_sub_image(image, image, INT32_MAX, INT32_MAX,
                            INT32_MIN, INT32_MIN, INT32_MAX, INT32_MAX)->data == image.data);
  CHECK_FALSE(sai::with_sub_image(image, image, -1, 1, 0, 0, 0, 0));
  auto malformed = image;
  malformed.data.pop_back();
  CHECK_FALSE(sai::with_sub_image(image, malformed, 1, 1, 0, 0, 0, 0));
  CHECK_FALSE(sai::with_sub_image(malformed, image, 1, 1, 0, 0, 0, 0));
  const auto rgba = sai::image_from_pixels(3, 2, 4, pixels).value();
  CHECK(sai::with_sub_image(image, rgba, 1, 1, 0, 0, 0, 0).error().code ==
        sai::error_code::type_mismatch);
}
