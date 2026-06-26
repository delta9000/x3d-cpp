// demo_animation_test.cpp — the interpolator demo scenes actually animate over
// time (TimeSensor -> interpolator -> field ROUTE moves pixels), and (Task 3)
// fixed sample frames match committed golden PPMs byte-for-byte.
#include "RenderItem.hpp"
#include "SceneExtractor.hpp"
#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DParse.hpp"
#include "X3DSceneBridge.hpp"

#include "cpuraster/Framebuffer.hpp"
#include "cpuraster/SceneRender.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
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

namespace {

constexpr int kW = 256, kH = 144, kFps = 30;

// Render demo `name` at the frame that lands on time `t = frameIndex/kFps`. We
// step from t=0 frame-by-frame to stay byte-identical to the binary's --animate
// output, even though the interpolators themselves only depend on `now`.
//
// NB: the demo scenes' ROUTE `toField`s use BARE field names
// (translation / rotation / diffuseColor), NOT the `set_`-prefixed aliases —
// buildFrom() route validation rejects the `set_` aliases on those inputOutput
// fields (they are not in the generated field table). The `set_fraction` input
// on the interpolators IS a real inputOnly field, so it is accepted as-is.
//
// IMPORTANT: this test intentionally builds the extractor with the DEFAULT
// (null) texture resolver and a bare RenderOptions (no glyph atlas, no skybox) —
// UNLIKE main.cpp's --animate path, which uses cr::makeTextureResolver(...). So
// the goldens are NOT the literal WebM frames; they are a deterministic
// render-path check, blessed via this exact path. Do NOT "align" this with
// main.cpp's extractor/RenderOptions setup — doing so invalidates all six
// committed goldens.
[[nodiscard]] cr::Framebuffer renderFrame(const std::string &name,
                                          int frameIndex) {
  const std::string scenePath =
      std::string(X3D_CPURASTER_ASSET_DIR) + "/demos/" + name + ".x3d";
  x3d::runtime::X3DDocument doc = x3d::codec::parseFile(scenePath);
  x3d::runtime::Scene &scene = doc.getScene();
  x3d::runtime::X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  ctx.buildFrom(scene);
  x3d::runtime::attachStandardRuntime(scene, ctx);

  ex::SceneExtractor extractor(ctx, scene);
  cr::RenderOptions opt;
  opt.width = kW;
  opt.height = kH;

  cr::Framebuffer fb(kW, kH);
  for (int f = 0; f <= frameIndex; ++f) {
    ctx.tick(static_cast<double>(f) / kFps);
    extractor.fullSnapshot();
    if (f == frameIndex) fb = cr::renderScene(ctx, extractor, opt);
  }
  return fb;
}

int frameDiff(const cr::Framebuffer &a, const cr::Framebuffer &b) {
  int n = 0;
  for (int y = 0; y < a.height(); ++y)
    for (int x = 0; x < a.width(); ++x) {
      g::vec4 ca = a.colorAt(x, y), cb = b.colorAt(x, y);
      if (std::fabs(ca.x - cb.x) + std::fabs(ca.y - cb.y) +
              std::fabs(ca.z - cb.z) > 0.05f)
        ++n;
    }
  return n;
}

std::string goldenPath(const std::string &name, int frameIndex) {
  char suffix[32];
  std::snprintf(suffix, sizeof suffix, "_f%04d.ppm", frameIndex);
  return std::string(X3D_CPURASTER_TEST_DIR) + "/golden/demos/" + name + suffix;
}

bool filesEqual(const std::string &a, const std::string &b) {
  std::ifstream fa(a, std::ios::binary), fb(b, std::ios::binary);
  if (!fa || !fb) return false;
  std::string sa((std::istreambuf_iterator<char>(fa)), {});
  std::string sb((std::istreambuf_iterator<char>(fb)), {});
  return sa == sb;
}

} // namespace

int main() {
  const char *scenes[] = {"anim_position", "anim_orientation", "anim_color",
                          "anim_dvd"};
  for (const char *s : scenes) {
    // Compare fractions 0.125 (t=0.5) and 0.375 (t=1.5): never rotationally
    // symmetric, so an orientation spin always moves pixels (frames 15/75 land
    // 180 deg apart at cycleInterval=4 and a symmetric texture looks identical).
    cr::Framebuffer early = renderFrame(s, 15); // t=0.5 (fraction 0.125)
    cr::Framebuffer late = renderFrame(s, 45);  // t=1.5 (fraction 0.375)
    int diff = frameDiff(early, late);
    std::fprintf(stderr, "%s: %d px changed between t=0.5 and t=1.5\n", s, diff);
    CHECK(diff > 50); // the interpolator visibly animated something.
  }
  const bool bless = std::getenv("X3D_CPURASTER_BLESS") != nullptr;
  const int goldenFrames[] = {30, 90};
  for (const char *s : scenes) {
    for (int idx : goldenFrames) {
      cr::Framebuffer fb = renderFrame(s, idx);
      const std::string gp = goldenPath(s, idx);
      if (bless) {
        CHECK(fb.writePPM(gp));
        std::fprintf(stderr, "blessed %s\n", gp.c_str());
      } else {
        char asuffix[40];
        std::snprintf(asuffix, sizeof asuffix, "_f%04d.actual.ppm", idx);
        const std::string actual = std::string(s) + asuffix;
        CHECK(fb.writePPM(actual));
        if (!filesEqual(actual, gp)) {
          std::fprintf(stderr, "FAIL golden mismatch: %s vs %s\n",
                       actual.c_str(), gp.c_str());
          ++failures;
        } else {
          std::remove(actual.c_str());
        }
      }
    }
  }
  if (failures) {
    std::fprintf(stderr, "demo_animation_test: %d failure(s)\n", failures);
    return 1;
  }
  std::printf("demo_animation_test: OK\n");
  return 0;
}
