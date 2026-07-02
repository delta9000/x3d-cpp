#include "emit.hpp"

#include "x3d/nodes/ColorRGBA.hpp"
#include "x3d/nodes/Coordinate.hpp"
#include "x3d/nodes/IndexedTriangleSet.hpp"
#include "x3d/nodes/Normal.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/TextureCoordinate.hpp"
#include "x3d/nodes/Transform.hpp"

namespace x3d::asset_import {

namespace {
using namespace x3d::core;
using namespace x3d::nodes;

// Builds a Shape (IndexedTriangleSet + Coordinate, plus Normal/TexCoord/Color
// when the mesh carries them) from one ImportMesh. Geometry-only: no
// Appearance/material wiring yet (later tasks).
std::shared_ptr<Shape> buildShape(const ImportMesh& m, const EmitOptions& opts) {
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
      children.push_back(buildShape(scene.meshes[meshIndex], opts));
    }
    transform->setChildren(children);
    doc.scene.addRootNode(transform);
  }

  return doc;
}

} // namespace x3d::asset_import
