// MultiFormatTextureResolver.hpp — std-only composer over the TextureResolver
// decode seam. namespace x3d::runtime::extract. Header-only, decoder-free, leaf.
//
// A single decoder backend rarely covers every format an embedder needs (stb is
// broad; wuffs is memory-safe; a JPEG-heavy app might want libjpeg-turbo). So a
// real TextureResolver is an ARRAY of per-format decoders behind one seam — the
// decode analog of the fetch seam's file/http/s3 routes.
//
// THE RULE (ADR-0024 §7, mirroring AssetResolver): route by a cheap key BEFORE
// calling a decoder, never use Failed to route. Fetch routes by URL scheme (a
// lexical prefix); decode routes by SNIFFED MAGIC BYTES (the URL extension can
// lie). Because dispatch picks the one correct decoder up front, each decoder
// only ever sees input it owns, so Failed keeps meaning "real decode failure" —
// no Unsupported/NotHandled status is needed and the frozen Ready/Pending/Failed
// seam type is unchanged. This composer is that thin std-only array, not a
// change to the seam.
#ifndef X3D_RUNTIME_EXTRACT_MULTI_FORMAT_TEXTURE_RESOLVER_HPP
#define X3D_RUNTIME_EXTRACT_MULTI_FORMAT_TEXTURE_RESOLVER_HPP

#include "TextureResolver.hpp"

#include <cstdint>
#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace x3d::runtime::extract {

// ---------------------------------------------------------------------------
// ImageFormat — the cheap routing key. One entry per decodable container, keyed
// off a magic-byte (or, for TGA, a structural) signature.
// ---------------------------------------------------------------------------
enum class ImageFormat { Png, Jpeg, Gif, Bmp, Pnm, Tga };

// ---------------------------------------------------------------------------
// sniffImageFormat — classify a byte prefix by signature. Decidable before any
// decoder is called. Returns nullopt for an unrecognizable blob.
//
// PNG/JPEG/GIF/BMP/PNM have leading magic numbers. TGA has none (its only
// optional signature is an *18-byte trailer*, "TRUEVISION-XFILE"), so it is
// classified last and only by a conservative header plausibility check — the
// same compromise stb_image and wuffs make. TGA is therefore the lowest-priority
// route; a blob that matches a real magic is never mistaken for TGA.
// ---------------------------------------------------------------------------
inline std::optional<ImageFormat> sniffImageFormat(
    const std::vector<std::uint8_t>& b) {
  const std::size_t n = b.size();

  // PNG: 89 50 4E 47 0D 0A 1A 0A
  if (n >= 8 && b[0] == 0x89 && b[1] == 0x50 && b[2] == 0x4E && b[3] == 0x47 &&
      b[4] == 0x0D && b[5] == 0x0A && b[6] == 0x1A && b[7] == 0x0A) {
    return ImageFormat::Png;
  }
  // JPEG: FF D8 FF
  if (n >= 3 && b[0] == 0xFF && b[1] == 0xD8 && b[2] == 0xFF) {
    return ImageFormat::Jpeg;
  }
  // GIF: "GIF87a" / "GIF89a" (prefix "GIF8" suffices to route)
  if (n >= 6 && b[0] == 'G' && b[1] == 'I' && b[2] == 'F' && b[3] == '8' &&
      (b[4] == '7' || b[4] == '9') && b[5] == 'a') {
    return ImageFormat::Gif;
  }
  // BMP: "BM"
  if (n >= 2 && b[0] == 'B' && b[1] == 'M') {
    return ImageFormat::Bmp;
  }
  // PNM (Netpbm): 'P' followed by '1'..'6'
  if (n >= 2 && b[0] == 'P' && b[1] >= '1' && b[1] <= '6') {
    return ImageFormat::Pnm;
  }
  // TGA (no magic): validate the 18-byte header conservatively. color-map type
  // in {0,1}, image type in {0,1,2,3,9,10,11}, pixel depth in {8,15,16,24,32}.
  if (n >= 18) {
    const std::uint8_t color_map_type = b[1];
    const std::uint8_t image_type = b[2];
    const std::uint8_t pixel_depth = b[16];
    const bool cmt_ok = color_map_type <= 1;
    const bool it_ok = image_type == 1 || image_type == 2 || image_type == 3 ||
                       image_type == 9 || image_type == 10 || image_type == 11;
    const bool depth_ok = pixel_depth == 8 || pixel_depth == 15 ||
                          pixel_depth == 16 || pixel_depth == 24 ||
                          pixel_depth == 32;
    if (cmt_ok && it_ok && depth_ok) {
      return ImageFormat::Tga;
    }
  }
  return std::nullopt;
}

// ---------------------------------------------------------------------------
// makeMultiFormatTextureResolver — compose per-format decoders into one seam.
//
// Reads the prefix of the file at `url`, sniffs its format, and dispatches to
// the ONE registered decoder for that format. Returns Failed when the file
// cannot be read, the format is unrecognized, or no decoder is registered for
// the sniffed format. The chosen decoder is then handed the same `url` and only
// ever sees input it owns, so its Failed remains unambiguous.
// ---------------------------------------------------------------------------
inline TextureResolver makeMultiFormatTextureResolver(
    std::map<ImageFormat, TextureResolver> decoders) {
  return [decoders = std::move(decoders)](
             const std::string& url) -> TexturePixelResult {
    // Sniff only the header — enough for every signature above (TGA needs 18).
    std::ifstream f(url, std::ios::binary);
    if (!f) {
      return TexturePixelResult::makeFailed();
    }
    std::vector<std::uint8_t> head(32);
    f.read(reinterpret_cast<char*>(head.data()),
           static_cast<std::streamsize>(head.size()));
    head.resize(static_cast<std::size_t>(f.gcount()));

    const std::optional<ImageFormat> fmt = sniffImageFormat(head);
    if (!fmt.has_value()) {
      return TexturePixelResult::makeFailed();
    }
    const auto it = decoders.find(*fmt);
    if (it == decoders.end() || !it->second) {
      return TexturePixelResult::makeFailed();
    }
    return it->second(url);
  };
}

}  // namespace x3d::runtime::extract

#endif  // X3D_RUNTIME_EXTRACT_MULTI_FORMAT_TEXTURE_RESOLVER_HPP
