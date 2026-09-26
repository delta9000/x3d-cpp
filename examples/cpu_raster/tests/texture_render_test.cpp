// texture_render_test.cpp — procedural textures, the resolver seam, PPM IO, and
// end-to-end textured rendering (incl. a no-V-flip UV-orientation check).
#include "RenderItem.hpp"
#include "SceneExtractor.hpp"
#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DParse.hpp"
#include "X3DSceneBridge.hpp"

#include "cpuraster/Framebuffer.hpp"
#include "cpuraster/ProceduralTexture.hpp"
#include "cpuraster/SceneRender.hpp"
#include "cpuraster/Texture.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>

namespace ex = x3d::runtime::extract;
namespace cr = x3d::cpuraster;
namespace g = x3d::cpuraster::glsl;

static int failures = 0;
#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::fprintf(stderr, "FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);     \
      ++failures;                                                              \
    }                                                                          \
  } while (0)

static cr::Texture toTex(const ex::TexturePixels &p, bool srgb = false) {
  return cr::Texture::fromRGBA8(p.rgba.data(), (int)p.width, (int)p.height,
                                true, true, srgb);
}

int main() {
  // ---- UV-grid orientation: red=U (→right), green=V (→up), bottom-left origin.
  {
    cr::Texture uv = toTex(cr::makeUvGrid(), /*srgb=*/false);
    g::vec4 bl = uv.sample({0.12f, 0.12f}); // bottom-left: low u, low v.
    g::vec4 br = uv.sample({0.88f, 0.12f}); // bottom-right: high u, low v.
    g::vec4 tl = uv.sample({0.12f, 0.88f}); // top-left: low u, high v.
    CHECK(br.x > bl.x + 0.3f); // U increases to the right.
    CHECK(tl.y > bl.y + 0.3f); // V increases UP (no V-flip).
    CHECK(br.x > br.y);        // bottom-right is red-dominant.
    CHECK(tl.y > tl.x);        // top-left is green-dominant.
  }

  // ---- Checker: two distinct tones, adjacent cells differ.
  {
    cr::Texture ck = toTex(cr::makeChecker(256, 8));
    float a = ck.sample({0.0625f, 0.0625f}).x; // centre of cell (0,0).
    float b = ck.sample({0.1875f, 0.0625f}).x; // centre of cell (1,0).
    CHECK(std::fabs(a - b) > 0.4f);            // light vs dark.
  }

  // ---- Resolver: proc: synthesizes; unknown ext fails.
  {
    cr::ex::TextureResolver r = cr::makeTextureResolver(".");
    CHECK(r("proc:checker").ready() && !r("proc:checker").pixels->rgba.empty());
    CHECK(r("proc:uvgrid").ready());
    CHECK(r("missing.png").failed());
  }

  // ---- PPM round-trip preserves orientation (bottom-left corner pixel).
  {
    ex::TexturePixels src = cr::makeUvGrid(64, 4);
    const std::string tmp = "._cpuraster_tex_test.ppm";
    CHECK(cr::writePpm(tmp, src));
    ex::TexturePixels rd = cr::readPpm(tmp);
    std::remove(tmp.c_str());
    CHECK(rd.width == src.width && rd.height == src.height && !rd.rgba.empty());
    // Bottom-left pixel (index 0) survives the top-down<->bottom-up flips.
    CHECK(rd.rgba[0] == src.rgba[0] && rd.rgba[1] == src.rgba[1]);
  }

  // ---- End-to-end: textured scene resolves + renders with detail.
  {
    const std::string scene = std::string(X3D_CPURASTER_ASSET_DIR) + "/textured_scene.x3d";
    x3d::runtime::X3DDocument doc;
    try {
      doc = x3d::codec::parseFile(scene);
    } catch (const std::exception &e) {
      std::fprintf(stderr, "parse failed: %s\n", e.what());
      return 1;
    }
    x3d::runtime::Scene &sc = doc.getScene();
    x3d::runtime::X3DExecutionContext ctx;
    ctx.buildSceneGraph(sc);
    ctx.buildFrom(sc);
    ctx.tick(0.0);

    ex::SceneExtractor extractor(ctx, sc, /*meshOpts=*/{},
                                 cr::makeTextureResolver(std::string(X3D_CPURASTER_ASSET_DIR)));
    extractor.fullSnapshot();

    int textured = 0, resolved = 0, withUv = 0;
    for (ex::RenderItemId id = 0; id < extractor.itemCount(); ++id) {
      const ex::RenderItem &it = extractor.item(id);
      if (!it.material.textures.empty()) {
        ++textured;
        if (it.material.textures[0].resolvedPixels.ready()) ++resolved;
      }
      if (!it.mesh->texcoords.empty()) ++withUv;
    }
    std::fprintf(stderr, "textured=%d resolved=%d withUv=%d items=%zu\n",
                 textured, resolved, withUv, extractor.itemCount());
    CHECK(textured >= 4);   // checker box, uv sphere, brick cyl, bars cone.
    CHECK(resolved >= 4);   // proc: textures all decoded onto the seam.
    CHECK(withUv >= 4);     // primitives carry generated texcoords.

    cr::RenderOptions opt;
    opt.width = 192; opt.height = 96;
    cr::Framebuffer fb = cr::renderScene(ctx, extractor, opt);

    // Texture detail => high luminance variance over drawn pixels (vs a flat
    // material, which would be near-constant per face).
    double n = 0, sum = 0, sum2 = 0;
    g::vec3 bg{0.08f, 0.10f, 0.16f};
    for (int y = 0; y < fb.height(); ++y)
      for (int x = 0; x < fb.width(); ++x) {
        g::vec4 c = fb.colorAt(x, y);
        if (std::fabs(c.x - bg.x) + std::fabs(c.y - bg.y) + std::fabs(c.z - bg.z) > 0.05f) {
          double L = 0.299 * c.x + 0.587 * c.y + 0.114 * c.z;
          n += 1; sum += L; sum2 += L * L;
        }
      }
    CHECK(n > 200);
    double var = n > 0 ? (sum2 / n - (sum / n) * (sum / n)) : 0.0;
    std::fprintf(stderr, "drawn=%.0f luminance variance=%.4f\n", n, var);
    CHECK(var > 0.01); // visible texture contrast.
  }

  // ---- Wrap modes (§18.4.9): REPEAT / MIRRORED_REPEAT / CLAMP_TO_EDGE /
  // CLAMP_TO_BOUNDARY on a tiny synthetic 4x1 ramp. Texture::fromRGBA8 takes a
  // Sampler so we can drive each mode directly (no scene needed).
  {
    // 4x1 RGBA8 ramp: R=G=B = 0,85,170,255 (centers at u=0.125,0.375,0.625,0.875).
    static const std::uint8_t px[16] = {
        0, 0, 0, 255, 85, 85, 85, 255, 170, 170, 170, 255, 255, 255, 255, 255};
    cr::Texture::Sampler s; // wrapS/wrapT up to each case; no sRGB decode.

    s.wrapS = ex::BoundaryMode::Repeat;
    cr::Texture rep = cr::Texture::fromRGBA8(px, 4, 1, s, false);
    CHECK(std::fabs(rep.sample({1.25f, 0.5f}).x - rep.sample({0.25f, 0.5f}).x) < 1e-4f);
    CHECK(std::fabs(rep.sample({-0.75f, 0.5f}).x - rep.sample({0.25f, 0.5f}).x) < 1e-4f);

    s.wrapS = ex::BoundaryMode::MirroredRepeat;
    cr::Texture mir = cr::Texture::fromRGBA8(px, 4, 1, s, false);
    // Odd tile reflects: u=1.125 -> 1-0.125=0.875 (edge texel ~1.0); u=-0.125
    // -> 1-0.875=0.125 (texel 0 ~0.0); u=1.875 -> 0.125.
    CHECK(mir.sample({1.125f, 0.5f}).x > 0.9f);
    CHECK(mir.sample({-0.125f, 0.5f}).x < 0.1f);
    CHECK(std::fabs(mir.sample({1.875f, 0.5f}).x - mir.sample({0.125f, 0.5f}).x) < 1e-4f);
    CHECK(mir.sample({1.125f, 0.5f}).x != rep.sample({1.125f, 0.5f}).x); // != repeat.

    s.wrapS = ex::BoundaryMode::ClampToEdge;
    cr::Texture cl = cr::Texture::fromRGBA8(px, 4, 1, s, false);
    CHECK(cl.sample({1.5f, 0.5f}).x > 0.9f);  // clamps to the right edge (1.0).
    CHECK(cl.sample({-0.5f, 0.5f}).x < 0.1f); // clamps to the left edge (0.0).

    s.wrapS = ex::BoundaryMode::ClampToBoundary;
    s.borderColor = {0.1f, 0.2f, 0.3f, 0.4f};
    cr::Texture bd = cr::Texture::fromRGBA8(px, 4, 1, s, false);
    g::vec4 bl = bd.sample({1.5f, 0.5f});
    CHECK(std::fabs(bl.x - 0.1f) < 1e-4f && std::fabs(bl.y - 0.2f) < 1e-4f &&
          std::fabs(bl.z - 0.3f) < 1e-4f); // outside [0,1] -> border color.
    CHECK(bd.sample({-0.25f, 0.5f}).x < 0.1f + 1e-4f); // also out on the left.
    CHECK(bd.sample({0.5f, 0.5f}).x > 0.1f);           // in range -> sampled, not border.

    // Magnification NEAREST_PIXEL fetches one texel; DEFAULT stays bilinear.
    s.wrapS = ex::BoundaryMode::Repeat;
    s.nearestMagnification = true;
    cr::Texture nn = cr::Texture::fromRGBA8(px, 4, 1, s, false);
    s.nearestMagnification = false;
    cr::Texture lin = cr::Texture::fromRGBA8(px, 4, 1, s, false);
    // u=0.2: bilinear blends texel0/1 (~0.1), nearest picks texel0 (0.0).
    CHECK(std::fabs(nn.sample({0.2f, 0.5f}).x - 0.0f) < 0.02f);
    CHECK(lin.sample({0.2f, 0.5f}).x > 0.05f);
  }

  if (failures) { std::fprintf(stderr, "texture_render_test: %d failure(s)\n", failures); return 1; }
  std::printf("texture_render_test: OK\n");
  return 0;
}
