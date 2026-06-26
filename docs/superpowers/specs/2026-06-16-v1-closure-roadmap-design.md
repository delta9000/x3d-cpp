# v1-Closure Roadmap — close the remaining gaps to a usable, useful X3D SDK

**Date:** 2026-06-16
**Status:** DESIGNED (the fan-out to v1; launch the workflow after the in-flight Script/SAI run lands)
**End goal:** a headless, renderer-agnostic X3D domain-runtime SDK (renderers/consumers embed it).
**Branch:** `modernize-x3d-spec`

## 1. v1 Definition of Done (the capabilities matrix)

A "usable, useful v1" = the headless runtime an embedder/renderer can adopt for the **common-scene** path,
documented. Status after this fan-out:

| Capability | State | Owner |
|---|---|---|
| Load all 4 encodings (XML/ClassicVRML/VRML97/JSON), multi-version 3.0–4.1, PROTO/EXTERNPROTO, lenient read, gzip | **DONE** | shipped |
| Per-version conformance validation + report (the M3 moat) | **DONE** | shipped |
| Scene graph: DEF/USE, transforms, bounds, binding stacks, LOD/Billboard/visibility | **DONE** | shipped |
| Animation: full interpolator set + TimeSensor + ROUTE cascade | **DONE** | shipped |
| Extraction → render feed: meshes, materials, lights, default + primitive texcoords | **DONE** | shipped |
| Interaction: input seam + TouchSensor + drag sensors + EXAMINE/FLY/LOOKAT nav | **DONE** | shipped (M2.5/M2D) |
| **Scripting/SAI** (ECMAScript via Duktape, in-process SAI core) | **IN FLIGHT** | `wf_ee656af0-f04` (T-SCRIPT) |
| **Textures usable end-to-end** (resolver seam + authored texcoord/TextureTransform/repeat + PoC bind) | **THIS FAN-OUT** | T-TEX |
| **Text rendered** (font-metrics seam + layout engine + glyph render-items + Text outputs) | **THIS FAN-OUT** | T-TEXT |
| **Embedder SDK surface**: curated public API façade + docs + runnable examples | **THIS FAN-OUT** | T-SDK |
| **v1 readiness gate**: golden + full ctest + corpus smoke + capabilities/conformance matrix + version tag | **THIS FAN-OUT** | T-GATE |

**Explicitly NOT in v1** (documented post-v1; the architecture already accommodates them as isolated additions):
WALK + collision/terrain (NAV-COLLISION); pick sensors (Picking component); full/dynamic SAI
(`createX3DFromString`, runtime node add/remove); NURBS geometry; Sound/Audio component; http/urn network
asset resolution; advanced components (full Geospatial, H-Anim, Particle systems, Physics, Layering/Layout,
CubeMap/env). Rationale: v1 targets the common-scene headless-runtime-feeding-a-renderer + scripting, with a
documented API — these are scene-dependent breadth, not spine.

## 2. Tracks

### T-SCRIPT — Scripting/SAI (IN FLIGHT, not re-planned here)
Running as `wf_ee656af0-f04` (ECMAScript/Duktape + SAI core + ScriptSystem). Its public entry points feed T-SDK.

### T-TEX — Textures usable end-to-end
The descriptor scaffolding exists (`RenderItem::TextureRef` carries Url/Inline/Movie + texcoords; byte-resolution
is deliberately outside the SDK). Gaps to close:
- **TextureResolver seam** — formalize the embedder-supplied decode: `resolve(url) → {width,height,RGBA bytes}`
  (consistent with `runtime/extract/AssetResolver.hpp` + the asset-seam philosophy). The SDK threads the resolved
  handle onto the RenderItem/TextureRef so a consumer binds without reinventing resolution; IO stays in the embedder.
- **Authored texture coordinates** — ensure `TextureCoordinate`/`TextureCoordinateGenerator` on IFS/etc. are
  extracted onto `RenderItem.texcoords` (not just default/§13 texcoords), and **`TextureTransform`** (center/
  rotation/scale/translation) is applied to texcoords (§18.4.8).
- **Repeat / TextureProperties** — surface `repeatS`/`repeatT` (and TextureProperties boundary/filter modes,
  descriptor-level) on `TextureRef` so the consumer sets sampler state correctly (§18.2.3).
- **Proof** — wire the PoC to resolve + bind one ImageTexture/PixelTexture end-to-end (replace the white fallback).
- **Tests** — texcoord/TextureTransform extraction; resolver seam round-trip with a stub decoder.
- *Defer:* MultiTexture compositing beyond the channel descriptor; MovieTexture frames; CubeMap.

### T-TEXT — Text rendered via a font-metrics seam
Spec-grounded (§15 text): the runtime can do all **layout**; only glyph metrics/outlines are IO.
- **FontMetrics seam** — embedder supplies, per (family, style, codepoint): an **advance width** and (optionally)
  a glyph **outline/mesh** or atlas UV. IO/rasterization stays in the embedder; default = a metrics stub
  (monospaced advance) so the SDK + tests work without a real font.
- **Layout engine** (pure, testable) — multi-line placement honoring FontStyle `size`/`spacing`
  (baseline = spacing×size), `justify` (BEGIN/END/FIRST/MIDDLE), `horizontal`, `leftToRight`/`topToBottom`,
  `Text.length` (per-line stretch/compress) and `maxExtent` (compress-to-fit, major axis). Emits per-glyph
  positioned quads (textured-glyph path) **and** the Text outputs `textBounds`/`lineBounds`/`origin`.
- **Extraction** — a Text branch producing glyph render-items (positioned quads + glyph UV/outline refs).
- **Tests** — layout math (justify/spacing/maxExtent/length) with the stub metrics; Text output fields.
- *Defer:* real font rasterization (embedder's job via the seam); bidi/complex shaping (language field) beyond
  left-to-right/top-to-bottom.

### T-SDK — Embedder API façade + docs + examples (depends on T-SCRIPT, T-TEX, T-TEXT)
Turns "engine that works" into "SDK someone adopts":
- **Public façade** — a curated `x3d::sdk` header/namespace (or `include/x3d/`) exposing the supported surface:
  `loadFile/loadString` (→ document + conformance findings), the runtime (`tick`, input setters, script reg),
  the extraction API (snapshot/render-items), and the seams (AssetResolver/TextureResolver/FontMetrics/ScriptEngine).
  Hides internals; documents what's stable vs experimental.
- **Docs** — a `docs/sdk/` usage guide (quick-start, the seams, the threading/tick model, the v1 capability matrix
  + the post-v1 list) + per-API doc comments.
- **Examples** — 2–3 runnable example programs under `examples/`: (1) headless load→validate→convert; (2)
  extract→feed-a-renderer (reuse the PoC path); (3) attach a Script/behavior + drive a few ticks.

### T-GATE — v1 readiness
- Golden (byte-identical or clean scoped regen), full ctest, corpus smoke across all tracks.
- A `docs/sdk/v1-capabilities.md` matrix (what works / what's post-v1) generated from the BACKLOG + this spec.
- BACKLOG sweep: mark v1 rows CLOSED; ensure every post-v1 exclusion has a row with a reason.
- Tag/record the v1 state (golden sha, ctest count, corpus conformance).

## 3. Fan-out structure (the workflow)

Fan out where files are disjoint; sequence where the extraction graph is shared. To maximize the fan-out, each
feature track puts its heavy logic in **new files** (`TextureExtract`, `TextLayout`/`TextExtract`, the seam
headers) so SceneExtractor/MeshBuilder take only a **thin dispatch hook** added once.

1. **Spec-check (parallel, read-only):** texture model details (TextureTransform/repeat/§18), Text layout
   (§15 justify/spacing/maxExtent/length), the FontMetrics + TextureResolver seam shapes, and the public-API
   conventions to mirror (existing headers/namespaces).
2. **Parallel seam + pure-logic headers (disjoint new files):** TextureResolver seam, FontMetrics seam,
   TextLayout pure engine, TextureTransform math — independent, fan out.
3. **Sequential integration (shared SceneExtractor/MeshBuilder/RenderItem):** texture extraction + dispatch hook;
   Text extraction + dispatch hook; PoC texture+Text bind. Each: impl → adversarial review (build+ctest+golden) → ≤2 fixes.
4. **T-SDK (after the features):** façade + docs + examples — impl → review.
5. **T-GATE:** golden + full ctest + corpus smoke + capabilities matrix + BACKLOG + version record.

Script: `docs/superpowers/plans/2026-06-16-v1-closure-workflow.js`. **Launch after T-SCRIPT lands** — it edits
`runtime/extract/*` and the PoC; keep one big push touching the extractor in flight at a time.

## 4. Resolved decisions (no back-and-forth; flag if you disagree)
1. **v1 = common-scene headless runtime + scripting + documented API.** The post-v1 list (collision, pick sensors, full SAI, NURBS, Sound, network resolution, advanced components) is breadth, deferred with reasons.
2. **Textures/Text/fonts close via SEAMS** (embedder supplies decoded pixels + glyph metrics) — keeps the core IO-free; the SDK does resolution-threading + layout, not decoding/rasterization.
3. **T-SDK is a first-class track** — the API façade + docs + examples are what make it an SDK, not just an engine.
4. **Fan out on disjoint files, sequence the shared extractor edits** — true parallel on the heavy logic via new files + thin dispatch hooks; no worktree-merge risk.
