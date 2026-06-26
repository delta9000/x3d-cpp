// runtime/io/stb/StbTextureResolver.cpp — the ONE translation unit where
// stb_image meets the TextureResolver seam. STB_IMAGE_IMPLEMENTATION is defined
// here and nowhere else; stb_image.h is included PRIVATE (see CMake) so it never
// leaks to consumers. The factory returns a plain std::function exchanging only
// std types, so linking x3d_stb pulls in no stb headers.
#include "StbTextureResolver.hpp"

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

// Decode-only: we never use stb's stdio file path (stbi_load); we read the file
// ourselves and decode from memory, identical to the wuffs backend, so the two
// backends differ only in the decoder and the swap-test isolates that variable.
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#include "stb_image.h"

namespace x3d::runtime::io::stb {

namespace {

// Read an entire local file into bytes. Returns false if the path cannot be
// opened (the resolver maps this to Failed).
bool readFile(const std::string& path, std::vector<std::uint8_t>& out) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f) {
    return false;
  }
  const std::streamoff size = f.tellg();
  if (size < 0) {
    return false;
  }
  out.resize(static_cast<std::size_t>(size));
  f.seekg(0);
  if (size > 0 && !f.read(reinterpret_cast<char*>(out.data()), size)) {
    return false;
  }
  return true;
}

}  // namespace

x3d::runtime::extract::TextureResolver makeStbTextureResolver() {
  using x3d::runtime::extract::TexturePixelResult;
  using x3d::runtime::extract::TexturePixels;

  return [](const std::string& url) -> TexturePixelResult {
    std::vector<std::uint8_t> bytes;
    if (!readFile(url, bytes) || bytes.empty()) {
      return TexturePixelResult::makeFailed();
    }

    // Bottom-left origin (GL convention) — the seam contract. stb returns
    // top-left by default, so flip on load to match wuffs' flipped output.
    stbi_set_flip_vertically_on_load(1);

    int w = 0;
    int h = 0;
    int channels_in_file = 0;
    stbi_uc* data = stbi_load_from_memory(
        bytes.data(), static_cast<int>(bytes.size()), &w, &h,
        &channels_in_file, /*desired_channels=*/4);
    if (data == nullptr || w <= 0 || h <= 0) {
      if (data != nullptr) {
        stbi_image_free(data);
      }
      return TexturePixelResult::makeFailed();
    }

    TexturePixels px;
    px.width = static_cast<std::uint32_t>(w);
    px.height = static_cast<std::uint32_t>(h);
    const std::size_t n = static_cast<std::size_t>(w) *
                          static_cast<std::size_t>(h) * 4u;
    px.rgba.assign(data, data + n);
    stbi_image_free(data);
    return TexturePixelResult::makeReady(std::move(px));
  };
}

}  // namespace x3d::runtime::io::stb
