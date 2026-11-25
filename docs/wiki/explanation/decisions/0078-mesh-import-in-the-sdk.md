---
title: "ADR-0078: Mesh import is an SDK component"
summary: The glTF/OBJ/FBX/USD importer moves from examples/asset_import into runtime/import as x3d_cpp::import, with glTF built in, Assimp and USD as source-tree backends, and `x3d import` as its CLI.
tags: [adr, import, gltf, usd, assimp, cli, build]
updated: 2026-11-25
related:
  - 0042-authoring-target.md
  - 0043-usd-material-portable-glsl-seam.md
  - 0044-asset-import-backend-selection.md
  - ../../reference/subsystems/asset-import.md
---

# ADR-0078: Mesh import is an SDK component

- Status: Accepted
- Date: 2026-11-25
- Amends: [ADR-0042](0042-authoring-target.md) (the `x3d_asset_import` tool),
  [ADR-0043](0043-usd-material-portable-glsl-seam.md) (where the canonical shader lives),
  [ADR-0044](0044-asset-import-backend-selection.md) (where the registry lives, and the
  fixture backend and `X3D_CPP_BUILD_CGLTF` flag)

## Context

The SDK exports glTF (`x3d extract -o model.glb`, the glTF export subsystem) but imported
models only through `examples/asset_import`, an out-of-SDK example that was off by
default. The example's intermediate scene had no skins, morph targets or animation, so
the round trip was one way: an animated or skinned X3D scene exported to glTF could not
come back. Hosts that wanted import had to copy example code.

## Decision

1. **The importer is `runtime/import/`, built as `x3d_cpp::import`.** It holds the
   intermediate scene (`ImportScene`), the backend seam (`ImportSource`,
   `BackendRegistry`), X3D emission, the texture plan, per-material GLSL, and the glTF
   backend. It links the authoring runtime and no parser or execution context, is
   installed with the SDK, and `x3d/sdk.hpp` re-exports it (`sdk::importFile`).
2. **glTF is built in; Assimp and USD are source-tree backends.** cgltf is a vendored
   header with no dependency, so the glTF backend is always built (the
   `X3D_CPP_BUILD_CGLTF` flag is gone). The Assimp backend needs a system Assimp and the
   USD backend fetches and patches tinyusdz, so they are the source-tree libraries
   `x3d_import_assimp` and `x3d_import_usd` behind `X3D_CPP_BUILD_ASSIMP` and
   `X3D_CPP_BUILD_USD`. They are not installed, and a host adds them to a registry with
   `addAssimpBackend()` / `addUsdBackend()`. The registry rules of ADR-0044 (highest
   positive priority, `--backend` by name, glTF ahead of Assimp) are unchanged; the
   `fixture:` backend is gone, since the tests build `ImportScene`s directly.
3. **The intermediate scene carries everything the exporter writes.** Skins, morph
   targets, animations (TRS, weights, and material, light and camera fields), unlit
   materials, multiple UV sets, vertex colours, texture samplers and lines and points
   are in the IR, and emission maps them to `HAnimHumanoid`, interpolators and ROUTEs.
   What X3D cannot hold is clamped or approximated and counted in
   `EmitReport::warnings`; it never throws on content.
4. **`x3d import` is the CLI.** It replaces `x3d_asset_import`, with its options (canonical
   output is left to `x3d canonicalize`)
   plus `--autoplay`, `--samples` and `--list-backends`, and picks the smallest general
   profile that fits.
5. **The round trip is the oracle.** `runtime/import/tests/RoundTripOracle.hpp` exports
   an X3D scene to glb, imports it, and compares both through `RuntimeSession` frame by
   frame (world-space triangles, lights, camera). A source that renders nothing fails
   the oracle, so a broken fixture cannot pass vacuously.
6. **The canonical `usd_preview_surface.frag` moves to `runtime/import/shaders/`.** The
   SDK embeds it for `materialGlsl()`; the CPU rasterizer test and the PoC renderer read
   it from there.

## Consequences

- `x3d extract` and `x3d import` round-trip static, animated, morphing and skinned
  scenes through glTF; the oracle runs in the behavior suite.
- The Assimp and USD backends are gated by the GitHub `import` job (`import` preset,
  `mise run build-import`, part of `ci-all`), not by `mise run ci`, because they need a
  system package and a network fetch.
- Fixes to tinyusdz live in one vendored patch, `runtime/import/usd/tinyusdz.patch`.
- `examples/asset_import/` is removed. The authoring footprint gate now scans
  `x3d_authoring_link_contract`, which already linked only `x3d_cpp::authoring`.
