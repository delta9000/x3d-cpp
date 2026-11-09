// Compile Wuffs in this translation unit; decoder headers stay private.
// See ADR-0024.
#include "WuffsTextureResolver.hpp"
#include "../ReadFile.hpp"
#include "DdsDecode.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

// Compile the whole wuffs library (all modules, incl. the C++ wuffs_aux image
// decode convenience API) into this TU. Including the amalgamation as C++ is
// what enables wuffs_aux.
#define WUFFS_IMPLEMENTATION
#include "vendor/wuffs-v0.3.c"

namespace x3d::runtime::io::wuffs {

namespace {

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
    if (!::x3d::runtime::io::detail::readFile(url, bytes) || bytes.empty()) {
      return TexturePixelResult::makeFailed();
    }
    // DDS: the same std-only decoder as the stb backend (wuffs has none).
    if (io::dds::isDds(bytes.data(), bytes.size())) {
      auto px = io::dds::decodeDds(bytes.data(), bytes.size());
      return px ? TexturePixelResult::makeReady(std::move(*px))
                : TexturePixelResult::makeFailed();
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
