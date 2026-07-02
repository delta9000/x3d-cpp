#include "fixture_source.hpp"

#include <stdexcept>

namespace x3d::asset_import {

namespace {

// Appends a quad face (4 verts + 2 CCW triangles) to `mesh`, given a center
// offset and two orthonormal tangent vectors `u`, `w` such that
// cross(u, w) == outward face normal. Vertices are emitted in CCW order as
// seen from outside the cube (from along +normal, looking back at the
// cube), which is the standard "outward-facing" winding.
void addFace(ImportMesh& mesh, Vec3 center, Vec3 u, Vec3 w, Vec3 normal) {
  const auto add = [](Vec3 a, Vec3 b) -> Vec3 { return {a.x + b.x, a.y + b.y, a.z + b.z}; };
  const auto sub = [](Vec3 a, Vec3 b) -> Vec3 { return {a.x - b.x, a.y - b.y, a.z - b.z}; };
  const auto scale = [](Vec3 a, float s) -> Vec3 { return {a.x * s, a.y * s, a.z * s}; };

  const Vec3 v0 = add(center, scale(add(u, w), -1.0f));  // center - u - w
  const Vec3 v1 = sub(add(center, u), w);                // center + u - w
  const Vec3 v2 = add(center, add(u, w));                // center + u + w
  const Vec3 v3 = add(sub(center, u), w);                // center - u + w

  const std::uint32_t base = static_cast<std::uint32_t>(mesh.positions.size());
  mesh.positions.push_back(v0);
  mesh.positions.push_back(v1);
  mesh.positions.push_back(v2);
  mesh.positions.push_back(v3);
  for (int i = 0; i < 4; ++i) mesh.normals.push_back(normal);
  mesh.uv.push_back({0.0f, 0.0f});
  mesh.uv.push_back({1.0f, 0.0f});
  mesh.uv.push_back({1.0f, 1.0f});
  mesh.uv.push_back({0.0f, 1.0f});

  mesh.indices.push_back(base + 0);
  mesh.indices.push_back(base + 1);
  mesh.indices.push_back(base + 2);
  mesh.indices.push_back(base + 0);
  mesh.indices.push_back(base + 2);
  mesh.indices.push_back(base + 3);
}

// Builds a unit cube (extents [-0.5, 0.5]) as a single ImportMesh: 6 faces *
// 4 verts = 24 positions, 6 faces * 2 tris = 12 tris = 36 indices, per-face
// normals, CCW winding, outward-facing.
ImportMesh makeCubeMesh(int materialIndex) {
  ImportMesh mesh;

  // +X face: u=+Y, w=+Z (u x w == +X)
  addFace(mesh, {0.5f, 0, 0}, {0, 0.5f, 0}, {0, 0, 0.5f}, {1, 0, 0});
  // -X face: u=+Z, w=+Y (u x w == -X)
  addFace(mesh, {-0.5f, 0, 0}, {0, 0, 0.5f}, {0, 0.5f, 0}, {-1, 0, 0});
  // +Y face: u=+Z, w=+X (u x w == +Y)
  addFace(mesh, {0, 0.5f, 0}, {0, 0, 0.5f}, {0.5f, 0, 0}, {0, 1, 0});
  // -Y face: u=+X, w=+Z (u x w == -Y)
  addFace(mesh, {0, -0.5f, 0}, {0.5f, 0, 0}, {0, 0, 0.5f}, {0, -1, 0});
  // +Z face: u=+X, w=+Y (u x w == +Z)
  addFace(mesh, {0, 0, 0.5f}, {0.5f, 0, 0}, {0, 0.5f, 0}, {0, 0, 1});
  // -Z face: u=+Y, w=+X (u x w == -Z)
  addFace(mesh, {0, 0, -0.5f}, {0, 0.5f, 0}, {0.5f, 0, 0}, {0, 0, -1});

  mesh.materialIndex = materialIndex;
  return mesh;
}

ImportMaterial makeMaterial(const std::string& name, Vec3 diffuse) {
  ImportMaterial mat;
  mat.name = name;
  mat.diffuse = diffuse;
  return mat;
}

ImportScene makeCubeScene() {
  ImportScene scene;
  scene.materials.push_back(makeMaterial("fixture_cube_material", {0.8f, 0.8f, 0.8f}));
  scene.meshes.push_back(makeCubeMesh(0));

  ImportNode root;
  root.name = "cube_root";
  root.meshIndices = {0};
  scene.nodes.push_back(root);
  scene.rootNode = 0;
  return scene;
}

ImportScene makeHierarchyScene() {
  ImportScene scene;
  scene.materials.push_back(makeMaterial("fixture_parent_material", {0.8f, 0.2f, 0.2f}));
  scene.materials.push_back(makeMaterial("fixture_child_material", {0.2f, 0.2f, 0.8f}));
  scene.meshes.push_back(makeCubeMesh(0));
  scene.meshes.push_back(makeCubeMesh(1));

  ImportNode child;
  child.name = "hierarchy_child";
  child.meshIndices = {1};
  child.localTransform.m = {1, 0, 0, 0,
                             0, 1, 0, 0,
                             0, 0, 1, 0,
                             2, 0, 0, 1};  // translated +2 on X (column-major)
  scene.nodes.push_back(child);  // index 0

  ImportNode parent;
  parent.name = "hierarchy_parent";
  parent.meshIndices = {0};
  parent.childIndices = {0};  // index of `child` above
  scene.nodes.push_back(parent);  // index 1

  scene.rootNode = 1;
  return scene;
}

ImportScene makeLitScene() {
  ImportScene scene = makeCubeScene();

  ImportCamera camera;
  scene.cameras.push_back(camera);

  ImportLight light;
  light.kind = ImportLight::Kind::Dir;
  light.color = {1, 1, 1};
  light.direction = {0, -1, -1};
  light.intensity = 1.0f;
  scene.lights.push_back(light);

  return scene;
}

}  // namespace

ImportScene FixtureSource::load(const std::string& name) {
  if (name == "cube") return makeCubeScene();
  if (name == "hierarchy") return makeHierarchyScene();
  if (name == "lit") return makeLitScene();
  throw std::runtime_error("unknown fixture: " + name);
}

}  // namespace x3d::asset_import
