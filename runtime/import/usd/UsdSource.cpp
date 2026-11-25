// UsdSource.cpp — reads USDA / USDC / USDZ through tinyusdz into the
// ImportScene IR.
//
// tinyusdz's Tydra converter supplies triangulated, single-indexed meshes,
// UsdPreviewSurface materials, skeletons and SkelAnimations. What Tydra does
// not convert is read here straight from the composed Stage: cameras, lights,
// time-sampled xformOps, PointInstancers, the stage's up axis and units.
#include "UsdSource.hpp"

#include "composition.hh"
#include "io-util.hh"
#include "prim-types.hh"
#include "tinyusdz.hh"
#include "tydra/render-data.hh"
#include "tydra/scene-access.hh"
#include "usdGeom.hh"
#include "usdLux.hh"
#include "usdSkel.hh"
#include "value-types.hh"
#include "xform.hh"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace x3d::runtime::importer {

namespace {

namespace tu = tinyusdz;
namespace ty = tinyusdz::tydra;

constexpr float kPi = 3.14159265358979f;

// USD matrices are row-major with row vectors (translation in m[3][0..2]).
// The column-vector matrix is the transpose, so column-major [c*4+r] = m[c][r].
Mat4 toMat4(const tu::value::matrix4d &t) {
  Mat4 m;
  for (int c = 0; c < 4; ++c)
    for (int r = 0; r < 4; ++r) m.m[static_cast<std::size_t>(c * 4 + r)] = static_cast<float>(t.m[c][r]);
  return m;
}

/// Splits a column-major affine matrix into T, R, S (no shear).
Trs decompose(const Mat4 &mat) {
  const auto &m = mat.m;
  Trs out;
  out.translation = {m[12], m[13], m[14]};
  float sx = std::sqrt(m[0] * m[0] + m[1] * m[1] + m[2] * m[2]);
  const float sy = std::sqrt(m[4] * m[4] + m[5] * m[5] + m[6] * m[6]);
  const float sz = std::sqrt(m[8] * m[8] + m[9] * m[9] + m[10] * m[10]);
  const float det = m[0] * (m[5] * m[10] - m[9] * m[6]) - m[4] * (m[1] * m[10] - m[9] * m[2]) +
                    m[8] * (m[1] * m[6] - m[5] * m[2]);
  if (det < 0) sx = -sx;
  out.scale = {sx, sy, sz};
  auto r = [&](int row, int col, float s) { return s != 0 ? m[static_cast<std::size_t>(col * 4 + row)] / s : 0.0f; };
  const float r00 = r(0, 0, sx), r10 = r(1, 0, sx), r20 = r(2, 0, sx);
  const float r01 = r(0, 1, sy), r11 = r(1, 1, sy), r21 = r(2, 1, sy);
  const float r02 = r(0, 2, sz), r12 = r(1, 2, sz), r22 = r(2, 2, sz);
  const float tr = r00 + r11 + r22;
  Vec4 q;
  if (tr > 0) {
    const float s = std::sqrt(tr + 1.0f) * 2;
    q = {(r21 - r12) / s, (r02 - r20) / s, (r10 - r01) / s, 0.25f * s};
  } else if (r00 > r11 && r00 > r22) {
    const float s = std::sqrt(1.0f + r00 - r11 - r22) * 2;
    q = {0.25f * s, (r01 + r10) / s, (r02 + r20) / s, (r21 - r12) / s};
  } else if (r11 > r22) {
    const float s = std::sqrt(1.0f + r11 - r00 - r22) * 2;
    q = {(r01 + r10) / s, 0.25f * s, (r12 + r21) / s, (r02 - r20) / s};
  } else {
    const float s = std::sqrt(1.0f + r22 - r00 - r11) * 2;
    q = {(r02 + r20) / s, (r12 + r21) / s, 0.25f * s, (r10 - r01) / s};
  }
  const float l = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
  out.rotation = l > 0 ? Vec4{q.x / l, q.y / l, q.z / l, q.w / l} : Vec4{0, 0, 0, 1};
  return out;
}

Mat4 instanceTransform(const Vec3 &t, const float q[4], const Vec3 &s) {
  const float w = q[0], x = q[1], y = q[2], z = q[3];
  const float r00 = 1 - 2 * (y * y + z * z), r01 = 2 * (x * y - w * z), r02 = 2 * (x * z + w * y);
  const float r10 = 2 * (x * y + w * z), r11 = 1 - 2 * (x * x + z * z), r12 = 2 * (y * z - w * x);
  const float r20 = 2 * (x * z - w * y), r21 = 2 * (y * z + w * x), r22 = 1 - 2 * (x * x + y * y);
  Mat4 m;
  m.m = {r00 * s.x, r10 * s.x, r20 * s.x, 0, r01 * s.y, r11 * s.y, r21 * s.y, 0,
         r02 * s.z, r12 * s.z, r22 * s.z, 0, t.x,       t.y,       t.z,       1};
  return m;
}

// tinyusdz's LoadUSDFromFile does not resolve composition arcs, so a composed
// scene (external references and payloads) would load as empty Xforms.
// Compose sublayers, then references, payloads, inherits and variants to a
// fixpoint, and bake the layer into a Stage. USDZ archives are self-contained.
bool loadComposedStage(const std::string &path, tu::Stage &stage, std::string &warn, std::string &err) {
  tu::Layer root;
  if (!tu::LoadLayerFromFile(path, &root, &warn, &err)) return false;
  const std::string base = tu::io::GetBaseDir(path);
  tu::AssetResolutionResolver resolver;
  resolver.set_current_working_path(base);
  resolver.set_search_paths({base});
  tu::Layer src = root;
  {
    tu::Layer out;
    if (tu::CompositeSublayers(resolver, src, &out, &warn, &err)) src = std::move(out);
  }
  for (int i = 0; i < 64; ++i) {
    bool unresolved = false;
    if (src.check_unresolved_references()) {
      unresolved = true;
      tu::Layer out;
      if (!tu::CompositeReferences(resolver, src, &out, &warn, &err)) return false;
      src = std::move(out);
    }
    if (src.check_unresolved_payload()) {
      unresolved = true;
      tu::Layer out;
      if (!tu::CompositePayload(resolver, src, &out, &warn, &err)) return false;
      src = std::move(out);
    }
    if (src.check_unresolved_inherits()) {
      unresolved = true;
      tu::Layer out;
      if (tu::CompositeInherits(src, &out, &warn, &err)) src = std::move(out);
    }
    if (src.check_unresolved_variant()) {
      unresolved = true;
      tu::Layer out;
      if (tu::CompositeVariant(src, &out, &warn, &err)) src = std::move(out);
    }
    if (!unresolved) break;
  }
  return tu::LayerToStage(src, &stage, &warn, &err);
}

struct InstanceSet {
  std::vector<Mat4> transforms;
  std::vector<std::string> prototypePaths;
};

// Tydra has no PointInstancer support; resolve each instancer from the Stage.
void collectInstancers(const tu::Prim &prim, const std::string &parentPath, std::map<std::string, InstanceSet> &out) {
  const std::string absPath = parentPath + "/" + prim.element_name();
  if (const auto *pi = prim.as<tu::PointInstancer>()) {
    std::vector<tu::value::point3f> positions;
    std::vector<tu::value::quath> orientations;
    std::vector<tu::value::float3> scales;
    std::vector<std::int32_t> protoIndices;
    const auto dflt = tu::value::TimeCode::Default();
    if (auto a = pi->positions.get_value()) a.value().get(dflt, &positions);
    if (auto a = pi->orientations.get_value()) a.value().get(dflt, &orientations);
    if (auto a = pi->scales.get_value()) a.value().get(dflt, &scales);
    if (auto a = pi->protoIndices.get_value()) a.value().get(dflt, &protoIndices);
    std::vector<std::string> protoPaths;
    if (pi->prototypes) {
      const tu::Relationship &rel = pi->prototypes.value();
      if (rel.type == tu::Relationship::Type::Path)
        protoPaths.push_back(rel.targetPath.full_path_name());
      else if (rel.type == tu::Relationship::Type::PathVector)
        for (const auto &tp : rel.targetPathVector) protoPaths.push_back(tp.full_path_name());
    }
    InstanceSet set;
    for (std::size_t i = 0; i < positions.size(); ++i) {
      const std::int32_t p = i < protoIndices.size() ? protoIndices[i] : 0;
      if (p < 0 || static_cast<std::size_t>(p) >= protoPaths.size()) continue;
      float q[4] = {1, 0, 0, 0};
      if (i < orientations.size()) {
        q[0] = tu::value::half_to_float(orientations[i].real);
        q[1] = tu::value::half_to_float(orientations[i].imag[0]);
        q[2] = tu::value::half_to_float(orientations[i].imag[1]);
        q[3] = tu::value::half_to_float(orientations[i].imag[2]);
      }
      Vec3 s{1, 1, 1};
      if (i < scales.size()) s = {scales[i][0], scales[i][1], scales[i][2]};
      set.transforms.push_back(instanceTransform({positions[i][0], positions[i][1], positions[i][2]}, q, s));
      set.prototypePaths.push_back(protoPaths[static_cast<std::size_t>(p)]);
    }
    if (!set.transforms.empty()) out[absPath] = std::move(set);
  }
  for (const auto &child : prim.children()) collectInstancers(child, absPath, out);
}

template <class T> T attr(const tu::TypedAttributeWithFallback<tu::Animatable<T>> &a, T fallback) {
  T v = fallback;
  a.get_value().get(tu::value::TimeCode::Default(), &v);
  return v;
}

/// A float input stored only in a prim's generic properties (for example
/// ShapingAPI's "inputs:shaping:cone:angle", which tinyusdz does not model).
template <class Light> std::optional<float> extraFloat(const Light &l, const std::string &name) {
  auto it = l.props.find(name);
  if (it == l.props.end() || !it->second.is_attribute()) return std::nullopt;
  if (auto v = it->second.get_attribute().template get_value<float>()) return v.value();
  return std::nullopt;
}

/// One TextureRef plus what the IR cannot carry directly.
struct PendingTexture {
  TextureRef ref;
  std::string varname;
  bool transformed = false;
  Vec2 scale{1, 1}, translation{0, 0};
  float rotationDeg = 0;
};

class Reader {
public:
  Reader(const tu::Stage &stage, const ty::RenderScene &rs, ImportScene &out) : stage_(stage), rs_(rs), out_(out) {}

  void run() {
    tcps_ = stage_.metas().timeCodesPerSecond.get_value();
    if (tcps_ <= 0) tcps_ = 24.0;
    materials();
    meshes();
    for (const auto &prim : stage_.root_prims()) collectInstancers(prim, "", instancers_);
    for (const auto &n : rs_.nodes) out_.roots.push_back(flatten(n));
    skeletons();
    finishAnimation();
    bakeTextureTransforms();
    stageFrame();
  }

private:
  const tu::Stage &stage_;
  const ty::RenderScene &rs_;
  ImportScene &out_;
  double tcps_ = 24.0;
  std::map<std::string, InstanceSet> instancers_;
  std::vector<std::vector<PendingTexture *>> matTextures_; ///< per IR material
  std::vector<std::vector<PendingTexture>> pending_;       ///< owns the above
  std::map<std::pair<int, bool>, int> materialVariant_;    ///< (usd material, doubleSided)
  std::map<std::tuple<float, float, float, float, bool>, int> displayMaterial_;
  std::vector<std::vector<int>> meshParts_; ///< tydra mesh -> IR meshes
  std::vector<std::vector<std::string>> meshTargetNames_; ///< per IR mesh
  std::map<std::string, std::vector<int>> nodesBySkeleton_; ///< skeleton path -> skinned nodes
  std::map<int, std::vector<int>> skinnedNodeMeshes_;       ///< node -> tydra mesh ids
  std::map<std::string, int> nodeByPath_;
  /// Raw-timecode channels; finishAnimation() converts times to seconds.
  std::vector<AnimationChannel> channels_;

  void warn(const std::string &w) {
    if (std::find(out_.warnings.begin(), out_.warnings.end(), w) == out_.warnings.end())
      out_.warnings.push_back(w);
  }

  // ---- materials ---------------------------------------------------------

  template <class T>
  std::optional<PendingTexture> texture(const ty::ShaderParam<T> &param, const char *slot, const std::string &mat) {
    if (!param.is_texture()) return std::nullopt;
    if (param.texture_id < 0 || static_cast<std::size_t>(param.texture_id) >= rs_.textures.size()) return std::nullopt;
    const ty::UVTexture &uv = rs_.textures[static_cast<std::size_t>(param.texture_id)];
    if (uv.texture_image_id < 0 || static_cast<std::size_t>(uv.texture_image_id) >= rs_.images.size()) return std::nullopt;
    const ty::TextureImage &img = rs_.images[static_cast<std::size_t>(uv.texture_image_id)];
    PendingTexture p;
    // Undecoded image bytes are the original file (a USDZ member); anything
    // else is referenced by its resolved path.
    if (!img.decoded && img.buffer_id >= 0 && static_cast<std::size_t>(img.buffer_id) < rs_.buffers.size() &&
        !rs_.buffers[static_cast<std::size_t>(img.buffer_id)].data.empty()) {
      EmbeddedTexture e;
      e.key = img.asset_identifier;
      e.bytes = rs_.buffers[static_cast<std::size_t>(img.buffer_id)].data;
      if (auto dot = img.asset_identifier.find_last_of('.'); dot != std::string::npos)
        e.hintExt = img.asset_identifier.substr(dot + 1);
      p.ref.embeddedIndex = static_cast<int>(out_.embedded.size());
      out_.embedded.push_back(std::move(e));
    } else {
      p.ref.externalPath = img.asset_identifier;
    }
    auto wrap = [&](ty::UVTexture::WrapMode w) {
      switch (w) {
        case ty::UVTexture::WrapMode::REPEAT: return TextureWrap::Repeat;
        case ty::UVTexture::WrapMode::MIRROR: return TextureWrap::MirroredRepeat;
        case ty::UVTexture::WrapMode::CLAMP_TO_BORDER:
          warn("UsdUVTexture wrap 'black' mapped to clamp-to-edge");
          return TextureWrap::ClampToEdge;
        default: return TextureWrap::ClampToEdge;
      }
    };
    p.ref.wrapS = wrap(uv.wrapS);
    p.ref.wrapT = wrap(uv.wrapT);
    p.varname = uv.varname_uv;
    if (uv.has_transform2d) {
      p.transformed = true;
      p.scale = {uv.tx_scale[0], uv.tx_scale[1]};
      p.translation = {uv.tx_translation[0], uv.tx_translation[1]};
      p.rotationDeg = uv.tx_rotation;
    }
    const bool normal = std::string(slot) == "normal";
    const bool identityScale = uv.scale[0] == 1 && uv.scale[1] == 1 && uv.scale[2] == 1 && uv.bias[0] == 0 &&
                               uv.bias[1] == 0 && uv.bias[2] == 0;
    const bool normalScale = uv.scale[0] == 2 && uv.scale[1] == 2 && uv.scale[2] == 2 && uv.bias[0] == -1 &&
                             uv.bias[1] == -1 && uv.bias[2] == -1;
    if (!(identityScale || (normal && normalScale)))
      warn("material '" + mat + "': UsdUVTexture scale/bias on the " + slot + " texture ignored");
    return p;
  }

  void materials() {
    for (const auto &um : rs_.materials) {
      ImportMaterial m;
      m.name = um.name;
      const ty::PreviewSurfaceShader &s = um.surfaceShader;
      auto v3 = [](const ty::ShaderParam<ty::vec3> &p, bool tex) {
        return tex ? Vec3{1, 1, 1} : Vec3{p.value[0], p.value[1], p.value[2]};
      };
      // A texture-connected input's value is only its fallback; X3D
      // multiplies the texture by the factor, so the factor becomes 1.
      m.diffuse = v3(s.diffuseColor, s.diffuseColor.is_texture());
      m.emissive = v3(s.emissiveColor, s.emissiveColor.is_texture());
      m.specular = v3(s.specularColor, s.specularColor.is_texture());
      m.opacity = s.opacity.is_texture() ? 1.0f : s.opacity.value;
      m.useSpecularWorkflow = s.useSpecularWorkflow;
      m.ior = s.ior.value;
      m.clearcoat = s.clearcoat.value;
      m.clearcoatRoughness = s.clearcoatRoughness.value;
      m.opacityThreshold = s.opacityThreshold.value;
      if (m.opacityThreshold > 0.0f) {
        m.alpha = AlphaMode::Mask;
        m.alphaCutoff = m.opacityThreshold;
      } else if (m.opacity < 1.0f || s.opacity.is_texture()) {
        m.alpha = AlphaMode::Blend;
      }
      PbrParams p;
      p.baseColor = {m.diffuse.x, m.diffuse.y, m.diffuse.z, m.opacity};
      p.metallic = s.metallic.is_texture() ? 1.0f : s.metallic.value;
      p.roughness = s.roughness.is_texture() ? 1.0f : s.roughness.value;
      m.pbr = p;
      if (s.useSpecularWorkflow)
        warn("material '" + m.name + "' uses the specular workflow; X3D PhysicalMaterial is metallic-roughness, "
             "the specular colour reaches only materialGlsl()");
      if (s.displacement.is_texture() || s.displacement.value != 0.0f)
        warn("material '" + m.name + "': displacement dropped");

      pending_.emplace_back();
      auto &pend = pending_.back();
      pend.reserve(8);
      auto add = [&](std::optional<PendingTexture> t, std::optional<TextureRef> &slot) {
        if (!t) return;
        pend.push_back(*t);
        slot = t->ref;
      };
      add(texture(s.diffuseColor, "base colour", m.name), m.textures.baseColor);
      add(texture(s.normal, "normal", m.name), m.textures.normal);
      add(texture(s.emissiveColor, "emissive", m.name), m.textures.emissive);
      add(texture(s.occlusion, "occlusion", m.name), m.textures.occlusion);
      add(texture(s.specularColor, "specular", m.name), m.textures.specular);
      // X3D packs roughness (G) and metallic (B) into one image.
      if (s.roughness.is_texture() || s.metallic.is_texture()) {
        const auto &r = s.roughness.is_texture() ? s.roughness : s.metallic;
        add(texture(r, "metallic-roughness", m.name), m.textures.metallicRoughness);
        auto chan = [&](const ty::ShaderParam<float> &q) {
          return rs_.textures[static_cast<std::size_t>(q.texture_id)].connectedOutputChannel;
        };
        if (s.roughness.is_texture() && s.metallic.is_texture() &&
            rs_.textures[static_cast<std::size_t>(s.roughness.texture_id)].texture_image_id !=
                rs_.textures[static_cast<std::size_t>(s.metallic.texture_id)].texture_image_id)
          warn("material '" + m.name + "' has separate roughness and metallic images; the roughness image is used "
               "as the packed one");
        if ((s.roughness.is_texture() && chan(s.roughness) != ty::UVTexture::Channel::G) ||
            (s.metallic.is_texture() && chan(s.metallic) != ty::UVTexture::Channel::B))
          warn("material '" + m.name + "' reads roughness/metallic from channels other than G/B; X3D reads G/B");
      }
      if (s.opacity.is_texture()) {
        const bool fromBase = s.diffuseColor.is_texture() &&
                              rs_.textures[static_cast<std::size_t>(s.opacity.texture_id)].texture_image_id ==
                                  rs_.textures[static_cast<std::size_t>(s.diffuseColor.texture_id)].texture_image_id;
        if (!fromBase)
          warn("material '" + m.name + "' reads opacity from its own image; X3D reads alpha from the base texture");
      }
      out_.materials.push_back(std::move(m));
      matTextures_.emplace_back();
      for (auto &t : pend) matTextures_.back().push_back(&t);
    }
  }

  /// The IR material for a USD material on a mesh with `doubleSided`.
  int materialFor(int usdMaterial, bool doubleSided) {
    if (usdMaterial < 0) return -1;
    if (!doubleSided) return usdMaterial;
    auto key = std::make_pair(usdMaterial, true);
    if (auto it = materialVariant_.find(key); it != materialVariant_.end()) return it->second;
    ImportMaterial copy = out_.materials[static_cast<std::size_t>(usdMaterial)];
    copy.doubleSided = true;
    out_.materials.push_back(std::move(copy));
    std::vector<PendingTexture *> texs = matTextures_[static_cast<std::size_t>(usdMaterial)];
    matTextures_.push_back(std::move(texs));
    const int idx = static_cast<int>(out_.materials.size()) - 1;
    materialVariant_[key] = idx;
    return idx;
  }

  /// A material from displayColor / displayOpacity for unbound meshes.
  int displayMaterial(Vec3 c, float opacity, bool doubleSided) {
    auto key = std::make_tuple(c.x, c.y, c.z, opacity, doubleSided);
    if (auto it = displayMaterial_.find(key); it != displayMaterial_.end()) return it->second;
    ImportMaterial m;
    m.name = "displayColor";
    m.diffuse = c;
    m.opacity = opacity;
    m.pbr = PbrParams{{c.x, c.y, c.z, opacity}, 0.0f, 0.5f};
    if (opacity < 1.0f) m.alpha = AlphaMode::Blend;
    m.doubleSided = doubleSided;
    out_.materials.push_back(std::move(m));
    matTextures_.emplace_back();
    const int idx = static_cast<int>(out_.materials.size()) - 1;
    displayMaterial_[key] = idx;
    return idx;
  }

  // ---- meshes ------------------------------------------------------------

  static std::vector<Vec3> vec3s(const ty::VertexAttribute &a) {
    std::vector<Vec3> out;
    if (a.format != ty::VertexAttributeFormat::Vec3) return out;
    const auto *f = static_cast<const float *>(a.buffer());
    for (std::size_t i = 0; i < a.vertex_count(); ++i) out.push_back({f[3 * i], f[3 * i + 1], f[3 * i + 2]});
    return out;
  }

  void meshes() {
    for (const auto &um : rs_.meshes) {
      ImportMesh base;
      base.name = um.prim_name;
      const std::size_t n = um.points.size();
      for (const auto &p : um.points) base.positions.push_back({p[0], p[1], p[2]});
      if (!um.normals.empty()) {
        base.normals = vec3s(um.normals);
        if (base.normals.size() != n) base.normals.clear();
      }
      std::uint32_t maxSlot = 0;
      for (const auto &kv : um.texcoords) maxSlot = std::max(maxSlot, kv.first + 1);
      base.uvSets.resize(maxSlot);
      for (const auto &kv : um.texcoords) {
        const ty::VertexAttribute &a = kv.second;
        if (a.format != ty::VertexAttributeFormat::Vec2 || a.vertex_count() != n) continue;
        const auto *f = static_cast<const float *>(a.buffer());
        auto &set = base.uvSets[kv.first];
        for (std::size_t i = 0; i < n; ++i) set.push_back({f[2 * i], f[2 * i + 1]});
      }
      // displayColor: per vertex becomes Color, constant goes to the material.
      Vec3 display{um.displayColor[0], um.displayColor[1], um.displayColor[2]};
      float displayOpacity = um.displayOpacity;
      if (um.vertex_colors.vertex_count() == n && n > 0) {
        const auto c = vec3s(um.vertex_colors);
        std::vector<float> alpha;
        if (um.vertex_opacities.vertex_count() == n && um.vertex_opacities.format == ty::VertexAttributeFormat::Float) {
          const auto *f = static_cast<const float *>(um.vertex_opacities.buffer());
          alpha.assign(f, f + n);
        }
        for (std::size_t i = 0; i < c.size(); ++i) {
          const float a = alpha.empty() ? 1.0f : alpha[i];
          base.colors.push_back({c[i].x, c[i].y, c[i].z, a});
          if (a < 1.0f) base.colorsHaveAlpha = true;
        }
        display = {1, 1, 1};
      } else if (um.vertex_colors.vertex_count() == 1) {
        const auto c = vec3s(um.vertex_colors);
        if (!c.empty()) display = c[0];
      }
      if (um.vertex_opacities.vertex_count() == 1 && um.vertex_opacities.format == ty::VertexAttributeFormat::Float)
        displayOpacity = *static_cast<const float *>(um.vertex_opacities.buffer());

      // Blend shapes: sparse offsets become dense deltas.
      std::vector<std::string> targetNames;
      for (const auto &kv : um.targets) {
        MorphTarget t;
        t.positions.assign(n, Vec3{});
        const bool hasNormals = !kv.second.normalOffsets.empty() && !base.normals.empty();
        if (hasNormals) t.normals.assign(n, Vec3{});
        for (std::size_t i = 0; i < kv.second.pointIndices.size(); ++i) {
          const std::uint32_t pi = kv.second.pointIndices[i];
          if (pi >= n) continue;
          if (i < kv.second.pointOffsets.size()) {
            const auto &o = kv.second.pointOffsets[i];
            t.positions[pi] = {o[0], o[1], o[2]};
          }
          if (hasNormals && i < kv.second.normalOffsets.size()) {
            const auto &o = kv.second.normalOffsets[i];
            t.normals[pi] = {o[0], o[1], o[2]};
          }
        }
        if (!kv.second.inbetweens.empty())
          warn("blend shape '" + kv.first + "': in-between shapes dropped");
        base.morphTargets.push_back(std::move(t));
        base.morphWeights.push_back(0.0f);
        targetNames.push_back(kv.first);
      }

      // Faces, split by materialBind GeomSubsets.
      const std::vector<std::uint32_t> &idx = um.faceVertexIndices();
      const std::size_t faces = idx.size() / 3;
      if (!um.is_triangulated() && !um.faceVertexCounts().empty() &&
          std::any_of(um.faceVertexCounts().begin(), um.faceVertexCounts().end(), [](std::uint32_t c) { return c != 3; }))
        warn("mesh '" + base.name + "' was not triangulated; non-triangle faces dropped");
      std::vector<int> owner(faces, -1);
      std::vector<int> partMaterial;
      for (const auto &kv : um.material_subsetMap) {
        const int part = static_cast<int>(partMaterial.size());
        partMaterial.push_back(kv.second.material_id);
        for (int f : kv.second.indices())
          if (f >= 0 && static_cast<std::size_t>(f) < faces) owner[static_cast<std::size_t>(f)] = part;
      }
      const int rest = static_cast<int>(partMaterial.size());
      partMaterial.push_back(um.material_id);

      std::vector<int> parts;
      for (int part = 0; part <= rest; ++part) {
        std::vector<std::size_t> faceList;
        for (std::size_t f = 0; f < faces; ++f)
          if ((owner[f] < 0 ? rest : owner[f]) == part) faceList.push_back(f);
        if (faceList.empty()) continue;
        ImportMesh m;
        if (partMaterial.size() == 1) {
          m = base;
          m.indices.assign(idx.begin(), idx.begin() + static_cast<std::ptrdiff_t>(faces * 3));
        } else {
          m = compact(base, idx, faceList);
        }
        const int usdMat = partMaterial[static_cast<std::size_t>(part)];
        m.materialIndex = usdMat >= 0 ? materialFor(usdMat, um.doubleSided)
                                      : displayMaterial(display, displayOpacity, um.doubleSided);
        out_.meshes.push_back(std::move(m));
        meshTargetNames_.push_back(targetNames);
        parts.push_back(static_cast<int>(out_.meshes.size()) - 1);
      }
      meshParts_.push_back(std::move(parts));
    }
  }

  /// The faces `faceList` of `base` with only the vertices they use.
  static ImportMesh compact(const ImportMesh &base, const std::vector<std::uint32_t> &idx,
                            const std::vector<std::size_t> &faceList) {
    ImportMesh m;
    m.name = base.name;
    m.colorsHaveAlpha = base.colorsHaveAlpha;
    m.morphWeights = base.morphWeights;
    m.uvSets.resize(base.uvSets.size());
    m.morphTargets.resize(base.morphTargets.size());
    std::unordered_map<std::uint32_t, std::uint32_t> remap;
    std::vector<std::uint32_t> order;
    for (std::size_t f : faceList)
      for (int k = 0; k < 3; ++k) {
        const std::uint32_t v = idx[f * 3 + static_cast<std::size_t>(k)];
        auto [it, inserted] = remap.emplace(v, static_cast<std::uint32_t>(order.size()));
        if (inserted) order.push_back(v);
        m.indices.push_back(it->second);
      }
    for (std::uint32_t v : order) {
      m.positions.push_back(base.positions[v]);
      if (!base.normals.empty()) m.normals.push_back(base.normals[v]);
      if (!base.colors.empty()) m.colors.push_back(base.colors[v]);
      for (std::size_t s = 0; s < base.uvSets.size(); ++s)
        if (!base.uvSets[s].empty()) m.uvSets[s].push_back(base.uvSets[s][v]);
      for (std::size_t t = 0; t < base.morphTargets.size(); ++t) {
        m.morphTargets[t].positions.push_back(base.morphTargets[t].positions[v]);
        if (!base.morphTargets[t].normals.empty()) m.morphTargets[t].normals.push_back(base.morphTargets[t].normals[v]);
      }
    }
    return m;
  }

  // ---- nodes -------------------------------------------------------------

  const tu::Prim *prim(const std::string &path) const {
    const tu::Prim *p = nullptr;
    std::string err;
    if (path.empty() || !stage_.find_prim_at_path(tu::Path(path, ""), p, &err)) return nullptr;
    return p;
  }

  int flatten(const ty::Node &un) {
    ImportNode n;
    n.name = un.prim_name;
    n.localTransform = toMat4(un.local_matrix);
    if (un.has_resetXform) warn("node '" + un.prim_name + "' resets the parent transform; kept under its parent");
    const int idx = static_cast<int>(out_.nodes.size());
    out_.nodes.push_back(n);
    nodeByPath_[un.abs_path] = idx;

    if (un.nodeType == ty::NodeType::Mesh && un.id >= 0 && static_cast<std::size_t>(un.id) < meshParts_.size()) {
      out_.nodes[static_cast<std::size_t>(idx)].meshIndices = meshParts_[static_cast<std::size_t>(un.id)];
      const ty::RenderMesh &rm = rs_.meshes[static_cast<std::size_t>(un.id)];
      if (rm.skel_id >= 0 && !rm.joint_and_weights.jointIndices.empty()) {
        nodesBySkeleton_[rs_.skeletons[static_cast<std::size_t>(rm.skel_id)].abs_path].push_back(idx);
        skinnedNodeMeshes_[idx].push_back(un.id);
      }
    }
    if (const tu::Prim *p = prim(un.abs_path)) {
      camera(*p, idx);
      light(*p, idx);
      xformAnimation(*p, idx);
    }

    const auto instIt = instancers_.find(un.abs_path);
    const InstanceSet *inst = instIt != instancers_.end() ? &instIt->second : nullptr;
    std::vector<int> children;
    for (const auto &child : un.children) {
      const bool isPrototype =
          inst && std::find(inst->prototypePaths.begin(), inst->prototypePaths.end(), child.abs_path) !=
                      inst->prototypePaths.end();
      if (isPrototype) {
        for (std::size_t i = 0; i < inst->transforms.size(); ++i) {
          if (inst->prototypePaths[i] != child.abs_path) continue;
          ImportNode in;
          in.name = child.prim_name + "_" + std::to_string(i);
          in.localTransform = inst->transforms[i];
          const int instIdx = static_cast<int>(out_.nodes.size());
          out_.nodes.push_back(in);
          const int protoIdx = flatten(child);
          out_.nodes[static_cast<std::size_t>(instIdx)].childIndices = {protoIdx};
          children.push_back(instIdx);
        }
        continue; // a prototype is drawn only through its instances
      }
      children.push_back(flatten(child));
    }
    out_.nodes[static_cast<std::size_t>(idx)].childIndices = children;
    return idx;
  }

  void camera(const tu::Prim &p, int node) {
    const auto *c = p.as<tu::GeomCamera>();
    if (!c) return;
    ImportCamera cam;
    cam.name = p.element_name();
    const float focal = attr(c->focalLength, 50.0f);
    const float hAp = attr(c->horizontalAperture, 20.965f);
    const float vAp = attr(c->verticalAperture, 15.2908f);
    const tu::value::float2 clip = attr(c->clippingRange, tu::value::float2{1.0f, 1000000.0f});
    tu::GeomCamera::Projection proj = tu::GeomCamera::Projection::Perspective;
    c->projection.get_value().get(tu::value::TimeCode::Default(), &proj);
    cam.aspectRatio = vAp > 0 ? hAp / vAp : 0;
    if (proj == tu::GeomCamera::Projection::Orthographic) {
      // Orthographic apertures are in tenths of a scene unit; xmag is half.
      cam.kind = ImportCamera::Kind::Orthographic;
      cam.xmag = hAp * 0.1f * 0.5f;
      cam.ymag = vAp * 0.1f * 0.5f;
    } else {
      cam.yfov = focal > 0 ? 2.0f * std::atan(0.5f * vAp / focal) : 0.785398f;
    }
    cam.znear = clip[0];
    cam.zfar = clip[1];
    out_.cameras.push_back(cam);
    out_.nodes[static_cast<std::size_t>(node)].camera = static_cast<int>(out_.cameras.size()) - 1;
  }

  template <class L> void lightCommon(const L &l, ImportLight &out) {
    const auto c = attr(l.color, tu::value::color3f{1, 1, 1});
    out.color = {c[0], c[1], c[2]};
    out.intensity = attr(l.intensity, 1.0f) * std::pow(2.0f, attr(l.exposure, 0.0f));
    if (attr(l.enableColorTemperature, false))
      warn("light '" + out.name + "': colour temperature ignored, the colour input is used");
  }

  void light(const tu::Prim &p, int node) {
    ImportLight out;
    out.name = p.element_name();
    if (const auto *d = p.as<tu::DistantLight>()) {
      out.kind = ImportLight::Kind::Directional;
      lightCommon(*d, out);
    } else if (const auto *s = p.as<tu::SphereLight>()) {
      lightCommon(*s, out);
      out.kind = ImportLight::Kind::Point;
      // ShapingAPI cone: angle is the half angle in degrees.
      if (auto angle = extraFloat(*s, "inputs:shaping:cone:angle"); angle && *angle < 180.0f) {
        out.kind = ImportLight::Kind::Spot;
        out.outerConeAngle = *angle * kPi / 180.0f;
        const float soft = std::clamp(extraFloat(*s, "inputs:shaping:cone:softness").value_or(0.0f), 0.0f, 1.0f);
        out.innerConeAngle = out.outerConeAngle * (1.0f - soft);
      }
    } else if (const auto *r = p.as<tu::RectLight>()) {
      lightCommon(*r, out);
      out.kind = ImportLight::Kind::Spot;
      out.outerConeAngle = kPi / 2;
      out.innerConeAngle = 0;
      warn("rect light '" + out.name + "' approximated as a hemispherical spot light");
    } else if (const auto *k = p.as<tu::DiskLight>()) {
      lightCommon(*k, out);
      out.kind = ImportLight::Kind::Spot;
      out.outerConeAngle = kPi / 2;
      out.innerConeAngle = 0;
      warn("disk light '" + out.name + "' approximated as a hemispherical spot light");
    } else if (const auto *y = p.as<tu::CylinderLight>()) {
      lightCommon(*y, out);
      out.kind = ImportLight::Kind::Point;
      warn("cylinder light '" + out.name + "' approximated as a point light");
    } else if (p.as<tu::DomeLight>()) {
      warn("dome light '" + out.name + "' dropped; X3D 4.0 has no environment light");
      return;
    } else {
      return;
    }
    out_.lights.push_back(out);
    out_.nodes[static_cast<std::size_t>(node)].light = static_cast<int>(out_.lights.size()) - 1;
  }

  /// Time-sampled xformOps become TRS channels sampled at their key times.
  void xformAnimation(const tu::Prim &p, int node) {
    const tu::Xformable *x = nullptr;
    if (!tu::CastToXformable(p, &x) || !x || !x->has_timesamples()) return;
    std::set<double> times;
    for (const auto &op : x->xformOps)
      if (auto ts = op.get_timesamples())
        for (std::size_t i = 0; i < ts->size(); ++i)
          if (auto t = ts->get_time(i)) times.insert(*t);
    if (times.empty()) return;
    AnimationChannel t, r, s;
    t.path = AnimationChannel::Path::Translation;
    r.path = AnimationChannel::Path::Rotation;
    s.path = AnimationChannel::Path::Scale;
    t.target = r.target = s.target = node;
    for (double tc : times) {
      auto m = x->GetLocalMatrix(tc);
      if (!m) continue;
      const Trs trs = decompose(toMat4(m.value()));
      for (auto *ch : {&t, &r, &s}) ch->times.push_back(static_cast<float>(tc));
      t.values.insert(t.values.end(), {trs.translation.x, trs.translation.y, trs.translation.z});
      r.values.insert(r.values.end(), {trs.rotation.x, trs.rotation.y, trs.rotation.z, trs.rotation.w});
      s.values.insert(s.values.end(), {trs.scale.x, trs.scale.y, trs.scale.z});
    }
    if (t.times.empty()) return;
    ImportNode &n = out_.nodes[static_cast<std::size_t>(node)];
    if (!n.trs) n.trs = decompose(n.localTransform);
    channels_.push_back(std::move(t));
    channels_.push_back(std::move(r));
    channels_.push_back(std::move(s));
  }

  // ---- skeletons ---------------------------------------------------------

  int addJoint(const ty::SkelNode &j, int parent, std::vector<int> &byId, std::map<std::string, int> &byName) {
    ImportNode n;
    n.name = j.joint_name.empty() ? j.joint_path : j.joint_name;
    if (auto slash = n.name.find_last_of('/'); slash != std::string::npos) n.name = n.name.substr(slash + 1);
    n.localTransform = toMat4(j.rest_transform);
    n.trs = decompose(n.localTransform);
    const int idx = static_cast<int>(out_.nodes.size());
    out_.nodes.push_back(n);
    if (parent >= 0) out_.nodes[static_cast<std::size_t>(parent)].childIndices.push_back(idx);
    if (j.joint_id >= 0) {
      if (static_cast<std::size_t>(j.joint_id) >= byId.size()) byId.resize(static_cast<std::size_t>(j.joint_id) + 1, -1);
      byId[static_cast<std::size_t>(j.joint_id)] = idx;
    }
    byName[j.joint_path] = idx;
    byName[j.joint_name] = idx;
    for (const auto &c : j.children) addJoint(c, idx, byId, byName);
    return idx;
  }

  void skeletons() {
    for (const ty::SkelHierarchy &sk : rs_.skeletons) {
      // Joints hang under the Skeleton prim's node (the SkelRoot otherwise).
      int parent = -1;
      if (auto it = nodeByPath_.find(sk.abs_path); it != nodeByPath_.end()) parent = it->second;
      std::vector<int> byId;
      std::map<std::string, int> byName;
      const int root = addJoint(sk.root_node, parent, byId, byName);
      if (parent < 0) out_.roots.push_back(root);

      // One skin per (skinned node, mesh) geomBindTransform.
      for (int node : nodesBySkeleton_[sk.abs_path]) {
        for (int meshId : skinnedNodeMeshes_[node]) {
          const ty::RenderMesh &rm = rs_.meshes[static_cast<std::size_t>(meshId)];
          ImportSkin skin;
          skin.name = out_.nodes[static_cast<std::size_t>(node)].name;
          skin.skeleton = root;
          std::vector<int> jointOf(byId.size(), -1);
          std::vector<const ty::SkelNode *> flat(byId.size(), nullptr);
          collect(sk.root_node, flat);
          for (std::size_t id = 0; id < byId.size(); ++id) {
            if (byId[id] < 0 || !flat[id]) continue;
            jointOf[id] = static_cast<int>(skin.joints.size());
            skin.joints.push_back(byId[id]);
            // Row-vector USD: mesh -> skel bind space -> joint space.
            skin.inverseBind.push_back(toMat4(rm.joint_and_weights.geomBindTransform *
                                              tu::inverse(flat[id]->bind_transform)));
          }
          out_.skins.push_back(std::move(skin));
          const int si = static_cast<int>(out_.skins.size()) - 1;
          if (out_.nodes[static_cast<std::size_t>(node)].skin >= 0)
            warn("node '" + out_.nodes[static_cast<std::size_t>(node)].name +
                 "' binds more than one skinned mesh; the first binding is used");
          else
            out_.nodes[static_cast<std::size_t>(node)].skin = si;
          const auto &ji = rm.joint_and_weights.jointIndices;
          const auto &jw = rm.joint_and_weights.jointWeights;
          const std::size_t es = static_cast<std::size_t>(std::max(1, rm.joint_and_weights.elementSize));
          for (int part : meshParts_[static_cast<std::size_t>(meshId)]) {
            ImportMesh &m = out_.meshes[static_cast<std::size_t>(part)];
            if (m.positions.size() != rm.points.size()) {
              warn("skinned mesh '" + m.name + "' split by material subsets; its weights are dropped");
              continue;
            }
            m.influences.assign(m.positions.size(), {});
            for (std::size_t v = 0; v < m.positions.size(); ++v)
              for (std::size_t k = 0; k < es; ++k) {
                const std::size_t at = v * es + k;
                if (at >= ji.size() || at >= jw.size() || jw[at] <= 0) continue;
                const int id = ji[at];
                if (id < 0 || static_cast<std::size_t>(id) >= jointOf.size() || jointOf[static_cast<std::size_t>(id)] < 0)
                  continue;
                m.influences[v].emplace_back(jointOf[static_cast<std::size_t>(id)], jw[at]);
              }
          }
        }
      }
      if (sk.anim_id >= 0 && static_cast<std::size_t>(sk.anim_id) < rs_.animations.size())
        skelAnimation(rs_.animations[static_cast<std::size_t>(sk.anim_id)], byName, nodesBySkeleton_[sk.abs_path]);
    }
  }

  static void collect(const ty::SkelNode &j, std::vector<const ty::SkelNode *> &flat) {
    if (j.joint_id >= 0 && static_cast<std::size_t>(j.joint_id) < flat.size()) flat[static_cast<std::size_t>(j.joint_id)] = &j;
    for (const auto &c : j.children) collect(c, flat);
  }

  template <class T, class Push>
  static void sampled(const ty::AnimationSampler<T> &s, AnimationChannel &ch, Push push) {
    ch.interpolation = s.interpolation == ty::AnimationSampler<T>::Interpolation::Step
                           ? AnimationChannel::Interpolation::Step
                           : AnimationChannel::Interpolation::Linear;
    for (const auto &k : s.samples) {
      ch.times.push_back(k.t);
      push(k.value);
    }
  }

  void skelAnimation(const ty::Animation &anim, const std::map<std::string, int> &joints,
                     const std::vector<int> &skinned) {
    using CT = ty::AnimationChannel::ChannelType;
    for (const auto &[jointName, byType] : anim.channels_map) {
      auto jt = joints.find(jointName);
      if (jt == joints.end()) {
        warn("SkelAnimation '" + anim.prim_name + "' drives unknown joint '" + jointName + "'");
        continue;
      }
      const int node = jt->second;
      ImportNode &n = out_.nodes[static_cast<std::size_t>(node)];
      for (const auto &[type, c] : byType) {
        AnimationChannel ch;
        ch.target = node;
        if (type == CT::Translation) {
          if (c.translations.static_value) {
            const auto &v = *c.translations.static_value;
            n.trs->translation = {v[0], v[1], v[2]};
          }
          ch.path = AnimationChannel::Path::Translation;
          sampled(c.translations, ch, [&](const ty::vec3 &v) { ch.values.insert(ch.values.end(), {v[0], v[1], v[2]}); });
        } else if (type == CT::Rotation) {
          if (c.rotations.static_value) {
            const auto &v = *c.rotations.static_value;
            n.trs->rotation = {v[0], v[1], v[2], v[3]};
          }
          ch.path = AnimationChannel::Path::Rotation;
          sampled(c.rotations, ch,
                  [&](const ty::quat &v) { ch.values.insert(ch.values.end(), {v[0], v[1], v[2], v[3]}); });
        } else if (type == CT::Scale) {
          if (c.scales.static_value) {
            const auto &v = *c.scales.static_value;
            n.trs->scale = {v[0], v[1], v[2]};
          }
          ch.path = AnimationChannel::Path::Scale;
          sampled(c.scales, ch, [&](const ty::vec3 &v) { ch.values.insert(ch.values.end(), {v[0], v[1], v[2]}); });
        } else {
          continue;
        }
        if (!ch.times.empty()) channels_.push_back(std::move(ch));
      }
    }
    if (anim.blendshape_weights_map.empty()) return;
    // Blend-shape weights drive every mesh bound to this skeleton.
    for (int node : skinned) {
      const auto &meshes = out_.nodes[static_cast<std::size_t>(node)].meshIndices;
      if (meshes.empty()) continue;
      const auto &names = meshTargetNames_[static_cast<std::size_t>(meshes[0])];
      if (names.empty()) continue;
      std::set<float> times;
      std::vector<float> defaults(names.size(), 0.0f);
      for (std::size_t t = 0; t < names.size(); ++t) {
        auto it = anim.blendshape_weights_map.find(names[t]);
        if (it == anim.blendshape_weights_map.end()) continue;
        if (it->second.static_value) defaults[t] = *it->second.static_value;
        for (const auto &k : it->second.samples) times.insert(k.t);
      }
      out_.nodes[static_cast<std::size_t>(node)].morphWeights = defaults;
      if (times.empty()) continue;
      AnimationChannel ch;
      ch.path = AnimationChannel::Path::Weights;
      ch.target = node;
      for (float tc : times) {
        ch.times.push_back(tc);
        for (std::size_t t = 0; t < names.size(); ++t) {
          auto it = anim.blendshape_weights_map.find(names[t]);
          ch.values.push_back(it == anim.blendshape_weights_map.end() ? 0.0f : sampleAt(it->second, tc, defaults[t]));
        }
      }
      channels_.push_back(std::move(ch));
    }
  }

  static float sampleAt(const ty::AnimationSampler<float> &s, float t, float fallback) {
    if (s.samples.empty()) return fallback;
    if (t <= s.samples.front().t) return s.samples.front().value;
    for (std::size_t i = 1; i < s.samples.size(); ++i)
      if (t <= s.samples[i].t) {
        const auto &a = s.samples[i - 1], &b = s.samples[i];
        if (s.interpolation == ty::AnimationSampler<float>::Interpolation::Step) return a.value;
        const float u = b.t > a.t ? (t - a.t) / (b.t - a.t) : 0.0f;
        return a.value + (b.value - a.value) * u;
      }
    return s.samples.back().value;
  }

  /// USD has one timeline: every channel goes into one animation, timed from
  /// the stage's startTimeCode in seconds.
  void finishAnimation() {
    if (channels_.empty()) return;
    double start = 0;
    if (stage_.metas().startTimeCode.authored()) start = stage_.metas().startTimeCode.get_value();
    for (const auto &c : channels_)
      if (!c.times.empty()) start = std::min(start, static_cast<double>(c.times.front()));
    ImportAnimation a;
    a.name = "timeline";
    for (auto &c : channels_) {
      for (float &t : c.times) t = static_cast<float>((t - start) / tcps_);
      a.channels.push_back(std::move(c));
    }
    out_.animations.push_back(std::move(a));
  }

  // ---- texture coordinates -----------------------------------------------

  /// Resolves each texture's primvar to a uv set and bakes UsdTransform2d
  /// into a derived set (X3D has no per-texture transform on a material).
  void bakeTextureTransforms() {
    for (std::size_t mi = 0; mi < out_.materials.size(); ++mi) {
      auto &texs = matTextures_[mi];
      if (texs.empty()) continue;
      std::vector<ImportMesh *> users;
      for (auto &m : out_.meshes)
        if (m.materialIndex == static_cast<int>(mi)) users.push_back(&m);
      std::size_t next = 0;
      for (auto *m : users) next = std::max(next, m->uvSets.size());
      std::map<std::tuple<int, float, float, float, float, float>, int> derived;
      auto slotOf = [&](const PendingTexture &t) { return t.varname == "st1" ? 1 : 0; };
      ImportMaterial &mat = out_.materials[mi];
      TextureSlots &slots = mat.textures;
      for (std::optional<TextureRef> *slot :
           {&slots.baseColor, &slots.normal, &slots.emissive, &slots.occlusion, &slots.specular,
            &slots.metallicRoughness}) {
        if (!*slot) continue;
        const PendingTexture *pt = nullptr;
        for (const auto *t : texs)
          if (t->ref.externalPath == (*slot)->externalPath && t->ref.embeddedIndex == (*slot)->embeddedIndex) pt = t;
        if (!pt) continue;
        const int src = slotOf(*pt);
        (*slot)->uvSet = src;
        if (!pt->transformed) continue;
        auto key = std::make_tuple(src, pt->scale.x, pt->scale.y, pt->translation.x, pt->translation.y, pt->rotationDeg);
        auto it = derived.find(key);
        if (it == derived.end()) {
          const int set = static_cast<int>(next++);
          it = derived.emplace(key, set).first;
          const float a = pt->rotationDeg * kPi / 180.0f, c = std::cos(a), s = std::sin(a);
          for (auto *m : users) {
            if (m->uvSets.size() <= static_cast<std::size_t>(set)) m->uvSets.resize(static_cast<std::size_t>(set) + 1);
            if (m->uvSets[static_cast<std::size_t>(src)].empty()) continue;
            const auto &in = m->uvSets[static_cast<std::size_t>(src)];
            std::vector<Vec2> outSet;
            outSet.reserve(in.size());
            for (const Vec2 &uv : in) {
              // UsdTransform2d: scale, then rotate counter-clockwise, then translate.
              const float x = uv.x * pt->scale.x, y = uv.y * pt->scale.y;
              outSet.push_back({c * x - s * y + pt->translation.x, s * x + c * y + pt->translation.y});
            }
            m->uvSets[static_cast<std::size_t>(set)] = std::move(outSet);
          }
        }
        (*slot)->uvSet = it->second;
      }
    }
  }

  // ---- stage frame -------------------------------------------------------

  /// X3D is Y-up in metres: wrap the roots when the stage says otherwise.
  void stageFrame() {
    const tu::Axis up = stage_.metas().upAxis.get_value();
    const double mpu = stage_.metas().metersPerUnit.get_value();
    if (up == tu::Axis::X) warn("stage upAxis X is not converted");
    const bool zUp = up == tu::Axis::Z;
    const bool scaled = mpu > 0 && std::fabs(mpu - 1.0) > 1e-9;
    if (!zUp && !scaled) return;
    const float k = scaled ? static_cast<float>(mpu) : 1.0f;
    ImportNode root;
    root.name = "stage";
    Trs trs;
    trs.scale = {k, k, k};
    if (zUp) trs.rotation = {-0.70710678f, 0, 0, 0.70710678f}; // -90 degrees about X
    root.trs = trs;
    // Column-major T*R*S for the same rotation.
    if (zUp)
      root.localTransform.m = {k, 0, 0, 0, 0, 0, -k, 0, 0, k, 0, 0, 0, 0, 0, 1};
    else
      root.localTransform.m = {k, 0, 0, 0, 0, k, 0, 0, 0, 0, k, 0, 0, 0, 0, 1};
    root.childIndices = out_.roots;
    out_.nodes.push_back(root);
    out_.roots = {static_cast<int>(out_.nodes.size()) - 1};
  }
};

} // namespace

ImportScene UsdSource::load(const std::string &path) {
  std::string warn, err;
  tu::Stage stage;
  const bool usdz = tu::IsUSDZ(path);
  const bool ok = usdz ? tu::LoadUSDFromFile(path, &stage, &warn, &err) : loadComposedStage(path, stage, warn, err);
  if (!ok) throw std::runtime_error("could not read USD file '" + path + "': " + err);

  ty::RenderScene rs;
  ty::RenderSceneConverter converter;
  ty::RenderSceneConverterEnv env(stage);
  env.mesh_config.triangulate = true;
  env.mesh_config.build_vertex_indices = true;
  env.mesh_config.compute_tangents_and_binormals = false;
  // Keep textures as their original compressed bytes. With loading on,
  // tinyusdz decodes every image into raw texels.
  env.scene_config.load_texture_assets = false;
  tu::USDZAsset usdzAsset;
  if (usdz) {
    if (!tu::ReadUSDZAssetInfoFromFile(path, &usdzAsset, &warn, &err))
      throw std::runtime_error("could not read USDZ asset table of '" + path + "': " + err);
    tu::AssetResolutionResolver arr;
    if (!tu::SetupUSDZAssetResolution(arr, &usdzAsset))
      throw std::runtime_error("could not set up USDZ asset resolution for '" + path + "'");
    env.asset_resolver = arr;
  } else {
    env.set_search_paths({tu::io::GetBaseDir(path)});
  }
  if (!converter.ConvertToRenderScene(env, &rs))
    throw std::runtime_error("could not convert USD stage '" + path + "': " + converter.GetError());

  ImportScene out;
  Reader(stage, rs, out).run();
  if (!converter.GetWarning().empty()) out.warnings.push_back("tinyusdz: " + converter.GetWarning());
  return out;
}

void addUsdBackend(BackendRegistry &registry) {
  registry.add({"usd",
                {".usd", ".usda", ".usdc", ".usdz"},
                [](const std::string &p) { return hasExtension(p, {".usd", ".usda", ".usdc", ".usdz"}) ? 100 : -1; },
                [] { return std::unique_ptr<ImportSource>(std::make_unique<UsdSource>()); }});
}

} // namespace x3d::runtime::importer
