// material_shader_test.cpp — the three MaterialModel ports produce sensible,
// model-distinct fragment colors for a head-on lit fragment.
#include "RenderItem.hpp"
#include "cpuraster/MaterialShader.hpp"
#include "cpuraster/Rasterizer.hpp"

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

// A fragment facing the camera at the origin-ish, front-facing, no texcoords.
static FragmentInput frontFrag() {
  FragmentInput f;
  f.posEye = {0, 0, -5};
  f.normalEye = {0, 0, 1};
  f.color = {1, 1, 1, 1};
  f.frontFacing = true;
  f.texcoord = {0.5f, 0.5f};
  return f;
}

// A 1x1 inline (PixelTexture) ref for the given slot, grey level `v` on all RGB.
static ex::TextureRef inlineRef(ex::TextureRef::Slot slot, std::uint8_t v) {
  ex::TextureRef r;
  r.slot = slot;
  r.source = ex::TextureRef::Source::Inline;
  r.inlinePixels.width = 1;
  r.inlinePixels.height = 1;
  r.inlinePixels.numComponents = 4;
  r.inlinePixels.data = {v, v, v, 255};
  return r;
}

static std::vector<EyeLight> oneLight() {
  EyeLight L;
  L.dirEye = {0, 0, -1}; // travels into the screen; -dir points at the camera.
  L.color = {1, 1, 1};
  return {L};
}

int main() {
  FragmentInput f = frontFrag();
  auto lights = oneLight();

  // ---- Phong: red diffuse -> red-dominant lit color ----
  {
    ex::MaterialDesc m;
    m.model = ex::MaterialModel::Phong;
    m.phong.diffuse = {0.8f, 0.0f, 0.0f};
    m.phong.ambientIntensity = 0.2f;
    FragmentShader fs = makePhongShader(m, lights, /*hasColors=*/false);
    g::vec4 o; bool kept = fs(f, o);
    CHECK(kept);
    CHECK(o.x > 0.3f && o.x > o.y + 0.2f && o.x > o.z + 0.2f); // red dominant.
    CHECK(o.w > 0.99f); // opaque.
  }

  // ---- Physical: blue base, dielectric -> blue-dominant lit color ----
  {
    ex::MaterialDesc m;
    m.model = ex::MaterialModel::Physical;
    m.physical.baseColor = {0.0f, 0.0f, 0.8f};
    m.physical.metallic = 0.0f;
    m.physical.roughness = 0.5f;
    FragmentShader fs = makePbrShader(m, lights, false);
    g::vec4 o; bool kept = fs(f, o);
    CHECK(kept);
    CHECK(o.z > 0.1f && o.z > o.x && o.z > o.y); // blue dominant.
  }

  // ---- Unlit: emissive green, lighting-independent ----
  {
    ex::MaterialDesc m;
    m.model = ex::MaterialModel::Unlit;
    m.emissive = {0.0f, 1.0f, 0.0f};
    FragmentShader fs = makeUnlitShader(m, false);
    g::vec4 o; bool kept = fs(f, o);
    CHECK(kept);
    CHECK(o.y > 0.9f && o.x < 0.1f && o.z < 0.1f); // pure green, unshaded.
  }

  // ---- Per-vertex color override (Phong hasColors) ----
  {
    ex::MaterialDesc m;
    m.model = ex::MaterialModel::Phong;
    m.phong.diffuse = {0.0f, 0.0f, 0.0f}; // black material...
    FragmentInput fc = frontFrag();
    fc.color = {1.0f, 0.0f, 0.0f, 1.0f}; // ...overridden red by vertex color.
    FragmentShader fs = makePhongShader(m, lights, /*hasColors=*/true);
    g::vec4 o; fs(fc, o);
    CHECK(o.x > o.y && o.x > o.z); // red wins despite black material.
  }

  // ---- Mask discard: transparent base under MASK below cutoff -> discard ----
  {
    ex::MaterialDesc m;
    m.model = ex::MaterialModel::Phong;
    m.phong.diffuse = {1, 1, 1};
    m.alphaMode = ex::AlphaMode::Mask;
    m.alphaCutoff = 0.5f;
    m.transparency = 0.9f; // alpha = 0.1 < cutoff.
    FragmentShader fs = makePhongShader(m, lights, false);
    g::vec4 o; bool kept = fs(f, o);
    CHECK(!kept); // discarded.
  }

  // ---- AO source: occlusionTexture only, never derived from the MR texture ----
  {
    // MR texture present with a BLACK R channel + a WHITE occlusion texture.
    // Old code read AO from tx.mr.x (=0) and ignored the bound occlusion slot;
    // correct code reads the occlusion slot (=1) -> full ambient, no darkening.
    ex::MaterialDesc m;
    m.model = ex::MaterialModel::Physical;
    m.physical.baseColor = {1, 1, 1};
    m.physical.metallic = 0.0f;
    m.physical.roughness = 0.5f;
    m.physical.occlusionStrength = 1.0f;
    m.textures.push_back(inlineRef(ex::TextureRef::Slot::MetallicRoughness, 0));
    m.textures.push_back(inlineRef(ex::TextureRef::Slot::Occlusion, 255));
    FragmentShader fs = makePbrShader(m, {}, /*hasColors=*/false);
    g::vec4 o; fs(f, o);
    CHECK(o.x > 0.1f); // ambient survives: AO came from occlusion (1), not MR (0).
  }
  {
    // MR texture present (black R), NO occlusion bound: AO must be 1 (the MR
    // texture's R is not an occlusion source), not the MR R value.
    ex::MaterialDesc m;
    m.model = ex::MaterialModel::Physical;
    m.physical.baseColor = {1, 1, 1};
    m.physical.metallic = 0.0f;
    m.physical.roughness = 0.5f;
    m.physical.occlusionStrength = 1.0f;
    m.textures.push_back(inlineRef(ex::TextureRef::Slot::MetallicRoughness, 0));
    FragmentShader fs = makePbrShader(m, {}, /*hasColors=*/false);
    g::vec4 o; fs(f, o);
    CHECK(o.x > 0.1f); // no AO darkening from the MR texture.
  }
  {
    // Control: a bound occlusion texture with a BLACK R channel DOES darken.
    ex::MaterialDesc m;
    m.model = ex::MaterialModel::Physical;
    m.physical.baseColor = {1, 1, 1};
    m.physical.metallic = 0.0f;
    m.physical.roughness = 0.5f;
    m.physical.occlusionStrength = 1.0f;
    m.textures.push_back(inlineRef(ex::TextureRef::Slot::Occlusion, 0));
    FragmentShader fs = makePbrShader(m, {}, /*hasColors=*/false);
    g::vec4 o; fs(f, o);
    CHECK(o.x < 0.02f); // AO=0 -> ambient term vanishes.
  }

  if (failures) { std::fprintf(stderr, "material_shader_test: %d failure(s)\n", failures); return 1; }
  std::printf("material_shader_test: OK\n");
  return 0;
}
