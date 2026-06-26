// rasterizer_test.cpp — triangle coverage, perspective-correct varyings via the
// 2x2-quad path, z-buffer occlusion, and back-face culling.
#include "cpuraster/Framebuffer.hpp"
#include "cpuraster/Rasterizer.hpp"
#include "cpuraster/glsl.hpp"

#include <cmath>
#include <cstdio>
#include <vector>

using namespace x3d::cpuraster;
namespace g = x3d::cpuraster::glsl;

static int failures = 0;
#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::fprintf(stderr, "FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);     \
      ++failures;                                                              \
    }                                                                          \
  } while (0)

static bool near(float a, float b, float e = 0.02f) { return std::fabs(a - b) < e; }

// A full-screen quad in NDC (identity MVP), CCW winding, at depth z.
static std::vector<Vertex> quad(float z) {
  return {
      {{-1, -1, z}, {0, 0, 1}, {1, 1, 1, 1}, {0, 0}},
      {{ 1, -1, z}, {0, 0, 1}, {1, 1, 1, 1}, {1, 0}},
      {{ 1,  1, z}, {0, 0, 1}, {1, 1, 1, 1}, {1, 1}},
      {{-1,  1, z}, {0, 0, 1}, {1, 1, 1, 1}, {0, 1}},
  };
}
static std::vector<std::uint32_t> quadIdx() { return {0, 1, 2, 0, 2, 3}; }

int main() {
  const g::mat4 I = g::mat4::identity();
  const g::mat3 I3 = g::mat3::identity();

  // ---- Coverage: a red quad fills the framebuffer ----
  {
    Framebuffer fb(16, 16);
    fb.clear({0, 0, 0});
    Rasterizer r(fb);
    auto red = [](const FragmentInput &, g::vec4 &o) { o = {1, 0, 0, 1}; return true; };
    r.drawTriangles(quad(0.0f), quadIdx(), I, I, I, I3, /*ccw=*/true,
                    /*solid=*/true, BlendMode::Opaque, red);
    g::vec4 ctr = fb.colorAt(8, 8);
    CHECK(near(ctr.x, 1.0f) && near(ctr.y, 0.0f) && near(ctr.z, 0.0f));
    // A corner is covered too (full-screen).
    g::vec4 corner = fb.colorAt(1, 1);
    CHECK(near(corner.x, 1.0f));
  }

  // ---- Depth occlusion: a nearer blue quad wins; the farther red is hidden ----
  {
    Framebuffer fb(16, 16);
    fb.clear({0, 0, 0});
    Rasterizer r(fb);
    auto red = [](const FragmentInput &, g::vec4 &o) { o = {1, 0, 0, 1}; return true; };
    auto blue = [](const FragmentInput &, g::vec4 &o) { o = {0, 0, 1, 1}; return true; };
    r.drawTriangles(quad(0.5f), quadIdx(), I, I, I, I3, true, true,
                    BlendMode::Opaque, red); // farther (z=0.5)
    r.drawTriangles(quad(-0.5f), quadIdx(), I, I, I, I3, true, true,
                    BlendMode::Opaque, blue); // nearer (z=-0.5)
    g::vec4 ctr = fb.colorAt(8, 8);
    CHECK(near(ctr.z, 1.0f) && near(ctr.x, 0.0f)); // blue, red occluded.
    // Draw red again farther — must NOT overwrite the nearer blue.
    r.drawTriangles(quad(0.9f), quadIdx(), I, I, I, I3, true, true,
                    BlendMode::Opaque, red);
    CHECK(near(fb.colorAt(8, 8).z, 1.0f));
  }

  // ---- Back-face cull: a CW (back-facing) solid quad draws nothing ----
  {
    Framebuffer fb(16, 16);
    fb.clear({0, 0, 0});
    Rasterizer r(fb);
    auto white = [](const FragmentInput &, g::vec4 &o) { o = {1, 1, 1, 1}; return true; };
    // Reverse winding -> back-facing for ccw=true + solid=true => culled.
    std::vector<std::uint32_t> cw = {0, 2, 1, 0, 3, 2};
    r.drawTriangles(quad(0.0f), cw, I, I, I, I3, /*ccw=*/true, /*solid=*/true,
                    BlendMode::Opaque, white);
    CHECK(near(fb.colorAt(8, 8).x, 0.0f)); // nothing drawn.
    // solid=false (double-sided) draws it regardless of winding.
    r.drawTriangles(quad(0.0f), cw, I, I, I, I3, true, /*solid=*/false,
                    BlendMode::Opaque, white);
    CHECK(near(fb.colorAt(8, 8).x, 1.0f));
  }

  // ---- Perspective-correct varying: interpolate texcoord across the quad ----
  {
    Framebuffer fb(32, 32);
    fb.clear({0, 0, 0});
    Rasterizer r(fb);
    // Output the interpolated U as red, V as green — center should read ~0.5.
    auto uv = [](const FragmentInput &f, g::vec4 &o) {
      o = {f.texcoord.x, f.texcoord.y, 0, 1};
      return true;
    };
    r.drawTriangles(quad(0.0f), quadIdx(), I, I, I, I3, true, true,
                    BlendMode::Opaque, uv);
    g::vec4 ctr = fb.colorAt(16, 16);
    CHECK(near(ctr.x, 0.5f, 0.05f) && near(ctr.y, 0.5f, 0.05f));
  }

  if (failures) { std::fprintf(stderr, "rasterizer_test: %d failure(s)\n", failures); return 1; }
  std::printf("rasterizer_test: OK\n");
  return 0;
}
