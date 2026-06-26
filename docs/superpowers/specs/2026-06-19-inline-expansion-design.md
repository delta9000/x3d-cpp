# Parse-time Inline Expansion — Design (Spec 1, Tier 1)

**Date:** 2026-06-19
**Status:** Approved design; ready for implementation plan
**Component:** Networking (X3D 4.0 §9.4.2 Inline), parse/scene-build layer

## Context & motivation

This is the **first of two specs** toward an asset-ingestion capability: importing
foreign 3D formats (OBJ first, then glTF, …) and composing them into a scene as
self-contained X3D units referenced by `<Inline url='asset.x3d'/>`.

- **Spec 1 (this doc):** parse-time `Inline` expansion — format-agnostic, pure
  X3D. Resolve `<Inline url='…'/>`, load and splice the sub-scene. Valuable on its
  own: *every* `Inline` in *every* scene benefits, not just converted assets.
- **Spec 2 (later):** `OBJ → .x3d` converter (`ObjReader` + MTL). Slots on top;
  the end-to-end demo (`<Inline url='chair.x3d'/>` where `chair.x3d` came from
  `chair.obj`) is the two stacked.

Building Spec 1 first de-risks integration and is spec-grounded X3D. The ISO prose
(§9.4.2) even *explicitly blesses* non-X3D content types in `Inline`
(`model/gltf-bin`, `model/stl`) via content negotiation, so the ingestion idea
that motivates the whole effort is spec-sanctioned, not a hack.

The real downstream consumer is the user's VR CAVE world-model: a runtime that
assembles worlds from composed assets needs `Inline` to actually resolve in-engine.

### Why parse-time, and why this is the proven shape

`Inline` resolution is the **same machine** as EXTERNPROTO resolution, which the
codebase already implements:

- `parseDocument` already runs `expandScene(scene, resolver, baseUrl, warnings)`
  to splice EXTERNPROTO instances, with a default `localFileProtoResolver` that
  `parseFile`s the target and guards cross-file cycles via a `thread_local`
  `activeFiles` stack.
- `Scene::resolvedProtoRoutes` already models *"body-internal ROUTEs of expanded
  instances, pre-resolved to concrete endpoints (proto-local DEF scope; NOT
  registered in `defs`)"* — the identical "isolated name scope, internal routes
  still fire" situation Inline needs.
- AUD-B (`ProtoInstance::expanded` flag + `Scene::expandedSources` side table)
  already established how to keep a writer round-trip byte-identical after an
  in-place expansion.

Inline expansion reuses all three precedents.

## Scope (Tier 1)

**In scope:**
- Parse-time expansion for `load = TRUE`: resolve `url` via an inline resolver,
  recursively parse, splice the sub-scene as the Inline's runtime children.
- DEF/USE isolation: the inlined scene keeps its own name scope; its DEFs are NOT
  exposed to the parent (ISO §9.4.2 — only IMPORT exposes them, and IMPORT is out
  of scope here).
- Merge the inlined scene's **internal** ROUTEs, pre-resolved in child scope, so
  self-animating assets work.
- Nested Inlines (recursion) and self-reference / cycle detection (spec-required
  hard error).
- `bbox` / `visible` / `global` field passthrough.
- Child `UNIT` / `PROFILE` handled in the child's own parse context.
- Byte-identical writer round-trip (expanded Inline re-emits as `<Inline url=…/>`).

**Out of scope (named follow-ups):**
- **IMPORT / EXPORT** cross-boundary routing (Tier 2). Parent driving/reading
  nodes *inside* an Inline. Model already exists (`X3DImportExport`,
  `Scene.imports/exports`).
- **Dynamic load / unload** (Tier 3): `load=FALSE` deferral, `set_load`/`set_url`
  runtime events. This is runtime-event behavior, not parse-time.
- **Cross-UNIT scaling**: wrapping spliced children in a scale Transform when the
  child declares different length units. Emit a warning; defer the scale-wrap.
- Non-X3D content types in `Inline` (that is Spec 2's converter; and the user
  chose convert-to-`.x3d` over polymorphic Inline).

## Architecture

### Placement

New header `runtime/InlineExpand.hpp`, mirroring `X3DProtoExpand.hpp`. Entry point:

```cpp
void expandInlines(Scene& scene,
                   const InlineResolver& resolver,
                   const std::string& baseUrl,
                   std::vector<std::string>& warnings);
```

Invoked in `parseDocument` **immediately after** the existing `expandScene(...)`
PROTO pass — same hook point, same lifecycle. Nested-Inline recursion is free: the
resolver `parseFile`s the target, which re-enters `parseDocument`, which expands
*its* Inlines before returning.

### Resolver seam (decided: mirror the PROTO resolver)

```cpp
using InlineResolver =
    std::function<std::shared_ptr<Scene>(const std::vector<std::string>& urls,
                                         const std::string& baseUrl)>;
```

Default `localFileInlineResolver`: resolves file URLs relative to `baseUrl`,
`parseFile`s, returns the sub-`Scene`; `http`/`https`/`urn` URLs skipped
(embedder-override territory) — the same policy as `localFileProtoResolver`. This
keeps `Inline` and EXTERNPROTO **symmetric** (both parse-time file-graph resolvers
that handle `baseUrl` + cycle stacks).

**Rejected alternative:** honor the bytes-level `AssetResolver` contract-(B)
literally. Less symmetric, carries no `baseUrl` (would need it bolted on for nested
relative URLs). The `AssetResolver` bytes seam stays the render-time texture path
it was built for. An embedder that needs custom Inline sourcing (e.g. http)
overrides the `InlineResolver`.

### DEF isolation + internal routes (the subtle core)

1. Splice the child scene's `rootNodes` as the Inline's runtime children.
2. **Do not** merge child `defs` into the parent `Scene::defs` — names stay
   isolated (spec-mandated; only IMPORT, out of scope, would expose them).
3. Pre-resolve the child's `routes` against the *child's* `defs` into concrete
   endpoints, and append them to a new `Scene::resolvedInlineRoutes` (a sibling of
   `resolvedProtoRoutes`). The scene-bridge registers these directly, bypassing
   parent name lookup.

Result: self-animating inlined assets (their own internal
`TimeSensor → Interpolator → Transform` ROUTEs) fire correctly while nothing leaks
into the parent namespace.

### Cycle detection (spec-required)

`thread_local activeInlineFiles` stack, exactly like `localFileProtoResolver`'s
`activeFiles`. A URL already on the stack = self-reference loop (direct or
indirect) → emit a diagnostic into `inlineWarnings`, skip that expansion (the
Inline stays empty). Per §9.4.2: *"it is an error for a model to Inline itself,
directly or indirectly … browsers shall not honor self-referential loading."*

### Writer round-trip (golden gate — reuse AUD-B)

An expanded Inline MUST re-emit as `<Inline url='…'/>`, not as its spliced
children. Reuse the AUD-B pattern: keep the Inline node's serialized fields
pristine (retain `url`, add no serialized children), and hold the spliced subtree
in a Scene-side structure that the runtime walkers consult. This protects the
golden corpus (byte-identical round-trip is a hard gate).

## Two implementation-critical details to resolve in the plan

1. **Spliced-subtree storage vs. writer pristineness.** Decide between a
   Scene-side side table (mirrors `expandedSources`) and a synthetic
   runtime-only children field with an `expanded` marker. Constraint: runtime
   walkers (transform/bounds/pick/extract) must descend into the content; the
   writer must not see it.
2. **Walker visibility.** `Inline` has no authored `children` field, yet the
   loaded content must be traversed by transform/bounds/pick/extract. Confirm the
   exact children-discovery mechanism these walkers use and how the spliced
   subtree is presented to them (side-table lookup vs. synthetic children).

## Field / edge-case handling

| Field / case | Tier-1 behavior |
|---|---|
| `load = TRUE` | Expand immediately at parse time (the default path). |
| `load = FALSE` | Skip expansion; retain `url` for round-trip. (Dynamic load = Tier 3.) |
| `url` (MFString) | Try candidates in order; first that resolves wins (PROTO policy). |
| `bbox*`, `visible`, `global` | Passthrough; extraction/bounds read the Inline's content. |
| child `UNIT` differs | Emit a warning; defer scale-wrapping (follow-up). |
| child `PROFILE`/`COMPONENT` | Processed in the child's own parse context (free via recursion). |
| unresolvable `url` | Lenient: warning into `inlineWarnings`, Inline stays empty (PROTO policy). |

## Error handling

Lenient, matching the EXTERNPROTO path: a missing/unparsable/cyclic target is
skipped with a diagnostic in `inlineWarnings` (a new field on `X3DDocument`,
sibling of `protoWarnings`), never thrown. Parsing continues so a partially broken
document still yields a usable scene.

## Testing

- **Unit — composition:** 2-file fixture (parent `<Inline url='child.x3d'/>` +
  child with a `Shape`). After `buildSceneGraph`, child geometry appears under the
  Inline; child DEF is NOT in parent `defs`.
- **Unit — DEF isolation:** parent and child both DEF a node with the same name →
  no collision; parent `resolve()` does not see the child's DEF.
- **Self-animation:** child with internal `TimeSensor → PositionInterpolator →
  Transform` ROUTE → tick the context → the inlined Transform moves.
- **Cycle:** `a.x3d` inlines `b.x3d` inlines `a.x3d` → terminates, warning emitted,
  no stack overflow.
- **Nested:** parent → child → grandchild inlines all resolve.
- **Round-trip (golden gate):** parse-then-write a scene with an Inline →
  byte-identical to source across XML / VRML / JSON writers.
- **Corpus:** re-run the full conformance sweep → no new crashes or regressions;
  Inline-bearing files in the archive now actually compose.

## Success criteria

1. `<Inline url='…'/>` with a resolvable local file loads and composes into the
   scene graph at parse/build time, with the content visible to transform / bounds
   / pick / extract.
2. The inlined scene's DEFs are isolated from the parent; its internal ROUTEs fire.
3. Self-reference and nested-cycle loops terminate with a diagnostic, never crash.
4. Writer round-trip stays byte-identical (golden corpus unchanged).
5. Full corpus sweep: no new crashes/regressions; ctest green.

## Out-of-band notes for Spec 2 (OBJ converter)

- ISO §9.4.2 sanctions non-X3D content types in `Inline` — useful framing.
- The converter emits a standalone X3D unit consumed by *this* spec's resolver, so
  the end-to-end path is convert-to-file then `<Inline url='chair.x3d'/>`.
