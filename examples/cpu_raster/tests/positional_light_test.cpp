// positional_light_test.cpp — PointLight and SpotLight must produce
// per-fragment, position-dependent shading (not the directional-only flat
// result). Drives the public consumer path: render_detail::buildEyeLights() turns extracted
// LightDescs into the shader's EyeLight set, then a Phong shader is evaluated at
// chosen fragments. All assertions are behavioural (luminance comparisons), so
// the test is agnostic to the internal EyeLight representation.
//
// Regression target: PointLight/SpotLight were dropped by buildEyeLights and the
// shader carried only a constant eye-space direction, so a single point light
// with the headlight off rendered a flat, ungraded disc.
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

// Luminance of a white-diffuse Phong fragment under `lights`, ambient 0 so the
// only signal is the lit contribution. linearToSRGB is monotonic, so ordering
// is preserved.
static float litLuma(const std::vector<EyeLight> &lights, FragmentInput f) {
  ex::MaterialDesc m;
  m.model = ex::MaterialModel::Phong;
  m.phong.diffuse = {0.8f, 0.8f, 0.8f};
  m.phong.ambientIntensity = 0.0f;
  FragmentShader fs = makePhongShader(m, lights, /*hasColors=*/false);
  g::vec4 o;
  fs(f, o);
  return o.x + o.y + o.z;
}

static FragmentInput frag(g::vec3 posEye, g::vec3 normalEye) {
  FragmentInput f;
  f.posEye = posEye;
  f.normalEye = normalEye;
  f.color = {1, 1, 1, 1};
  f.frontFacing = true;
  return f;
}

static ex::LightDesc pointLight(SFVec3f loc) {
  ex::LightDesc L;
  L.type = ex::LightDesc::Type::Point;
  L.worldLocation = loc;
  L.color = {1, 1, 1};
  L.intensity = 1.0f;
  return L;
}

int main() {
  const rt::Mat4 view = rt::Mat4::identity(); // eye == world for the test.

  // ---- A) A point light is not dropped (headlight off => one real light) ----
  {
    auto lights = render_detail::buildEyeLights({pointLight({0, 0, 0})}, view,
                                 /*headlightOn=*/false);
    CHECK(lights.size() == 1);
  }

  // ---- B) Per-fragment gradient: facing the light is brighter than grazing.
  // The flat-disc bug rendered these equal. Light at the camera origin; both
  // fragments equidistant, differing only in normal orientation. ----
  {
    auto lights = render_detail::buildEyeLights({pointLight({0, 0, 0})}, view, false);
    float facing = litLuma(lights, frag({0, 0, -3}, {0, 0, 1}));  // N·L = 1
    float grazing = litLuma(lights, frag({0, 0, -3}, {0, 1, 0})); // N·L = 0
    CHECK(facing > grazing + 0.1f);
  }

  // ---- C) Distance attenuation: nearer is brighter (linear attenuation). ----
  {
    ex::LightDesc L = pointLight({0, 0, 0});
    L.attenuation = {0, 1, 0}; // pure linear: factor ~ 1/d
    auto lights = render_detail::buildEyeLights({L}, view, false);
    float near = litLuma(lights, frag({0, 0, -2}, {0, 0, 1}));
    float far = litLuma(lights, frag({0, 0, -6}, {0, 0, 1}));
    CHECK(near > far + 0.05f);
  }

  // ---- D) Radius cutoff: a fragment beyond `radius` gets no contribution. ----
  {
    ex::LightDesc small = pointLight({0, 0, 0});
    small.radius = 1.0f; // fragment at distance 5 is outside.
    ex::LightDesc big = pointLight({0, 0, 0});
    big.radius = 100.0f;
    FragmentInput f = frag({0, 0, -5}, {0, 0, 1});
    float inside = litLuma(render_detail::buildEyeLights({big}, view, false), f);
    float outside = litLuma(render_detail::buildEyeLights({small}, view, false), f);
    CHECK(inside > 0.1f);       // lit within radius
    CHECK(outside < inside - 0.1f); // dark beyond radius
  }

  // ---- E) Spot cone: on-axis is lit, well outside the cutOffAngle is dark.
  // Both fragments' normals face the light, so only the cone explains the
  // difference. ----
  {
    ex::LightDesc s;
    s.type = ex::LightDesc::Type::Spot;
    s.worldLocation = {0, 0, 0};
    s.worldDirection = {0, 0, -1}; // beam shines down -Z into the scene.
    s.color = {1, 1, 1};
    s.intensity = 1.0f;
    s.beamWidth = 0.2f;
    s.cutOffAngle = 0.3f; // ~17°
    auto lights = render_detail::buildEyeLights({s}, view, false);
    // On axis: 5 units down -Z, normal back toward the light.
    float onAxis = litLuma(lights, frag({0, 0, -5}, {0, 0, 1}));
    // Off axis: ~45° from the beam axis (outside the 17° cutoff), normal still
    // toward the light so N·L > 0.
    float offAxis = litLuma(lights, frag({5, 0, -5}, {-0.707f, 0, 0.707f}));
    CHECK(onAxis > 0.1f);
    CHECK(offAxis < onAxis - 0.1f);
  }

  // ---- F) Regression guard: directional lights still work through the same
  // path (the refactor must not break the original case). ----
  {
    ex::LightDesc d;
    d.type = ex::LightDesc::Type::Directional;
    d.worldDirection = {0, 0, -1}; // travels into the screen.
    d.color = {1, 1, 1};
    d.intensity = 1.0f;
    auto lights = render_detail::buildEyeLights({d}, view, false);
    CHECK(lights.size() == 1);
    float lit = litLuma(lights, frag({0, 0, -3}, {0, 0, 1})); // N·L = 1
    CHECK(lit > 0.1f);
  }

  if (failures) {
    std::fprintf(stderr, "positional_light_test: %d failure(s)\n", failures);
    return 1;
  }
  std::printf("positional_light_test: OK\n");
  return 0;
}
