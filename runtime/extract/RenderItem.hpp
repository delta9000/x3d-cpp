// RenderItem.hpp — M2.5 extraction descriptor layer (Layer A1), pure POD.
//
// namespace x3d::runtime::extract. Header-only, golden-untouched, node-as-truth.
// This file is the renderer-facing CONTRACT of the extraction seam: a set of
// plain-old-data descriptors a consumer (the out-of-SDK PoC renderer, or any
// other renderer) reads. It depends ONLY on X3Dtypes (the SF/MF value types),
// Mat4, Aabb, TextureResolver, and std. It pulls in NO Bounds/Transform/Pick
// system header, so it introduces no cycle and stays a leaf of the dependency
// graph.
//
// DESIGN NOTES (locked by the M2.5 design spec, 2026-06-14):
//   * IDENTITY is per-PATH (RenderItemId/PathKey), while GEOMETRY and MATERIAL
//     are content/node-keyed and legitimately shared across placements (USE).
//   * RenderDelta is THE single authoritative change channel. RenderItem itself
//     carries NO changeBits — there is exactly one encoding of "what changed".
//   * Several fields here are descriptor-only superset surface, present so the
//     contract is forward-stable but NOT exercised by the first PoC. Each such
//     field is flagged `descriptor-only, not exercised by PoC` inline.
//
// v1-CLOSURE ADDITIONS (2026-06-16):
//   * TextureRef::resolvedPixels — the result of the embedder's TextureResolver
//     call, threaded onto the descriptor by TextureExtract.hpp so a consumer
//     can bind without reinventing resolution. Defaults to makeFailed() (= the
//     white-fallback PoC behaviour). Source::Inline textures are always decoded
//     directly from inlinePixels and do NOT go through the resolver; for them
//     resolvedPixels remains makeFailed() and the consumer reads inlinePixels
//     instead.
//   * MeshData::isGlyphMesh — marks a mesh produced by the Text extraction path
//     (TextExtract.hpp). Each quad carries 4 positions (XY plane, Z=0) + 4 atlas
//     texcoords from FontMetrics::GlyphMetrics {u0,v0,u1,v1} + 6 indices (two
//     CCW tris). The associated TextureRef (in MaterialDesc::textures[0]) points
//     to the consumer-managed glyph atlas. The existing MeshData fields suffice
//     for the glyph-quad path; isGlyphMesh is a ROUTING FLAG so consumers can
//     select the text-render shader branch without inspecting mesh topology.
//     isGlyphMesh is false for all non-text meshes (no change to existing paths).
#ifndef X3D_RUNTIME_EXTRACT_RENDERITEM_HPP
#define X3D_RUNTIME_EXTRACT_RENDERITEM_HPP

#include "Aabb.hpp"            // x3d::runtime::Aabb
#include "Mat4.hpp"            // x3d::runtime::Mat4
#include "PackedMesh.hpp"      // PackedMesh (Phase 1 binary geometry)
#include "TextureDesc.hpp"     // TextureDesc (Phase 1 binary texture descriptor)
#include "TextureResolver.hpp" // TexturePixelResult (resolved-pixel seam, T-TEX)
#include "TextureTransform2D.hpp" // ExtendedSamplerParams / TexCoordGenDesc (T-TEX §18.4.8/9)
#include "Topology.hpp"        // Topology enum (moved out of RenderItem.hpp, Phase 1)
#include "x3d/core/X3Dtypes.hpp"        // SFVec2f, SFVec3f, SFColor, SFColorRGBA, SFImage, MF*
#include "X3DFieldValue.hpp"   // X3DFieldValue discriminated union (Phase 3)
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace x3d::nodes { class X3DNode; } // forward decl; descriptors key side-table identity by raw ptr.

namespace x3d::runtime::extract {

using namespace x3d::core;
using x3d::nodes::X3DNode;

// ---------------------------------------------------------------------------
// PathKey — the per-PATH identity of a renderable placement.
//
// A PathKey is the full root..leaf chain of node pointers leading to a
// geometry-bearing Shape. Identity is the ENTIRE vector compared element-wise:
// two PathKeys are equal iff they are the same length and every node pointer
// matches. The hash below is a hashmap BUCKET key ONLY — it is intentionally
// lossy and MUST NEVER be used as identity (a hash collision must fall through
// to full-vector equality, never alias two distinct placements). This closes
// the hash-collision-aliasing flaw from the design review.
// ---------------------------------------------------------------------------
using PathKey = std::vector<const X3DNode *>;

// Full-vector equality functor for use as an unordered_map KeyEqual.
struct PathKeyEqual {
  bool operator()(const PathKey &a, const PathKey &b) const {
    return a == b; // std::vector::operator== is element-wise, length-checked.
  }
};

// Bucket-only hash. NOT identity — combines the pointer chain into a bucket
// index; collisions are resolved by PathKeyEqual.
struct PathKeyHash {
  std::size_t operator()(const PathKey &p) const {
    std::size_t h = 1469598103934665603ull; // FNV-ish seed
    for (const X3DNode *n : p) {
      std::size_t x = std::hash<const X3DNode *>{}(n);
      h ^= x + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
    }
    return h;
  }
};

// ---------------------------------------------------------------------------
// RenderItemId — a dense uint32 handle into the extractor's path-interning
// table. Stable across frames while topology is stable. NOT a raw hash and NOT
// a pointer: it is an opaque dense index the consumer can use as an array key.
// ---------------------------------------------------------------------------
using RenderItemId = std::uint32_t;
inline constexpr RenderItemId kInvalidRenderItemId = 0xFFFFFFFFu;

// ---------------------------------------------------------------------------
// GeomId — content identity of a mesh. Two placements with an equal GeomId
// reference identical geometry content (upload-once / instance-N). contentVersion
// is bumped by the extractor when a geometry node's CONTENT field changes.
// ---------------------------------------------------------------------------
struct GeomId {
  const X3DNode *node = nullptr;
  std::uint32_t contentVersion = 0;

  bool operator==(const GeomId &o) const {
    return node == o.node && contentVersion == o.contentVersion;
  }
  bool operator!=(const GeomId &o) const { return !(*this == o); }
};

// Bucket-only hash for GeomId (consumer GpuMesh caches key on this).
struct GeomIdHash {
  std::size_t operator()(const GeomId &g) const {
    std::size_t h = std::hash<const X3DNode *>{}(g.node);
    h ^= static_cast<std::size_t>(g.contentVersion) + 0x9e3779b97f4a7c15ull +
         (h << 6) + (h >> 2);
    return h;
  }
};

// ---------------------------------------------------------------------------
// MeshData — extracted triangle geometry in the geometry node's LOCAL frame.
//
//   * positions/normals are LOCAL-frame — NEVER world-baked. The per-path world
//     transform lives on the RenderItem side (worldTransform), so one mesh is
//     uploaded once and instanced across placements.
//   * indices are ALWAYS populated. When a type is natively indexed they are the
//     source indices; when flat/per-face normals or *PerVertex=false break vertex
//     sharing the builder EXPANDS to non-indexed and writes a trivial 0..N-1
//     index run. A consumer can therefore always issue an indexed draw.
//   * texcoords use the X3D LOCAL origin, which is bottom-left = the GL
//     convention. There is NO V-flip here and a consumer must NOT reflexively
//     flip V (doing so double-flips and breaks textures).
// ---------------------------------------------------------------------------
// Primitive topology the `indices` run describes (B4). Default Triangles keeps
// every pre-B4 mesh byte-for-byte the same. Lines = consecutive index PAIRS are
// independent segments (GL_LINES); Points = each index is one vertex (GL_POINTS).
//
// EXPLICIT CONSUMER CONTRACT (do NOT overload topology to mean unlit): the
// SHADING-PATH selector a consumer must honor is
//     topology != Triangles  OR  !hasNormals  =>  bind the UNLIT program,
//                                                  skip the normal-matrix and
//                                                  light uniforms, and disable
//                                                  GL_CULL_FACE.
// Lines and points are ALWAYS unlit, colored from a per-vertex Color (when
// hasColors) else the material baseColor. The producer additionally sets
// solid=false on every line/point mesh so a consumer's existing cull-disable
// path (solid=false => double-sided, no GL_CULL_FACE) covers them with no extra
// branch; the topology check above is the belt-and-braces guard.
// Topology enum is defined in Topology.hpp (included above).

struct MeshData {
  std::vector<SFVec3f> positions;
  std::vector<std::uint32_t> indices; // always populated (0..N-1 when expanded).
  std::vector<SFVec3f> normals;
  std::vector<SFVec2f> texcoords;
  std::vector<std::vector<SFVec2f>> texcoordSets;
  std::vector<SFColorRGBA> colors;

  // LATTICE-INDEX-RETAINING form (B5/B6). For lattice-derived geometry
  // (ElevationGrid, GeoElevationGrid, and B3 Extrusion) every EXPANDED corner in
  // `positions` carries the id of the UNIQUE source lattice/control vertex it was
  // synthesized from (e.g. row*xDim+col for a height grid). This is what lets B6
  // re-derive face adjacency in SOURCE space and weld creaseAngle-smooth normals
  // WITHOUT re-indexing the expanded buffer (coincident-distinct verts stay
  // distinct). Parallel to `positions` when present; EMPTY for geometry whose
  // corners have no shared-lattice identity (composed sets, analytic primitives).
  std::vector<std::uint32_t> latticeIndex;

  Topology topology = Topology::Triangles; // B4: Triangles (default) / Lines / Points.
  bool ccw = true;        // winding: X3D default true (CCW front).
  bool solid = true;      // solid=true => back-face cullable.
  bool hasNormals = false;
  bool hasColors = false; // per-vertex Color present => OVERRIDES Material diffuse.

  // T-TEXT (v1-closure): set to true for meshes produced by the Text extraction
  // path (TextExtract.hpp). Each glyph is a quad: 4 positions in the XY plane
  // (Z=0, in the Text node's local frame), 4 atlas texcoords from
  // GlyphMetrics::{u0,v0,u1,v1}, 6 indices (two CCW triangles). The associated
  // glyph atlas is referenced via MaterialDesc::textures[0]. False for ALL
  // non-text geometry; existing consumers must check this flag before binding
  // the text-render shader branch.
  bool isGlyphMesh = false;
};

// ---------------------------------------------------------------------------
// Geometry — union of AoS (array-of-structs MeshData) and Packed (binary slab
// PackedMesh). Default is AoS to keep all pre-Phase-1 code unchanged.
// Phase 1 adds the Packed variant for embedder-supplied binary geometry.
// ---------------------------------------------------------------------------
struct Geometry {
  enum class Kind { AoS, Packed };
  Kind kind = Kind::AoS;
  MeshData aos;      // populated when kind == AoS (all pre-Phase-1 paths).
  PackedMesh packed; // populated when kind == Packed (binary geometry resolver).
  bool is_packed() const { return kind == Kind::Packed; }
};

// ---------------------------------------------------------------------------
// Texture descriptor. URLs are surfaced VERBATIM; asset resolution (loading the
// bytes) is OUTSIDE the SDK. descriptor-only for the first PoC (textures are M4).
// ---------------------------------------------------------------------------
struct SamplerParams {
  bool repeatS = true;
  bool repeatT = true;
  bool generateMipmaps = true;
  float anisotropicDegree = 1.0f;
  // descriptor-only, not exercised by PoC.
};

struct TextureRef {
  // Slot identifies the semantic role of a texture within a material.
  //
  // COLOR SPACE CONVENTION (ORM channel-packing, MAT-008):
  //   Slot          | Expected color space  | Notes
  //   --------------|----------------------|--------------------------------------
  //   BaseColor     | sRGB                 | Gamma-encoded; GPU samples & converts
  //   Emissive      | sRGB                 | Gamma-encoded
  //   Diffuse       | sRGB                 | Phong diffuse (sRGB)
  //   Normal        | Linear               | Tangent-space normal map (RGB linear)
  //   Occlusion     | Linear               | R channel only; G/B ignored
  //   MetallicRoughness | Linear           | ORM pack: R=occlusion, G=roughness,
  //                                        |   B=metallic (glTF §3.9.4 / X3D §17)
  //                                        |   One slot for the packed ORM texture;
  //                                        |   NOT split into separate R/G/B slots.
  //   Specular      | sRGB                 | Phong specular
  //   Shininess     | Linear               | Alpha modulates shininess — MAT-002
  //   Ambient       | Linear               | RGB modulates ambient     — MAT-003
  //
  // Consumers MUST treat MetallicRoughness as a single packed ORM texture and
  // NOT request separate Metallic / Roughness / Occlusion textures from the same
  // image. The SDK emits exactly one MetallicRoughness TextureRef per packed map.
  enum class Slot {
    BaseColor,
    Normal,
    Emissive,
    Occlusion,
    MetallicRoughness,
    Specular,
    Diffuse,
    Shininess, // Material.shininessTexture (alpha modulates shininess) — MAT-002
    Ambient    // Material.ambientTexture (RGB modulates ambient)       — MAT-003
  };
  enum class Source {
    Url,    // ImageTexture / url list — resolve outside the SDK.
    Inline, // PixelTexture — pixels carried inline below.
    Movie,  // MovieTexture — descriptor-only, not exercised by PoC.
    Buffer  // Phase 1 binary extension: raw bytes provided by the embedder.
            // bufferBytes carries the raw encoded bytes; mimeHint is a MIME
            // type hint ("image/png", "image/jpeg", etc.). The SDK does NOT
            // decode them; the consumer decodes and uploads. resolved_desc
            // carries the decoded/structured TextureDesc once the embedder
            // materialises it (optional: empty = not yet decoded).
  };

  Slot slot = Slot::BaseColor;
  Source source = Source::Url;
  MFString url;            // verbatim; ordered fallback. Empty for Inline.
  bool repeatS = true;
  bool repeatT = true;
  SamplerParams sampler;
  SFImage inlinePixels;    // PixelTexture content when source == Inline.
  int channel = 0;         // MultiTexture stage; descriptor-only, not exercised by PoC.
  SFString texCoordMapping; // X3D v4 xxxTextureMapping label; empty = UV set 0.

  // T-TEX (v1-closure): resolved decoded pixels, threaded by TextureExtract.hpp
  // after the embedder's TextureResolver callback returns. Starts as makeFailed()
  // (= PoC white-fallback behaviour when no resolver is wired).
  //
  // CONSUMER CONTRACT:
  //   - If resolvedPixels.ready()  → upload resolvedPixels.pixels as the GPU
  //     texture (width/height/rgba are valid RGBA8 data).
  //   - If resolvedPixels.pending() → atlas not yet ready; render flat baseColor
  //     and retry next frame.
  //   - If resolvedPixels.failed()  → no decoded data; use white fallback or
  //     inlinePixels (for Source::Inline) as appropriate.
  //
  // Source::Inline textures are decoded directly from inlinePixels; the resolver
  // is NEVER called for them and resolvedPixels remains makeFailed() for inline
  // sources — consumers must check source == Source::Inline first.
  TexturePixelResult resolvedPixels;

  // T-TEX (v1-closure): the full §18.4.9 sampler descriptor. Populated by
  // TextureExtract::enrichTextureRefs() from the source texture node's
  // repeatS/T + TextureProperties (boundary/filter/mipmap/anisotropy). The legacy
  // SamplerParams `sampler` + repeatS/T bools above stay populated for existing
  // consumers; extSampler is the superset for a consumer that reads
  // TextureProperties. When no TextureProperties is present, extSampler's boundary
  // modes derive from repeatS/T (Repeat / ClampToEdge per §18.2.3).
  ExtendedSamplerParams extSampler;

  // T-TEX (v1-closure): §18.4.8 TextureCoordinateGenerator descriptor. When the
  // geometry's texCoord field was a TextureCoordinateGenerator (rather than an
  // explicit TextureCoordinate), hasTexCoordGen is true and texCoordGen carries
  // the mode+parameter so the renderer computes per-vertex UVs from view state.
  // When false, the consumer uses MeshData::texcoords (authored or default).
  bool hasTexCoordGen = false;
  TexCoordGenDesc texCoordGen;

  // Phase 1 binary extension: Source::Buffer fields.
  // Raw bytes provided by the embedder (encoded PNG/JPEG/KTX2/etc.).
  // Only meaningful when source == Source::Buffer.
  std::vector<uint8_t> bufferBytes;
  // MIME type hint for the buffer bytes ("image/png", "image/ktx2", etc.).
  std::string mimeHint;
  // Decoded/structured descriptor — filled in by the consumer once it decodes
  // the buffer; empty until then.
  std::optional<TextureDesc> resolved_desc;
};

// ---------------------------------------------------------------------------
// MaterialDesc — one tagged superset descriptor read reflection-generic by spec
// field name. Shared BY VALUE across RenderItems; NOT part of identity.
//
//   * MaterialModel selects how a consumer should interpret the fields: a Phong
//     (X3D Material) path, a Physical (PhysicalMaterial / glTF-style) path, or
//     Unlit (emissive-only debug / UnlitMaterial).
//   * baseColor composes the diffuse/base RGB with alpha. toRGBA() DOCUMENTS the
//     X3D convention alpha = 1.0 - transparency (Material default transparency=0
//     => alpha=1; a naive alpha=transparency would make spec-default materials
//     fully invisible).
//   * Null material under a PRESENT Appearance => Phong diffuse=0.8 grey (the
//     spec default Material). Null Appearance entirely => Unlit white emissive,
//     an absolute always-draws debug fallback (flagged non-spec).
// ---------------------------------------------------------------------------
enum class MaterialModel : uint8_t { Phong, Physical, Unlit };

// Transparency/alpha handling mode read off the Appearance.
// descriptor-only, not exercised by PoC (the first PoC draws opaque).
enum class AlphaMode { Opaque, Mask, Blend };

struct PhongParams {
  SFColor  diffuse           {0.8f, 0.8f, 0.8f};
  SFColor  specular          {0.0f, 0.0f, 0.0f};
  float    ambientIntensity  = 0.2f;
  float    shininess         = 0.2f;
  float    occlusionStrength = 1.0f;
};
struct PhysicalParams {
  SFColor  baseColor         {1.0f, 1.0f, 1.0f};
  float    metallic          = 1.0f;
  float    roughness         = 1.0f;
  float    occlusionStrength = 1.0f;
};

struct MaterialExtensionField {
  std::string name;
  X3DFieldType type = X3DFieldType::SFString;  // Phase 3: now live
  X3DFieldValue value;  // replaced std::any placeholder (Phase 3)
};
struct MaterialExtensionDesc {
  std::string typeName;
  std::vector<MaterialExtensionField> fields;
};

struct MaterialDesc {
  MaterialModel model = MaterialModel::Phong;

  // X3DOneSidedMaterialNode (shared):
  SFColor  emissive      {0.0f, 0.0f, 0.0f};
  float    normalScale   = 1.0f;
  float    transparency  = 0.0f;

  // X3DMaterialNode + Appearance (shared):
  AlphaMode alphaMode   = AlphaMode::Opaque;
  float     alphaCutoff = 0.5f;
  bool      doubleSided = false;

  // per-variant (only the block for `model` is valid):
  PhongParams     phong;
  PhysicalParams  physical;

  std::vector<TextureRef> textures;
  // backMaterial: X3D v4 Appearance.backMaterial (MAT-006). Uses unique_ptr to
  // avoid incomplete-type issue with self-referential optional<MaterialDesc>.
  std::unique_ptr<MaterialDesc> backMaterial;
  // backMaterialConstraintMet: true iff backMaterial is the same model type AND
  // the same texture slot set as the front (design §2 diagnostic — enforcement
  // is the consumer's call). Default true = no back material (trivially met).
  bool backMaterialConstraintMet = true;
  std::vector<MaterialExtensionDesc> extensions;

  // Copy/move ops — unique_ptr<MaterialDesc> suppresses defaults; restore them.
  MaterialDesc() = default;
  MaterialDesc(const MaterialDesc &o)
      : model(o.model), emissive(o.emissive), normalScale(o.normalScale),
        transparency(o.transparency), alphaMode(o.alphaMode),
        alphaCutoff(o.alphaCutoff), doubleSided(o.doubleSided),
        phong(o.phong), physical(o.physical), textures(o.textures),
        backMaterial(o.backMaterial
                         ? std::make_unique<MaterialDesc>(*o.backMaterial)
                         : nullptr),
        backMaterialConstraintMet(o.backMaterialConstraintMet),
        extensions(o.extensions) {}
  MaterialDesc &operator=(const MaterialDesc &o) {
    if (this != &o) {
      model = o.model; emissive = o.emissive; normalScale = o.normalScale;
      transparency = o.transparency; alphaMode = o.alphaMode;
      alphaCutoff = o.alphaCutoff; doubleSided = o.doubleSided;
      phong = o.phong; physical = o.physical; textures = o.textures;
      backMaterial = o.backMaterial
                         ? std::make_unique<MaterialDesc>(*o.backMaterial)
                         : nullptr;
      backMaterialConstraintMet = o.backMaterialConstraintMet;
      extensions = o.extensions;
    }
    return *this;
  }
  MaterialDesc(MaterialDesc &&) noexcept = default;
  MaterialDesc &operator=(MaterialDesc &&) noexcept = default;

  // Composes the per-model RGB surface with alpha = 1 - transparency.
  SFColorRGBA toRGBA() const;
};

inline SFColorRGBA MaterialDesc::toRGBA() const {
  float a = 1.0f - transparency;
  switch (model) {
    case MaterialModel::Phong:
      return SFColorRGBA{phong.diffuse.r, phong.diffuse.g, phong.diffuse.b, a};
    case MaterialModel::Physical:
      return SFColorRGBA{physical.baseColor.r, physical.baseColor.g, physical.baseColor.b, a};
    case MaterialModel::Unlit:
      return SFColorRGBA{emissive.r, emissive.g, emissive.b, a};
  }
  return SFColorRGBA{0.8f, 0.8f, 0.8f, a};
}

// ---------------------------------------------------------------------------
// Shader descriptors — ComposedShader / ShaderProgram introspection surface.
// ShaderProgramDesc is authored by ComposedShader; each stage carries its GLSL
// source. ShaderFieldBinding describes one author-declared <field> on the shader.
// ShaderProgramDesc on a RenderItem means the renderer should bind this program
// instead of the fixed-function material path.
// ---------------------------------------------------------------------------
struct ShaderStageDesc {
  enum class Stage { Vertex, Fragment, Geometry, TessControl, TessEval, Compute };
  Stage stage = Stage::Fragment;
  std::string source;
  std::string entryPoint = "main";
};

struct ShaderFieldBinding {
  std::string name;
  X3DFieldType type = X3DFieldType::SFString;
  X3DFieldValue value;
  AccessType access = AccessType::InputOutput;
};

struct ShaderProgramDesc {
  std::vector<ShaderStageDesc> stages;
  std::vector<ShaderFieldBinding> fields;
  bool isSelected = false;
  bool isValid = false;
  std::string lastError;
  std::vector<std::string> attributeBindings;
};

// ---------------------------------------------------------------------------
// LightDesc — a light resolved to WORLD space at collection time (sidesteps the
// first-path M2C-1 issue: each light is accumulated fresh down its placement).
//
//   * `global` is the AUTHORED flag, CARRIED not promoted. Verified spec
//     defaults: DirectionalLight global=false, PointLight/SpotLight global=true.
//     A consumer must NOT silently promote a global=false light to scene-wide.
//   * scopeRoot is the enclosing grouping node the light was collected under, so
//     a non-global light can be scoped to descendants of its PathKey ancestor.
// ---------------------------------------------------------------------------
struct LightDesc {
  enum class Type { Directional, Point, Spot };

  Type type = Type::Directional;
  SFColor color{1.0f, 1.0f, 1.0f};
  float intensity = 1.0f;
  float ambientIntensity = 0.0f;

  SFVec3f worldDirection{0.0f, 0.0f, -1.0f}; // normalized, world frame.
  SFVec3f worldLocation{0.0f, 0.0f, 0.0f};   // world frame (Point/Spot).
  SFVec3f attenuation{1.0f, 0.0f, 0.0f};

  float radius = 100.0f;     // Point/Spot.
  float beamWidth = 1.5708f; // Spot.
  float cutOffAngle = 0.7854f; // Spot.

  bool global = false;                 // authored; carried not promoted.
  const X3DNode *scopeRoot = nullptr;  // enclosing grouping node for scoping.
};

// ---------------------------------------------------------------------------
// CameraDesc — the bound viewpoint resolved for the frame.
//
//   * fieldOfView is X3D's MIN-dimension FOV (default 0.7854). A consumer maps
//     it to the SHORTER window dimension and derives the other axis by aspect;
//     treating it naively as vertical FOV makes wide windows look wrong.
//   * viewMatrix is first-path-resolved (documented caveat — the bound Viewpoint
//     is realistically never USE'd under two distinct frames).
//   * ortho + the OrthoViewpoint projection params are descriptor-only surface:
//     surfaced so the contract is stable, not exercised by the first PoC.
// ---------------------------------------------------------------------------
struct CameraDesc {
  Mat4 viewMatrix = Mat4::identity();
  float fieldOfView = 0.7854f; // X3D min-dimension FOV.
  float nearPlane = 0.1f;
  float farPlane = 10000.0f;

  bool ortho = false;          // descriptor-only, not exercised by PoC.
  // OrthoViewpoint fieldOfView is an MFFloat {l,b,r,t}; surfaced here when ortho.
  std::vector<float> orthoFieldOfView; // descriptor-only, not exercised by PoC.

  bool cameraChanged = false;  // surfaced for a caching consumer.
};

// ---------------------------------------------------------------------------
// BackgroundDesc — the bound Background's sky/ground gradient.
// ---------------------------------------------------------------------------
struct BackgroundDesc {
  std::vector<SFColor> skyColor;
  std::vector<float> skyAngle;
  std::vector<SFColor> groundColor;
  std::vector<float> groundAngle;

  bool backgroundChanged = false; // surfaced for a caching consumer.
};

// ---------------------------------------------------------------------------
// RenderDelta — THE single authoritative change channel.
//
// fullSnapshot() returns a RenderDelta with EVERY item in `added` (so frame 0
// and frame N share one upload path). delta() partitions changed RenderItemIds
// into the buckets below; the scalar bits flag the full-recomputed camera/
// background/lights surfaces for a caching consumer.
//
// RenderItem carries NO changeBits: this struct is the only encoding of change.
// ---------------------------------------------------------------------------
struct RenderDelta {
  std::vector<RenderItemId> added;
  std::vector<RenderItemId> removed;
  std::vector<RenderItemId> updatedTransform;
  std::vector<RenderItemId> updatedGeometry;
  std::vector<RenderItemId> updatedMaterial;

  bool cameraChanged = false;
  bool backgroundChanged = false;
  bool lightsChanged = false;
};

} // namespace x3d::runtime::extract

#endif // X3D_RUNTIME_EXTRACT_RENDERITEM_HPP
