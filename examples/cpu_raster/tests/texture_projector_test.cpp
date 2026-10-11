// texture_projector_test.cpp — REQ-PROJECTION: §42 texture projectors lit by
// the CPU reference host (ADR-0061).
//
// A white plane at z = 0 faces a camera on +Z with the headlight off and no
// other light. A projector 5 in front of it casts a 2 x 2 PixelTexture (red,
// green bottom; blue, yellow top), so each quadrant of the projected square
// takes that texel's colour, and outside the volume the plane stays black.
//   * TextureProjector (perspective, fieldOfView 0.6) and
//     TextureProjectorParallel (fieldOfView -1 -1 1 1), in Phong and PBR.
//   * A scoped projector (global FALSE) in another group does not light it.
//   * nearDistance beyond the plane leaves it dark.
// Before this change projectors were ignored and the plane was black.
#include "RuntimeSession.hpp"
#include "X3DParse.hpp"
#include "cpuraster/SceneRender.hpp"

#include <cmath>
#include <cstdio>
#include <string>

using namespace x3d::cpuraster;
namespace rt = x3d::runtime;
namespace g = x3d::cpuraster::glsl;

static int failures = 0;
#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::fprintf(stderr, "FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);     \
      ++failures;                                                              \
    }                                                                          \
  } while (0)

static const char *kTexture =
    "<PixelTexture containerField='texture' "
    "image='2 2 3 0xFF0000 0x00FF00 0x0000FF 0xFFFF00'/>";

static Framebuffer render(const std::string &projector, const std::string &material) {
  const std::string scene =
      "<X3D profile='Full' version='4.0'><Scene>"
      "<Viewpoint position='0 0 10'/><NavigationInfo headlight='false'/>"
      "<Background skyColor='0 0 0'/>" +
      projector +
      "<Shape><Appearance>" + material +
      "</Appearance><IndexedFaceSet coordIndex='0 1 2 3 -1'><Coordinate "
      "point='-10 -10 0 10 -10 0 10 10 0 -10 10 0'/></IndexedFaceSet></Shape>"
      "</Scene></X3D>";
  auto session = rt::RuntimeSession::create(x3d::codec::parseDocument(scene));
  session->fullSnapshot();
  RenderOptions opt;
  opt.width = opt.height = 128;
  return renderScene(session->context(), session->extractor(), opt);
}

// The hue a pixel adds over the unlit plane (the corner pixel, which the PBR
// program's constant ambient floor keeps above black): R, G, B, Y, or '.'
// when it adds nothing ('?' otherwise).
static char hue(const Framebuffer &fb, int x, int y) {
  const g::vec3 c = fb.colorAt(x, y).xyz() - fb.colorAt(1, 1).xyz();
  const float peak = std::max({c.x, c.y, c.z});
  if (peak < 0.05f) return '.';
  const g::vec3 n = c * (1.0f / peak);
  struct { char name; g::vec3 rgb; } hues[] = {
      {'R', {1, 0, 0}}, {'G', {0, 1, 0}}, {'B', {0, 0, 1}}, {'Y', {1, 1, 0}}};
  for (const auto &h : hues) {
    const g::vec3 d = n - h.rgb;
    if (g::dot(d, d) < 0.05f) return h.name;
  }
  std::fprintf(stderr, "  pixel (%d,%d) = %.2f %.2f %.2f\n", x, y, c.x, c.y, c.z);
  return '?';
}

// The pixel offset of a point `units` from the axis on the plane z = 0.
static int px(float units) {
  return static_cast<int>(std::lround(units / (10 * std::tan(3.14159265f / 8)) * 64));
}

int main() {
  const char *materials[] = {
      "<Material diffuseColor='1 1 1'/>",
      "<PhysicalMaterial baseColor='1 1 1' metallic='0' roughness='1'/>"};
  const std::string perspective =
      std::string("<TextureProjector location='0 0 5' direction='0 0 -1' upVector='0 1 0' "
                  "fieldOfView='0.6'>") + kTexture + "</TextureProjector>";
  const std::string parallel =
      std::string("<TextureProjectorParallel location='0 0 5' direction='0 0 -1'>") +
      kTexture + "</TextureProjectorParallel>";
  const int c = 64;
  const float half = 5 * std::tan(0.3f); // perspective half-extent on the plane
  for (const char *material : materials) {
    {
      auto fb = render(perspective, material);
      const int q = px(half / 2), out = px(half * 1.5f);
      CHECK(hue(fb, c - q, c - q) == 'R');
      CHECK(hue(fb, c + q, c - q) == 'G');
      CHECK(hue(fb, c - q, c + q) == 'B');
      CHECK(hue(fb, c + q, c + q) == 'Y');
      CHECK(hue(fb, c + out, c) == '.');
      CHECK(hue(fb, c, c + out) == '.');
    }
    {
      auto fb = render(parallel, material);
      const int q = px(0.5f), out = px(1.5f);
      CHECK(hue(fb, c - q, c - q) == 'R');
      CHECK(hue(fb, c + q, c + q) == 'Y');
      CHECK(hue(fb, c - out, c) == '.');
    }
  }
  {  // Scoped to another group.
    auto fb = render("<Group>" + perspective.substr(0, 17) + " global='false'" +
                         perspective.substr(17) + "</Group>",
                     materials[0]);
    CHECK(hue(fb, c, c) == '.');
  }
  {  // nearDistance beyond the plane.
    auto fb = render(perspective.substr(0, 17) + " nearDistance='6'" + perspective.substr(17),
                     materials[0]);
    CHECK(hue(fb, c + px(half / 2), c + px(half / 2)) == '.');
  }

  if (failures) {
    std::fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
  }
  std::printf("texture projectors: perspective and parallel quadrants in Phong and "
              "PBR, scope and near range OK\n");
  return 0;
}
