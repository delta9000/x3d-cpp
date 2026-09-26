// Texture.hpp — a CPU sampler2D for the rasterizer. Builds an RGBA8 surface from
// the extraction seam's two inline pixel sources (PixelTexture SFImage, or an
// embedder-resolved TexturePixels via TextureRef::resolvedPixels) and samples it
// with bilinear filtering + repeat/clamp/mirror/border wrap, origin bottom-left
// (X3D == GL, NO V-flip — same invariant the PoC honors).
//
// SAMPLER STATE: the §18.4.9 wrap/filter modes the extractor surfaces on
// TextureRef::extSampler are honored: REPEAT, CLAMP, CLAMP_TO_EDGE,
// CLAMP_TO_BOUNDARY (border color, default = the TextureProperties default
// 0,0,0,0 — borderColor is not currently extracted), and MIRRORED_REPEAT.
// Each mode clamps to its own §18.4.9 Table 18.7 range; under bilinear
// filtering CLAMP and CLAMP_TO_BOUNDARY let the outermost taps read the border
// colour, so CLAMP differs from CLAMP_TO_EDGE near the edge.
// Magnification NEAREST_PIXEL/FASTEST selects nearest-neighbor fetch;
// AVG_PIXEL/NICEST/DEFAULT stay bilinear. MIPMAPPING (the minification filters)
// is deliberately UNIMPLEMENTED — the CPU sampler has a single mip level.
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
using namespace x3d::core; // SFImage etc. (ADR-0039 moved core types into x3d::core)
namespace ex = x3d::runtime::extract;

class Texture {
public:
  // Wrap + interpolation state, modeled on a GL sampler object (§18.4.9).
  struct Sampler {
    ex::BoundaryMode wrapS = ex::BoundaryMode::Repeat;
    ex::BoundaryMode wrapT = ex::BoundaryMode::Repeat;
    glsl::vec4 borderColor{0.0f, 0.0f, 0.0f, 0.0f}; // CLAMP_TO_BOUNDARY border.
    bool nearestMagnification = false;
  };

  Texture() = default;

  bool valid() const { return w_ > 0 && h_ > 0; }
  int width() const { return w_; }
  int height() const { return h_; }

  // Build from a decoded RGBA8 buffer (tightly packed, bottom-left origin).
  static Texture fromRGBA8(const std::uint8_t *rgba, int w, int h, bool repeatS,
                           bool repeatT, bool srgb) {
    Sampler s;
    s.wrapS = repeatS ? ex::BoundaryMode::Repeat : ex::BoundaryMode::ClampToEdge;
    s.wrapT = repeatT ? ex::BoundaryMode::Repeat : ex::BoundaryMode::ClampToEdge;
    return fromRGBA8(rgba, w, h, s, srgb);
  }

  static Texture fromRGBA8(const std::uint8_t *rgba, int w, int h,
                           const Sampler &sampler, bool srgb) {
    Texture t;
    if (!rgba || w <= 0 || h <= 0) return t;
    t.w_ = w;
    t.h_ = h;
    t.sampler_ = sampler;
    t.srgb_ = srgb;
    t.data_.assign(rgba, rgba + static_cast<std::size_t>(w) * h * 4);
    return t;
  }

  // Build from an inline X3D SFImage (1..4 components, bottom-up == GL, no flip).
  static Texture fromSFImage(const SFImage &img, bool repeatS, bool repeatT,
                             bool srgb) {
    Sampler s;
    s.wrapS = repeatS ? ex::BoundaryMode::Repeat : ex::BoundaryMode::ClampToEdge;
    s.wrapT = repeatT ? ex::BoundaryMode::Repeat : ex::BoundaryMode::ClampToEdge;
    return fromSFImage(img, s, srgb);
  }

  static Texture fromSFImage(const SFImage &img, const Sampler &sampler,
                             bool srgb) {
    const int w = img.width, h = img.height, nc = img.numComponents;
    if (w <= 0 || h <= 0 || nc < 1 || nc > 4) return {};
    if (img.data.size() < static_cast<std::size_t>(w) * h * nc) return {};
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
    return fromRGBA8(rgba.data(), w, h, sampler, srgb);
  }

  // Resolve a TextureRef to a CPU texture using ONLY the seam's inline sources:
  //   Source::Inline  -> SFImage pixels.
  //   Source::Url     -> resolvedPixels (the embedder-decoded RGBA8 the SDK
  //                      threaded on, when a TextureResolver was wired).
  // Anything else (unresolved Url, Movie, pending) -> invalid (caller falls back
  // to the flat material color, exactly like the PoC white-fallback).
  //
  // The §18.4.9 wrap/filter state comes from ref.extSampler (populated by
  // TextureExtract::enrichTextureRefs; when no TextureProperties is present it
  // derives Repeat/ClampToEdge from the texture's repeatS/T — see
  // extendedSamplerOf). The legacy repeatS/T bools are the same information.
  static Texture fromRef(const ex::TextureRef &ref, bool srgb) {
    using Src = ex::TextureRef::Source;
    const Sampler s = samplerOf(ref);
    if (ref.source == Src::Inline)
      return fromSFImage(ref.inlinePixels, s, srgb);
    if (ref.source == Src::Url && ref.resolvedPixels.ready() &&
        !ref.resolvedPixels.pixels->rgba.empty()) {
      const auto &p = *ref.resolvedPixels.pixels;
      return fromRGBA8(p.rgba.data(), static_cast<int>(p.width),
                       static_cast<int>(p.height), s, srgb);
    }
    return {};
  }

  // GLSL texture(sampler2D, vec2) — wrapped, filtered, sRGB-decoded if color.
  glsl::vec4 sample(glsl::vec2 uv) const {
    if (!valid()) return {1, 1, 1, 1};
    // Continuous texel coords (GL samples at texel centers, +0.5).
    float fx = wrapf(uv.x, sampler_.wrapS, w_) * w_ - 0.5f;
    float fy = wrapf(uv.y, sampler_.wrapT, h_) * h_ - 0.5f;
    if (sampler_.nearestMagnification) // GL_NEAREST: texel whose center is nearest.
      return texel(static_cast<int>(std::floor(fx + 0.5f)),
                   static_cast<int>(std::floor(fy + 0.5f)));
    int x0 = static_cast<int>(std::floor(fx));
    int y0 = static_cast<int>(std::floor(fy));
    float tx = fx - x0, ty = fy - y0;
    glsl::vec4 c00 = texel(x0, y0), c10 = texel(x0 + 1, y0);
    glsl::vec4 c01 = texel(x0, y0 + 1), c11 = texel(x0 + 1, y0 + 1);
    glsl::vec4 a = lerp(c00, c10, tx), b = lerp(c01, c11, tx);
    return lerp(a, b, ty);
  }

  // Map a §18.4.9 sampler descriptor onto the CPU sampler state.
  static Sampler samplerOf(const ex::TextureRef &ref) {
    Sampler s;
    s.wrapS = ref.extSampler.boundaryModeS;
    s.wrapT = ref.extSampler.boundaryModeT;
    s.nearestMagnification = magIsNearest(ref.extSampler.magnificationFilter);
    return s;
  }

private:
  // NEAREST_PIXEL / FASTEST fetch the nearest texel; AVG_PIXEL / NICEST /
  // DEFAULT interpolate (bilinear).
  static bool magIsNearest(ex::MagFilter f) {
    return f == ex::MagFilter::NearestPixel || f == ex::MagFilter::Fastest;
  }

  // Map a texture coordinate to [0,1] (wrapped) or into the mode's clamp range
  // (§18.4.9 Table 18.7), given the axis texel count N:
  //   CLAMP              -> [0, 1]
  //   CLAMP_TO_EDGE      -> [1/(2N), 1 - 1/(2N)]
  //   CLAMP_TO_BOUNDARY  -> [-1/(2N), 1 + 1/(2N)]
  //   MIRRORED_REPEAT    -> mirrored, then clamped as CLAMP_TO_EDGE.
  // CLAMP and CLAMP_TO_BOUNDARY let bilinear taps reach outside the image there,
  // where `texel` reads the border colour (CLAMP_TO_EDGE never does).
  static float wrapf(float c, ex::BoundaryMode m, int n) {
    const float half = 0.5f / static_cast<float>(n);
    switch (m) {
      case ex::BoundaryMode::Repeat:
        return c - std::floor(c);
      case ex::BoundaryMode::MirroredRepeat: {
        const float f = c - std::floor(c);
        const int i = static_cast<int>(std::floor(c));
        return glsl::clampf((i & 1) ? (1.0f - f) : f, half, 1.0f - half);
      }
      case ex::BoundaryMode::Clamp:
        return glsl::clampf(c, 0.0f, 1.0f);
      case ex::BoundaryMode::ClampToEdge:
        return glsl::clampf(c, half, 1.0f - half);
      case ex::BoundaryMode::ClampToBoundary:
        return glsl::clampf(c, -half, 1.0f + half);
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
  // Wrap/clamp an integer texel index per the axis boundary mode. Sets `border`
  // when the index falls outside the image and the mode reads the border colour
  // (CLAMP / CLAMP_TO_BOUNDARY); REPEAT / MIRRORED_REPEAT wrap and CLAMP_TO_EDGE
  // clamps to the edge texel, both staying inside the image.
  static int wrapIndex(int x, int n, ex::BoundaryMode m, bool &border) {
    border = false;
    switch (m) {
      case ex::BoundaryMode::Repeat:
        return ((x % n) + n) % n;
      case ex::BoundaryMode::MirroredRepeat: {
        const int period = 2 * n;
        const int k = ((x % period) + period) % period;
        return k < n ? k : (period - 1 - k);
      }
      case ex::BoundaryMode::ClampToEdge:
        return x < 0 ? 0 : (x >= n ? n - 1 : x);
      default: // Clamp / ClampToBoundary: an out-of-image tap reads the border.
        if (x < 0 || x >= n) { border = true; return 0; }
        return x;
    }
  }
  // Fetch a texel with integer wrap, decode to float (sRGB→linear if color).
  glsl::vec4 texel(int x, int y) const {
    bool bx = false, by = false;
    x = wrapIndex(x, w_, sampler_.wrapS, bx);
    y = wrapIndex(y, h_, sampler_.wrapT, by);
    if (bx || by) return sampler_.borderColor;
    const std::size_t i = (static_cast<std::size_t>(y) * w_ + x) * 4;
    float r = data_[i + 0] / 255.0f, g = data_[i + 1] / 255.0f,
          b = data_[i + 2] / 255.0f, a = data_[i + 3] / 255.0f;
    if (srgb_) { r = srgbToLinear(r); g = srgbToLinear(g); b = srgbToLinear(b); }
    return {r, g, b, a};
  }

  int w_ = 0, h_ = 0;
  bool srgb_ = false;
  Sampler sampler_;
  std::vector<std::uint8_t> data_; // RGBA8, bottom-left origin.
};

} // namespace x3d::cpuraster

#endif // X3D_CPURASTER_TEXTURE_HPP
