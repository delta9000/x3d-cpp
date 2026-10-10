// cube_map_test.cpp — REQ-CUBE: §34.4.1 ComposedCubeMapTexture sampled as an
// environment map by the CPU reference host.
//
// An unlit Sphere of radius 2 faces a camera on +Z. Its cube map has six 1x1
// faces of distinct colours, so each pixel names the face its lookup
// direction pierced (§34.2.2: the (s, t, r) coordinate is a direction from
// the origin; Figure 34.1 faces front = -Z, back = +Z, right = +X ...).
//   * No generator: the camera-space reflection vector. The sphere's centre
//     reflects back toward the viewer (+Z, back face); where its normal leans
//     45 degrees right, left, up or down the reflection runs along that axis.
//   * CAMERASPACEPOSITION: the direction to the fragment (-Z, front face).
//   * CAMERASPACENORMAL: the normal (+Z at the centre, +X on the right).
//   * Inside a MultiTexture, a cube stage combines like any other stage.
//   * §34.4.3 ImageCubeMapTexture: a DDS cube file with the same six colours
//     decodes (x3d_stb) to the same faces.
// Before this change the cube ref sampled white, so every pixel was white.
#include "RuntimeSession.hpp"
#include "StbTextureResolver.hpp"
#include "X3DParse.hpp"
#include "cpuraster/SceneRender.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

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

static const char *kCube =
    "<ComposedCubeMapTexture>"
    "<PixelTexture containerField='frontTexture' image='1 1 3 0xFF0000'/>"
    "<PixelTexture containerField='backTexture' image='1 1 3 0x00FF00'/>"
    "<PixelTexture containerField='leftTexture' image='1 1 3 0x0000FF'/>"
    "<PixelTexture containerField='rightTexture' image='1 1 3 0xFFFF00'/>"
    "<PixelTexture containerField='topTexture' image='1 1 3 0xFF00FF'/>"
    "<PixelTexture containerField='bottomTexture' image='1 1 3 0x00FFFF'/>"
    "</ComposedCubeMapTexture>";

// A radius-2 sphere as an IndexedFaceSet with per-vertex normals, so it can
// carry a TextureCoordinateGenerator (the Sphere node has no texCoord field).
static std::string sphere(const std::string &generator) {
  const int rings = 48, segments = 96;
  std::string points, normals, index;
  for (int i = 0; i <= rings; ++i) {
    const double phi = 3.14159265358979 * i / rings;
    for (int j = 0; j <= segments; ++j) {
      const double theta = 2 * 3.14159265358979 * j / segments;
      const double x = std::sin(phi) * std::sin(theta), y = std::cos(phi),
                   z = std::sin(phi) * std::cos(theta);
      char buf[96];
      std::snprintf(buf, sizeof buf, "%.5f %.5f %.5f ", 2 * x, 2 * y, 2 * z);
      points += buf;
      std::snprintf(buf, sizeof buf, "%.5f %.5f %.5f ", x, y, z);
      normals += buf;
    }
  }
  for (int i = 0; i < rings; ++i)
    for (int j = 0; j < segments; ++j) {
      const int a = i * (segments + 1) + j, b = a + segments + 1;
      index += std::to_string(a) + " " + std::to_string(b) + " " +
               std::to_string(b + 1) + " " + std::to_string(a + 1) + " -1 ";
    }
  return "<IndexedFaceSet coordIndex='" + index + "'><Coordinate point='" +
         points + "'/><Normal vector='" + normals + "'/>" + generator +
         "</IndexedFaceSet>";
}

static Framebuffer render(const std::string &texture,
                          const std::string &generator = "") {
  const std::string scene =
      "<X3D profile='Full' version='4.0'><Scene>"
      "<Viewpoint position='0 0 10'/><NavigationInfo headlight='false'/>"
      "<Shape><Appearance>" +
      texture + "</Appearance>" + sphere(generator) +
      "</Shape></Scene></X3D>";
  rt::SessionOptions options;
  options.textureResolver = x3d::runtime::io::stb::makeStbTextureResolver();
  auto session =
      rt::RuntimeSession::create(x3d::codec::parseDocument(scene), options);
  session->fullSnapshot();
  RenderOptions opt;
  opt.width = opt.height = 128;
  return renderScene(session->context(), session->extractor(), opt);
}

// The face colour nearest a pixel ("?" when none is close).
static char face(const Framebuffer &fb, int x, int y) {
  const g::vec3 c = fb.colorAt(x, y).xyz();
  struct { char name; g::vec3 rgb; } faces[] = {
      {'F', {1, 0, 0}}, {'B', {0, 1, 0}}, {'L', {0, 0, 1}},
      {'R', {1, 1, 0}}, {'T', {1, 0, 1}}, {'D', {0, 1, 1}},
      {'h', {0.5f, 0.5f, 0}}};
  for (const auto &f : faces) {
    const g::vec3 d = c - f.rgb;
    if (g::dot(d, d) < 0.02f) return f.name;
  }
  std::fprintf(stderr, "  pixel (%d,%d) = %.2f %.2f %.2f\n", x, y, c.x, c.y, c.z);
  return '?';
}

static std::string generator(const char *mode) {
  return std::string("<TextureCoordinateGenerator mode='") + mode + "'/>";
}

int main() {
  // Pixel offset of the sphere point whose normal leans 45 degrees off +Z
  // (sqrt(2) along the axis, seen from 10 - sqrt(2) away; fieldOfView pi/4).
  const float r = std::sqrt(2.0f);
  const int c = 64, o = static_cast<int>(std::lround(
                        r / (10 - r) / std::tan(3.14159265f / 8) * 64));
  // ... and 60 degrees off +Z, for the normal itself to favour the side face.
  const float s = 2 * std::sin(1.0472f), z = 2 * std::cos(1.0472f);
  const int o60 = static_cast<int>(std::lround(
      s / (10 - z) / std::tan(3.14159265f / 8) * 64));

  {  // Default: the camera-space reflection vector.
    auto fb = render(kCube);
    CHECK(face(fb, c, c) == 'B');
    CHECK(face(fb, c + o, c) == 'R');
    CHECK(face(fb, c - o, c) == 'L');
    CHECK(face(fb, c, c + o) == 'T'); // row 0 is the bottom (GL origin)
    CHECK(face(fb, c, c - o) == 'D');
  }
  {  // An explicit generator decides the direction.
    auto position = render(kCube, generator("CAMERASPACEPOSITION"));
    CHECK(face(position, c, c) == 'F');
    CHECK(face(position, c + o, c) == 'F');
    auto normal = render(kCube, generator("CAMERASPACENORMAL"));
    CHECK(face(normal, c, c) == 'B');
    CHECK(face(normal, c + o60, c) == 'R');
    CHECK(face(normal, c, c + o60) == 'T');
    auto reflection = render(kCube, generator("CAMERASPACEREFLECTIONVECTOR"));
    CHECK(face(reflection, c + o, c) == 'R');
  }
  {  // A cube stage inside MultiTexture: yellow (right) x 50% grey.
    auto fb = render(std::string("<MultiTexture mode='\"MODULATE\" \"MODULATE\"'>") +
                     kCube + "<PixelTexture image='1 1 1 0x80'/></MultiTexture>");
    CHECK(face(fb, c + o, c) == 'h');
  }
  {  // ImageCubeMapTexture: a 1x1 BGRA DDS cube, faces stored +X, -X, +Y,
     // -Y, +Z, -Z (DDS's left-handed layout; +Z is the X3D front).
    std::vector<std::uint8_t> dds(128, 0);
    auto put = [&](std::size_t at, std::uint32_t v) {
      for (int i = 0; i < 4; ++i) dds[at + i] = static_cast<std::uint8_t>(v >> (8 * i));
    };
    dds[0] = 'D'; dds[1] = 'D'; dds[2] = 'S'; dds[3] = ' ';
    put(4, 124); put(8, 0x1007); put(12, 1); put(16, 1); put(76, 32);
    put(80, 0x41); put(88, 32); put(92, 0xFF0000); put(96, 0xFF00);
    put(100, 0xFF); put(104, 0xFF000000u); put(108, 0x1008); put(112, 0xFE00);
    const std::uint8_t bgra[6][4] = {{0, 255, 255, 255}, {255, 0, 0, 255},
                                     {255, 0, 255, 255}, {255, 255, 0, 255},
                                     {0, 0, 255, 255},   {0, 255, 0, 255}};
    for (const auto &face : bgra) dds.insert(dds.end(), face, face + 4);
    const auto path = std::filesystem::temp_directory_path() / "x3d_cube_map_test.dds";
    std::ofstream(path, std::ios::binary)
        .write(reinterpret_cast<const char *>(dds.data()),
               static_cast<std::streamsize>(dds.size()));
    auto fb = render("<ImageCubeMapTexture url='\"" + path.string() + "\"'/>");
    CHECK(face(fb, c, c) == 'B');
    CHECK(face(fb, c + o, c) == 'R');
    CHECK(face(fb, c - o, c) == 'L');
    CHECK(face(fb, c, c + o) == 'T');
    CHECK(face(fb, c, c - o) == 'D');
    auto position = render("<ImageCubeMapTexture url='\"" + path.string() + "\"'/>",
                           generator("CAMERASPACEPOSITION"));
    CHECK(face(position, c, c) == 'F');
  }
  return failures ? 1 : 0;
}
