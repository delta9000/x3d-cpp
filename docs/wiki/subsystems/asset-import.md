---
title: Asset-Import Subsystem
summary: Asset-import, scene-graph emission, texture re-encoding pipeline, and the footprint-gated x3d_cpp::authoring target.
tags: [subsystem, asset-import, authoring, texture-pipeline, CLI, cmake]
updated: 2026-07-01
related:
  - ../architecture.md
  - ../decisions/0041-authoring-target.md
---

# Asset-Import Subsystem

The Asset-Import Subsystem provides the capability to ingest 3D assets (e.g., OBJ models or synthetic fixtures) and translate them into standards-compliant **X3D 4.0** scenes. It defines a lean, footprint-gated serialization/authoring layer, a backend-agnostic import seam, and a consumer-side texture pipeline.

## Purpose

The main goals of this subsystem are:
1. **Headless Scene Authoring**: Allow programmatic construction and serialization of conformant X3D documents without needing the full parser or runtime execution engines.
2. **Standardized 4.0 Export**: Map input geometries, material properties (Phong & PBR), lights, cameras, and textures to spec-normative X3D 4.0 nodes.
3. **Footprint Quarantine**: Enforce compile-time and link-time separation between the lightweight authoring surface and the heavier runtime engine via a footprint gate script.

## Data Flow Pipeline

The ingestion pipeline flows linearly from a source 3D file through to the serialized X3D output:

```mermaid
graph TD
    Src[Source File] --> Backend[ImportSource Backend]
    Backend --> IR[ImportScene POD IR]
    IR --> Emit[emit]
    subgraph Slim Authoring Layer (x3d_cpp::authoring)
        Emit --> Doc[X3DDocument]
        Doc --> SelfVal[Self-Validation: Range-Check & Profile-Fit]
    end
    SelfVal --> Profile[Narrowest Profile / auto]
    Doc --> TexPipeline[Texture Pipeline]
    TexPipeline --> TexPlan[TexturePlan / assets/ directory]
    Profile --> Writer[Writer Selection]
    TexPlan --> Writer
    Writer --> Output[out.x3d + assets/*.png]
```

## Seams and Backends

The subsystem decouples the importer from the scene generator through the `ImportSource` seam:

- **`ImportScene` POD IR** ([import_source.hpp](file:///home/ben/code/x3d-cpp/examples/asset_import/import_source.hpp)): A backend-agnostic intermediate representation representing nodes, mesh geometry (vertices, normals, UVs, indices), materials, lights, cameras, and embedded texture bytes. No third-party types leak past this boundary.
- **`FixtureSource`** ([fixture_source.cpp](file:///home/ben/code/x3d-cpp/examples/asset_import/fixture_source.cpp)): A dependency-free synthetic scene generator used for unit tests and CI testing when assimp is absent.
- **`CgltfSource`** ([cgltf_source.cpp](file:///home/ben/code/x3d-cpp/examples/asset_import/cgltf_source.cpp)): A glTF 2.0 (`.gltf`/`.glb`) backend built on the vendored, header-only [cgltf](https://github.com/jkuhlmann/cgltf) parser (MIT). It walks the parsed document into the IR — meshes, PBR metallic-roughness materials, textures (external URI, GLB bin buffer-view, and base64 `data:` URI), node hierarchy with TRS, perspective cameras, and `KHR_lights_punctual` lights. Guarded by `X3D_ASSET_IMPORT_HAVE_CGLTF`; **built by default** (`X3D_CPP_BUILD_CGLTF=ON`), so glTF is a first-class path with no assimp requirement.
- **`AssimpSource`** ([assimp_source.cpp](file:///home/ben/code/x3d-cpp/examples/asset_import/assimp_source.cpp)): A backend that wraps the Assimp library. It parses 40+ standard 3D formats, triangulates meshes, computes missing normals, and maps them to the intermediate representation. Guarded by `X3D_ASSET_IMPORT_HAVE_ASSIMP`.
- **`UsdSource`** ([usd_source.cpp](file:///home/ben/code/x3d-cpp/examples/asset_import/usd_source.cpp)): A USD/USDZ backend wrapping vendored tinyusdz (tydra `RenderScene`). It composes references/payloads/subLayers, resolves in-archive USDZ assets, and extracts a full **UsdPreviewSurface** material (scalars + one-hop `UsdUVTexture` connections). Guarded by `X3D_CPP_BUILD_USD`. See [ADR-0043](../decisions/0043-usd-material-portable-glsl-seam.md) for the material-fidelity ceilings and the portable-GLSL seam.

### Backend selection — priority registry

Backends are resolved by a pure [`BackendRegistry`](file:///home/ben/code/x3d-cpp/examples/asset_import/backend_registry.hpp) rather than a hardcoded extension map, so more than one backend can claim the same file type. Each backend declares a `priority(input)` (inspecting the whole input, so the `fixture:` prefix and file extensions share one path); `select()` picks the highest positive score, and `--backend <name>` forces a specific backend regardless of priority.

| Input | Default backend | Priority | Notes |
| --- | --- | --- | --- |
| `fixture:<name>` | `FixtureSource` | 100 | always available |
| `*.gltf`, `*.glb` | `CgltfSource` | 100 | assimp also claims these at priority 10 (fallback) |
| `*.usd/.usda/.usdc/.usdz` | `UsdSource` | 100 | needs `-DX3D_CPP_BUILD_USD=ON` |
| `*.obj/.fbx/.dae/…` | `AssimpSource` | 50 | needs `-DX3D_CPP_BUILD_ASSIMP=ON` |

Because cgltf outranks assimp for glTF, a bare `x3d_asset_import model.glb` uses cgltf; `--backend assimp model.glb` forces the assimp path. The seam's genericity is proven by a tolerant `cgltf`-vs-`assimp` differential swap-test ([backend_swap_test.cpp](file:///home/ben/code/x3d-cpp/examples/asset_import/tests/backend_swap_test.cpp)) over a committed `twobox.glb` — see [ADR-0044](../decisions/0044-asset-import-backend-selection.md).

## X3D 4.0 Mapping Table

The `emit` stage ([emit.cpp](file:///home/ben/code/x3d-cpp/examples/asset_import/emit.cpp)) translates the intermediate representation into a conformant X3D 4.0 hierarchy according to the following mapping:

| Input IR Element | → X3D 4.0 Node | Mapping Detail |
|---|---|---|
| `ImportNode` tree | Nested `Transform` | Matrix decomposed into `translation`, `rotation` (axis-angle), `scale`, and `scaleOrientation`. |
| Shared Mesh Index | `DEF` / `USE` on `Shape` | Meshes referenced multiple times are instanced to prevent geometry duplication. |
| `ImportMesh` | `Shape` | Hosts the geometry and appearance nodes. |
| Mesh Geometry | `IndexedTriangleSet` | Coordinates (`Coordinate.point`), Normals (`Normal`), and UVs (`TextureCoordinate.point`) mapped to vertices. |
| Phong Material | `Material` | Maps diffuse, emissive, specular, shininess, and transparency (1 - opacity). |
| PBR Material | `PhysicalMaterial` | Maps baseColor, metallic, roughness, emissiveColor, occlusionStrength, and transparency. UsdPreviewSurface `ior`/`clearcoat`/specular-workflow/`opacityMode` have no X3D node equivalent — carried only via `--emit-glsl` (see below). |
| Textures | Texture slots | Linked as relative `ImageTexture.url` references under material properties. |
| `ImportCamera` | `Viewpoint` | Maps viewpoint coordinate position and orientation; sets `fieldOfView`. |
| `ImportLight` | `DirectionalLight` / `PointLight` / `SpotLight` | Maps color, intensity, location, direction, radius, and attenuation properties. |

## Texture Pipeline

The consumer-side texture pipeline ([texture_pipeline.cpp](file:///home/ben/code/x3d-cpp/examples/asset_import/texture_pipeline.cpp)) decodes, recompresses, and rewrites texture references:
1. **Byte Resolution**: Reads external texture files relative to the model directory or extracts embedded blobs.
2. **Deduplication**: Content-hashes raw texture bytes using FNV-1a, collapsing identical textures onto a single file on disk and a single URL.
3. **Web-Safe Passthrough**: Copies existing web-safe formats (`png`, `jpeg`) untouched when `recompress` is disabled.
4. **Re-encoding**: Other formats are decoded via the SDK's `stb` decode seam (`x3d_stb`) and re-encoded to PNG via `stbi_write_png`.
5. **Asset Layout**: Saves unique files to `<outdir>/assets/<hash>.<ext>` and exposes a relative `TexturePlan` mapping for `emit()`.

## USD Material Port & Portable-GLSL Seam

USD/USDZ materials port to realtime through a stack of four ceilings, fully worked
out in [ADR-0043](../decisions/0043-usd-material-portable-glsl-seam.md):

1. **Input ceiling** — tinyusdz resolves only the `UsdPreviewSurface` shading model;
   MaterialX `standard_surface` / arbitrary UsdShade graphs are unsupported
   (flatten-to-UsdPreviewSurface or skip, matching the wider ecosystem).
2. **IR** — `ImportMaterial` is a UsdPreviewSurface-grade record carrying the fields
   tydra populates (`ior`, `clearcoat`, `clearcoatRoughness`, `opacityThreshold`,
   `useSpecularWorkflow`, specular texture, `opacityMode`), with `AlphaMode` derived
   from the opacity signals.
3. **Realtime BRDF** — the canonical
   [`usd_preview_surface.frag`](file:///home/ben/code/x3d-cpp/examples/cpu_raster/shaders/usd_preview_surface.frag)
   implements the Storm/glTF metallic-roughness reference BRDF. It is the **frozen
   contract of a portable-GLSL seam**: one shader source that runs byte-identically in
   the CPU rasterizer's [GLSL interpreter](shaders.md) *and* on desktop OpenGL
   (`poc_renderer`). The interpreter's `seedUniforms` binds the full texture-slot set
   (base/normal/emissive/metallic-roughness/occlusion + `uHas*Tex` guards) and seeds
   the UsdPreviewSurface fidelity uniforms to their spec fallbacks — so the direct
   render path stays metallic-roughness while the extras engage only when a host binds
   real values. At spec defaults the interpreter output is pixel-parity (< 1/255 MAD)
   with the native fixed-function PBR; the full `USDZ → X3D → interpreter` loop renders
   a real textured USDZ with normal/MR/occlusion maps landing.
4. **X3D material-model ceiling** — the X3D `PhysicalMaterial` node models only
   glTF-metallic-roughness (no `ior`/`clearcoat`/specular-workflow/`opacityMode`), so
   the `USD → X3D → renderer` path is capped there. The `--emit-glsl` mode carries full
   UsdPreviewSurface fidelity by baking those parameters as `const` into a
   self-contained per-material fragment shader (a text transform over the one canonical
   shader — [glsl_emit.cpp](file:///home/ben/code/x3d-cpp/examples/asset_import/glsl_emit.cpp)),
   the only faithful realtime channel past the X3D intermediate.

## The `x3d_cpp::authoring` Slim Target

The interface target `x3d_cpp::authoring` acts as a firewall separating authoring from runtime features.
- **Includes allowed**: Node construction, document/scene containers, XML/JSON/VRML codecs, math, and range validation.
- **Subsystems excluded**: `X3DExecutionContext`, extraction (`SceneExtractor`, `MeshBuilder`), script/SAI, sound, and physics.

### Footprint Gate
The automated footprint gate ([scripts/authoring-footprint.sh](file:///home/ben/code/x3d-cpp/scripts/authoring-footprint.sh)) protects the slim target from transitively linking in unwanted dependencies:
- **Symbol Scan**: Validates that the stripped binary contains zero occurrences of forbidden symbols like `SceneExtractor`, `parseDocument`, `parseFile`, or `ScriptSystem`.
- **Size Baseline**: Compares the `.text` segment size of the compiled smoke test against the baseline in [baseline.tsv](file:///home/ben/code/x3d-cpp/examples/asset_import/footprint/baseline.tsv), failing if size grows more than 10%.

## Key Files

| File Path | Role |
|---|---|
| [include/x3d/authoring.hpp](file:///home/ben/code/x3d-cpp/include/x3d/authoring.hpp) | Umbrella header exposing the slim `x3d_cpp::authoring` target surface. |
| [examples/asset_import/main.cpp](file:///home/ben/code/x3d-cpp/examples/asset_import/main.cpp) | CLI runner for file conversion, statistics logging, and `--verify` re-parsing. |
| [examples/asset_import/emit.cpp](file:///home/ben/code/x3d-cpp/examples/asset_import/emit.cpp) | Direct mapping logic from IR to the node graph. |
| [examples/asset_import/texture_pipeline.cpp](file:///home/ben/code/x3d-cpp/examples/asset_import/texture_pipeline.cpp) | Texture decoding, deduplication, and PNG re-encoding pipeline. |
| [scripts/authoring-footprint.sh](file:///home/ben/code/x3d-cpp/scripts/authoring-footprint.sh) | Automated footprint gate script validating symbol purity and size constraints. |

## CLI Usage

Run the asset import tool:
```bash
./build-asset-import/examples/asset_import/x3d_asset_import <input> \
    [-o out.x3d] \
    [--format xml|json|vrml|canonical] \
    [--assets-dir DIR] \
    [--no-textures] \
    [--recompress] \
    [--profile auto|interchange|immersive|full] \
    [--emit-glsl DIR] \
    [--verify] \
    [--stats]
```
`--emit-glsl DIR` writes one portable `<material>.frag` per material with the full
UsdPreviewSurface parameters baked in (the fidelity the X3D `PhysicalMaterial` node
cannot carry). The shaders run in the CPU rasterizer's GLSL interpreter and on desktop
GL — see [ADR-0043](../decisions/0043-usd-material-portable-glsl-seam.md).
For example, to convert and verify a synthetic cube:
```bash
./build-asset-import/examples/asset_import/x3d_asset_import fixture:cube -o /tmp/cube.x3d --stats --verify
```

## Relationship to ISO/IEC 19777-4

Rather than adopting the verbose, Java-style transliteration of the ISO/IEC 19777-4 C++ binding (which is MFC-legacy based and binds obsolete X3D v3.3 specifications), `x3d_cpp::authoring` targets headless, performant modern C++20 authoring:
- **Clean Namespace**: Uses unqualified types within `x3d::nodes::*` rather than Java-like prefixes (e.g. `CX3D...`).
- **RAII Lifetimes**: Standard C++ smart pointer (`std::shared_ptr`) ownership instead of explicit `.dispose()` calls.
- **Neutral Identifiers**: Standard C++ types (`std::vector`, struct tuples) are used to enforce a compile-time safe, fluent API.

This design is structurally isomorphic to the underlying UOM specs, ensuring that a future conformance wrapper or adapter can easily map our C++ model to standard-compliant 19777-4 bindings if needed.
