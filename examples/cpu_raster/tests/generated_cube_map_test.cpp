// generated_cube_map_test.cpp — REQ-CUBE: §34.4.2 GeneratedCubeMapTexture
// rendered by the CPU reference host.
//
// An unlit radius-2 sphere at the origin carries a GeneratedCubeMapTexture and
// is surrounded by six unlit boxes, one per axis, in the Figure 34.1 face
// colours (front -Z red, back +Z green, left -X blue, right +X yellow, top +Y
// magenta, bottom -Y cyan). The camera is on +Z. With the default lookup (the
// reflection vector) the sphere's centre reflects the box behind the camera,
// and where its normal leans 45 degrees the reflection runs along that axis,
// so each pixel names the box its generated face saw.
//   * update ALWAYS and NEXT_FRAME_ONLY render the faces; NONE with nothing
//     rendered before leaves the texture unresolved (white).
//   * The sphere (solid FALSE, so visible from inside) does not appear in its
//     own cube, where it would hide every box.
//   * Under a rotating Transform the faces follow the local axes and so does
//     the lookup, so the reflection still shows the same boxes (ADR-0060).
//   * With a persistent GeneratedCubeCache, NEXT_FRAME_ONLY keeps the faces
//     after GeneratedCubeMapSystem resets it to NONE: a box changed later is
//     not seen until update is set again.
// Before this change the generated cube was an empty ref that sampled white.
#include "RuntimeSession.hpp"
#include "X3DParse.hpp"
#include "cpuraster/SceneRender.hpp"

#include <any>
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

static std::string box(const char *def, const char *at, const char *rgb) {
  return std::string("<Transform translation='") + at +
         "'><Shape><Appearance><UnlitMaterial DEF='" + def +
         "' emissiveColor='" + rgb +
         "'/></Appearance><Box size='16 16 16'/></Shape></Transform>";
}

static std::string scene(const std::string &update,
                         const std::string &rotation = "0 1 0 0") {
  return "<X3D profile='Full' version='4.0'><Scene>"
         "<Viewpoint position='0 0 10'/><NavigationInfo headlight='false'/>"
         "<Background skyColor='0 0 0'/>"
         "<Transform rotation='" +
         rotation +
         "'><Shape><Appearance><GeneratedCubeMapTexture DEF='G' size='32' "
         "update='" +
         update +
         "'/></Appearance><Sphere radius='2' solid='false'/></Shape></Transform>" +
         box("F", "0 0 -20", "1 0 0") + box("B", "0 0 20", "0 1 0") +
         box("L", "-20 0 0", "0 0 1") + box("R", "20 0 0", "1 1 0") +
         box("T", "0 20 0", "1 0 1") + box("D", "0 -20 0", "0 1 1") +
         "</Scene></X3D>";
}

static Framebuffer draw(rt::RuntimeSession &s,
                        GeneratedCubeCache *cache = nullptr) {
  RenderOptions opt;
  opt.width = opt.height = 128;
  opt.generatedCubes = cache;
  return renderScene(s.context(), s.extractor(), opt);
}

static Framebuffer render(const std::string &x3d) {
  auto s = rt::RuntimeSession::create(x3d::codec::parseDocument(x3d));
  s->fullSnapshot();
  return draw(*s);
}

// The box colour nearest a pixel ("W" for white, "?" when none is close).
static char face(const Framebuffer &fb, int x, int y) {
  const g::vec3 c = fb.colorAt(x, y).xyz();
  struct { char name; g::vec3 rgb; } faces[] = {
      {'F', {1, 0, 0}}, {'B', {0, 1, 0}}, {'L', {0, 0, 1}}, {'R', {1, 1, 0}},
      {'T', {1, 0, 1}}, {'D', {0, 1, 1}}, {'W', {1, 1, 1}}};
  for (const auto &f : faces) {
    const g::vec3 d = c - f.rgb;
    if (g::dot(d, d) < 0.02f) return f.name;
  }
  std::fprintf(stderr, "  pixel (%d,%d) = %.2f %.2f %.2f\n", x, y, c.x, c.y, c.z);
  return '?';
}

int main() {
  // Pixel offset of the sphere point whose normal leans 45 degrees off +Z
  // (sqrt(2) along the axis, seen from 10 - sqrt(2) away; fieldOfView pi/4).
  const float r = std::sqrt(2.0f);
  const int c = 64, o = static_cast<int>(std::lround(
                        r / (10 - r) / std::tan(3.14159265f / 8) * 64));

  for (const char *update : {"ALWAYS", "NEXT_FRAME_ONLY"}) {
    auto fb = render(scene(update));
    CHECK(face(fb, c, c) == 'B');
    CHECK(face(fb, c + o, c) == 'R');
    CHECK(face(fb, c - o, c) == 'L');
    CHECK(face(fb, c, c + o) == 'T'); // row 0 is the bottom (GL origin)
    CHECK(face(fb, c, c - o) == 'D');
  }
  {  // NONE and never rendered: unresolved.
    auto fb = render(scene("NONE"));
    CHECK(face(fb, c, c) == 'W');
  }
  {  // Rotated 90 degrees about Y: faces and lookup both in the local frame.
    auto fb = render(scene("ALWAYS", "0 1 0 1.5707963"));
    CHECK(face(fb, c, c) == 'B');
    CHECK(face(fb, c + o, c) == 'R');
    CHECK(face(fb, c, c + o) == 'T');
  }
  {  // NEXT_FRAME_ONLY with a persistent cache keeps its faces.
    using Update = x3d::core::GeneratedCubeMapTextureUpdateChoices;
    auto s = rt::RuntimeSession::create(
        x3d::codec::parseDocument(scene("NEXT_FRAME_ONLY")));
    s->fullSnapshot();
    GeneratedCubeCache cache;
    s->tick(0);
    s->delta();
    CHECK(face(draw(*s, &cache), c, c) == 'B');
    auto *back = s->scene().resolve("B").get();
    s->context().postEvent(back, "emissiveColor",
                           std::any(x3d::core::SFColor{1, 1, 1}));
    s->tick(0.1); // resets update to NONE
    s->delta();
    CHECK(face(draw(*s, &cache), c, c) == 'B');
    s->context().postEvent(s->scene().resolve("G").get(), "update",
                           std::any(Update::NEXT_FRAME_ONLY));
    s->tick(0.2);
    s->delta();
    CHECK(face(draw(*s, &cache), c, c) == 'W');
  }

  if (failures) {
    std::fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
  }
  std::printf("generated cube map: update modes, self-exclusion, local frame "
              "and cache OK\n");
  return 0;
}
