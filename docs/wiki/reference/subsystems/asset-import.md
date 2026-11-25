---
title: Mesh import
summary: x3d_cpp::import reads glTF (and, with optional backends, OBJ, FBX, Collada, USD, ...) into an intermediate scene and emits an X3D 4.0 document; `x3d import` is its CLI.
tags: [subsystem, import, gltf, usd, assimp, texture-pipeline, cli]
updated: 2026-11-25
related:
  - ../../explanation/decisions/0077-mesh-import-in-the-sdk.md
  - ../../explanation/decisions/0044-asset-import-backend-selection.md
  - ../../explanation/decisions/0043-usd-material-portable-glsl-seam.md
  - gltf-export.md
  - cli-suite.md
---

# Mesh import

`runtime/import/` is the SDK's mesh importer. A backend reads a model file into
an `ImportScene`, the intermediate representation in `ImportScene.hpp` that
holds values and indices and no third-party types. `planTextures()` writes the
scene's images, and `emitDocument()` builds an X3D 4.0 document from it.
`importFile()` runs all three. The library is `x3d_cpp::import`. It links the
authoring runtime and no parser, execution context or extractor. `x3d/sdk.hpp`
re-exports it as `sdk::importer`, `sdk::importFile`, `sdk::ImportOptions` and
`sdk::ImportResult`. The decision is
[ADR-0077](../../explanation/decisions/0077-mesh-import-in-the-sdk.md).

`x3d import` is the command-line front end (see [CLI](#cli)). Together with
`x3d extract -o model.glb` ([glTF export](gltf-export.md)) it gives glTF round
trips in both directions.

## Backends

| Backend | Formats | Library | Build option | Priority |
|---|---|---|---|---|
| `gltf` (cgltf) | `.gltf`, `.glb` | `x3d_cpp::import` (installed) | always built | 100 |
| `assimp` | every extension the system Assimp lists (OBJ, FBX, Collada, PLY, STL, 3DS, ...) | `x3d_import_assimp` (source tree) | `X3D_CPP_BUILD_ASSIMP` (needs a system Assimp) | 50; 10 for glTF |
| `usd` (tinyusdz) | `.usd`, `.usda`, `.usdc`, `.usdz` | `x3d_import_usd` (source tree) | `X3D_CPP_BUILD_USD` (fetches tinyusdz, applies `runtime/import/usd/tinyusdz.patch`) | 100 |

`builtinBackends()` returns a registry with `gltf`. `addAssimpBackend()` and
`addUsdBackend()` add the others. `BackendRegistry::select` picks the highest
positive priority (the first registered wins ties), and `ImportOptions::backend`
or `--backend` picks one by name. The Assimp and USD libraries are not
installed, because they carry third-party dependencies the installed SDK does
not.

The tinyusdz patch adds USD composition (references, payloads, inherits,
variants), reads the full UsdLuxLightAPI inputs (colour, intensity, exposure,
...) on every light type, and lets Tydra read a SkelAnimation whose
`blendShapeWeights` carry only time samples.

## What each backend reads

| Content | glTF | Assimp | USD |
|---|---|---|---|
| Hierarchy | all scene roots; TRS or matrix; `EXT_mesh_gpu_instancing` | node tree; FBX pivots collapsed, FBX axes and units applied | every root prim; PointInstancer instances; Z-up and `metersPerUnit` wrapped in a `stage` root |
| Geometry | triangles, strips, fans, lines, points; positions, normals, every `TEXCOORD_n`, `COLOR_0` | triangulated faces, lines, points; all UV sets; colour set 0; smooth normals generated when missing | triangulated meshes; every primvar UV set; `displayColor` and `displayOpacity`; GeomSubsets split by material |
| Materials | metallic-roughness, unlit, `KHR_materials_*` (emissive_strength, ior, clearcoat, specular, pbrSpecularGlossiness) | Phong, PBR and unlit, glTF alpha mode | UsdPreviewSurface (both workflows, ior, clearcoat, opacityThreshold); `displayColor` materials |
| Textures | external, GLB buffer view, data URI; samplers; `KHR_texture_transform` baked into UVs | external and embedded (compressed or raw BGRA) | external and USDZ-packed; wrap modes; UsdTransform2d baked into UVs |
| Cameras | perspective, orthographic | perspective, orthographic | GeomCamera, perspective and orthographic |
| Lights | `KHR_lights_punctual` | directional, point, spot; area as point | Distant; Sphere (spot with ShapingAPI cone); Rect and Disk as hemispherical spots; Cylinder as point |
| Skins | skins with inverse bind matrices | bones (one skin per node over its bones) | UsdSkel skeletons, joint influences, `geomBindTransform` |
| Morph targets | position and normal targets, default weights | anim meshes (stored as deltas) | BlendShapes |
| Animation | TRS, weights, `KHR_animation_pointer` (material, light and camera fields); STEP, LINEAR, CUBICSPLINE | TRS keys (Assimp gives no per-key interpolation), morph channels | Xform time samples, SkelAnimation joints and blend-shape weights |

Each backend reports what it does not read in `ImportReport::warnings`. glTF
refuses a file that requires an extension outside the list above, as the glTF
specification asks. It warns for Draco, meshopt, BasisU, WebP and material
variants it leaves unread, and for files with several scenes. Assimp warns for
height, opacity, displacement and reflection maps. USD warns for texture
scale/bias and channel layouts it cannot map, and drops DomeLight.

## X3D mapping

`emitDocument()` (`ImportEmit.cpp`) builds this graph. Every node DEF follows
the source name, made unique.

| ImportScene | X3D |
|---|---|
| Node | `Transform` (translation, rotation, scale; a sheared matrix is approximated) |
| Mesh | `Shape` with `IndexedTriangleSet`, `IndexedLineSet` or `PointSet`; `Coordinate`, `Normal`, `ColorRGBA` |
| UV sets | `TextureCoordinate`, or `MultiTextureCoordinate` with `mapping` `uv<n>` when a material uses more than one set |
| Metallic-roughness material | `PhysicalMaterial` with base, metallic-roughness, normal, emissive and occlusion textures |
| Phong material | `Material` with diffuse, specular, shininess, ambient, normal, emissive and occlusion textures |
| Unlit material | `UnlitMaterial` (base colour as emissive) |
| Alpha mode, cutoff | `Appearance.alphaMode`, `alphaCutoff` |
| Double-sided | `solid FALSE` |
| Texture | `ImageTexture` with `TextureProperties` for filters and wrap modes; DEF/USE shared |
| Camera | `Viewpoint` or `OrthoViewpoint` at its node |
| Light | `DirectionalLight`, `PointLight` or `SpotLight` at its node, `global TRUE` |
| Skin | `HAnimHumanoid` (version 2.0): `HAnimJoint` tree, `HAnimSegment`, skin `Coordinate` and `Normal`, per-joint `skinCoordIndex`/`skinCoordWeight` |
| Animation | one `TimeSensor` per animation (loops); `PositionInterpolator`, `OrientationInterpolator`, `ScalarInterpolator`, `ColorInterpolator` and `ROUTE`s to the target fields |
| Morph weights | `CoordinateInterpolator` and `NormalInterpolator` over the morphed points, keyed at the weight keys |

`ImportOptions::autoplay` (`--autoplay`) picks which TimeSensors start enabled:
the first (default), all or none. A host starts another by setting its
`enabled`. STEP keys become a doubled key, and CUBICSPLINE curves and colours
(X3D blends colour in HSV) are sampled at `samplesPerInterval` linear steps
per key interval.

### Approximations

`EmitReport::warnings` counts each of these:

| Case | What happens |
|---|---|
| Light intensity above 1 | clamped (X3D intensity is a [0,1] factor) |
| Light or emissive colour outside [0,1] | clamped |
| Field of view outside (0, π) | replaced by π/4 |
| Animated value outside its X3D range | clamped |
| Texture on a set the mesh lacks; Phong-only slot on a PBR material and the reverse | texture dropped |
| Triangles with out-of-range indices | dropped |
| Node or joint matrix with shear | approximated by translation, rotation and scale |
| Singular inverse bind matrix | rest pose used |
| Joint bound differently by two skins | first binding used |
| Skinned mesh without weights; skinned points | left in the bind pose |
| Weight channel on a node without morph targets; channel with no X3D target | dropped |

Values X3D materials cannot hold (ior, clearcoat, the specular workflow,
opacityMode) stay in `ImportResult::scene`. `materialGlsl(material)` returns
the canonical `runtime/import/shaders/usd_preview_surface.frag` with that
material's values baked in as constants (`--emit-glsl`). See
[ADR-0043](../../explanation/decisions/0043-usd-material-portable-glsl-seam.md).

## Textures

`planTextures()` (`TexturePlan.cpp`) writes each referenced image once, named
by content hash, to `<assetsRoot>/assets/`, and gives `emitDocument()` a url
relative to the output document. PNG and JPEG bytes are copied unchanged
unless `recompress` is set. Raw embedded pixels (Assimp) are written as PNG.
Other images are decoded with `ImportOptions::decoder` (for example the stb
`TextureResolver`) and written as PNG; with no decoder they are copied as they
are and listed in `TexturePlanReport::notWebSafe`. `textures = false`
(`--no-textures`) writes no images and no `ImageTexture` nodes.

## CLI

```
x3d import <model> -o <output> [options]
x3d import --list-backends
```

| Option | Does |
|---|---|
| `-o <output>` | Output file; the encoding follows the extension (`.x3d`, `.x3dv`, `.json`). Missing directories are created |
| `-f xml\|vrml\|json` | Output encoding, needed for stdout |
| `--assets-dir <dir>` | Where `assets/` goes (default: the output's directory) |
| `--no-textures`, `--recompress` | As above |
| `--profile auto\|interchange\|interactive\|immersive\|full` | `auto` (default) picks the smallest of these that fits and warns when none does |
| `--backend <name>` | Force a backend |
| `--autoplay first\|all\|none` | Which animations start enabled |
| `--samples <n>` | Samples per key interval for sampled curves (default 8) |
| `--emit-glsl <dir>` | Write `<material>.frag` per material |
| `--verify` | Re-read the written document and run the `validate` checks |
| `--stats` | Print counts to stderr |

Exit codes: 0 ok, 1 usage error, 2 import or I/O failure, 3 `--verify` found
issues. The CLI links the Assimp and USD backends when they are built, and stb
when `X3D_CPP_BUILD_STB` is on. Steps are in
[Import a model](../../how-to/import-a-model.md).

## Tests

| Test | Covers |
|---|---|
| `x3d_import` (`runtime/import/tests/`, behavior suite) | registry, emit mapping and warnings, texture plan, material GLSL, the glTF backend, and the round-trip oracle |
| `x3d_import_assimp`, `x3d_import_usd` (`import` preset, GitHub `import` job) | each backend on fixtures, Assimp against glTF on `twobox.glb`, and round trips through Assimp |
| `x3d_cli_test` | `x3d import` options, exit codes, and `extract` → `import` |

The round-trip oracle (`RoundTripOracle.hpp`) writes an X3D scene to glb with
the SDK's exporter, imports it, and compares both documents frame by frame
through `RuntimeSession`: world-space triangles and colours, lights, and the
camera, at several times. It covers static geometry, materials, lights and
viewpoints; Transform, material, light and camera animation; morph targets; and
a skinned humanoid. Fixtures under `runtime/import/tests/fixtures/` are
generated in-repo (`gen_gltf_fixture.py`, hand-written USDA) and carry no
external license. Commit only CC0 or CC-BY models.

## Key files

| Source | Role |
|---|---|
| `runtime/import/AssetImport.hpp` | `importFile`, `importScene`, options, report |
| `runtime/import/ImportScene.hpp` | Intermediate representation |
| `runtime/import/ImportSource.hpp` | `ImportSource`, `BackendRegistry` |
| `runtime/import/ImportEmit.cpp` | X3D emission |
| `runtime/import/TexturePlan.cpp` | Image output |
| `runtime/import/MaterialGlsl.cpp`, `shaders/` | Per-material GLSL |
| `runtime/import/gltf/CgltfSource.cpp` | glTF backend |
| `runtime/import/assimp/AssimpSource.cpp` | Assimp backend |
| `runtime/import/usd/UsdSource.cpp`, `tinyusdz.patch` | USD backend |
| `cmake/x3d/import.cmake` | Optional backends and tests |
| `tools/x3d_cli.cpp` (`cmdImport`) | `x3d import` |
