// Texture.hpp — a CPU sampler2D for the rasterizer. Builds an RGBA8 surface from
// the extraction seam's two inline pixel sources (PixelTexture SFImage, or an
// embedder-resolved TexturePixels via TextureRef::resolvedPixels) and samples it
// with bilinear filtering + repeat/clamp wrap, origin bottom-left (X3D == GL, NO
// V-flip — same invariant the PoC honors).
//
// sRGB: color slots (BaseColor/Diffuse/Emissive/Specular) are authored sRGB; the
// PoC uploads them as GL_SRGB8_ALPHA8 so the GPU linearises on sample and the
// shaders read linear. We replicate that by decoding sRGB→linear AT SAMPLE TIME
// when `srgb` is set, so the ported shaders need no in-shader pow(2.2) (matching
// the GL path exactly). Data textures (Normal/ORM/Occlusion) sample raw (linear).
//
// Out-of-SDK consumer code. namespace x3d::cpuraster.
#ifndef X3D_CPURASTER_TEXTURE_HPP
#define X3D_CPURASTER_TEXTURE_HPP

#include "RenderItem.hpp" // ex::TextureRef / SFImage / TexturePixels
#include "glsl.hpp"

#include <cmath>
#include <cstdint>
#include <vector>

namespace x3d::cpuraster {

class Texture {
public:
  Texture() = default;

  bool valid() const { return w_ > 0 && h_ > 0; }
  int width() const { return w_; }
  int height() const { return h_; }

  // Build from a decoded RGBA8 buffer (tightly packed, bottom-left origin).
  static Texture fromRGBA8(const std::uint8_t *rgba, int w, int h, bool repeatS,
                           bool repeatT, bool srgb) {
    Texture t;
    if (!rgba || w <= 0 || h <= 0) return t;
    t.w_ = w;
    t.h_ = h;
    t.repeatS_ = repeatS;
    t.repeatT_ = repeatT;
    t.srgb_ = srgb;
    t.data_.assign(rgba, rgba + static_cast<std::size_t>(w) * h * 4);
    return t;
  }

  // Build from an inline X3D SFImage (1..4 components, bottom-up == GL, no flip).
  static Texture fromSFImage(const SFImage &img, bool repeatS, bool repeatT,
                             bool srgb) {
    Texture t;
    const int w = img.width, h = img.height, nc = img.numComponents;
    if (w <= 0 || h <= 0 || nc < 1 || nc > 4) return t;
    if (img.data.size() < static_cast<std::size_t>(w) * h * nc) return t;
    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(w) * h * 4, 255);
    for (int i = 0; i < w * h; ++i) {
      const std::uint8_t *p = img.data.data() + static_cast<std::size_t>(i) * nc;
      std::uint8_t r = 0, g = 0, b = 0, a = 255;
      switch (nc) {
        case 1: r = g = b = p[0]; break;
        case 2: r = g = b = p[0]; a = p[1]; break;
        case 3: r = p[0]; g = p[1]; b = p[2]; break;
        case 4: r = p[0]; g = p[1]; b = p[2]; a = p[3]; break;
      }
      rgba[i * 4 + 0] = r; rgba[i * 4 + 1] = g;
      rgba[i * 4 + 2] = b; rgba[i * 4 + 3] = a;
    }
    return fromRGBA8(rgba.data(), w, h, repeatS, repeatT, srgb);
  }

  // Resolve a TextureRef to a CPU texture using ONLY the seam's inline sources:
  //   Source::Inline  -> SFImage pixels.
  //   Source::Url     -> resolvedPixels (the embedder-decoded RGBA8 the SDK
  //                      threaded on, when a TextureResolver was wired).
  // Anything else (unresolved Url, Movie, pending) -> invalid (caller falls back
  // to the flat material color, exactly like the PoC white-fallback).
  static Texture fromRef(const x3d::runtime::extract::TextureRef &ref, bool srgb) {
    using Src = x3d::runtime::extract::TextureRef::Source;
    if (ref.source == Src::Inline)
      return fromSFImage(ref.inlinePixels, ref.repeatS, ref.repeatT, srgb);
    if (ref.source == Src::Url && ref.resolvedPixels.ready() &&
        !ref.resolvedPixels.pixels.rgba.empty()) {
      const auto &p = ref.resolvedPixels.pixels;
      return fromRGBA8(p.rgba.data(), static_cast<int>(p.width),
                       static_cast<int>(p.height), ref.repeatS, ref.repeatT, srgb);
    }
    return {};
  }

  // GLSL texture(sampler2D, vec2) — bilinear, wrapped, sRGB-decoded if color.
  glsl::vec4 sample(glsl::vec2 uv) const {
    if (!valid()) return {1, 1, 1, 1};
    // Continuous texel coords (GL bilinear: sample centers at +0.5).
    float fx = wrap(uv.x, repeatS_) * w_ - 0.5f;
    float fy = wrap(uv.y, repeatT_) * h_ - 0.5f;
    int x0 = static_cast<int>(std::floor(fx));
    int y0 = static_cast<int>(std::floor(fy));
    float tx = fx - x0, ty = fy - y0;
    glsl::vec4 c00 = texel(x0, y0), c10 = texel(x0 + 1, y0);
    glsl::vec4 c01 = texel(x0, y0 + 1), c11 = texel(x0 + 1, y0 + 1);
    glsl::vec4 a = lerp(c00, c10, tx), b = lerp(c01, c11, tx);
    return lerp(a, b, ty);
  }

private:
  static float wrap(float c, bool repeat) {
    if (repeat) {
      float f = c - std::floor(c); // [0,1)
      return f;
    }
    return glsl::clampf(c, 0.0f, 1.0f);
  }
  static float srgbToLinear(float c) {
    return c <= 0.04045f ? c / 12.92f
                         : std::pow((c + 0.055f) / 1.055f, 2.4f);
  }
  static glsl::vec4 lerp(glsl::vec4 a, glsl::vec4 b, float t) {
    return {glsl::mixf(a.x, b.x, t), glsl::mixf(a.y, b.y, t),
            glsl::mixf(a.z, b.z, t), glsl::mixf(a.w, b.w, t)};
  }
  // Fetch a texel with integer wrap, decode to float (sRGB→linear if color).
  glsl::vec4 texel(int x, int y) const {
    x = repeatS_ ? ((x % w_) + w_) % w_ : (x < 0 ? 0 : (x >= w_ ? w_ - 1 : x));
    y = repeatT_ ? ((y % h_) + h_) % h_ : (y < 0 ? 0 : (y >= h_ ? h_ - 1 : y));
    const std::size_t i = (static_cast<std::size_t>(y) * w_ + x) * 4;
    float r = data_[i + 0] / 255.0f, g = data_[i + 1] / 255.0f,
          b = data_[i + 2] / 255.0f, a = data_[i + 3] / 255.0f;
    if (srgb_) { r = srgbToLinear(r); g = srgbToLinear(g); b = srgbToLinear(b); }
    return {r, g, b, a};
  }

  int w_ = 0, h_ = 0;
  bool repeatS_ = true, repeatT_ = true, srgb_ = false;
  std::vector<std::uint8_t> data_; // RGBA8, bottom-left origin.
};

} // namespace x3d::cpuraster

#endif // X3D_CPURASTER_TEXTURE_HPP
