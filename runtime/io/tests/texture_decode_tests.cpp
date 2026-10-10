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

// ---- DDS (REQ-CUBE: §34.4.3 ImageCubeMapTexture) --------------------------
// Both backends route "DDS " bytes to the shared std-only decoder
// (runtime/io/dds/DdsDecode.hpp). The files are built here byte by byte.
namespace {

void put32(std::vector<std::uint8_t>& b, std::size_t at, std::uint32_t v) {
  for (int i = 0; i < 4; ++i) b[at + i] = static_cast<std::uint8_t>(v >> (8 * i));
}

// A DDS header: `fourCC` 0 means uncompressed 32-bit BGRA (A8R8G8B8 masks).
std::vector<std::uint8_t> ddsHeader(std::uint32_t w, std::uint32_t h,
                                    const char* fourCC, std::uint32_t caps2) {
  std::vector<std::uint8_t> b(128, 0);
  b[0] = 'D'; b[1] = 'D'; b[2] = 'S'; b[3] = ' ';
  put32(b, 4, 124);
  put32(b, 8, 0x1007);  // CAPS | HEIGHT | WIDTH | PIXELFORMAT
  put32(b, 12, h);
  put32(b, 16, w);
  put32(b, 76, 32);
  if (fourCC) {
    put32(b, 80, 0x4);
    for (int i = 0; i < 4; ++i) b[84 + i] = static_cast<std::uint8_t>(fourCC[i]);
  } else {
    put32(b, 80, 0x41);  // RGB | ALPHAPIXELS
    put32(b, 88, 32);
    put32(b, 92, 0x00FF0000u);
    put32(b, 96, 0x0000FF00u);
    put32(b, 100, 0x000000FFu);
    put32(b, 104, 0xFF000000u);
  }
  put32(b, 108, 0x1000);
  put32(b, 112, caps2);
  return b;
}

std::string writeTemp(const char* name, const std::vector<std::uint8_t>& bytes) {
  const std::string path = std::string(TEMP_DIR) + "/" + name;
  std::ofstream(path, std::ios::binary)
      .write(reinterpret_cast<const char*>(bytes.data()),
             static_cast<std::streamsize>(bytes.size()));
  return path;
}

// RGBA of texel (x, y) of `layer`, bottom-left origin.
std::vector<int> texel(const TexturePixels& p, std::uint32_t layer, std::uint32_t x,
                       std::uint32_t y) {
  const std::size_t i = ((layer * p.height + y) * p.width + x) * 4;
  return {p.rgba[i], p.rgba[i + 1], p.rgba[i + 2], p.rgba[i + 3]};
}

}  // namespace

TEST_CASE("texture_dds_cube_and_2d") {
  // A 2x2 BGRA cube map. DDS face k (+X, -X, +Y, -Y, +Z, -Z) is solid
  // grey 10*(k+1) except its top-left texel (the first stored), which is red.
  auto cube = ddsHeader(2, 2, nullptr, 0x200 | 0xFC00);
  for (int face = 0; face < 6; ++face)
    for (int i = 0; i < 4; ++i) {
      const std::uint8_t g = static_cast<std::uint8_t>(10 * (face + 1));
      if (i == 0) cube.insert(cube.end(), {0, 0, 255, 255});  // B G R A
      else cube.insert(cube.end(), {g, g, g, 255});
    }
  const std::string cubePath = writeTemp("cube_bgra.dds", cube);

  // A 4x4 DXT1 2D texture: one block, colour0 = pure red, every index 0.
  auto dxt1 = ddsHeader(4, 4, "DXT1", 0);
  dxt1.insert(dxt1.end(), {0x00, 0xF8, 0x00, 0x00, 0, 0, 0, 0});
  const std::string dxt1Path = writeTemp("red_dxt1.dds", dxt1);

  // A 4x4 DXT5 2D texture: alpha0 = 255, alpha1 = 0, every alpha index 1
  // (alpha 0); colour0 = pure green.
  auto dxt5 = ddsHeader(4, 4, "DXT5", 0);
  dxt5.insert(dxt5.end(), {255, 0, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24,
                           0xE0, 0x07, 0x00, 0x00, 0, 0, 0, 0});
  const std::string dxt5Path = writeTemp("green_dxt5.dds", dxt5);

  // Only the +X face present.
  auto partial = ddsHeader(1, 1, nullptr, 0x200 | 0x400);
  partial.insert(partial.end(), {0, 255, 0, 255});
  const std::string partialPath = writeTemp("partial.dds", partial);

  auto truncated = cube;
  truncated.resize(150);
  const std::string truncatedPath = writeTemp("truncated.dds", truncated);

  for (const TextureResolver& r : {makeStbTextureResolver(), makeWuffsTextureResolver()}) {
    const TexturePixelResult c = r(cubePath);
    REQUIRE((c.ready()));
    CHECK((c.pixels->width == 2 && c.pixels->height == 2 && c.pixels->layers == 6));
    CHECK((c.pixels->rgba.size() == 2u * 2u * 4u * 6u));
    // X3D layer order front, back, left, right, top, bottom = DDS face
    // +Z, -Z, -X, +X, +Y, -Y (k = 4, 5, 1, 0, 2, 3).
    const int ddsFace[6] = {4, 5, 1, 0, 2, 3};
    for (std::uint32_t layer = 0; layer < 6; ++layer) {
      const int g = 10 * (ddsFace[layer] + 1);
      CHECK((texel(*c.pixels, layer, 1, 0) == std::vector<int>{g, g, g, 255}));
      // The first stored (top-left) texel lands in the top row: y = 1.
      CHECK((texel(*c.pixels, layer, 0, 1) == std::vector<int>{255, 0, 0, 255}));
    }

    const TexturePixelResult red = r(dxt1Path);
    REQUIRE((red.ready()));
    CHECK((red.pixels->layers == 1 && red.pixels->width == 4));
    CHECK((texel(*red.pixels, 0, 2, 3) == std::vector<int>{255, 0, 0, 255}));

    const TexturePixelResult green = r(dxt5Path);
    REQUIRE((green.ready()));
    CHECK((texel(*green.pixels, 0, 1, 1) == std::vector<int>{0, 255, 0, 0}));

    const TexturePixelResult part = r(partialPath);
    REQUIRE((part.ready()));
    CHECK((texel(*part.pixels, 0, 0, 0) == std::vector<int>{255, 255, 255, 255}));
    CHECK((texel(*part.pixels, 3, 0, 0) == std::vector<int>{0, 255, 0, 255}));  // right = +X

    CHECK((r(truncatedPath).failed()));
  }
}
