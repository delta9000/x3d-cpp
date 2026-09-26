// glsl_test.cpp — the GLSL emulation math layer + the CPU sampler.
#include "cpuraster/Texture.hpp"
#include "cpuraster/glsl.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>

using namespace x3d::cpuraster;
namespace ex = x3d::runtime::extract;
namespace g = x3d::cpuraster::glsl;

static int failures = 0;
#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::fprintf(stderr, "FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);     \
      ++failures;                                                              \
    }                                                                          \
  } while (0)

static bool near(float a, float b, float e = 1e-4f) { return std::fabs(a - b) < e; }

int main() {
  // ---- vec3 ops ----
  g::vec3 a{1, 2, 3}, b{4, 5, 6};
  CHECK(near(g::dot(a, b), 32.0f));
  g::vec3 c = g::cross({1, 0, 0}, {0, 1, 0});
  CHECK(near(c.x, 0) && near(c.y, 0) && near(c.z, 1));
  CHECK(near(g::length(g::vec3{3, 4, 0}), 5.0f));
  g::vec3 n = g::normalize(g::vec3{0, 3, 4});
  CHECK(near(g::length(n), 1.0f) && near(n.y, 0.6f) && near(n.z, 0.8f));

  // ---- mix / clamp / pow ----
  CHECK(near(g::mixf(0.0f, 10.0f, 0.25f), 2.5f));
  CHECK(near(g::clampf(5.0f, 0.0f, 1.0f), 1.0f));
  g::vec3 p = g::pow_(g::vec3{2, 2, 2}, 3.0f);
  CHECK(near(p.x, 8.0f));
  g::vec3 m = g::mix(g::vec3{0, 0, 0}, g::vec3{2, 4, 6}, 0.5f);
  CHECK(near(m.x, 1) && near(m.y, 2) && near(m.z, 3));

  // ---- reflect ----
  g::vec3 r = g::reflect(g::vec3{1, -1, 0}, g::vec3{0, 1, 0});
  CHECK(near(r.x, 1) && near(r.y, 1) && near(r.z, 0));

  // ---- mat4 from Mat4 (column-major import, no transpose) ----
  x3d::runtime::Mat4 T = x3d::runtime::Mat4::translation({10, 20, 30});
  g::mat4 M(T);
  g::vec4 out = M * g::vec4{1, 1, 1, 1};
  CHECK(near(out.x, 11) && near(out.y, 21) && near(out.z, 31) && near(out.w, 1));

  // ---- mat3 * vec3 ----
  g::mat3 I = g::mat3::identity();
  g::vec3 iv = I * g::vec3{7, 8, 9};
  CHECK(near(iv.x, 7) && near(iv.y, 8) && near(iv.z, 9));

  // ---- normalMatrix of a pure-rotation*translation is its rotation 3x3 ----
  x3d::runtime::Mat4 rot = x3d::runtime::Mat4::rotation({0, 1, 0, 1.0f});
  g::mat3 nm = g::normalMatrix(rot);
  g::vec3 tn = g::normalize(nm * g::vec3{0, 0, 1});
  CHECK(near(g::length(tn), 1.0f)); // rotation preserves unit length.

  // ---- linearToSRGB monotonic + endpoints ----
  g::vec3 s0 = g::linearToSRGB(g::vec3{0, 0, 0});
  g::vec3 s1 = g::linearToSRGB(g::vec3{1, 1, 1});
  CHECK(near(s0.x, 0.0f, 1e-3f) && near(s1.x, 1.0f, 1e-3f));
  CHECK(g::linearToSRGB(g::vec3{0.5f, 0, 0}).x > 0.5f); // sRGB lifts midtones.

  // ---- Texture bilinear: 2x2 distinct texels, center samples the average ----
  SFImage img;
  img.width = 2; img.height = 2; img.numComponents = 3;
  img.data = {0, 0, 0,  255, 0, 0,  0, 255, 0,  0, 0, 255}; // BL,BR,TL,TR
  Texture tex = Texture::fromSFImage(img, /*repeatS=*/true, /*repeatT=*/true, /*srgb=*/false);
  CHECK(tex.valid() && tex.width() == 2 && tex.height() == 2);
  g::vec4 mid = tex.sample({0.5f, 0.5f});
  CHECK(near(mid.x, 0.25f, 0.02f) && near(mid.y, 0.25f, 0.02f) &&
        near(mid.z, 0.25f, 0.02f));

  // ---- Texture boundary modes (§18.4.9 Table 18.7) ----
  // A 2x2 all-white texture sampled with bilinear filtering and the default
  // transparent-black border. At u=0.01 (within half a texel of the edge):
  //   CLAMP          clamps to [0,1]; the outer tap is the border colour, so the
  //                  sample is the edge white blended toward transparent black.
  //   CLAMP_TO_EDGE  clamps to [1/(2N), 1-1/(2N)]; both taps are the edge texel,
  //                  so the sample is pure white. This is the distinguishing case.
  {
    std::uint8_t white[2 * 2 * 4];
    for (int i = 0; i < 2 * 2; ++i) {
      white[i * 4 + 0] = 255; white[i * 4 + 1] = 255;
      white[i * 4 + 2] = 255; white[i * 4 + 3] = 255;
    }
    using BM = ex::BoundaryMode;

    Texture::Sampler clampS; clampS.wrapS = clampS.wrapT = BM::Clamp;
    Texture clampTex = Texture::fromRGBA8(white, 2, 2, clampS, false);
    g::vec4 cl = clampTex.sample({0.01f, 0.5f});
    CHECK(cl.x > 0.3f && cl.x < 0.7f); // ~0.52 white + border black

    Texture::Sampler edgeS; edgeS.wrapS = edgeS.wrapT = BM::ClampToEdge;
    Texture edgeTex = Texture::fromRGBA8(white, 2, 2, edgeS, false);
    g::vec4 ed = edgeTex.sample({0.01f, 0.5f});
    CHECK(near(ed.x, 1.0f)); // pure edge texel, no border blending

    // CLAMP_TO_BOUNDARY clamps to [-1/(2N), 1+1/(2N)]; u=-0.1 lies inside that
    // range, so it is not pulled to 0 and reaches further into the border than
    // CLAMP's clamped-to-0 sample, which would give 0.5.
    Texture::Sampler bndS; bndS.wrapS = bndS.wrapT = BM::ClampToBoundary;
    Texture bndTex = Texture::fromRGBA8(white, 2, 2, bndS, false);
    g::vec4 bd = bndTex.sample({-0.1f, 0.5f});
    CHECK(near(bd.x, 0.3f, 0.02f)); // 0.7*border + 0.3*white

    // MIRRORED_REPEAT mirrors then clamps as CLAMP_TO_EDGE, so near the edge it
    // also yields the pure edge texel.
    Texture::Sampler mirS; mirS.wrapS = mirS.wrapT = BM::MirroredRepeat;
    Texture mirTex = Texture::fromRGBA8(white, 2, 2, mirS, false);
    g::vec4 mi = mirTex.sample({0.01f, 0.5f});
    CHECK(near(mi.x, 1.0f));
  }

  if (failures) { std::fprintf(stderr, "glsl_test: %d failure(s)\n", failures); return 1; }
  std::printf("glsl_test: OK\n");
  return 0;
}
