// ImportEmit.cpp — ImportScene -> X3DDocument.
#include "ImportEmit.hpp"

#include "Interpolation.hpp" // Quat, rotationFromQuat
#include "Mat4.hpp"
#include "SceneBuilder.hpp"

#include "x3d/nodes/Appearance.hpp"
#include "x3d/nodes/Color.hpp"
#include "x3d/nodes/ColorInterpolator.hpp"
#include "x3d/nodes/ColorRGBA.hpp"
#include "x3d/nodes/Coordinate.hpp"
#include "x3d/nodes/CoordinateInterpolator.hpp"
#include "x3d/nodes/DirectionalLight.hpp"
#include "x3d/nodes/HAnimHumanoid.hpp"
#include "x3d/nodes/HAnimJoint.hpp"
#include "x3d/nodes/HAnimSegment.hpp"
#include "x3d/nodes/HAnimSite.hpp"
#include "x3d/nodes/ImageTexture.hpp"
#include "x3d/nodes/IndexedFaceSet.hpp"
#include "x3d/nodes/IndexedLineSet.hpp"
#include "x3d/nodes/IndexedTriangleSet.hpp"
#include "x3d/nodes/Material.hpp"
#include "x3d/nodes/MultiTextureCoordinate.hpp"
#include "x3d/nodes/Normal.hpp"
#include "x3d/nodes/NormalInterpolator.hpp"
#include "x3d/nodes/OrientationInterpolator.hpp"
#include "x3d/nodes/OrthoViewpoint.hpp"
#include "x3d/nodes/PhysicalMaterial.hpp"
#include "x3d/nodes/PointLight.hpp"
#include "x3d/nodes/PointSet.hpp"
#include "x3d/nodes/PositionInterpolator.hpp"
#include "x3d/nodes/ScalarInterpolator.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/SpotLight.hpp"
#include "x3d/nodes/TextureCoordinate.hpp"
#include "x3d/nodes/TextureProperties.hpp"
#include "x3d/nodes/TimeSensor.hpp"
#include "x3d/nodes/Transform.hpp"
#include "x3d/nodes/UnlitMaterial.hpp"
#include "x3d/nodes/Viewpoint.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace x3d::runtime::importer {

namespace {

using namespace x3d::core;
using namespace x3d::nodes;
using RtMat4 = x3d::runtime::Mat4;
using NodePtr = std::shared_ptr<X3DNode>;

constexpr float kPi = 3.14159265358979f;

// ---------------------------------------------------------------- math

RtMat4 toRt(const Mat4 &m) {
  RtMat4 r;
  r.m = m.m;
  return r;
}

Mat4 fromRt(const RtMat4 &m) {
  Mat4 r;
  r.m = m.m;
  return r;
}

Vec4 normalizeQuat(Vec4 q) {
  const float l = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
  if (l < 1e-12f) return {0, 0, 0, 1};
  return {q.x / l, q.y / l, q.z / l, q.w / l};
}

SFRotation rotationOf(const Vec4 &q) {
  const Vec4 n = normalizeQuat(q);
  SFRotation r = x3d::runtime::rotationFromQuat(x3d::runtime::Quat{n.x, n.y, n.z, n.w});
  // A zero angle is the default rotation, whatever axis the conversion picked.
  if (r.angle == 0.0f) r = SFRotation{0, 0, 1, 0};
  return r;
}

RtMat4 matFromTrs(const Trs &t) {
  return x3d::runtime::transformMatrix(
      SFVec3f{t.translation.x, t.translation.y, t.translation.z}, rotationOf(t.rotation),
      SFVec3f{t.scale.x, t.scale.y, t.scale.z}, SFVec3f{0, 0, 0}, SFRotation{0, 0, 1, 0});
}

/// Decomposes M = T*R*S. `shear` receives the largest deviation of the
/// rebuilt basis from the original, so callers can report a matrix TRS cannot
/// hold.
Trs decompose(const Mat4 &mat, float *shear = nullptr) {
  const auto &m = mat.m;
  Trs t;
  t.translation = {m[12], m[13], m[14]};
  Vec3 c0{m[0], m[1], m[2]}, c1{m[4], m[5], m[6]}, c2{m[8], m[9], m[10]};
  auto len = [](const Vec3 &v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); };
  float sx = len(c0), sy = len(c1), sz = len(c2);
  // A reflection: put it into the x scale so the rotation stays proper.
  const float det = c0.x * (c1.y * c2.z - c1.z * c2.y) - c0.y * (c1.x * c2.z - c1.z * c2.x) +
                    c0.z * (c1.x * c2.y - c1.y * c2.x);
  if (det < 0) sx = -sx;
  t.scale = {sx, sy, sz};
  auto div = [](Vec3 v, float s, Vec3 fb) {
    if (std::fabs(s) < 1e-12f) return fb;
    return Vec3{v.x / s, v.y / s, v.z / s};
  };
  const Vec3 r0 = div(c0, sx, {1, 0, 0}), r1 = div(c1, sy, {0, 1, 0}), r2 = div(c2, sz, {0, 0, 1});
  RtMat4 rot = RtMat4::identity();
  rot.m[0] = r0.x; rot.m[1] = r0.y; rot.m[2] = r0.z;
  rot.m[4] = r1.x; rot.m[5] = r1.y; rot.m[6] = r1.z;
  rot.m[8] = r2.x; rot.m[9] = r2.y; rot.m[10] = r2.z;
  const SFRotation r = x3d::runtime::rotationFromMatrix(rot);
  const auto q = x3d::runtime::quatFromRotation(r);
  t.rotation = normalizeQuat({static_cast<float>(q.x), static_cast<float>(q.y),
                              static_cast<float>(q.z), static_cast<float>(q.w)});
  if (shear) {
    const RtMat4 back = matFromTrs(t);
    float worst = 0;
    for (int i = 0; i < 12; ++i)
      if (i % 4 != 3) worst = std::max(worst, std::fabs(back.m[i] - m[i]));
    *shear = worst;
  }
  return t;
}

float clamp01(float v) { return std::clamp(std::isfinite(v) ? v : 0.0f, 0.0f, 1.0f); }
SFColor color3(const Vec3 &c) { return SFColor{clamp01(c.x), clamp01(c.y), clamp01(c.z)}; }
SFVec3f vec3(const Vec3 &v) { return SFVec3f{v.x, v.y, v.z}; }

SFVec3f unitNormal(const Vec3 &n) {
  const float l = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
  if (l < 1e-12f || !std::isfinite(l)) return SFVec3f{0, 0, 1};
  return SFVec3f{n.x / l, n.y / l, n.z / l};
}

// ---------------------------------------------------------------- names

/// X3D DEF ids: no whitespace or the characters " ' # , . [ ] \ { }, and not
/// starting with a digit, + or -.
class NameTable {
public:
  std::string make(const std::string &wanted, const std::string &fallback) {
    std::string s;
    for (char c : wanted) {
      const auto u = static_cast<unsigned char>(c);
      s += (std::isalnum(u) || c == '_' || c == '-' || u >= 0x80) ? c : '_';
    }
    if (s.empty()) s = fallback;
    if (std::isdigit(static_cast<unsigned char>(s[0])) || s[0] == '-' || s[0] == '+')
      s = "_" + s;
    std::string out = s;
    for (int n = 2; used_.count(out); ++n) out = s + "_" + std::to_string(n);
    used_.insert(out);
    return out;
  }

private:
  std::unordered_set<std::string> used_;
};

// ---------------------------------------------------------------- tracks

/// An X3D interpolator track: fraction keys and one value tuple per key.
struct Track {
  MFFloat keys;
  std::vector<std::vector<float>> values;
};

float hermite(float v0, float b0, float v1, float a1, float dt, float u) {
  const float u2 = u * u, u3 = u2 * u;
  return (2 * u3 - 3 * u2 + 1) * v0 + (u3 - 2 * u2 + u) * dt * b0 + (-2 * u3 + 3 * u2) * v1 +
         (u3 - u2) * dt * a1;
}

/// Converts a glTF-style channel into X3D keys. `comp` floats per value.
/// STEP becomes repeated keys (an X3D jump); CUBICSPLINE and `sampleLinear`
/// channels are sampled `steps` times per interval.
Track buildTrack(const AnimationChannel &ch, std::size_t comp, double duration, bool isQuat,
                 bool sampleLinear, int steps) {
  Track tr;
  const std::size_t n = ch.times.size();
  if (n == 0 || comp == 0) return tr;
  const bool cubic = ch.interpolation == AnimationChannel::Interpolation::CubicSpline;
  const std::size_t stride = cubic ? 3 * comp : comp;
  if (ch.values.size() < n * stride) return tr;
  auto value = [&](std::size_t k) {
    const std::size_t base = k * stride + (cubic ? comp : 0);
    return std::vector<float>(ch.values.begin() + static_cast<std::ptrdiff_t>(base),
                              ch.values.begin() + static_cast<std::ptrdiff_t>(base + comp));
  };
  auto frac = [&](float t) {
    return duration > 0 ? static_cast<float>(std::clamp(t / duration, 0.0, 1.0)) : 0.0f;
  };
  auto push = [&](float t, std::vector<float> v) {
    if (isQuat && v.size() == 4) {
      const Vec4 q = normalizeQuat({v[0], v[1], v[2], v[3]});
      v = {q.x, q.y, q.z, q.w};
    }
    tr.keys.push_back(frac(t));
    tr.values.push_back(std::move(v));
  };
  steps = std::max(1, steps);
  for (std::size_t k = 0; k < n; ++k) {
    const float t0 = ch.times[k];
    push(t0, value(k));
    if (k + 1 == n) break;
    const float t1 = ch.times[k + 1];
    const float dt = t1 - t0;
    if (ch.interpolation == AnimationChannel::Interpolation::Step) {
      push(t1, value(k)); // held to the next key, which then jumps
    } else if (cubic) {
      const std::size_t b0 = k * stride + 2 * comp, a1 = (k + 1) * stride;
      const std::vector<float> v0 = value(k), v1 = value(k + 1);
      for (int s = 1; s < steps; ++s) {
        const float u = static_cast<float>(s) / static_cast<float>(steps);
        std::vector<float> v(comp);
        for (std::size_t c = 0; c < comp; ++c)
          v[c] = hermite(v0[c], ch.values[b0 + c], v1[c], ch.values[a1 + c], dt, u);
        push(t0 + u * dt, std::move(v));
      }
    } else if (sampleLinear) {
      const std::vector<float> v0 = value(k), v1 = value(k + 1);
      for (int s = 1; s < steps; ++s) {
        const float u = static_cast<float>(s) / static_cast<float>(steps);
        std::vector<float> v(comp);
        for (std::size_t c = 0; c < comp; ++c) v[c] = v0[c] + (v1[c] - v0[c]) * u;
        push(t0 + u * dt, std::move(v));
      }
    }
  }
  return tr;
}

// ---------------------------------------------------------------- emitter

struct MaterialOut {
  NodePtr appearance;
  NodePtr material;
};

/// One skinned mesh placed in a humanoid: its range in the shared skinCoord.
struct SkinPart {
  int node = -1;
  int mesh = -1;
  std::size_t offset = 0;
};

struct SkinGroup {
  std::vector<int> skins;
  std::set<int> joints;      ///< node indices named by the skins
  std::set<int> skeleton;    ///< joints plus nodes on the way to them
  int root = -1;             ///< common ancestor; -1: the scene root
  bool rootIsJoint = false;
  std::vector<SkinPart> parts;
  std::shared_ptr<HAnimHumanoid> humanoid;
  NodePtr skinCoord, skinNormal;
  std::size_t vertexCount = 0;
  bool emitted = false;
};

class Emitter {
public:
  Emitter(const ImportScene &s, const EmitOptions &o, EmitReport &r) : s_(s), o_(o), rep_(r) {}

  x3d::runtime::X3DDocument run() {
    doc_.version = "4.0";
    doc_.profile = x3d::runtime::Profile::Full;
    const std::size_t nn = s_.nodes.size();
    parent_.assign(nn, -1);
    for (std::size_t i = 0; i < nn; ++i)
      for (int c : s_.nodes[i].childIndices)
        if (validNode(c) && parent_[static_cast<std::size_t>(c)] < 0)
          parent_[static_cast<std::size_t>(c)] = static_cast<int>(i);
    nodeOut_.assign(nn, nullptr);
    scanAnimatedWeights();
    buildSkinGroups();

    std::vector<int> roots = s_.roots;
    if (roots.empty())
      for (std::size_t i = 0; i < nn; ++i)
        if (parent_[i] < 0) roots.push_back(static_cast<int>(i));
    for (int r : roots) {
      if (!validNode(r)) continue;
      if (NodePtr n = emitNode(r)) doc_.scene.addRootNode(n);
    }
    // Skins whose joints span several top-level trees hang from the scene.
    for (std::size_t g = 0; g < groups_.size(); ++g)
      if (groups_[g].root < 0 && !groups_[g].emitted)
        if (NodePtr h = emitHumanoid(static_cast<int>(g))) doc_.scene.addRootNode(h);
    // Cameras and lights no node places.
    for (std::size_t c = 0; c < s_.cameras.size(); ++c)
      if (!placedCameras_.count(static_cast<int>(c)))
        doc_.scene.addRootNode(buildCamera(static_cast<int>(c)));
    for (std::size_t l = 0; l < s_.lights.size(); ++l)
      if (!placedLights_.count(static_cast<int>(l)))
        doc_.scene.addRootNode(buildLight(static_cast<int>(l)));

    emitAnimations();
    flushWarnings();
    return std::move(doc_);
  }

private:
  const ImportScene &s_;
  const EmitOptions &o_;
  EmitReport &rep_;
  x3d::runtime::X3DDocument doc_;
  NameTable names_;
  std::vector<int> parent_;
  std::vector<NodePtr> nodeOut_;
  std::map<std::pair<int, bool>, MaterialOut> materials_;
  std::map<std::string, NodePtr> textures_;
  std::map<std::pair<int, int>, NodePtr> shapes_; ///< (mesh, node or -1)
  /// node -> (mesh, Coordinate, Normal) of shapes whose weights it animates
  std::map<int, std::vector<std::tuple<int, NodePtr, NodePtr>>> morphShapes_;
  std::map<int, std::vector<NodePtr>> lightNodes_, cameraNodes_;
  std::set<int> placedCameras_, placedLights_;
  std::set<int> animatedWeightNodes_;
  std::vector<SkinGroup> groups_;
  std::vector<int> skinGroupOf_;   ///< skin -> group
  std::vector<int> groupAtNode_;   ///< node -> group whose humanoid sits there
  std::map<std::string, std::size_t> clampCounts_;

  bool validNode(int i) const { return i >= 0 && static_cast<std::size_t>(i) < s_.nodes.size(); }
  void clamped(const std::string &what) { ++clampCounts_[what]; }

  void flushWarnings() {
    for (const auto &[what, n] : clampCounts_)
      rep_.warnings.push_back(what + " (" + std::to_string(n) + ")");
  }

  std::string defFor(int node, const std::string &fallbackPrefix) {
    const std::string &nm = s_.nodes[static_cast<std::size_t>(node)].name;
    return names_.make(nm, fallbackPrefix + std::to_string(node));
  }

  void define(const NodePtr &n, const std::string &name) {
    n->setDEF(name);
    doc_.scene.define(name, n);
  }

  // ---------------------------------------------------------- materials

  std::string mappingName(int uvSet) const { return "uv" + std::to_string(uvSet); }

  /// The uv sets a material samples, in slot order.
  std::vector<int> uvSetsOf(int materialIndex) const {
    std::vector<int> out;
    if (materialIndex < 0 || static_cast<std::size_t>(materialIndex) >= s_.materials.size())
      return out;
    const TextureSlots &t = s_.materials[static_cast<std::size_t>(materialIndex)].textures;
    for (const auto *slot : {&t.baseColor, &t.normal, &t.emissive, &t.occlusion,
                             &t.metallicRoughness, &t.specular, &t.shininess, &t.ambient})
      if (slot->has_value() && std::find(out.begin(), out.end(), (*slot)->uvSet) == out.end())
        out.push_back((*slot)->uvSet);
    return out;
  }

  static x3d::core::TextureMinificationModeChoices minFilter(TextureFilter f) {
    using M = x3d::core::TextureMinificationModeChoices;
    switch (f) {
      case TextureFilter::Default: return M::DEFAULT;
      case TextureFilter::Nearest: return M::NEAREST_PIXEL;
      case TextureFilter::Linear: return M::AVG_PIXEL;
      case TextureFilter::NearestMipmapNearest: return M::NEAREST_PIXEL_NEAREST_MIPMAP;
      case TextureFilter::LinearMipmapNearest: return M::AVG_PIXEL_NEAREST_MIPMAP;
      case TextureFilter::NearestMipmapLinear: return M::NEAREST_PIXEL_AVG_MIPMAP;
      case TextureFilter::LinearMipmapLinear: return M::AVG_PIXEL_AVG_MIPMAP;
    }
    return M::DEFAULT;
  }

  static x3d::core::TextureMagnificationModeChoices magFilter(TextureFilter f) {
    using M = x3d::core::TextureMagnificationModeChoices;
    switch (f) {
      case TextureFilter::Default: return M::DEFAULT;
      case TextureFilter::Nearest:
      case TextureFilter::NearestMipmapNearest:
      case TextureFilter::NearestMipmapLinear: return M::NEAREST_PIXEL;
      case TextureFilter::Linear:
      case TextureFilter::LinearMipmapNearest:
      case TextureFilter::LinearMipmapLinear: return M::AVG_PIXEL;
    }
    return M::DEFAULT;
  }

  static x3d::core::TextureBoundaryModeChoices boundary(TextureWrap w) {
    using B = x3d::core::TextureBoundaryModeChoices;
    switch (w) {
      case TextureWrap::Repeat: return B::REPEAT;
      case TextureWrap::ClampToEdge: return B::CLAMP_TO_EDGE;
      case TextureWrap::MirroredRepeat: return B::MIRRORED_REPEAT;
    }
    return B::REPEAT;
  }

  NodePtr texture(const TextureRef &ref) {
    if (!o_.textures) return nullptr;
    const std::string url = urlForRef(*o_.textures, ref);
    if (url.empty()) return nullptr;
    const std::string key = url + "|" + std::to_string(static_cast<int>(ref.wrapS)) + "," +
                            std::to_string(static_cast<int>(ref.wrapT)) + "," +
                            std::to_string(static_cast<int>(ref.minFilter)) + "," +
                            std::to_string(static_cast<int>(ref.magFilter));
    if (auto it = textures_.find(key); it != textures_.end()) return it->second;
    auto tex = std::make_shared<ImageTexture>();
    tex->setUrl(MFString{url});
    tex->setRepeatSUnchecked(ref.wrapS != TextureWrap::ClampToEdge);
    tex->setRepeatTUnchecked(ref.wrapT != TextureWrap::ClampToEdge);
    const bool needsProperties = ref.wrapS == TextureWrap::MirroredRepeat ||
                                 ref.wrapT == TextureWrap::MirroredRepeat ||
                                 ref.minFilter != TextureFilter::Default ||
                                 ref.magFilter != TextureFilter::Default;
    if (needsProperties) {
      auto props = std::make_shared<TextureProperties>();
      props->setBoundaryModeS(boundary(ref.wrapS));
      props->setBoundaryModeT(boundary(ref.wrapT));
      props->setMinificationFilter(minFilter(ref.minFilter));
      props->setMagnificationFilter(magFilter(ref.magFilter));
      const bool mip = ref.minFilter == TextureFilter::Default ||
                       ref.minFilter == TextureFilter::NearestMipmapNearest ||
                       ref.minFilter == TextureFilter::LinearMipmapNearest ||
                       ref.minFilter == TextureFilter::NearestMipmapLinear ||
                       ref.minFilter == TextureFilter::LinearMipmapLinear;
      props->setGenerateMipMapsUnchecked(mip);
      tex->setTexturePropertiesUnchecked(props);
    }
    define(tex, names_.make("Tex", "Tex"));
    textures_.emplace(key, tex);
    ++rep_.textures;
    return tex;
  }

  /// One shared Appearance per material (and a separate unlit one for line
  /// and point geometry, which X3D does not light).
  MaterialOut appearance(int materialIndex, bool linesOrPoints) {
    const auto key = std::make_pair(materialIndex, linesOrPoints);
    if (auto it = materials_.find(key); it != materials_.end()) return it->second;
    MaterialOut out;
    auto app = std::make_shared<Appearance>();
    const bool valid =
        materialIndex >= 0 && static_cast<std::size_t>(materialIndex) < s_.materials.size();
    const ImportMaterial def;
    const ImportMaterial &m = valid ? s_.materials[static_cast<std::size_t>(materialIndex)] : def;
    const std::vector<int> sets = uvSetsOf(materialIndex);
    const bool mapped = sets.size() > 1;
    auto mapping = [&](const std::optional<TextureRef> &r) {
      return (mapped && r) ? mappingName(r->uvSet) : std::string();
    };
    const Vec4 base = m.pbr ? m.pbr->baseColor : Vec4{m.diffuse.x, m.diffuse.y, m.diffuse.z, m.opacity};
    const float transparency = 1.0f - clamp01(base.w);

    if (m.unlit || linesOrPoints) {
      auto mat = std::make_shared<UnlitMaterial>();
      mat->setEmissiveColor(color3({base.x, base.y, base.z}));
      mat->setTransparency(transparency);
      if (m.textures.baseColor)
        if (auto t = texture(*m.textures.baseColor)) {
          mat->setEmissiveTexture(t);
          mat->setEmissiveTextureMapping(mapping(m.textures.baseColor));
        }
      out.material = mat;
    } else if (m.pbr) {
      auto mat = std::make_shared<PhysicalMaterial>();
      mat->setBaseColor(color3({base.x, base.y, base.z}));
      mat->setMetallic(clamp01(m.pbr->metallic));
      mat->setRoughness(clamp01(m.pbr->roughness));
      mat->setEmissiveColor(color3(m.emissive));
      if (m.emissive.x > 1 || m.emissive.y > 1 || m.emissive.z > 1)
        clamped("emissive colours clamped to [0,1]");
      mat->setTransparency(transparency);
      mat->setNormalScaleUnchecked(std::max(0.0f, m.normalScale));
      mat->setOcclusionStrength(clamp01(m.occlusionStrength));
      const TextureSlots &t = m.textures;
      if (t.baseColor)
        if (auto x = texture(*t.baseColor)) {
          mat->setBaseTexture(x);
          mat->setBaseTextureMapping(mapping(t.baseColor));
        }
      if (t.normal)
        if (auto x = texture(*t.normal)) {
          mat->setNormalTexture(x);
          mat->setNormalTextureMapping(mapping(t.normal));
        }
      if (t.emissive)
        if (auto x = texture(*t.emissive)) {
          mat->setEmissiveTexture(x);
          mat->setEmissiveTextureMapping(mapping(t.emissive));
        }
      if (t.occlusion)
        if (auto x = texture(*t.occlusion)) {
          mat->setOcclusionTexture(x);
          mat->setOcclusionTextureMapping(mapping(t.occlusion));
        }
      if (t.metallicRoughness)
        if (auto x = texture(*t.metallicRoughness)) {
          mat->setMetallicRoughnessTexture(x);
          mat->setMetallicRoughnessTextureMapping(mapping(t.metallicRoughness));
        }
      if (t.specular || t.shininess || t.ambient)
        clamped("Phong-only texture slots on a metallic-roughness material dropped");
      out.material = mat;
    } else {
      auto mat = std::make_shared<Material>();
      mat->setDiffuseColor(color3({base.x, base.y, base.z}));
      mat->setSpecularColor(color3(m.specular));
      mat->setEmissiveColor(color3(m.emissive));
      mat->setAmbientIntensity(clamp01(m.ambientIntensity));
      mat->setShininess(clamp01(m.shininess));
      mat->setTransparency(transparency);
      mat->setNormalScaleUnchecked(std::max(0.0f, m.normalScale));
      mat->setOcclusionStrength(clamp01(m.occlusionStrength));
      const TextureSlots &t = m.textures;
      if (t.baseColor)
        if (auto x = texture(*t.baseColor)) {
          mat->setDiffuseTexture(x);
          mat->setDiffuseTextureMapping(mapping(t.baseColor));
        }
      if (t.normal)
        if (auto x = texture(*t.normal)) {
          mat->setNormalTexture(x);
          mat->setNormalTextureMapping(mapping(t.normal));
        }
      if (t.emissive)
        if (auto x = texture(*t.emissive)) {
          mat->setEmissiveTexture(x);
          mat->setEmissiveTextureMapping(mapping(t.emissive));
        }
      if (t.occlusion)
        if (auto x = texture(*t.occlusion)) {
          mat->setOcclusionTexture(x);
          mat->setOcclusionTextureMapping(mapping(t.occlusion));
        }
      if (t.specular)
        if (auto x = texture(*t.specular)) {
          mat->setSpecularTexture(x);
          mat->setSpecularTextureMapping(mapping(t.specular));
        }
      if (t.shininess)
        if (auto x = texture(*t.shininess)) {
          mat->setShininessTexture(x);
          mat->setShininessTextureMapping(mapping(t.shininess));
        }
      if (t.ambient)
        if (auto x = texture(*t.ambient)) {
          mat->setAmbientTexture(x);
          mat->setAmbientTextureMapping(mapping(t.ambient));
        }
      if (t.metallicRoughness)
        clamped("metallic-roughness textures on a Phong material dropped");
      out.material = mat;
    }
    switch (m.alpha) {
      case AlphaMode::Opaque: app->setAlphaMode(x3d::core::AlphaModeChoices::OPAQUE); break;
      case AlphaMode::Mask:
        app->setAlphaMode(x3d::core::AlphaModeChoices::MASK);
        app->setAlphaCutoff(clamp01(m.alphaCutoff));
        break;
      case AlphaMode::Blend: app->setAlphaMode(x3d::core::AlphaModeChoices::BLEND); break;
    }
    app->setMaterial(out.material);
    out.appearance = app;
    if (valid) {
      const std::string base = names_.make(m.name, "Material" + std::to_string(materialIndex));
      define(out.material, linesOrPoints ? names_.make(base + "_unlit", base) : base);
      define(app, names_.make(base + "_appearance", base));
    }
    ++rep_.materials;
    materials_.emplace(key, out);
    return out;
  }

  bool doubleSided(int materialIndex) const {
    if (materialIndex < 0 || static_cast<std::size_t>(materialIndex) >= s_.materials.size())
      return false;
    return s_.materials[static_cast<std::size_t>(materialIndex)].doubleSided;
  }

  /// glTF multiplies vertex colours by the base colour; X3D replaces it.
  Vec4 colorFactor(int materialIndex) const {
    if (materialIndex < 0 || static_cast<std::size_t>(materialIndex) >= s_.materials.size())
      return {1, 1, 1, 1};
    const ImportMaterial &m = s_.materials[static_cast<std::size_t>(materialIndex)];
    return m.pbr ? m.pbr->baseColor : Vec4{1, 1, 1, 1};
  }

  // ---------------------------------------------------------- geometry

  static std::vector<Vec3> morphed(const std::vector<Vec3> &base,
                                   const std::vector<MorphTarget> &targets,
                                   const std::vector<float> &weights, bool normals) {
    std::vector<Vec3> out = base;
    for (std::size_t t = 0; t < targets.size() && t < weights.size(); ++t) {
      const auto &d = normals ? targets[t].normals : targets[t].positions;
      if (d.size() != out.size() || weights[t] == 0) continue;
      for (std::size_t i = 0; i < out.size(); ++i) {
        out[i].x += weights[t] * d[i].x;
        out[i].y += weights[t] * d[i].y;
        out[i].z += weights[t] * d[i].z;
      }
    }
    return out;
  }

  const std::vector<float> &weightsFor(int node, const ImportMesh &mesh) const {
    if (validNode(node) && !s_.nodes[static_cast<std::size_t>(node)].morphWeights.empty())
      return s_.nodes[static_cast<std::size_t>(node)].morphWeights;
    return mesh.morphWeights;
  }

  static MFVec3f toMF(const std::vector<Vec3> &v) {
    MFVec3f out;
    out.reserve(v.size());
    for (const auto &p : v) out.push_back(vec3(p));
    return out;
  }

  static MFVec3f toNormals(const std::vector<Vec3> &v) {
    MFVec3f out;
    out.reserve(v.size());
    for (const auto &n : v) out.push_back(unitNormal(n));
    return out;
  }

  /// TextureCoordinate or MultiTextureCoordinate for the sets the material
  /// samples (set 0 when it samples none).
  NodePtr texCoordFor(const ImportMesh &mesh, int materialIndex) {
    std::vector<int> sets = uvSetsOf(materialIndex);
    if (sets.empty()) sets.push_back(0);
    auto one = [&](int set) -> std::shared_ptr<TextureCoordinate> {
      if (set < 0 || static_cast<std::size_t>(set) >= mesh.uvSets.size()) return nullptr;
      const auto &uv = mesh.uvSets[static_cast<std::size_t>(set)];
      if (uv.size() != mesh.positions.size()) return nullptr;
      auto tc = std::make_shared<TextureCoordinate>();
      MFVec2f pts;
      pts.reserve(uv.size());
      for (const auto &t : uv) pts.push_back(SFVec2f{t.x, t.y});
      tc->setPoint(pts);
      return tc;
    };
    if (sets.size() == 1) {
      auto tc = one(sets[0]);
      if (!tc && !(sets[0] == 0 && mesh.uvSets.empty()))
        clamped("textures on a missing texture coordinate set dropped");
      return tc;
    }
    auto multi = std::make_shared<MultiTextureCoordinate>();
    MFNode list;
    for (int set : sets) {
      if (auto tc = one(set)) {
        tc->setMapping(mappingName(set));
        list.push_back(tc);
      } else {
        clamped("textures on a missing texture coordinate set dropped");
      }
    }
    if (list.empty()) return nullptr;
    multi->setTexCoord(list);
    return multi;
  }

  NodePtr colorNode(const ImportMesh &mesh) {
    if (mesh.colors.size() != mesh.positions.size() || mesh.colors.empty()) return nullptr;
    const Vec4 f = colorFactor(mesh.materialIndex);
    if (mesh.colorsHaveAlpha) {
      auto c = std::make_shared<ColorRGBA>();
      MFColorRGBA v;
      v.reserve(mesh.colors.size());
      for (const auto &x : mesh.colors)
        v.push_back(SFColorRGBA{clamp01(x.x * f.x), clamp01(x.y * f.y), clamp01(x.z * f.z),
                                clamp01(x.w * f.w)});
      c->setColor(v);
      return c;
    }
    auto c = std::make_shared<Color>();
    MFColor v;
    v.reserve(mesh.colors.size());
    for (const auto &x : mesh.colors)
      v.push_back(SFColor{clamp01(x.x * f.x), clamp01(x.y * f.y), clamp01(x.z * f.z)});
    c->setColor(v);
    return c;
  }

  /// Builds a Shape for `meshIndex` as placed by `node` (whose morph weights
  /// apply). Returns the Coordinate and Normal it used.
  std::shared_ptr<Shape> buildShape(int meshIndex, int node, NodePtr *coordOut,
                                    NodePtr *normalOut) {
    const ImportMesh &mesh = s_.meshes[static_cast<std::size_t>(meshIndex)];
    const auto &w = weightsFor(node, mesh);
    auto coord = std::make_shared<Coordinate>();
    coord->setPoint(toMF(morphed(mesh.positions, mesh.morphTargets, w, false)));
    std::shared_ptr<Normal> normal;
    if (mesh.normals.size() == mesh.positions.size() && !mesh.normals.empty() &&
        mesh.topology == ImportMesh::Topology::Triangles) {
      normal = std::make_shared<Normal>();
      normal->setVector(toNormals(morphed(mesh.normals, mesh.morphTargets, w, true)));
    }
    auto shape = std::make_shared<Shape>();
    const bool linear = mesh.topology != ImportMesh::Topology::Triangles;
    shape->setAppearance(appearance(mesh.materialIndex, linear).appearance);
    const std::size_t nv = mesh.positions.size();
    switch (mesh.topology) {
      case ImportMesh::Topology::Triangles: {
        auto its = std::make_shared<IndexedTriangleSet>();
        MFInt32 idx;
        idx.reserve(mesh.indices.size());
        for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
          if (mesh.indices[i] >= nv || mesh.indices[i + 1] >= nv || mesh.indices[i + 2] >= nv) {
            clamped("triangles with out-of-range indices dropped");
            continue;
          }
          for (int k = 0; k < 3; ++k) idx.push_back(static_cast<int>(mesh.indices[i + k]));
        }
        its->setIndexUnchecked(idx);
        its->setCoord(coord);
        if (normal) its->setNormal(normal);
        if (auto tc = texCoordFor(mesh, mesh.materialIndex)) its->setTexCoord(tc);
        if (auto c = colorNode(mesh)) its->setColor(c);
        its->setSolidUnchecked(!doubleSided(mesh.materialIndex));
        its->setCcwUnchecked(true);
        shape->setGeometry(its);
        break;
      }
      case ImportMesh::Topology::Lines: {
        auto ils = std::make_shared<IndexedLineSet>();
        MFInt32 idx;
        for (std::size_t i = 0; i + 1 < mesh.indices.size(); i += 2) {
          if (mesh.indices[i] >= nv || mesh.indices[i + 1] >= nv) continue;
          idx.push_back(static_cast<int>(mesh.indices[i]));
          idx.push_back(static_cast<int>(mesh.indices[i + 1]));
          idx.push_back(-1);
        }
        ils->setCoordIndexUnchecked(idx);
        ils->setCoord(coord);
        if (auto c = colorNode(mesh)) ils->setColor(c);
        shape->setGeometry(ils);
        break;
      }
      case ImportMesh::Topology::Points: {
        auto ps = std::make_shared<PointSet>();
        ps->setCoord(coord);
        if (auto c = colorNode(mesh)) ps->setColor(c);
        shape->setGeometry(ps);
        break;
      }
    }
    if (coordOut) *coordOut = coord;
    if (normalOut) *normalOut = normal;
    ++rep_.shapes;
    return shape;
  }

  /// The Shape for one mesh placement. Shapes are shared (DEF/USE) unless the
  /// node's own morph weights differ or are animated.
  NodePtr shapeFor(int meshIndex, int node) {
    const ImportMesh &mesh = s_.meshes[static_cast<std::size_t>(meshIndex)];
    const bool own = !mesh.morphTargets.empty() &&
                     (animatedWeightNodes_.count(node) ||
                      !s_.nodes[static_cast<std::size_t>(node)].morphWeights.empty());
    const auto key = std::make_pair(meshIndex, own ? node : -1);
    if (auto it = shapes_.find(key); it != shapes_.end()) return it->second;
    NodePtr coord, normal;
    auto shape = buildShape(meshIndex, node, &coord, &normal);
    const std::string base = names_.make(mesh.name, "Mesh" + std::to_string(meshIndex));
    define(shape, base);
    if (own && animatedWeightNodes_.count(node)) {
      define(coord, names_.make(base + "_coord", base));
      if (normal) define(normal, names_.make(base + "_normal", base));
      morphShapes_[node].emplace_back(meshIndex, coord, normal);
    }
    shapes_.emplace(key, shape);
    return shape;
  }

  // ---------------------------------------------------------- cameras, lights

  NodePtr buildCamera(int index) {
    const ImportCamera &c = s_.cameras[static_cast<std::size_t>(index)];
    const Trs t = decompose(c.local);
    NodePtr out;
    if (c.kind == ImportCamera::Kind::Orthographic) {
      auto vp = std::make_shared<OrthoViewpoint>();
      vp->setPosition(vec3(t.translation));
      vp->setOrientation(rotationOf(t.rotation));
      vp->setFieldOfView(MFFloat{-c.xmag, -c.ymag, c.xmag, c.ymag});
      if (!c.name.empty()) vp->setDescription(c.name);
      if (c.znear > 0) vp->setNearDistance(c.znear);
      if (c.zfar > 0) vp->setFarDistance(c.zfar);
      out = vp;
    } else {
      auto vp = std::make_shared<Viewpoint>();
      vp->setPosition(vec3(t.translation));
      vp->setOrientation(rotationOf(t.rotation));
      // X3D fieldOfView spans the smaller viewport side; glTF yfov the height.
      float fov = c.yfov;
      if (c.aspectRatio > 0 && c.aspectRatio < 1)
        fov = 2.0f * std::atan(std::tan(c.yfov * 0.5f) * c.aspectRatio);
      if (!(fov > 0) || fov >= kPi) {
        clamped("camera fields of view outside (0, pi) replaced by pi/4");
        fov = kPi / 4;
      }
      vp->setFieldOfView(fov);
      if (!c.name.empty()) vp->setDescription(c.name);
      if (c.znear > 0) vp->setNearDistance(c.znear);
      if (c.zfar > 0) vp->setFarDistance(c.zfar);
      out = vp;
    }
    define(out, names_.make(c.name.empty() ? "" : c.name + "_view", "Camera" + std::to_string(index)));
    cameraNodes_[index].push_back(out);
    ++rep_.viewpoints;
    return out;
  }

  float lightIntensity(float v) {
    if (v > 1) clamped("light intensities above 1 clamped (X3D intensity is a [0,1] factor)");
    return clamp01(v);
  }

  NodePtr buildLight(int index) {
    const ImportLight &l = s_.lights[static_cast<std::size_t>(index)];
    const float radius = l.range > 0 ? l.range : std::numeric_limits<float>::max();
    NodePtr out;
    if (l.color.x > 1 || l.color.y > 1 || l.color.z > 1) clamped("light colours clamped to [0,1]");
    switch (l.kind) {
      case ImportLight::Kind::Directional: {
        auto n = std::make_shared<DirectionalLight>();
        n->setDirection(vec3(l.direction));
        n->setColor(color3(l.color));
        n->setIntensity(lightIntensity(l.intensity));
        n->setGlobal(true);
        out = n;
        break;
      }
      case ImportLight::Kind::Point: {
        auto n = std::make_shared<PointLight>();
        n->setLocation(vec3(l.position));
        n->setColor(color3(l.color));
        n->setIntensity(lightIntensity(l.intensity));
        n->setAttenuationUnchecked(vec3(l.attenuation));
        n->setRadiusUnchecked(radius);
        n->setGlobal(true);
        out = n;
        break;
      }
      case ImportLight::Kind::Spot: {
        auto n = std::make_shared<SpotLight>();
        n->setLocation(vec3(l.position));
        n->setDirection(vec3(l.direction));
        n->setColor(color3(l.color));
        n->setIntensity(lightIntensity(l.intensity));
        n->setAttenuationUnchecked(vec3(l.attenuation));
        n->setRadiusUnchecked(radius);
        const float outer = std::clamp(l.outerConeAngle, 1e-4f, kPi / 2);
        const float inner = std::clamp(l.innerConeAngle, 1e-4f, outer);
        n->setCutOffAngleUnchecked(outer);
        n->setBeamWidthUnchecked(inner);
        n->setGlobal(true);
        out = n;
        break;
      }
    }
    define(out, names_.make(l.name.empty() ? "" : l.name + "_light", "Light" + std::to_string(index)));
    lightNodes_[index].push_back(out);
    ++rep_.lights;
    return out;
  }

  // ---------------------------------------------------------- hierarchy

  bool skinnedHere(int node) const {
    const int skin = s_.nodes[static_cast<std::size_t>(node)].skin;
    return skin >= 0 && static_cast<std::size_t>(skin) < skinGroupOf_.size() &&
           skinGroupOf_[static_cast<std::size_t>(skin)] >= 0;
  }

  /// The node's own content (meshes not skinned elsewhere, camera, light).
  void ownContent(int node, MFNode &children) {
    const ImportNode &n = s_.nodes[static_cast<std::size_t>(node)];
    if (!skinnedHere(node))
      for (int m : n.meshIndices)
        if (m >= 0 && static_cast<std::size_t>(m) < s_.meshes.size())
          children.push_back(shapeFor(m, node));
    if (n.camera >= 0 && static_cast<std::size_t>(n.camera) < s_.cameras.size()) {
      children.push_back(buildCamera(n.camera));
      placedCameras_.insert(n.camera);
    }
    if (n.light >= 0 && static_cast<std::size_t>(n.light) < s_.lights.size()) {
      children.push_back(buildLight(n.light));
      placedLights_.insert(n.light);
    }
  }

  Trs trsOf(int node) {
    const ImportNode &n = s_.nodes[static_cast<std::size_t>(node)];
    if (n.trs) return *n.trs;
    float shear = 0;
    Trs t = decompose(n.localTransform, &shear);
    if (shear > 1e-4f) clamped("node matrices with shear approximated by translation/rotation/scale");
    return t;
  }

  NodePtr emitNode(int node) {
    if (static_cast<std::size_t>(node) < groupAtNode_.size() &&
        groupAtNode_[static_cast<std::size_t>(node)] >= 0)
      return emitHumanoid(groupAtNode_[static_cast<std::size_t>(node)]);
    auto tr = std::make_shared<Transform>();
    const Trs t = trsOf(node);
    tr->setTranslation(vec3(t.translation));
    tr->setRotation(rotationOf(t.rotation));
    tr->setScale(vec3(t.scale));
    MFNode children;
    ownContent(node, children);
    for (int c : s_.nodes[static_cast<std::size_t>(node)].childIndices)
      if (validNode(c) && parent_[static_cast<std::size_t>(c)] == node)
        if (NodePtr n = emitNode(c)) children.push_back(n);
    tr->setChildren(children);
    define(tr, defFor(node, "Node"));
    nodeOut_[static_cast<std::size_t>(node)] = tr;
    ++rep_.transforms;
    return tr;
  }

  // ---------------------------------------------------------- skins

  int lca(int a, int b) const {
    std::vector<int> pa;
    for (int x = a; x >= 0; x = parent_[static_cast<std::size_t>(x)]) pa.push_back(x);
    for (int y = b; y >= 0; y = parent_[static_cast<std::size_t>(y)])
      if (std::find(pa.begin(), pa.end(), y) != pa.end()) return y;
    return -1;
  }

  void buildSkinGroups() {
    const std::size_t ns = s_.skins.size();
    skinGroupOf_.assign(ns, -1);
    groupAtNode_.assign(s_.nodes.size(), -1);
    // Union skins that share a joint.
    std::vector<int> uf(ns);
    for (std::size_t i = 0; i < ns; ++i) uf[i] = static_cast<int>(i);
    std::function<int(int)> find = [&](int x) {
      return uf[static_cast<std::size_t>(x)] == x ? x
                                                   : uf[static_cast<std::size_t>(x)] =
                                                         find(uf[static_cast<std::size_t>(x)]);
    };
    std::map<int, int> jointOwner;
    for (std::size_t i = 0; i < ns; ++i) {
      bool ok = !s_.skins[i].joints.empty();
      for (int j : s_.skins[i].joints) ok = ok && validNode(j);
      if (!ok) {
        rep_.warnings.push_back("skin " + std::to_string(i) +
                                " has no valid joints; its meshes are emitted unskinned");
        uf[i] = -1;
        continue;
      }
      for (int j : s_.skins[i].joints) {
        auto [it, fresh] = jointOwner.emplace(j, static_cast<int>(i));
        if (!fresh) uf[static_cast<std::size_t>(find(static_cast<int>(i)))] = find(it->second);
      }
    }
    std::map<int, int> groupOfRoot;
    for (std::size_t i = 0; i < ns; ++i) {
      if (uf[i] < 0) continue;
      const int r = find(static_cast<int>(i));
      auto [it, fresh] = groupOfRoot.emplace(r, static_cast<int>(groups_.size()));
      if (fresh) groups_.emplace_back();
      SkinGroup &g = groups_[static_cast<std::size_t>(it->second)];
      g.skins.push_back(static_cast<int>(i));
      for (int j : s_.skins[i].joints) g.joints.insert(j);
      skinGroupOf_[i] = it->second;
    }
    for (std::size_t gi = 0; gi < groups_.size(); ++gi) {
      SkinGroup &g = groups_[gi];
      int root = *g.joints.begin();
      for (int j : g.joints) {
        root = lca(root, j);
        if (root < 0) break;
      }
      g.root = root;
      g.rootIsJoint = root >= 0 && g.joints.count(root);
      for (int j : g.joints)
        for (int x = j; x >= 0 && x != root; x = parent_[static_cast<std::size_t>(x)])
          g.skeleton.insert(x);
      if (g.rootIsJoint) g.skeleton.insert(root);
      if (root >= 0) groupAtNode_[static_cast<std::size_t>(root)] = static_cast<int>(gi);
    }
    for (std::size_t n = 0; n < s_.nodes.size(); ++n) {
      const int skin = s_.nodes[n].skin;
      if (skin < 0 || static_cast<std::size_t>(skin) >= ns || skinGroupOf_[static_cast<std::size_t>(skin)] < 0)
        continue;
      for (int m : s_.nodes[n].meshIndices)
        if (m >= 0 && static_cast<std::size_t>(m) < s_.meshes.size())
          groups_[static_cast<std::size_t>(skinGroupOf_[static_cast<std::size_t>(skin)])]
              .parts.push_back({static_cast<int>(n), m, 0});
    }
  }

  RtMat4 restLocal(int node) { return matFromTrs(trsOf(node)); }

  NodePtr emitJoint(SkinGroup &g, int node, MFNode &jointList, MFNode &segments) {
    auto j = std::make_shared<HAnimJoint>();
    const Trs t = trsOf(node);
    j->setTranslation(vec3(t.translation));
    j->setRotation(rotationOf(t.rotation));
    j->setScale(vec3(t.scale));
    const std::string def = defFor(node, "Joint");
    j->setName(def);
    define(j, def);
    nodeOut_[static_cast<std::size_t>(node)] = j;
    jointList.push_back(j);
    ++rep_.joints;
    MFNode children, rigid;
    ownContent(node, rigid);
    for (int c : s_.nodes[static_cast<std::size_t>(node)].childIndices) {
      if (!validNode(c) || parent_[static_cast<std::size_t>(c)] != node) continue;
      if (g.skeleton.count(c))
        children.push_back(emitJoint(g, c, jointList, segments));
      else if (NodePtr n = emitNode(c))
        rigid.push_back(n);
    }
    if (!rigid.empty()) {
      auto seg = std::make_shared<HAnimSegment>();
      seg->setName(def + "_segment");
      seg->setChildren(rigid);
      define(seg, names_.make(def + "_segment", def));
      children.push_back(seg);
      segments.push_back(seg);
    }
    j->setChildren(children);
    return j;
  }

  NodePtr emitHumanoid(int gi) {
    SkinGroup &g = groups_[static_cast<std::size_t>(gi)];
    if (g.emitted) return nullptr;
    g.emitted = true;
    auto h = std::make_shared<HAnimHumanoid>();
    g.humanoid = h;
    h->setVersion(x3d::core::HanimVersionChoices::_2_0);
    MFNode skeleton, jointList, segments, sites;
    std::string hname;
    if (g.rootIsJoint || g.root < 0) {
      std::string skinName;
      for (int s : g.skins)
        if (!s_.skins[static_cast<std::size_t>(s)].name.empty()) {
          skinName = s_.skins[static_cast<std::size_t>(s)].name;
          break;
        }
      hname = names_.make(skinName, "Humanoid");
      if (g.rootIsJoint) {
        skeleton.push_back(emitJoint(g, g.root, jointList, segments));
      } else {
        // Joints in several top-level trees: each tree holding joints joins
        // the skeleton.
        std::vector<int> roots = s_.roots;
        for (int r : roots)
          if (validNode(r) && g.skeleton.count(r))
            skeleton.push_back(emitJoint(g, r, jointList, segments));
      }
    } else {
      const Trs t = trsOf(g.root);
      h->setTranslation(vec3(t.translation));
      h->setRotation(rotationOf(t.rotation));
      h->setScale(vec3(t.scale));
      hname = defFor(g.root, "Humanoid");
      nodeOut_[static_cast<std::size_t>(g.root)] = h;
      MFNode rigid;
      ownContent(g.root, rigid);
      for (int c : s_.nodes[static_cast<std::size_t>(g.root)].childIndices) {
        if (!validNode(c) || parent_[static_cast<std::size_t>(c)] != g.root) continue;
        if (g.skeleton.count(c))
          skeleton.push_back(emitJoint(g, c, jointList, segments));
        else if (NodePtr n = emitNode(c))
          rigid.push_back(n);
      }
      if (!rigid.empty()) {
        auto site = std::make_shared<HAnimSite>();
        site->setName(hname + "_site");
        site->setChildren(rigid);
        define(site, names_.make(hname + "_site", hname));
        skeleton.push_back(site);
        sites.push_back(site);
      }
    }
    h->setName(hname);
    define(h, hname);
    h->setSkeleton(skeleton);
    h->setJoints(jointList);
    h->setSegments(segments);
    h->setSites(sites);
    bindHumanoid(g, jointList);
    ++rep_.humanoids;
    return h;
  }

  /// Binding pose, skin coordinates and weights for one humanoid.
  void bindHumanoid(SkinGroup &g, const MFNode &jointList) {
    // Node index for each emitted joint, in joints-list order.
    std::vector<int> jointNodes;
    std::unordered_map<const X3DNode *, int> nodeOfJoint;
    for (std::size_t n = 0; n < nodeOut_.size(); ++n)
      if (nodeOut_[n]) nodeOfJoint[nodeOut_[n].get()] = static_cast<int>(n);
    for (const auto &j : jointList) jointNodes.push_back(nodeOfJoint.at(j.get()));

    // Bind matrices: the inverse of each skin joint's inverse bind matrix;
    // other skeleton nodes keep their rest transform.
    std::map<int, RtMat4> bindWorld;
    std::map<int, RtMat4> ibmOf;
    for (int s : g.skins) {
      const ImportSkin &skin = s_.skins[static_cast<std::size_t>(s)];
      for (std::size_t k = 0; k < skin.joints.size(); ++k) {
        const RtMat4 ibm = k < skin.inverseBind.size() ? toRt(skin.inverseBind[k]) : RtMat4::identity();
        auto [it, fresh] = ibmOf.emplace(skin.joints[k], ibm);
        if (!fresh) {
          float d = 0;
          for (int i = 0; i < 16; ++i) d = std::max(d, std::fabs(it->second.m[i] - ibm.m[i]));
          if (d > 1e-4f) clamped("joints bound differently by two skins use the first binding");
        }
      }
    }
    MFVec3f bindT, bindS;
    MFRotation bindR;
    for (int n : jointNodes) {
      int p = parent_[static_cast<std::size_t>(n)];
      while (p >= 0 && !g.skeleton.count(p)) p = -1; // parent joint or none
      const RtMat4 parentBind = p >= 0 ? bindWorld.at(p) : RtMat4::identity();
      RtMat4 world;
      if (auto it = ibmOf.find(n); it != ibmOf.end()) {
        auto inv = it->second.tryInverse();
        if (!inv) {
          clamped("singular inverse bind matrices replaced by the rest pose");
          world = parentBind * restLocal(n);
        } else {
          world = *inv;
        }
      } else {
        world = parentBind * restLocal(n);
      }
      bindWorld[n] = world;
      const auto parentInv = parentBind.tryInverse();
      const RtMat4 local = parentInv ? (*parentInv) * world : world;
      float shear = 0;
      const Trs t = decompose(fromRt(local), &shear);
      if (shear > 1e-3f) clamped("joint binding matrices with shear approximated");
      bindT.push_back(vec3(t.translation));
      bindR.push_back(rotationOf(t.rotation));
      bindS.push_back(vec3(t.scale));
    }
    g.humanoid->setJointBindingPositions(bindT);
    g.humanoid->setJointBindingRotations(bindR);
    g.humanoid->setJointBindingScales(bindS);

    // Skin coordinates: every skinned mesh placement, concatenated.
    std::vector<Vec3> points, normals;
    bool anyNormals = false;
    for (auto &part : g.parts) {
      const ImportMesh &m = s_.meshes[static_cast<std::size_t>(part.mesh)];
      part.offset = points.size();
      const auto &w = weightsFor(part.node, m);
      const auto p = morphed(m.positions, m.morphTargets, w, false);
      points.insert(points.end(), p.begin(), p.end());
      if (m.normals.size() == m.positions.size() && !m.normals.empty()) {
        anyNormals = true;
        const auto n = morphed(m.normals, m.morphTargets, w, true);
        normals.resize(part.offset);
        normals.insert(normals.end(), n.begin(), n.end());
      }
    }
    normals.resize(points.size());
    g.vertexCount = points.size();
    auto coord = std::make_shared<Coordinate>();
    coord->setPoint(toMF(points));
    define(coord, names_.make(g.humanoid->getDEF() + "_skinCoord", "SkinCoord"));
    g.humanoid->setSkinCoord(coord);
    g.skinCoord = coord;
    std::shared_ptr<Normal> normal;
    if (anyNormals) {
      normal = std::make_shared<Normal>();
      normal->setVector(toNormals(normals));
      define(normal, names_.make(g.humanoid->getDEF() + "_skinNormal", "SkinNormal"));
      g.humanoid->setSkinNormal(normal);
      g.skinNormal = normal;
    }

    // Weights per joint.
    std::map<int, std::pair<MFInt32, MFFloat>> weights;
    MFNode skinShapes;
    for (const auto &part : g.parts) {
      const ImportMesh &m = s_.meshes[static_cast<std::size_t>(part.mesh)];
      const ImportSkin &skin =
          s_.skins[static_cast<std::size_t>(s_.nodes[static_cast<std::size_t>(part.node)].skin)];
      if (m.influences.size() != m.positions.size())
        clamped("skinned meshes without per-vertex weights left in the bind pose");
      for (std::size_t v = 0; v < m.influences.size() && v < m.positions.size(); ++v)
        for (const auto &[ji, wt] : m.influences[v]) {
          if (ji < 0 || static_cast<std::size_t>(ji) >= skin.joints.size() || !(wt > 0)) continue;
          auto &e = weights[skin.joints[static_cast<std::size_t>(ji)]];
          e.first.push_back(static_cast<int>(part.offset + v));
          e.second.push_back(wt);
        }
      skinShapes.push_back(buildSkinShape(m, part, coord, normal));
    }
    for (std::size_t k = 0; k < jointList.size(); ++k) {
      auto it = weights.find(jointNodes[k]);
      if (it == weights.end()) continue;
      auto *joint = dynamic_cast<HAnimJoint *>(jointList[k].get());
      joint->setSkinCoordIndexUnchecked(it->second.first);
      joint->setSkinCoordWeight(it->second.second);
    }
    g.humanoid->setSkin(skinShapes);
  }

  NodePtr buildSkinShape(const ImportMesh &m, const SkinPart &part,
                         const std::shared_ptr<Coordinate> &coord,
                         const std::shared_ptr<Normal> &normal) {
    auto shape = std::make_shared<Shape>();
    const bool linear = m.topology != ImportMesh::Topology::Triangles;
    shape->setAppearance(appearance(m.materialIndex, linear).appearance);
    const int off = static_cast<int>(part.offset);
    const std::size_t nv = m.positions.size();
    if (m.topology == ImportMesh::Topology::Triangles) {
      auto ifs = std::make_shared<IndexedFaceSet>();
      MFInt32 ci, li;
      for (std::size_t i = 0; i + 2 < m.indices.size(); i += 3) {
        if (m.indices[i] >= nv || m.indices[i + 1] >= nv || m.indices[i + 2] >= nv) continue;
        for (int k = 0; k < 3; ++k) {
          ci.push_back(off + static_cast<int>(m.indices[i + k]));
          li.push_back(static_cast<int>(m.indices[i + k]));
        }
        ci.push_back(-1);
        li.push_back(-1);
      }
      ifs->setCoordIndexUnchecked(ci);
      ifs->setCoord(coord);
      if (normal && m.normals.size() == nv && nv > 0) {
        ifs->setNormal(normal);
        ifs->setNormalIndexUnchecked(ci);
      }
      if (auto tc = texCoordFor(m, m.materialIndex)) {
        ifs->setTexCoord(tc);
        ifs->setTexCoordIndexUnchecked(li);
      }
      if (auto c = colorNode(m)) {
        ifs->setColor(c);
        ifs->setColorIndexUnchecked(li);
      }
      ifs->setSolidUnchecked(!doubleSided(m.materialIndex));
      ifs->setCcwUnchecked(true);
      ifs->setConvexUnchecked(true);
      shape->setGeometry(ifs);
    } else if (m.topology == ImportMesh::Topology::Lines) {
      auto ils = std::make_shared<IndexedLineSet>();
      MFInt32 ci;
      for (std::size_t i = 0; i + 1 < m.indices.size(); i += 2) {
        if (m.indices[i] >= nv || m.indices[i + 1] >= nv) continue;
        ci.push_back(off + static_cast<int>(m.indices[i]));
        ci.push_back(off + static_cast<int>(m.indices[i + 1]));
        ci.push_back(-1);
      }
      ils->setCoordIndexUnchecked(ci);
      ils->setCoord(coord);
      shape->setGeometry(ils);
    } else {
      clamped("skinned point meshes emitted in the bind pose only");
      auto ps = std::make_shared<PointSet>();
      auto own = std::make_shared<Coordinate>();
      own->setPoint(toMF(m.positions));
      ps->setCoord(own);
      shape->setGeometry(ps);
    }
    define(shape, names_.make(m.name, "SkinMesh" + std::to_string(part.mesh)));
    ++rep_.shapes;
    return shape;
  }

  // ---------------------------------------------------------- animation

  void scanAnimatedWeights() {
    for (const auto &a : s_.animations)
      for (const auto &c : a.channels)
        if (c.path == AnimationChannel::Path::Weights && validNode(c.target))
          animatedWeightNodes_.insert(c.target);
  }

  struct AnimCtx {
    NodePtr timer;
    double duration = 0;
  };

  void route(const NodePtr &from, const char *ff, const NodePtr &to, const char *tf) {
    // SceneBuilder owns its document by value: lend doc_ to it for the
    // checked route and take it back whether or not the route is accepted.
    x3d::authoring::SceneBuilder b(std::move(doc_));
    try {
      b.route(from, ff, to, tf);
      ++rep_.routes;
    } catch (const std::exception &e) {
      rep_.warnings.push_back(std::string("route dropped: ") + e.what());
    }
    doc_ = b.take();
  }


  template <class Interp> std::shared_ptr<Interp> addInterpolator(AnimCtx &ctx, const MFFloat &keys,
                                                                   const std::string &name) {
    auto in = std::make_shared<Interp>();
    in->setKey(keys);
    define(in, names_.make(name, "Interpolator"));
    doc_.scene.addRootNode(in);
    route(ctx.timer, "fraction_changed", in, "set_fraction");
    ++rep_.interpolators;
    return in;
  }

  std::string channelName(const NodePtr &target, const char *what) const {
    return (target && !target->getDEF().empty() ? target->getDEF() : std::string("target")) + "_" + what;
  }

  void animateVec3(AnimCtx &ctx, const Track &tr, const NodePtr &target, const char *field,
                   const char *label) {
    auto in = addInterpolator<PositionInterpolator>(ctx, tr.keys, channelName(target, label));
    MFVec3f kv;
    for (const auto &v : tr.values) kv.push_back(SFVec3f{v[0], v[1], v[2]});
    in->setKeyValue(kv);
    route(in, "value_changed", target, field);
  }

  void animateRotation(AnimCtx &ctx, const Track &tr, const NodePtr &target) {
    auto in = addInterpolator<OrientationInterpolator>(ctx, tr.keys, channelName(target, "rotation"));
    MFRotation kv;
    for (const auto &v : tr.values) kv.push_back(rotationOf({v[0], v[1], v[2], v[3]}));
    in->setKeyValue(kv);
    route(in, "value_changed", target, "rotation");
  }

  void animateScalar(AnimCtx &ctx, const Track &tr, const NodePtr &target, const char *field,
                     float lo, float hi, std::size_t comp = 0, bool invert = false) {
    auto in = addInterpolator<ScalarInterpolator>(ctx, tr.keys, channelName(target, field));
    MFFloat kv;
    bool clampedAny = false;
    for (const auto &v : tr.values) {
      float x = v[comp];
      if (invert) x = 1.0f - x;
      if (x < lo || x > hi) clampedAny = true;
      kv.push_back(std::clamp(x, lo, hi));
    }
    if (clampedAny) clamped("animated values outside their X3D range clamped");
    in->setKeyValue(kv);
    route(in, "value_changed", target, field);
  }

  void animateColor(AnimCtx &ctx, const Track &tr, const NodePtr &target, const char *field) {
    auto in = addInterpolator<ColorInterpolator>(ctx, tr.keys, channelName(target, field));
    MFColor kv;
    for (const auto &v : tr.values) kv.push_back(color3({v[0], v[1], v[2]}));
    in->setKeyValueUnchecked(kv);
    route(in, "value_changed", target, field);
  }

  /// Morph weights -> CoordinateInterpolator (and NormalInterpolator) on each
  /// shape the node places, or on its humanoid's skinCoord.
  void animateWeights(AnimCtx &ctx, const AnimationChannel &ch) {
    const int node = ch.target;
    const ImportNode &n = s_.nodes[static_cast<std::size_t>(node)];
    std::size_t targets = 0;
    for (int m : n.meshIndices)
      if (m >= 0 && static_cast<std::size_t>(m) < s_.meshes.size())
        targets = std::max(targets, s_.meshes[static_cast<std::size_t>(m)].morphTargets.size());
    if (targets == 0) {
      clamped("weight channels on nodes without morph targets dropped");
      return;
    }
    const Track tr = buildTrack(ch, targets, ctx.duration, false, false, o_.samplesPerInterval);
    if (tr.keys.empty()) return;
    auto keyWeights = [&](std::size_t k) { return tr.values[k]; };

    if (skinnedHere(node)) {
      SkinGroup &g = groups_[static_cast<std::size_t>(
          skinGroupOf_[static_cast<std::size_t>(n.skin)])];
      if (!g.skinCoord) return;
      auto ci = addInterpolator<CoordinateInterpolator>(ctx, tr.keys, channelName(g.skinCoord, "morph"));
      auto *base = dynamic_cast<Coordinate *>(g.skinCoord.get());
      MFVec3f kv;
      MFVec3f nkv;
      auto *baseN = g.skinNormal ? dynamic_cast<Normal *>(g.skinNormal.get()) : nullptr;
      for (std::size_t k = 0; k < tr.keys.size(); ++k) {
        MFVec3f pts = base->getPoint();
        MFVec3f nrm = baseN ? baseN->getVector() : MFVec3f{};
        for (const auto &part : g.parts) {
          if (part.node != node) continue;
          const ImportMesh &m = s_.meshes[static_cast<std::size_t>(part.mesh)];
          const auto p = morphed(m.positions, m.morphTargets, keyWeights(k), false);
          for (std::size_t i = 0; i < p.size(); ++i) pts[part.offset + i] = vec3(p[i]);
          if (baseN && m.normals.size() == m.positions.size() && !m.normals.empty()) {
            const auto q = morphed(m.normals, m.morphTargets, keyWeights(k), true);
            for (std::size_t i = 0; i < q.size(); ++i) nrm[part.offset + i] = unitNormal(q[i]);
          }
        }
        kv.insert(kv.end(), pts.begin(), pts.end());
        nkv.insert(nkv.end(), nrm.begin(), nrm.end());
      }
      ci->setKeyValue(kv);
      route(ci, "value_changed", g.skinCoord, "point");
      if (baseN) {
        auto ni = addInterpolator<NormalInterpolator>(ctx, tr.keys, channelName(g.skinNormal, "morph"));
        ni->setKeyValue(nkv);
        route(ni, "value_changed", g.skinNormal, "vector");
      }
      return;
    }

    for (const auto &[meshIndex, coord, normal] : morphShapes_[node]) {
      const ImportMesh &m = s_.meshes[static_cast<std::size_t>(meshIndex)];
      auto ci = addInterpolator<CoordinateInterpolator>(ctx, tr.keys, channelName(coord, "morph"));
      MFVec3f kv, nkv;
      for (std::size_t k = 0; k < tr.keys.size(); ++k) {
        const auto p = toMF(morphed(m.positions, m.morphTargets, keyWeights(k), false));
        kv.insert(kv.end(), p.begin(), p.end());
        if (normal) {
          const auto q = toNormals(morphed(m.normals, m.morphTargets, keyWeights(k), true));
          nkv.insert(nkv.end(), q.begin(), q.end());
        }
      }
      ci->setKeyValue(kv);
      route(ci, "value_changed", coord, "point");
      if (normal) {
        auto ni = addInterpolator<NormalInterpolator>(ctx, tr.keys, channelName(normal, "morph"));
        ni->setKeyValue(nkv);
        route(ni, "value_changed", normal, "vector");
      }
    }
  }

  std::vector<NodePtr> materialTargets(int materialIndex) {
    std::vector<NodePtr> out;
    for (const auto &[key, m] : materials_)
      if (key.first == materialIndex) out.push_back(m.material);
    return out;
  }

  void emitChannel(AnimCtx &ctx, const AnimationChannel &ch) {
    using P = AnimationChannel::Path;
    const int steps = o_.samplesPerInterval;
    auto nodeTarget = [&]() -> NodePtr {
      return validNode(ch.target) ? nodeOut_[static_cast<std::size_t>(ch.target)] : nullptr;
    };
    switch (ch.path) {
      case P::Translation:
      case P::Scale: {
        NodePtr t = nodeTarget();
        if (!t) break;
        const Track tr = buildTrack(ch, 3, ctx.duration, false, false, steps);
        if (!tr.keys.empty())
          animateVec3(ctx, tr, t, ch.path == P::Translation ? "translation" : "scale",
                      ch.path == P::Translation ? "translation" : "scale");
        return;
      }
      case P::Rotation: {
        NodePtr t = nodeTarget();
        if (!t) break;
        const Track tr = buildTrack(ch, 4, ctx.duration, true, false, steps);
        if (!tr.keys.empty()) animateRotation(ctx, tr, t);
        return;
      }
      case P::Weights:
        if (!validNode(ch.target)) break;
        animateWeights(ctx, ch);
        return;
      case P::MaterialBaseColor:
      case P::MaterialEmissive: {
        const auto targets = materialTargets(ch.target);
        if (targets.empty()) break;
        const bool base = ch.path == P::MaterialBaseColor;
        const std::size_t comp = base ? 4 : 3;
        const Track tr = buildTrack(ch, comp, ctx.duration, false, true, steps);
        if (tr.keys.empty()) return;
        bool alphaMoves = false;
        if (base)
          for (const auto &v : tr.values) alphaMoves = alphaMoves || v[3] != tr.values[0][3];
        for (const NodePtr &mat : targets) {
          const bool unlit = dynamic_cast<UnlitMaterial *>(mat.get()) != nullptr;
          const bool phong = dynamic_cast<Material *>(mat.get()) != nullptr;
          if (base) {
            animateColor(ctx, tr, mat, unlit ? "emissiveColor" : phong ? "diffuseColor" : "baseColor");
            if (alphaMoves) animateScalar(ctx, tr, mat, "transparency", 0, 1, 3, true);
          } else if (!unlit) {
            animateColor(ctx, tr, mat, "emissiveColor");
          }
        }
        return;
      }
      case P::MaterialMetallic:
      case P::MaterialRoughness: {
        bool any = false;
        const Track tr = buildTrack(ch, 1, ctx.duration, false, false, steps);
        for (const NodePtr &mat : materialTargets(ch.target))
          if (dynamic_cast<PhysicalMaterial *>(mat.get())) {
            animateScalar(ctx, tr, mat, ch.path == P::MaterialMetallic ? "metallic" : "roughness", 0, 1);
            any = true;
          }
        if (any) return;
        break;
      }
      case P::MaterialAlphaCutoff: {
        const Track tr = buildTrack(ch, 1, ctx.duration, false, false, steps);
        bool any = false;
        for (const auto &[key, m] : materials_)
          if (key.first == ch.target) {
            animateScalar(ctx, tr, m.appearance, "alphaCutoff", 0, 1);
            any = true;
          }
        if (any) return;
        break;
      }
      case P::LightColor:
      case P::LightIntensity:
      case P::LightRange:
      case P::LightInnerCone:
      case P::LightOuterCone: {
        auto it = lightNodes_.find(ch.target);
        if (it == lightNodes_.end()) break;
        const bool color = ch.path == P::LightColor;
        const Track tr = buildTrack(ch, color ? 3 : 1, ctx.duration, false, color, steps);
        if (tr.keys.empty()) return;
        bool any = false;
        for (const NodePtr &l : it->second) {
          const bool spot = dynamic_cast<SpotLight *>(l.get()) != nullptr;
          const bool directional = dynamic_cast<DirectionalLight *>(l.get()) != nullptr;
          switch (ch.path) {
            case P::LightColor: animateColor(ctx, tr, l, "color"); any = true; break;
            case P::LightIntensity: animateScalar(ctx, tr, l, "intensity", 0, 1); any = true; break;
            case P::LightRange:
              if (!directional) {
                animateScalar(ctx, tr, l, "radius", 0, std::numeric_limits<float>::max());
                any = true;
              }
              break;
            case P::LightInnerCone:
              if (spot) { animateScalar(ctx, tr, l, "beamWidth", 1e-4f, kPi / 2); any = true; }
              break;
            case P::LightOuterCone:
              if (spot) { animateScalar(ctx, tr, l, "cutOffAngle", 1e-4f, kPi / 2); any = true; }
              break;
            default: break;
          }
        }
        if (any) return;
        break;
      }
      case P::CameraYfov: {
        auto it = cameraNodes_.find(ch.target);
        if (it == cameraNodes_.end()) break;
        const Track tr = buildTrack(ch, 1, ctx.duration, false, false, steps);
        bool any = false;
        for (const NodePtr &v : it->second)
          if (dynamic_cast<Viewpoint *>(v.get())) {
            animateScalar(ctx, tr, v, "fieldOfView", 1e-4f, kPi - 1e-4f);
            any = true;
          }
        if (any) return;
        break;
      }
    }
    clamped("animation channels with no X3D target dropped");
  }

  void emitAnimations() {
    for (std::size_t a = 0; a < s_.animations.size(); ++a) {
      const ImportAnimation &anim = s_.animations[a];
      double duration = 0;
      for (const auto &c : anim.channels)
        if (!c.times.empty()) duration = std::max(duration, static_cast<double>(c.times.back()));
      if (!(duration > 0)) {
        // A single-key animation is a pose; hold it over a nominal second.
        bool any = false;
        for (const auto &c : anim.channels) any = any || !c.times.empty();
        if (!any) continue;
        duration = 1;
      }
      auto ts = std::make_shared<TimeSensor>();
      ts->setCycleInterval(duration);
      ts->setLoop(true);
      const bool enabled = o_.autoplay == Autoplay::All || (o_.autoplay == Autoplay::First && a == 0);
      ts->setEnabled(enabled);
      if (!anim.name.empty()) ts->X3DTimeDependentNode::setDescription(anim.name);
      define(ts, names_.make(anim.name, "Animation" + std::to_string(a)));
      doc_.scene.addRootNode(ts);
      ++rep_.timeSensors;
      AnimCtx ctx{ts, duration};
      for (const auto &c : anim.channels) emitChannel(ctx, c);
    }
  }
};

} // namespace

x3d::runtime::X3DDocument emitDocument(const ImportScene &scene, const EmitOptions &opts,
                                       EmitReport *report) {
  EmitReport local;
  EmitReport &rep = report ? *report : local;
  Emitter e(scene, opts, rep);
  return e.run();
}

} // namespace x3d::runtime::importer
