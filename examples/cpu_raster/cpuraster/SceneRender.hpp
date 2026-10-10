// SceneRender.hpp — orchestration: turn a snapshotted SceneExtractor into a
// Framebuffer. This is the CPU-rasterizer analogue of the PoC's render loop, and
// it deliberately reuses the PoC's conventions so a CPU frame matches the GL one:
//   * perspective() maps X3D's MIN-dimension fieldOfView to the shorter axis.
//   * a "view-all" fit camera frames the scene's per-path world bounds when no
//     Viewpoint is authored (X3DExecutionContext::viewMatrix() is identity then).
//   * eye-space directional + positional (point/spot) lights + the §23.4.4
//     NavigationInfo headlight (on whenever headlight is TRUE, the default).
//   * opaque pass (depth-write) then a back-to-front transparency pass.
//
// Per item the shader is chosen by MaterialModel (Phong/Physical/Unlit) unless an
// author program override is supplied (the GLSL interpreter path) — see
// makeAuthorShader in main.cpp, plumbed via the `authorShaderFor` hook.
//
// Out-of-SDK consumer code. namespace x3d::cpuraster.
#ifndef X3D_CPURASTER_SCENE_RENDER_HPP
#define X3D_CPURASTER_SCENE_RENDER_HPP

#include "Framebuffer.hpp"
#include "GeometryBounds.hpp"
#include "Intersect.hpp"
#include "MaterialShader.hpp"
#include "Rasterizer.hpp"
#include "SceneExtractor.hpp"
#include "X3DExecutionContext.hpp"
#include "glsl.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace x3d::cpuraster {
using namespace x3d::core;  // SFColor etc. (ADR-0039 namespaces)
using namespace x3d::nodes; // X3DNode (ADR-0039 namespaces)

namespace rt = x3d::runtime;

namespace render_detail { struct SkyboxTextures; } // defined below.

// §34.4.2 GeneratedCubeMapTexture faces, keyed by TextureRef::generatedCube.node
// (six RGBA8 layers, front, back, left, right, top, bottom). Kept across frames
// by the caller: a cube whose update is NONE shows the faces last rendered.
struct GeneratedCubeCache {
  std::unordered_map<const void *, rt::extract::TexturePixelsRef> faces;
};

struct RenderOptions {
  int width = 800;
  int height = 600;
  glsl::vec3 clearColor{0.10f, 0.12f, 0.18f}; // overridden by bound Background.
  // Optional glyph atlas (BuiltinFont::atlas). When set, Text glyph meshes
  // (MeshData::isGlyphMesh) sample it (alpha-tested) so text renders as letters
  // instead of solid cells. Must be paired with the matching FontMetrics passed
  // to the extractor via MeshBuildOptions. Null => glyph cells fill flat.
  const Texture *glyphAtlas = nullptr;
  // Optional hook: given a RenderItem, return a FragmentShader to use INSTEAD of
  // the material model (the GLSL-interpreter author path). Return a default-
  // constructed (empty) std::function to fall through to the material shader.
  std::function<FragmentShader(const rt::extract::RenderItem &,
                               const std::vector<EyeLight> &, bool /*hasColors*/)>
      authorShaderFor;
  // Optional skybox: six resolved panorama faces (Background *Url fields). Null
  // => no skybox; faces composite over the sky/ground gradient by alpha.
  const render_detail::SkyboxTextures *skybox = nullptr;
  // Optional persistent GeneratedCubeMapTexture cache. Null => a per-call cache,
  // so only cubes whose update is not NONE are drawn (NONE leaves them white).
  GeneratedCubeCache *generatedCubes = nullptr;
};

namespace render_detail {

// X3D min-dimension perspective (column-major, GL clip-z in [-1,1]) — identical
// math to the PoC's perspective().
inline rt::Mat4 perspective(float minFov, float aspect, float zNear, float zFar) {
  rt::Mat4 r{}; // zero
  float fovY = minFov;
  if (aspect < 1.0f)
    fovY = 2.0f * std::atan(std::tan(minFov * 0.5f) / aspect);
  const float fF = 1.0f / std::tan(fovY * 0.5f);
  r.m[0] = fF / aspect;
  r.m[5] = fF;
  r.m[10] = (zFar + zNear) / (zNear - zFar);
  r.m[11] = -1.0f;
  r.m[14] = (2.0f * zFar * zNear) / (zNear - zFar);
  return r;
}

inline glsl::vec3 v3norm(glsl::vec3 a) { return glsl::normalize(a); }

// One X3D colour ramp: cols[0] is the pole colour (angle 0); cols[i] sits at
// angs[i-1] (the angle array holds the interior breakpoints, one fewer than the
// colours). Linearly interpolates between segments; clamps past the last angle.
inline glsl::vec3 colorRamp(float ang, const std::vector<SFColor> &cols,
                            const std::vector<float> &angs) {
  const glsl::vec3 c0 = glsl::vec3(cols.front());
  if (cols.size() == 1 || angs.empty()) return c0;
  if (ang <= angs[0]) {
    const float t = angs[0] > 1e-6f ? ang / angs[0] : 0.0f;
    return c0 + (glsl::vec3(cols[1]) - c0) * t;
  }
  for (std::size_t i = 1; i < angs.size() && i + 1 < cols.size(); ++i) {
    if (ang <= angs[i]) {
      const float span = angs[i] - angs[i - 1];
      const float t = span > 1e-6f ? (ang - angs[i - 1]) / span : 0.0f;
      return glsl::vec3(cols[i]) + (glsl::vec3(cols[i + 1]) - glsl::vec3(cols[i])) * t;
    }
  }
  return glsl::vec3(cols.back());
}

// The panorama cube shares the environment-texture face convention
// (Texture.hpp cubeFaceUv).
using x3d::cpuraster::CubeFace;
using x3d::cpuraster::FaceSample;
using x3d::cpuraster::cubeFaceUv;

// Six resolved panorama faces (X3D Background *Url fields).
struct SkyboxTextures {
  Texture front, back, right, left, top, bottom;
  bool any() const {
    return front.valid() || back.valid() || right.valid() || left.valid() ||
           top.valid() || bottom.valid();
  }
  const Texture &face(CubeFace f) const {
    switch (f) {
      case CubeFace::Front:  return front;
      case CubeFace::Back:   return back;
      case CubeFace::Right:  return right;
      case CubeFace::Left:   return left;
      case CubeFace::Top:    return top;
      case CubeFace::Bottom: return bottom;
    }
    return front;
  }
};

// Panorama colour for a view direction: the face texel composited over the
// sky/ground gradient by its alpha (§Background — the cube draws in front of the
// gradient, alpha reveals it). No/empty face -> the gradient unchanged.
inline glsl::vec3 skyboxColor(const glsl::vec3 &dir, const SkyboxTextures &sb,
                              const glsl::vec3 &gradient) {
  const FaceSample fs = cubeFaceUv(dir);
  const Texture &t = sb.face(fs.face);
  if (!t.valid()) return gradient;
  const glsl::vec4 texel = t.sample(fs.uv);
  const float a = glsl::clampf(texel.w, 0.0f, 1.0f);
  return gradient + (texel.xyz() - gradient) * a; // mix(gradient, texel, a)
}

// X3D Background sky/ground sphere (§Background). `angleFromUp` ∈ [0,π] is the
// angle between the view ray and +Y (0 = zenith, π = nadir). The sky ramp keys
// off the zenith; the ground ramp keys off the nadir and occludes the sky where
// it is defined. A lone skyColor with no angles stays flat (the old clear).
inline glsl::vec3 skyGroundColor(float angleFromUp,
                                 const std::vector<SFColor> &skyColor,
                                 const std::vector<float> &skyAngle,
                                 const std::vector<SFColor> &groundColor,
                                 const std::vector<float> &groundAngle) {
  const glsl::vec3 sky = skyColor.empty()
                             ? glsl::vec3{0, 0, 0}
                             : colorRamp(angleFromUp, skyColor, skyAngle);
  if (!groundColor.empty()) {
    constexpr float kPi = 3.14159265358979323846f;
    const float fromNadir = kPi - angleFromUp;
    const float maxGround = groundAngle.empty() ? 0.0f : groundAngle.back();
    if (fromNadir <= maxGround)
      return colorRamp(fromNadir, groundColor, groundAngle);
  }
  return sky;
}

inline rt::Mat4 lookAt(glsl::vec3 eye, glsl::vec3 center, glsl::vec3 up) {
  glsl::vec3 f = v3norm(center - eye);
  glsl::vec3 s = v3norm(glsl::cross(f, up));
  glsl::vec3 u = glsl::cross(s, f);
  rt::Mat4 m = rt::Mat4::identity();
  m.m[0] = s.x; m.m[4] = s.y; m.m[8] = s.z;   m.m[12] = -glsl::dot(s, eye);
  m.m[1] = u.x; m.m[5] = u.y; m.m[9] = u.z;   m.m[13] = -glsl::dot(u, eye);
  m.m[2] = -f.x; m.m[6] = -f.y; m.m[10] = -f.z; m.m[14] = glsl::dot(f, eye);
  return m;
}

// Build the eye-space light set: every directional, point, and spot LightDesc is
// resolved into eye space (directions via transformDirection, positions via
// transformPoint). The bound NavigationInfo headlight is added as a camera-space
// directional light WHENEVER headlight is TRUE (default), independent of the
// scene's own lights — §23.4.4: headlight TRUE means the browser turns on a
// headlight regardless of other lights (FALSE turns it off).
inline std::vector<EyeLight>
buildEyeLights(const std::vector<rt::extract::LightDesc> &lights,
               const rt::Mat4 &view, bool headlightOn) {
  using Type = rt::extract::LightDesc::Type;
  std::vector<EyeLight> out;
  // Annex B.6 requires eight authored lights; the browser headlight is extra.
  for (const auto &L : lights) {
    EyeLight e;
    e.color = glsl::vec3{L.color.r, L.color.g, L.color.b};
    e.intensity = L.intensity;
    e.ambientIntensity = L.ambientIntensity;
    if (L.type == Type::Directional) {
      e.dirEye = view.transformDirection(L.worldDirection);
    } else {
      // Point or Spot: a positional light at an eye-space location.
      e.positional = true;
      e.posEye = view.transformPoint(L.worldLocation);
      e.attenuation =
          glsl::vec3{L.attenuation.x, L.attenuation.y, L.attenuation.z};
      e.radius = L.radius;
      if (L.type == Type::Spot) {
        e.isSpot = true;
        e.spotDirEye = view.transformDirection(L.worldDirection);
        e.beamWidth = L.beamWidth;
        e.cutOffAngle = L.cutOffAngle;
      }
    }
    out.push_back(e);
  }
  if (headlightOn) {
    EyeLight h;
    h.dirEye = glsl::vec3{0, 0, -1}; // straight down the camera's -Z.
    h.color = glsl::vec3{1, 1, 1};
    // §23.4.4 pins the headlight exactly: intensity 1, color (1 1 1),
    // ambientIntensity 0.0, direction (0 0 -1).
    h.ambientIntensity = 0.0f;
    out.push_back(h);
  }
  return out;
}

// MeshData -> rasterizer Vertex array (defaults fill missing normal/color/uv).
inline std::vector<Vertex> toVertices(const rt::extract::MeshData &m) {
  std::vector<Vertex> v(m.positions.size());
  for (std::size_t i = 0; i < m.positions.size(); ++i) {
    v[i].pos = m.positions[i];
    if (m.texcoordSets.size() > 1)
      for (const auto &set : m.texcoordSets)
        v[i].texcoordSets.push_back(i < set.size() ? glsl::vec2(set[i])
                                                   : glsl::vec2{});
    v[i].normal = (i < m.normals.size()) ? glsl::vec3(m.normals[i])
                                         : glsl::vec3{0, 1, 0};
    v[i].color = (m.hasColors && i < m.colors.size())
                     ? glsl::vec4(m.colors[i])
                     : glsl::vec4{1, 1, 1, 1};
    v[i].texcoord = (i < m.texcoords.size()) ? glsl::vec2(m.texcoords[i])
                                             : glsl::vec2{0, 0};
  }
  return v;
}

inline glsl::vec3 centroid(const rt::extract::MeshData &m) {
  glsl::vec3 c{0, 0, 0};
  if (m.positions.empty()) return c;
  for (const auto &p : m.positions) c = c + glsl::vec3(p);
  return c / static_cast<float>(m.positions.size());
}

// Each TextureRef reachable from a material (MultiTexture stages and cube
// faces included); `Refs` is a const or mutable TextureRef vector.
template <typename Refs, typename F> void forEachTextureRef(Refs &refs, F &&f) {
  for (auto &r : refs) {
    f(r);
    forEachTextureRef(r.multiStages, f);
    forEachTextureRef(r.cubeFaces, f);
  }
}

inline bool usesGeneratedCube(const rt::extract::RenderItem &it,
                              const void *node) {
  if (!node) return false;
  bool uses = false;
  auto check = [&](const rt::extract::TextureRef &r) {
    uses = uses || r.generatedCube.node == node;
  };
  forEachTextureRef(it.material.textures, check);
  if (it.material.backMaterial)
    forEachTextureRef(it.material.backMaterial->textures, check);
  return uses;
}

// Draw the scene from one camera into a width x height framebuffer. Items
// whose material uses the GeneratedCubeMapTexture `exclude` are skipped (a
// generated cube does not see the geometry it is applied to); generated cube
// refs sample their faces from `cubes`.
inline Framebuffer renderView(const rt::X3DExecutionContext &ctx,
                              rt::extract::SceneExtractor &extractor,
                              const RenderOptions &opt, const rt::Mat4 &viewRT,
                              const rt::Mat4 &projRT, int width, int height,
                              const void *exclude,
                              const GeneratedCubeCache &cubes) {
  namespace ex = rt::extract;

  Framebuffer fb(width, height);

  // ---- Background flat clear (bound Background's first skyColor) -------------
  // The full sky/ground gradient is painted per-pixel below once the camera is
  // known; this flat fill is the fallback (no Background, or a single skyColor)
  // and the initial clear under the gradient.
  glsl::vec3 clear = opt.clearColor;
  if (const X3DNode *bg = ctx.boundBackground()) {
    auto sky = rt::geombounds::getField<std::vector<SFColor>>(*bg, "skyColor", {});
    if (!sky.empty()) clear = glsl::vec3(sky[0]);
  }
  fb.clear(clear);
  if (extractor.itemCount() == 0) return fb;

  const rt::Aabb bounds = extractor.sceneWorldBounds();
  const glsl::mat4 viewG(viewRT), projG(projRT);

  // ---- Background sky/ground gradient (overwrites the flat clear) -----------
  // When the bound Background carries a real ramp (>1 sky colour or any ground),
  // each pixel's view ray is unprojected to a world direction whose angle from
  // +Y selects the §Background sky/ground colour. A single skyColor stays flat.
  if (const X3DNode *bg = ctx.boundBackground()) {
    const auto skyC = rt::geombounds::getField<std::vector<SFColor>>(*bg, "skyColor", {});
    const auto skyA = rt::geombounds::getField<std::vector<float>>(*bg, "skyAngle", {});
    const auto grC = rt::geombounds::getField<std::vector<SFColor>>(*bg, "groundColor", {});
    const auto grA = rt::geombounds::getField<std::vector<float>>(*bg, "groundAngle", {});
    const bool hasSkybox = opt.skybox && opt.skybox->any();
    if (skyC.size() > 1 || !grC.empty() || hasSkybox) {
      const rt::Mat4 invView = viewRT.inverse();
      const float p0 = projRT.m[0], p5 = projRT.m[5];
      for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
          const float xn = 2.0f * (x + 0.5f) / width - 1.0f;
          const float yn = 2.0f * (y + 0.5f) / height - 1.0f; // fb is bottom-up
          const SFVec3f wd =
              invView.transformDirection(SFVec3f{xn / p0, yn / p5, -1.0f});
          const glsl::vec3 d = v3norm(glsl::vec3(wd));
          const float ang = std::acos(glsl::clampf(d.y, -1.0f, 1.0f));
          glsl::vec3 bg = skyGroundColor(ang, skyC, skyA, grC, grA);
          if (hasSkybox) bg = skyboxColor(d, *opt.skybox, bg);
          fb.setColor(x, y, glsl::vec4(bg, 1.0f));
        }
      }
    }
  }

  // ---- Lights ---------------------------------------------------------------
  bool headlightOn = true;
  if (const X3DNode *nav = ctx.boundNavigationInfo())
    headlightOn = rt::geombounds::getField<bool>(*nav, "headlight", true);
  const std::vector<ex::LightDesc> lights = extractor.lights();
  std::vector<EyeLight> eyeLights = buildEyeLights(lights, viewRT, headlightOn);
  // §17.2.2 shadowTest: only visible Shapes with castShadow TRUE occlude.
  // Build eye-space triangles once, shared by every shadow-enabled light.
  if (std::any_of(lights.begin(), lights.end(),
                  [](const auto &l) { return l.shadows; })) {
    const auto extent = bounds.size();
    const float scale = std::hypot(extent.x, extent.y, extent.z);
    const float inverseScale = scale > 0 ? 1.0f / scale : 1.0f;
    auto triangles = std::make_shared<std::vector<std::array<SFVec3f, 3>>>();
    for (ex::RenderItemId id = 0; id < extractor.itemCount(); ++id) {
      const auto &it = extractor.item(id);
      if (!it.castShadow || it.mesh->topology != ex::Topology::Triangles ||
          usesGeneratedCube(it, exclude))
        continue;
      const ex::MeshData deformed =
          it.skin ? extractor.deformedMesh(id) : ex::MeshData{};
      const auto &mesh = it.skin ? deformed : *it.mesh;
      const auto mv = viewRT * it.worldTransform;
      for (std::size_t j = 0; j + 2 < mesh.indices.size(); j += 3) {
        std::array<SFVec3f, 3> t;
        for (int k = 0; k < 3; ++k)
          t[k] = (glsl::vec3(
                      mv.transformPoint(mesh.positions[mesh.indices[j + k]])) *
                  inverseScale)
                     .toSF();
        triangles->push_back(t);
      }
    }
    for (std::size_t i = 0; i < lights.size(); ++i) {
      if (!lights[i].shadows)
        continue;
      const auto light = eyeLights[i];
      const float intensity = lights[i].shadowIntensity;
      eyeLights[i].shadowVisibility = [triangles, light, intensity,
                                       inverseScale](const glsl::vec3 &p) {
        const auto delta = light.positional ? light.posEye - p : -light.dirEye;
        const float distance = light.positional
                                   ? glsl::length(delta)
                                   : std::numeric_limits<float>::infinity();
        const auto direction = glsl::normalize(delta);
        // Offset along the ray to avoid numerical self-intersections.
        constexpr float epsilon = 1e-5f;
        rt::Ray ray{(p * inverseScale + direction * epsilon).toSF(),
                    direction.toSF()};
        for (const auto &t : *triangles)
          if (auto hit = rt::rayTriangle(ray, t[0], t[1], t[2]);
              hit && *hit > epsilon && *hit < distance * inverseScale - epsilon)
            return 1.0f - intensity;
        return 1.0f;
      };
    }
  }

  // §24.4.2: the bound Fog reduced for the fragment shaders. visibilityRange is
  // world-scaled by the extractor; 0 disables fog (applyFog no-ops).
  FogParams globalFog;
  {
    const ex::FogDesc fd = extractor.fog();
    globalFog.color = glsl::vec3(fd.color);
    globalFog.type = (fd.fogType == ex::FogDesc::Type::Exponential) ? 1 : 0;
    globalFog.visibilityRange = fd.visibilityRange;
  }

  Rasterizer raster(fb);

  // ---- Partition opaque vs transparent (PoC B7 rule) ------------------------
  std::vector<ex::RenderItemId> opaque;
  std::vector<std::pair<float, ex::RenderItemId>> blended; // (eyeZ, id)
  for (ex::RenderItemId id = 0; id < extractor.itemCount(); ++id) {
    const ex::RenderItem &it = extractor.item(id);
    if (it.mesh->positions.empty()) continue;
    const ex::MaterialDesc &mat = it.material;
    const bool isBlended =
        (mat.alphaMode == ex::AlphaMode::Blend) || (mat.transparency > 0.0f);
    if (!isBlended) {
      opaque.push_back(id);
    } else {
      glsl::vec3 wc = it.worldTransform.transformPoint(centroid(*it.mesh).toSF());
      glsl::vec3 ec = viewRT.transformPoint(wc.toSF());
      blended.emplace_back(ec.z, id);
    }
  }
  std::sort(blended.begin(), blended.end(),
            [](const auto &a, const auto &b) { return a.first < b.first; });

  auto drawOne = [&](ex::RenderItemId id, BlendMode blend) {
    const ex::RenderItem *source = &extractor.item(id);
    // §34.4.2: give generated cube refs their rendered faces, as if decoded
    // from a six-layer image (Texture::fromCubeRef).
    ex::RenderItem withFaces;
    bool generated = false;
    auto scan = [&](const ex::TextureRef &r) {
      generated = generated || r.generatedCube.node;
    };
    forEachTextureRef(source->material.textures, scan);
    if (source->material.backMaterial)
      forEachTextureRef(source->material.backMaterial->textures, scan);
    if (generated) {
      if (usesGeneratedCube(*source, exclude)) return;
      withFaces = *source;
      auto patch = [&](ex::TextureRef &r) {
        if (!r.generatedCube.node) return;
        auto found = cubes.faces.find(r.generatedCube.node);
        r.resolvedPixels = found == cubes.faces.end()
                               ? ex::TexturePixelResult::makeFailed()
                               : ex::TexturePixelResult::makeReady(found->second);
      };
      forEachTextureRef(withFaces.material.textures, patch);
      if (withFaces.material.backMaterial)
        forEachTextureRef(withFaces.material.backMaterial->textures, patch);
      source = &withFaces;
    }
    const ex::RenderItem &it = *source;

    // §24.4.3: an item inside a LocalFog's grouping scope is fogged by that
    // LocalFog (nearest wins); otherwise the bound global Fog applies.
    FogParams fog = globalFog;
    if (it.localFog >= 0 && it.localFog < (int)extractor.snapshotLocalFogs().size()) {
      const ex::LocalFogDesc &lf = extractor.snapshotLocalFogs()[it.localFog];
      fog.color = glsl::vec3(lf.color);
      fog.type = (lf.fogType == ex::FogDesc::Type::Exponential) ? 1 : 0;
      fog.visibilityRange = lf.visibilityRange;
    }
    const ex::MeshData deformed = it.skin ? extractor.deformedMesh(id) : ex::MeshData{};
    const ex::MeshData &mesh = it.skin ? deformed : *it.mesh;
    if (mesh.positions.empty() || mesh.indices.empty()) return;
    const std::vector<Vertex> verts = toVertices(mesh);
    const glsl::mat4 modelG(it.worldTransform);
    const glsl::mat3 normalMat = glsl::normalMatrix(viewRT * it.worldTransform);
    const glsl::vec4 baseColor = glsl::vec4(it.material.toRGBA());
    const ex::FillPropertiesDesc &fill = it.material.fill;

    // REQ-CLIP (§11.4.1): the descriptor carries WORLD-space clip planes;
    // the rasterizer tests them in eye space, so map each through the view
    // matrix as a plane (p_eye = view^-T p_world).
    std::vector<glsl::vec4> clipEye;
    for (const ex::ClipPlaneDesc &cp : it.clipPlanes) {
      const SFVec4f q = rt::transformPlane(viewRT, cp.planeWorld);
      clipEye.push_back(glsl::vec4{q.x, q.y, q.z, q.w});
    }

    // Text glyph quads: sample the glyph atlas (alpha-tested) so letters render
    // as shapes, not solid cells. Unlit, double-sided (text reads from both
    // sides), in the material's color. Falls through to the normal path when no
    // atlas is wired (cells fill flat — the SDK-stub behavior).
    if (mesh.isGlyphMesh && opt.glyphAtlas) {
      const Texture *atlas = opt.glyphAtlas;
      const glsl::vec4 ink = baseColor;
      FragmentShader glyphFs = [atlas, ink](const FragmentInput &f,
                                            glsl::vec4 &out) -> bool {
        glsl::vec4 t = atlas->sample(f.texcoord);
        if (t.w < 0.5f) return false; // transparent background -> discard.
        out = glsl::vec4(ink.xyz(), ink.w);
        return true;
      };
      raster.drawTriangles(verts, mesh.indices, modelG, viewG, projG, normalMat,
                           mesh.ccw, /*solid=*/false, blend,
                           withFillProperties(glyphFs, fill, fog), fill,
                           clipEye);
      return;
    }

    // §11.2.2.5: authored normals light lines/points; otherwise use their
    // unlit material color, which differs from a lit Material's diffuse color.
    const glsl::vec4 linePointColor =
        glsl::vec4(it.material.unlitGeometryRGBA());
    const FragmentShader linePointShader = mesh.hasNormals
        ? makeMaterialShader(it.material, eyeLights, mesh.hasColors, false, fog)
        : FragmentShader{};
    if (mesh.topology == ex::Topology::Lines) {
      // §12.4.6 LineProperties.linewidthScaleFactor (0/absent => default width).
      const float w = it.material.line.applied && it.material.line.linewidthScaleFactor > 0.0f
                          ? it.material.line.linewidthScaleFactor
                          : 1.0f;
      raster.drawLines(verts, mesh.indices, modelG, viewG, projG, linePointColor,
                       mesh.hasColors, w, linePointShader);
      return;
    }
    if (mesh.topology == ex::Topology::Points) {
      // §12.4.8 PointProperties: scale + distance attenuation clamped to [min,max].
      const ex::PointPropertiesDesc &pp = it.material.point;
      raster.drawPoints(verts, mesh.indices, modelG, viewG, projG, linePointColor,
                        mesh.hasColors, pp.pointSizeScaleFactor,
                        glsl::vec3{pp.attenuation.x, pp.attenuation.y,
                                   pp.attenuation.z},
                        pp.pointSizeMinValue, pp.pointSizeMaxValue,
                        linePointShader);
      return;
    }

    const bool forceUnlit = !mesh.hasNormals;
    FragmentShader fs;
    if (opt.authorShaderFor) fs = opt.authorShaderFor(it, eyeLights, mesh.hasColors);
    if (!fs)
      fs = makeMaterialShader(it.material, eyeLights, mesh.hasColors, forceUnlit,
                              fog);
    raster.drawTriangles(verts, mesh.indices, modelG, viewG, projG, normalMat,
                         mesh.ccw, mesh.solid, blend,
                         withFillProperties(fs, fill, fog), fill, clipEye);
  };

  for (ex::RenderItemId id : opaque) drawOne(id, BlendMode::Opaque);
  for (const auto &kv : blended) drawOne(kv.second, BlendMode::Blend);

  return fb;
}

} // namespace render_detail

inline Framebuffer renderScene(const rt::X3DExecutionContext &ctx,
                               rt::extract::SceneExtractor &extractor,
                               const RenderOptions &opt) {
  namespace ex = rt::extract;
  using namespace render_detail;

  if (extractor.itemCount() == 0) {
    GeneratedCubeCache none;
    return renderView(ctx, extractor, opt, rt::Mat4{}, rt::Mat4{}, opt.width,
                      opt.height, nullptr, none);
  }

  // ---- Camera: bound Viewpoint, else a view-all fit of the scene bounds -----
  const rt::Aabb bounds = extractor.sceneWorldBounds();
  ex::CameraDesc cam = extractor.camera();
  const bool noViewpoint = (ctx.boundViewpoint() == nullptr);
  rt::Mat4 viewRT = cam.viewMatrix;
  if (noViewpoint && !bounds.empty) {
    glsl::vec3 c{(bounds.min.x + bounds.max.x) * 0.5f,
                 (bounds.min.y + bounds.max.y) * 0.5f,
                 (bounds.min.z + bounds.max.z) * 0.5f};
    SFVec3f sz = bounds.size();
    float radius = 0.5f * std::sqrt(sz.x * sz.x + sz.y * sz.y + sz.z * sz.z);
    float fov = cam.fieldOfView;
    float dist = radius / std::sin(glsl::maxf(0.1f, fov) * 0.5f) * 1.25f;
    glsl::vec3 dir = v3norm(glsl::vec3{0.45f, 0.35f, 1.0f});
    glsl::vec3 eye = c + dir * dist;
    viewRT = lookAt(eye, c, glsl::vec3{0, 1, 0});
  }

  const float aspect = static_cast<float>(opt.width) / static_cast<float>(opt.height);
  float zNear = 0.1f, zFar = 10000.0f;
  if (!bounds.empty) {
    SFVec3f sz = bounds.size();
    float diag = std::hypot(sz.x, sz.y, sz.z);
    if (diag > 0.0f) {
      const auto center =
          viewRT.transformPoint({(bounds.min.x + bounds.max.x) * 0.5f,
                                 (bounds.min.y + bounds.max.y) * 0.5f,
                                 (bounds.min.z + bounds.max.z) * 0.5f});
      zFar = std::max(diag * 100.0f,
                      std::hypot(center.x, center.y, center.z) + diag);
      zNear = std::max(std::numeric_limits<float>::min(), diag * 0.001f);
    }
  }
  const rt::Mat4 projRT = perspective(cam.fieldOfView, aspect, zNear, zFar);

  // ---- §34.4.2 GeneratedCubeMapTexture faces ---------------------------------
  // Each cube whose update is not NONE is re-rendered before the frame: six
  // size x size views with a pi/2 field of view from the local origin of the
  // first Shape using it, along that Shape's local axes (ADR-0060), in Figure
  // 34.1 face order. Faces of other generated cubes come from the cache as it
  // was before this frame.
  GeneratedCubeCache frameCache;
  GeneratedCubeCache &cache = opt.generatedCubes ? *opt.generatedCubes : frameCache;
  struct PendingCube {
    const void *node;
    int size;
    rt::Mat4 world;
  };
  std::vector<PendingCube> pending;
  for (ex::RenderItemId id = 0; id < extractor.itemCount(); ++id) {
    const ex::RenderItem &it = extractor.item(id);
    forEachTextureRef(it.material.textures, [&](const ex::TextureRef &r) {
      const auto &g = r.generatedCube;
      if (!g.node || g.update == "NONE") return;
      for (const auto &p : pending)
        if (p.node == g.node) return;
      pending.push_back({g.node, g.size, it.worldTransform});
    });
  }
  if (!pending.empty()) {
    const GeneratedCubeCache previous = cache;
    for (const auto &p : pending) {
      const glsl::vec3 origin(p.world.transformPoint({0, 0, 0}));
      auto axis = [&](float x, float y, float z) {
        return v3norm(glsl::vec3(p.world.transformDirection({x, y, z})));
      };
      // front -Z, back +Z, left -X, right +X, top +Y, bottom -Y: (look, up).
      const std::array<std::array<glsl::vec3, 2>, 6> faces{{
          {axis(0, 0, -1), axis(0, 1, 0)},
          {axis(0, 0, 1), axis(0, 1, 0)},
          {axis(-1, 0, 0), axis(0, 1, 0)},
          {axis(1, 0, 0), axis(0, 1, 0)},
          {axis(0, 1, 0), axis(0, 0, 1)},
          {axis(0, -1, 0), axis(0, 0, -1)},
      }};
      const rt::Mat4 faceProj =
          perspective(1.57079632679f, 1.0f, zNear, zFar);
      ex::TexturePixels pixels;
      pixels.width = pixels.height = static_cast<std::uint32_t>(p.size);
      pixels.layers = 6;
      for (const auto &[look, up] : faces) {
        const Framebuffer face =
            renderView(ctx, extractor, opt, lookAt(origin, origin + look, up),
                       faceProj, p.size, p.size, p.node, previous);
        pixels.rgba.insert(pixels.rgba.end(), face.pixels().begin(),
                           face.pixels().end());
      }
      for (std::size_t i = 3; i < pixels.rgba.size(); i += 4)
        pixels.rgba[i] = 255; // an environment is opaque.
      cache.faces[p.node] =
          std::make_shared<const ex::TexturePixels>(std::move(pixels));
    }
  }

  return renderView(ctx, extractor, opt, viewRT, projRT, opt.width, opt.height,
                    nullptr, cache);
}

} // namespace x3d::cpuraster

#endif // X3D_CPURASTER_SCENE_RENDER_HPP
