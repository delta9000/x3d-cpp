// runtime/io/wuffs/WuffsTextureResolver.cpp — the ONE translation unit where
// the wuffs amalgamation is compiled (WUFFS_IMPLEMENTATION) and meets the
// TextureResolver seam. The wuffs header is included PRIVATE (see CMake) and
// WUFFS_IMPLEMENTATION is defined nowhere else, so the 1.8 MB codec body lives
// in exactly one object file and never leaks to consumers. The factory returns
// a plain std::function exchanging only std types.
#include "WuffsTextureResolver.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

// Compile the whole wuffs library (all modules, incl. the C++ wuffs_aux image
// decode convenience API) into this TU. Including the amalgamation as C++ is
// what enables wuffs_aux.
#define WUFFS_IMPLEMENTATION
#include "vendor/wuffs-v0.3.c"

namespace x3d::runtime::io::wuffs {

namespace {

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

// Force the destination pixel format to RGBA8 straight-alpha so the decoded
// bytes match stb_image's stbi_load(..., 4) output channel-for-channel. The
// default wuffs_aux SelectPixfmt returns BGRA, which would never be byte-equal.
class Rgba8Callbacks final : public wuffs_aux::DecodeImageCallbacks {
 public:
  wuffs_base__pixel_format SelectPixfmt(
      const wuffs_base__image_config& /*image_config*/) override {
    return wuffs_base__make_pixel_format(
        WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL);
  }
};

}  // namespace

x3d::runtime::extract::TextureResolver makeWuffsTextureResolver() {
  using x3d::runtime::extract::TexturePixelResult;
  using x3d::runtime::extract::TexturePixels;

  return [](const std::string& url) -> TexturePixelResult {
    std::vector<std::uint8_t> bytes;
    if (!readFile(url, bytes) || bytes.empty()) {
      return TexturePixelResult::makeFailed();
    }

    Rgba8Callbacks callbacks;
    wuffs_aux::sync_io::MemoryInput input(bytes.data(), bytes.size());
    wuffs_aux::DecodeImageResult result = wuffs_aux::DecodeImage(callbacks,
                                                                 input);
    if (!result.error_message.empty()) {
      return TexturePixelResult::makeFailed();
    }

    const std::uint32_t w = result.pixbuf.pixcfg.width();
    const std::uint32_t h = result.pixbuf.pixcfg.height();
    wuffs_base__table_u8 tab = result.pixbuf.plane(0);
    if (w == 0 || h == 0 || tab.ptr == nullptr) {
      return TexturePixelResult::makeFailed();
    }

    TexturePixels px;
    px.width = w;
    px.height = h;
    px.rgba.resize(static_cast<std::size_t>(w) *
                   static_cast<std::size_t>(h) * 4u);

    // wuffs decodes top-left origin; the seam contract is bottom-left, so copy
    // source row r into destination row (h-1-r). The source plane may be padded
    // (tab.stride >= w*4); the destination is tightly packed (stride = w*4).
    const std::size_t row_bytes = static_cast<std::size_t>(w) * 4u;
    for (std::uint32_t r = 0; r < h; ++r) {
      const std::uint8_t* src = tab.ptr + static_cast<std::size_t>(r) *
                                              static_cast<std::size_t>(tab.stride);
      std::uint8_t* dst = px.rgba.data() +
                          static_cast<std::size_t>(h - 1u - r) * row_bytes;
      std::copy(src, src + row_bytes, dst);
    }

    return TexturePixelResult::makeReady(std::move(px));
  };
}

}  // namespace x3d::runtime::io::wuffs
