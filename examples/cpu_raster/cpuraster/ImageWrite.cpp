// ImageWrite.cpp — the single stb_image_write translation unit for the CPU
// rasterizer (PNG output). Vendored locally; outside the SDK by design.
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "cpuraster/ImageWrite.hpp"

#include <cstddef>
#include <vector>

namespace x3d::cpuraster {

bool writePngRGB(const std::string &path, int width, int height,
                 const std::uint8_t *rgbaBottomUp) {
  if (width <= 0 || height <= 0 || rgbaBottomUp == nullptr) return false;
  std::vector<std::uint8_t> rgb(static_cast<std::size_t>(width) * height * 3);
  for (int y = 0; y < height; ++y) {
    const int src = height - 1 - y; // bottom-up framebuffer -> top-down PNG.
    for (int x = 0; x < width; ++x) {
      const std::size_t s = (static_cast<std::size_t>(src) * width + x) * 4;
      const std::size_t d = (static_cast<std::size_t>(y) * width + x) * 3;
      rgb[d] = rgbaBottomUp[s];
      rgb[d + 1] = rgbaBottomUp[s + 1];
      rgb[d + 2] = rgbaBottomUp[s + 2];
    }
  }
  return stbi_write_png(path.c_str(), width, height, 3, rgb.data(), width * 3) != 0;
}

} // namespace x3d::cpuraster
