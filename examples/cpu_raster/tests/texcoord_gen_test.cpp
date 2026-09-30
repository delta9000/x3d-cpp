// texcoord_gen_test.cpp — TXF-2/TXF-4: §18.4.8 TextureCoordinateGenerator.
//
// The view-dependent modes (SPHERE / CAMERASPACENORMAL / CAMERASPACEPOSITION /
// CAMERASPACEREFLECTIONVECTOR / COORD-EYE) are computed at render time from
// eye-space state — the seam only carries the MODE. This test pins:
//   (1) the exact per-mode UV formulas (hand-computed, non-circular), including
//       the front-facing normal flip, via the shader's detail::texCoordGenUv; and
//   (2) that makeMaterialShader actually SAMPLES with those UVs — a ramp texture
//       (R = u) reveals the coordinate the shader computed for each mode.
#include "RenderItem.hpp"
#include "cpuraster/MaterialShader.hpp"
#include "cpuraster/SceneRender.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
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

static bool feq(float a, float b, float t = 5e-3f) { return std::fabs(a - b) < t; }

// A 256x1 ramp texture: R = i (so sample(u).x ~ u). Phong samples in display
// space (no sRGB decode), so R reads back the coordinate directly.
static ex::TextureRef rampTexture() {
  const int W = 256;
  std::vector<std::uint8_t> px;
  px.reserve(W * 3);
  for (int i = 0; i < W; ++i) {
    px.push_back(static_cast<std::uint8_t>(i)); // R = u.
    px.push_back(0);
    px.push_back(0);
  }
  ex::TextureRef tex;
  tex.slot = ex::TextureRef::Slot::BaseColor;
  tex.source = ex::TextureRef::Source::Inline;
  tex.inlinePixels = SFImage{W, 1, 3, px};
  return tex;
}

// Ambient-only light: isolates the material colour (N·L = 0 for ±Z normals).
static std::vector<EyeLight> ambientOnlyLight() {
  EyeLight al;
  al.dirEye = {1, 0, 0};
  al.color = {1, 1, 1};
  al.ambientIntensity = 1.0f;
  return {al};
}

int main() {
  using Mode = ex::TexCoordGenMode;
  using x3d::cpuraster::detail::texCoordGenUv;

  // ===== (1) Exact per-mode formulas =====================================
  {
    // SPHERE: u = Nx/2+0.5, v = Ny/2+0.5 (N normalized camera-space normal).
    g::vec2 s = texCoordGenUv(Mode::Sphere, {0, 0, -3}, {0.6f, 0, 0.8f}, true);
    CHECK(feq(s.x, 0.8f) && feq(s.y, 0.5f));

    // Back-facing flips the normal -> mirrored sphere UV.
    g::vec2 sb = texCoordGenUv(Mode::Sphere, {0, 0, -3}, {0.6f, 0, 0.8f}, false);
    CHECK(feq(sb.x, 0.2f) && feq(sb.y, 0.5f));

    // CAMERASPACENORMAL: (Nx, Ny).
    g::vec2 cn = texCoordGenUv(Mode::CameraSpaceNormal, {0, 0, -3},
                               {0.8f, 0.6f, 0}, true);
    CHECK(feq(cn.x, 0.8f) && feq(cn.y, 0.6f));

    // CAMERASPACEPOSITION: (Px, Py).
    g::vec2 cp =
        texCoordGenUv(Mode::CameraSpacePosition, {0.8f, 0.5f, -3}, {0, 0, 1}, true);
    CHECK(feq(cp.x, 0.8f) && feq(cp.y, 0.5f));

    // COORD-EYE aliases CAMERASPACEPOSITION, including signed/unbounded UVs.
    // The position must not be normalized, biased, or changed by face/normal.
    for (bool front : {true, false}) {
      g::vec2 ce = texCoordGenUv(Mode::CoordEye, {0.8f, 0.2f, -3},
                                {0, 0, 1}, front);
      CHECK(feq(ce.x, 0.8f) && feq(ce.y, 0.2f));
      g::vec2 raw = texCoordGenUv(Mode::CoordEye, {-2.0f, 3.0f, -4},
                                 {0.6f, 0, 0.8f}, front);
      CHECK(feq(raw.x, -2.0f) && feq(raw.y, 3.0f));
    }

    // CAMERASPACEREFLECTIONVECTOR: R = 2·dot(E,N)·N − E, E=normalize(-P).
    // P=(0,0,-3) -> E=(0,0,1). N=(0.8,0.3317,-0.5): R=(-0.8,-0.3317,-0.5).
    g::vec2 cr = texCoordGenUv(Mode::CameraSpaceReflectionVector, {0, 0, -3},
                               {0.8f, 0.3317f, -0.5f}, true);
    CHECK(feq(cr.x, -0.8f, 1e-2f) && feq(cr.y, -0.3317f, 1e-2f));

    // Off-axis eye vector E=(-0.6,0,0.8), N=(0,0,1): R=(0.6,0,0.8).
    // Flipping N leaves the reflection unchanged.
    for (bool front : {true, false}) {
      g::vec2 offAxis = texCoordGenUv(Mode::CameraSpaceReflectionVector,
                                     {3, 0, -4}, {0, 0, 1}, front);
      CHECK(feq(offAxis.x, 0.6f) && feq(offAxis.y, 0.0f));
    }
  }

  // ===== (2) The shader samples WITH the generated UVs ===================
  // Each case: a material whose base texture is the ramp, a distinguishing
  // eye-space fragment, and the expected sampled u (= the generated coordinate).
  struct Case {
    Mode mode;
    g::vec3 posEye, normalEye;
    bool front;
    float expU;
    const char *name;
  };
  const Case cases[] = {
      {Mode::Sphere, {0, 0, -3}, {0.6f, 0, 0.8f}, true, 0.8f, "SPHERE"},
      {Mode::CameraSpaceNormal, {0, 0, -3}, {0.8f, 0.6f, 0}, true, 0.8f,
       "CAMERASPACENORMAL"},
      {Mode::CameraSpacePosition, {0.8f, 0.5f, -3}, {0, 0, 1}, true, 0.8f,
       "CAMERASPACEPOSITION"},
      {Mode::CoordEye, {0.8f, 0.2f, -3}, {0, 0, 1}, true, 0.8f,
       "COORD-EYE front"},
      {Mode::CoordEye, {0.2f, 0.8f, -3}, {0, 0, 1}, false, 0.2f,
       "COORD-EYE back"},
      // Signed -0.8 wraps to +0.2 with the ramp's REPEAT sampler.
      {Mode::CameraSpaceReflectionVector, {0, 0, -3}, {0.8f, 0.3317f, -0.5f}, true,
       0.2f, "CAMERASPACEREFLECTIONVECTOR"},
      {Mode::CameraSpaceReflectionVector, {3, 0, -4}, {0, 0, 1}, true,
       0.6f, "CAMERASPACEREFLECTIONVECTOR off-axis"},
  };

  for (const Case &c : cases) {
    ex::MaterialDesc m;
    m.model = ex::MaterialModel::Phong;
    m.phong.diffuse = {1, 1, 1};
    m.phong.ambientIntensity = 1.0f;
    ex::TextureRef tex = rampTexture();
    tex.hasTexCoordGen = true;
    tex.texCoordGen.mode = c.mode;
    m.textures.push_back(tex);

    FragmentShader fs = makeMaterialShader(m, ambientOnlyLight(), false, false);
    FragmentInput f;
    f.posEye = c.posEye;
    f.normalEye = c.normalEye;
    f.texcoord = {0.0f, 0.0f}; // ignored: the generator must override this.
    f.frontFacing = c.front;
    g::vec4 o;
    const bool kept = fs(f, o);
    // With no UV-gen the zero authored texcoord would sample u~0 (black); the
    // generator must instead read u ~ expU.
    CHECK(kept);
    if (!feq(o.x, c.expU, 2e-2f))
      std::fprintf(stderr, "  %s: sampled R=%.3f (expected ~%.3f)\n", c.name, o.x,
                   c.expU);
    CHECK(feq(o.x, c.expU, 2e-2f));

    // PBR uses the same generated base UV. Its 0.03 ambient term plus 0.97
    // from the light gives unit diffuse illumination; sRGB decode/encode cancel.
    m.model = ex::MaterialModel::Physical;
    m.physical.baseColor = {1, 1, 1};
    m.physical.metallic = 0;
    auto lights = ambientOnlyLight();
    lights.front().ambientIntensity = 0.97f;
    FragmentShader pbr = makeMaterialShader(m, lights, false, false);
    CHECK(pbr(f, o));
    if (!feq(o.x, c.expU, 2e-2f))
      std::fprintf(stderr, "  PBR %s: sampled R=%.3f (expected ~%.3f)\n",
                   c.name, o.x, c.expU);
    CHECK(feq(o.x, c.expU, 2e-2f));
  }

  // Sanity: without a generator the AUTHORED texcoord is sampled instead. Set it
  // to (0.5,0.5) on the ramp -> R ~ 0.5, distinctly different from the 0.8 the
  // generator produced above, proving those cases exercised the generator.
  {
    ex::MaterialDesc m;
    m.model = ex::MaterialModel::Phong;
    m.phong.diffuse = {1, 1, 1};
    m.phong.ambientIntensity = 1.0f;
    m.textures.push_back(rampTexture()); // hasTexCoordGen == false.
    FragmentShader fs = makeMaterialShader(m, ambientOnlyLight(), false, false);
    FragmentInput f;
    f.posEye = {0, 0, -3};
    f.normalEye = {0.6f, 0, 0.8f};
    f.texcoord = {0.5f, 0.5f};
    f.frontFacing = true;
    g::vec4 o;
    fs(f, o);
    CHECK(feq(o.x, 0.5f, 2e-2f));
  }

  if (failures) {
    std::fprintf(stderr, "texcoord_gen_test: %d failure(s)\n", failures);
    return 1;
  }
  std::printf("texcoord_gen_test: OK\n");
  return 0;
}
