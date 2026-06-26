# Script + SAI Runtime — Design Spec

**Date:** 2026-06-16
**Status:** DESIGNED (next big push after M2D; do NOT launch until the M2D workflow lands)
**Goal milestone:** toward **M4** (the Script/SAI behavior runtime — the second real consumer of the event system)
**Branch:** `modernize-x3d-spec`
**Builds on:** the event cascade (`X3DExecutionContext`, single-timestamp cascade), the dynamic-field
foundation (`docs/superpowers/specs/2026-06-05-dynamic-field-foundation-spec.md`, Script/ProtoInstance
author fields), and the seam philosophy (IO-free core + embedder-supplied capabilities, like the asset resolver).

## 1. Goal

Make `Script` nodes actually *run*: declared `inputOnly` events invoke script code; the code reads/writes
fields and touches the scene via the SAI; `outputOnly` writes become events in the same cascade. Ship a
language-agnostic **ScriptEngine seam** (the spec mandates no specific language) plus a **reference
ECMAScript backend** so scripted behavior works out of the box. This is the core of the X3D behavior runtime.

## 2. Spec grounding (ISO/IEC 19775-1 §29 Scripting, verified via spec_rag + prose mirror this session)

- **Script node (§29.4.1):** `url` (MFString — inline `ecmascript:`/`javascript:` or external), `directOutput`,
  `mustEvaluate`, `load`, `autoRefresh`/`autoRefreshTimeLimit`, + **dynamic author fields** declared with
  access types (`inputOnly`/`outputOnly`/`initializeOnly`/`inputOutput`) — same syntax as PROTO interfaces.
- **Language-agnostic (§29.1):** "Browsers are not required to support any specific language… shall adhere
  to that language binding (ISO/IEC 19777)." → a pluggable engine seam is the spec-correct architecture.
- **Execution (§29.2.2):** scripts receive events in **timestamp order**; events a script generates carry the
  generating event's timestamp (conceptually zero elapsed time). `set_url` → `shutdown()` old, `initialize()` new.
- **Lifecycle (§29.2.3-5):** `initialize()` before the first event; `shutdown()` on removal/url-change;
  `eventsProcessed()` after a batch of received events (lets order-independent scripts emit fewer events);
  `prepareEvents()` **once per timestamp, before any ROUTE processing** (collect async/external data, emit
  sensor-like events).
- **directOutput (§29.2.6):** if TRUE, a Script with SFNode/MFNode access may **directly post events to**
  those nodes and read their last field values (SAI write access). If FALSE, it communicates only through
  its declared `outputOnly` fields.
- **mustEvaluate (§29.4.1):** FALSE → browser may delay input delivery until outputs are needed (lazy);
  TRUE → deliver inputs ASAP (for side-effecting scripts, e.g. network).
- **SAI (§29.4.1, §4.9; ISO 19775-2):** the run-time scene-access services a script uses — read/set fields,
  send events, add/delete routes, traverse, create/destroy nodes, query the browser (currentTime, world URL,
  name/version). This push ships the **in-process core** of these; the dynamic-construction + external/network
  SAI is deferred (§6).

## 3. Scope

**In scope (the big push, two tracks):**

**Track A — language-agnostic runtime (testable with a C++ mock backend, no JS):**
- `ScriptEngine` seam — abstract interface a backend implements: `load(node, source) → handle`,
  `initialize/shutdown(handle)`, `invoke(handle, fnName, eventName, value, timestamp)`,
  `eventsProcessed(handle)`, `prepareEvents(handle, now)`.
- `SaiContext` — the in-process SAI surface the backend calls back into: `getField/setField(node, name)`
  (via reflection / dynamic-field foundation), `addRoute/deleteRoute`, `getName/getVersion`,
  `currentTime/currentFrameRate`, `print`. Honors `directOutput` (gates writes to other nodes).
- `ScriptSystem` — a System wired into `tick()`/cascade: manages Script lifecycle (load on `load=TRUE`/url,
  `initialize()` before first event, `shutdown()` on url-change/removal), dispatches each `inputOnly` event
  to the engine, runs `prepareEvents()` once-per-timestamp before routes and `eventsProcessed()` after a
  batch, and turns `outputOnly` writes into cascade events at the correct timestamp. Honors `mustEvaluate`
  (eager vs lazy) and `directOutput`.

**Track B — reference ECMAScript backend (real inline scripts):**
- **Duktape 2.7.0 is already vendored** at `runtime/script/vendor/duktape/` (`duktape.c`/`.h` + `duk_config.h` +
  `DUKTAPE_LICENSE.txt`, MIT; commit `ba7a2a1`; compiles clean `gcc -c -std=c99` → 492 KB). ES5.1+, no IO deps,
  matches the `runtime/parse/tinfl.h` bundled-permissive-dep pattern. QuickJS/V8 remain swap-in via the seam.
  → Track B needs no fetch step; the workflow just CMake-wires it.
- ECMAScript binding (ISO 19777-1 conventions) marshalling the **core field types** ↔ JS: SF/MF of
  Bool/Int32/Float/Double/Time/String, SFVec2f/3f/4f(+d), SFColor(RGBA), SFRotation, SFNode — SF* as JS
  objects with the conventional accessors, MF* as array-like.
- The `Browser` global object (currentTime, currentFrameRate, getName, getVersion, addRoute, deleteRoute,
  print) backed by `SaiContext`; per-field handler dispatch (`set_<name>` / function-named) + the lifecycle
  functions; inline `ecmascript:`/`javascript:` url decoding.

**Out of scope (deferred, each with a real reason):**
- **SCR-ASYNC** — asynchronous/spontaneous scripts (§29.2.7, sensor-like): needs threading + nondeterministic
  timestamp assignment; breaks the deterministic single-thread cascade this runtime relies on.
- **SCR-SAI-DYN** — dynamic scene construction SAI (`createX3DFromString`/`Stream`, node/PROTO create/destroy,
  profile/component introspection) and the **external/networked SAI**: large; the in-process field/route/
  browser-info core covers the scripted-behavior corpus. Add when a consumer needs runtime graph mutation.
- **SCR-REFRESH** — `autoRefresh`/`autoRefreshTimeLimit` periodic url re-fetch: IO + timer driven; niche.
- **Non-ECMAScript bindings** (Java, ISO 19777-2): BY-DESIGN — the seam exists precisely so other backends
  plug in; only the ECMAScript reference backend ships.

## 4. Architecture

```
event cascade (X3DExecutionContext.tick(now))
  ├─ prepareEvents(now)  ── ScriptSystem → engine.prepareEvents (once/timestamp, BEFORE routes)
  ├─ route processing / inputOnly delivery ── ScriptSystem dispatches set_<field> → engine.invoke
  │        engine reads/writes fields + scene via SaiContext (directOutput gates node writes)
  │        outputOnly writes → cascade events (timestamp of the triggering event)
  └─ eventsProcessed()   ── ScriptSystem → engine.eventsProcessed (after the batch)

ScriptEngine (seam, abstract)  ◀── implemented by ──  EcmaScriptBackend (Duktape)
        ▲                                                    │ marshals fields ↔ JS, Browser object
        └──────────── SaiContext (in-process SAI) ◀──────────┘ (field/route/browser-info; directOutput gate)
```

### 4.1 Components

| Unit | File(s) | Responsibility |
|---|---|---|
| `ScriptEngine` seam | `runtime/script/ScriptEngine.hpp` (new) | Abstract backend interface (load/initialize/invoke/eventsProcessed/prepareEvents/shutdown). |
| `SaiContext` | `runtime/script/SaiContext.hpp` (new) | In-process SAI: field get/set (reflection + dynamic-field foundation), addRoute/deleteRoute, browser info; `directOutput` write-gate. |
| `ScriptSystem` | `runtime/script/ScriptSystem.hpp` (new) | Lifecycle + cascade wiring; honors mustEvaluate/directOutput/load; registered so `tick()` drives it. Holds the **scheme→backend registry** (§4.2) and resolves each Script's `url` preference-list to the first registered backend (ECMAScript is the only one registered now). |
| Mock backend | `runtime/script/tests/MockScriptEngine.hpp` (new, test-only) | Records calls / scripted responses — lets Track A be tested with zero JS. |
| Duktape vendor | `runtime/script/vendor/duktape/{duktape.c,duktape.h,duk_config.h}` (vendored) | The bundled engine; compiled by CMake. |
| `EcmaScriptBackend` | `runtime/script/EcmaScriptBackend.hpp/.cpp` (new) | Implements `ScriptEngine` on Duktape: field↔JS marshalling, Browser object, handler dispatch, inline-url decode. |
| Build wiring | `CMakeLists.txt` | Compile duktape + the backend; new test targets. |

Boundaries: Track A never depends on Duktape (seam + mock). Track B is the only place the engine lives.

### 4.2 Language-agnostic seam — do NOT pigeonhole to ECMAScript

ECMAScript is the only backend we *ship*, but the seam must not bake in JS assumptions — the spec is
explicitly multi-language and we keep that door open at zero extra cost:

- **url-scheme → backend registry (the dispatch).** `ScriptSystem` holds a registry keyed by language
  identifier; per the spec a Script's `url` is a **preference-ordered list** and the browser runs the
  **first entry whose language has a registered backend** (§29.2.8). Language is inferred from the scheme/
  extension (`ecmascript:`/`javascript:`/`.js` → "ecmascript"; `.class`/`.jar` → "java"; custom protocols
  like `lua:`/`python:` are spec-sanctioned, §9.2.3). Adding a language later = **register one backend**;
  `ScriptSystem`, `SaiContext`, lifecycle, event ordering, and the `directOutput` gate are untouched.
- **The seam carries no JS types.** `ScriptEngine::invoke`/field exchange pass X3D field values (the runtime's
  own `FieldValue`/reflection types), never JS objects. The X3D-field ↔ native-value **marshalling lives
  entirely inside each backend** (the `SFVec3f{x,y,z}` / array-like mapping is `EcmaScriptBackend`'s private
  concern, per ISO 19777-1). A Lua/Java/Python backend would map the same `FieldValue`s to its own idiom.
- **Conformance split (state it in scenes).** ECMAScript (19777-1) and Java (19777-2) are the only ISO-normative
  bindings → portable. `lua:`/`python:` Scripts are custom extensions the seam allows but that only run where
  that backend is registered → non-portable. The registry makes this an honest, per-deployment choice.

This is design-only here (ECMAScript is the lone registered backend); it just guarantees the runtime never has
to be refactored to admit Lua/Java/Python — they slot in as isolated backend modules. See the forward note in the
session log for the per-language embedding/marshalling/weight comparison.

## 5. Known risk — golden hash

This work **prefers codegen-free** (Script node + its emit methods are generated; author fields use the existing
dynamic-field foundation; the runtime reads/writes them via reflection). **IF** dispatching a Script's dynamic
`inputOnly` field to a handler needs a generator/template change (e.g. the dynamic-field foundation lacks per-field
handler registration for Script), that is **AUTHORIZED** (human approved 2026-06-16): do it the right way — edit the
**generator/templates** (NEVER hand-edit `generated_cpp_bindings/`), then `mise run gen`, commit the regenerated
bindings, and update the committed golden hash so `mise run golden` passes. A golden **change is acceptable** when it
is a clean regeneration scoped to exactly the dynamic-field-handler addition; a hand-edited or unrelated-node golden
change is not. Byte-identical remains the preferred outcome.

## 6. Testing strategy

- **Track A (mock backend)** `x3d_script_system`: lifecycle ordering (initialize before first event;
  prepareEvents before routes; eventsProcessed after a batch; shutdown on url-change); inputOnly→invoke;
  outputOnly→cascade event at the right timestamp; directOutput gate (write to another node allowed only when
  TRUE); mustEvaluate eager vs lazy.
- **Track B (Duktape)** `x3d_ecmascript_backend`: real inline scripts — `ecmascript:` decode; an `initialize()`
  that prints; a `set_fraction`-style handler that reads an input and writes an outputOnly that fires a ROUTE;
  field marshalling round-trips for each core SF/MF type; Browser.addRoute + currentTime.
- **Corpus smoke**: run a handful of real Script-bearing `.x3d` from `<x3d-render-workspace>/testdata`
  through parse → expand → initialize → a few ticks; assert no crash and an expected output event on a known scene.
- Gates: golden byte-identical (see §5), full ctest green.

## 7. Deferred follow-ups (BACKLOG, with reasons) — see §3 out-of-scope: SCR-ASYNC, SCR-SAI-DYN, SCR-REFRESH.

## 8. Resolved decisions (made without back-and-forth; flag if you disagree)
1. **ScriptEngine seam + reference backend** (not engine-baked-in). Spec-mandated (no required language) + matches the IO-free/seam architecture.
2. **Reference backend = Duktape** (MIT, 2-file, smallest embed; ES5.1+ covers X3D's simple scripts). QuickJS/V8 swap in via the seam. Bundled like `tinfl.h`.
3. **Two tracks** — language-agnostic runtime (mock-tested) + Duktape backend — so the spec-faithful core is verifiable without the engine.
4. **In-process core SAI only** (fields/routes/browser-info). Dynamic-construction + external SAI deferred (SCR-SAI-DYN).
5. **Async scripts deferred** (SCR-ASYNC) — preserves the deterministic single-thread cascade.

## 9. Workflow design (the big push)

Single fan-out workflow, same gating as the M2D push (adversarial review per unit, golden+ctest gate):
1. **Spec-check (parallel, read-only):** (a) ECMAScript binding field↔JS mapping + Browser object (ISO 19777-1
   conventions — flag convention-vs-normative since 19777 may be outside the prose mirror); (b) cascade
   lifecycle ordering (prepareEvents/eventsProcessed/initialize, §29.2 + §4.4.8 event model); (c)
   directOutput/mustEvaluate exact rules; (d) the dynamic-field foundation API for Script author fields
   (read the 2026-06-05 spec + the runtime) — including whether handler dispatch needs codegen (the §5 risk).
2. **Implement (sequential where files are shared):** U1 seam + SaiContext + mock backend; U2 ScriptSystem +
   cascade wiring (mock-tested); U3 vendor Duktape + CMake + backend skeleton; U4 ECMAScript marshalling +
   Browser + handler dispatch + end-to-end + corpus smoke. Each: implement → adversarial review (build -j4 +
   ctest + golden-untouched) → ≤2 fixes.
3. **Verify:** golden gate + full ctest + corpus smoke + BACKLOG (Script/SAI core CLOSED toward M4;
   SCR-ASYNC/SCR-SAI-DYN/SCR-REFRESH logged).

Script: `docs/superpowers/plans/2026-06-16-script-sai-runtime-workflow.js` (ready; launch only after M2D lands —
no file overlap with M2D, but keep one big push in flight at a time).
