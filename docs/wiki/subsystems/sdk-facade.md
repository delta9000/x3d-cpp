---
title: SDK Façade
summary: The single public header consumers include — x3d::sdk — which re-exports the curated v1 embedder surface from namespace x3d::sdk.
tags: [subsystem, sdk, facade, public-api]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/cli-suite.md
  - ../decisions/0001-ext-firewall.md
  - ../guides/gate-system.md
---

# SDK Façade

## Purpose

The SDK façade is the boundary between the x3d-cpp runtime internals and every embedder (renderer, tool, or script). It owns exactly one file — `include/x3d/sdk.hpp` — and one CMake target — `x3d_cpp::sdk` (an `INTERFACE` library). Embedders include that one header and link that one target; everything else is an implementation detail they do not name. The façade adds no compiled translation unit of its own: it re-exports curated `using` declarations from `namespace x3d::sdk`, covering loading, the execution context, extraction descriptors, serialization writers, and the five embedder seams. Symbols are tagged `[STABLE]` (frozen pre-v2; breaking change requires a major version bump) or `[EXPERIMENTAL]` (shape may still evolve; embedder wiring is maturing).

## Key files

| File / directory | Role |
|---|---|
| `include/x3d/sdk.hpp` | The single consumer-facing header; all re-exports live here |
| `include/x3d/tests/sdk_facade_test.cpp` | Compile-and-run surface-lock test (ctest `x3d_sdk_facade`) |
| `examples/01_load_validate_convert.cpp` | Worked example: load → validate → convert (ctest `x3d_example_01_load_validate_convert`) |
| `examples/02_extract_render_feed.cpp` | Worked example: extract → feed a renderer (ctest `x3d_example_02_extract_render_feed`) |
| `examples/03_attach_behavior_tick.cpp` | Worked example: subclass `System`, register, drive ticks (ctest `x3d_example_03_attach_behavior_tick`) |
| `examples/README.md` | Index of the three worked examples |
| `CMakeLists.txt` (line ~108, ~1460–1481) | `add_library(x3d_cpp_sdk INTERFACE)` + `x3d_cpp::sdk` alias, T-SDK ctests, example loop |

## Interfaces and seams

### Exposed interface

The canonical usage pattern (from the `sdk.hpp` file header):

```cpp
#include "x3d/sdk.hpp"
namespace sdk = x3d::sdk;

// 1. Load
sdk::X3DDocument doc = sdk::parseFile("scene.x3dv");

// 2. Wire execution context
sdk::X3DExecutionContext ctx;
ctx.buildSceneGraph(doc.scene);
ctx.buildFrom(doc.scene);          // returns sdk::BridgeResult
ctx.addSystem(...);                // register behaviors before first tick

// 3. Extract
sdk::SceneExtractor ex(ctx, doc.scene);
sdk::RenderDelta f0 = ex.fullSnapshot();   // upload f0.added
while (running) {
    ctx.tick(now);
    sdk::RenderDelta d = ex.delta();       // apply incremental changes
}
```

The complete `x3d::sdk` namespace, grouped by area:

**Loading** (`[STABLE]`)

- `parseFile(path)` / `parseDocument(text, hint, base, resolver)` — parse any of the four X3D encodings (XML, ClassicVRML, VRML97, JSON; gzip-aware) into an `X3DDocument`.
- `Encoding` — `enum class { XML, ClassicVRML, VRML97, JSON, Unknown }`.
- `localFileProtoResolver` / `ProtoDeclarationResolver` — the default file-local EXTERNPROTO resolver callback type.
- `X3DDocument` — `{ version, profile, head, scene, rangeWarnings, protoWarnings, inlineWarnings }`.
- `Scene`, `Profile`, `Head`, `Component`, `Unit`, `Meta`, `Route` — document model types.
- `X3DNode` (global namespace) — generated node base; `fields()`, get/set, `getDEF`/`setDEF`.
- `RangeDiagnostic` (global namespace) — one out-of-range value captured by the lenient read path.
- `ProtoWarning` — a PROTO/EXTERNPROTO expansion diagnostic.

**Serialization** (`[STABLE]`)

- `XmlWriter`, `JsonWriter`, `VrmlWriter`, `CanonicalXmlWriter` — reflection-driven writers; each exposes `writeDocument(doc) -> std::string`.

**Execution context** (`[STABLE]`)

- `X3DExecutionContext` — owns the route graph + event cascade; setup methods `buildSceneGraph`, `buildFrom`, `addSystem`, `addScriptSystem`; per-frame `tick(now)`; pull surface (`dirtyTracker()`, `worldTransform()` / `worldTransformAny()` / `localBounds()` / `worldBounds()`, `boundViewpoint()`, `viewMatrix()`, `pick(ray)`); input setters `setPointer`, `setPointerButton`, `setKey`.
- `BridgeResult` — `{ routesAdded, rejected[] }` returned by `buildFrom`.
- `RouteError` — `{ index, reason }` for one rejected ROUTE.
- `System` — abstract behavior base; subclass to attach a custom behavior family.
- `DirtyTracker` — per-tick change set (read via `ctx.dirtyTracker()`).
- `PickResult` — result of `ctx.pick(ray)`.
- `Ray`, `Mat4`, `Aabb` — SF*-native math types.

**Extraction / render feed** (`[STABLE]`)

- `SceneExtractor` — `fullSnapshot()` / `delta()` / `item(id)` / `camera()` / `lights()` / `background()` / `sceneWorldBounds()`.
- `RenderItem` — `{ path, worldTransform, geometry, material, mesh, lights, ... }`.
- `RenderDelta` — `{ added, removed, updatedTransform, updatedGeometry, updatedMaterial, *Changed }`.
- `RenderItemId` / `kInvalidRenderItemId`, `PathKey` / `PathKeyHash` / `PathKeyEqual`, `GeomId` / `GeomIdHash`.
- `MeshData` — `positions/indices/normals/texcoords/colors/topology/...`.
- `Topology` — `enum class { Triangles, Lines, Points }`.
- `MaterialDesc` / `MaterialModel` / `AlphaMode` (including `toRGBA()`), `LightDesc`, `CameraDesc`, `BackgroundDesc`.

**Seams — embedder-supplied IO**

Stability is marked **per seam**, not for the group: a seam is frozen `[STABLE]` only once a
second independent backend has carried the interface with no signature change, under a
CI-gated swap-test. Four of the five have cleared that bar; `GeoProjection` has not. The live
tracker is the [Seam-Status Matrix](../seam-status.md).

| Seam | Stability | Re-exported types | Contract |
|---|---|---|---|
| Asset resolution | `[STABLE]` ([ADR-0023](../decisions/0023-assetresolver-second-backend-swap-test.md); libcurl + S3) | `AssetResolver`, `AssetResult`, `AssetStatus`, `AssetKind` | `function<AssetResult(url, AssetKind)>`; render-time may return `Pending`; parse-time must answer synchronously |
| Texture decode | `[STABLE]` ([ADR-0024](../decisions/0024-textureresolver-second-backend-swap-test.md); stb_image + wuffs) | `TextureResolver`, `TexturePixelResult`, `TexturePixels`, `TextureResolveStatus`, `TextureRef`, `SamplerParams` | `function<TexturePixelResult(url)>`; embedder maps url → decoded RGBA pixels |
| Font metrics | `[STABLE]` ([ADR-0025](../decisions/0025-fontmetrics-second-backend-swap-test.md); stb_truetype + FreeType) | `FontMetrics`, `FontKey`, `GlyphMetrics`, `GlyphResult`, `GlyphStatus`, `makeMonospaceStub` | `function<GlyphResult(const FontKey&)>`; SDK does all Text layout; default = monospaced stub (advanceEm 0.6) |
| Geo-projection | `[EXPERIMENTAL]` — no second backend yet; shape may still gain fields | `GeoProjection`, `GeoSystemDesc` | `function<SFVec3f(SFVec3d, double elev, GeoSystemDesc)>`; supplied via `MeshBuildOptions::geoProjection`; empty ⇒ flat fallback |
| Script / SAI | `[STABLE]` ([ADR-0022](../decisions/0022-scriptengine-second-backend-swap-test.md); Duktape + QuickJS) | `ScriptEngine`, `ScriptSystem`, `SaiContext`, `ScriptHandle`, `kInvalidScriptHandle` | Abstract engine; wrap in `ScriptSystem`, register via `ctx.addScriptSystem`; `SaiContext` is the backend↔runtime channel |

Also re-exported: `MeshBuildOptions` — tessellation density knobs (`sphereRings`, `sphereSegments`, `radialSlices`, `geoProjection`, `fontMetrics`).

### Seam points

- **CMake INTERFACE target** — `x3d_cpp_sdk` adds the `include/` directory to the consumer's include path and transitively links `x3d_cpp::x3d_cpp` (the full runtime + nodes). No new compiled TU is introduced; the façade is pure re-export.
- **No internals pulled in** — `sdk.hpp` includes the internal runtime headers by name (e.g. `X3DParse.hpp`, `SceneExtractor.hpp`) but presents only the curated `using` declarations. Embedders that write `using namespace x3d::sdk` get only what is listed; the deeper `x3d::runtime` and `x3d::runtime::extract` namespaces are accessible but not part of the stable contract.
- **The ext firewall is not surfaced** — `include/x3d/ext.hpp` and `runtime/ext/` are a separate opt-in (CMake `X3D_CPP_BUILD_EXT=ON`); `sdk.hpp` does not include it. See [ADR-0001: Ext Firewall](../decisions/0001-ext-firewall.md).

## How it is tested

- `ctest --preset dev -R x3d_sdk_facade` — the compile-and-run surface-lock test (`include/x3d/tests/sdk_facade_test.cpp`). Drives the complete spine — load → wire context → extract → tick → delta — using only `x3d::sdk::*` names through the single include. Any re-export that drifts or is dropped causes a compile failure. Asserts: version/profile parsed, scene non-empty, no range warnings, no rejected routes, snapshot produces at least one `RenderItem` with box positions + triangle topology, emissive material, valid camera/background/bounds, input setters compile and run, seam types (`MeshBuildOptions`, `TextureResolver`, `FontMetrics`, `AssetResolver`) nameable through the façade.

- The three worked examples — registered as the ctest targets `x3d_example_01_load_validate_convert`, `x3d_example_02_extract_render_feed`, and `x3d_example_03_attach_behavior_tick` (`examples/01_load_validate_convert.cpp`, `02_extract_render_feed.cpp`, `03_attach_behavior_tick.cpp`) — built and run as ctests (enabled when `X3D_CPP_BUILD_EXAMPLES=ON`, which defaults ON for the top-level project). Each links only `x3d_cpp::sdk` and uses only the `x3d::sdk` surface; they act as a rot-guard for the embeddable API.

- `ctest --preset dev -R x3d_corpus_smoke` — bounded corpus smoke test (`tools/corpus_sweep.cpp`). The sweep binary routes its entire pipeline (`parse → context → extract → tick`) exclusively through the `x3d::sdk` façade; any API gap surfaces here. Full sweep is 17,719 files, 0 crashes, 99.95% OK (invoked via `mise run corpus`).

- Indirectly: all 104+ ctests that link `x3d_cpp::sdk` exercise the façade surface (the majority of the test suite uses `x3d_cpp::sdk` as their only link target). Golden-file gate (`mise run golden`) locks byte-identical round-trip output; see [Gate System](../guides/gate-system.md).

## Related specs and ADRs

- [Architecture](../architecture.md) — section 6 "The SDK façade — `x3d::sdk`" is the architectural rationale; the full data-flow diagram shows where the façade sits in the stack.
- [CLI Suite](../subsystems/cli-suite.md) — the first real `x3d::sdk` consumer; `tools/x3d_cli.cpp` and `tools/x3d-cli/` link only `x3d_cpp::sdk`.
- [ADR-0001: Ext Firewall](../decisions/0001-ext-firewall.md) — why `include/x3d/ext.hpp` is a separate header/target, not part of `sdk.hpp`.
- [Gate System](../guides/gate-system.md) — the golden / conformance / cli-gate regression gates that run over the SDK surface.
- Spec: `docs/superpowers/specs/2026-06-14-m25-extraction-poc-renderer-design.md` — extraction seam design (pull-per-path, RenderDelta identity).
- Spec: `docs/superpowers/specs/2026-06-16-script-sai-runtime-design.md` — Script/SAI seam (`ScriptEngine` abstract, `SaiContext` channel).
- Spec: `docs/superpowers/specs/2026-06-18-binary-mesh-texture-abstractions.md` — `TextureResolver`/`AssetResolver` seam design.
- `docs/sdk/README.md` — v1 capability matrix and post-v1 exclusion list (referenced in `sdk.hpp` stability comment).
