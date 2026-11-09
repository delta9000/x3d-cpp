---
title: Asset-Import Subsystem
summary: Asset-import, scene-graph emission, texture re-encoding pipeline, and the footprint-gated x3d_cpp::authoring target.
tags: [subsystem, asset-import, authoring, texture-pipeline, CLI, cmake]
updated: 2026-07-01
related:
  - ../architecture.md
  - ../decisions/0041-authoring-target.md
---

# Asset import

`examples/asset_import` converts foreign assets into an X3D document through
an engine-independent intermediate representation. Conversion is separate
from scene loading and execution.

## Data flow

`ImportSource` produces `ImportScene`; the texture pipeline creates a texture
plan; `emit()` builds X3D nodes; the selected writer serializes the document.
The intermediate representation contains values and indices rather than
third-party library types.

## Backend selection

`BackendRegistry::select` chooses the highest positive priority, with the first
registered backend winning ties. `--backend` selects a backend by name.

| Input | Backend | Build option |
|---|---|---|
| `fixture:` | FixtureSource | Always available |
| glTF/GLB | CgltfSource | `X3D_CPP_BUILD_CGLTF`, on by default |
| USD/USDZ | UsdSource | `X3D_CPP_BUILD_USD` |
| Assimp-supported files | AssimpSource | `X3D_CPP_BUILD_ASSIMP` |

Cgltf outranks Assimp for glTF. The optional Assimp implementation remains a
fallback for those files. See
[ADR-0044](../decisions/0044-asset-import-backend-selection.md).

## X3D mapping

`examples/asset_import/emit.cpp` maps the intermediate scene to these nodes.

| Intermediate value | X3D representation |
|---|---|
| Node hierarchy | Transform hierarchy |
| Mesh | Shape and IndexedTriangleSet |
| Positions, normals, UVs | Coordinate, Normal, TextureCoordinate |
| Phong material | Material |
| Metallic-roughness material | PhysicalMaterial |
| Texture reference | ImageTexture |
| Camera | Viewpoint |
| Light | DirectionalLight, PointLight, or SpotLight |

## Textures

`texture_pipeline.cpp` resolves external and embedded bytes, deduplicates by
content hash, and writes assets under `assets/`. PNG/JPEG bytes pass through
unless recompression is requested. Other inputs use the optional stb decoder
and are encoded as PNG. The writer receives relative asset URLs.

## USD materials and portable GLSL

The USD implementation imports UsdPreviewSurface values and texture links.
PhysicalMaterial carries the metallic-roughness subset. `--emit-glsl` writes
per-material shader sources with additional preview-surface parameters baked
in. It does not expand the X3D material model. See
[ADR-0043](../decisions/0043-usd-material-portable-glsl-seam.md) for the decision.

## Authoring target

`x3d_cpp::authoring` and `x3d/authoring.hpp` provide node construction, document
containers, range checks, and writers. They exclude parser, execution, and
extraction libraries. `scripts/authoring-footprint.sh` rejects excluded
subsystem symbols in the authoring smoke executable; it has no byte-size budget.

## Build and CLI

See the [consumer README](https://github.com/delta9000/x3d-cpp/blob/main/examples/asset_import/README.md) for build
commands and command options. Run `mise tasks` for the maintained task catalog.

## Key files

The consumer owns its format conversion and asset output.

| Source | Role |
|---|---|
| `examples/asset_import/import_source.hpp` | Intermediate scene and source interface |
| `examples/asset_import/backend_registry.hpp` | Backend selection |
| `examples/asset_import/emit.cpp` | X3D graph emission |
| `examples/asset_import/texture_pipeline.cpp` | Asset byte processing |
| `include/x3d/authoring.hpp` | Authoring façade |
