#include "emit.hpp"

#include "x3d/nodes/Appearance.hpp"
#include "x3d/nodes/ColorRGBA.hpp"
#include "x3d/nodes/Coordinate.hpp"
#include "x3d/nodes/DirectionalLight.hpp"
#include "x3d/nodes/ImageTexture.hpp"
#include "x3d/nodes/IndexedTriangleSet.hpp"
#include "x3d/nodes/Material.hpp"
#include "x3d/nodes/Normal.hpp"
#include "x3d/nodes/PhysicalMaterial.hpp"
#include "x3d/nodes/PointLight.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/SpotLight.hpp"
#include "x3d/nodes/TextureCoordinate.hpp"
#include "x3d/nodes/Transform.hpp"
#include "x3d/nodes/Viewpoint.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace x3d::asset_import {

namespace {
using namespace x3d::core;
using namespace x3d::nodes;

// Translation/rotation/scale decomposed from an ImportNode::localTransform.
struct Trs {
  SFVec3f translation{0, 0, 0};
  SFVec3f scale{1, 1, 1};
  SFRotation rotation{0, 0, 1, 0};
};

// Decomposes a column-major Mat4 into translation, scale, and a pure
// rotation (as axis-angle). Basis columns are normalized by their lengths
// (the scale) before quaternion extraction; a near-zero-length basis column
// falls back to identity for that axis to avoid divide-by-zero.
Trs decomposeTRS(const Mat4& mat) {
  const auto& m = mat.m;
  Trs trs;
  trs.translation = SFVec3f{m[12], m[13], m[14]};

  Vec3 col0{m[0], m[1], m[2]};
  Vec3 col1{m[4], m[5], m[6]};
  Vec3 col2{m[8], m[9], m[10]};

  auto length = [](const Vec3& v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
  };

  const float sx = length(col0);
  const float sy = length(col1);
  const float sz = length(col2);
  trs.scale = SFVec3f{sx, sy, sz};

  constexpr float kEps = 1e-8f;
  auto normalized = [&](const Vec3& v, float len, Vec3 fallback) {
    if (len < kEps) return fallback;
    return Vec3{v.x / len, v.y / len, v.z / len};
  };
  const Vec3 r0 = normalized(col0, sx, Vec3{1, 0, 0});
  const Vec3 r1 = normalized(col1, sy, Vec3{0, 1, 0});
  const Vec3 r2 = normalized(col2, sz, Vec3{0, 0, 1});

  // 3x3 rotation matrix (columns r0, r1, r2) -> quaternion (trace method).
  const float trace = r0.x + r1.y + r2.z;
  float qw, qx, qy, qz;
  if (trace > 0.0f) {
    const float s = std::sqrt(trace + 1.0f) * 2.0f;  // s = 4*qw
    qw = 0.25f * s;
    qx = (r1.z - r2.y) / s;
    qy = (r2.x - r0.z) / s;
    qz = (r0.y - r1.x) / s;
  } else if (r0.x > r1.y && r0.x > r2.z) {
    const float s = std::sqrt(1.0f + r0.x - r1.y - r2.z) * 2.0f;  // s = 4*qx
    qw = (r1.z - r2.y) / s;
    qx = 0.25f * s;
    qy = (r1.x + r0.y) / s;
    qz = (r2.x + r0.z) / s;
  } else if (r1.y > r2.z) {
    const float s = std::sqrt(1.0f + r1.y - r0.x - r2.z) * 2.0f;  // s = 4*qy
    qw = (r2.x - r0.z) / s;
    qx = (r1.x + r0.y) / s;
    qy = 0.25f * s;
    qz = (r2.y + r1.z) / s;
  } else {
    const float s = std::sqrt(1.0f + r2.z - r0.x - r1.y) * 2.0f;  // s = 4*qz
    qw = (r0.y - r1.x) / s;
    qx = (r2.x + r0.z) / s;
    qy = (r2.y + r1.z) / s;
    qz = 0.25f * s;
  }

  const float angle = 2.0f * std::acos(std::clamp(qw, -1.0f, 1.0f));
  const float s = std::sqrt(std::max(0.0f, 1.0f - qw * qw));
  trs.rotation = (s < 1e-6f) ? SFRotation{0, 0, 1, 0}
                             : SFRotation{qx / s, qy / s, qz / s, angle};
  return trs;
}

// The checked X3D setters throw std::out_of_range outside [0,1]; the IR
// carries values from arbitrary importers (assimp shininess in particular
// runs ~0..1000), so every component gets clamped before it reaches a
// setter.
float clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }
float clamp11(float v) { return std::clamp(v, -1.0f, 1.0f); }
SFVec3f clampNormal(const Vec3& n) {
  float len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
  if (len < 1e-6f) return SFVec3f{0.0f, 0.0f, 1.0f};
  float invLen = 1.0f / len;
  return SFVec3f{clamp11(n.x * invLen), clamp11(n.y * invLen), clamp11(n.z * invLen)};
}
SFColor clampColor(const Vec3& c) {
  return SFColor{clamp01(c.x), clamp01(c.y), clamp01(c.z)};
}

// Builds an ImageTexture for one resolved TextureRef, or nullptr if the plan
// has no URL for it (unresolved/failed decode). ImageTexture.url carries the
// single resolved relative path under assets/.
SFNode buildImageTexture(const TexturePlan& plan, const TextureRef& ref) {
  const std::string url = urlForRef(plan, ref);
  if (url.empty()) return nullptr;
  auto tex = std::make_shared<ImageTexture>();
  tex->setUrl(MFString{url});
  tex->setRepeatSUnchecked(true);
  tex->setRepeatTUnchecked(true);
  return tex;
}

// Wires a Phong (Material) material's TextureSlots into the matching setters,
// looking URLs up in the plan. Slots with no plan URL are skipped.
void wireTextures(const ImportMaterial& mat, const TexturePlan& plan,
                   Material& material) {
  if (mat.textures.baseColor) {
    if (auto t = buildImageTexture(plan, *mat.textures.baseColor))
      material.setDiffuseTexture(t);
  }
  if (mat.textures.normal) {
    if (auto t = buildImageTexture(plan, *mat.textures.normal))
      material.setNormalTexture(t);
  }
  if (mat.textures.emissive) {
    if (auto t = buildImageTexture(plan, *mat.textures.emissive))
      material.setEmissiveTexture(t);
  }
  if (mat.textures.occlusion) {
    if (auto t = buildImageTexture(plan, *mat.textures.occlusion))
      material.setOcclusionTexture(t);
  }
  if (mat.textures.specular) {
    if (auto t = buildImageTexture(plan, *mat.textures.specular))
      material.setSpecularTexture(t);
  }
}

// PBR variant: baseColor -> baseTexture, metallicRoughness -> its slot.
void wireTexturesPbr(const ImportMaterial& mat, const TexturePlan& plan,
                      PhysicalMaterial& material) {
  if (mat.textures.baseColor) {
    if (auto t = buildImageTexture(plan, *mat.textures.baseColor))
      material.setBaseTexture(t);
  }
  if (mat.textures.normal) {
    if (auto t = buildImageTexture(plan, *mat.textures.normal))
      material.setNormalTexture(t);
  }
  if (mat.textures.emissive) {
    if (auto t = buildImageTexture(plan, *mat.textures.emissive))
      material.setEmissiveTexture(t);
  }
  if (mat.textures.occlusion) {
    if (auto t = buildImageTexture(plan, *mat.textures.occlusion))
      material.setOcclusionTexture(t);
  }
  if (mat.textures.metallicRoughness) {
    if (auto t = buildImageTexture(plan, *mat.textures.metallicRoughness))
      material.setMetallicRoughnessTexture(t);
  }
}

// Builds the Appearance for one ImportMaterial: PhysicalMaterial (X3D 4.0
// PBR) when the material carries `pbr`, else Material (Phong). When a
// TexturePlan is supplied, the material's TextureSlots are resolved to
// ImageTexture URLs and assigned to the matching slot setters.
SFNode buildAppearance(const ImportMaterial& mat, const EmitOptions& opts) {
  auto appearance = std::make_shared<Appearance>();

  if (mat.pbr) {
    const PbrParams& pbr = *mat.pbr;
    auto material = std::make_shared<PhysicalMaterial>();
    material->setBaseColor(
        SFColor{clamp01(pbr.baseColor.x), clamp01(pbr.baseColor.y), clamp01(pbr.baseColor.z)});
    material->setMetallic(clamp01(pbr.metallic));
    material->setRoughness(clamp01(pbr.roughness));
    material->setEmissiveColor(clampColor(mat.emissive));
    material->setTransparency(clamp01(1.0f - pbr.baseColor.w));
    if (opts.textures) wireTexturesPbr(mat, *opts.textures, *material);
    appearance->setMaterial(material);
  } else {
    auto material = std::make_shared<Material>();
    material->setDiffuseColor(clampColor(mat.diffuse));
    material->setSpecularColor(clampColor(mat.specular));
    material->setEmissiveColor(clampColor(mat.emissive));
    material->setShininess(clamp01(mat.shininess / 128.0f));
    material->setTransparency(clamp01(1.0f - mat.opacity));
    if (opts.textures) wireTextures(mat, *opts.textures, *material);
    appearance->setMaterial(material);
  }

  return appearance;
}

// Builds a Shape (IndexedTriangleSet + Coordinate, plus Normal/TexCoord/Color
// when the mesh carries them) from one ImportMesh, plus its Appearance
// (Material/PhysicalMaterial) resolved via ImportMesh::materialIndex.
std::shared_ptr<Shape> buildShape(const ImportMesh& m, const ImportScene& scene,
                                   const EmitOptions& opts) {
  auto its = std::make_shared<IndexedTriangleSet>();

  MFInt32 idx(m.indices.begin(), m.indices.end());
  its->setIndexUnchecked(idx);

  auto coord = std::make_shared<Coordinate>();
  MFVec3f pts;
  pts.reserve(m.positions.size());
  for (const auto& p : m.positions) pts.push_back(SFVec3f{p.x, p.y, p.z});
  coord->setPoint(pts);
  its->setCoord(coord);

  if (!m.normals.empty()) {
    auto normal = std::make_shared<Normal>();
    MFVec3f nv;
    nv.reserve(m.normals.size());
    for (const auto& n : m.normals) nv.push_back(clampNormal(n));
    normal->setVector(nv);
    its->setNormal(normal);
  }

  if (opts.includeTextures && !m.uv.empty()) {
    auto texCoord = std::make_shared<TextureCoordinate>();
    MFVec2f uv;
    uv.reserve(m.uv.size());
    for (const auto& t : m.uv) uv.push_back(SFVec2f{t.x, t.y});
    texCoord->setPoint(uv);
    its->setTexCoord(texCoord);
  }

  if (!m.colors.empty()) {
    auto color = std::make_shared<ColorRGBA>();
    MFColorRGBA cv;
    cv.reserve(m.colors.size());
    for (const auto& c : m.colors) cv.push_back(SFColorRGBA{c.x, c.y, c.z, c.w});
    color->setColor(cv);
    its->setColor(color);
  }

  its->setSolidUnchecked(false);
  its->setCcwUnchecked(true);

  auto shape = std::make_shared<Shape>();
  shape->setGeometry(its);

  if (m.materialIndex >= 0 &&
      static_cast<std::size_t>(m.materialIndex) < scene.materials.size()) {
    shape->setAppearance(buildAppearance(scene.materials[m.materialIndex], opts));
  }

  return shape;
}

// Returns the Shape for mesh `meshIndex`, building it (and DEF'ing it as
// "Mesh<index>") on first use; later calls for the same index return the
// same shared_ptr so the writer emits USE instead of a duplicate DEF.
SFNode getOrBuildMeshShape(int meshIndex, const ImportScene& scene, const EmitOptions& opts,
                            std::unordered_map<int, SFNode>& meshCache) {
  auto it = meshCache.find(meshIndex);
  if (it != meshCache.end()) return it->second;

  auto shape = buildShape(scene.meshes[meshIndex], scene, opts);
  shape->setDEF("Mesh" + std::to_string(meshIndex));
  meshCache.emplace(meshIndex, shape);
  return shape;
}

// Builds one X3D light node from an ImportLight. The IR already carries
// world-space direction/position; we clamp intensity to [0,1] (the X3D
// light intensity range) before the range-checked setters.
SFNode buildLight(const ImportLight& l) {
  switch (l.kind) {
    case ImportLight::Kind::Dir: {
      auto light = std::make_shared<DirectionalLight>();
      light->setDirection(SFVec3f{l.direction.x, l.direction.y, l.direction.z});
      light->setColor(clampColor(l.color));
      light->setIntensity(clamp01(l.intensity));
      light->setGlobal(true);
      return light;
    }
    case ImportLight::Kind::Point: {
      auto light = std::make_shared<PointLight>();
      light->setLocation(SFVec3f{l.position.x, l.position.y, l.position.z});
      light->setColor(clampColor(l.color));
      light->setIntensity(clamp01(l.intensity));
      light->setAttenuationUnchecked(
          SFVec3f{l.attenuation.x, l.attenuation.y, l.attenuation.z});
      light->setRadiusUnchecked(l.radius);
      light->setGlobal(true);
      return light;
    }
    case ImportLight::Kind::Spot: {
      auto light = std::make_shared<SpotLight>();
      light->setLocation(SFVec3f{l.position.x, l.position.y, l.position.z});
      light->setDirection(SFVec3f{l.direction.x, l.direction.y, l.direction.z});
      light->setColor(clampColor(l.color));
      light->setIntensity(clamp01(l.intensity));
      light->setAttenuationUnchecked(
          SFVec3f{l.attenuation.x, l.attenuation.y, l.attenuation.z});
      light->setRadiusUnchecked(l.radius);
      light->setCutOffAngleUnchecked(l.cutOffAngle);
      light->setBeamWidthUnchecked(l.beamWidth);
      light->setGlobal(true);
      return light;
    }
  }
  // Unreachable but keeps the compiler happy across all compilers.
  return std::make_shared<DirectionalLight>();
}

// Builds one Viewpoint from an ImportCamera. Position and orientation come
// from the camera's world matrix (column-major): translation = column 3,
// rotation = the rotation part via decomposeTRS.
SFNode buildViewpoint(const ImportCamera& cam) {
  auto vp = std::make_shared<Viewpoint>();
  const Trs trs = decomposeTRS(cam.world);
  vp->setPosition(trs.translation);
  vp->setOrientation(trs.rotation);
  vp->setFieldOfView(cam.yfov);
  vp->setNearDistance(cam.znear);
  vp->setFarDistance(cam.zfar);
  return vp;
}

// Recursively emits one ImportNode (and its childIndices subtree) as a
// Transform: TRS decomposed from the node's localTransform, its own meshes'
// Shapes plus its children's Transforms as the group's children.
SFNode emitNode(int nodeIndex, const ImportScene& scene, const EmitOptions& opts,
                 std::unordered_map<int, SFNode>& meshCache) {
  const ImportNode& node = scene.nodes[nodeIndex];
  auto transform = std::make_shared<Transform>();

  const Trs trs = decomposeTRS(node.localTransform);
  transform->setTranslation(trs.translation);
  transform->setRotation(trs.rotation);
  transform->setScale(trs.scale);

  MFNode children;
  children.reserve(node.meshIndices.size() + node.childIndices.size());
  for (int meshIndex : node.meshIndices) {
    if (meshIndex < 0 || static_cast<std::size_t>(meshIndex) >= scene.meshes.size())
      continue;
    children.push_back(getOrBuildMeshShape(meshIndex, scene, opts, meshCache));
  }
  for (int childIndex : node.childIndices) {
    if (childIndex < 0 || static_cast<std::size_t>(childIndex) >= scene.nodes.size())
      continue;
    children.push_back(emitNode(childIndex, scene, opts, meshCache));
  }
  transform->setChildren(children);

  return transform;
}

} // namespace

x3d::runtime::X3DDocument emit(const ImportScene& scene, const EmitOptions& opts) {
  x3d::runtime::X3DDocument doc;
  doc.version = "4.0";
  doc.profile = x3d::runtime::Profile::Interchange;

  // Cameras and lights are world-space in the IR and live at the scene root;
  // emit them first so they precede the geometry root Transform.
  for (const ImportCamera& cam : scene.cameras) {
    doc.scene.addRootNode(buildViewpoint(cam));
  }
  for (const ImportLight& light : scene.lights) {
    doc.scene.addRootNode(buildLight(light));
  }

  if (scene.rootNode >= 0 &&
      static_cast<std::size_t>(scene.rootNode) < scene.nodes.size()) {
    std::unordered_map<int, SFNode> meshCache;
    doc.scene.addRootNode(emitNode(scene.rootNode, scene, opts, meshCache));
  }

  return doc;
}

} // namespace x3d::asset_import
