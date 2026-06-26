---
title: Script / SAI Runtime
summary: Script node runtime — ScriptEngine seam, SAI execution context, ECMAScript (Duktape) backend, and dynamic field marshalling.
tags: [subsystem, script, sai, ecmascript, duktape, quickjs, dynamic-fields]
updated: 2026-06-23
related:
  - ../architecture.md
  - ../subsystems/routes.md
  - ../seam-status.md
  - ../decisions/0013-js-engine-choice.md
  - ../decisions/0014-dynamic-field-foundation.md
  - ../decisions/0022-scriptengine-second-backend-swap-test.md
---

# Script / SAI Runtime

## Purpose

The Script / SAI Runtime makes `Script` nodes execute. It owns the full ISO/IEC 19775-1 §29.2 lifecycle (load → initialize → prepareEvents → invoke → eventsProcessed → shutdown), enforces the `directOutput` and `mustEvaluate` field gates, and exposes the in-process Scene Access Interface (SAI) as the only sanctioned channel from script code back into the running scene. The subsystem is language-agnostic: the `ScriptEngine` abstract seam carries no scripting-language types, so a Lua, Python, or alternative ECMAScript backend could be substituted without touching the runtime.

!!! note "Seam proven generic — second backend (QuickJS)"
    A **second, independent `ScriptEngine` backend now exists**: `QuickJsBackend`
    (`runtime/script/QuickJsBackend.{hpp,cpp}`, quickjs-ng v0.15.1) behind the
    `X3D_CPP_BUILD_QUICKJS` build option, with **no core `#ifdef`**. It implements the
    *unchanged* `ScriptEngine` interface alongside the Duktape `EcmaScriptBackend`. The
    `x3d_quickjs_swap` test (`runtime/script/tests/quickjs_swap_test.cpp`) drives the same
    fixtures through both backends and asserts **identical observable parity** (the field writes
    and ROUTE-target values into the cascade), CI-gated. Because the interface carried two
    backends with no signature change, the Script/SAI seam (`ScriptEngine` / `ScriptSystem` /
    `SaiContext`) is now **`[STABLE]`** in `include/x3d/sdk.hpp`. See the
    [Seam-Status Matrix](../seam-status.md) and
    [ADR-0022](../decisions/0022-scriptengine-second-backend-swap-test.md). The `SCR-*`
    conformance claims are backend-independent — the swap-test evidences they hold identically
    under QuickJS.

## Key files

| File / directory | Role |
|---|---|
| `runtime/script/ScriptSystem.hpp` | `System` subclass that drives one language backend; owns per-script `Entry` state (SAI surface, load handle, deferred-event queue, tick flags); wires `prepareEvents` and `eventsProcessed` phases around the cascade drain |
| `runtime/script/ScriptEngine.hpp` | Abstract backend seam: `load / initialize / prepareEvents / invoke / eventsProcessed / shutdown`; defines `ScriptHandle` (opaque `uint64_t`, 0 = invalid) |
| `runtime/script/SaiContext.hpp` | In-process SAI surface bound to one Script node: field get/set, dynamic `addRoute`/`deleteRoute`, browser-info queries (`getName`, `getVersion`, `currentTime`, `currentFrameRate`, `print`); enforces the `directOutput` gate |
| `runtime/script/EcmaScriptBackend.hpp` | `ScriptEngine` implementation using vendored Duktape 2.7.0; one `duk_context` per loaded script; `pushValue`/`toValue` static methods exposed for unit testing |
| `runtime/script/EcmaScriptBackend.cpp` | Full marshalling (SF/MF types ↔ JS), Browser global object, `seedAuthorGlobals`/`readbackAuthorGlobals` for dynamic author-field integration |
| `runtime/script/vendor/duktape/` | Vendored Duktape 2.7.0 (single-file C library) |
| `runtime/script/tests/MockScriptEngine.hpp` | Recording `ScriptEngine` for language-free Track-A tests; arms reactions per event name; exposes a `calls` log |
| `runtime/script/tests/` | All unit and integration tests for this subsystem |

## Interfaces and seams

### Exposed interface

**`ScriptSystem`** is registered with `X3DExecutionContext` via `addScriptSystem()`. Thereafter the context drives both phases each tick:

- `ScriptSystem::update(now, ctx)` — called by the context's per-tick System pass; runs `prepareEvents` for every loaded script before any ROUTE processing (§29.2.5, SCR-001 once-per-timestamp guard).
- `ScriptSystem::runEventsProcessed(ctx)` — called by the context after the cascade drain; flushes deferred (`mustEvaluate=FALSE`) inputs, then calls `eventsProcessed` at most once per script that received an event this cascade (§29.2.4).

**Script lifecycle entry points** (called by the cascade or the input seam):

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

**Source resolution helpers** (static, usable without a system instance):

```cpp
// Returns the decoded body of the first inline ecmascript:/javascript:/vrmlscript: entry.
static std::string ScriptSystem::decodeInlineSource(const MFString &url);

// Prefers Script.getSourceCode() (CDATA/reader-captured path), falls back to decodeInlineSource.
static std::string ScriptSystem::scriptSource(const Script &script);
```

**`SaiContext`** is handed to each backend at `load()` time and is the only channel from script code into the scene:

```cpp
// Read a field value (always permitted, §29.4.1).
std::any SaiContext::getField(X3DNode *node, const std::string &fieldName) const;

// Set a field as a cascade event (cross-node only if directOutput=TRUE; else throws, §29.2.6).
void SaiContext::setField(X3DNode *node, const std::string &fieldName, std::any value);

// Dynamic route management (requires directOutput=TRUE, §29.4.1).
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

**`ScriptEngine`** abstract interface (implement to add a new language backend):

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

**`EcmaScriptBackend`** additionally exposes two static marshalling methods (made public for unit testing):

```cpp
static void     EcmaScriptBackend::pushValue(duk_context *ctx, const std::any &value, X3DFieldType type);
static std::any EcmaScriptBackend::toValue  (duk_context *ctx, duk_idx_t idx,         X3DFieldType type);
```

### Seam points

- **`X3DExecutionContext::addScriptSystem()`** — registers the `ScriptSystem` as the post-cascade hook; the context drives `update()` (prepareEvents phase) and `runEventsProcessed()` around each cascade drain. `ScriptSystem` must be registered *first* among systems to guarantee §29.2.5 ordering.

- **`ScriptEngine` seam** — the only place Duktape API calls appear. All values crossing the seam are `std::any` boxed with the runtime's own field types and a `X3DFieldType` tag; no scripting-language objects cross this boundary.

- **`DynamicFieldStore` / `effectiveFields()`** — author `<field>` declarations captured by readers into the dynamic-field store (see [ADR-0014](../decisions/0014-dynamic-field-foundation.md)). `SaiContext::findField()` calls `effectiveFields(node)` to resolve author fields alongside static fields. `EcmaScriptBackend::seedAuthorGlobals()` reads author initial values into JS globals before `initialize()`; `readbackAuthorGlobals()` reads them back after every handler and posts them as cascade events so ROUTEs fan out.

- **`Script.sourceCode`** — readers write inline `<![CDATA[...]]>` blocks / JSON source members / VRML body text into `Script.getSourceCode()`. `ScriptSystem::scriptSource()` prefers this over the `url` inline-scheme decode, enabling file-authored scripts (the SCR-SAI-DYN S1 closure).

- **`directOutput` gate** — `SaiContext::setField()` on a node other than the owning Script, and both `addRoute`/`deleteRoute`, throw `std::logic_error` when `Script.getDirectOutput()` is `false`. This is the safest conformant response to the spec's "result is UNDEFINED" clause (§29.4.1).

- **`mustEvaluate` flag** — `ScriptSystem::deliverInputEvent()` invokes the engine immediately when `true` (§29.4.1 eager path) or queues a `DeferredEvent` when `false` (flushed at `runEventsProcessed` time, a permitted delay).

## How it is tested

**Track-A (no JavaScript, uses `MockScriptEngine`):**

- `ctest --preset dev -R x3d_sai_context` (`runtime/script/tests/sai_context_test.cpp`) — field get/set round-trip via reflection; `directOutput` write-gate (cross-node rejected/allowed); `addRoute`/`deleteRoute` gate and live-route effect; browser-info surface; full seam call-log ordering via the mock.
- `ctest --preset dev -R x3d_script_system` (`runtime/script/tests/script_system_test.cpp`) — `initialize()` before first event; `load=FALSE` defers load; `prepareEvents` once per timestamp; inputOnly → `invoke` with triggering timestamp; outputOnly write → cascade; `eventsProcessed` once per batch, only for receivers; `set_url` shutdown → reload → reinitialize order; `directOutput` gate end-to-end; `mustEvaluate` eager vs. deferred paths; SCR-002 teardown shutdown.

**Track-B (real Duktape backend):**

- `ctest --preset dev -R x3d_ecmascript_backend` (`runtime/script/tests/ecmascript_backend_test.cpp`) — inline source decode; load/initialize/invoke/eventsProcessed/prepareEvents/shutdown lifecycle; syntax-error → `kInvalidScriptHandle`; multiple independent `duk_context` instances; field marshalling round-trips for all SF/MF scalar and structured types (T12); handler receives `(value, timestamp)` (T13); `Browser` global (`getName`/`getVersion`/`currentTime`/`print`) backed by `SaiContext` (T14); end-to-end `Browser.addRoute` with `SFNode` marshalling and live route propagation (T15); `directOutput=FALSE` — `Browser.addRoute` throws into JS, no route added (T16).
- `ctest --preset dev -R x3d_ecmascript_corpus_smoke` (`runtime/script/tests/ecmascript_corpus_smoke_test.cpp`) — corpus smoke over the X3D conformance archive (script nodes parsed without crash).

**Track-C (second backend — QuickJS, built only with `-DX3D_CPP_BUILD_QUICKJS=ON`):**

- `ctest -R x3d_quickjs_backend` (`runtime/script/tests/quickjs_backend_test.cpp`) — the QuickJS backend's own lifecycle + marshalling + Browser + handler-dispatch tests (mirrors the Duktape T1–T16 surface).
- `ctest -R x3d_quickjs_swap` (`runtime/script/tests/quickjs_swap_test.cpp`) — **the genericity proof**: drives identical fixtures through both the Duktape and QuickJS backends and asserts identical observable behavior (cascade field writes + ROUTE-target values). Gated in CI by the `QuickJS seam swap-test` job.

**Integration (file-authored Script un-tabling, SCR-SAI-DYN S1):**

- `ctest --preset dev -R x3d_script_author_runtime` (`runtime/script/tests/script_author_runtime_test.cpp`) — end-to-end: author fields via `DynamicFieldStore` + `Script.sourceCode` → `ScriptSystem` → `EcmaScriptBackend` → author outputOnly write fans out via ROUTE to a real target node.
- `ctest --preset dev -R x3d_script_corpus_e2e` (`runtime/script/tests/script_corpus_e2e_test.cpp`) — real-reader paths (XML, ClassicVRML) over `AuthorScriptExample.x3d`/`.x3dv` + the real Savage `CanopyExample.x3d`; previously-inert scripts now dispatch their handlers.
- `ctest --preset dev -R x3d_script_cdata_audit` — CDATA capture audit over the corpus.

## Related specs and ADRs

- [ADR-0013: ECMAScript Engine Choice for Script/SAI](../decisions/0013-js-engine-choice.md) — rationale for Duktape as the shipped backend and quickjs-ng as the migration target.
- [ADR-0022: Second ScriptEngine Backend (QuickJS) + Behavioral Swap-Test](../decisions/0022-scriptengine-second-backend-swap-test.md) — the genericity proof: a second independent backend + a CI-gated swap-test freezes the seam `[STABLE]`; the pattern for every seam card.
- [Seam-Status Matrix](../seam-status.md) — the live tracker; ScriptEngine is the first GREEN row.
- [ADR-0014: Dynamic Field Foundation for Script Author Fields](../decisions/0014-dynamic-field-foundation.md) — the `DynamicFieldStore` + `effectiveFields()` design that underpins author-field dispatch and readback.
- [Routes subsystem](../subsystems/routes.md) — the route graph and field-address model that `SaiContext::addRoute`/`deleteRoute` and `setField` post events into.
- Spec: `docs/superpowers/specs/2026-06-16-script-sai-runtime-design.md` — the dated design spec for this subsystem (lifecycle ordering, mustEvaluate, directOutput gate, §29.2 mapping).
- Spec: `docs/superpowers/specs/2026-06-17-script-cdata-untabling-design.md` — the S1 un-tabling design: `Script.sourceCode`, `seedAuthorGlobals`/`readbackAuthorGlobals`, reader CDATA capture.
- ISO/IEC 19775-1 §29 (Script component) and §4.4.8.3 (event-processing model) — normative per-tick ordering; §29.4.1 (SAI surface + `directOutput`/`mustEvaluate` gates); ISO/IEC 19777-1 (ECMAScript language binding, field-to-JS type mapping).
