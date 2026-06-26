// tier1_features_test.cpp — extracted-but-ignored seam data the rasterizer now
// consumes: (A) the Background sky/ground gradient, (B) two-sided materials via
// Appearance.backMaterial, and (C) TextureCoordinateGenerator (Sphere) UVs.
// Behavioural assertions through the public render/shader API.
#include "RenderItem.hpp"
#include "cpuraster/MaterialShader.hpp"
#include "cpuraster/SceneRender.hpp"

#include <cstdio>
#include <memory>
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

static FragmentInput frag(g::vec3 posEye, g::vec3 normalEye, bool front) {
  FragmentInput f;
  f.posEye = posEye;
  f.normalEye = normalEye;
  f.color = {1, 1, 1, 1};
  f.texcoord = {0, 0};
  f.frontFacing = front;
  return f;
}

int main() {
  // ===================================================================== A) ==
  // Background sky/ground gradient: zenith=sky[0], nadir=ground, mid-sky blends.
  {
    const float PI = 3.14159265358979f;
    std::vector<SFColor> sky = {{1, 1, 1}, {0, 0, 1}}; // white zenith -> blue
    std::vector<float> skyA = {PI * 0.5f};             // blue at the horizon
    std::vector<SFColor> grd = {{0, 1, 0}};            // green ground
    std::vector<float> grdA = {PI * 0.5f};             // up to the horizon

    g::vec3 zenith = render_detail::skyGroundColor(0.0f, sky, skyA, grd, grdA);
    CHECK(zenith.x > 0.9f && zenith.y > 0.9f && zenith.z > 0.9f); // white

    g::vec3 nadir = render_detail::skyGroundColor(PI, sky, skyA, grd, grdA);
    CHECK(nadir.y > 0.9f && nadir.x < 0.1f && nadir.z < 0.1f); // green ground

    g::vec3 mid = render_detail::skyGroundColor(PI * 0.25f, sky, skyA, grd, grdA);
    CHECK(mid.x > 0.3f && mid.x < 0.7f); // halfway white->blue: red ~0.5

    // A single skyColor (no angles) stays flat — backward-compatible clear.
    g::vec3 flat = render_detail::skyGroundColor(1.0f, {{0.2f, 0.3f, 0.4f}}, {},
                                                 {}, {});
    CHECK(flat.x > 0.19f && flat.x < 0.21f && flat.z > 0.39f && flat.z < 0.41f);
  }

  // ===================================================================== B) ==
  // Two-sided material: front face uses the material, back face uses
  // backMaterial (different colour).
  {
    ex::MaterialDesc m;
    m.model = ex::MaterialModel::Phong;
    m.phong.diffuse = {1, 0, 0}; // red front
    m.phong.ambientIntensity = 0.5f;
    m.backMaterial = std::make_unique<ex::MaterialDesc>();
    m.backMaterial->model = ex::MaterialModel::Phong;
    m.backMaterial->phong.diffuse = {0, 0, 1}; // blue back
    m.backMaterial->phong.ambientIntensity = 0.5f;
    m.backMaterialConstraintMet = true;

    std::vector<EyeLight> noLights; // ambient-only isolates the material colour.
    FragmentShader fs = makeMaterialShader(m, noLights, /*hasColors=*/false,
                                           /*forceUnlit=*/false);
    g::vec4 of, ob;
    fs(frag({0, 0, -3}, {0, 0, 1}, /*front=*/true), of);
    fs(frag({0, 0, -3}, {0, 0, 1}, /*front=*/false), ob);
    CHECK(of.x > of.z + 0.1f); // front red-dominant
    CHECK(ob.z > ob.x + 0.1f); // back blue-dominant
  }

  // ===================================================================== C) ==
  // TextureCoordinateGenerator (Sphere): base-texture UVs come from the
  // camera-space normal, not the (here zero) authored texcoord. A 2x1 red|green
  // texture sampled with a +X normal (sphere u=1) reads GREEN; without UV-gen the
  // zero texcoord (u=0) would read RED.
  {
    ex::MaterialDesc m;
    m.model = ex::MaterialModel::Phong;
    m.phong.diffuse = {1, 1, 1};
    m.phong.ambientIntensity = 0.6f; // ambient carries the texel colour.
    ex::TextureRef tex;
    tex.slot = ex::TextureRef::Slot::BaseColor;
    tex.source = ex::TextureRef::Source::Inline;
    tex.inlinePixels = SFImage{2, 1, 3, {255, 0, 0, 0, 255, 0}}; // left red, right green
    tex.hasTexCoordGen = true;
    tex.texCoordGen.mode = ex::TexCoordGenMode::Sphere;
    m.textures.push_back(tex);

    std::vector<EyeLight> noLights;
    FragmentShader fs = makeMaterialShader(m, noLights, false, false);
    g::vec4 o;
    // Normal {0.6,0,0.8} -> sphere u = 0.6*0.5+0.5 = 0.8 (right half -> green).
    // (u must avoid 1.0, which wraps back to the red texel under repeat.)
    fs(frag({0, 0, -3}, {0.6f, 0, 0.8f}, /*front=*/true), o);
    CHECK(o.y > o.x + 0.1f); // green-dominant (would be red without UV-gen)
  }

  if (failures) {
    std::fprintf(stderr, "tier1_features_test: %d failure(s)\n", failures);
    return 1;
  }
  std::printf("tier1_features_test: OK\n");
  return 0;
}
