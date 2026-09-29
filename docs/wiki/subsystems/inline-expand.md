---
title: Inline Expansion
summary: Parse-time Inline expansion enabling foreign-asset ingestion and general sub-scene composition via the X3D Inline seam.
tags: [subsystem, inline, expansion, ingestion, obj, gltf, parse-time]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/parse-readers.md
  - ../subsystems/proto-expand.md
  - ../subsystems/scene-graph.md
  - ../subsystems/codecs-writers.md
  - ../subsystems/ext-firewall.md
  - ../decisions/0017-inline-expansion-parse-time.md
---

# Inline Expansion

## Purpose

The Inline Expansion subsystem resolves `<Inline url='…'/>` nodes at parse time, loads and splices the referenced sub-scene into the parent scene graph, and ensures the result round-trips byte-identically as `<Inline url='…'/>` on write. It is the first tier of the asset-ingestion capability (Spec 1, shipped 2026-06-19) and the foundation for the foreign-format ingestion roadmap (OBJ, glTF via `Inline`).

The subsystem owns four responsibilities:

- **URL resolution** — try each candidate URL in order; first resolvable wins.
- **DEF isolation** — the child scene's name scope never leaks into the parent (ISO 19775-1 §9.4.2). The sanctioned escape hatch — an explicit `<IMPORT inlineDEF='…' importedDEF='…' AS='…'/>` — is wired by `wireInlineImports` (see below), which is the only way a child DEF becomes visible in the parent.
- **Route hoisting** — the child's internal ROUTEs are pre-resolved against the child's own DEF scope and registered as concrete `resolvedInlineRoutes` so self-animating assets tick correctly.
- **Writer round-trip** — expanded Inlines are recorded in `Scene::expandedInlines` so all four writers re-emit the original `<Inline url='…'/>` rather than the synthetic `Group` subtree.

This mirrors the EXTERNPROTO expansion machinery in `runtime/X3DProtoExpand.hpp` and reuses the AUD-B `expandedSources` writer-redirect pattern.

Child Scenes returned by the parsing front door retain their source UNIT
declarations in `Scene::sourceUnits`, including when reached through
`expandedInlineScenes`. An empty snapshot means that the child declared no
units. Retention does not yet convert child values to the parent's units.

## Key files

| File / directory | Role |
|---|---|
| `runtime/InlineExpand.hpp` | Entry point `expandInlines()`, resolver typedef `InlineResolver`, helpers `readUrl`/`readLoad`/`makeGroup`/`hoistChildRoutes`/`replaceInParent` (all in `inline_detail` namespace), and `wireInlineImports()` — the §9.2 cross-Inline escape hatch |
| `runtime/X3DImportExport.hpp` | Data-only structs `Import` and `Export`; stored in `Scene` and consumed by `wireInlineImports` |
| `runtime/parse/X3DParse.hpp` | `parseDocument` — calls `expandInlines` immediately after the PROTO pass, then `wireInlineImports`, then re-runs `resolveRoutes`; provides `localFileInlineResolver` (the default, file-relative, cycle-guarded resolver) |
| `runtime/X3DScene.hpp` | `Scene::expandedInlines` (`unordered_map<X3DNode*, shared_ptr<X3DNode>>`), `Scene::expandedInlineScenes` (original Inline node -> child `Scene`, retained for IMPORT resolution), and `Scene::resolvedInlineRoutes` (`vector<ResolvedProtoRoute>`) — the side tables this subsystem populates |
| `runtime/X3DDocument.hpp` | `X3DDocument::inlineWarnings` — the lenient-diagnostic channel for unresolvable or cyclic Inline URLs |
| `runtime/events/X3DSceneBridge.hpp` | Registers `resolvedInlineRoutes` directly (bypasses parent name lookup), making child-internal ROUTEs live |
| `runtime/codecs/{XmlWriter,CanonicalXmlWriter,VrmlWriter,JsonWriter}.hpp` | Each checks `scene_->expandedInlines` before writing a Group node and re-emits the stored Inline instead |
| `runtime/parse/tests/inline_expand_test.cpp` | Unit: composition, DEF isolation, `parseDocument` injection seam, walk depth cap |
| `runtime/parse/tests/inline_routes_test.cpp` | Unit: child-internal ROUTEs fire after tick |
| `runtime/parse/tests/inline_carriers_test.cpp` | Unit: `<IMPORT>`/`<EXPORT>` carrier structs parsed and stored |
| `runtime/parse/tests/core_diagnostics_test.cpp` | Unit: `IMPORT ... AS` alias wired to the Inline's exported DEF so a ROUTE to the imported name resolves (`import_export_wire_*`) |
| `runtime/parse/tests/inline_cycle_test.cpp` | Unit: direct/indirect self-reference terminates with a diagnostic |
| `runtime/parse/tests/inline_containment_cycle_test.cpp` | Unit: containment-cycle guard in the expansion walk (visited-set) |
| `runtime/parse/tests/inline_roundtrip_test.cpp` | Integration: parse-then-write round-trip produces byte-identical output across all encodings |

## Interfaces and seams

### Exposed interface

```cpp
namespace x3d::runtime {

/// Callable that resolves an Inline's url list to a parsed sub-Scene.
/// Returns nullptr on failure or cycle (lenient path).
using InlineResolver = std::function<std::shared_ptr<Scene>(
    const std::vector<std::string> &urls, const std::string &baseUrl)>;

/// Expand every load=TRUE Inline in `scene`.
/// Failures are lenient: unresolvable/cyclic Inlines emit into `warnings`
/// and the Inline node remains un-expanded (round-trips normally).
void expandInlines(Scene &scene,
                   const InlineResolver &resolver,
                   const std::string &baseUrl,
                   std::vector<InlineWarning> &warnings);

} // namespace x3d::runtime
```

`expandInlines` is called from `parseDocument` in `runtime/parse/X3DParse.hpp` immediately after `expandScene` (the PROTO pass). Nested Inlines recurse automatically because `localFileInlineResolver` calls `parseFile`, which re-enters `parseDocument` and expands any Inlines in the child before returning.

The `InlineWarning::Kind` enum covers `UnresolvedUrl` (first-class, no throw — matching the EXTERNPROTO policy).

### Seam points

- **`InlineResolver` injection** — `parseDocument` accepts an `InlineResolver` parameter (default: `localFileInlineResolver`). An embedder supplying a network fetcher, virtual filesystem, or format-converting resolver (e.g. a glTF-to-Scene converter) passes it here; the core never changes.

- **Runtime injection** — `attachStandardRuntime` and `RuntimeSession::SessionOptions` accept the same `InlineResolver` plus `baseUrl`. No resolver means deferred Inline nodes stay inert. The parsed `Scene` does not retain an IO callback; callers that want late loading pass their resolver again. `InlineRuntimeSystem` attempts a deferred URL once per value; `load=FALSE` unloads an expanded subtree and a changed `url` replaces it.

- **`localFileInlineResolver`** (in `X3DParse.hpp`) — the default resolver. Resolves file-like URLs relative to `baseUrl`, calls `parseFile` on the target, and guards cycles via a `thread_local std::vector<std::string> activeFiles` stack. `http`/`https`/`urn:` schemes are skipped (embedder-override territory), matching `localFileProtoResolver`'s policy.

- **`Scene::expandedInlines`** — a `Group*`-keyed map populated by `expandInlines`. Each writer checks this map when it would emit a `Group` node; on a hit it emits the stored `Inline` node instead. This is the writer round-trip contract: the map must remain valid for the lifetime of any writer pass over the scene.

- **`Scene::resolvedInlineRoutes`** — a `vector<ResolvedProtoRoute>` populated by `hoistChildRoutes`. `X3DSceneBridge` registers these directly into the event graph, bypassing parent DEF lookup (child nodes are reachable via pointer; they are just not named in the parent scope).

- **`X3DDocument::inlineWarnings`** — the diagnostic collection for the lenient error path. Callers inspect this after `parseDocument` to surface unresolvable or cyclic Inline diagnostics.

- **`Scene::expandedInlineScenes`** — an `Inline*`-keyed map from the ORIGINAL Inline node to the child `Scene` it expanded to, retained so `wireInlineImports` can resolve an imported DEF/`<EXPORT AS>` alias. The child's full DEF table is still never merged into the parent.

- **`wireInlineImports(Scene&)`** — called after expansion. It registers a parent alias only when the child explicitly EXPORTs the requested public name (`AS` or the DEF name). A private child DEF cannot be imported. Routes to the new alias then resolve against the exported node.

- **`X3DImportExport.hpp` (`Import`/`Export` structs)** — parsed and stored in `Scene::imports`/`Scene::exports` by the readers and consumed by `wireInlineImports` for cross-Inline routing. `<EXPORT>` in a child scene is fully honoured; a parent `<EXPORT>` (exposing a local DEF upward) is still only carried as data.

### Expansion mechanics

1. Walk the scene graph collecting `(parent, inlineNode)` pairs. A `visited` set prevents re-walking through USE-shared or cyclic containment structures (this guard is needed because `expandInlines` runs before `breakContainmentCycles`). The walk also stops descending at `kMaxNestingDepth` (MEM-1, like the other graph walkers): proto expansion can nest deeper than the parse-time cap, and the walk recurses on the native stack. Field values are read with `FieldRead.hpp`'s exception-free `fieldValueAs`, so an error inside the walk propagates instead of silently dropping a subtree.
2. For each collected Inline with `load=TRUE`, call the resolver. On success:
   - Wrap the child's `rootNodes` in a synthetic `Group` via `X3DNodeFactory::create("Group")`.
   - Call `hoistChildRoutes`: resolve the child's `routes` against the child's own `defs`, append concrete endpoints to `scene.resolvedInlineRoutes`. Also hoist any already-resolved `resolvedProtoRoutes` and `resolvedInlineRoutes` from nested expansions.
   - Record `scene.expandedInlines[group.get()] = inl` for writer round-trip.
   - Replace the Inline node in its parent slot (`replaceInParent`), or in `scene.rootNodes` for root-level Inlines.
3. `load=FALSE` Inlines stay in place. With an injected resolver, `InlineRuntimeSystem` runs this same expansion pass after a later `load=TRUE` event, enrolls the new subtree with live systems, and connects child and IMPORT routes. On unload it removes routes, queued events, aliases and per-node system state before restoring the original Inline. The extractor diffs a fresh topology snapshot on load, unload and replacement, reporting removed and added RenderItems.
4. Child DEFs are **never** merged into `scene.defs`, enforcing ISO §9.4.2 DEF isolation.

## How it is tested

- `ctest --preset dev -R x3d_parse_tests` (doctest case: `inline_expand_test`) — composition (Inline → Group+Shape under a parent Transform), DEF isolation (child DEF "Geo" absent from parent `resolve()`), writer round-trip map entry set, `parseDocument` custom-resolver injection seam. Doctest case `inline_expand: the graph walk stops at kMaxNestingDepth` covers the depth cap.
- `ctest --preset dev -R x3d_inline_routes` — child-internal `TimeSensor → PositionInterpolator → Transform` ROUTE fires after a tick.
- `ctest --preset dev -R x3d_parse_tests` (doctest case: `inline_carriers_test`) — `<IMPORT>`/`<EXPORT>` statement structs are parsed and accessible on `X3DDocument`.
- `ctest --preset dev -R x3d_inline_cycle` — direct and indirect self-reference terminates with a diagnostic; no stack overflow or infinite loop.
- `ctest --preset dev -R x3d_inline_containment_cycle` — the visited-set guard in the expansion walk handles containment-cycle nodes without recursing forever.
- `ctest --preset dev -R x3d_inline_roundtrip` — parse-then-write a scene containing an Inline produces byte-identical output across XML, VRML, and JSON writers.
- `ctest --preset dev -R x3d_corpus_smoke` (250-file bounded) and `mise run corpus` (full 17,719-file sweep) — regression gate; confirmed 0 crashes and golden zero-drift after the Spec-1 ship.

## Related specs and ADRs

- [ADR-0017: OBJ/glTF Ingestion via Parse-Time Inline Expansion](../decisions/0017-inline-expansion-parse-time.md)
- [ADR-0001: Ext Firewall](../decisions/0001-ext-firewall.md) — governs where binary/ExternalGeometry handling lives; Inline stays in the spec-correct core
- [Proto Expand subsystem](../subsystems/proto-expand.md) — the structural mirror: `expandScene` / `X3DProtoExpand.hpp` (same parse-time splice pattern; `resolvedProtoRoutes` sibling to `resolvedInlineRoutes`)
- [Parse Readers subsystem](../subsystems/parse-readers.md) — `parseDocument` hosts the call site; `localFileInlineResolver` lives in `X3DParse.hpp`
- [Scene Graph subsystem](../subsystems/scene-graph.md) — `Scene::expandedInlines`, `Scene::resolvedInlineRoutes`, `Scene::imports`, `Scene::exports` are owned here
- [Codecs / Writers subsystem](../subsystems/codecs-writers.md) — all four writers consult `expandedInlines` for round-trip fidelity
- [Ext Firewall subsystem](../subsystems/ext-firewall.md) — `ExternalGeometry` (the single-mesh lazy-proxy extension node) is the composing peer; glTF scene ingestion routes through Inline (this subsystem), not through ExternalGeometry
- Spec: `docs/superpowers/specs/2026-06-19-inline-expansion-design.md` — full Tier-1 design (scope, architecture, field/edge-case table, success criteria)
- ISO 19775-1 §9.4.2 — normative prose for Inline node semantics, DEF isolation, and the self-referential-loading prohibition
