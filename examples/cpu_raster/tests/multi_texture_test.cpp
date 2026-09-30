// multi_texture_test.cpp — §18.4.3 MultiTexture stage combining in the CPU
// rasterizer. Stages come from the extraction seam as one TextureRef per channel
// on the base-colour slot; MaterialShader::detail::combineBaseStages folds them
// with the OpenGL texture-environment convention (see REQ-MULTITEXTURE).
//
// Fail-before/pass-after:
//   * two-stage MODULATE gives the PRODUCT of the two textures (previously only
//     the first matching slot was sampled),
//   * REPLACE takes the SECOND stage,
//   * a single texture is unchanged (base * texel, i.e. one MODULATE stage).
#include "RenderItem.hpp"
#include "cpuraster/MaterialShader.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

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

static bool near(float a, float b, float eps = 0.01f) {
  return std::fabs(a - b) <= eps;
}

// A 1x1 inline (PixelTexture) Ref for a base-colour stage: grey level `v` on all
// RGB, opaque, `channel` = stage index, with the §18.4.3 blend controls.
static ex::TextureRef stage(std::uint8_t v, int channel, const char *mode,
                            const char *source = "", const char *function = "",
                            float factor = 1.0f) {
  ex::TextureRef r;
  r.slot = ex::TextureRef::Slot::BaseColor;
  r.source = ex::TextureRef::Source::Inline;
  r.channel = channel;
  r.multiMode = mode;
  r.multiSource = source;
  r.multiFunction = function;
  r.multiColor = {factor, factor, factor};
  r.multiAlpha = factor;
  r.inlinePixels.width = 1;
  r.inlinePixels.height = 1;
  r.inlinePixels.numComponents = 4;
  r.inlinePixels.data = {v, v, v, 255};
  return r;
}

// Unlit path: output RGB is the combined base colour (no lighting, no fog).
static g::vec4 shadeUnlit(const ex::MaterialDesc &m) {
  FragmentInput f;
  f.posEye = {0, 0, -5};
  f.normalEye = {0, 0, 1};
  f.frontFacing = true;
  f.texcoord = {0.5f, 0.5f};
  FragmentShader fs = makeUnlitShader(m, /*hasColors=*/false);
  g::vec4 o;
  fs(f, o);
  return o;
}

static ex::MaterialDesc unlitWhite(const std::vector<ex::TextureRef> &texs) {
  ex::MaterialDesc m;
  m.model = ex::MaterialModel::Unlit;
  m.emissive = {1.0f, 1.0f, 1.0f}; // base accumulation = white.
  m.textures = texs;
  return m;
}

int main() {
  const float k1 = 128.0f / 255.0f; // 0.502
  const float k2 = 64.0f / 255.0f;  // 0.251
  const float k3 = 192.0f / 255.0f; // 0.753

  // ---- Single texture unchanged: base(white) * texel(k1) = k1 --------------
  {
    g::vec4 o = shadeUnlit(unlitWhite({stage(128, 0, "MODULATE")}));
    CHECK(near(o.x, k1) && near(o.y, k1) && near(o.z, k1));
    CHECK(near(o.w, 1.0f)); // opaque.
  }

  // ---- Two-stage MODULATE: product k1 * k2 (NOT just the first stage) -------
  {
    g::vec4 o = shadeUnlit(
        unlitWhite({stage(128, 0, "MODULATE"), stage(64, 1, "MODULATE")}));
    CHECK(near(o.x, k1 * k2));
    CHECK(!near(o.x, k1)); // fail-before: first slot alone would be k1.
  }

  // ---- REPLACE takes the SECOND stage --------------------------------------
  {
    g::vec4 o = shadeUnlit(
        unlitWhite({stage(64, 0, "REPLACE"), stage(192, 1, "REPLACE")}));
    CHECK(near(o.x, k3));
    CHECK(!near(o.x, k2)); // fail-before: first slot alone would be k2.
  }

  // ---- OFF disables a stage (leaves the accumulation) ----------------------
  {
    // Stage 0 OFF (ignored), stage 1 MODULATE k2 -> white * k2.
    g::vec4 o = shadeUnlit(
        unlitWhite({stage(100, 0, "OFF"), stage(64, 1, "MODULATE")}));
    CHECK(near(o.x, k2));
  }

  // ---- source=FACTOR: arg1 = texel * MultiTexture color/alpha --------------
  {
    // White texel * factor 0.5, MODULATE against white base -> 0.5.
    g::vec4 o = shadeUnlit(unlitWhite({stage(255, 0, "MODULATE", "FACTOR", "", 0.5f)}));
    CHECK(near(o.x, 0.5f));
  }

  // ---- ADD / ADDSIGNED / SUBTRACT / SELECTARG2 on a non-white base ---------
  {
    ex::MaterialDesc m = unlitWhite({stage(128, 0, "ADD")}); // base 0.2 + k1.
    m.emissive = {0.2f, 0.2f, 0.2f};
    CHECK(near(shadeUnlit(m).x, 0.2f + k1));
  }
  {
    ex::MaterialDesc m = unlitWhite({stage(128, 0, "ADDSIGNED")}); // 0.2+k1-0.5.
    m.emissive = {0.2f, 0.2f, 0.2f};
    CHECK(near(shadeUnlit(m).x, 0.2f + k1 - 0.5f));
  }
  {
    // ISO 19775-1 Table 18.3: SUBTRACT = Arg1 - Arg2, Arg1 = this stage's texture
    // and Arg2 = the previous stage (default source), so texture - accumulation.
    ex::MaterialDesc m = unlitWhite({stage(128, 0, "SUBTRACT")}); // k1 - base 0.2.
    m.emissive = {0.2f, 0.2f, 0.2f};
    CHECK(near(shadeUnlit(m).x, k1 - 0.2f));
  }
  {
    // SELECTARG2 keeps the accumulation (base 0.2), ignoring the texel.
    ex::MaterialDesc m = unlitWhite({stage(255, 0, "SELECTARG2")});
    m.emissive = {0.2f, 0.2f, 0.2f};
    CHECK(near(shadeUnlit(m).x, 0.2f));
  }

  // ---- function=COMPLEMENT: (1 - result) -----------------------------------
  {
    // MODULATE white*k1 -> k1, COMPLEMENT -> 1 - k1.
    g::vec4 o = shadeUnlit(unlitWhite({stage(128, 0, "MODULATE", "", "COMPLEMENT")}));
    CHECK(near(o.x, 1.0f - k1));
  }

  if (failures) {
    std::fprintf(stderr, "multi_texture_test: %d failure(s)\n", failures);
    return 1;
  }
  std::printf("multi_texture_test: OK\n");
  return 0;
}
