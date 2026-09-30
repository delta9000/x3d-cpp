// clip_plane_test.cpp — REQ-CLIP (§11.4.1): the CPU rasterizer discards fragments
// in the clipped half-space of each enabled clip plane. A plane (a,b,c,d) is
// satisfied where a*x+b*y+c*z+d >= 0 in the space the plane is expressed in
// (here EYE space, matching the interpolated vPosEye).
#include "cpuraster/Framebuffer.hpp"
#include "cpuraster/Rasterizer.hpp"
#include "cpuraster/glsl.hpp"

#include <cmath>
#include <cstdio>
#include <cstdint>
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

// Full-screen quad in NDC (identity MVP), CCW. Under identity matrices eye space
// == clip space, so vPosEye.x/y equals the vertex NDC position.
static std::vector<Vertex> quad() {
  return {
      {{-1, -1, 0}, {0, 0, 1}, {1, 1, 1, 1}, {0, 0}},
      {{ 1, -1, 0}, {0, 0, 1}, {1, 1, 1, 1}, {1, 0}},
      {{ 1,  1, 0}, {0, 0, 1}, {1, 1, 1, 1}, {1, 1}},
      {{-1,  1, 0}, {0, 0, 1}, {1, 1, 1, 1}, {0, 1}},
  };
}
static std::vector<std::uint32_t> quadIdx() { return {0, 1, 2, 0, 2, 3}; }

int main() {
  const g::mat4 I = g::mat4::identity();
  const g::mat3 I3 = g::mat3::identity();
  auto red = [](const FragmentInput &, g::vec4 &o) { o = {1, 0, 0, 1}; return true; };

  // ---- No plane: the whole quad draws -------------------------------------
  {
    Framebuffer fb(16, 16);
    fb.clear({0, 0, 0});
    Rasterizer r(fb);
    r.drawTriangles(quad(), quadIdx(), I, I, I, I3, true, true,
                    BlendMode::Opaque, red);
    CHECK(near(fb.colorAt(3, 8).x, 1.0f));  // left half
    CHECK(near(fb.colorAt(12, 8).x, 1.0f)); // right half
  }

  // ---- Plane (1,0,0,0): keep x>=0, clip the x<0 (left) half ----------------
  // NDC x<0 maps to screen x<8 (framebuffer is bottom-left origin).
  {
    Framebuffer fb(16, 16);
    fb.clear({0, 0, 0});
    Rasterizer r(fb);
    std::vector<g::vec4> clip{{1, 0, 0, 0}};
    r.drawTriangles(quad(), quadIdx(), I, I, I, I3, true, true,
                    BlendMode::Opaque, red, {}, clip);
    CHECK(near(fb.colorAt(3, 8).x, 0.0f)); // clipped half -> untouched (black).
    CHECK(near(fb.colorAt(12, 8).x, 1.0f)); // kept half -> red.
  }

  // ---- Two planes keep only the +x+y quadrant ------------------------------
  {
    Framebuffer fb(16, 16);
    fb.clear({0, 0, 0});
    Rasterizer r(fb);
    std::vector<g::vec4> clip{{1, 0, 0, 0}, {0, 1, 0, 0}};
    r.drawTriangles(quad(), quadIdx(), I, I, I, I3, true, true,
                    BlendMode::Opaque, red, {}, clip);
    CHECK(near(fb.colorAt(12, 12).x, 1.0f)); // +x +y kept.
    CHECK(near(fb.colorAt(3, 12).x, 0.0f));  // -x clipped.
    CHECK(near(fb.colorAt(12, 3).x, 0.0f));  // -y clipped.
    CHECK(near(fb.colorAt(3, 3).x, 0.0f));   // -x -y clipped.
  }

  // ---- A plane that clips everything discards the whole quad ---------------
  {
    Framebuffer fb(16, 16);
    fb.clear({0, 0, 0});
    Rasterizer r(fb);
    std::vector<g::vec4> clip{{1, 0, 0, -2}}; // x >= 2: outside [-1,1].
    r.drawTriangles(quad(), quadIdx(), I, I, I, I3, true, true,
                    BlendMode::Opaque, red, {}, clip);
    CHECK(near(fb.colorAt(8, 8).x, 0.0f));
  }

  if (failures) {
    std::fprintf(stderr, "clip_plane_test: %d failure(s)\n", failures);
    return 1;
  }
  std::printf("clip_plane_test: OK\n");
  return 0;
}
