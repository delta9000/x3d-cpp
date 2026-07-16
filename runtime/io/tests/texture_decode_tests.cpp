// runtime/io/tests/texture_decode_tests.cpp — the TextureResolver decode
// genericity proof (ADR-0024), one grouped doctest binary.
//
//   U1  per-backend: stb_image decodes fixtures + rejects bad input
//   U2  per-backend: wuffs decodes fixtures + rejects bad input
//   U3  swap-test:   stb vs wuffs decode each lossless fixture BYTE-EQUAL,
//                    plus Failed-parity on corrupt / missing input
//   U2.5 composer:   makeMultiFormatTextureResolver sniff-routes by magic bytes
//
// ABI ISOLATION (the AssetResolver lesson, ADR-0023): this TU talks ONLY through
// the seam factories and includes NO decoder headers (no stb_image.h, no wuffs
// amalgamation). The factories exchange std types, so no heavy backend header
// can leak in and disagree with the linked lib.
#include "MultiFormatTextureResolver.hpp"
#include "StbTextureResolver.hpp"
#include "TextureResolver.hpp"
#include "WuffsTextureResolver.hpp"
#include "doctest/doctest.h"

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

using namespace x3d::runtime::extract;
using x3d::runtime::io::stb::makeStbTextureResolver;
using x3d::runtime::io::wuffs::makeWuffsTextureResolver;

namespace {

std::string fx(const char* name) {
  return std::string(FIXTURES_DIR) + "/" + name;
}

std::vector<std::uint8_t> readBytes(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(f),
                                   std::istreambuf_iterator<char>());
}

// The lossless swap matrix: the intersection of stb_image and wuffs v0.3.4
// codecs that decode to exactly-reproducible 8-bit RGBA. (PNM is stb-only in
// wuffs v0.3.x — added in v0.4 — so it is NOT in this gate; see ADR-0024 §3.)
const char* const kLosslessFixtures[] = {
    "rgba_gradient.png",
    "rgb_checker.bmp",
    "palette.gif",
    "rgba.tga",
};

}  // namespace

TEST_CASE("texture_backend_a_stb") {
  TextureResolver r = makeStbTextureResolver();
  CHECK((static_cast<bool>(r)));

  const TexturePixelResult ok = r(fx("rgba_gradient.png"));
  CHECK((ok.ready()));
  CHECK((ok.pixels->width == 16));
  CHECK((ok.pixels->height == 16));
  CHECK((ok.pixels->rgba.size() == 16u * 16u * 4u));

  CHECK((r(fx("does-not-exist.png")).failed()));
  CHECK((r(fx("garbage.bin")).failed()));
  CHECK((r("http://example.com/x.png").failed()));
}

TEST_CASE("texture_backend_b_wuffs") {
  TextureResolver r = makeWuffsTextureResolver();
  CHECK((static_cast<bool>(r)));

  const TexturePixelResult ok = r(fx("rgba_gradient.png"));
  CHECK((ok.ready()));
  CHECK((ok.pixels->width == 16));
  CHECK((ok.pixels->height == 16));
  CHECK((ok.pixels->rgba.size() == 16u * 16u * 4u));

  CHECK((r(fx("does-not-exist.png")).failed()));
  CHECK((r(fx("garbage.bin")).failed()));
  CHECK((r("http://example.com/x.png").failed()));
}

TEST_CASE("texture_swap_byte_equal") {
  TextureResolver a = makeStbTextureResolver();
  TextureResolver b = makeWuffsTextureResolver();

  for (const char* name : kLosslessFixtures) {
    CAPTURE(name);
    const TexturePixelResult ra = a(fx(name));
    const TexturePixelResult rb = b(fx(name));

    CHECK((ra.ready()));
    CHECK((rb.ready()));
    CHECK((ra.pixels->width == rb.pixels->width));
    CHECK((ra.pixels->height == rb.pixels->height));
    CHECK((ra.pixels->rgba.size() == rb.pixels->rgba.size()));
    // The proof: two independent decoders, byte-identical RGBA surface.
    CHECK((ra.pixels->rgba == rb.pixels->rgba));
  }
}

TEST_CASE("texture_swap_failed_parity") {
  TextureResolver a = makeStbTextureResolver();
  TextureResolver b = makeWuffsTextureResolver();

  // Corrupt input (valid PNG signature, garbage body): both fail, identically.
  CHECK((a(fx("truncated.png")).failed()));
  CHECK((b(fx("truncated.png")).failed()));

  // Missing path: both fail.
  CHECK((a(fx("nope.png")).failed()));
  CHECK((b(fx("nope.png")).failed()));
}

TEST_CASE("multiformat_sniff") {
  CHECK((sniffImageFormat(readBytes(fx("rgba_gradient.png"))) ==
         ImageFormat::Png));
  CHECK((sniffImageFormat(readBytes(fx("rgb_checker.bmp"))) ==
         ImageFormat::Bmp));
  CHECK((sniffImageFormat(readBytes(fx("palette.gif"))) == ImageFormat::Gif));
  CHECK((sniffImageFormat(readBytes(fx("rgba.tga"))) == ImageFormat::Tga));
  CHECK((!sniffImageFormat(readBytes(fx("garbage.bin"))).has_value()));
}

TEST_CASE("multiformat_composer_routing") {
  // An array of decoders behind one seam: route each format to a chosen backend.
  std::map<ImageFormat, TextureResolver> decoders;
  decoders[ImageFormat::Png] = makeWuffsTextureResolver();
  decoders[ImageFormat::Bmp] = makeStbTextureResolver();
  decoders[ImageFormat::Gif] = makeStbTextureResolver();
  decoders[ImageFormat::Tga] = makeWuffsTextureResolver();
  TextureResolver r = makeMultiFormatTextureResolver(std::move(decoders));

  for (const char* name : kLosslessFixtures) {
    CAPTURE(name);
    const TexturePixelResult res = r(fx(name));
    CHECK((res.ready()));
    CHECK((res.pixels->width == 16));
    CHECK((res.pixels->height == 16));
  }

  // Unsniffable blob: Failed (routing happens before any decoder is called).
  CHECK((r(fx("garbage.bin")).failed()));

  // A sniffed format with no registered decoder: Failed, not a misroute.
  std::map<ImageFormat, TextureResolver> only_bmp;
  only_bmp[ImageFormat::Bmp] = makeStbTextureResolver();
  TextureResolver r2 = makeMultiFormatTextureResolver(std::move(only_bmp));
  CHECK((r2(fx("rgba_gradient.png")).failed()));  // PNG sniffed, none registered
  CHECK((r2(fx("rgb_checker.bmp")).ready()));
}
