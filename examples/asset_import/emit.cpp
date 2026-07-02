#include "emit.hpp"

#include "x3d/nodes/Appearance.hpp"
#include "x3d/nodes/ColorRGBA.hpp"
#include "x3d/nodes/Coordinate.hpp"
#include "x3d/nodes/IndexedTriangleSet.hpp"
#include "x3d/nodes/Material.hpp"
#include "x3d/nodes/Normal.hpp"
#include "x3d/nodes/PhysicalMaterial.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/TextureCoordinate.hpp"
#include "x3d/nodes/Transform.hpp"

#include <algorithm>

namespace x3d::asset_import {

namespace {
using namespace x3d::core;
using namespace x3d::nodes;

// The checked X3D setters throw std::out_of_range outside [0,1]; the IR
// carries values from arbitrary importers (assimp shininess in particular
// runs ~0..1000), so every component gets clamped before it reaches a
// setter.
float clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }
SFColor clampColor(const Vec3& c) {
  return SFColor{clamp01(c.x), clamp01(c.y), clamp01(c.z)};
}

// Builds the Appearance for one ImportMaterial: PhysicalMaterial (X3D 4.0
// PBR) when the material carries `pbr`, else Material (Phong).
SFNode buildAppearance(const ImportMaterial& mat) {
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
    appearance->setMaterial(material);
  } else {
    auto material = std::make_shared<Material>();
    material->setDiffuseColor(clampColor(mat.diffuse));
    material->setSpecularColor(clampColor(mat.specular));
    material->setEmissiveColor(clampColor(mat.emissive));
    material->setShininess(clamp01(mat.shininess / 128.0f));
    material->setTransparency(clamp01(1.0f - mat.opacity));
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
    for (const auto& n : m.normals) nv.push_back(SFVec3f{n.x, n.y, n.z});
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
    shape->setAppearance(buildAppearance(scene.materials[m.materialIndex]));
  }

  return shape;
}

} // namespace

x3d::runtime::X3DDocument emit(const ImportScene& scene, const EmitOptions& opts) {
  x3d::runtime::X3DDocument doc;
  doc.version = "4.0";
  doc.profile = x3d::runtime::Profile::Interchange;

  // Task 5 covers the geometry core: emit the root ImportNode's meshes,
  // wrapped in a Transform. Full hierarchy recursion + TRS + DEF/USE is
  // Task 7.
  if (scene.rootNode >= 0 &&
      static_cast<std::size_t>(scene.rootNode) < scene.nodes.size()) {
    const ImportNode& root = scene.nodes[scene.rootNode];
    auto transform = std::make_shared<Transform>();
    MFNode children;
    children.reserve(root.meshIndices.size());
    for (int meshIndex : root.meshIndices) {
      if (meshIndex < 0 || static_cast<std::size_t>(meshIndex) >= scene.meshes.size())
        continue;
      children.push_back(buildShape(scene.meshes[meshIndex], scene, opts));
    }
    transform->setChildren(children);
    doc.scene.addRootNode(transform);
  }

  return doc;
}

} // namespace x3d::asset_import
