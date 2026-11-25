// ImportScene.hpp
// The engine-independent intermediate scene every mesh-import backend fills.
//
// A backend (glTF via cgltf, Assimp, USD via tinyusdz) converts its source
// format into these plain values and indices; emitDocument() turns them into
// X3D nodes. No third-party type crosses this boundary, so a backend can be
// swapped without touching the emitter.
//
// Conventions every backend must follow:
//   - Right-handed, Y-up, metres: the X3D convention. Backends convert
//     Z-up or other units before filling the scene.
//   - Mat4 is column-major with column vectors (translation in m[12..14]).
//   - Quaternions are (x, y, z, w).
//   - Texture coordinates use X3D's origin (0,0 = bottom-left of the image).
//     glTF's top-left coordinates are flipped by its backend.
//   - Cameras look down -Z with +Y up in their node's frame; lights shine down
//     -Z. Both sit at their node's origin unless `local` / `position` say
//     otherwise.
#ifndef X3D_RUNTIME_IMPORT_IMPORT_SCENE_HPP
#define X3D_RUNTIME_IMPORT_IMPORT_SCENE_HPP

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace x3d::runtime::importer {

struct Vec2 {
  float x = 0, y = 0;
};
struct Vec3 {
  float x = 0, y = 0, z = 0;
};
struct Vec4 {
  float x = 0, y = 0, z = 0, w = 0;
};
/// Column-major 4x4, identity by default.
struct Mat4 {
  std::array<float, 16> m{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
};

/// A node transform as separate parts: M = T * R * S.
struct Trs {
  Vec3 translation{0, 0, 0};
  Vec4 rotation{0, 0, 0, 1}; ///< unit quaternion (x, y, z, w)
  Vec3 scale{1, 1, 1};
};

enum class AlphaMode { Opaque, Mask, Blend };

/// UsdPreviewSurface opacityMode: Transparent attenuates only the diffuse term;
/// Presence scales the whole lit result (the glTF BLEND equivalent).
enum class OpacityMode { Transparent, Presence };

enum class TextureWrap { Repeat, ClampToEdge, MirroredRepeat };
/// Sampler filters. Default leaves the X3D browser default in place.
enum class TextureFilter {
  Default,
  Nearest,
  Linear,
  NearestMipmapNearest,
  LinearMipmapNearest,
  NearestMipmapLinear,
  LinearMipmapLinear
};

/// A texture image: an external path (relative to the model's directory) or
/// an index into ImportScene::embedded. Exactly one is set.
struct TextureRef {
  std::optional<std::string> externalPath;
  std::optional<int> embeddedIndex;
  /// Index into ImportMesh::uvSets of every mesh using this material. A
  /// backend that applies a texture transform bakes it into a uv set of its
  /// own and points here.
  int uvSet = 0;
  TextureWrap wrapS = TextureWrap::Repeat;
  TextureWrap wrapT = TextureWrap::Repeat;
  TextureFilter minFilter = TextureFilter::Default;
  TextureFilter magFilter = TextureFilter::Default;
};

struct TextureSlots {
  std::optional<TextureRef> baseColor; ///< diffuse / base colour
  std::optional<TextureRef> normal;
  std::optional<TextureRef> emissive;
  std::optional<TextureRef> occlusion;
  std::optional<TextureRef> metallicRoughness; ///< glTF packing: G roughness, B metallic
  std::optional<TextureRef> specular;          ///< Phong specular colour
  std::optional<TextureRef> shininess;         ///< Phong shininess
  std::optional<TextureRef> ambient;           ///< Phong ambient (occlusion-like)
};

struct PbrParams {
  Vec4 baseColor{1, 1, 1, 1};
  float metallic = 1.0f;
  float roughness = 1.0f;
};

struct ImportMaterial {
  std::string name;
  // Phong values (used when `pbr` is empty and `unlit` is false).
  Vec3 diffuse{0.8f, 0.8f, 0.8f};
  Vec3 specular{0, 0, 0};
  Vec3 emissive{0, 0, 0};
  float ambientIntensity = 0.2f;
  /// X3D shininess, already in [0,1] (Assimp's 0..~1000 exponent is scaled by
  /// its backend).
  float shininess = 0.2f;
  float opacity = 1.0f;
  /// Metallic-roughness parameters; when set, the material is PhysicalMaterial.
  std::optional<PbrParams> pbr;
  /// Unlit (KHR_materials_unlit): emitted as UnlitMaterial with the base or
  /// diffuse colour as its emissive colour.
  bool unlit = false;
  AlphaMode alpha = AlphaMode::Opaque;
  float alphaCutoff = 0.5f;
  bool doubleSided = false;
  float normalScale = 1.0f;
  float occlusionStrength = 1.0f;
  TextureSlots textures;

  // UsdPreviewSurface / glTF extension values that the X3D material nodes
  // cannot hold. They reach a renderer only through materialGlsl().
  bool useSpecularWorkflow = false; ///< true: `specular` is F0, metallic ignored
  float ior = 1.5f;
  float clearcoat = 0.0f;
  float clearcoatRoughness = 0.01f;
  float opacityThreshold = 0.0f; ///< >0: alpha-test cutout
  OpacityMode opacityMode = OpacityMode::Transparent;
};

/// One morph target: per-vertex differences from the base mesh.
struct MorphTarget {
  std::vector<Vec3> positions; ///< same length as ImportMesh::positions
  std::vector<Vec3> normals;   ///< empty, or the length of ImportMesh::normals
};

/// One vertex's joint influences: (index into ImportSkin::joints, weight).
using JointInfluences = std::vector<std::pair<int, float>>;

struct ImportMesh {
  enum class Topology { Triangles, Lines, Points };
  std::string name;
  Topology topology = Topology::Triangles;
  std::vector<Vec3> positions;
  std::vector<Vec3> normals;             ///< empty or one per position
  std::vector<std::vector<Vec2>> uvSets; ///< each empty or one per position
  std::vector<Vec4> colors;              ///< empty or one per position
  bool colorsHaveAlpha = false;
  /// Triangles: 3 per face, counter-clockwise front faces. Lines: 2 per
  /// segment. Points: unused (every position is a point).
  std::vector<std::uint32_t> indices;
  int materialIndex = -1;

  std::vector<MorphTarget> morphTargets;
  /// Default morph weights (one per target). The emitted coordinates are the
  /// base plus these weighted targets, unless the placing node overrides them.
  std::vector<float> morphWeights;

  /// Skin influences, empty or one entry per position. Meaningful only on a
  /// node that names a skin.
  std::vector<JointInfluences> influences;
};

struct ImportCamera {
  enum class Kind { Perspective, Orthographic };
  std::string name;
  Kind kind = Kind::Perspective;
  float yfov = 0.785398f; ///< perspective: vertical field of view, radians
  float aspectRatio = 0;  ///< width / height; 0 when the source does not say
  float xmag = 1, ymag = 1; ///< orthographic half extents
  float znear = 0.1f;
  float zfar = 0; ///< 0 = infinite
  /// Camera frame within its node (identity: at the origin looking down -Z).
  Mat4 local;
};

struct ImportLight {
  enum class Kind { Directional, Point, Spot };
  std::string name;
  Kind kind = Kind::Directional;
  Vec3 color{1, 1, 1};
  float intensity = 1.0f;
  float range = 0; ///< 0 = unlimited
  float innerConeAngle = 0;          ///< spot: full-intensity half angle
  float outerConeAngle = 0.785398f;  ///< spot: cut-off half angle
  /// Point and spot falloff (constant, linear, quadratic). glTF lights fall
  /// off with the inverse square.
  Vec3 attenuation{0, 0, 1};
  Vec3 position{0, 0, 0};   ///< within the node
  Vec3 direction{0, 0, -1}; ///< within the node
};

struct EmbeddedTexture {
  std::string key;     ///< a stable name for diagnostics
  std::string hintExt; ///< "png", "jpg", ... (lower case, no dot)
  /// The encoded file. When `width` and `height` are set instead, raw RGBA8
  /// texels with the top row first (Assimp's uncompressed textures).
  std::vector<std::uint8_t> bytes;
  std::uint32_t width = 0, height = 0;
};

struct ImportSkin {
  std::string name;
  std::vector<int> joints;          ///< node indices
  std::vector<Mat4> inverseBind;    ///< one per joint; mesh space -> joint space
  int skeleton = -1;                ///< optional common root node
};

struct ImportNode {
  std::string name;
  Mat4 localTransform;
  /// The same transform as separate parts, when the source stores it that way
  /// (glTF TRS nodes, every animated node). Emit decomposes `localTransform`
  /// when this is empty.
  std::optional<Trs> trs;
  std::vector<int> meshIndices;
  std::vector<int> childIndices;
  int camera = -1; ///< index into ImportScene::cameras
  int light = -1;  ///< index into ImportScene::lights
  int skin = -1;   ///< index into ImportScene::skins; skins every mesh here
  /// Morph weight override for this node's meshes (empty: the mesh defaults).
  std::vector<float> morphWeights;
};

struct AnimationChannel {
  enum class Path {
    Translation,
    Rotation,
    Scale,
    Weights,
    MaterialBaseColor,       ///< VEC4; diffuse for Phong, emissive for unlit
    MaterialEmissive,        ///< VEC3
    MaterialMetallic,        ///< SCALAR
    MaterialRoughness,       ///< SCALAR
    MaterialAlphaCutoff,     ///< SCALAR
    LightColor,              ///< VEC3
    LightIntensity,          ///< SCALAR
    LightRange,              ///< SCALAR
    LightInnerCone,          ///< SCALAR
    LightOuterCone,          ///< SCALAR
    CameraYfov,              ///< SCALAR
  };
  enum class Interpolation { Linear, Step, CubicSpline };
  Path path = Path::Translation;
  Interpolation interpolation = Interpolation::Linear;
  /// Node index for node paths; material, light or camera index for the
  /// property paths.
  int target = -1;
  std::vector<float> times; ///< seconds, strictly increasing
  /// Flattened output values. CubicSpline stores (in-tangent, value,
  /// out-tangent) per key, as glTF does. Weights store one value per morph
  /// target per key.
  std::vector<float> values;
};

struct ImportAnimation {
  std::string name;
  std::vector<AnimationChannel> channels;
};

struct ImportScene {
  std::vector<ImportNode> nodes;
  std::vector<int> roots; ///< top-level nodes of the scene, in order
  std::vector<ImportMesh> meshes;
  std::vector<ImportMaterial> materials;
  std::vector<ImportCamera> cameras;
  std::vector<ImportLight> lights;
  std::vector<EmbeddedTexture> embedded;
  std::vector<ImportSkin> skins;
  std::vector<ImportAnimation> animations;
  /// Things the backend read but could not carry, as one line each.
  std::vector<std::string> warnings;
};

} // namespace x3d::runtime::importer

#endif // X3D_RUNTIME_IMPORT_IMPORT_SCENE_HPP
