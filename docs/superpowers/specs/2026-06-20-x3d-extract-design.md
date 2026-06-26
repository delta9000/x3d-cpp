# `x3d extract` — X3D → STL geometry export (2nd utility set, command 1)

**Date:** 2026-06-20
**Status:** Approved design; build sequenced behind the in-flight `strengthen-validate` work (both touch `tools/x3d_cli.cpp`).
**Goal:** Export the tessellated geometry of any X3D scene to a single merged binary STL — the binary/ingestion machinery run *backwards* (`MeshBuilder`/`SceneExtractor` → mesh file), validated by a self-oracle round-trip through our own `StlReader`.

## Why
The 2nd utility set is "balanced": one external-oracle command (`canonicalize`, next) + one high-value self-oracle command (`extract`, this). `extract` ships a real "X3D → printable mesh" tool, dogfoods the renderer-facing `SceneExtractor` seam, and reuses the just-shipped `StlReader` (for its oracle) — zero new heavy machinery, no JDK/byte-match risk.

## Architecture — thin over `SceneExtractor`
`SceneExtractor` already walks the scene, tessellates every geometry `MeshBuilder` supports (IndexedFaceSet, triangle/line/point sets, Box/Sphere/Cone/Cylinder, ElevationGrid, …), and produces `RenderItem`s carrying local-frame `MeshData` + a world transform. So:
```
x3d extract = parseFile -> SceneExtractor (headless) -> for each RenderItem:
                 transform MeshData triangles to world space -> accumulate
              -> write one merged binary STL
```
The ONLY new code is a small **binary-STL writer** (the inverse of `runtime/ext/codecs/StlReader.hpp`). This uses `SceneExtractor` exactly as a renderer would — the dogfooding payoff.

**STL writer placement:** STL *export* adds no X3D nodes/dialect — it's a consumer-side op like a renderer — so it stays **core** (NOT behind the `X3D_CPP_BUILD_EXT` firewall). A small `tools/x3d-cli/stl_write.hpp` (or `x3d::util` mesh-export helper) takes a triangle list → binary STL bytes; both the CLI and the writer↔reader unit test use it. (The self-oracle test re-imports via `StlReader`, which IS ext-gated, so that test runs under `X3D_CPP_BUILD_EXT`.)

## Behavior
`x3d extract <in> [-o out.stl]` (stdout if no `-o`). v1 emits **one merged, world-space binary STL**: STL has no grouping/materials, and a single triangle soup is exactly what 3D-printing / mesh tools consume. Binary (not ASCII) because it is both the standard print format and what `StlReader` parses (so the self-oracle closes). Each STL facet: a normal (use the triangle's geometric normal) + 3 world-space vertices.

Exit codes mirror the suite: 0 ok, 2 parse/IO failure, 1 usage error. No-geometry scene → a valid empty STL (0 triangles) + a stderr note. Geometry types `MeshBuilder` skips (NURBS, 2D primitives) → counted and reported to stderr, never fatal.

## The self-oracle gate (extract's "prove it")
Java-free round-trip: `extract` X3D → STL, then **re-import the STL via `StlReader` → `PackedMesh`, and compare geometry** against the mesh `SceneExtractor` produced:
- triangle count equal;
- AABB equal within tolerance;
- vertex positions equal within tolerance (the STL is the merged world-space mesh; the comparison is set/bounds-based given STL has no vertex identity).

This validates the STL writer ↔ reader agree end-to-end. It runs under `X3D_CPP_BUILD_EXT` (needs `StlReader`). A `mise run cli-gate`-style extract check over a corpus subset (no crash; non-empty geometry preserved; round-trip holds) is the differential layer; an optional external mesh oracle (assimp/meshlab tri-count+bounds) is a documented follow-on, not v1.

## Testing
- **Unit (core):** extract a `Box` / `Sphere` / `IndexedFaceSet` fixture → assert exact triangle count + AABB. The binary-STL writer gets its own writer→`StlReader`→read-back unit test (a hand-built triangle list round-trips bit-exactly through write+read).
- **Self-oracle (ext-gated):** the export→`StlReader`→compare round-trip on those fixtures + a corpus subset.
- **CLI test:** `tools/tests/x3d_cli_test.sh` — `x3d extract` produces a non-empty STL whose facet count matches a known fixture; malformed input exits non-zero without crashing.

## v1 scope cuts (deliberate)
- **Merged single mesh, binary STL only.** No materials, no per-shape structure — that is OBJ's job, deferred. ASCII STL deferred. OBJ (per-shape groups + `Material`) and PLY are explicit follow-ons.
- No external (non-Java) mesh oracle in v1 — the self-round-trip is the gate.
- `canonicalize` (the external-oracle command) is the next, separate spec.

## Success criteria
1. `x3d extract scene.x3d -o out.stl` writes a valid binary STL of the scene's merged world-space geometry; mesh tools / `StlReader` load it.
2. The self-oracle round-trip (export → `StlReader` → compare) holds on the fixtures + a corpus subset.
3. Gates: `mise run build` green, golden zero-drift, generated layer untouched, ext quarantine intact (the export path is core; only the oracle test is ext-gated).
