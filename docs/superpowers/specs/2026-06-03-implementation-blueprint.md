# X3D Runtime — Sequenced Implementation Blueprint

**Date:** 2026-06-03
**Status:** Build sequence (synthesized from the five sibling specs)
**Branch:** `modernize-x3d-spec`

## What this is

One ordered, end-to-end build plan that fuses the four parsing specs
(parsing-frontend, classicvrml-reader, vrml97-reader) and the behavior-runtime
spec (behavior-runtime + event-system-design + week-plan) into a single
dependency-ordered sequence. It resolves the contradictions between the specs
(layout/namespace/entry-point names), fixes the wiring once, and gives each
workstream a concrete file list, CMake/test hooks, corpus fixtures, and a copy-
paste verification command.

### Two load-bearing invariants (carried verbatim into every workstream)

- **Node-as-truth.** The generated OOP node model + reflection `FieldTable`
  remain the single source of truth. Readers build `runtime::X3DDocument` by
  going through `X3DNodeFactory::create` + `FieldInfo` thunks + `FieldValueIO`;
  systems are runtime-side projections *over* nodes. No entity store, no ECS,
  no per-node bookkeeping baked into generated headers.
- **Systems-seam.** Every new file is **header-only**, under `runtime/parse/`
  or `runtime/events/`, in `namespace x3d::codec` (readers) or
  `x3d::runtime` (behaviors), driven purely by reflection + the generated
  accessors.

### Codegen-change audit (flagged, per the task requirement)

The cascade foundation (sub-project #1) already shipped the **only** codegen
changes the runtime needs: the `inputOnly` `setOn<Name>Handler` + populated
`set` thunks for `inputOnly`/`outputOnly` (see `event-system-design.md`
"Codegen changes", commit history `af292d2`/`a9328ef`). Confirm those thunks
exist before starting WS-F.

**Every workstream in this blueprint is runtime-only and additive — NO codegen
or golden regeneration is expected.** Two watch-items that would break that
rule if hit, and must be treated as STOP-and-flag conditions, not silently
worked around:

1. **WS-A/B/C (readers):** if a reader needs a node/field that has no
   `FieldInfo` (e.g. a `ProtoField` access-type or `fieldTypeFromString`
   mapping that the reflection layer cannot express), that is a model-layer
   addition in `runtime/X3DProto.hpp` / `X3DReflection.hpp`, **not** a template
   change — but flag it in the PR. The `Vrml97Dialect` table test (asserting
   every mapped X3D target resolves to a real `FieldInfo`/factory type) is the
   tripwire that catches generated-binding drift.
2. **WS-F/G (behaviors):** the spec asserts the `inputOnly`/`outputOnly` thunks
   already exist. If any interpolator or TimeSensor output field turns out to
   lack a populated `set` thunk, STOP — that is a codegen change and must be
   raised, not patched in the runtime.

### Layout reconciliation (the one real conflict between specs — resolved here)

The specs disagree on where readers live and what they are named:

- `parsing-frontend-spec.md` → `runtime/parse/`, `namespace x3d::codec`,
  unified `X3DReader` interface, `XmlReaderAdapter` wrapping the existing
  `XmlReader`, umbrella `X3DParse.hpp`, shared `NodeBuilder` + `VrmlTokenizer`.
- `classicvrml-reader-spec.md` / `vrml97-reader-spec.md` → `runtime/codecs/`,
  standalone `VrmlReader`/`VrmlLexer`, optional `X3DReader` base.

**Decision for this blueprint: follow the parsing-frontend layout** (`runtime/parse/`,
unified `X3DReader`, shared `NodeBuilder` + `VrmlTokenizer`, `X3DParse.hpp`
front door). It is the superset and the only design that lets ClassicVRML and
VRML97 share one tokenizer/parser instead of two. The two single-encoding specs
are treated as the **detailed grammar references** for their respective readers
(PROTO/IS/IMPORT productions, the VRML97 dialect table, corpus fixtures) layered
into that unified structure. Where a single-encoding spec says "add to
`X3DCodecs.hpp`", read it as "add to `X3DParse.hpp`".

---

## Ordered workstreams (dependency sequence)

```
WS-A  Parse front-end        (interface + sniffing + NodeBuilder + tokenizer + XmlReaderAdapter)
  │
  ├── WS-B  ClassicVRML reader (.x3dv)   ── depends A
  │     │
  │     └── WS-C  VRML97 reader (.wrl)    ── depends A,B  (shares tokenizer/parser via dialect)
  │
  └── WS-D  JSON reader (.json)          ── depends A     (independent of B/C; can run parallel)
        │
        └── WS-E  parseDocument/parseFile front door + sniff dispatch ── depends A,B,C,D

WS-F  Scene bridge + ROUTE validation   ── depends A (a readable Scene); independent of B–E
  │
  └── WS-G  System seam + interpolators  ── depends F
        │
        └── WS-H  TimeSensor / time-dependent lifecycle ── depends F,G
```

Readers (A→E) and behaviors (F→H) are two largely independent rails joined only
by `runtime::X3DDocument`/`Scene`. WS-A must land first (it gives both rails a
real loaded document). WS-D can be built in parallel with WS-B/WS-C. The
canonical end-to-end demo (WS-H verify) wants a reader, so run the reader rail
to at least WS-B before the behavior demo.

---

## WS-A — Parse front-end (interface, sniffing, shared core)

**Goal:** the unified `X3DReader` interface, encoding sniffing, the shared
node-agnostic `NodeBuilder`, the shared `VrmlTokenizer`, and an
`XmlReaderAdapter` that wraps the existing `XmlReader` — so there is one front
door and one reusable build core before any new grammar is written.

**NEW files**
- `runtime/parse/X3DReader.hpp` — abstract base (`encoding()`,
  `readDocument(const std::string&)`); `namespace x3d::codec`.
- `runtime/parse/Encoding.hpp` — `enum class Encoding`; `sniffByExtension`,
  `sniffByContent`, `sniff` (content wins when confident; gzip-magic detect).
- `runtime/parse/NodeBuilder.hpp` — `beginNode`, `applyField`, `attachChild`,
  `defineDef`/`resolveUse`, `collectFieldValue(VrmlTokenizer&, X3DFieldType)`.
  Lifts `XmlReader`'s `applyAttribute`/`attachChild`/`findField` logic into free
  functions keyed on parsed name/value strings.
- `runtime/parse/VrmlTokenizer.hpp` — shared lexer (punctuation `{ } [ ]`,
  quoted strings with `\"`/`\\`, identifiers/numbers, `#`-to-EOL comments with
  first-line header exemption, comma==whitespace, line/col tracking). Reuse the
  escape logic in `FieldValueIO::parseMFString`/`tokenize`.
- `runtime/parse/XmlReaderAdapter.hpp` — wraps `codec::XmlReader`; implements
  `X3DReader` (`encoding()==XML`). XmlReader itself is NOT moved or modified.
- `runtime/parse/X3DParse.hpp` — umbrella; for now includes the above + the
  XmlReaderAdapter (readers added as later workstreams land).
- `runtime/parse/tests/reader_test.cpp` — start with the **sniffing** increment
  (extension + content classify four encodings; content overrides wrong
  extension; gzip magic detected) and an XmlReaderAdapter round-trip vs. raw
  `XmlReader`.

**TOUCHED files**
- `CMakeLists.txt` — add a `runtime/parse/` glob block mirroring the existing
  `runtime/codecs/` block: include dir, install dir, per-header
  `compile_parse_*` syntax tests; add `runtime/parse` to the `x3d_cpp`
  `target_include_directories`; pull `X3DParse.hpp` into the
  `x3d_cpp_all_headers` aggregate TU.
- `runtime/codecs/X3DCodecs.hpp` — unchanged (front-end is the new front door).

**CMake/test wiring**
- New `add_executable(x3d_parse_reader runtime/parse/tests/reader_test.cpp)` +
  `add_test(NAME x3d_parse_reader ...)`, built and RUN (like
  `x3d_codec_roundtrip`).

**Corpus fixtures** — none yet (sniffing uses string literals + tiny
writer-produced documents).

**Verify**
```
cmake -S . -B build && cmake --build build && ctest --test-dir build -R 'parse_reader|compile_parse' --output-on-failure
```

---

## WS-B — ClassicVRML reader (`.x3dv`)

**Goal:** `ClassicVrmlReader : X3DReader` parsing ISO/IEC 19776-2 into
`X3DDocument`; the inverse of `VrmlWriter`. Uses WS-A's `VrmlTokenizer` +
`NodeBuilder`. Grammar reference: `classicvrml-reader-spec.md`.

**NEW files**
- `runtime/parse/ClassicVrmlReader.hpp` — header line (`#X3D Vx.y utf8` →
  `doc.version`), header statements (`PROFILE`/`COMPONENT`/`UNIT`/`META` →
  `Head`), scene body (`DEF`/`USE`/`Type{…}` via `NodeBuilder`; field-vs-node
  disambiguation by `findField` on the current node; `ROUTE A.f TO B.g`),
  `IMPORT`/`EXPORT`, and **structural** PROTO/EXTERNPROTO/instance/`IS` capture
  into `runtime/X3DProto.hpp` (carried, NOT expanded). Script `{…}` skipped via
  balanced-brace.
- `runtime/parse/tests/vrml_reader_test.cpp` — lexer tokens; minimal
  `Shape{geometry Box{size …}}`; DEF/USE pointer identity; ROUTE resolves;
  COMPONENT/UNIT/META; PROTO/EXTERNPROTO/IS captured; MF mixed comma+whitespace;
  `MFString` quote preservation; `VrmlWriter→ClassicVrmlReader→VrmlWriter`
  byte-stable on fixtures 1–2 below.

**TOUCHED files**
- `runtime/parse/X3DParse.hpp` — add `#include "ClassicVrmlReader.hpp"`.
- `runtime/X3DProto.hpp` — ONLY if `fieldTypeFromString` / a `ProtoField`
  access-type setter is missing (model-layer helper, not codegen — flag per
  audit item 1).
- `CMakeLists.txt` — new `add_executable(x3d_parse_vrml_reader …)` + test
  (parse glob auto-covers the new header's syntax test).

**Corpus fixtures** (copy minimal ones into `runtime/parse/tests/data/`, or
point at the corpus path under `<x3d-render-workspace>`)
1. `…/x3d/content/examples/Basic/Geospatial/MBARI/ship.x3dv` — 6-line minimal
   `Shape{Box}` smoke fixture.
2. `…/x3d/content/examples/HelloWorld.x3dv` — canonical happy path (Viewpoint/
   Transform/Sphere/Text/Shape + header statements).
3. `…/x3d/tools/Vrml97ToX3dNist/JavaLBExamples/AddDynamicRoutes.x3dv` — Script +
   ROUTE (Script carried, not executed).
4. `…/x3d/content/examples/Basic/NURBS/originals/animated_patch.x3dv` — larger
   MF arrays (exercises the token re-join path).

**Verify**
```
cmake -S . -B build && cmake --build build && ctest --test-dir build -R 'parse_vrml_reader|compile_parse_ClassicVrmlReader' --output-on-failure
```

---

## WS-C — VRML97 reader (`.wrl`)

**Goal:** `Vrml97Reader : X3DReader` reusing WS-A's tokenizer + WS-B's statement
loop, adding the thin VRML97 dialect layer (header acceptance + the small
node/field/access-keyword rename table + `TRUE`/`FALSE` normalization). Grammar
+ table reference: `vrml97-reader-spec.md`. VRML97 in, X3D out (no separate
object model).

**NEW files**
- `runtime/parse/Vrml97Dialect.hpp` — `mapNodeName`, `mapFieldName(nodeType,…)`
  (deny-by-omission-safe: unknown tokens pass through), `accessTypeFromVrml97`
  (`eventIn→inputOnly`, `eventOut→outputOnly`, `field→initializeOnly`,
  `exposedField→inputOutput`), `LOD.level→children`, `Switch.choice→children`,
  `TRUE/FALSE→true/false`. Authored by hand (tiny + stable).
- `runtime/parse/Vrml97Reader.hpp` — header `#VRML V2.0 utf8` (default
  `profile=Immersive`, `doc.version` VRML97-equivalent); `#VRML V1.0` →
  `std::runtime_error`; `#X3D V3/V4` → switch the dialect OFF (reads `.x3dv`
  too); applies the dialect during name lookup; otherwise delegates to the
  shared statement loop. Collected `warnings()` + optional `setStrict(true)`.
- `runtime/parse/tests/vrml97_reader_test.cpp` — dialect-table test (every
  mapped X3D target resolves to a real `FieldInfo`/factory type — the
  spec-drift tripwire); per-fixture parse + sampled field value + empty
  `warnings()` for clean fixtures; `LOD.level`/`Switch.choice` alias; VRML 1.0
  rejected.

**TOUCHED files**
- `runtime/parse/X3DParse.hpp` — add `Vrml97Dialect.hpp` + `Vrml97Reader.hpp`.
- `CMakeLists.txt` — new `add_executable(x3d_parse_vrml97_reader …)` + test.
  Gate the two gzip fixtures (#6, #7 below) behind an `X3D_PARSE_GZIP` option
  (default OFF) so no zlib dependency is forced; sniffer only *detects* gzip.

**Corpus fixtures** (absolute paths verified present)
1. `$X3D_CORPUS_DIR/x3d-code/www.web3d.org/x3d/tools/Vrml97ToX3dNist/test/NewShape.wrl` — minimal Shape/Geometry/Appearance (strict).
2. `…/Vrml97ToX3dNist/test/TranslationTestScene.wrl` — broad node coverage; diff vs. its `…ToX3dTranslated.x3d` sibling where present.
3. `…/Vrml97ToX3dNist/test/Rollers.wrl` — DEF/USE + ROUTE + interpolators.
4. `…/legacy/showcase/Examples/Other/Followers/Chasers.wrl` — EXTERNPROTO + comments.
5. `…/legacy/showcase/Examples/Other/Followers/Dampers.wrl` — comment/whitespace robustness.
6. `<x3d-render-workspace>/tests/floops/floops/s36/s36.wrl` — gzip (~1.47MB inflated); gated.
7. `…/tests/floops/floops/s47/s47.wrl` — second gzip; gated.

**Verify**
```
cmake -S . -B build && cmake --build build && ctest --test-dir build -R 'parse_vrml97_reader|compile_parse_Vrml97' --output-on-failure
```

---

## WS-D — JSON reader (`.json`)

**Goal:** `JsonReader : X3DReader` over a bundled `JsonLite`, walking the
X3D-JSON shape produced by `JsonWriter` (its inverse). Independent of WS-B/WS-C;
may be built in parallel after WS-A.

**NEW files**
- `runtime/parse/JsonLite.hpp` — tiny recursive-descent JSON parser
  (object/array/string/number/bool/null), no deps (sibling to `XmlLite`).
- `runtime/parse/JsonReader.hpp` — `{"X3D":{"@profile","@version","head",
  "Scene":{"-children":[…]}}}`; `TypeName` key → `beginNode`; `@field` →
  value fields (`@DEF`/`@USE` special); `-childField` arrays → `attachChild`
  with the de-prefixed key as containerField; `head.meta/component/unit` →
  `Head`; `ROUTE` object → `routes`. JSON scalars/arrays converted to the
  wire string `FieldValueIO::parseValue` expects (numbers space-joined, bools
  `true/false`, MFString → quoted list) — bypassing `collectFieldValue`.
- `runtime/parse/tests/json_reader_test.cpp` — `JsonWriter→JsonReader`
  structural-equality round-trip; DEF/USE pointer identity; head metadata;
  corpus smoke (`HelloWorld.json`).

**TOUCHED files**
- `runtime/parse/X3DParse.hpp` — add `JsonLite.hpp` + `JsonReader.hpp`.
- `CMakeLists.txt` — new `add_executable(x3d_parse_json_reader …)` + test.

**Corpus fixtures**
1. `<x3d-render-workspace>/…/HelloWorld.json` (or generate via `JsonWriter`
   from the in-repo sample document if no corpus `.json` is convenient).

**Verify**
```
cmake -S . -B build && cmake --build build && ctest --test-dir build -R 'parse_json_reader|compile_parse_Json' --output-on-failure
```

---

## WS-E — Front door: `parseDocument` / `parseFile` + sniff dispatch

**Goal:** the top-level convenience that hands bytes (or a path) to the right
reader without naming the format. Depends on A–D (needs all four readers
constructible).

**NEW files** — none (functions land in the existing `X3DParse.hpp`).

**TOUCHED files**
- `runtime/parse/X3DParse.hpp` — add
  `runtime::X3DDocument parseDocument(const std::string& text, Encoding hint=Unknown)`
  (resolves `Unknown` via `sniffByContent`, constructs the matching reader,
  delegates; throws on unknown-after-sniff) and
  `runtime::X3DDocument parseFile(const std::string& path)` (reads bytes,
  `sniff(path,bytes)`, dispatches).
- `runtime/parse/tests/reader_test.cpp` — add the **unified entry** increment:
  `parseDocument(text)` with no hint produces a document equal to the
  explicitly-dispatched reader, for each of the four encodings; and the
  round-trip-parity matrix (write with each Writer, read back with the matching
  reader via `parseDocument`, assert structural equality).

**CMake/test wiring** — none new (reuses `x3d_parse_reader`).

**Verify**
```
cmake -S . -B build && cmake --build build && ctest --test-dir build -R 'parse_reader' --output-on-failure
```

---

## WS-F — Scene bridge + ROUTE validation

**Goal:** make a parsed `Scene` tickable — resolve DEF-named ROUTEs onto the
pointer-based `EventGraph`, validating each at registration. Reference:
`behavior-runtime-spec.md` §1 + `week-plan.md` Day 1. Depends on WS-A only (any
readable `Scene`); independent of WS-B–E.

**NEW files**
- `runtime/events/X3DSceneBridge.hpp` — `struct RouteError`, `struct
  BridgeResult`, `BridgeResult buildRoutes(Scene&, X3DExecutionContext&)`. Calls
  `scene.resolveRoutes()`; per-route validation in order: (1) unresolved
  endpoint → skip silently (not rejected); (2) unknown field → reject;
  (3) direction (source `OutputOnly`/`InputOutput`; sink `InputOnly`/
  `InputOutput`) → reject; (4) exact `X3DFieldType` tag match → reject on
  mismatch. Endpoint = `weak_ptr::lock().get()` + `x3dName`; non-owning. Never
  throws on a bad route.
- `runtime/events/tests/scene_bridge_test.cpp` — load a scene with ROUTEs,
  bridge it, assert `routesAdded` + each `rejected` reason category; unresolved
  endpoint skipped (not rejected).

**TOUCHED files**
- `runtime/events/X3DExecutionContext.hpp` — add thin
  `BridgeResult buildFrom(Scene&)` wrapper calling `buildRoutes`.
- `CMakeLists.txt` — new `add_executable(x3d_event_scene_bridge …)` + test
  (events glob auto-covers the new header's syntax test).

**Corpus fixtures** — a hand-built `Scene` (or `parseDocument` of a tiny
TimeSensor+Transform `.x3dv` if WS-B has landed); no external corpus required.

**Verify**
```
cmake -S . -B build && cmake --build build && ctest --test-dir build -R 'event_scene_bridge|compile_event_X3DSceneBridge' --output-on-failure
```

---

## WS-G — System seam + interpolator family

**Goal:** generalize `ActiveNode` into a `System` over node collections; factor
shared interpolation math; implement the eight interpolators as event-driven
systems. Reference: `behavior-runtime-spec.md` §2–3 + `week-plan.md` Days 2–3.
Depends on WS-F.

**NEW files**
- `runtime/events/X3DSystem.hpp` — `class System { attach(node,ctx);
  update(now,ctx) }`.
- `runtime/events/Interpolation.hpp` — `KeySpan locateKeySpan(const MFFloat&,
  float)`; `rgbToHsv`/`hsvToRgb`; quaternion SLERP helpers.
- `runtime/events/ScalarInterpolatorSystem.hpp`,
  `ColorInterpolatorSystem.hpp` (HSV), `OrientationInterpolatorSystem.hpp`
  (SLERP), `PositionInterpolator2DSystem.hpp`,
  `CoordinateInterpolatorSystem.hpp`, `CoordinateInterpolator2DSystem.hpp`,
  `NormalInterpolatorSystem.hpp` (slerp-on-sphere + renormalize),
  `PositionInterpolatorSystem.hpp` (port of `PositionInterpolatorBehavior`).
- `runtime/events/tests/interpolator_test.cpp` (or one file per interpolator) —
  fraction at 0/0.5/1 + one off-key; assert emitted `value_changed` reaches a
  sink. One multi-interpolator scene test driven by one source.

**TOUCHED files**
- `runtime/events/X3DExecutionContext.hpp` — add
  `addSystem(std::shared_ptr<System>)`; in `tick()` call each system's
  `update(now,*this)` before draining the cascade. Keep `addActiveNode` as a
  deprecated thin adapter (`ActiveNode`→one-node `System`) so `cascade_test` /
  `animation_test` keep passing.
- `runtime/events/PositionInterpolatorBehavior.hpp` — keep as a compat shim
  (or delete after `animation_test` is repointed at
  `PositionInterpolatorSystem`); must not break the existing test.
- `CMakeLists.txt` — new `add_executable(x3d_event_interpolators …)` + test.

**Corpus fixtures** — none (synthetic key/keyValue arrays; math-only).

**Verify**
```
cmake -S . -B build && cmake --build build && ctest --test-dir build -R 'event_interpolators|event_cascade|event_animation|compile_event' --output-on-failure
```
(Note: `cascade_test`/`animation_test` must STILL PASS — behavior-preserving
port.)

---

## WS-H — TimeSensor / `X3DTimeDependentNode` lifecycle

**Goal:** the reusable time-dependent lifecycle base + `TimeSensorSystem`
(enabled/loop/start/stop/pause/resume; outputs `isActive`/`isPaused`/
`elapsedTime`/`cycleTime`/`time`/`fraction_changed`). Reference:
`behavior-runtime-spec.md` §4 + `week-plan.md` Days 4–5. Depends on WS-F, WS-G.

**NEW files**
- `runtime/events/X3DTimeDependentSystem.hpp` — base `System` holding the
  state machine keyed by node pointer (never on the node).
- `runtime/events/TimeSensorSystem.hpp` — derives from it; reads via generated
  getters (`getEnabled`/`getLoop`/`getStartTime`/…); all writes via
  `ctx.postEvent(node,"<x3dName>",value)`; edge-triggered change events.
- `runtime/events/tests/timesensor_test.cpp` — start gating; single-shot
  completion + deactivation; looping wrap + `cycleTime` pulses; `stopTime` early
  stop; pause/resume elapsed-time; `enabled=false` emits nothing.
- `runtime/events/tests/scene_animation_test.cpp` — END-TO-END: `parseFile`/
  `parseDocument` a `.x3dv` with `TimeSensor→PositionInterpolator→Transform` +
  ROUTEs → `buildFrom(scene)` → `tick()` loop → assert Transform translation
  changes across ticks. (This is the WS-E + WS-B + WS-H join — the "real browser
  base" proof.)
- *(optional)* `examples/x3d-tick.cpp` — tiny driver program.

**TOUCHED files**
- `runtime/events/TimeSensorBehavior.hpp` — compat shim or delete after porting.
- `CMakeLists.txt` — `add_executable(x3d_event_timesensor …)` +
  `add_executable(x3d_event_scene_animation …)` + tests; optional
  `examples/` add_executable behind an `X3D_BUILD_EXAMPLES` option.

**Corpus fixtures** — a small hand-authored or corpus `.x3dv` with a
TimeSensor + PositionInterpolator + Transform + 2 ROUTEs (e.g. derived from
`HelloWorld.x3dv` + an interpolator, committed under
`runtime/events/tests/data/anim.x3dv`).

**Verify**
```
cmake -S . -B build && cmake --build build && ctest --test-dir build -R 'event_timesensor|event_scene_animation|compile_event' --output-on-failure
```

---

## Full-suite gate (run after every workstream)

```
cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure
```

The golden-drift gate in CI (`.github/workflows/ci.yml`) must stay green with NO
golden regeneration across all eight workstreams; a golden diff is a signal that
the codegen-change tripwire (audit §1/§2) was hit and the work strayed off the
runtime-only path.
