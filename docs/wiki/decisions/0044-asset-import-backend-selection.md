---
title: "ADR-0044: Asset-Import Backend Selection — Priority Registry + cgltf glTF Path"
summary: Replace the extension if/else backend dispatch with a priority-based BackendRegistry (plus a --backend override) so that multiple backends can claim the same file, and add a default-ON, header-only cgltf glTF backend that outranks assimp for glTF; the ImportSource seam's genericity is proven by a tolerant cgltf-vs-assimp differential swap-test.
tags: [adr, asset-import, backends, gltf, cgltf, seam]
updated: 2026-07-09
related:
  - ../subsystems/asset-import.md
  - 0042-authoring-target.md
  - 0043-usd-material-portable-glsl-seam.md
  - 0022-scriptengine-second-backend-swap-test.md
---

# ADR-0044: Asset-Import Backend Selection — Priority Registry + cgltf glTF Path

## Status

Accepted

## Context

The asset-import consumer chose its `ImportSource` backend with a hardcoded
if/else in `main.cpp`, keyed on the input: a `fixture:` prefix selected
`FixtureSource`, USD extensions selected `UsdSource`, and everything else fell
through to `AssimpSource`. That mapping assumed each file type has exactly one
backend, so glTF (`.gltf`/`.glb`) could only ever route to assimp.

We wanted a second, lightweight glTF backend built on
[cgltf](https://github.com/jkuhlmann/cgltf) — a single MIT-licensed header with
no heavy dependencies, unlike assimp. Introducing it makes two extensions
claimable by two backends at once, which the extension→backend map cannot
express. It also gives the `ImportScene`/`ImportSource` seam a genericity proof
of the same shape the SDK's other seams already carry ([ADR-0022](0022-scriptengine-second-backend-swap-test.md)):
two independent backends satisfying one frozen interface, checked by CI.

## Decision

1. **Priority `BackendRegistry` replaces the if/else.**
   `backend_registry.hpp`
   defines an `ImportBackend { name, priority(input), make() }` descriptor and a
   pure registry with `select(input)` (highest positive priority wins),
   `byName(name)`, and `names()`. `priority` inspects the **whole input string**,
   so the `fixture:` prefix and file extensions resolve through one uniform path
   — no special-casing remains in `main`. The registry class holds only its
   vector, so its selection logic is unit-tested with fake backends independent
   of any backend's build gate.

2. **Registration is explicit and macro-gated.** `registerBuiltinBackends` in
   `main.cpp` adds each backend inside `#if X3D_ASSET_IMPORT_HAVE_*`, so there is
   no static-initialization-order dependence and a backend compiled out simply
   never registers.

3. **cgltf outranks assimp for glTF.** Priorities: `fixture`=100 on the
   `fixture:` prefix; `cgltf`=100 on `.gltf`/`.glb`; `assimp`=10 on `.gltf`/`.glb`
   and 50 on its other formats; `usd`=100 on the USD extensions. A bare
   `x3d_asset_import model.glb` therefore uses cgltf; assimp remains the
   broader-format path and the glTF fallback.

4. **`--backend <name>` forces a specific backend**, bypassing priority (via
   `byName`), so `--backend assimp model.glb` still works even though cgltf
   outranks it — which is exactly what the swap-test needs.

5. **cgltf is default ON.** `X3D_CPP_BUILD_CGLTF` defaults ON because the header
   adds negligible build cost, making glTF a first-class lightweight path with no
   assimp requirement. `X3D_CPP_BUILD_ASSIMP` and `X3D_CPP_BUILD_USD` stay OFF by
   default.

6. **Genericity is proven by a tolerant differential swap-test.**
   `backend_swap_test.cpp` (compiled only when both `HAVE_CGLTF` and
   `HAVE_ASSIMP`) loads one committed `twobox.glb` through both backends and
   asserts they agree on structural invariants — total triangle count, camera and
   light counts, and the authored material's PBR metallic/roughness (±1e-4) —
   tolerant to vertex ordering and material naming. Material **count** is
   deliberately not asserted equal: assimp's glTF importer appends an unnamed
   default material that cgltf does not, while both recover the authored material
   identically. The gate runs in the examples-gate (`validate-examples.sh`), not
   the core seam-swap CI jobs, because asset-import is an out-of-SDK example.

## Consequences

- glTF import no longer requires assimp; the default build converts `.gltf`/`.glb`
  through cgltf, carrying meshes, PBR metallic-roughness materials, textures
  (external URI, GLB bin buffer-view, and base64 data: URI), node hierarchy with
  TRS, perspective cameras, and `KHR_lights_punctual` lights.
- Adding a future backend is a one-line `reg.add({...})` with a priority function;
  overlap resolves by priority, not by editing a dispatch chain.
- The swap-test converts backend *disagreement* into a build failure, so a
  regression in either backend's glTF handling is caught mechanically.
- The `ImportScene` IR and `emit` are unchanged — cgltf produces the existing IR,
  so the whole downstream pipeline (texture plan, emit, writers, `--verify`) is
  untouched.
- Morph targets, skins/animation, and KHR extensions beyond `lights_punctual` are
  out of scope for this cut, as the IR carries none of them.
