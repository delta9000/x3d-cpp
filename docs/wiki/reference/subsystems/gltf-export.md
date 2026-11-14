---
title: glTF export
summary: "writeGlb: how extracted render items become a binary glTF 2.0 file and what is left out."
tags: [subsystem, extract, gltf, export]
updated: 2026-11-14
related:
  - extract.md
  - cli-suite.md
---

# glTF export

The items come from the [Extraction pipeline](extract.md); the command-line
entry is `x3d extract -o out.glb` ([CLI suite](cli-suite.md)).

- `extract::writeGlb(extractor, snapshot.added, options, &report)` (also
  `sdk::writeGlb`) serializes render items to a GLB.
- It reads only the render feed, so it exports what a renderer would be given:
  the scene after PROTO and Inline expansion, with Switch and LOD selections
  applied and every MeshBuilder geometry tessellated.
- `x3d extract in.x3d -o out.glb` is the CLI form.

| X3D side | glTF side |
|---|---|
| One `RenderItem` | One root node with the item's world `matrix` (the grouping hierarchy is flattened), named after the nearest DEF on its path |
| One `MeshData` per material | One mesh; DEF/USE placements share its accessors |
| positions, normals, `texcoords` and further `texcoordSets`, per-vertex colours | `POSITION` (with min/max), `NORMAL` (normalized), `TEXCOORD_n` with `t` flipped to glTF's top-left origin, `COLOR_0` (RGBA float) |
| Triangles, Lines, Points | modes 4, 1, 0; `ccw=FALSE` triangles are turned round |
| `PhysicalMaterial` | `pbrMetallicRoughness` directly |
| `Material` | Approximation: diffuse → base colour, metallic 0, roughness `(2 / (128·shininess + 2))^¼`. Specular colour and ambient intensity are lost |
| `UnlitMaterial`, no material, lines/points without normals | `KHR_materials_unlit` with the emissive colour |
| `transparency` | base colour alpha with `alphaMode` BLEND; `AlphaMode::Mask` → MASK with its cutoff |
| `solid=FALSE` or a two-sided material | `doubleSided` |
| Textures in the base colour/diffuse, normal, emissive, occlusion and metallic-roughness slots | The matching glTF texture property. On an unlit surface (an `Appearance` with a texture and no `Material`, or `UnlitMaterial.emissiveTexture`) the emissive texture is the `baseColorTexture`, since `KHR_materials_unlit` shows the base colour. `repeatS/T` → sampler wrap |
| Texture images | A PNG or JPEG `ImageTexture` is embedded when `GltfWriteOptions::readImage` returns its bytes (the CLI reads local files, resolved against the texture's source document), so the GLB does not depend on where it is written; otherwise it is an external `images[].uri`, the url as authored. A PixelTexture, or any other url texture the host's `TextureResolver` has decoded → an embedded PNG (stored uncompressed). Identical images are written once |
| DirectionalLight, PointLight, SpotLight (`extractor.lights()`) | `KHR_lights_punctual` lights, each on a node with the world location and a rotation aiming −Z along the world direction. `radius` → `range`; `beamWidth`/`cutOffAngle` → inner/outer cone, with inner pulled inside outer |
| Viewpoint, OrthoViewpoint | Cameras on nodes carrying the viewpoint's world pose, the bound one first; `description` (else DEF) → name; `nearDistance`/`farDistance` → `znear`/`zfar` |
| PositionInterpolator / OrientationInterpolator routed to a Transform's `translation`, `scale` or `rotation` | A glTF animation: the Transform is kept as a node (see below), the interpolator's `key`/`keyValue` become a `LINEAR` sampler (rotations as quaternions), and key times are `key × cycleInterval` of the TimeSensor routed to the interpolator's `set_fraction`. One animation per TimeSensor, named by its DEF |

- Approximations: X3D light `intensity` is a unitless factor and is written
  unchanged as glTF intensity (lux or candela), so absolute brightness will
  not match a physically based viewer; `ambientIntensity`, the `attenuation`
  coefficients and light scoping (`global=FALSE`) have no glTF equivalent and
  are dropped.
- X3D `fieldOfView` spans the smaller viewport dimension and is written as
  `yfov`, which is the same angle for a landscape viewport.
- An OrthoViewpoint's off-centre extent is centred.
- Colours are copied as authored with no sRGB/linear conversion.

`writeGlb(items, extras, ...)` takes the lights and cameras explicitly;
`gltfExtrasFrom(extractor)` collects them, and the extractor overload does both.

Colour, metallic, roughness and alpha factors are clamped to [0,1], so an
out-of-range authored value still gives a valid file.

- Not written, each counted in `GltfWriteReport`: packed (binary) geometry
  items; MovieTexture, MultiTexture, cube and 3D textures, textures on
  generated texture coordinates and undecoded textures with no PNG/JPEG url
  (`droppedTextures`); specular, shininess and ambient textures and the
  lighting maps of an unlit surface (`unmappedTextureSlots`); back materials;
  author shaders; GeoViewpoint.
- A normal map is written without a `TANGENT` attribute, so viewers derive the
  tangent space.
- Skinned H-Anim items are written as static meshes.
- Backgrounds and fog are not exported.
- **Animated Transforms.** A scene with no exportable animation is written
  flat, as above.
- Each Transform that an animation targets becomes glTF nodes, once per
  placement (a USE'd Transform under two parents gives two node chains and two
  channels on one sampler).
- X3D composes `T·C·R·SR·S·SR⁻¹·C⁻¹` where glTF has only `T·R·S`, so the
  Transform is written as `{translation T+C, rotation R, scale S}` with a
  child `{translation −C}` when it has a `center`, or as three nodes `{T+C,
  R}` → `{rotation SR, scale S}` → `{translation SR⁻¹(−C), rotation SR⁻¹}`
  when it has a `scaleOrientation`.
- Translation keys have the centre added.
- Plain Transforms above an animated one collapse into one `matrix` node;
  shapes below hang from the animated node with the remainder of their world
  matrix, so every shape's composed matrix at export time equals its
  `RenderItem::worldTransform`.
- A repeated `key` (an X3D jump) is nudged forward by one float step, because
  glTF sample times must strictly increase.
- Animation limits: only those three Transform fields.
- Interpolators that drive anything else (colours, scalars, coordinates,
  viewpoints, lights), chains through other nodes (sequencers, Scripts,
  EaseInEaseOut, followers), `center` and `scaleOrientation` animation, and
  H-Anim motion are not exported; dropped interpolator routes are counted in
  `skippedAnimationRoutes`, and a channel whose Transform has no exported
  shape under it in `skippedAnimationChannels`. glTF has no loop flag, so
  `TimeSensor.loop` is left to the viewer.
- Lights and cameras under an animated Transform stay where they were at
  export time.

The JSON and the PNG encoder are written by hand; the tests
(`runtime/extract/tests/gltf_writer_test.cpp`) check the container and
selected values, and exports of the CLI fixtures and the lion-head models pass
the Khronos glTF validator with no errors.
