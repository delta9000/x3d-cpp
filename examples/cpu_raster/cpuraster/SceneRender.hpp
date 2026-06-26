// SceneRender.hpp — orchestration: turn a snapshotted SceneExtractor into a
// Framebuffer. This is the CPU-rasterizer analogue of the PoC's render loop, and
// it deliberately reuses the PoC's conventions so a CPU frame matches the GL one:
//   * perspective() maps X3D's MIN-dimension fieldOfView to the shorter axis.
//   * a "view-all" fit camera frames the scene's per-path world bounds when no
//     Viewpoint is authored (X3DExecutionContext::viewMatrix() is identity then).
//   * eye-space directional + positional (point/spot) lights + the
//     NavigationInfo headlight fallback.
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
#include "MaterialShader.hpp"
#include "Rasterizer.hpp"
#include "SceneExtractor.hpp"
#include "X3DExecutionContext.hpp"
#include "glsl.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <utility>
#include <vector>

namespace x3d::cpuraster {

namespace rt = x3d::runtime;

namespace render_detail { struct SkyboxTextures; } // defined below.

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

enum class CubeFace { Front, Back, Right, Left, Top, Bottom };
struct FaceSample { CubeFace face; glsl::vec2 uv; };

// Pick the cube face a world-space direction pierces and the in-face UV.
// Faces (from the origin): front=-Z, back=+Z, right=+X, left=-X, top=+Y,
// bottom=-Y. UV orientation matches "image displayed normally in 2D" viewed
// from the origin (§Background). Pinned by skybox_test.
inline FaceSample cubeFaceUv(const glsl::vec3 &dir) {
  const float ax = std::fabs(dir.x), ay = std::fabs(dir.y), az = std::fabs(dir.z);
  auto remap = [](float a, float b) { // [-1,1] -> [0,1]
    return glsl::vec2{a * 0.5f + 0.5f, b * 0.5f + 0.5f};
  };
  if (az >= ax && az >= ay) {
    const float u = dir.x / az, v = dir.y / az;
    return dir.z < 0.0f ? FaceSample{CubeFace::Front, remap(u, v)}    // -Z
                        : FaceSample{CubeFace::Back, remap(-u, v)};   // +Z
  }
  if (ax >= ay) {
    const float u = -dir.z / ax, v = dir.y / ax;
    return dir.x > 0.0f ? FaceSample{CubeFace::Right, remap(-u, v)}   // +X
                        : FaceSample{CubeFace::Left, remap(u, v)};    // -X
  }
  const float u = dir.x / ay, v = -dir.z / ay;
  return dir.y > 0.0f ? FaceSample{CubeFace::Top, remap(u, -v)}       // +Y
                      : FaceSample{CubeFace::Bottom, remap(u, v)};    // -Y
}

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
// transformPoint). If none contribute and the headlight is on, a single
// camera-space directional light down -Z is added as the fallback.
inline std::vector<EyeLight>
buildEyeLights(const std::vector<rt::extract::LightDesc> &lights,
               const rt::Mat4 &view, bool headlightOn) {
  using Type = rt::extract::LightDesc::Type;
  std::vector<EyeLight> out;
  for (const auto &L : lights) {
    if (out.size() >= 8) break;
    EyeLight e;
    e.color = glsl::vec3{L.color.r * L.intensity, L.color.g * L.intensity,
                         L.color.b * L.intensity};
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
  if (out.empty() && headlightOn) {
    EyeLight h;
    h.dirEye = glsl::vec3{0, 0, -1};
    h.color = glsl::vec3{1, 1, 1};
    out.push_back(h);
  }
  return out;
}

// MeshData -> rasterizer Vertex array (defaults fill missing normal/color/uv).
inline std::vector<Vertex> toVertices(const rt::extract::MeshData &m) {
  std::vector<Vertex> v(m.positions.size());
  for (std::size_t i = 0; i < m.positions.size(); ++i) {
    v[i].pos = m.positions[i];
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

} // namespace render_detail

inline Framebuffer renderScene(const rt::X3DExecutionContext &ctx,
                               rt::extract::SceneExtractor &extractor,
                               const RenderOptions &opt) {
  namespace ex = rt::extract;
  using namespace render_detail;

  Framebuffer fb(opt.width, opt.height);

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
    float diag = std::sqrt(sz.x * sz.x + sz.y * sz.y + sz.z * sz.z);
    if (diag > 0.0f) { zFar = diag * 100.0f; zNear = glsl::maxf(0.001f, diag * 0.001f); }
  }
  const rt::Mat4 projRT = perspective(cam.fieldOfView, aspect, zNear, zFar);
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
      for (int y = 0; y < opt.height; ++y) {
        for (int x = 0; x < opt.width; ++x) {
          const float xn = 2.0f * (x + 0.5f) / opt.width - 1.0f;
          const float yn = 2.0f * (y + 0.5f) / opt.height - 1.0f; // fb is bottom-up
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
  const std::vector<EyeLight> eyeLights = buildEyeLights(lights, viewRT, headlightOn);

  Rasterizer raster(fb);

  // ---- Partition opaque vs transparent (PoC B7 rule) ------------------------
  std::vector<ex::RenderItemId> opaque;
  std::vector<std::pair<float, ex::RenderItemId>> blended; // (eyeZ, id)
  for (ex::RenderItemId id = 0; id < extractor.itemCount(); ++id) {
    const ex::RenderItem &it = extractor.item(id);
    if (it.mesh.positions.empty()) continue;
    const ex::MaterialDesc &mat = it.material;
    const bool isBlended =
        (mat.alphaMode == ex::AlphaMode::Blend) || (mat.transparency > 0.0f);
    if (!isBlended) {
      opaque.push_back(id);
    } else {
      glsl::vec3 wc = it.worldTransform.transformPoint(centroid(it.mesh).toSF());
      glsl::vec3 ec = viewRT.transformPoint(wc.toSF());
      blended.emplace_back(ec.z, id);
    }
  }
  std::sort(blended.begin(), blended.end(),
            [](const auto &a, const auto &b) { return a.first < b.first; });

  auto drawOne = [&](ex::RenderItemId id, BlendMode blend) {
    const ex::RenderItem &it = extractor.item(id);
    const ex::MeshData &mesh = it.mesh;
    if (mesh.positions.empty() || mesh.indices.empty()) return;
    const std::vector<Vertex> verts = toVertices(mesh);
    const glsl::mat4 modelG(it.worldTransform);
    const glsl::mat3 normalMat = glsl::normalMatrix(viewRT * it.worldTransform);
    const glsl::vec4 baseColor = glsl::vec4(it.material.toRGBA());

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
                           mesh.ccw, /*solid=*/false, blend, glyphFs);
      return;
    }

    // Topology paths (B4): lines/points are unlit constant-color.
    if (mesh.topology == ex::Topology::Lines) {
      raster.drawLines(verts, mesh.indices, modelG, viewG, projG, baseColor,
                       mesh.hasColors);
      return;
    }
    if (mesh.topology == ex::Topology::Points) {
      raster.drawPoints(verts, mesh.indices, modelG, viewG, projG, baseColor,
                        mesh.hasColors);
      return;
    }

    const bool forceUnlit = !mesh.hasNormals;
    FragmentShader fs;
    if (opt.authorShaderFor) fs = opt.authorShaderFor(it, eyeLights, mesh.hasColors);
    if (!fs)
      fs = makeMaterialShader(it.material, eyeLights, mesh.hasColors, forceUnlit);
    raster.drawTriangles(verts, mesh.indices, modelG, viewG, projG, normalMat,
                         mesh.ccw, mesh.solid, blend, fs);
  };

  for (ex::RenderItemId id : opaque) drawOne(id, BlendMode::Opaque);
  for (const auto &kv : blended) drawOne(kv.second, BlendMode::Blend);

  return fb;
}

} // namespace x3d::cpuraster

#endif // X3D_CPURASTER_SCENE_RENDER_HPP
