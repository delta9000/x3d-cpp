// skybox_test.cpp — Background panorama cube: face selection + UV, and alpha
// compositing of a face texture over the sky/ground gradient.
#include "RenderItem.hpp"
#include "cpuraster/MaterialShader.hpp"
#include "cpuraster/SceneRender.hpp"

#include <cstdint>
#include <cstdio>

using namespace x3d::cpuraster;
namespace g = x3d::cpuraster::glsl;
using render_detail::CubeFace;

static int failures = 0;
#define CHECK(c)                                                               \
  do {                                                                         \
    if (!(c)) {                                                                \
      std::fprintf(stderr, "FAIL %s:%d  %s\n", __FILE__, __LINE__, #c);        \
      ++failures;                                                              \
    }                                                                          \
  } while (0)

int main() {
  // ---- Face selection: principal axes pick the expected faces. ----
  auto fr = render_detail::cubeFaceUv({0, 0, -1}); // down -Z -> front, centred
  CHECK(fr.face == CubeFace::Front);
  CHECK(fr.uv.x > 0.45f && fr.uv.x < 0.55f && fr.uv.y > 0.45f && fr.uv.y < 0.55f);
  CHECK(render_detail::cubeFaceUv({1, 0, 0}).face == CubeFace::Right);
  CHECK(render_detail::cubeFaceUv({-1, 0, 0}).face == CubeFace::Left);
  CHECK(render_detail::cubeFaceUv({0, 1, 0}).face == CubeFace::Top);
  CHECK(render_detail::cubeFaceUv({0, -1, 0}).face == CubeFace::Bottom);
  CHECK(render_detail::cubeFaceUv({0, 0, 1}).face == CubeFace::Back);

  // ---- UV moves the right way: biasing +X off the -Z axis moves the front
  // face U toward the right edge (the +X side is the right of the 2D image). ----
  CHECK(render_detail::cubeFaceUv({0.4f, 0, -1}).uv.x > 0.55f);

  // ---- Compositing: an opaque front-face texel replaces the gradient; alpha<1
  // lets the gradient show through; an empty face falls back to the gradient. ----
  {
    render_detail::SkyboxTextures sb;
    std::uint8_t green[4] = {0, 255, 0, 255};
    sb.front = Texture::fromRGBA8(green, 1, 1, true, true, /*srgb=*/true);
    g::vec3 grad{1, 0, 0}; // red gradient behind
    g::vec3 opaque = render_detail::skyboxColor({0, 0, -1}, sb, grad);
    CHECK(opaque.y > 0.9f && opaque.x < 0.1f); // green wins (alpha 1)

    std::uint8_t halfgreen[4] = {0, 255, 0, 128};
    sb.front = Texture::fromRGBA8(halfgreen, 1, 1, true, true, true);
    g::vec3 blended = render_detail::skyboxColor({0, 0, -1}, sb, grad);
    CHECK(blended.x > 0.2f && blended.y > 0.2f); // red gradient shows through

    render_detail::SkyboxTextures empty;
    g::vec3 fall = render_detail::skyboxColor({0, 0, -1}, empty, grad);
    CHECK(fall.x > 0.9f && fall.y < 0.1f); // no face -> gradient unchanged
  }

  if (failures) {
    std::fprintf(stderr, "skybox_test: %d failure(s)\n", failures);
    return 1;
  }
  std::printf("skybox_test: OK\n");
  return 0;
}
