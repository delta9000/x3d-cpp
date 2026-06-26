---
title: Topology Classification
summary: Minimal enum that classifies extracted mesh primitives as Triangles, Lines, or Points so the extraction pipeline and renderers agree on draw mode without an ad-hoc integer.
tags: [subsystem, extract, topology, mesh, lines, points]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/extract.md
---

# Topology Classification

## Purpose

`Topology` is a three-value scoped enum that labels the draw mode of every mesh the extraction pipeline produces. It answers the question "what primitive does each run of indices describe?" so `MeshBuilder`, `PackedMesh`, `MeshData`, and consumer renderers all share a single vocabulary without encoding the answer as a raw integer or a boolean pair.

The enum was factored into its own header (Phase 1 / B4) so both the AoS path (`MeshData` in `RenderItem.hpp`) and the binary-slab path (`PackedMesh`) can depend on it without either depending on the other. The default value `Triangles` was chosen so every mesh that existed before B4 remains byte-for-byte unchanged — no existing golden output was touched.

## Key files

| File | Role |
|---|---|
| `runtime/extract/Topology.hpp` | Defines `enum class Topology { Triangles, Lines, Points }` in `x3d::runtime::extract`. The entire subsystem is this one 8-line header. |
| `runtime/extract/RenderItem.hpp` | Includes `Topology.hpp`; `MeshData::topology` carries the classification for AoS geometry. |
| `runtime/extract/PackedMesh.hpp` | Includes `Topology.hpp`; `PackedMesh::topology` carries the classification for binary-slab geometry. |
| `runtime/extract/MeshBuilder.hpp` | Sets `topology` to `Lines` or `Points` in the B4 line/point branches (`IndexedLineSet`, `LineSet`, `PointSet`); leaves the default `Triangles` for all composed/parametric geometry. |
| `runtime/extract/TextExtract.hpp` | Explicitly sets `mesh.topology = Topology::Triangles` to make the glyph-quad path unambiguous. |
| `runtime/extract/tests/mesh_builder_b4_test.cpp` | Primary acceptance suite for the B4 topology extension. |
| `runtime/extract/tests/mesh_builder_tc1_test.cpp` | Confirms lines/points carry no implicit UV and that topology propagates through the texcoord path. |
| `runtime/extract/tests/text_extract_test.cpp` | Asserts `Topology::Triangles` on glyph-quad output. |

## Interfaces and seams

### Exposed interface

```cpp
// runtime/extract/Topology.hpp
namespace x3d::runtime::extract {
enum class Topology { Triangles, Lines, Points };
} // namespace x3d::runtime::extract
```

That is the complete public surface of this subsystem. There are no functions, no class members, no registration calls.

### Where the value lives on descriptors

`MeshData` (the AoS extraction result, defined in `runtime/extract/RenderItem.hpp`):

```cpp
Topology topology = Topology::Triangles; // B4: Triangles (default) / Lines / Points.
```

`PackedMesh` (the binary-slab geometry descriptor, defined in `runtime/extract/PackedMesh.hpp`):

```cpp
Topology topology = Topology::Triangles;
```

Both fields default to `Triangles`, so code written before B4 is unaffected.

### Seam points

- **MeshBuilder → MeshData** — `buildLocalMesh()` in `runtime/extract/MeshBuilder.hpp` is the only producer for the AoS path. It sets `mesh.topology = Topology::Lines` in the `IndexedLineSet` and `LineSet` branches, and `mesh.topology = Topology::Points` in the `PointSet` branch. All other branches (T1/T2 composed sets, T4 analytic primitives, Text glyph quads) leave the default `Triangles`.
- **Embedder resolver → PackedMesh** — an embedder supplying binary geometry via the `externalGeometryResolver` seam fills `PackedMesh::topology` directly. The SDK does not inspect it; the consumer reads it at draw time.
- **Consumer renderer** — the renderer reads `MeshData::topology` (or `PackedMesh::topology`) and selects `GL_TRIANGLES`, `GL_LINES`, or `GL_POINTS`. The `RenderItem.hpp` header documents the exact shading-path selector the consumer must honor:
  ```
  topology != Triangles  OR  !hasNormals  =>  bind the UNLIT program,
                                               skip the normal-matrix and
                                               light uniforms, and disable
                                               GL_CULL_FACE.
  ```
  Additionally, the producer sets `solid = false` on every line and point mesh, so a consumer's existing cull-disable path (solid=false ⇒ no `GL_CULL_FACE`) covers line/point meshes without an extra branch; the topology check is the belt-and-braces guard.

## How it is tested

- `ctest --preset dev -R x3d_mesh_builder_b4` — B4 acceptance suite (`runtime/extract/tests/mesh_builder_b4_test.cpp`). Covers:
  - Default topology is `Triangles`; a triangle mesh is byte-identical to pre-B4 output.
  - `IndexedLineSet`: each -1-delimited `coordIndex` run becomes consecutive vertex pairs; `topology=Lines`, `hasNormals=false`, `solid=false`.
  - `IndexedLineSet` honors per-vertex `Color` and `ColorRGBA` (alpha preserved).
  - `LineSet`: `vertexCount` partitions the implicit coord run into polylines.
  - `PointSet`: whole point array becomes a 0..N-1 `GL_POINTS` run; `topology=Points`; `Color` honored.
  - Degenerate guards: 1-vertex polyline, out-of-range indices, and empty `PointSet` all yield empty meshes with no out-of-bounds read, and the geometry type remains recognized.

- `ctest --preset dev -R x3d_mesh_builder_tc1` — texcoord path (`runtime/extract/tests/mesh_builder_tc1_test.cpp`). Asserts that `IndexedLineSet` and `PointSet` carry `topology=Lines`/`topology=Points` and that no implicit UVs are generated for non-triangle primitives.

- `ctest --preset dev -R x3d_text_extract` — Text extraction (`runtime/extract/tests/text_extract_test.cpp`). Asserts `topology == Topology::Triangles` on glyph-quad output, confirming the explicit assignment in `TextExtract.hpp` is in force.

## Related specs and ADRs

- Spec: `docs/superpowers/specs/2026-06-14-m25-extraction-poc-renderer-design.md` — the M2.5 extraction design that defines the `MeshData` + `RenderItem` contract; B4 (line/point topology) is described in the MeshBuilder scope section.
- See [Extract subsystem](../subsystems/extract.md) for the full extraction pipeline (`SceneExtractor` → `MeshBuilder` → `PackedMesh` / `MeshData` → `RenderItem`) that this enum is part of.
