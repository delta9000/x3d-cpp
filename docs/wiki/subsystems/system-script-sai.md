---
title: Script / SAI Runtime
summary: Script node runtime — ScriptEngine seam, SAI execution context, ECMAScript (Duktape) backend, and dynamic field marshalling.
tags: [subsystem, script, sai, ecmascript, duktape, quickjs, dynamic-fields]
updated: 2026-11-05
related:
  - ../architecture.md
  - ../subsystems/routes.md
  - ../seam-status.md
  - ../decisions/0013-js-engine-choice.md
  - ../decisions/0014-dynamic-field-foundation.md
  - ../decisions/0022-scriptengine-second-backend-swap-test.md
  - sai-sdk.md
  - ../decisions/0047-sai-sister-repo-split.md
---

# Script / SAI Runtime

`runtime/script/SaiContext.hpp` is the native Script-backend API. The
[portable native provider](sai-provider.md) is separate and does not expose
Script nodes. Linking the packages does not connect their scenes.

## Purpose

The Script / SAI Runtime makes `Script` nodes execute. It owns the full ISO/IEC 19775-1 §29.2 lifecycle (load → initialize → prepareEvents → invoke → eventsProcessed → shutdown), enforces the `directOutput` and `mustEvaluate` field gates, and exposes the in-process Scene Access Interface (SAI) as the only sanctioned channel from script code back into the running scene. The subsystem is language-agnostic: the `ScriptEngine` abstract seam carries no scripting-language types, so a Lua, Python, or alternative ECMAScript backend could be substituted without touching the runtime.

Duktape's `EcmaScriptBackend` builds with `X3D_CPP_BUILD_SCRIPT`, on by default
and independent of the test build. Optional `QuickJsBackend` builds with
`X3D_CPP_BUILD_QUICKJS`. Its comparison suite runs authored scripts through
independent engines and compares resulting field and route values. Concrete
ownership and author-field APIs follow
[ADR-0057](../decisions/0057-scene-owned-author-fields.md).

## Ownership migration

Construct parsed-scene contexts with `X3DExecutionContext ctx(doc.scene.authorFields)`
before attaching ScriptSystem. Standalone contexts have their own fresh store;
register programmatic fields with `ctx.authorFields().addAuthorField(...)`.
`SaiContext` and both engine backends use this owner for seeding, reads and
readback; there is no ambient store. `ScriptEngine`'s abstract virtual interface
is unchanged. The concrete runtime types require a rebuild and the global
`dynamicFieldStore()` API has been removed. FieldInfo copies track their node
lifetime and cannot activate fields at a recycled node address.

This migration preserves programmatically registered node-valued fields during
cloning. The XML reader reads SFNode/MFNode author-field defaults from child
elements (USE included) and the XML writer re-emits a Script's node fields as
children; the JSON and ClassicVRML readers still leave them empty
. It does not claim full author-field IS, Script-language or SAI
conformance.

## Key files

| File / directory | Role |
|---|---|
| `runtime/script/ScriptSystem.hpp` | `System` subclass that drives one language backend; owns per-script `Entry` state (SAI surface, load handle, deferred-event queue, tick flags); wires `prepareEvents` and `eventsProcessed` phases around the cascade drain |
| `runtime/script/ScriptEngine.hpp` | Abstract backend seam: `load / initialize / prepareEvents / invoke / eventsProcessed / shutdown`; defines `ScriptHandle` (opaque `uint64_t`, 0 = invalid) |
| `runtime/script/SaiContext.hpp` | In-process SAI surface bound to one Script node: field get/set, dynamic `addRoute`/`deleteRoute`, browser-info queries (`getName`, `getVersion`, `currentTime`, `currentFrameRate`, `print`; scripts also see `Browser.println` as an alias of `print`); enforces the `directOutput` gate |
| `runtime/script/EcmaScriptBackend.hpp` | `ScriptEngine` implementation using vendored Duktape 2.7.0; one `duk_context` per loaded script; `pushValue`/`toValue` static methods exposed for unit testing |
| `runtime/script/EcmaScriptBackend.cpp` | Full marshalling (SF/MF types ↔ JS), Browser global object, `seedAuthorGlobals`/`readbackAuthorGlobals()` for dynamic author-field integration |
| `runtime/script/FieldTypesPrelude.hpp` | ES5.1 source both backends evaluate at load: the ISO/IEC 19777-1 field-type constructors (`SFVec3f`, `SFRotation`, `MFString`, ...) and their prototypes |
| `runtime/script/vendor/duktape/` | Vendored Duktape 2.7.0 (single-file C library) |
| `runtime/script/tests/MockScriptEngine.hpp` | Recording `ScriptEngine` for language-free Track-A tests; arms reactions per event name; exposes a `calls` log |
| `runtime/script/tests/` | All unit and integration tests for this subsystem |

## Interfaces and seams

### Exposed interface

`ScriptSystem` is registered with `X3DExecutionContext` via `addScriptSystem()`. Thereafter the context drives both phases each tick:

- `ScriptSystem::update(now, ctx)` — called by the context's per-tick System pass; runs `prepareEvents` for every loaded script before any ROUTE processing (§29.2.5,  once-per-timestamp guard).
- `ScriptSystem::runEventsProcessed(ctx)` — called by the context after the cascade drain; flushes deferred (`mustEvaluate=FALSE`) inputs, then calls `eventsProcessed` at most once per script that received an event this cascade (§29.2.4).
- `ScriptSystem::onAuthorInput(...)` — registered as a cascade author-input listener. A routed event on a Script `inputOnly` field calls `deliverInputEvent`, so the function of the same name runs (§29.2). A routed event on an `inputOutput` field refreshes the script's global via `ScriptEngine::updateField`. Embedders must not bridge author inputs to `deliverInputEvent` themselves, or each handler runs twice.

Script lifecycle entry points (called by the cascade or the input seam):

```cpp
// Enroll a Script node; loads + initializes it if load=TRUE (§29.2.3).
void ScriptSystem::attach(X3DNode *node, X3DExecutionContext &ctx);

// Deliver one inputOnly author event (§29.2.2).
// mustEvaluate=TRUE -> invoke immediately; FALSE -> defer to batch flush.
void ScriptSystem::deliverInputEvent(Script *script,
    const std::string &eventName,
    std::any value, X3DFieldType type, double timestamp);

// Shut down old script, swap url, reload + initialize (§29.2.3).
void ScriptSystem::setUrl(Script *script,
    const MFString &newUrl, X3DExecutionContext &ctx);
```

Source resolution helpers (static, usable without a system instance):

```cpp
// Returns the decoded body of the first inline ecmascript:/javascript:/vrmlscript: entry.
static std::string ScriptSystem::decodeInlineSource(const MFString &url);

// Prefers Script.getSourceCode() (CDATA/reader-captured path), falls back to decodeInlineSource.
static std::string ScriptSystem::scriptSource(const Script &script);
```

`SaiContext` is handed to each backend at `load()` time and is the only channel from script code into the scene:

```cpp
// Read a field value (always permitted, §29.4.1).
std::any SaiContext::getField(X3DNode *node, const std::string &fieldName) const;

// Set a field. A SELF-write (the Script's own field) is posted as a cascade
// event so it fans out via ROUTEs; a permitted CROSS-node write (directOutput=TRUE)
// is a direct value mutation — no event, no ROUTE fan-out (SAI §4.5.2 / ADR-0029).
// Else (directOutput=FALSE) it throws (§29.2.6).
void SaiContext::setField(X3DNode *node, const std::string &fieldName, std::any value);

// Dynamic route management (requires directOutput=TRUE, §29.4.1). addRoute
// validates like a document ROUTE and throws std::invalid_argument on a null
// node (INVALID_NODE), an unknown field (INVALID_FIELD), a non-output source,
// a non-input sink, or a type mismatch. deleteRoute rejects null nodes.
void SaiContext::addRoute(X3DNode *fromNode, const std::string &fromField,
                          X3DNode *toNode,   const std::string &toField);
void SaiContext::deleteRoute(X3DNode *fromNode, const std::string &fromField,
                             X3DNode *toNode,   const std::string &toField);

// Browser object surface.
const std::string &SaiContext::getName() const;
const std::string &SaiContext::getVersion() const;
double SaiContext::currentTime() const;
double SaiContext::currentFrameRate() const;
void   SaiContext::print(const std::string &message);
```

`ScriptEngine` abstract interface (implement to add a new language backend):

```cpp
virtual ScriptHandle load(X3DNode &scriptNode, const std::string &source,
                          SaiContext &sai) = 0;
virtual void initialize(ScriptHandle handle) = 0;
virtual void shutdown(ScriptHandle handle) = 0;
virtual void prepareEvents(ScriptHandle handle, double now) = 0;
virtual void invoke(ScriptHandle handle, const std::string &eventName,
                    const std::any &value, X3DFieldType type,
                    double timestamp) = 0;
virtual void eventsProcessed(ScriptHandle handle, double timestamp) = 0;
```

`EcmaScriptBackend` additionally exposes two static marshalling methods (made public for unit testing):

```cpp
static void     EcmaScriptBackend::pushValue(duk_context *ctx, const std::any &value, X3DFieldType type);
static std::any EcmaScriptBackend::toValue  (duk_context *ctx, duk_idx_t idx,         X3DFieldType type);
```

### Seam points

- `X3DExecutionContext::addScriptSystem()` — registers the `ScriptSystem` as the post-cascade hook; the context drives `update()` (prepareEvents phase) and `runEventsProcessed()` around each cascade drain. `ScriptSystem` must be registered *first* among systems to guarantee §29.2.5 ordering.

- `ScriptEngine` seam — the only place Duktape API calls appear. All values crossing the seam are `std::any` boxed with the runtime's own field types and a `X3DFieldType` tag; no scripting-language objects cross this boundary.

- `DynamicFieldStore` / `effectiveFields()` — author `<field>` declarations captured by readers into `Scene::authorFields`, retained on declarations for PROTO templates (see [ADR-0014](../decisions/0014-dynamic-field-foundation.md)). `SaiContext::findField()` calls `effectiveFields(node, ctx.authorFields())` to resolve author fields alongside static fields. `EcmaScriptBackend::seedAuthorGlobals()` reads author initial values into JS globals before `initialize()`; `readbackAuthorGlobals()` reads them back after every handler and posts them as cascade events so ROUTEs fan out. Readback uses the active cascade timestamp through `SaiContext`.

- `Script.sourceCode` — readers write inline `<![CDATA[...]]>` blocks / JSON source members / VRML body text into `Script.getSourceCode()`. `ScriptSystem::scriptSource()` prefers this over the `url` inline-scheme decode, enabling file-authored scripts (the  S1 closure).

- SFNode handles (`NodeHandleTable`) — script code never holds a C++ pointer. Each backend interns a node into the per-script `SaiContext::nodeHandles()` table and hands the engine a small integer id. Duktape stores it under a hidden `\xff` symbol; QuickJS stores it in the opaque slot of a native `X3DNode` class. Scripts can neither read nor forge the id. Resolving an id yields the node's real owning `shared_ptr`, so an `SFNode` a script writes into a field shares ownership like DEF/USE, which matches the ECMAScript binding's node-reference semantics (ISO/IEC 19777-1). An id whose node the scene has dropped, or an unknown id, resolves to null. See [ADR-0048](../decisions/0048-script-node-handles.md).

- Field-type objects (`FieldTypesPrelude.hpp`) — a structured SF value is a JS object with own numeric properties (`x/y/z`, `r/g/b`, `x/y/z/angle`); that shape is what crosses the seam, and an object literal such as `{x: 1, y: 2, z: 3}` is still accepted. The prelude adds what ISO/IEC 19777-1 clause 7 scripts expect on top of it:
    - Constructors: `SFVec2f/2d/3f/3d/4f/4d`, `SFColor`, `SFColorRGBA`, `SFRotation` (four numbers, axis plus angle, or from and to vectors), `SFImage`, and every `MF*` name. MF values are plain JS arrays, so `new MFVec3f(a, b)` returns an `Array`.
    - Vector methods: `add`, `subtract`, `multiply`, `divide`, `negate`, `dot`, `length`, `normalize`, `toString`, plus `cross` on the 3-component types.
    - `SFColor`/`SFColorRGBA`: `setHSV`, `getHSV`, `toString`.
    - `SFRotation`: `getAxis`, `setAxis`, `inverse`, `multiply`, `multVec`, `slerp`, `toString`. `a.multiply(b)` applies `a` first and then `b`, the Open Inventor convention X_ITE also uses; the standard's wording does not fix the order.
    - Index access (`v[0]`, `c[2]`, `r[3]`) reads and writes the named component.

    Values the runtime pushes into a script (handler arguments, seeded author fields) get the same prototype. Each backend keeps its own name-to-prototype table (Duktape: the global stash; QuickJS: a read-only, non-configurable global), so a script that overwrites `SFVec3f` does not change what pushed values receive. Not provided: `SFNode`/`MFNode` construction from a string, `SFMatrix3f/4f` methods (matrices stay flat row-major arrays), `VrmlMatrix`, and the remaining `Browser` services (`createX3DFromURL`, `loadURL`, `replaceWorld`, `currentScene`, `createNode`). See SCRIPT-FIELD-TYPE-OBJECTS and SCRIPT-BROWSER-SERVICES.

- SFNode field access — a node handle exposes the node's fields as properties (ISO/IEC 19777-1). `node.translation` reads through `SaiContext::getField`, and the value carries the field-type methods. `node.translation = v` and the `set_` alias write through `SaiContext::setField`: allowed on another node only when `directOutput` is `TRUE` (otherwise the assignment throws), a direct non-routable mutation per ADR-0029, and an event post when the node is the Script itself. An unknown name reads as `undefined`; writing an unknown, `outputOnly` or `initializeOnly` field throws a `TypeError`. `SaiContext::fieldInfo()` supplies the field type for marshalling. Duktape implements this as a Proxy over the handle, QuickJS as exotic property hooks on its `X3DNode` class; the unforgeable id of ADR-0048 is unchanged. The SFNode methods (`getNodeName`, `getNodeType`, `toXMLString`, ...) are not provided. (SCRIPT-SFNODE-FIELD-ACCESS)

- Scene creation (`SaiContext::SceneFactory`) — `Browser.createX3DFromString(text)` and `Browser.createVrmlFromString(text)` call `SaiContext::createX3DFromString`, which forwards to a host-supplied factory (`ScriptSystem::setSceneFactory`). The script layer has no parser of its own; a host that links the runtime library passes `codec::makeStringSceneFactory()` (`runtime/parse/X3DParse.hpp`), as `x3d sim` does. Without a factory the calls throw. Both names take any encoding: a full document, an XML fragment with no `<X3D>` element, or ClassicVRML with no header line. The factory receives the complete string, including bytes after embedded NUL characters; the default factory strips a leading UTF-8 BOM before detecting the encoding. The result is an MFNode array of the new root nodes that also answers to `.rootNodes`, so `group.children = Browser.createX3DFromString(s).rootNodes` works. Creating nodes is not gated by `directOutput`; inserting them into another node's field is. `SaiContext` keeps the created roots alive for the script's lifetime, because a script's handle does not own a node (ADR-0048). The default factory expands PROTOs declared in the text and fetches nothing: Inline and EXTERNPROTO urls are left unresolved. Not carried over from the text: its ROUTEs, IMPORT/EXPORT statements and Script author fields. A node handle also has a string form, `[SFNode Transform]`. (SCRIPT-BROWSER-SERVICES)

- Deterministic clock — in both backends a small JS shim installed once the global object exists redirects the JS `Date()` (`Date.now()` and a zero-argument `new Date()`/`Date()`, in milliseconds = seconds × 1000) to the same injected `X3DExecutionContext` clock that `Browser.currentTime` reads, so a Script that reads `Date()` stays reproducible under `x3d sim`'s deterministic clock while `Date()` with explicit arguments, `Date.parse`/`Date.UTC` and the prototype methods are untouched.

- Engine error containment — every call into Duktape runs under `duk_safe_call` (Duktape is compiled as C++ with `DUK_USE_CPP_EXCEPTIONS`), so a script error raised while values are marshalled cannot abort the host. That covers a throwing getter, `toJSON`, Proxy trap or global accessor. A throw during readback drops that field's event for that callback, and the error is logged. QuickJS readback likewise drops the field on any pending exception instead of emitting a half-read value. See [ADR-0048](../decisions/0048-script-node-handles.md).

- Call budget — `ScriptEngine::setCallBudget()` (default 2 s, zero disables) bounds each entry into script code: load, initialize, one invoke, prepareEvents, eventsProcessed, shutdown. A call that runs past it is interrupted (Duktape exec-timeout check; QuickJS interrupt handler), logged as a script error, and the script stays loaded. A load whose top level times out fails. `ScriptEngine::setMemoryLimit()` (default 256 MiB, zero disables) caps script heaps: per script in Duktape (a counting allocator), across all scripts in QuickJS (its shared runtime). An allocation past the cap fails and becomes a contained out-of-memory error. This includes exposing newly created scene roots: a failed conversion discards its partial array and throws, and later script calls remain usable. See [ADR-0048](../decisions/0048-script-node-handles.md).

- `directOutput` gate & write semantics — `SaiContext::setField()` on a node other than the owning Script, and both `addRoute`/`deleteRoute`, throw `std::logic_error` when `Script.getDirectOutput()` is `false` (the safest conformant response to the spec's "result is UNDEFINED" clause, §29.4.1). A permitted SELF-write is posted as a cascade event (fans out via ROUTEs, §29.2.4); a permitted CROSS-node write is a direct value mutation via the reflection setter — no event is generated and no ROUTE fans out from the written field (SAI 19775-2 §4.5.2 / [ADR-0029](../decisions/0029-directoutput-non-routable.md)), though the target still generates its own output events per the field's access type. `SaiContext::setField()` falls back to the dynamic-field store for an author-declared target (absent from the static reflection table).

- External url fetch (`AssetResolver` seam) — a Script whose `url` list carries external (non-`ecmascript:`/`javascript:`/`vrmlscript:`) entries is fetched through the injectable `extract::AssetResolver` given to `ScriptSystem`: entries are tried in preference order, `Ready` bytes become the source, `Pending` defers to a later tick (the contract's retry), and `Failed` advances. The default resolver is the IO-free null stub, so a default-configured system never does I/O. The CLI (`tools/x3d-cli/sim_runtime.hpp`) injects `io::file::makeFileResolver()`.

- `autoRefresh` / `autoRefreshTimeLimit` — a loaded Script with `autoRefresh > 0` re-fetches and re-initializes on that interval (shutdown → resolve+load → initialize, §29.2.3), stopping once `autoRefreshTimeLimit` (measured from the load that opened the refresh window; 0 = no limit) has elapsed.

- Script output re-emit — an explicit same-value assignment to a Script `outputOnly` global still generates an event (§29.2.4 / ISO 19777-1); the no-op diff suppression in both backends' `readbackAuthorGlobals()` applies to `inputOutput` only (there the global is an echo of the stored value). (SCRIPT-OUTPUTONLY-REEMIT)

- `mustEvaluate` flag — `ScriptSystem::deliverInputEvent()` invokes the engine immediately when `true` (§29.4.1 eager path) or queues a `DeferredEvent` when `false` (flushed at `runEventsProcessed()` time, a permitted delay).

## Related

- [ADR-0013: ECMAScript Engine Choice for Script/SAI](../decisions/0013-js-engine-choice.md)
- [ADR-0022: Second ScriptEngine Backend (QuickJS) + Behavioral Swap-Test](../decisions/0022-scriptengine-second-backend-swap-test.md)
- [Seam-Status Matrix](../seam-status.md)
- [ADR-0014: Dynamic Field Foundation for Script Author Fields](../decisions/0014-dynamic-field-foundation.md)
- [Routes subsystem](../subsystems/routes.md)

- ISO/IEC 19775-1 §29 (Script component) and §4.4.8.3 (event-processing model) — normative per-tick ordering; §29.4.1 (SAI surface + `directOutput`/`mustEvaluate` gates); ISO/IEC 19777-1 (ECMAScript language binding, field-to-JS type mapping).
