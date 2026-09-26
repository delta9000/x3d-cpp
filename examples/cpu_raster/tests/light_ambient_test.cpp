// light_ambient_test.cpp — two spec-fidelity guarantees in the CPU reference
// shading core:
//
//   (a) §23.4.4 NavigationInfo.headlight: headlight TRUE (default) turns on a
//       camera-space headlight REGARDLESS of the scene's own lights; FALSE turns
//       it off. buildEyeLights() is the consumer's headlight seam.
//
//   (b) §17.2.2.4 per-light ambientIntensity: a light's ambientIntensity scales
//       the ambient term (ambientIntensity_i · materialAmbientIntensity ·
//       diffuseColor), so a light with ambientIntensity 0 contributes none.
//
// Regression target: both were ignored — the headlight was a no-lights fallback,
// and the ambient term was light-independent (LightDesc.ambientIntensity was
// extracted but never read).
#include "RenderItem.hpp"
#include "cpuraster/MaterialShader.hpp"
#include "cpuraster/SceneRender.hpp"

#include <cstdio>
#include <vector>

using namespace x3d::cpuraster;
namespace ex = x3d::runtime::extract;
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

// A front-facing fragment whose normal is +Z (toward the camera).
static FragmentInput frontFrag() {
  FragmentInput f;
  f.posEye = {0, 0, -5};
  f.normalEye = {0, 0, 1};
  f.color = {1, 1, 1, 1};
  f.frontFacing = true;
  return f;
}

// Phong luminance of a fragment under `lights` whose only possible contribution
// is the ambient term (the light travels +Z, so N·L = 0; specular is 0).
static float ambientLuma(const std::vector<EyeLight> &lights,
                         float materialAmbient) {
  ex::MaterialDesc m;
  m.model = ex::MaterialModel::Phong;
  m.phong.diffuse = {0.8f, 0.8f, 0.8f};
  m.phong.specular = {0.0f, 0.0f, 0.0f};
  m.phong.ambientIntensity = materialAmbient;
  FragmentShader fs = makePhongShader(m, lights, /*hasColors=*/false);
  g::vec4 o;
  fs(frontFrag(), o);
  return o.x + o.y + o.z;
}

static ex::LightDesc directional(float ambientIntensity) {
  ex::LightDesc L;
  L.type = ex::LightDesc::Type::Directional;
  L.worldDirection = {0, 0, 1}; // travels +Z: L = {0,0,-1}, N·L = 0.
  L.color = {1, 1, 1};
  L.intensity = 1.0f;
  L.ambientIntensity = ambientIntensity;
  return L;
}

int main() {
  const rt::Mat4 view = rt::Mat4::identity(); // eye == world.
  const auto lights = [&](float ai) {
    return render_detail::buildEyeLights({directional(ai)}, view,
                                         /*headlightOn=*/false);
  };

  // ---- (b) per-light ambientIntensity ----
  {
    // ambientIntensity 0 => the light contributes no ambient (only the light's
    // diffuse/specular, which are N·L=0 here) => black.
    CHECK(ambientLuma(lights(0.0f), 0.2f) < 0.01f);
    // ambientIntensity 1 => a visible ambient term.
    CHECK(ambientLuma(lights(1.0f), 0.2f) > 0.1f);
    // Ambient scales with the light's ambientIntensity (monotonic).
    CHECK(ambientLuma(lights(1.0f), 0.2f) > ambientLuma(lights(0.5f), 0.2f));
    // Material ambientIntensity still multiplies the per-light term.
    CHECK(ambientLuma(lights(1.0f), 0.0f) < 0.01f);
  }

  // ---- (a) headlight independence ----
  {
    // No scene lights, headlight on (default) => the headlight alone.
    CHECK(render_detail::buildEyeLights({}, view, true).size() == 1);
    // No scene lights, headlight off => nothing.
    CHECK(render_detail::buildEyeLights({}, view, false).empty());
    // One scene light + headlight on => the headlight is ADDED on top.
    CHECK(render_detail::buildEyeLights({directional(0.0f)}, view, true).size() ==
          2);
    // One scene light + headlight off => only the scene light.
    CHECK(render_detail::buildEyeLights({directional(0.0f)}, view, false)
              .size() == 1);

    // The headlight never starves itself out of the 8-slot cap: 8 authored
    // lights + headlight still keeps the headlight (7 authored + headlight).
    std::vector<ex::LightDesc> many;
    for (int i = 0; i < 8; ++i) many.push_back(directional(0.0f));
    auto capped = render_detail::buildEyeLights(many, view, true);
    CHECK(capped.size() == 8);
    // §23.4.4 pins the headlight ambientIntensity to 0.0; it is appended last.
    CHECK(capped.back().ambientIntensity == 0.0f);
  }

  if (failures) {
    std::fprintf(stderr, "light_ambient_test: %d failure(s)\n", failures);
    return 1;
  }
  std::printf("light_ambient_test: OK\n");
  return 0;
}
