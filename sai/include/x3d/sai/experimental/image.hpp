#ifndef X3D_SAI_EXPERIMENTAL_IMAGE_HPP
#define X3D_SAI_EXPERIMENTAL_IMAGE_HPP

#include "types.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <span>

namespace x3d::sai::experimental {

// Portable owning-image operations for the modern binding proposal. Pixels
// are left-to-right, bottom-to-top, with intensity/alpha or RGBA channel order.
// These operate on values; publishing the result uses the ordinary field
// setter and therefore preserves its access, buffering and event contracts.
namespace detail {
inline unexpected image_error(error_code code, const char *operation,
                              const char *message) {
  sai_error error;
  error.code = code;
  error.operation = operation;
  error.message = message;
  return failure(std::move(error));
}
inline bool image_layout_valid(const image &value) noexcept {
  if (value.width < 0 || value.height < 0 || value.components < 0 ||
      value.components > 4)
    return false;
  const auto pixels = std::uint64_t(value.width) * std::uint64_t(value.height);
  if (pixels && value.components == 0)
    return false;
  const auto bytes = pixels * std::uint64_t(value.components);
  return bytes <= std::numeric_limits<std::size_t>::max() &&
         bytes == value.data.size();
}
} // namespace detail

// SFImage's packed unsigned pixel representation. High bytes precede low
// bytes in the owning channel array, independently of host byte order.
inline result<image> image_from_pixels(std::int32_t width, std::int32_t height,
                                       std::int32_t components,
                                       std::span<const std::uint32_t> pixels) {
  constexpr auto operation = "image_from_pixels";
  if (width < 0 || height < 0 || components < 0 || components > 4 ||
      (width > 0 && height > 0 && components == 0))
    return detail::image_error(error_code::invalid_value, operation,
                               "invalid image dimensions or component count");
  const auto count = std::uint64_t(width) * std::uint64_t(height);
  const auto bytes = count * std::uint64_t(components);
  if (count != pixels.size() ||
      bytes > std::numeric_limits<std::size_t>::max())
    return detail::image_error(error_code::invalid_value, operation,
                               "pixel count does not match image dimensions");
  if (components < 4 && components > 0) {
    const auto maximum = (std::uint32_t{1} << (components * 8)) - 1;
    for (const auto pixel : pixels)
      if (pixel > maximum)
        return detail::image_error(error_code::invalid_value, operation,
                                   "packed pixel exceeds its component depth");
  }
  image output{width, height, components, {}};
  output.data.reserve(static_cast<std::size_t>(bytes));
  for (const auto pixel : pixels)
    for (std::int32_t channel = components - 1; channel >= 0; --channel)
      output.data.push_back(static_cast<std::uint8_t>(pixel >> (channel * 8)));
  return output;
}

inline result<std::vector<std::uint32_t>> image_pixels(const image &source) {
  if (!detail::image_layout_valid(source))
    return detail::image_error(error_code::invalid_value, "image_pixels",
                               "malformed owning image");
  const auto count = std::size_t(source.width) * std::size_t(source.height);
  std::vector<std::uint32_t> output(count, 0);
  for (std::size_t index = 0; index < count; ++index)
    for (std::int32_t channel = 0; channel < source.components; ++channel)
      output[index] = (output[index] << 8) |
                      source.data[index * source.components + channel];
  return output;
}

// C++ 7.7.6 setSubImage: copy without resizing or pixel-format conversion.
// Signed offsets clip against both source and destination bounds. Negative
// extents are rejected; zero extents are a no-op. The published text leaves
// these edge cases open; this is the SDK's explicit safety policy for them.
// Returning a new owning value also makes self-copy/overlap deterministic.
inline result<image> with_sub_image(const image &destination,
                                    const image &source,
                                    std::int32_t width, std::int32_t height,
                                    std::int32_t source_x, std::int32_t source_y,
                                    std::int32_t destination_x,
                                    std::int32_t destination_y) {
  constexpr auto operation = "with_sub_image";
  if (!detail::image_layout_valid(destination) ||
      !detail::image_layout_valid(source) || width < 0 || height < 0)
    return detail::image_error(error_code::invalid_value, operation,
                               "malformed image or negative subimage extent");
  if (destination.components != source.components)
    return detail::image_error(error_code::type_mismatch, operation,
                               "subimage pixel formats must match");
  image output = destination;
  // 64-bit coordinate arithmetic cannot overflow for any int32 argument.
  const auto begin_x = std::max({std::int64_t{0}, -std::int64_t(source_x),
                                  -std::int64_t(destination_x)});
  const auto begin_y = std::max({std::int64_t{0}, -std::int64_t(source_y),
                                  -std::int64_t(destination_y)});
  const auto end_x = std::min({std::int64_t(width),
      std::int64_t(source.width) - source_x,
      std::int64_t(destination.width) - destination_x});
  const auto end_y = std::min({std::int64_t(height),
      std::int64_t(source.height) - source_y,
      std::int64_t(destination.height) - destination_y});
  if (begin_x >= end_x || begin_y >= end_y)
    return output;
  const auto components = std::size_t(source.components);
  const auto row_bytes = std::size_t(end_x - begin_x) * components;
  for (auto y = begin_y; y < end_y; ++y) {
    const auto from = (std::size_t(source_y + y) * source.width +
                       std::size_t(source_x + begin_x)) * components;
    const auto to = (std::size_t(destination_y + y) * destination.width +
                     std::size_t(destination_x + begin_x)) * components;
    std::copy_n(source.data.begin() + from, row_bytes, output.data.begin() + to);
  }
  return output;
}

} // namespace x3d::sai::experimental
#endif
