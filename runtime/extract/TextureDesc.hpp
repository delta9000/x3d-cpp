// TextureDesc.hpp — Phase 1 binary texture descriptor. namespace
// x3d::runtime::extract. Header-only, std-only (no node deps). Describes a
// decoded or compressed texture image that an embedder can produce and attach
// to a TextureRef::Source::Buffer ref. The SDK never decodes image bytes — the
// consumer fills in a TextureDesc once it has decoded/loaded the TextureRef's
// bufferBytes.
//
// Format enum values follow KTX2/glTF conventions where applicable.
#ifndef X3D_RUNTIME_EXTRACT_TEXTURE_DESC_HPP
#define X3D_RUNTIME_EXTRACT_TEXTURE_DESC_HPP

#include <cstdint>
#include <vector>

namespace x3d::runtime::extract {

// ---------------------------------------------------------------------------
// PixelFormat — texture pixel/block format. Uncompressed formats first;
// compressed formats start at BC1_RGB. The boundary between uncompressed and
// compressed is used by is_compressed() below.
// BasisUniversal = 255 is a sentinel for a Basis Universal supercompressed blob
// that the consumer must transcode to a GPU format before upload.
// ---------------------------------------------------------------------------
enum class PixelFormat : uint8_t {
  // Uncompressed
  R8         = 0,
  RG8        = 1,
  RGB8       = 2,
  RGBA8      = 3,
  R16F       = 4,
  RG16F      = 5,
  RGB16F     = 6,
  RGBA16F    = 7,

  // Block-compressed (GPU-native, no decode needed beyond header parsing)
  BC1_RGB    = 8,   // DXT1, no alpha
  BC1_RGBA   = 9,   // DXT1 with punch-through alpha
  BC2        = 10,  // DXT3 explicit alpha
  BC3        = 11,  // DXT5 interpolated alpha
  BC4        = 12,  // ATI1 single-channel
  BC5        = 13,  // ATI2 two-channel (e.g. normal maps)
  BC6H       = 14,  // HDR unsigned
  BC7        = 15,  // BPTC high-quality RGBA

  // Mobile compressed
  ETC2_RGB   = 16,
  ETC2_RGBA  = 17,

  // ASTC
  ASTC_4x4_RGBA = 18,

  // Supercompressed — must transcode
  BasisUniversal = 255,
};

// ---------------------------------------------------------------------------
// TextureType — dimensionality / array kind.
// ---------------------------------------------------------------------------
enum class TextureType : uint8_t {
  Tex2D   = 0,
  Cube    = 1,
  Array2D = 2,
  Tex3D   = 3,
};

// ---------------------------------------------------------------------------
// ColorSpace — how to interpret stored values at sample time.
// ---------------------------------------------------------------------------
enum class ColorSpace : uint8_t {
  Linear = 0,
  sRGB   = 1,
};

// ---------------------------------------------------------------------------
// WrapMode — texture coordinate wrapping.
// ---------------------------------------------------------------------------
enum class WrapMode : uint8_t {
  Repeat         = 0,
  ClampToEdge    = 1,
  MirroredRepeat = 2,
};

// ---------------------------------------------------------------------------
// FilterMode — sampling filter.
// ---------------------------------------------------------------------------
enum class FilterMode : uint8_t {
  Nearest        = 0,
  Linear         = 1,
  LinearMipLinear = 2,
};

// ---------------------------------------------------------------------------
// MipLevelDesc — extent + byte range for one mip level in TextureDesc::data.
// mips[0] = full resolution.
// ---------------------------------------------------------------------------
struct MipLevelDesc {
  uint32_t byte_offset = 0;
  uint32_t byte_length = 0;
  uint32_t width       = 0;
  uint32_t height      = 0;
  uint32_t depth       = 1;
};

// ---------------------------------------------------------------------------
// KhrTextureTransform — §KHR_texture_transform UV offset/rotation/scale.
// Matches the glTF extension schema.
// ---------------------------------------------------------------------------
struct KhrTextureTransform {
  float offset[2]        = {0.0f, 0.0f};
  float rotation         = 0.0f;
  float scale[2]         = {1.0f, 1.0f};
  int   texcoord_index   = 0;
};

// ---------------------------------------------------------------------------
// TextureDesc — owned binary texture descriptor. Carries the raw pixel or
// compressed bytes plus all metadata a consumer needs to upload it to a GPU.
// ---------------------------------------------------------------------------
struct TextureDesc {
  // Owned pixel / compressed bytes. All mip levels are concatenated here;
  // mips[i] carries the byte_offset + byte_length slice for each level.
  std::vector<uint8_t> data;

  // Mip chain descriptors. mips[0] = full-res. An empty vector means no
  // explicit mips (consumer may generate on upload). A single entry means
  // just the base level with no chain.
  std::vector<MipLevelDesc> mips;

  PixelFormat format      = PixelFormat::RGBA8;
  TextureType type        = TextureType::Tex2D;
  ColorSpace  color_space = ColorSpace::sRGB;

  uint32_t width         = 0;
  uint32_t height        = 0;
  uint32_t depth         = 1;
  uint32_t array_layers  = 1;

  WrapMode   wrap_s  = WrapMode::Repeat;
  WrapMode   wrap_t  = WrapMode::Repeat;
  FilterMode filter  = FilterMode::LinearMipLinear;

  float anisotropy = 1.0f;

  bool               has_khr_transform = false;
  KhrTextureTransform khr_transform{};

  // ---------------------------------------------------------------------------
  // Predicate helpers
  // ---------------------------------------------------------------------------
  bool has_mips()      const { return mips.size() > 1; }
  bool is_compressed() const { return format >= PixelFormat::BC1_RGB; }
  bool empty()         const { return data.empty(); }
};

} // namespace x3d::runtime::extract
#endif // X3D_RUNTIME_EXTRACT_TEXTURE_DESC_HPP
