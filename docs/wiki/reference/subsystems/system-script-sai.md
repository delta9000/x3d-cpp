---
title: Script and SAI runtime
summary: Script node runtime contract — ScriptSystem lifecycle, the SaiContext surface, the ScriptEngine seam, its Duktape and QuickJS backends, and their limits.
tags: [subsystem, script, sai, ecmascript, duktape, quickjs, dynamic-fields]
updated: 2026-11-15
related:
  - ../../explanation/architecture.md
  - ../script-api.md
  - ../../how-to/supply-resources.md
  - routes.md
  - ../seam-matrix.md
  - ../../explanation/decisions/0013-js-engine-choice.md
  - ../../explanation/decisions/0014-dynamic-field-foundation.md
  - ../../explanation/decisions/0022-scriptengine-second-backend-swap-test.md
  - ../../explanation/decisions/0029-directoutput-non-routable.md
  - ../../explanation/decisions/0048-script-node-handles.md
  - ../../explanation/decisions/0057-scene-owned-author-fields.md
  - ../../sai/index.md
  - ../../explanation/decisions/0047-sai-sister-repo-split.md
---

# Script and SAI runtime

## Purpose

The Script runtime makes `Script` nodes execute. It runs the ISO/IEC 19775-1
§29.2 lifecycle (load, initialize, prepareEvents, invoke, eventsProcessed,
shutdown), enforces the `directOutput` and `mustEvaluate` gates, and gives each
script an in-process `SaiContext`, its only channel back into the scene.

`SaiContext` is the native Script-backend API. It is not the
[SAI SDK](../../sai/index.md): the SDK's
[portable native provider](../../sai/reference/native-provider.md) does not
expose Script nodes, and linking both packages does not connect their scenes.

What script code sees (the `Browser` object, field-type objects, node handles)
is in the [Script ECMAScript API](../script-api.md). Wiring scripts into a host
is in [Supply resources through the seams](../../how-to/supply-resources.md#run-scripts).

## Key files

| File / directory | Role |
|---|---|
| `runtime/script/ScriptSystem.hpp` | `System` that drives one engine; owns per-script state (SAI surface, handle, deferred events, url and refresh state) |
| `runtime/script/ScriptEngine.hpp` | Abstract backend seam; `ScriptHandle` is an opaque `uint64_t`, 0 is invalid |
| `runtime/script/SaiContext.hpp` | In-process SAI surface bound to one Script node |
| `runtime/script/NodeHandleTable.hpp` | Per-script table that maps SFNode handle ids to nodes |
| `runtime/script/FieldTypesPrelude.hpp` | ES5.1 source both backends evaluate at load: the ISO/IEC 19777-1 field-type constructors and prototypes |
| `runtime/script/DeterministicDate.hpp` | QuickJS shim that routes `Date()` to the context clock; Duktape does the same through its `DUK_USE_DATE_GET_NOW` hook |
| `runtime/script/EcmaScriptBackend.hpp`, `.cpp` | Duktape 2.7.0 engine, one heap per script |
| `runtime/script/QuickJsBackend.hpp`, `.cpp` | QuickJS engine, one runtime per script |
| `runtime/script/vendor/duktape/` | Vendored Duktape |
| `runtime/script/tests/MockScriptEngine.hpp` | Recording engine for language-free lifecycle tests |

## Build targets

| Target | Option | Default | Installed |
|---|---|---|---|
| Seam, `ScriptSystem`, `SaiContext` | part of `x3d_cpp::sdk` | always | yes |
| `x3d_ecmascript_backend` (Duktape) | `X3D_CPP_BUILD_SCRIPT` | ON | no; in-tree target only |
| `x3d_quickjs` (QuickJS) | `X3D_CPP_BUILD_QUICKJS` | OFF | no; in-tree target only |

A consumer of the installed package supplies its own `ScriptEngine` or builds
the SDK in-tree to link a bundled engine.

## Interfaces and seams

### ScriptSystem

| Member | Contract |
|---|---|
| `ScriptSystem(engine, browserName, browserVersion, resolver = nullptr)` | `browserName` and `browserVersion` answer `Browser.getName()` and `getVersion()`. A null resolver becomes the IO-free null resolver |
| `setSceneFactory(SaiContext::SceneFactory)` | Parser behind `Browser.createX3DFromString` and `createVrmlFromString` for every script, enrolled now or later. Without one those calls throw |
| `setResourceLimits(SaiContext::ResourceLimits)` | Must be called before the first `attach` |
| `attach(node, ctx)` | Enrolls a Script node; loads and initializes it if `load` is TRUE (§29.2.3) |
| `deliverInputEvent(script, eventName, value, type, timestamp)` | Delivers one `inputOnly` event: invokes now when `mustEvaluate` is TRUE, otherwise queues it for the eventsProcessed flush |
| `setUrl(script, newUrl, ctx)` | Shuts the script down, swaps the url, reloads and initializes (§29.2.3) |
| `saiFor(script)`, `handleFor(script)` | The script's `SaiContext` and engine handle |
| `decodeInlineSource(url)` (static) | Body of the first `ecmascript:`, `javascript:` or `vrmlscript:` entry |
| `scriptSource(script)` (static) | `Script.sourceCode` if set, otherwise `decodeInlineSource(url)` |
| `shutdown()` | Unloads every script while the context is alive |

`X3DExecutionContext::addScriptSystem(sys)` registers the system and wires
three hooks:

| Hook | Runs | Does |
|---|---|---|
| `update(now, ctx)` | Each tick, ahead of every other system | prepareEvents for every loaded script, once per timestamp (§29.2.5) |
| `runEventsProcessed(ctx)` | After the cascade drains | Flushes deferred inputs, then eventsProcessed at most once per script that received an event (§29.2.4) |
| `onAuthorInput(...)` | On each event that reaches a Script author field | `inputOnly`: runs the handler through `deliverInputEvent`. `inputOutput`: refreshes the script global through `ScriptEngine::updateField` |

Hosts must not forward author inputs to `deliverInputEvent` themselves; the
handler would run twice.

### SaiContext

| Member | Contract |
|---|---|
| `getField(node, field)` | Always permitted (§29.4.1) |
| `setField(node, field, value)` | Own field: posted as a cascade event, so ROUTEs fan out. Another node: requires `directOutput`, and is a direct mutation with no event and no ROUTE fan-out from the written field (ADR-0029). Falls back to the author-field store for declared fields |
| `addRoute(from, fromField, to, toField)` | Requires `directOutput`. Validated like a document ROUTE: throws `std::invalid_argument` on a null node, an unknown field, a non-output source, a non-input sink or a type mismatch |
| `deleteRoute(...)` | Requires `directOutput`; rejects null nodes |
| `fieldInfo(node, field)` | Field type for marshalling; resolves author fields and `set_`/`_changed` aliases |
| `createX3DFromString(text)` | Forwards to the scene factory; keeps the created roots alive for the script's lifetime |
| `getName()`, `getVersion()`, `currentTime()`, `currentFrameRate()`, `print(text)` | The `Browser` object's data |
| `log()`, `logTruncated()` | Captured `print` output and whether it hit the log cap |
| `nodeHandles()` | The script's `NodeHandleTable` |

A write or route call that `directOutput` forbids throws `std::logic_error`,
the safest response to the "result is UNDEFINED" clause of §29.4.1.

### ScriptEngine

Implement these to add a language backend:

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
virtual void updateField(ScriptHandle handle, const std::string &name,
                         const std::any &value, X3DFieldType type) {}
```

Values cross the seam as `std::any` holding the runtime's own field types plus
an `X3DFieldType` tag; no language objects cross it. SFString and MFString
values keep their byte length, embedded NULs included.

An engine is single-threaded: all calls on one instance, for any of its
scripts, come from one thread at a time (see
[Tick model](../../explanation/tick-model.md)).

`EcmaScriptBackend::pushValue` and `toValue` are public static marshalling
functions for unit tests.

### Seam points

| Seam | Contract |
|---|---|
| Author fields | Readers capture `<field>` declarations into `Scene::authorFields` ([ADR-0057](../../explanation/decisions/0057-scene-owned-author-fields.md)). `SaiContext` and both engines read and write through the context's store; there is no global store. Engines seed author values into globals before `initialize()` and read them back after every handler, posting changes as cascade events at the active timestamp |
| Node-valued author defaults | XML, JSON and ClassicVRML readers keep SFNode and MFNode defaults with DEF/USE identity; all writers emit them |
| `Script.sourceCode` | Readers store CDATA, JSON source members and VRML body text here; `scriptSource()` prefers it over the url |
| External url fetch | Non-inline url entries go through the `extract::AssetResolver` given to `ScriptSystem`, in preference order: `Ready` bytes become the source, `Pending` retries on a later tick, `Failed` tries the next entry. The default resolver does no I/O; `x3d sim` passes `io::file::makeFileResolver()` |
| `autoRefresh` | A loaded script with `autoRefresh > 0` reloads and reinitializes on that interval (§29.2.3) until `autoRefreshTimeLimit` has passed since the load that opened the window (0 = no limit) |
| Scene factory | `codec::makeStringSceneFactory()` (`runtime/parse/X3DParse.hpp`) is the SDK's parser-backed factory |
| Node handles | Scripts hold table ids, never pointers. An id resolves to the node's owning `shared_ptr`; a dropped or unknown id resolves to null ([ADR-0048](../../explanation/decisions/0048-script-node-handles.md)) |

### Limits

| Limit | Set with | Default | On breach |
|---|---|---|---|
| Time per call into script code (load, initialize, one invoke, prepareEvents, eventsProcessed, shutdown) | `ScriptEngine::setCallBudget()` | 2 s | Interrupted and logged as a script error; the script stays loaded. A load whose top level times out fails |
| Heap per script | `ScriptEngine::setMemoryLimit()` | 256 MiB | The allocation fails and becomes a contained out-of-memory error. One script at its cap leaves the others their full budget |
| Captured `print` log | `ResourceLimits::logBytes` | 1 MiB | Keeps the prefix; `logTruncated()` turns true |
| Scene source text, cumulative | `ResourceLimits::sceneSourceBytes` | 16 MiB | Scene creation throws a catchable error |
| Scene factory calls | `ResourceLimits::sceneCalls` | 4096 | Same; failed calls still count |
| Retained created roots | `ResourceLimits::createdRoots` | 65536 | Same |

A zero call budget or memory limit disables it. A zero resource limit is a
zero quota. Resource limits are per `SaiContext`, never reset between callbacks
or reloads, and are separate from heap limits; they do not account for parser
expansion, native graph sizes, field writes or route creation.

Every Duktape entry runs under `duk_safe_call` with C++ exceptions, so a script
error during marshalling (a throwing getter, `toJSON`, Proxy trap or global
accessor) cannot abort the host. A throw during readback drops that field's
event for that callback and is logged; QuickJS readback likewise drops the
field on a pending exception.

## How it is tested

| CTest target | Covers |
|---|---|
| `x3d_script_system` | §29.2 lifecycle and event ordering with `MockScriptEngine` |
| `x3d_sai_context` | `SaiContext` field access, gates and route validation |
| `x3d_ecmascript_backend` | Duktape marshalling, Browser object, field types, node fields, limits |
| `x3d_script_author_runtime` | Author-field seeding, readback and routed outputs |
| `x3d_ecmascript_corpus_smoke`, `x3d_script_corpus_e2e` | Corpus scripts through the full runtime |
| `x3d_quickjs_backend` | QuickJS backend (`X3D_CPP_BUILD_QUICKJS`) |
| `x3d_quickjs_swap` | The same scripts on both engines give the same traces ([ADR-0022](../../explanation/decisions/0022-scriptengine-second-backend-swap-test.md)) |

Script conformance gaps are tracked as `SCRIPT-*` entries in
`docs/conformance/findings.yaml`.

## Related specs and ADRs

- ISO/IEC 19775-1 §29 (Script component) and §4.4.8.3 (event model);
  ISO/IEC 19777-1 (ECMAScript binding).
- [Script ECMAScript API](../script-api.md)
- [Supply resources through the seams](../../how-to/supply-resources.md#run-scripts)
- [ADR-0013: ECMAScript engine choice](../../explanation/decisions/0013-js-engine-choice.md)
- [ADR-0014: Dynamic field foundation](../../explanation/decisions/0014-dynamic-field-foundation.md)
- [ADR-0022: Second ScriptEngine backend and swap test](../../explanation/decisions/0022-scriptengine-second-backend-swap-test.md)
- [ADR-0029: directOutput writes are not routable](../../explanation/decisions/0029-directoutput-non-routable.md)
- [ADR-0048: Script node handles](../../explanation/decisions/0048-script-node-handles.md)
- [ADR-0057: Scene-owned author fields](../../explanation/decisions/0057-scene-owned-author-fields.md)
- [ADR-0047: SAI sister repository split](../../explanation/decisions/0047-sai-sister-repo-split.md)
- [Routes subsystem](routes.md)
- [Seam matrix](../seam-matrix.md)
- [Architecture](../../explanation/architecture.md)
- [SAI SDK](../../sai/index.md)
