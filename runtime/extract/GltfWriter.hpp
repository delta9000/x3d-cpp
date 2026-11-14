// GltfWriter.hpp
// Writes extracted render items as a binary glTF 2.0 asset (.glb).
//
// The input is what SceneExtractor already produces: one RenderItem per
// placed shape, with a world matrix, a shared mesh and a material descriptor.
// So the exporter sees the scene after PROTO/Inline expansion, Switch/LOD
// selection and tessellation, and every geometry MeshBuilder supports comes
// out as triangles, lines or points.
//
//   sdk::SceneExtractor ex(ctx, doc.scene);
//   auto frame = ex.fullSnapshot();
//   std::string glb = extract::writeGlb(ex, frame.added);
//
// Written:
//   - one glTF node per item, carrying the item's world matrix (the X3D
//     grouping hierarchy is flattened), named after the nearest DEF on its path;
//   - one glTF mesh per distinct MeshData and material, so DEF/USE instances
//     share buffers: POSITION, NORMAL, TEXCOORD_n, COLOR_0, uint32 indices;
//   - materials: PhysicalMaterial maps directly to pbrMetallicRoughness;
//     Material (Phong) is approximated (diffuse -> baseColor, metallic 0,
//     roughness from shininess); UnlitMaterial and unlit lines/points use
//     KHR_materials_unlit. transparency becomes alpha with BLEND.
//     solid=FALSE becomes doubleSided;
//   - textures for the base colour, normal, emissive, occlusion and
//     metallic-roughness slots; on an unlit surface (UnlitMaterial, or a
//     texture with no Material) the emissive texture is the base colour
//     texture. A PNG or JPEG ImageTexture is embedded when
//     GltfWriteOptions::readImage supplies its bytes, else referenced by its
//     url as authored. A PixelTexture, and any other texture the host's
//     TextureResolver has decoded, is embedded as a PNG. Texture coordinates
//     are flipped to glTF's top-left origin;
//   - lights as KHR_lights_punctual (directional, point, spot), each on a node
//     placed and aimed in world space;
//   - Viewpoint and OrthoViewpoint nodes as cameras, the bound one first;
//   - animation: a Transform whose translation, rotation or scale is driven
//     by a PositionInterpolator or OrientationInterpolator is kept as a glTF
//     node (with helper nodes for center and scaleOrientation), and the
//     interpolator's keys become a LINEAR sampler. One glTF animation per
//     driving TimeSensor; its cycleInterval sets the duration. Shapes below
//     an animated Transform are its children instead of world-matrix roots.
//
// Approximations to know about:
//   - X3D light intensity is a unitless [0,1] factor; it is written unchanged
//     as glTF intensity (lux / candela). ambientIntensity, the attenuation
//     coefficients and light scoping (global=FALSE) have no glTF equivalent
//     and are dropped; radius becomes range.
//   - X3D fieldOfView is the angle across the smaller viewport dimension;
//     it is written as yfov, which is the same thing for a landscape viewport.
//     An OrthoViewpoint's off-centre extent is centred.
//   - Embedded PNGs are stored uncompressed.
//   - Colour values are copied as authored, with no sRGB/linear conversion.
//
// Not written (each is counted in GltfWriteReport): packed/binary geometry
// items, MovieTexture, MultiTexture, cube and 3D textures, textures on
// generated texture coordinates, undecoded textures with no PNG/JPEG url,
// specular/shininess/ambient maps, back materials, author shaders, GeoViewpoint. Skinned H-Anim items are
// written as static meshes in the mesh's stored pose.
//
// Animation limits: only the three Transform properties above are animated.
// Interpolators driving anything else (colours, scalars, coordinates,
// viewpoints, lights), chains through other nodes (sequencers, scripts,
// EaseInEaseOut, followers), and H-Anim motion are not exported; routes out
// of an interpolator that are dropped are counted. glTF has no loop flag, so
// TimeSensor.loop is left to the viewer, and startTime is not carried.
#ifndef X3D_RUNTIME_EXTRACT_GLTF_WRITER_HPP
#define X3D_RUNTIME_EXTRACT_GLTF_WRITER_HPP

#include "SceneExtractor.hpp"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace x3d::runtime::extract {

struct GltfWriteOptions {
  /// asset.generator in the output.
  std::string generator = "x3d-cpp";
  /// Returns the bytes of the image file behind a texture url, or "" if it
  /// cannot be read. `baseUrl` is the url of the document the texture node
  /// came from (may be empty). When set, PNG and JPEG files it returns are
  /// embedded in the GLB; without it, or when it returns nothing, the image
  /// is referenced by its url as authored, which a viewer resolves against
  /// the .glb's own location.
  std::function<std::string(const std::string &url, const std::string &baseUrl)>
      readImage;
};

struct GltfWriteReport {
  std::size_t nodes = 0;     ///< items written
  std::size_t meshes = 0;    ///< distinct glTF meshes
  std::size_t materials = 0; ///< distinct glTF materials
  std::size_t textures = 0;  ///< distinct glTF textures
  std::size_t embeddedImages = 0; ///< images embedded (PNG or JPEG)
  std::size_t externalImages = 0; ///< texture uses left as url references
  std::size_t lights = 0;    ///< KHR_lights_punctual lights
  std::size_t cameras = 0;   ///< cameras
  std::size_t triangles = 0; ///< triangles over all nodes (instances counted)

  std::size_t skippedEmpty = 0;    ///< items with no positions or indices
  std::size_t skippedPacked = 0;   ///< packed/binary geometry items
  /// Textures with no image glTF can carry: movie, multi, cube and 3D
  /// textures, generated texture coordinates, a mesh without texture
  /// coordinates, or an undecoded image that is not a PNG/JPEG url.
  std::size_t droppedTextures = 0;
  /// Textures in a slot glTF has no property for: specular, shininess and
  /// ambient maps, and the lighting maps of an unlit surface.
  std::size_t unmappedTextureSlots = 0;
  std::size_t staticSkins = 0;     ///< skinned items written without a skin
  std::size_t droppedBackMaterials = 0;
  std::size_t droppedShaders = 0;
  std::size_t skippedViewpoints = 0; ///< GeoViewpoint

  std::size_t animations = 0;        ///< glTF animations written
  std::size_t animationChannels = 0; ///< channels over all animations
  /// Channels whose Transform is on no exported item's path, or whose key and
  /// value counts disagree.
  std::size_t skippedAnimationChannels = 0;
  /// ROUTEs out of an interpolator that could not become a channel: any
  /// interpolator other than Position/Orientation, or a destination that is
  /// not a Transform's translation, rotation or scale.
  std::size_t skippedAnimationRoutes = 0;
};

/// One camera to export. `cameraToWorld` places it; like an X3D viewpoint a
/// glTF camera looks down its local -Z with +Y up.
struct GltfCamera {
  std::string name;
  Mat4 cameraToWorld = Mat4::identity();
  bool ortho = false;
  float yfov = 0.7854f;    ///< perspective: vertical field of view, radians
  float xmag = 1.0f, ymag = 1.0f; ///< orthographic half extents
  float znear = 0.1f;
  float zfar = 0.0f;       ///< <= 0: infinite (perspective) / 10000 (ortho)
};

/// Keyframes for one property of one Transform.
struct GltfAnimationChannel {
  enum class Path { Translation, Rotation, Scale };
  const X3DNode *target = nullptr; ///< the Transform node
  Path path = Path::Translation;
  std::vector<float> times;  ///< seconds, one per key, non-decreasing
  /// 3 floats per key (translation, scale) or a quaternion x,y,z,w (rotation).
  std::vector<float> values;
};

/// The channels that play together: in X3D terms, everything one TimeSensor
/// drives.
struct GltfAnimation {
  std::string name;
  std::vector<GltfAnimationChannel> channels;
};

/// Scene content beyond the render items.
struct GltfSceneExtras {
  std::vector<LightDesc> lights;
  std::vector<GltfCamera> cameras;
  std::vector<GltfAnimation> animations;
};

/// The lights, viewpoints and animations of the extractor's scene:
/// `extractor.lights()`; every Viewpoint/OrthoViewpoint, the bound one first
/// (GeoViewpoints are counted in `report->skippedViewpoints`); and one
/// animation per TimeSensor, from the ROUTEs
/// TimeSensor.fraction_changed -> Position/OrientationInterpolator ->
/// Transform.translation|rotation|scale. Key times are key * cycleInterval.
/// Other interpolator routes are counted in `report->skippedAnimationRoutes`.
GltfSceneExtras gltfExtrasFrom(const SceneExtractor &extractor,
                               GltfWriteReport *report = nullptr);

/// Serialize `items`, plus the lights and cameras in `extras`, to GLB bytes
/// (the string may contain NULs).
std::string writeGlb(const std::vector<const RenderItem *> &items,
                     const GltfSceneExtras &extras,
                     const GltfWriteOptions &options = {},
                     GltfWriteReport *report = nullptr);

/// Convenience over an extractor and item ids (a snapshot's `added` list);
/// includes the scene's lights and viewpoints (see gltfExtrasFrom).
std::string writeGlb(const SceneExtractor &extractor,
                     const std::vector<RenderItemId> &ids,
                     const GltfWriteOptions &options = {},
                     GltfWriteReport *report = nullptr);

} // namespace x3d::runtime::extract

#endif // X3D_RUNTIME_EXTRACT_GLTF_WRITER_HPP
