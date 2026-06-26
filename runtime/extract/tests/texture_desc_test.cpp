// texture_desc_test.cpp — Phase 1: TextureDesc struct + mip math.
#include "TextureDesc.hpp"
#include "doctest/doctest.h"
#include <vector>

using namespace x3d::runtime::extract;

TEST_CASE("texture_desc_test") {
  // Construct RGBA8 + 2-level mip chain
  TextureDesc td;
  td.format = PixelFormat::RGBA8;
  td.type = TextureType::Tex2D;
  td.color_space = ColorSpace::sRGB;
  td.width = 4;
  td.height = 4;

  // Mip 0: 4x4 = 16 pixels * 4 bytes = 64 bytes
  // Mip 1: 2x2 = 4 pixels * 4 bytes = 16 bytes
  MipLevelDesc mip0; mip0.byte_offset = 0; mip0.byte_length = 64; mip0.width = 4; mip0.height = 4;
  MipLevelDesc mip1; mip1.byte_offset = 64; mip1.byte_length = 16; mip1.width = 2; mip1.height = 2;
  td.mips = {mip0, mip1};
  td.data.resize(80, 0xFF);

  CHECK((td.has_mips()));
  CHECK((!td.is_compressed()));
  CHECK((!td.empty()));
  CHECK((td.mips[0].byte_length == 64));
  CHECK((td.mips[1].byte_offset == 64));

  // BC1 is compressed
  TextureDesc bc;
  bc.format = PixelFormat::BC1_RGB;
  CHECK((bc.is_compressed()));

  // BasisUniversal is compressed
  TextureDesc basis;
  basis.format = PixelFormat::BasisUniversal;
  CHECK((basis.is_compressed()));

  // Empty default
  TextureDesc empty;
  CHECK((empty.empty()));
  CHECK((!empty.has_mips()));

  // KHR transform default off
  CHECK((!td.has_khr_transform));

  return;
}
