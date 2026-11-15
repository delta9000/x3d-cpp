---
title: glTF export
summary: "writeGlb: how extracted render items become a binary glTF 2.0 file and what is left out."
tags: [subsystem, extract, gltf, export]
updated: 2026-11-15
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
| PositionInterpolator / OrientationInterpolator routed to a Viewpoint's or OrthoViewpoint's `position` / `orientation`, a PositionInterpolator routed to a PointLight's or SpotLight's `location`, or to a DirectionalLight's or SpotLight's `direction` | Channels in the same animation: the camera or light node carries the authored value as its `translation`/`rotation` (a direction as the rotation aiming −Z along it), under a `matrix` node for the Transforms above it |
| PositionInterpolator / OrientationInterpolator routed to a Transform's `center` / `scaleOrientation` | Channels on the helper nodes (below): the `T+C` translation, the `SR` and `SR⁻¹` rotations and the tail translation |
| Color/ScalarInterpolator routed to a material, light or viewpoint property (table below) | `KHR_animation_pointer` channels on the glTF property, in the same animation |
| SplinePositionInterpolator, SplineScalarInterpolator, SquadOrientationInterpolator, and ColorInterpolator (X3D blends in HSV) | Sampled: each key interval is split into 8 `LINEAR` steps, valued by the runtime's own interpolation |
| NurbsPositionInterpolator, NurbsOrientationInterpolator | Sampled at 65 even fractions over the curve |
| GeoPositionInterpolator (`value_changed`, the local position) | Sampled 8 steps per key interval, since it interpolates in geographic coordinates and then projects |

Animated properties (`KHR_animation_pointer`):

| X3D field | glTF property |
|---|---|
| `Material.diffuseColor`, `PhysicalMaterial.baseColor`, the `emissiveColor` of an unlit surface | `baseColorFactor` RGB |
| `transparency` (any material) | `baseColorFactor` alpha (1 − transparency); the material is written with `alphaMode` BLEND. Colour and transparency in one animation share one VEC4 channel |
| `emissiveColor` of a lit material | `emissiveFactor` (written even when black) |
| `PhysicalMaterial.metallic` / `roughness` | `metallicFactor` / `roughnessFactor` |
| `Material.shininess` | `roughnessFactor` through the static mapping, sampled 8 steps per interval |
| A light's `color` / `intensity`; a PointLight's or SpotLight's `radius` | `KHR_lights_punctual` `color` / `intensity` / `range` |
| `SpotLight.beamWidth` / `cutOffAngle` | `spot.innerConeAngle` and `spot.outerConeAngle`, clamped together so inner < outer ≤ π/2 |
| `Viewpoint.fieldOfView` | `perspective.yfov` |

- A material an animation drives is written as glTF materials of its own, so
  its channel moves no other shape. A colour channel aimed at a shape whose
  per-vertex colours replace the material colour is skipped and counted.
- Most viewers do not load `KHR_animation_pointer`; they show the static
  values, which are the values at export time. It is listed in
  `extensionsUsed`, not `extensionsRequired`.

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
- Lights and cameras: a light or viewpoint below an animated Transform is a
  child of that Transform's node, so it moves with it. One whose own
  `location`, `position` or `orientation` is animated keeps that value as its
  node's `translation`/`rotation` under a `matrix` node for its ancestors.
  Everything else is still a world-space root.
- An animated `center` or `scaleOrientation` gets its helper nodes even when
  it is zero at export time. Between samples, the tail translation
  `SR⁻¹(−C)` is linear while the true value turns with `SR`, so it is sampled
  8 steps per interval: exact at every sample, within about 1% of the
  Transform's size between them for a 1.2 radian sweep.
- When a field and the property it shares a glTF value with (a translation
  and a centre, a colour and a transparency, the two cone angles) are
  animated by different TimeSensors, each animation writes the shared value
  with the other field at its export-time value.
- Not exported: CoordinateInterpolator, CoordinateInterpolator2D and
  NormalInterpolator (they need morph targets); H-Anim skinning and motion;
  PositionInterpolator2D (glTF has no texture transform without
  `KHR_texture_transform`, which the exporter does not write);
  NurbsSurfaceInterpolator (driven by a 2D fraction, not a TimeSensor);
  chains through other
  nodes (sequencers, Scripts, EaseInEaseOut, followers); and fields with no
  glTF property, such as `specularColor`, `ambientIntensity` and
  `attenuation`.
- Dropped interpolator routes are counted in `skippedAnimationRoutes`, and a
  channel whose target was not written (a Transform with nothing exported
  under it, a material on no exported shape, a light that is off) in
  `skippedAnimationChannels`.
- glTF has no loop flag, so `TimeSensor.loop` is left to the viewer.

The JSON and the PNG encoder are written by hand; the tests
(`runtime/extract/tests/gltf_writer_test.cpp`) check the container and
selected values, and exports of the CLI fixtures and the lion-head models pass
the Khronos glTF validator with no errors. The validator checks
`KHR_animation_pointer` channels only partly (it reports
`INCOMPLETE_EXTENSION_SUPPORT`).
