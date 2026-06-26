---
title: "ADR-0017: OBJ/glTF Ingestion via Parse-Time Inline Expansion"
summary: OBJ and glTF files are ingested by first converting them to standalone X3D units, then composing them via parse-time Inline expansion — not via a polymorphic Inline reader or a flat-merge importer.
tags: [adr, inline-expansion, ingestion, obj, gltf, parse-time]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/inline-expand.md
  - ../decisions/0001-ext-firewall.md
---

# ADR-0017: OBJ/glTF Ingestion via Parse-Time Inline Expansion

## Status

Accepted — 2026-06-19 (Spec 1 shipped as branch `inline-expansion` merged to `modernize-x3d-spec` @ `883e4c7`; Spec 2 OBJ converter not yet started)

## Context

The project needed a path to import foreign 3D assets (OBJ first, then glTF, then others) into a running scene. Three candidate shapes were live:

**Option A — polymorphic Inline reader.** Teach the `InlineResolver` to sniff the content type and dispatch to a format-specific reader (OBJ parser, glTF parser, …) that returns a `Scene` directly. ISO 19775-1 §9.4.2 explicitly blesses non-X3D content types in `Inline` (`model/gltf-bin`, `model/stl`), so this is spec-sanctioned. It collapses the two steps (convert + compose) into one.

**Option B — flat merge / asset-level importer.** Import the foreign asset at the application layer, merge its geometry into the parent scene's node tree without using `Inline` at all. Simpler for single-asset cases; loses the DEF-isolation and re-compositional identity that `Inline` provides.

**Option C — convert-to-.x3d first, then compose via `Inline`.** The converter produces a standalone `.x3d` file (or in-memory `Scene`). The existing `Inline` machinery — which needed to be built anyway — resolves it. Ingestion and composition are decoupled steps.

Two forces pulled strongly toward Option C:

**Force 1: `Inline` expansion was a pre-existing deferred seam, valuable on its own.** Every X3D scene in the archive that uses `<Inline url='...'/>` (including self-animating assets with internal `TimeSensor → Interpolator → Transform` ROUTEs) was silently dropped. Building Spec 1 (parse-time Inline expansion) first delivered standalone value across all 17,719 corpus files and closed a post-v1-deferred seam, regardless of which converter came later. There was no reason to delay Spec 1 behind a converter.

**Force 2: Decoupling de-risks integration and preserves the architecture.** A polymorphic `InlineResolver` (Option A) must be invoked inside `parseDocument`, at parse time, by the core. That means the OBJ/glTF parser code would need to be either bundled with the core or injected via the resolver seam. Option C keeps the converter separate — the resolver just calls `parseFile` on a `.x3d` artifact; the format-specific work is in a separate converter that can be developed, tested, and shipped independently. This is the same decoupling the generator uses (generate once, compile separately) and the same pattern as the [ext firewall](0001-ext-firewall.md).

**Force 3: The EXTERNPROTO expansion machinery was the proven template.** `parseDocument` already ran `expandScene(scene, resolver, baseUrl, warnings)` to splice EXTERNPROTO instances, with `localFileProtoResolver` handling file lookup and a `thread_local activeFiles` stack for cycle detection. Inline expansion is the same machine: resolve a URL → get a child scene → splice it in → keep DEFs isolated → hoist internal ROUTEs → redirect the writer. AUD-B (`ProtoInstance::expanded` flag + `Scene::expandedSources` side table) had already established the byte-identical writer round-trip pattern for in-place expansion. All three building blocks were already present.

**Routing clarification (decided 2026-06-20, binding).** After the binary-geometry deep dive, the routing rule was made explicit: a **scene source** (glTF — hierarchy/materials/multi-mesh; OBJ+MTL with groups/materials) routes through `Inline` → becomes an X3D subtree (Transform/Shape/Appearance/Material + geometry leaves). A **single-mesh source** (binary STL, PLY) routes through `ExternalGeometry` → becomes a `PackedMesh` leaf. They compose: a glTF-via-Inline subtree's Shapes can hold `ExternalGeometry` leaves, giving X3D identity for structure and binary fidelity for meshes.

The **real downstream forcing function** is the VR CAVE world-model: a runtime that assembles worlds from composed assets needs `Inline` to actually resolve in-engine, with DEF isolation preserved so per-asset namespaces cannot collide.

## Decision

We decided to build foreign-asset ingestion as two decoupled specs, foundation first:

**Spec 1 (shipped 2026-06-19):** Parse-time `Inline` expansion — format-agnostic, pure X3D. `expandInlines()` (`runtime/InlineExpand.hpp`) mirrors `expandScene()` (`runtime/X3DProtoExpand.hpp`) and is invoked by `parseDocument` immediately after the PROTO expansion pass. It resolves each `<Inline url='...'/>` with `load=TRUE` via an injectable `InlineResolver` (default `localFileInlineResolver`, declared in `runtime/parse/X3DParse.hpp`), builds a synthetic `Group` from the child scene's root nodes, hoists the child's internal ROUTEs into `Scene::resolvedInlineRoutes` (pre-resolved against the child's DEF scope), and records the mapping from the synthetic `Group` back to the original `Inline` node in `Scene::expandedInlines` so writers re-emit `<Inline url='...'/>` rather than the spliced content. Child DEFs are never merged into the parent `Scene::defs`. Cycle detection uses a `thread_local std::vector<std::string> activeFiles` stack identical to the PROTO resolver.

**Spec 2 (next, not started):** An OBJ converter (`ObjReader` + MTL) that produces a standalone X3D unit (IndexedFaceSet/Coordinate/Normal/TexCoord + Material/Appearance/ImageTexture). The end-to-end path is: `chair.obj` → converter → `chair.x3d` → `<Inline url='chair.x3d'/>` resolved by Spec 1's resolver. The converter is a separate component; Spec 1 is not modified.

**glTF (Spec 3+):** Routes through `Inline` as well — the `InlineResolver` for a `.gltf`/`.glb` source would be a glTF→`Scene` converter injected by the embedder. ISO §9.4.2's explicit listing of `model/gltf-bin` as an Inline content type makes this spec-sanctioned rather than an extension.

## Consequences

**Positive:**

- Spec 1 delivers value to every X3D file that uses `<Inline>`, not just converted assets. Self-animating inlined assets (internal `TimeSensor → Interpolator → Transform` ROUTEs) now fire correctly because child routes are pre-resolved and hoisted into `Scene::resolvedInlineRoutes` before the scene bridge registers routes.
- DEF isolation is spec-mandated (ISO 19775-1 §9.4.2) and correctly implemented: child DEFs never leak into the parent namespace, so two Inline assets that both define a node named `"RedMaterial"` cannot collide.
- The writer round-trip remains byte-identical (golden corpus unchanged): expanded Inlines re-emit as `<Inline url='...'/>` via the `Scene::expandedInlines` redirect, matching the AUD-B pattern for PROTO expansion.
- The converter (Spec 2) is decoupled from the composition machinery (Spec 1). Each can be developed, tested, and replaced independently. Adding a new source format is a new converter, not a change to the expansion engine.
- The `InlineResolver` seam is injectable at `parseDocument` — embedders that need network fetch, virtual filesystems, or custom content negotiation supply their own resolver; the default `localFileInlineResolver` handles file-local resolution and stays out of the way.
- Nested Inlines (parent → child → grandchild) resolve free-of-charge via the recursion: `localFileInlineResolver` calls `parseFile`, which calls `parseDocument`, which expands child Inlines before returning. No special stack handling is needed beyond the cycle-detection guard.
- The corpus sweep (17,719 files, 0 crashes) confirmed zero regressions after Spec 1 shipped. ctest reached 140/140, golden zero-drift.

**Trade-offs / costs:**

- The two-step path (convert-to-.x3d, then `<Inline url='...'/>`) means an OBJ asset must either be pre-converted offline or converted in-process by the embedder before `parseDocument` is called. There is no zero-integration "drop an OBJ path here" shortcut in the core SDK. This is intentional (the SDK is headless; I/O policy belongs to the embedder), but it is friction for new consumers.
- `<Inline load='FALSE'>` (deferred load) and the `set_load`/`set_url` runtime events are Tier 3 (not yet implemented). An Inline with `load=FALSE` round-trips correctly (its url is preserved) but its content is never loaded until an embedder explicitly implements the dynamic load path.
- IMPORT/EXPORT cross-boundary routing (Tier 2) is out of scope: a parent scene cannot drive or read nodes inside an Inline's DEF scope without IMPORT, which is not implemented. The model (`X3DImportExport`, `Scene.imports/exports`) exists; the activation is deferred.
- Cross-UNIT scaling (child scene declares a different length unit than parent) emits a warning and defers the scale-wrap transform. Assets with mismatched units appear at the wrong physical scale until a follow-up addresses this.
- `USE`-shared Inline nodes (the same `<Inline>` referenced via `USE` at multiple sites) expand only at the first site. Subsequent USE references share the original un-expanded Inline node rather than each getting an independent expansion. This is a documented Tier-1 carve-out.
- Deep acyclic Inline+EXTERNPROTO chains have no depth bound (no shared max-depth counter alongside the cycle stack). This is theoretical — the corpus sweep found no case — but a pathological document could overflow the call stack.

## Related

- [Architecture](../architecture.md)
- [Inline Expansion subsystem](../subsystems/inline-expand.md) — the subsystem page covering `runtime/InlineExpand.hpp`, `localFileInlineResolver`, and the `Scene::expandedInlines`/`resolvedInlineRoutes` data model
- [ADR-0001: External-Code Firewall](0001-ext-firewall.md) — companion decision: binary geometry stays behind EXTERNPROTO + opt-in firewall; Inline handles scene-level sources (glTF, OBJ+MTL), ExternalGeometry handles single-mesh sources (STL, PLY)
- Design spec: `docs/superpowers/specs/2026-06-19-inline-expansion-design.md` — the approved Spec 1 design (context, architecture, field/edge-case table, success criteria)
- Ingestion roadmap memory: `docs/superpowers/` — ongoing decisions on Spec 2 (OBJ converter), binary/glTF routing, ExternalGeometry node; see `x3d-cpp-gen-ingestion-roadmap.md` in project memory
- `runtime/InlineExpand.hpp` — `expandInlines()` implementation; the `InlineResolver` type alias; `inline_detail::hoistChildRoutes()` and `makeGroup()`
- `runtime/parse/X3DParse.hpp` — `parseDocument()` call site (lines 127–131); `localFileInlineResolver` default resolver with `thread_local` cycle-detection stack
- `runtime/X3DScene.hpp` — `Scene::expandedInlines` (writer redirect map) and `Scene::resolvedInlineRoutes` (hoisted child routes)
- `runtime/X3DDocument.hpp` — `X3DDocument::inlineWarnings` (diagnostic channel, sibling of `protoWarnings`)
