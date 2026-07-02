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
- **`AssimpSource`** ([assimp_source.cpp](file:///home/ben/code/x3d-cpp/examples/asset_import/assimp_source.cpp)): A backend that wraps the Assimp library. It parses 40+ standard 3D formats, triangulates meshes, computes missing normals, and maps them to the intermediate representation. Guarded by `X3D_ASSET_IMPORT_HAVE_ASSIMP`.

## X3D 4.0 Mapping Table

The `emit` stage ([emit.cpp](file:///home/ben/code/x3d-cpp/examples/asset_import/emit.cpp)) translates the intermediate representation into a conformant X3D 4.0 hierarchy according to the following mapping:

| Input IR Element | → X3D 4.0 Node | Mapping Detail |
|---|---|---|
| `ImportNode` tree | Nested `Transform` | Matrix decomposed into `translation`, `rotation` (axis-angle), `scale`, and `scaleOrientation`. |
| Shared Mesh Index | `DEF` / `USE` on `Shape` | Meshes referenced multiple times are instanced to prevent geometry duplication. |
| `ImportMesh` | `Shape` | Hosts the geometry and appearance nodes. |
| Mesh Geometry | `IndexedTriangleSet` | Coordinates (`Coordinate.point`), Normals (`Normal`), and UVs (`TextureCoordinate.point`) mapped to vertices. |
| Phong Material | `Material` | Maps diffuse, emissive, specular, shininess, and transparency (1 - opacity). |
| PBR Material | `PhysicalMaterial` | Maps baseColor, metallic, roughness, emissiveColor, and transparency. |
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
    [--verify] \
    [--stats]
```
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
