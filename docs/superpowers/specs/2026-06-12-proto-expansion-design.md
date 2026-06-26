# PROTO / EXTERNPROTO Expansion — Design (2026-06-12)

**Status:** approved design, ready for implementation planning.
**Milestone:** M1 closeout (Step 1 of the 2026-06-07 resequencing). Higher corpus
leverage than Script/SAI (PROTO ~6% of the 17.7k-file corpus; 819 of ~1066 PROTO
files are XML, 1314 files reference EXTERNPROTO).
**Predecessor:** Step 0 (lenient-read range warnings) complete. Readers currently
capture PROTO declarations/instances *structurally* (VRML family only) with no
expansion; `ProtoInstance::expand()` returns `nullptr`.

---

## 1. Goal

Turn captured `<ProtoDeclare>` / `<ExternProtoDeclare>` / `<ProtoInstance>` data
into concrete node trees at parse time, so every downstream system (event
cascade, M2 transform/bounds, extraction) sees only real nodes — while preserving
the ability to round-trip the original `<ProtoInstance>` on write.

**Non-goals:** Script/SAI (tabled S1–S4). Network/`urn:` asset loading (the full
M2.5 asset-resolver seam) — only a narrow local-file declaration resolver ships.

## 2. Locked decisions (from brainstorming Q&A)

1. **Expansion model:** eager, at the parse front door. Each expanded primary node
   keeps a link back to its source `ProtoInstance` for round-trip.
2. **EXTERNPROTO scope:** full local-PROTO expansion + a narrow synchronous
   `ProtoDeclarationResolver` seam whose default resolves relative/absolute
   **local-file** URLs (covers the ~688 file-based EXTERN files). `http(s)`/`urn:`
   return `nullptr` from the default and are left to an embedder-supplied override
   — **no network code ships.** The full M2.5 asset seam later subsumes this.
3. **IS wiring depth:** full — (a) forward initializeOnly/inputOutput interface
   values into IS-mapped body fields at expand time; (b) register body-internal
   ROUTEs into the cascade; (c) redirect external routes that target an instance's
   inputOnly/outputOnly/inputOutput interface fields onto the IS-mapped body
   endpoints. Parametric **and** animated PROTOs work live.
4. **XML capture:** included. `XmlReader` gains PROTO/IS capture into the same
   Scene data model the VRML readers use, so expansion reaches all four encodings.

### Two judgment calls (approved)

- **Proto-local name scoping (E):** body DEF names have their own scope and must
  not leak into the document DEF table. Body-internal routes are **pre-resolved to
  direct `FieldAddress` endpoints** (we hold the cloned node pointers) and stored
  in a dedicated `Scene.resolvedProtoRoutes` channel the bridge consumes directly
  — *not* name-mangled into `Scene.defs`.
- **Golden safety (H):** the `expanded-root → source-ProtoInstance` link is kept
  **scene-side as a map**, NOT as a member on the generated `X3DNode`. This keeps
  the whole change runtime-only and the codegen golden **byte-identical**.

## 3. Scope guard: runtime-only, no golden change

All work lands in `runtime/` (incl. `runtime/parse/`, `runtime/codecs/`,
`runtime/events/`). Clone uses the existing `X3DNode::fields()` reflection table
and `X3DNodeFactory`; the source link is a scene-side map; no template or model
change. **The golden tree must remain byte-identical** (sha256
`223b73941e93e8bdd1779ffd12d7e35a64c65a986968d01eea6369bd4dd021f2`); the
`tests/test_golden_tree.py` / `scripts/check_golden.sh` gate is the verification.

## 4. Architecture

```
parseFile(path) / parseDocument(text, baseUrl?)
  └─ read tree  ── readers now CAPTURE Proto* + IS (XML + VRML)
  └─ EXPANSION PASS  (new: X3DProtoExpand)
       for each ProtoInstance (in tree slots + scene.protoInstances):
         obtain declaration  ─ local | ProtoDeclarationResolver(urls, baseUrl)
         deepClone(body)      ─ X3DProtoClone (reflection + factory, USE-aware)
         value-forward IS     ─ init/inputOutput interface vals → body fields
         pre-resolve body routes → scene.resolvedProtoRoutes  (FieldAddress)
         build interface redirect entries → scene.protoRedirects
         recurse into nested ProtoInstances
         splice primary node into parent slot
         record expandedSources[root*] = sourceInstance
       collect ProtoWarnings → X3DDocument
  └─ (consumer) X3DExecutionContext::buildFrom(scene)
       X3DSceneBridge: add scene.routes + resolvedProtoRoutes,
       redirect external routes naming an instance's interface field
       through scene.protoRedirects to the real body endpoint(s)
Writers: if expandedSources has root*, emit original <ProtoInstance>.
```

## 5. Components

### 5.1 Data-model additions — `runtime/X3DProto.hpp`, `runtime/X3DScene.hpp`

`X3DProto.hpp`:
- `struct IsConnection { std::shared_ptr<X3DNode> node; std::string nodeField; std::string protoField; };`
- `ProtoBody` gains `std::vector<IsConnection> isConnections;`
- **Parent linkage (placement):** `ProtoInstance` is not an `X3DNode`, and today the
  readers drop the parent's child slot when they hit an instance (it is recorded
  only in `scene.protoInstances`). To splice the expansion into the right place,
  `ProtoInstance` gains `std::weak_ptr<X3DNode> parent;` and
  `std::string parentField;` (empty `parent` ⇒ the instance is a Scene root). Capture
  records these; `expandScene` reads them to place the primary node.

`X3DScene.hpp` (Scene):
- `std::unordered_map<X3DNode*, ProtoInstance> expandedSources;` — round-trip link.
- `std::vector<ResolvedRoute> resolvedProtoRoutes;` — pre-resolved body-internal
  routes (endpoints as `FieldAddress`, i.e. `(X3DNode*, fieldName)`).
- `protoRedirects`: maps an exposed interface endpoint to its IS-mapped body
  endpoints, keyed so the bridge can rewrite external routes. Concretely:
  `std::unordered_map<FieldAddress, std::vector<FieldAddress>>` keyed by
  `(instancePrimaryNode*, interfaceFieldName)`.

(`ResolvedRoute` = a `Route` with both endpoints already resolved to raw
`X3DNode*`+field; reuse `FieldAddress` pairs to avoid DEF-table dependence.)

### 5.2 Capture — `runtime/codecs/XmlReader.hpp`, `runtime/parse/ClassicVrmlReader.hpp`

- **VRML/ClassicVRML:** at the `name IS protoField` site (currently consumed and
  discarded, `ClassicVrmlReader.hpp:~409`), record
  `IsConnection{currentNode, name, protoField}` on the enclosing ProtoBody.
- **XML (new):** capture, into the existing Scene Proto structures:
  - `<ProtoDeclare name>` → `ProtoDeclaration`
  - `<ProtoInterface>` / `<field name type accessType value>` → `ProtoField`
    (incl. SFNode/MFNode `nodeDefault` child nodes)
  - `<ProtoBody>` → `ProtoBody.nodes` + nested `<ROUTE>` → `ProtoBody.routes`
  - `<IS><connect nodeField protoField/></IS>` nested in a body node →
    `IsConnection`
  - `<ExternProtoDeclare name url>` → `ExternProtoDeclaration`
  - `<ProtoInstance name DEF/USE containerField>` + `<fieldValue name value>` →
    `ProtoInstance` (recorded in the tree slot AND `scene.protoInstances`)

### 5.3 Deep clone — `runtime/X3DProtoClone.hpp` (new, header-only)

```
std::shared_ptr<X3DNode> deepClone(
    const std::shared_ptr<X3DNode>& src,
    std::unordered_map<const X3DNode*, std::shared_ptr<X3DNode>>& cloneMap);
```
- If `src` already in `cloneMap`, return the existing clone (preserves intra-body
  **DEF/USE shared identity**).
- Else `X3DNodeFactory::create(src->nodeTypeName())`, insert into `cloneMap`, then
  walk `src->fields()`: scalar fields copied via `dst.set(field.get(src))`;
  SFNode/MFNode fields recursively `deepClone`d and re-set.
- Skips event-only fields (no `get`/`set` value to copy).

### 5.4 Expansion engine — `runtime/X3DProtoExpand.hpp` (new, header-only)

```
struct ExpandResult { std::shared_ptr<X3DNode> primary;
                      std::vector<ProtoWarning> warnings; };
ExpandResult expandInstance(const ProtoInstance&, Scene&,
                            const ProtoDeclarationResolver&, ExpandGuard&);
void expandScene(Scene&, const ProtoDeclarationResolver&,
                 const std::string& baseUrl, std::vector<ProtoWarning>& out);
```
Per-instance algorithm:
1. **Resolve declaration.** Local → `instance.declaration`. EXTERN →
   `resolver(externDecl.url, baseUrl)`. `ExpandGuard` tracks an active-URL set +
   depth cap (default e.g. 32) → cycle/recursion → `ProtoWarning`, stop.
2. **Clone body** with a fresh `cloneMap`. `primary = clone(body.nodes[0])`;
   remaining body nodes cloned too (kept alive via the routes/redirects that
   reference them, and via `primary`'s field graph).
3. **Effective interface values:** start from each `ProtoField` default, override
   with matching `instance.fieldValues` (by name).
4. **Value-forward:** for each `IsConnection` whose `protoField` is
   initializeOnly/inputOutput, set the *cloned* `node`'s `nodeField` to the
   effective value via reflection. (SFNode/MFNode interface values: set node(s).)
5. **Body-internal routes:** build a body-local DEF map from cloned nodes, resolve
   each `body.routes` entry to `(clonedFromNode*, fromField)`→`(clonedToNode*,
   toField)`, append to `scene.resolvedProtoRoutes`.
6. **Interface redirect:** for each exposed inputOnly/outputOnly/inputOutput
   interface field, add `protoRedirects[(primary*, protoField)] += {clonedNode*,
   nodeField}` for every matching `IsConnection` (fan-out for inputOutput).
7. **Recurse:** expand any nested `ProtoInstance` found in the cloned body.
8. **Splice:** caller replaces the source `ProtoInstance` in its parent slot with
   `primary` (inheriting DEF + containerField); register
   `scene.expandedSources[primary.get()] = instance` and the DEF in `scene.defs`.

`expandScene` walks root nodes + `scene.protoInstances`, drives the above, and is
the single integration point the front door calls.

### 5.5 Resolver seam — `runtime/parse/X3DProtoResolver.hpp` (new)

```
using ProtoDeclarationResolver =
  std::function<std::shared_ptr<ProtoDeclaration>(
      const std::vector<std::string>& urls, const std::string& baseUrl)>;
std::shared_ptr<ProtoDeclaration> localFileProtoResolver(
      const std::vector<std::string>& urls, const std::string& baseUrl);
```
Default `localFileProtoResolver`: for each candidate URL, split a trailing
`#Fragment`; if the URL is file-like (not `http(s)://` / `urn:`), resolve it
relative to `baseUrl`'s directory, `parseFile` the target document, and return the
`ProtoDeclaration` named by the fragment (or, if no fragment, by the EXTERN
declaration's own name). First successful candidate wins. `http`/`urn` → return
`nullptr`. Interface mismatch between the EXTERN interface and the resolved
declaration → **warn (lenient), do not throw**; expansion proceeds best-effort.

### 5.6 Front-door integration — `runtime/parse/X3DParse.hpp`

- `parseDocument` gains an optional `baseUrl` parameter (default empty);
  `parseFile` passes the file's parent directory as `baseUrl`.
- After the existing parse + range-warning collection, call
  `expandScene(doc.scene, resolver, baseUrl, protoWarnings)` with the default
  resolver (overridable via an optional parameter, mirroring how a consumer could
  inject one). Append `protoWarnings` to `X3DDocument` (a `protoWarnings` vector
  alongside the existing `rangeWarnings`).

### 5.7 Bridge / cascade wiring — `runtime/events/X3DSceneBridge.hpp`

- After adding `scene.routes` (existing path), add every
  `scene.resolvedProtoRoutes` entry directly via `ctx.addRoute(from, to)` (already
  `FieldAddress` — no DEF resolution needed).
- When resolving an external `Route` whose endpoint `(node, field)` names an
  instance's exposed interface field (i.e. `field` is not a real field on `node`
  but `protoRedirects` has `(node*, field)`), rewrite that endpoint to each
  redirect target and add the resulting route(s) — fan-out on the input side,
  fan-in on the output side. This is what lets external events reach/leave
  expanded PROTO bodies.

### 5.8 Round-trip — `runtime/codecs/XmlWriter.hpp` (+ VRML writer)

Before writing a node, check `scene.expandedSources` for the node pointer. If
present, emit the original `<ProtoInstance name DEF containerField>` with its
`<fieldValue>`s instead of serializing the expanded subtree.
`ProtoDeclare`/`ExternProtoDeclare` continue to emit from `scene.protoDeclarations`
/ `externProtoDeclarations` as today.

## 6. Diagnostics — `ProtoWarning`

A small struct mirroring `RangeDiagnostic`'s spirit: `{ enum kind; std::string
instanceName; std::string detail; }` with kinds `UnresolvedExtern`,
`MissingDeclaration`, `InterfaceMismatch`, `RecursionLimit`, `UnknownField`.
Collected non-fatally into `X3DDocument.protoWarnings`. Consistent with the
lenient-read policy: keep parsing, surface what happened.

## 7. Testing (TDD — red→green per unit)

- **deepClone:** clones values correctly; intra-body `USE` yields a *shared*
  (same-pointer) clone, not a duplicate.
- **Local parametric PROTO:** a proto with a `size` field IS-connected to a Box's
  `size`; instance overrides `size`; expansion produces a Box with the overridden
  value spliced into the parent slot.
- **IS live event:** a proto exposing an inputOnly (e.g. `set_fraction`) IS-mapped
  to an interior interpolator; an external ROUTE into the instance field drives the
  interpolator and produces the expected `value_changed` after a cascade tick.
- **EXTERNPROTO (file):** two-file fixture (extern declaration file + instance
  doc) resolves via `localFileProtoResolver` and expands. An `http://` url yields
  an `UnresolvedExtern` warning, no throw.
- **Round-trip:** parse → expand → write re-emits `<ProtoInstance>` (+ fieldValues),
  not the expansion.
- **Recursion guard:** a self/mutually-referential extern is bounded by the depth
  cap and produces a `RecursionLimit` warning.
- **Corpus smoke:** a real PROTO file from the corpus expands without throwing;
  record the conformance-sweep delta (no parse regressions elsewhere).
- **Golden gate:** `test_golden_tree` byte-identical; `pytest` + `ctest` green.

## 8. Risks / notes

- **MFNode/containerField placement:** a proto used where a typed slot is required
  (e.g. a geometry proto under `Shape.geometry`) relies on the instance's
  `containerField` + the primary node's type routing through the existing
  `attachChild` logic. Covered by setting `primary`'s containerField from the
  instance.
- **Body node lifetime:** cloned auxiliary body nodes (not reachable from
  `primary`'s field graph) are kept alive by the shared_ptrs held in
  `resolvedProtoRoutes` / `protoRedirects`. Ensure those hold `shared_ptr` (or the
  Scene retains a clone-roots vector) so nothing is freed post-expansion.
- **Nested + ordering:** declarations may appear after instances; expansion runs
  as a post-parse pass when all same-document declarations are known. EXTERN
  resolution is on-demand during that pass.
- **Default resolver reentrancy:** `localFileProtoResolver` calls `parseFile`,
  which itself runs an expansion pass on the sub-document. The `ExpandGuard`
  active-URL set must thread through to break import cycles across files.

## 9. Definition of done

- All four encodings capture PROTO/IS; local + file-EXTERN instances expand
  eagerly at the front door; IS value-forwarding + body routes + external-route
  redirection all wired; round-trip re-emits `<ProtoInstance>`.
- `X3DDocument.protoWarnings` populated for the unresolved/mismatch/recursion cases.
- Golden byte-identical; `pytest` + `ctest` green; corpus PROTO files expand with
  no new parse regressions (sweep delta recorded).
