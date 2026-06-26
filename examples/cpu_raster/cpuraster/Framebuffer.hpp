// Framebuffer.hpp — an RGBA8 color buffer + a float depth buffer for the
// headless CPU rasterizer, with PNG (default) and dependency-free PPM (P6)
// writers.
//
// PNG output uses a vendored stb_image_write (ImageWrite.cpp) — a CONSUMER-side
// encoder, kept outside the SDK. PPM stays available (zero third-party code) so
// the target still builds/writes on a bare runner, and mirrors the PoC
// renderer's --screenshot P6 output so the two can be diffed.
//
// Out-of-SDK consumer code (examples/cpu_raster/). namespace x3d::cpuraster.
#ifndef X3D_CPURASTER_FRAMEBUFFER_HPP
#define X3D_CPURASTER_FRAMEBUFFER_HPP

#include "ImageWrite.hpp"
#include "glsl.hpp"

#include <cstdint>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

namespace x3d::cpuraster {

class Framebuffer {
public:
  Framebuffer(int width, int height)
      : w_(width < 1 ? 1 : width), h_(height < 1 ? 1 : height),
        color_(static_cast<std::size_t>(w_) * h_ * 4, 0),
        depth_(static_cast<std::size_t>(w_) * h_,
               std::numeric_limits<float>::infinity()) {}

  int width() const { return w_; }
  int height() const { return h_; }

  // Clear color is given linear; stored as 8-bit. Depth clears to +inf (far).
  void clear(glsl::vec3 rgb) {
    for (int y = 0; y < h_; ++y)
      for (int x = 0; x < w_; ++x) setColor(x, y, {rgb.x, rgb.y, rgb.z, 1.0f});
    std::fill(depth_.begin(), depth_.end(),
              std::numeric_limits<float>::infinity());
  }

  float depth(int x, int y) const { return depth_[idx(x, y)]; }
  void setDepth(int x, int y, float z) { depth_[idx(x, y)] = z; }

  // Read back a pixel as linear RGBA in [0,1] (8-bit → float).
  glsl::vec4 colorAt(int x, int y) const {
    const std::size_t i = idx(x, y) * 4;
    return {color_[i + 0] / 255.0f, color_[i + 1] / 255.0f,
            color_[i + 2] / 255.0f, color_[i + 3] / 255.0f};
  }

  // Write a pixel. `rgba` is the FINAL framebuffer color (already gamma-encoded
  // by the shader, as in GL). Values are clamped to [0,1] and quantised to 8-bit.
  void setColor(int x, int y, glsl::vec4 rgba) {
    const std::size_t i = idx(x, y) * 4;
    color_[i + 0] = quant(rgba.x);
    color_[i + 1] = quant(rgba.y);
    color_[i + 2] = quant(rgba.z);
    color_[i + 3] = quant(rgba.w);
  }

  // Source-over blend a fragment onto the existing pixel (transparency pass).
  void blendColor(int x, int y, glsl::vec4 src) {
    glsl::vec4 dst = colorAt(x, y);
    const float a = glsl::clampf(src.w, 0.0f, 1.0f);
    glsl::vec4 out{glsl::mixf(dst.x, src.x, a), glsl::mixf(dst.y, src.y, a),
                   glsl::mixf(dst.z, src.z, a), dst.w + a * (1.0f - dst.w)};
    setColor(x, y, out);
  }

  const std::vector<std::uint8_t> &pixels() const { return color_; }

  // Write an RGB PNG (rows flipped to top-down, matching writePPM). Encoding
  // lives in ImageWrite.cpp (vendored stb_image_write).
  bool writePNG(const std::string &path) const {
    return writePngRGB(path, w_, h_, color_.data());
  }

  // Write a binary PPM (P6). GL/raster origin is bottom-left; PPM is top-down,
  // so rows are flipped — matching the PoC's writeFramebufferPPM().
  bool writePPM(const std::string &path) const {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out << "P6\n" << w_ << " " << h_ << "\n255\n";
    for (int row = h_ - 1; row >= 0; --row) {
      for (int x = 0; x < w_; ++x) {
        const std::size_t i = (static_cast<std::size_t>(row) * w_ + x) * 4;
        out.put(static_cast<char>(color_[i + 0]));
        out.put(static_cast<char>(color_[i + 1]));
        out.put(static_cast<char>(color_[i + 2]));
      }
    }
    return static_cast<bool>(out);
  }

private:
  std::size_t idx(int x, int y) const {
    if (x < 0) x = 0; else if (x >= w_) x = w_ - 1;
    if (y < 0) y = 0; else if (y >= h_) y = h_ - 1;
    return static_cast<std::size_t>(y) * w_ + x;
  }
  static std::uint8_t quant(float c) {
    return static_cast<std::uint8_t>(glsl::clampf(c, 0.0f, 1.0f) * 255.0f + 0.5f);
  }

  int w_, h_;
  std::vector<std::uint8_t> color_; // RGBA8, row-major, bottom-up (GL origin).
  std::vector<float> depth_;        // smaller = nearer (NDC z in [-1,1]).
};

} // namespace x3d::cpuraster

#endif // X3D_CPURASTER_FRAMEBUFFER_HPP
