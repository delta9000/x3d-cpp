// Script + SAI runtime — language-agnostic Script execution on the event cascade.
// Design spec: docs/superpowers/specs/2026-06-16-script-sai-runtime-design.md
// LAUNCH ONLY AFTER the M2D interaction-layer workflow has landed (keep one big push in flight at a time).
// Run with: Workflow({scriptPath: "<this file>"}).
//
// Track A (U1-U2: seam + SaiContext + ScriptSystem, mock-tested) is language-agnostic (scheme→backend registry).
// Track B (U3-U4: Duktape reference backend) — Duktape 2.7.0 is ALREADY VENDORED at runtime/script/vendor/duktape/
// (commit ba7a2a1, compiles clean), so U3 just CMake-wires it; no fetch step, fully autonomous. See spec §4.2/§5/§9.

export const meta = {
  name: 'script-sai-runtime',
  description: 'Language-agnostic Script execution: ScriptEngine seam + in-process SAI + ScriptSystem on the cascade (mock-tested), plus a Duktape reference ECMAScript backend; adversarial review + golden/ctest gate',
  phases: [
    { title: 'Spec-check', detail: 'ECMAScript binding, cascade lifecycle, directOutput/mustEvaluate, dynamic-field API + golden risk (parallel)' },
    { title: 'Implement', detail: 'U1 seam+SAI+mock → U2 ScriptSystem → U3 Duktape vendor+backend → U4 marshalling+Browser+e2e' },
    { title: 'Review', detail: 'adversarial per unit: build -j4 + ctest + golden-untouched' },
    { title: 'Fix', detail: 'address review issues, re-review (≤2)' },
    { title: 'Verify', detail: 'golden + ctest + corpus smoke + BACKLOG' },
  ],
}

const REPO = '<repo-root>'
const SPEC = 'docs/superpowers/specs/2026-06-16-script-sai-runtime-design.md'
const DYNFIELD_SPEC = 'docs/superpowers/specs/2026-06-05-dynamic-field-foundation-spec.md'

const COMMON = `Repo: ${REPO} (branch modernize-x3d-spec, the project trunk — commit directly to it).
Read the design spec first: ${SPEC} (authoritative; ISO §29 citations). The Script author-field mechanism is the
dynamic-field foundation: ${DYNFIELD_SPEC}.
BUILD RULE: always \`cmake --build build -j4\` (NEVER unbounded -j — OOM); tests via ctest.
GOLDEN RULE: PREFER codegen-free (Script node + emit methods are generated; author fields use the dynamic-field
foundation; the runtime reads/writes via reflection). BUT a generator change is AUTHORIZED by the human if dispatching
a Script's dynamic inputOnly field to a handler genuinely requires it. If so: modify the GENERATOR/TEMPLATES (under
src/x3d_cpp_gen/ + templates/), NEVER hand-edit files under generated_cpp_bindings/; then REGENERATE via the project's
gen task (\`mise run gen\`), COMMIT the regenerated bindings, and update any committed golden hash/expectation in the
golden test so \`mise run golden\` passes against the new committed state. A golden CHANGE is acceptable when it is a
clean regeneration scoped to exactly the intended dynamic-field-handler addition; a golden change from hand-editing
generated files, or that touches unrelated nodes, is NOT acceptable. Keep the change minimal and data-driven.
Anchors: event runtime in runtime/events/ (X3DExecutionContext: tick(now) runs systems then cascade_.process();
addRoute; inputOnly handlers dispatch to std::function via setOn<Name>Handler; outputOnly via emit<Name>); Systems
implement update(double now, X3DExecutionContext& ctx); new code lives under runtime/script/; the C++ test harness uses
check(cond,msg), tests in runtime/<area>/tests/ wired in the ROOT CMakeLists.txt; permissive single-file deps are
vendored like runtime/parse/tinfl.h.
Follow TDD (red→green). Commit each unit with a clear message when green.`

const FORMULA_SCHEMA = {
  type: 'object', additionalProperties: false,
  properties: {
    findings: { type: 'string', description: 'the precise, codeable answer' },
    apiOrMapping: { type: 'string', description: 'concrete API signatures / type mappings / ordering an implementer codes from' },
    codegenRisk: { type: 'string', description: 'whether any part needs a generator/template change (the golden risk); empty if none' },
    convention_vs_normative: { type: 'string', description: 'flag where the answer is established X3D convention vs normative prose (esp. ISO 19777 if outside the mirror)' },
    citations: { type: 'array', items: { type: 'string' } },
  },
  required: ['findings', 'apiOrMapping', 'citations'],
}
const REVIEW_SCHEMA = {
  type: 'object', additionalProperties: false,
  properties: {
    approved: { type: 'boolean' },
    buildPassed: { type: 'boolean' },
    testsPassed: { type: 'boolean' },
    goldenUntouched: { type: 'boolean' },
    blockedExternally: { type: 'boolean', description: 'true if the unit could not complete for an environment reason (e.g. Duktape source unobtainable), NOT a quality failure' },
    issues: { type: 'array', items: { type: 'string' } },
  },
  required: ['approved', 'buildPassed', 'testsPassed', 'goldenUntouched', 'blockedExternally', 'issues'],
}

// ---------- Phase 1: spec-check (parallel, read-only) ----------
phase('Spec-check')
const CHECKS = [
  { key: 'ecma', q: 'ISO/IEC 19777-1 ECMAScript binding: how each X3D field type maps to a JS value/object (SFBool/SFInt32/SFFloat/SFDouble/SFTime/SFString; SFVec2f/3f/4f(+d) accessors; SFColor/SFColorRGBA; SFRotation; SFNode; and the MF* array-like wrappers), and the Browser global object API (currentTime, currentFrameRate, getName, getVersion, addRoute, deleteRoute, print). 19777 may be OUTSIDE the prose mirror — derive from any references found + the established, stable X3D ECMAScript binding conventions, and clearly flag convention vs normative.' },
  { key: 'lifecycle', q: 'ISO 19775-1 §29.2 + §4.4.8 event model: the EXACT ordering of Script lifecycle relative to the event cascade — initialize() before first event; prepareEvents() once per timestamp BEFORE any ROUTE processing; inputOnly delivery in timestamp order; eventsProcessed() after a batch of received events; output events carry the triggering event timestamp; shutdown() on url-change/removal. Give the precise per-tick sequence an implementer wires into X3DExecutionContext.tick().' },
  { key: 'gates', q: 'ISO 19775-1 §29.2.6 + §29.4.1: exact semantics of directOutput (when TRUE: may directly post events to nodes accessed via SFNode/MFNode + read their last field values; when FALSE: only via declared outputOnly fields) and mustEvaluate (FALSE: browser may delay input delivery until outputs needed; TRUE: deliver ASAP). State precisely what the runtime must enforce/allow for each.' },
  { key: 'dynfield', q: `Read ${DYNFIELD_SPEC} AND the runtime to determine the concrete API for a Script node's DYNAMIC author fields: how to enumerate them, read/write their values by name+type, and especially how an inputOnly author field's arrival is dispatched to a handler (is there per-field std::function handler registration usable from a System, or does it require a generator/template change?). This is the golden-risk question (spec §5) — answer definitively with file:line evidence.` },
]
const sr = await parallel(CHECKS.map(c => () =>
  agent(`${COMMON}\n\nTASK: Using the project spec RAG (\`cd ${REPO} && uv run python scripts/spec_rag.py query "<text>"\`, \`node <Name>\`, \`field <Node>.<field>\`), the prose mirror ($X3D_SPEC_PROSE/scripting.md, eventUtilities.md, concepts.md), and reading the runtime/specs where noted, answer precisely:\n\n${c.q}\n\nReturn a codeable answer (concrete API/mapping/ordering), flag any codegen risk, flag convention-vs-normative, and cite sources.`,
    { schema: FORMULA_SCHEMA, label: `spec:${c.key}`, phase: 'Spec-check', model: 'sonnet' })
))
const S = {}
CHECKS.forEach((c, i) => { S[c.key] = sr[i] || { findings: '(spec-check failed — re-derive)', apiOrMapping: '', citations: [] } })
const fmt = (x) => x ? `findings: ${x.findings}\napi/mapping: ${x.apiOrMapping}\ncodegenRisk: ${x.codegenRisk||'(none)'}\nconvention_vs_normative: ${x.convention_vs_normative||'(n/a)'}\ncitations: ${(x.citations||[]).join(', ')}` : '(none)'
log(`spec-check done. dynfield codegen risk: ${S.dynfield.codegenRisk || 'none reported'}`)

// ---------- Phase 2: sequential implement units ----------
const UNITS = [
  {
    id: 'U1-seam-sai',
    title: 'ScriptEngine seam + SaiContext + mock backend',
    model: undefined, effort: 'high',
    prompt:
`UNIT U1 — Create the language-agnostic foundation under runtime/script/ (Track A; NO JS engine here):
- ScriptEngine.hpp: abstract interface — load(node, source)->handle, initialize(handle), shutdown(handle),
  invoke(handle, eventName, value, timestamp), eventsProcessed(handle), prepareEvents(handle, now).
- SaiContext.hpp: the in-process SAI surface a backend calls back into — getField/setField(node, name) (via the
  reflection + dynamic-field-foundation API per the spec-check below), addRoute/deleteRoute, getName/getVersion,
  currentTime/currentFrameRate, print. setField on a node OTHER than the owning Script must be GATED by the
  Script's directOutput (per spec-check gates).
- tests/MockScriptEngine.hpp (test-only): a ScriptEngine that records calls and replays scripted field writes —
  so the runtime is testable with zero JS.
LANGUAGE-AGNOSTIC (do NOT pigeonhole to ECMAScript, spec §4.2): the seam carries NO JS types — invoke()/field
exchange pass the runtime's own FieldValue/reflection types, never JS objects (each backend privately marshals to
its idiom). Provide a scheme→backend REGISTRY abstraction (key = language id inferred from a url's scheme/extension:
ecmascript:/javascript:/.js → "ecmascript"; .class/.jar → "java"; custom lua:/python: allowed) — ScriptSystem (U2)
will resolve a Script's url preference-list to the first registered backend. Only ECMAScript gets registered (U3/U4);
the registry must not assume JS.
Dynamic-field API (spec-checked): ${fmt(S.dynfield)}
directOutput/mustEvaluate rules (spec-checked): ${fmt(S.gates)}
TDD: tests/sai_context_test.cpp (check() harness), target x3d_sai_context — assert field get/set round-trips,
addRoute/deleteRoute take effect in the cascade, browser-info getters, and the directOutput write-gate (write to a
second node rejected when directOutput=FALSE, allowed when TRUE). Register the target in the ROOT CMakeLists.txt.
Build -j4, run. Commit: "script: M4 ScriptEngine seam + in-process SaiContext + mock backend".`,
  },
  {
    id: 'U2-script-system',
    title: 'ScriptSystem (lifecycle + cascade wiring, mock-tested)',
    model: undefined, effort: 'high',
    prompt:
`UNIT U2 — Create runtime/script/ScriptSystem.hpp: a System (update(now,ctx)) wiring Script lifecycle + events into
the cascade (depends U1). Register it so tick() drives it. Behavior (spec-checked ordering below):
- Load: when load=TRUE and url present, decode the source and engine.load(); engine.initialize() BEFORE the first
  event. On set_url, engine.shutdown() the old then load+initialize the new.
- prepareEvents(): call once per timestamp, BEFORE ROUTE processing.
- inputOnly delivery: dispatch each received author inputOnly event to engine.invoke(eventName, value, timestamp)
  in timestamp order. outputOnly writes the script makes become cascade events carrying the triggering timestamp.
- eventsProcessed(): call after the batch of received events.
- Honor mustEvaluate (eager vs lazy input delivery) and directOutput (passed to SaiContext as the write-gate).
Lifecycle ordering (spec-checked): ${fmt(S.lifecycle)}
CODEGEN: if wiring inputOnly author-field dispatch requires a generator/template change (see the dynfield spec-check
codegenRisk: "${S.dynfield.codegenRisk || 'none reported'}"), you ARE AUTHORIZED to make it (the human approved a
generator change if needed). Do it the right way per the GOLDEN RULE above: edit the generator/templates (not the
generated files), \`mise run gen\`, commit the regenerated bindings, update the committed golden hash/expectation so
\`mise run golden\` passes. Keep it minimal + data-driven. If NO codegen is needed, use the existing dynamic-field
handler API and keep golden byte-identical (preferred).
TDD: tests/script_system_test.cpp with MockScriptEngine, target x3d_script_system — assert: initialize before first
event; prepareEvents before routes; inputOnly→invoke; outputOnly→cascade event at correct timestamp; eventsProcessed
after a batch; shutdown on set_url; directOutput gate; mustEvaluate eager-vs-lazy. Register target. Build -j4, run.
Commit: "script: M4 ScriptSystem — lifecycle + cascade wiring (mock-tested)".`,
  },
  {
    id: 'U3-duktape-vendor',
    title: 'Vendor Duktape + CMake + backend skeleton',
    model: 'sonnet', effort: undefined,
    prompt:
`UNIT U3 — Track B foundation: CMake-wire the (already vendored) Duktape engine + a backend skeleton.
Duktape 2.7.0 is ALREADY VENDORED at runtime/script/vendor/duktape/ (duktape.c/.h, duk_config.h, DUKTAPE_LICENSE.txt;
MIT; compiles clean \`gcc -c -std=c99\`). Do NOT fetch or hand-write an engine — use the vendored files.
1. CMake: compile runtime/script/vendor/duktape/duktape.c into the runtime lib/test as needed (guarded so the rest
   of the build is unaffected; it is C — compile as C, link into the C++ targets that need the backend).
2. runtime/script/EcmaScriptBackend.hpp/.cpp: a ScriptEngine implementation SKELETON on Duktape — create/destroy a
   duk context per Script, decode inline "ecmascript:"/"javascript:" urls, and invoke a named global function with no
   args (full field marshalling is U4). Register it in the U1 scheme→backend registry under "ecmascript".
   A minimal smoke test that a trivial inline script runs and prints.
TDD where feasible. Target x3d_ecmascript_backend (skeleton). Build -j4, run. Commit: "script: M4 CMake-wire Duktape + EcmaScriptBackend skeleton".`,
  },
  {
    id: 'U4-ecma-marshalling',
    title: 'ECMAScript marshalling + Browser + handler dispatch + e2e',
    model: undefined, effort: 'high',
    prompt:
`UNIT U4 — Complete the Duktape backend (depends U2 + U3). If U3 reported BLOCKED (Duktape not vendored), STOP and
report blockedExternally too — this unit cannot proceed without the engine.
- Field marshalling (ISO 19777-1 ECMAScript binding) both directions for the core types: SF/MF of
  Bool/Int32/Float/Double/Time/String; SFVec2f/3f/4f(+d); SFColor/SFColorRGBA; SFRotation; SFNode; MF* as array-like.
- The Browser global object backed by SaiContext: currentTime, currentFrameRate, getName, getVersion, addRoute,
  deleteRoute, print.
- Handler dispatch: route engine.invoke(eventName,...) to the matching JS function; expose initializeOnly/inputOutput
  field values to the script; collect outputOnly writes back through SaiContext.
ECMAScript binding (spec-checked): ${fmt(S.ecma)}
TDD: extend x3d_ecmascript_backend — a real inline script with initialize() printing, a set_<name> handler that reads
an input and writes an outputOnly that drives a ROUTE; marshalling round-trips per core type; Browser.addRoute +
currentTime. PLUS a corpus smoke: pick 2-3 Script-bearing .x3d under <x3d-render-workspace>/testdata, parse →
expand → initialize → tick, assert no crash + an expected output on one known scene. Build -j4, run.
Commit: "script: M4 ECMAScript field marshalling + Browser object + end-to-end".`,
  },
]

async function buildUnit(u) {
  const impl = await agent(`${COMMON}\n\n${u.prompt}`,
    { label: `impl:${u.id}`, phase: 'Implement', model: u.model, effort: u.effort })
  let review = await agent(
    `${COMMON}\n\nADVERSARIAL review of unit ${u.id} (${u.title}). Do NOT trust the implementer; read the diff, build -j4, run the relevant ctest target.\nImplementer report:\n${impl}\n\nVerify spec-compliance vs ${SPEC} + ISO §29 citations, correctness of the lifecycle/marshalling, and no regression of the event suite. GOLDEN: run \`mise run golden\` — it must PASS. Set goldenUntouched=true if golden is byte-identical OR a LEGITIMATE scoped regeneration (i.e. \`git show --stat\` shows a generator/template edit under src/x3d_cpp_gen|templates AND regenerated generated_cpp_bindings/ AND an updated golden hash, all together, scoped to exactly the dynamic-field-handler change for Script — the human authorized this). Set goldenUntouched=FALSE only if generated files were HAND-EDITED, the golden gate fails, or the codegen change touches unrelated nodes. blockedExternally is unused now (Duktape is pre-vendored). approved=true only if spec-correct AND build passes AND tests pass AND golden is legitimate.`,
    { schema: REVIEW_SCHEMA, label: `review:${u.id}`, phase: 'Review', model: 'sonnet' })
  let tries = 0
  while (!review.approved && !review.blockedExternally && tries < 2) {
    tries++
    log(`${u.id} issues (attempt ${tries}): ${(review.issues||[]).join(' | ')}`)
    await agent(`${COMMON}\n\nFix unit ${u.id}. Address ALL reviewer issues, keep TDD, rebuild -j4 + re-run tests, keep golden byte-identical, commit:\n${(review.issues||[]).map((s,i)=>`${i+1}. ${s}`).join('\n')}`,
      { label: `fix:${u.id}#${tries}`, phase: 'Fix', model: u.model, effort: u.effort })
    review = await agent(`${COMMON}\n\nRe-review unit ${u.id} after fixes: build -j4 + ctest + golden-untouched (git show --stat HEAD). approved=true only if all gates pass.`,
      { schema: REVIEW_SCHEMA, label: `re-review:${u.id}#${tries}`, phase: 'Review', model: 'sonnet' })
  }
  log(`${u.id}: ${review.approved ? 'APPROVED' : (review.blockedExternally ? 'BLOCKED (external — human vendor step)' : 'NOT APPROVED')} (build=${review.buildPassed} tests=${review.testsPassed} golden=${review.goldenUntouched})`)
  return review
}

phase('Implement')
const results = {}
let trackAOk = true
for (const u of UNITS) {
  // Track A (U1,U2) is the autonomous core. If a Track-B unit (U3) is externally blocked, still report; U4 will self-skip.
  const r = await buildUnit(u)
  results[u.id] = r
  if ((u.id === 'U1-seam-sai' || u.id === 'U2-script-system') && !r.approved) {
    trackAOk = false
    log(`Track A unit ${u.id} not approved — stopping (the language-agnostic core must land before the backend).`)
    break
  }
}

// ---------- Phase 3: verify + BACKLOG ----------
phase('Verify')
const VERIFY_SCHEMA = {
  type: 'object', additionalProperties: false,
  properties: {
    goldenByteIdentical: { type: 'boolean' },
    ctestPassed: { type: 'boolean' },
    ctestSummary: { type: 'string' },
    corpusSmoke: { type: 'string' },
    backlogUpdated: { type: 'boolean' },
    notes: { type: 'string' },
  },
  required: ['goldenByteIdentical', 'ctestPassed', 'ctestSummary', 'backlogUpdated', 'notes'],
}
const verify = await agent(
  `${COMMON}

FINAL VERIFICATION GATE for the Script/SAI runtime push.
Track A approved: ${trackAOk}. Unit outcomes: ${Object.entries(results).map(([k,v])=>`${k}=${v.approved?'ok':(v.blockedExternally?'blocked-external':'failed')}`).join(', ')}.
1. \`mise run golden\` — must PASS. If the bindings were regenerated for the AUTHORIZED dynamic-field-handler codegen change, that is fine: confirm it is a CLEAN regeneration (generator/template edit + regenerated generated_cpp_bindings/ + updated committed golden hash, all together) scoped to that change, and report the new golden sha256. Byte-identical (no codegen) is even better — say which happened. FAIL only if generated files were hand-edited, the gate fails, or unrelated nodes changed.
2. \`cmake --build build -j4 && ctest --test-dir build --output-on-failure\` — all pass, incl. x3d_sai_context, x3d_script_system, and (if Track B landed) x3d_ecmascript_backend. Report the count.
3. Corpus smoke: if the ECMAScript backend landed, parse+initialize+tick a couple of Script-bearing .x3d from <x3d-render-workspace>/testdata and report. If Track B is blocked-external, say so plainly.
4. BACKLOG (docs/superpowers/BACKLOG.md): add a "Script/SAI runtime (toward M4)" section — mark the language-agnostic core (seam + SaiContext + ScriptSystem) CLOSED with commit SHAs; mark the Duktape backend CLOSED or BLOCKED-EXTERNAL (human must vendor duktape) as applicable; log SCR-ASYNC, SCR-SAI-DYN, SCR-REFRESH deferrals with the spec §3 reasons. Commit the BACKLOG update.
Return the gate results.`,
  { schema: VERIFY_SCHEMA, label: 'verify:golden+ctest+smoke+backlog', phase: 'Verify', model: 'sonnet' })

return { specChecks: S, unitResults: results, trackAApproved: trackAOk, verification: verify }
