#ifndef X3D_ASSET_IMPORT_IMPORT_SOURCE_HPP
#define X3D_ASSET_IMPORT_IMPORT_SOURCE_HPP
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace x3d::asset_import {

struct Vec2 { float x = 0, y = 0; };
struct Vec3 { float x = 0, y = 0, z = 0; };
struct Vec4 { float x = 0, y = 0, z = 0, w = 0; };
// Column-major 4x4 (backend converts into this convention). Identity default.
struct Mat4 { std::array<float, 16> m{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}; };

enum class AlphaMode { Opaque, Mask, Blend };

struct PbrParams { Vec4 baseColor{1,1,1,1}; float metallic = 1.0f; float roughness = 1.0f; };

// A texture reference: either an external path (relative to the model dir) OR an
// index into ImportScene::embedded. Exactly one is set.
struct TextureRef {
  std::optional<std::string> externalPath;
  std::optional<int> embeddedIndex;
};
struct TextureSlots {
  std::optional<TextureRef> baseColor;   // diffuse / baseColor
  std::optional<TextureRef> normal;
  std::optional<TextureRef> emissive;
  std::optional<TextureRef> occlusion;
  std::optional<TextureRef> metallicRoughness; // PBR
  std::optional<TextureRef> specular;          // Phong
};

struct ImportMesh {
  std::vector<Vec3> positions, normals;
  std::vector<Vec2> uv;
  std::vector<Vec4> colors;
  std::vector<std::uint32_t> indices;   // triangles (multiples of 3)
  int materialIndex = -1;
};
struct ImportMaterial {
  std::string name;
  Vec3 diffuse{0.8f, 0.8f, 0.8f}, emissive{0, 0, 0}, specular{0, 0, 0};
  float shininess = 0.0f;    // assimp SHININESS (0..~1000)
  float opacity = 1.0f;
  AlphaMode alpha = AlphaMode::Opaque;
  std::optional<PbrParams> pbr;
  TextureSlots textures;
};
struct ImportNode {
  std::string name;
  Mat4 localTransform;
  std::vector<int> meshIndices;
  std::vector<int> childIndices;
};
struct ImportCamera { Mat4 world; float yfov = 0.7854f, znear = 0.1f, zfar = 1000.0f; };
struct ImportLight {
  enum class Kind { Dir, Point, Spot } kind = Kind::Dir;
  Vec3 color{1, 1, 1}, position{0, 0, 0}, direction{0, 0, -1};
  float intensity = 1.0f, cutOffAngle = 0.785398f, beamWidth = 1.570796f, radius = 100.0f;
  Vec3 attenuation{1, 0, 0};
};
struct EmbeddedTexture { std::string key, hintExt; std::vector<std::uint8_t> bytes; };

struct ImportScene {
  std::vector<ImportNode> nodes;
  int rootNode = -1;
  std::vector<ImportMesh> meshes;
  std::vector<ImportMaterial> materials;
  std::vector<ImportCamera> cameras;
  std::vector<ImportLight> lights;
  std::vector<EmbeddedTexture> embedded;
};

struct ImportSource {
  virtual ImportScene load(const std::string& path) = 0;
  virtual ~ImportSource() = default;
};

} // namespace x3d::asset_import
#endif
