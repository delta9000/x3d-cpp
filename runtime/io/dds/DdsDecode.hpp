// runtime/io/dds/DdsDecode.hpp — std-only DirectDraw Surface (DDS) decoder for
// the TextureResolver decode backends (stb, wuffs). Header-only, no codec
// dependency. §34.4.3 ImageCubeMapTexture recommends DDS for cube maps; a 2D
// DDS also decodes, so an ImageTexture may name one.
//
// Supported: the first mip level of 2D textures and cube maps in
//   * uncompressed RGB(A) / luminance(+alpha) with any 8-bit channel masks,
//     8, 16, 24 or 32 bits per pixel;
//   * BC1 (DXT1), BC2 (DXT3) and BC3 (DXT5), by FourCC or a DX10 header
//     (DXGI 71/72, 74/75, 77/78, and RGBA8/BGRA8/BGRX8 28/29, 87/91, 88/93).
// Volume textures, texture arrays and other formats return nullopt.
//
// Output follows the seam contract (TextureResolver.hpp): RGBA8, bottom-left
// origin, so rows are flipped from DDS's top-first order. A cube map yields
// layers = 6 in X3D face order front, back, left, right, top, bottom. DDS
// cube faces are laid out for a left-handed space (+X, -X, +Y, -Y, +Z, -Z);
// X3D is right-handed, so mirroring z maps them as front = +Z, back = -Z,
// left = -X, right = +X, top = +Y, bottom = -Y, each face then upright as seen
// from the cube's centre (Figure 34.1). An absent face in a partial cube map is
// opaque white. Pinned by texture_decode_tests (DDS cases).
#ifndef X3D_RUNTIME_IO_DDS_DDS_DECODE_HPP
#define X3D_RUNTIME_IO_DDS_DDS_DECODE_HPP

#include "TextureResolver.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace x3d::runtime::io::dds {

inline bool isDds(const std::uint8_t *b, std::size_t n) {
  return n >= 4 && b[0] == 'D' && b[1] == 'D' && b[2] == 'S' && b[3] == ' ';
}

namespace detail {

inline std::uint32_t u32(const std::uint8_t *p) {
  return static_cast<std::uint32_t>(p[0]) | static_cast<std::uint32_t>(p[1]) << 8 |
         static_cast<std::uint32_t>(p[2]) << 16 | static_cast<std::uint32_t>(p[3]) << 24;
}

// 8-bit value of the channel `mask` selects in `pixel` (0 when the mask is 0).
inline std::uint8_t channel(std::uint32_t pixel, std::uint32_t mask) {
  if (mask == 0) return 0;
  int shift = 0;
  while (!((mask >> shift) & 1u)) ++shift;
  const std::uint32_t max = mask >> shift;
  const std::uint32_t v = (pixel & mask) >> shift;
  return static_cast<std::uint8_t>((v * 255u + max / 2) / max);
}

inline void rgb565(std::uint16_t c, std::uint8_t out[4]) {
  out[0] = static_cast<std::uint8_t>(((c >> 11) & 31) * 255 / 31);
  out[1] = static_cast<std::uint8_t>(((c >> 5) & 63) * 255 / 63);
  out[2] = static_cast<std::uint8_t>((c & 31) * 255 / 31);
  out[3] = 255;
}

// Decode one 4x4 BC1/BC2/BC3 block into `rgba` (16 texels, row-major, top row
// first). `kind` is 1, 2 or 3.
inline void decodeBlock(const std::uint8_t *block, int kind, std::uint8_t rgba[64]) {
  const std::uint8_t *colour = kind == 1 ? block : block + 8;
  const std::uint16_t c0 = static_cast<std::uint16_t>(colour[0] | colour[1] << 8);
  const std::uint16_t c1 = static_cast<std::uint16_t>(colour[2] | colour[3] << 8);
  std::uint8_t palette[4][4];
  rgb565(c0, palette[0]);
  rgb565(c1, palette[1]);
  const bool fourColour = kind != 1 || c0 > c1;
  for (int c = 0; c < 3; ++c) {
    if (fourColour) {
      palette[2][c] = static_cast<std::uint8_t>((2 * palette[0][c] + palette[1][c]) / 3);
      palette[3][c] = static_cast<std::uint8_t>((palette[0][c] + 2 * palette[1][c]) / 3);
    } else {
      palette[2][c] = static_cast<std::uint8_t>((palette[0][c] + palette[1][c]) / 2);
      palette[3][c] = 0;
    }
  }
  palette[2][3] = 255;
  palette[3][3] = fourColour ? 255 : 0; // BC1 three-colour mode: index 3 is clear.
  const std::uint32_t indices = u32(colour + 4);
  for (int i = 0; i < 16; ++i)
    std::copy_n(palette[(indices >> (2 * i)) & 3], 4, rgba + 4 * i);
  if (kind == 2) { // explicit 4-bit alpha
    for (int i = 0; i < 16; ++i) {
      const int nibble = (block[i / 2] >> (4 * (i & 1))) & 15;
      rgba[4 * i + 3] = static_cast<std::uint8_t>(nibble * 17);
    }
  } else if (kind == 3) { // interpolated alpha
    const int a0 = block[0], a1 = block[1];
    int alpha[8] = {a0, a1};
    for (int i = 1; i < 7; ++i)
      alpha[i + 1] = a0 > a1 ? ((7 - i) * a0 + i * a1) / 7
                   : i < 5   ? ((5 - i) * a0 + i * a1) / 5
                   : i == 5  ? 0
                             : 255;
    std::uint64_t bits = 0;
    for (int i = 0; i < 6; ++i) bits |= static_cast<std::uint64_t>(block[2 + i]) << (8 * i);
    for (int i = 0; i < 16; ++i)
      rgba[4 * i + 3] = static_cast<std::uint8_t>(alpha[(bits >> (3 * i)) & 7]);
  }
}

} // namespace detail

// Decode DDS bytes; nullopt when the file is malformed or the format is not
// supported.
inline std::optional<x3d::runtime::extract::TexturePixels>
decodeDds(const std::uint8_t *b, std::size_t n) {
  using detail::u32;
  if (!isDds(b, n) || n < 128 || u32(b + 4) != 124 || u32(b + 76) != 32)
    return std::nullopt;
  const std::uint32_t height = u32(b + 12), width = u32(b + 16);
  const std::uint32_t mipCount = std::max<std::uint32_t>(1, u32(b + 28));
  const std::uint32_t pfFlags = u32(b + 80), fourCC = u32(b + 84);
  const std::uint32_t bits = u32(b + 88);
  const std::uint32_t masks[4] = {u32(b + 92), u32(b + 96), u32(b + 100), u32(b + 104)};
  const std::uint32_t caps2 = u32(b + 112);
  if (width == 0 || height == 0 || width > 16384 || height > 16384) return std::nullopt;
  if (caps2 & 0x200000u) return std::nullopt; // volume texture

  constexpr std::uint32_t kAlphaPixels = 0x1, kFourCC = 0x4, kRgb = 0x40,
                          kLuminance = 0x20000;
  auto fourcc = [](const char *s) { return u32(reinterpret_cast<const std::uint8_t *>(s)); };
  std::size_t offset = 128;
  int block = 0; // 0 uncompressed, else BC kind 1..3
  std::uint32_t bpp = bits;
  std::uint32_t m[4] = {masks[0], masks[1], masks[2],
                        (pfFlags & kAlphaPixels) ? masks[3] : 0u};
  bool cube = (caps2 & 0x200u) != 0;
  std::uint32_t faceMask = cube ? (caps2 & 0xFC00u) : 0;
  if (pfFlags & kFourCC) {
    if (fourCC == fourcc("DXT1")) block = 1;
    else if (fourCC == fourcc("DXT2") || fourCC == fourcc("DXT3")) block = 2;
    else if (fourCC == fourcc("DXT4") || fourCC == fourcc("DXT5")) block = 3;
    else if (fourCC == fourcc("DX10")) {
      if (n < 148) return std::nullopt;
      const std::uint32_t dxgi = u32(b + 128), dimension = u32(b + 132);
      const std::uint32_t misc = u32(b + 136), arraySize = u32(b + 140);
      offset = 148;
      if (dimension != 3 || arraySize != 1) return std::nullopt; // 2D, not an array
      if (misc & 0x4u) { cube = true; faceMask = 0xFC00u; }
      switch (dxgi) {
        case 71: case 72: block = 1; break;
        case 74: case 75: block = 2; break;
        case 77: case 78: block = 3; break;
        case 28: case 29:
          bpp = 32; m[0] = 0xFFu; m[1] = 0xFF00u; m[2] = 0xFF0000u; m[3] = 0xFF000000u; break;
        case 87: case 91:
          bpp = 32; m[0] = 0xFF0000u; m[1] = 0xFF00u; m[2] = 0xFFu; m[3] = 0xFF000000u; break;
        case 88: case 93:
          bpp = 32; m[0] = 0xFF0000u; m[1] = 0xFF00u; m[2] = 0xFFu; m[3] = 0; break;
        default: return std::nullopt;
      }
    } else {
      return std::nullopt;
    }
  } else if (!(pfFlags & (kRgb | kLuminance)) ||
             (bpp != 8 && bpp != 16 && bpp != 24 && bpp != 32)) {
    return std::nullopt;
  }
  const bool luminance = !block && (pfFlags & kLuminance) && !(pfFlags & kRgb);

  auto levelBytes = [&](std::uint32_t w, std::uint32_t h) -> std::size_t {
    if (block)
      return static_cast<std::size_t>((w + 3) / 4) * ((h + 3) / 4) * (block == 1 ? 8 : 16);
    return static_cast<std::size_t>(w) * h * (bpp / 8);
  };
  std::size_t faceBytes = 0; // one face's whole mip chain
  for (std::uint32_t level = 0, w = width, h = height; level < mipCount; ++level) {
    faceBytes += levelBytes(w, h);
    w = std::max<std::uint32_t>(1, w / 2);
    h = std::max<std::uint32_t>(1, h / 2);
  }

  // Decode the top level at `data` into bottom-up RGBA8 rows at `out`.
  auto decodeLevel = [&](const std::uint8_t *data, std::uint8_t *out) {
    auto put = [&](std::uint32_t x, std::uint32_t y, const std::uint8_t *px) {
      std::copy_n(px, 4, out + ((static_cast<std::size_t>(height - 1 - y)) * width + x) * 4);
    };
    if (block) {
      std::uint8_t texels[64];
      const std::size_t stride = block == 1 ? 8 : 16;
      for (std::uint32_t by = 0; by < (height + 3) / 4; ++by)
        for (std::uint32_t bx = 0; bx < (width + 3) / 4; ++bx) {
          detail::decodeBlock(data + (by * ((width + 3) / 4) + bx) * stride, block, texels);
          for (std::uint32_t i = 0; i < 16; ++i) {
            const std::uint32_t x = bx * 4 + i % 4, y = by * 4 + i / 4;
            if (x < width && y < height) put(x, y, texels + 4 * i);
          }
        }
      return;
    }
    const std::uint32_t bytes = bpp / 8;
    for (std::uint32_t y = 0; y < height; ++y)
      for (std::uint32_t x = 0; x < width; ++x) {
        const std::uint8_t *p = data + (static_cast<std::size_t>(y) * width + x) * bytes;
        std::uint32_t pixel = 0;
        for (std::uint32_t k = 0; k < bytes; ++k) pixel |= static_cast<std::uint32_t>(p[k]) << (8 * k);
        std::uint8_t px[4];
        if (luminance) {
          px[0] = px[1] = px[2] = detail::channel(pixel, m[0]);
        } else {
          for (int c = 0; c < 3; ++c) px[c] = detail::channel(pixel, m[c]);
        }
        px[3] = m[3] ? detail::channel(pixel, m[3]) : 255;
        put(x, y, px);
      }
  };

  x3d::runtime::extract::TexturePixels result;
  result.width = width;
  result.height = height;
  const std::size_t layerBytes = static_cast<std::size_t>(width) * height * 4;
  if (!cube) {
    if (offset + levelBytes(width, height) > n) return std::nullopt;
    result.rgba.resize(layerBytes);
    decodeLevel(b + offset, result.rgba.data());
    return result;
  }
  // Faces are stored +X, -X, +Y, -Y, +Z, -Z (those present); the X3D layer
  // each becomes is front, back, left, right, top, bottom = +Z, -Z, -X, +X, +Y, -Y.
  static constexpr int kLayerOfDdsFace[6] = {3, 2, 4, 5, 0, 1};
  result.layers = 6;
  result.rgba.assign(layerBytes * 6, 255);
  for (int face = 0; face < 6; ++face) {
    if (!(faceMask & (0x400u << face))) continue;
    if (offset + levelBytes(width, height) > n) return std::nullopt;
    decodeLevel(b + offset, result.rgba.data() + layerBytes * kLayerOfDdsFace[face]);
    offset += faceBytes;
  }
  return result;
}

} // namespace x3d::runtime::io::dds

#endif // X3D_RUNTIME_IO_DDS_DDS_DECODE_HPP
