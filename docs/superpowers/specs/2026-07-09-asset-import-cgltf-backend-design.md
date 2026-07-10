# cgltf Import Backend + Priority-Registry Backend Selection — Design

**Date:** 2026-07-09
**Status:** approved (brainstorm)
**Subsystem:** `examples/asset_import/` (out-of-SDK authoring consumer)
**Relates to:** ADR seam-genericity pattern (frozen interface + 2 independent backends + CI swap-test, U1–U4); `2026-07-01-asset-import-authoring-target-design.md`.

## Motivation

The asset-import consumer selects an `ImportSource` backend with a hardcoded
if/else in `main.cpp` keyed on file extension: a `fixture:` prefix → `FixtureSource`,
USD extensions → `UsdSource`, everything else → `AssimpSource`. glTF (`.gltf`/`.glb`)
falls through to assimp.

Adding **cgltf** as a second glTF backend makes two extensions (`.gltf`, `.glb`)
claimable by *two* backends at once. That overlap is deliberate: it forces a real
**backend-selection abstraction** (priority + explicit override) instead of an
extension→backend map, and it gives the `ImportSource` seam its genericity proof —
cgltf and assimp independently satisfying the frozen interface for the *same* input,
verified by a tolerant differential swap-test (U1–U4).

cgltf is a single MIT-licensed header with no heavy dependencies (unlike assimp), so
it can be the **default-ON, first-class lightweight glTF path**; assimp stays
opt-in for its broader "40+ formats" coverage.

## Decisions (locked in brainstorm)

1. **Selection model:** priority registry + `--backend <name>` override. Each backend
   declares a priority for a given input; the registry auto-selects the highest
   positive; `--backend` bypasses priority to force a specific backend.
2. **cgltf priority > assimp for glTF:** `cgltf`=100, `assimp`=10 on `.gltf`/`.glb`.
   A bare `x3d_asset_import model.glb` uses cgltf; `--backend assimp` forces the old path.
3. **cgltf fidelity:** full parity — meshes, PBR metallic-roughness materials, textures
   (external URI + GLB bin buffer-view + base64 data-URI), node hierarchy with TRS,
   perspective cameras, and `KHR_lights_punctual` lights.
4. **Swap-test:** an invariant differential gate — one committed `.glb` loaded through
   both backends; assert structural invariants match within tolerance.
5. **Build gating:** new `X3D_CPP_BUILD_CGLTF`, **default ON** (header-only). assimp
   stays default OFF. Swap-test runs only when both are ON (CI builds a both-ON config).
6. **Fixture:** the test `.glb` is produced by a small committed generator script
   (reproducible), not an opaque committed binary.

## Architecture

A linear addition to the existing pipeline; only the *backend selection* step changes
shape.

```
input ── BackendRegistry.select(input) / .byName(--backend) ──▶ ImportSource
             (cgltf | assimp | usd | fixture)                        │
                                                                     ▼
                                                    ImportScene ──▶ emit ──▶ … (unchanged)
```

### Component 1 — `BackendRegistry` (new, pure, unit-testable)

`examples/asset_import/backend_registry.{hpp,cpp}` — no backend dependencies.

```cpp
namespace x3d::asset_import {

struct ImportBackend {
  std::string name;                                       // "cgltf","assimp","usd","fixture"
  std::function<int(const std::string& input)> priority;  // sees whole input; <=0 = declines
  std::function<std::unique_ptr<ImportSource>()> make;
};

class BackendRegistry {
public:
  void add(ImportBackend b);
  // Highest positive priority for `input`; nullptr if none claim it.
  const ImportBackend* select(const std::string& input) const;
  // Exact name match, ignoring priority (the --backend override path). nullptr if absent.
  const ImportBackend* byName(const std::string& name) const;
  std::vector<std::string> names() const;                 // for --help / error text
private:
  std::vector<ImportBackend> backends_;
};

} // namespace x3d::asset_import
```

- `priority` takes the **whole input string**, not just the extension, so `fixture:cube`
  and `model.glb` route through one uniform mechanism — no special-casing left in `main`.
- `select` returns the backend with the greatest positive priority; ties resolve to the
  first registered (registration order is deterministic, so ties are stable). Non-positive
  priorities are treated as "declines".
- The class is **pure** (holds only its vector); a unit test drives it with fake backends
  to prove the selection/override/decline logic without compiling any real backend.

### Component 2 — backend registration (gated, in `main`'s TU)

`registerBuiltinBackends(BackendRegistry&)` lives in `main.cpp`'s translation unit
(already compiled with every `HAVE_*` macro and linking all backend sources). Each `add()`
is wrapped in `#if X3D_ASSET_IMPORT_HAVE_*`:

| Backend | priority(input) |
|---|---|
| fixture  | `100` if `input` starts with `fixture:`, else `-1` |
| cgltf    | `100` if ext ∈ {`.gltf`,`.glb`}, else `-1` |
| assimp   | `10` if ext ∈ {`.gltf`,`.glb`}; `50` for its other formats (`.obj`,`.fbx`,…); else `-1` |
| usd      | `100` if ext ∈ {`.usd`,`.usda`,`.usdc`,`.usdz`}, else `-1` |

No static-initialization-order dependence — the registry is built explicitly at the top
of `main`. `--backend X` on an input X declines (priority ≤ 0) still forces X via
`byName` (e.g. `--backend assimp foo.glb` works even though cgltf outranks it).

### Component 3 — `CgltfSource` backend (new, gated)

`examples/asset_import/cgltf_source.{hpp,cpp}`, macro `X3D_ASSET_IMPORT_HAVE_CGLTF`.
Vendors `examples/asset_import/third_party/cgltf.h` (MIT); exactly one TU defines
`CGLTF_IMPLEMENTATION`. `load(path)`: `cgltf_parse_file` → `cgltf_load_buffers` → walk
into `ImportScene`:

- **Meshes:** each triangle primitive → one `ImportMesh`. Read POSITION, NORMAL,
  TEXCOORD_0, COLOR_0 accessors + indices; carry `materialIndex`.
- **Materials:** `pbr_metallic_roughness` → `PbrParams{baseColor, metallic, roughness}`;
  emissive factor; `alpha_mode`/`alpha_cutoff` → `AlphaMode`. Texture slots baseColor,
  normal, emissive, occlusion, metallicRoughness → `TextureSlots`.
- **Textures / images:** external `uri` → `TextureRef.externalPath`; GLB bin buffer-view
  or base64 `data:` URI → `EmbeddedTexture{bytes}` + `TextureRef.embeddedIndex`. A tiny
  inline base64 decoder handles data-URIs; **no image decode here** — bytes are decoded
  downstream by the existing stb texture pipeline.
- **Hierarchy:** nodes + child indices; per-node TRS or matrix composed into column-major
  `Mat4`. glTF is column-major, right-handed, Y-up — the same convention as our `Mat4` and
  X3D — so **no axis flip / transpose**. Synthetic root when the glTF scene has >1 root node.
- **Cameras:** perspective cameras → `ImportCamera{yfov, znear, zfar, world}` (world from
  the owning node's accumulated transform).
- **Lights:** `KHR_lights_punctual` → `ImportLight` (directional/point/spot; color;
  intensity; `range`→`radius`; inner/outer cone → `beamWidth`/`cutOffAngle`).

### Component 4 — `main.cpp` dispatch (simplified)

The if/else block is replaced by:

```cpp
BackendRegistry reg; registerBuiltinBackends(reg);
const ImportBackend* b = backendFlag.empty() ? reg.select(input) : reg.byName(backendFlag);
if (!b) { /* error: no backend for <input>; known backends: reg.names(); hint the OFF gate */ }
source = b->make();
scene  = source->load(inputForBackend(input));   // strips "fixture:" for the fixture backend
```

New CLI: `--backend <name>` (documented in `--help` with the priority note). Unknown
`--backend` value → error listing `reg.names()`.

## Testing

- **`tests/backend_registry_test.cpp`** (ungated, pure): fake backends prove
  select-by-priority (cgltf-analog 100 beats assimp-analog 10 on `.glb`), `byName`
  override, decline (`-1`) handling, `fixture:` routing, and unknown-input → nullptr.
- **`tests/backend_swap_test.cpp`** (gated `HAVE_CGLTF && HAVE_ASSIMP`): load
  `assets/fixtures/<name>.glb` through both backends; assert within tolerance:
  mesh count, total triangle count, material count, per-material PBR params (±1e-4),
  node count, camera count, light count. Tolerant to vertex ordering and material naming.
- **`tests/cgltf_source_test.cpp`** (gated `HAVE_CGLTF`): cgltf loads the fixture and
  populates every ImportScene section (geometry non-empty, PBR present, ≥1 embedded or
  external texture, hierarchy depth ≥ 2, ≥1 camera, ≥1 light).
- **Fixture generator:** `assets/fixtures/gen_gltf_fixture.py` (or equivalent) writes the
  committed `.glb`; documented so the binary is reproducible, not opaque.

## Build & CI

- Top `CMakeLists.txt`: `option(X3D_CPP_BUILD_CGLTF "Build cgltf glTF import backend" ON)`.
- `examples/asset_import/CMakeLists.txt`: when ON, append `cgltf_source.cpp` to
  `ASSET_IMPORT_SOURCES`, set `HAVE_CGLTF=1`, define `X3D_ASSET_IMPORT_HAVE_CGLTF`; add the
  `x3d_assetimport_cgltf` + `x3d_assetimport_backend_registry` test targets; add
  `x3d_assetimport_backend_swap` only when `HAVE_CGLTF AND HAVE_ASSIMP`.
- `scripts/validate-examples.sh`: add a both-backends-ON asset-import configuration so the
  examples-gate actually exercises the swap-test (the default build stays cgltf-only).
- **Footprint gate is unaffected:** backends never link `x3d_cpp::authoring`; the emit TU
  still includes only `x3d/authoring.hpp`. The plan verifies the symbol scan still passes.

## Docs (docs-are-part-of-the-diff)

- `NOTICE`: add cgltf (MIT). (DoD — third-party dep.)
- `docs/wiki/subsystems/asset-import.md`: backend registry, priority table, `--backend`,
  cgltf backend, swap-test.
- `examples/asset_import/README.md`: backend table + priority + `--backend`.
- `docs/sdk/v1-capabilities.md`: glTF is now first-class (cgltf, default-ON).
- New ADR `docs/wiki/decisions/NNNN-asset-import-backend-selection.md`: priority registry +
  swap-test genericity decision; add a `coverage.md` Decisions row + `mkdocs.yml` nav entry.

## Scope / non-goals

- No changes to the `ImportScene` IR or `emit` — cgltf produces the existing IR.
- No new image-decode dependency — data-URI base64 is decoded inline; texel decode stays in
  the stb pipeline.
- glTF morph targets, skins/animation, and KHR extensions beyond `lights_punctual` are out
  of scope for this cut (the IR carries none of them today).
- Not promoted into the core SDK seam swap-test CI jobs — asset_import is an out-of-SDK
  example, so its gate lives in `validate-examples`/examples-gate.
