# File-authored Script un-tabling (SCR-SAI-DYN S1) + RTC-5/6 — design

Date: 2026-06-17
Branch: `modernize-x3d-spec`
Status: approved (brainstorm), pending implementation
Backlog rows closed by this work: **SCR-SAI-DYN** (S1 increment), **RTC-5**, **RTC-6**

## 1. Problem

The v1 SDK shipped (T-GATE: 94/94 ctest, golden byte-identical) with one named
**#1 remaining hole**: file-authored Script nodes are **inert**. A `<Script>` in a
`.x3d`/`.x3dv`/`.wrl`/`.json` file that carries `<field>` author declarations and an
inline `<![CDATA[…]]>` (or `url ["ecmascript:…"]`) body does not run — its handlers
never fire and routes to/from its author fields silently drop. The programmatic /
inline-API path works; only the **reader-captured** path is dead.

Two root causes (both verified in-tree):

1. **Codec capture is missing.** `XmlReader` never reads `Element.text` (CDATA *is*
   parsed by `XmlLite` into `Element.text:351-358`, just never consumed) and has no
   handling for `<field>` child elements. The VRML/JSON readers likewise do not
   capture inline interface declarations as author fields. `Script.hpp` already has
   the destination slots — `sourceCode` (SFString inputOutput), `field` (MFNode,
   `types={"field"}`), `url`, `directOutput`, `mustEvaluate` — but no codec populates
   them.
2. **No dynamic-field foundation (S1).** Author-declared fields are not in the static
   generated reflection table, so they are invisible to every consumer that resolves
   fields via `node.fields()`:
   - `SaiContext::findField` (`SaiContext.hpp:171`) — script get/set/route via the SAI.
   - `X3DSceneBridge detail::findField` (`X3DSceneBridge.hpp:57`) — **ROUTE endpoint
     resolution**: a `ROUTE` to/from an author field finds no field and is rejected.
   - `SaiContext.hpp:168` already carries the `NOTE (S1)` marker: "switch this to
     `node.effectiveFields()` … for now it walks the static `fields()` table".

Two **conformance** items the BACKLOG explicitly gates on this exact un-tabling are
also in scope (they "couple to Script/SAI un-tabling"):

- **RTC-5** — the event cascade enforces per-ROUTE dedup but not the spec's
  per-FIELD-per-timestamp cap (§4.4.8.3). Fan-in (two ROUTEs into one field) double-
  delivers; a node re-emitting on input can re-drive a loop the per-edge guard meant
  to break.
- **RTC-6** — `tick()` does one pass (systems → cascade drain → propagate); there is
  no §4.4.8.3 step-4 re-evaluation loop, so a sensor→route→sensor chain whose second
  sensor is ordered earlier lags one frame.

## 2. Goal

A real scene with a `<Script>` (author `<field>`s + CDATA/inline source) **parses →
routes wire → `initialize()` runs → author `inputOnly`/`inputOutput` events dispatch
to handlers → `outputOnly`/`inputOutput` writes drive the cascade**, in **all four
encodings**, with **golden byte-identical** preserved and the cascade conformant per
RTC-5/6. Proven by a corpus end-to-end test where a file-authored script actually
executes.

Non-goals (remain deferred under SCR-SAI-DYN's larger surface): dynamic scene
construction (`createX3DFromString`, runtime node/PROTO create/destroy), external /
networked SAI (ISO 19775-2), `autoRefresh` re-fetch (SCR-REFRESH), asynchronous
scripts (SCR-ASYNC). This is the **S1** increment only.

## 3. Architecture

### 3.1 The linchpin: S1 dynamic-field foundation

One hard dependency gates everything else. S1 defines **two interface seams**:

**Seam 1 — `AuthorFieldDecl` (the reader→runtime contract).** A neutral, encoding-
agnostic struct every reader emits:

```cpp
struct AuthorFieldDecl {
  std::string   x3dName;       // e.g. "fraction", "on"
  X3DFieldType  type;          // SFFloat, MFVec3f, …
  AccessType    access;        // InputOnly | OutputOnly | InitializeOnly | InputOutput
  std::any      initialValue;  // boxed default (empty for inputOnly/outputOnly)
};
```

Readers parse `<field name type accessType value>` / VRML `field SFFloat x 0.5` /
JSON field members into these. This decouples the four readers from the storage model.

**Seam 2 — `DynamicFieldStore` + `effectiveFields(const X3DNode&)`.** A **per-node
side-table keyed by node identity** (the established `TransformSystem.world_` /
`BoundsSystem` memo pattern), holding, per node:

- a vector of synthesized `FieldInfo` entries for the author fields, whose `get`/`set`
  thunks read/write …
- a boxed `std::any` value store (the live author-field values).

The synthesized `FieldInfo` obeys the existing reflection contract exactly: `get`
empty for `inputOnly`, `set` empty for read-only, `type`/`access`/`x3dName` set from
the decl. `effectiveFields(node)` returns `node.fields()` (static) concatenated with
the node's author `FieldInfo`s. Consumers switch from `node.fields()` to
`effectiveFields(node)`:

- `X3DSceneBridge detail::findField` — so author-field ROUTEs resolve and wire.
- `SaiContext::findField` — so script get/set/addRoute see author fields (closes the
  `NOTE (S1)`).

(Other `node.fields()` sites — extraction, bounds, range-validate, material/texture —
operate only on generated geometry/appearance nodes and never need author fields;
they stay on `fields()`. Listed here so the switch is deliberate, not blanket.)

### 3.2 Why a side-table, not a generator change

The entire v1 fan-out held golden **byte-identical** (codegen-free). The alternative —
regenerate `X3DNode` with a virtual `effectiveFields()` + an instance store — would
churn golden across every generated node for a concern that is **Script-local** in
practice. The side-table keeps golden byte-identical and matches existing precedent
(`TransformSystem.world_`, `BoundsSystem` memo, `PickSystem` path cache). **This is
the one deliberate architectural call in the design.** Trade-off accepted: identity-
keyed side-tables carry the usual lifetime caveat (entries are keyed by `const
X3DNode*`); author fields live as long as their Script node, which the document owns,
so lifetime is bounded by the document — no dangling risk in the load→tick→extract
model.

### 3.3 Source path

Readers write the inline body into `Script.sourceCode`. `ScriptSystem` currently
decodes inline source from `url` only (`decodeInlineSource(e.script->getUrl())`,
`ScriptSystem.hpp:276`). Extend it to **prefer `getSourceCode()` when non-empty**,
else fall back to the existing `url` `ecmascript:`/`javascript:`/`vrmlscript:` decode.
No new runtime surface; one decision point.

### 3.4 Per-encoding capture detail

| Encoding | Author field decls | Inline source | Round-trip writer |
|---|---|---|---|
| **XML** | `<field name accessType type value>` child elements of `<Script>` | `<![CDATA[…]]>` → `Element.text` → `sourceCode` (also accept `url` inline scheme) | re-emit `<field>` children + CDATA |
| **ClassicVRML / VRML97** | inline `field/eventIn/eventOut <type> <name> [value]` interface decls in the Script body | `url ["ecmascript:…"]` (already captured) → also mirror to `sourceCode` | re-emit interface decls + url body |
| **JSON** | `-field` / field member array on the Script object | source member / `@url` inline scheme → `sourceCode` | re-emit field decls + source |

Each reader maps `field/eventIn/eventOut` access keywords to `AccessType`
(`field→initializeOnly` or `inputOutput` per spec context; `eventIn→inputOnly`;
`eventOut→outputOnly`; X3D XML uses the explicit `accessType` attribute). Decls flow
into the `DynamicFieldStore` via `AuthorFieldDecl`. Round-trip writers re-emit so the
PRF-style write→reparse fidelity holds (the conformance moat the corpus run depends on).

### 3.5 Runtime wiring (consumes S1)

- `ScriptSystem`: dispatch author `inputOnly`/`inputOutput` events to handlers (look
  up the handler by author-field name); source from `sourceCode` (§3.3); ensure
  `prepareEvents`/`eventsProcessed` ordering unchanged.
- `EcmaScriptBackend`: marshal author fields — seed JS globals from each decl's
  `initialValue` at `initialize()`; after a handler runs, read back `outputOnly` /
  `inputOutput` author-field values from the JS scope into the store and emit them as
  cascade events. Reuse the existing `pushValue`/`toValue` marshallers (already cover
  all core SF/MF types).

### 3.6 RTC-5 / RTC-6 (cascade conformance, S1-independent)

- **RTC-5**: add a per-`(node,field,timestamp)` "already produced" guard at the
  node/output layer, distinct from the existing per-`RouteEdge` guard
  (`X3DEventCascade.hpp:47`). Fan-in delivers once; a field emits at most one event
  per timestamp. §4.4.8.3.
- **RTC-6**: in `X3DExecutionContext::tick` (`:101-110`), loop steps 2–3 (evaluate
  systems/sensors → drain routes) until a pass produces no new events, before post-
  processing. §4.4.8.3 step 4. Bounded by the per-field cap from RTC-5 (guarantees
  termination).

## 4. Fan-out plan

Foundation-first, then a parallel wave, then integration. Considered and rejected:
vertical per-encoding slices (race on shared `SaiContext`/`ScriptSystem`/store) and
fully-sequential (wastes genuine file-level independence).

**Phase 0 — S1 foundation (1 agent, sequential, on-branch, TDD).**
`AuthorFieldDecl`, `DynamicFieldStore`, `effectiveFields()`; switch
`X3DSceneBridge::findField` + `SaiContext::findField` to it. Pre-register all Phase-1
test targets in CMake (pointing at the test-file paths Phase-1 will create) so the
parallel wave never touches CMake — the single shared-write hazard is eliminated up
front. Unit tests: author field get/set through the store; an author-field ROUTE
wires and delivers. **Commits to the branch; Phase 1 worktrees branch from this
commit so they inherit S1.**

**Phase 1 — parallel wave (5 agents, `isolation: worktree`, each TDD-green in
isolation, each commits to its worktree branch and returns its commit SHA).** Files
are disjoint across agents:
- **A. XML capture** — `runtime/codecs/XmlReader.hpp` + XML writer + new round-trip test.
- **B. VRML capture** — `runtime/parse/ClassicVrmlReader.hpp`, `Vrml97Reader.hpp` + VRML writer + test.
- **C. JSON capture** — `runtime/parse/JsonReader.hpp` + JSON writer + test.
- **D. Runtime wiring** — `runtime/script/ScriptSystem.hpp`, `EcmaScriptBackend.hpp/.cpp` (source-from-`sourceCode`, author-event dispatch, author-field marshalling) + end-to-end "author field drives a ROUTE" test.
- **E. RTC-5 + RTC-6** — `runtime/events/X3DEventCascade.hpp`, `X3DExecutionContext.hpp` + regressions per the BACKLOG rows.

**Phase 2 — integration + gate (1 agent, on-branch, sequential).** Cherry-pick each
Phase-1 commit SHA onto the S1 base (shared object store makes worktree commits
reachable by SHA); resolve any residual conflict (CMake pre-staged in Phase 0 → none
expected); run `mise run build` (ctest) + `mise run golden`; add a corpus end-to-end
test where a file-authored Script actually executes (a Savage `.x3d` from the existing
smoke set + a `.x3dv`, currently inert); flip BACKLOG rows (SCR-SAI-DYN S1 increment,
RTC-5, RTC-6) to CLOSED with SHAs; update `docs/sdk/v1-capabilities.md` (Scripting row:
experimental seam → file-authored Scripts supported). Returns a final report.

## 5. Testing & invariants

- **TDD throughout** (matches the runtime's build style).
- **Golden byte-identical is a hard invariant** — codegen-free; `mise run golden` must
  pass unchanged at every phase and at the gate.
- **ctest stays green** — current 94/94 is the floor; new tests add to it.
- Reader agents add **round-trip** tests (the PRF-1..5 pattern: read → write → reparse
  → assert decls + source survive).
- D + Phase 2 add the **behavioral proof**: a file-authored script's handler runs and
  its output drives a ROUTE / observable field change.

## 6. Risks

- **Worktree integration (Phase 2 cherry-pick).** Mitigated by disjoint files + CMake
  pre-staged in Phase 0. If a merge conflict survives, Phase 2 reports back and the
  orchestrator (main loop) intervenes — the workflow returns control, it does not
  silently force.
- **Side-table lifetime** — bounded by document ownership (§3.2); no dangling in the
  load→tick→extract model. Dynamic node removal (out of scope) would revisit this
  under M2C-2.
- **Access-keyword → `AccessType` mapping** drift across encodings — pinned by the
  round-trip tests per encoding.

## 7. Deferred (logged, unchanged)

Dynamic scene construction / external SAI (SCR-SAI-DYN S2+), SCR-ASYNC, SCR-REFRESH —
all retain their existing BACKLOG rows and reasons. This work closes only the S1
increment of SCR-SAI-DYN plus RTC-5/6.
