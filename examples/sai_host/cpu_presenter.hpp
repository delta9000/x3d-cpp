#ifndef SAI_CPU_HOST_PRESENTER_HPP
#define SAI_CPU_HOST_PRESENTER_HPP

#include <x3d/sai_presentation.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace host {
struct image_summary {
  std::size_t covered_pixels = 0;
  std::uint64_t checksum = 14695981039346656037ull;
  friend bool operator==(const image_summary&, const image_summary&) = default;
};

// A deliberately small consumer-owned orthographic camera and triangle
// rasterizer. No window, GPU, network, texture loading, or hidden runtime loop.
class cpu_presenter {
  static constexpr int width = 96, height = 72;
  struct vertex { double x, y, z; };
  std::array<std::uint32_t, width * height> pixels_{};
  std::array<double, width * height> depth_{};

  static vertex project(const x3d::core::SFVec3f& p) {
    // Fixed host camera basis; the SDK supplies only local mesh + world matrix.
    const double x = 0.89442719 * p.x - 0.44721360 * p.z;
    const double y = -0.18257419 * p.x + 0.91287093 * p.y - 0.36514837 * p.z;
    const double z = 0.40824829 * p.x + 0.40824829 * p.y + 0.81649658 * p.z;
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) ||
        std::abs(x) > 1000 || std::abs(y) > 1000 || std::abs(z) > 1000)
      throw std::runtime_error("mesh position is outside the bounded host camera domain");
    return {(x + 4) * width / 8, (3 - y) * height / 6, 10 - z};
  }
  static double edge(const vertex& a, const vertex& b, double x, double y) {
    return (b.x - a.x) * (y - a.y) - (b.y - a.y) * (x - a.x);
  }
  void triangle(vertex a, vertex b, vertex c, std::uint32_t color) {
    double area = edge(a, b, c.x, c.y);
    if (std::abs(area) < 1e-10)
      return;
    if (area < 0) {
      std::swap(b, c);
      area = -area;
    }
    const int lo_x = static_cast<int>(std::floor(std::clamp(
      std::min({a.x, b.x, c.x}), 0.0, double(width))));
    const int hi_x = static_cast<int>(std::ceil(std::clamp(
      std::max({a.x, b.x, c.x}), 0.0, double(width))));
    const int lo_y = static_cast<int>(std::floor(std::clamp(
      std::min({a.y, b.y, c.y}), 0.0, double(height))));
    const int hi_y = static_cast<int>(std::ceil(std::clamp(
      std::max({a.y, b.y, c.y}), 0.0, double(height))));
    for (int y = lo_y; y < hi_y; ++y) {
      for (int x = lo_x; x < hi_x; ++x) {
        const double wa = edge(b, c, x + 0.5, y + 0.5);
        const double wb = edge(c, a, x + 0.5, y + 0.5);
        const double wc = edge(a, b, x + 0.5, y + 0.5);
        if (wa < 0 || wb < 0 || wc < 0)
          continue;
        const double z = (wa * a.z + wb * b.z + wc * c.z) / area;
        const auto index = static_cast<std::size_t>(y * width + x);
        if (z < depth_[index]) {
          depth_[index] = z;
          pixels_[index] = color;
        }
      }
    }
  }

public:
  image_summary draw(const x3d::sai::experimental::native::presentation_frame& frame) {
    pixels_.fill(0);
    depth_.fill(std::numeric_limits<double>::infinity());
    if (frame.items.size() > 16)
      throw std::runtime_error("CPU presenter accepts at most 16 placements");
    for (const auto& item : frame.items) {
      if (!item.mesh || item.mesh->positions.size() > 4096 ||
          item.mesh->indices.size() > 12288 ||
          item.mesh->topology != x3d::runtime::extract::Topology::Triangles ||
          item.mesh->indices.size() % 3 != 0)
        throw std::runtime_error("CPU presenter requires a bounded indexed triangle mesh");
      const auto color = std::uint32_t{0xff000000u} |
        (std::uint32_t(item.placement_key * 2654435761u) & 0x00ffffffu);
      const auto& indices = item.mesh->indices;
      for (std::size_t i = 0; i < indices.size(); i += 3) {
        std::array<vertex, 3> triangle_vertices;
        for (std::size_t j = 0; j < 3; ++j) {
          if (indices[i + j] >= item.mesh->positions.size())
            throw std::runtime_error("mesh index is out of range");
          triangle_vertices[j] = project(item.world_transform.transformPoint(
            item.mesh->positions[indices[i + j]]));
        }
        triangle(triangle_vertices[0], triangle_vertices[1], triangle_vertices[2], color);
      }
    }
    image_summary summary;
    for (const auto pixel : pixels_) {
      summary.covered_pixels += pixel != 0;
      // Explicit byte order makes the checksum independent of host endianness.
      for (unsigned shift = 0; shift < 32; shift += 8) {
        summary.checksum ^= (pixel >> shift) & 0xffu;
        summary.checksum *= 1099511628211ull;
      }
    }
    if (!frame.items.empty() &&
        (summary.covered_pixels < 16 || summary.covered_pixels == pixels_.size()))
      throw std::runtime_error("CPU frame has implausible covered-pixel count");
    return summary;
  }
};
} // namespace host
#endif
