// fog_shader_test.cpp — ENV-10: the §17 fog term in the CPU material shaders.
//
// applyFog is the final step of every model (unlit/phong/pbr). Checked here
// directly against the spec formulas at d = 0, V/2 and d >= V, for LINEAR and
// EXPONENTIAL, and that visibilityRange 0 disables fog entirely.
#include "RenderItem.hpp"
#include "cpuraster/MaterialShader.hpp"
#include "cpuraster/Rasterizer.hpp"

#include <cmath>
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

static bool feq(float a, float b, float e = 1e-4f) { return std::fabs(a - b) < e; }

// applyFog parity against the raw §17 formulas, independently recomputed.
static g::vec3 specFog(g::vec3 c, float d, int type, float V, g::vec3 fogCol) {
  if (V <= 0.0f) return c;
  float f = 0.0f;
  if (d < V)
    f = (type == 1) ? std::exp(-d / (V - d)) : (V - d) / V;
  return c * f + fogCol * (1.0f - f);
}

int main() {
  const g::vec3 surface{0.2f, 0.4f, 0.6f};
  const g::vec3 fogCol{1.0f, 1.0f, 1.0f};

  // ---- LINEAR (type 0) at d = 0, V/2, V, >V -------------------------------
  {
    FogParams fog; fog.color = fogCol; fog.type = 0; fog.visibilityRange = 10.0f;
    CHECK(feq(g::length(detail::applyFog(surface, 0.0f, fog) - surface), 0.0f));
    CHECK(feq(g::length(detail::applyFog(surface, 5.0f, fog) - specFog(surface, 5.0f, 0, 10.0f, fogCol)), 0.0f));
    // d = V and d > V: fully fogged (f = 0).
    CHECK(feq(g::length(detail::applyFog(surface, 10.0f, fog) - fogCol), 0.0f));
    CHECK(feq(g::length(detail::applyFog(surface, 25.0f, fog) - fogCol), 0.0f));
    // Halfway through a 10m range the interpolant is (10-5)/10 = 0.5.
    CHECK(feq(g::length(detail::applyFog(surface, 5.0f, fog) - (surface * 0.5f + fogCol * 0.5f)), 0.0f));
  }

  // ---- EXPONENTIAL (type 1) at d = 0, V/2, V, >V --------------------------
  {
    FogParams fog; fog.color = fogCol; fog.type = 1; fog.visibilityRange = 10.0f;
    CHECK(feq(g::length(detail::applyFog(surface, 0.0f, fog) - surface), 0.0f)); // exp(0)=1.
    CHECK(feq(g::length(detail::applyFog(surface, 5.0f, fog) - specFog(surface, 5.0f, 1, 10.0f, fogCol)), 0.0f));
    CHECK(feq(g::length(detail::applyFog(surface, 10.0f, fog) - fogCol), 0.0f));
    CHECK(feq(g::length(detail::applyFog(surface, 25.0f, fog) - fogCol), 0.0f));
    // exp(-5/(10-5)) = exp(-1) ~ 0.3679.
    CHECK(feq(detail::applyFog(surface, 5.0f, fog).x,
              surface.x * std::exp(-1.0f) + fogCol.x * (1.0f - std::exp(-1.0f)), 1e-4f));
  }

  // ---- visibilityRange 0 disables fog (all distances unchanged) -----------
  {
    FogParams fog; fog.color = fogCol; fog.type = 0; fog.visibilityRange = 0.0f;
    CHECK(feq(g::length(detail::applyFog(surface, 0.0f, fog) - surface), 0.0f));
    CHECK(feq(g::length(detail::applyFog(surface, 1000.0f, fog) - surface), 0.0f));
  }

  // ---- Fog reaches the material models as the final step ------------------
  // Unlit: emissive green at d=0 stays green; far away it becomes fog white.
  {
    ex::MaterialDesc m;
    m.model = ex::MaterialModel::Unlit;
    m.emissive = {0.0f, 1.0f, 0.0f};
    FogParams fog; fog.color = fogCol; fog.type = 0; fog.visibilityRange = 10.0f;

    FragmentInput near; near.posEye = {0, 0, 0};   // d = 0 -> f = 1, no fog.
    FragmentInput far;  far.posEye = {0, 0, -40}; // d >= V.

    FragmentShader fs = makeUnlitShader(m, /*hasColors=*/false, fog);
    g::vec4 oNear, oFar;
    CHECK(fs(near, oNear));
    CHECK(fs(far, oFar));
    CHECK(oNear.y > 0.9f && oNear.x < 0.1f);       // near: unchanged green.
    CHECK(oFar.x > 0.9f && oFar.z > 0.9f && oFar.y > 0.9f); // far: fog white.
  }
  // Phong: a far fragment trends toward the fog colour.
  {
    ex::MaterialDesc m;
    m.model = ex::MaterialModel::Phong;
    m.phong.diffuse = {0.0f, 0.0f, 1.0f};
    FogParams fog; fog.color = {1.0f, 0.0f, 0.0f}; fog.type = 0; fog.visibilityRange = 10.0f;
    FragmentInput far; far.posEye = {0, 0, -40};
    FragmentShader fs = makePhongShader(m, {}, /*hasColors=*/false, fog);
    g::vec4 o; fs(far, o);
    CHECK(o.x > 0.9f && o.z < 0.1f); // red fog colour dominates.
  }
  // PBR: a far fragment trends toward the fog colour (after sRGB encode).
  {
    ex::MaterialDesc m;
    m.model = ex::MaterialModel::Physical;
    m.physical.baseColor = {0.0f, 0.0f, 1.0f};
    FogParams fog; fog.color = {1.0f, 0.0f, 0.0f}; fog.type = 0; fog.visibilityRange = 10.0f;
    FragmentInput far; far.posEye = {0, 0, -40};
    FragmentShader fs = makePbrShader(m, {}, /*hasColors=*/false, fog);
    g::vec4 o; fs(far, o);
    CHECK(o.x > 0.9f && o.z < 0.1f);
  }

  if (failures == 0) std::printf("fog_shader_test: OK\n");
  return failures == 0 ? 0 : 1;
}
