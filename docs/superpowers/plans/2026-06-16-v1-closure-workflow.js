// v1-closure fan-out — textures end-to-end + Text via font-metrics seam + embedder SDK façade/docs/examples + v1 gate.
// Design: docs/superpowers/specs/2026-06-16-v1-closure-roadmap-design.md
// LAUNCH ONLY AFTER the Script/SAI workflow (wf_ee656af0-f04) lands — this edits runtime/extract/* and the PoC.
// (T-SCRIPT is a separate in-flight track; this closes T-TEX, T-TEXT, T-SDK, T-GATE.)
// Run with: Workflow({scriptPath: "<this file>"}).
//
// Fan-out shape: parallel spec-check (read-only) + parallel new-file authoring (header-only/pure, self-checked
// standalone with `g++ -fsyntax-only`, NOT via the shared build/ dir) → sequential integration on the shared
// extractor (cmake -j4 builds serialized) → SDK façade → v1 gate. Each integration unit is adversarially reviewed.

export const meta = {
  name: 'v1-closure',
  description: 'Close the remaining gaps to a usable v1 SDK: textures end-to-end, Text via a font-metrics seam, the embedder API façade + docs + examples, and a v1 readiness gate — fan-out with adversarial review',
  phases: [
    { title: 'Spec-check', detail: 'texture model, Text layout, seam shapes, API conventions, PoC bind points (parallel)' },
    { title: 'Author', detail: 'parallel disjoint new files: TextureResolver/FontMetrics seams, TextLayout + TextureTransform pure logic (self-checked standalone)' },
    { title: 'Integrate', detail: 'sequential on shared extractor: RenderItem fields → texture extract → Text extract → PoC bind' },
    { title: 'Review', detail: 'adversarial per integration unit: build -j4 + ctest + golden' },
    { title: 'Fix', detail: 'address issues, re-review (≤2)' },
    { title: 'SDK', detail: 'embedder API façade + docs + runnable examples' },
    { title: 'Gate', detail: 'golden + full ctest + corpus smoke + v1 capability matrix + BACKLOG' },
  ],
}

const REPO = '<repo-root>'
const SPEC = 'docs/superpowers/specs/2026-06-16-v1-closure-roadmap-design.md'

const COMMON = `Repo: ${REPO} (branch modernize-x3d-spec, the trunk — commit directly). Read the design spec first: ${SPEC}.
BUILD RULE: cmake build via \`cmake --build build -j4\` (NEVER unbounded -j — OOM); tests via ctest. For STANDALONE
self-checks of a single new header (Author phase), use \`g++ -std=c++20 -fsyntax-only -I generated_cpp_bindings -I runtime
-I runtime/extract -I runtime/scene -I runtime/events -I runtime/math <header or a tiny TU>\` — do NOT invoke cmake in the
Author phase (it would race the shared build/ dir with sibling agents).
GOLDEN RULE: prefer codegen-free (runtime/extraction only). If a generator/template change is genuinely needed, edit the
generator/templates (never hand-edit generated_cpp_bindings/), \`mise run gen\`, commit regenerated bindings + update the
golden hash so \`mise run golden\` passes; a clean scoped regeneration is acceptable, hand-edits/unrelated drift are not.
Anchors (verified): RenderItem.hpp (runtime/extract/) already has MeshData{positions,normals,texcoords,indices,Topology}
+ a TextureRef descriptor (Url/Inline/Movie, repeatS/T, channel, inlinePixels) — byte-resolution is intentionally outside
the SDK; SceneExtractor.hpp drives extraction (buildLocalMesh per geometry); MeshBuilder.hpp builds per-type meshes incl.
a Text branch placeholder; runtime/extract/AssetResolver.hpp is the existing asset seam; the PoC lives under the
X3D_CPP_BUILD_POC path (see mise 'poc' task) and currently white-fallbacks textures; §13 primitive texcoords already done;
the C++ test harness uses check(cond,msg), tests in runtime/<area>/tests/ wired in the ROOT CMakeLists.txt.
Follow TDD (red→green). Commit each unit with a clear message when green.`

const SPEC_SCHEMA = {
  type: 'object', additionalProperties: false,
  properties: {
    findings: { type: 'string' }, api: { type: 'string', description: 'concrete signatures/fields/formulas to implement' },
    citations: { type: 'array', items: { type: 'string' } },
  }, required: ['findings', 'api', 'citations'],
}
const AUTHORED_SCHEMA = {
  type: 'object', additionalProperties: false,
  properties: {
    file: { type: 'string' }, publicInterface: { type: 'string', description: 'the exact public types/signatures other units must call' },
    syntaxChecked: { type: 'boolean' }, commit: { type: 'string' },
  }, required: ['file', 'publicInterface', 'syntaxChecked'],
}
const REVIEW_SCHEMA = {
  type: 'object', additionalProperties: false,
  properties: {
    approved: { type: 'boolean' }, buildPassed: { type: 'boolean' }, testsPassed: { type: 'boolean' },
    goldenOk: { type: 'boolean', description: 'byte-identical OR a clean scoped regeneration' },
    issues: { type: 'array', items: { type: 'string' } },
  }, required: ['approved', 'buildPassed', 'testsPassed', 'goldenOk', 'issues'],
}

// ---------- Phase 1: spec-check (parallel, read-only) ----------
phase('Spec-check')
const CHECKS = [
  { key: 'tex', q: 'ISO §18 Texturing: how authored TextureCoordinate / TextureCoordinateGenerator are applied to geometry; TextureTransform (center, rotation, scale, translation) math on (s,t); repeatS/repeatT + TextureProperties boundary/filter modes. Concrete (s,t) transform formula + what to surface on a render item.' },
  { key: 'text', q: 'ISO §15 Text/FontStyle: exact layout — baseline spacing = spacing×size; justify BEGIN/END/FIRST/MIDDLE (major + minor axis); horizontal vs vertical; leftToRight/topToBottom; Text.length per-line stretch/compress; maxExtent compress-to-fit along the major axis; and how textBounds/lineBounds/origin are computed. Give the codeable layout algorithm.' },
  { key: 'seams', q: 'Read runtime/extract/AssetResolver.hpp + RenderItem.hpp TextureRef. Propose the minimal TextureResolver seam (url → {w,h,RGBA}) and FontMetrics seam (family,style,codepoint → advance + optional outline/atlas-uv) consistent with the existing asset-seam style + the IO-free core. Give the exact interface signatures + a default stub for each.' },
  { key: 'api', q: 'Survey the existing public surface (CMakeLists x3d_cpp::x3d_cpp / x3d_cpp::nodes, the runtime entry points: X3DParse/parseFile, X3DExecutionContext tick + input setters, SceneExtractor snapshot, the seams). Propose the curated x3d::sdk façade shape (one umbrella header + namespace) that re-exports the supported surface and the seams, mirroring existing naming/conventions. List what is stable vs experimental.' },
  { key: 'poc', q: 'Read the PoC renderer (X3D_CPP_BUILD_POC path) — identify exactly where it consumes RenderItems/materials/lights and where the texture white-fallback + any Text gap are, so the integration units know the bind points to wire a resolved texture and Text glyph quads.' },
]
const sr = await parallel(CHECKS.map(c => () =>
  agent(`${COMMON}\n\nTASK (read-only): using spec_rag (\`cd ${REPO} && uv run python scripts/spec_rag.py query "<text>"\`/\`node <N>\`), the prose mirror, and reading the noted files, answer precisely:\n\n${c.q}\n\nReturn codeable findings + concrete API/signatures/formulas + citations.`,
    { schema: SPEC_SCHEMA, label: `spec:${c.key}`, phase: 'Spec-check', model: 'sonnet' })))
const C = {}; CHECKS.forEach((c, i) => { C[c.key] = sr[i] || { findings: '(failed)', api: '', citations: [] } })
const sfmt = (x) => x ? `findings: ${x.findings}\napi: ${x.api}\ncitations: ${(x.citations||[]).join(', ')}` : '(none)'
log('spec-check done: ' + CHECKS.map(c => c.key).join(', '))

// ---------- Phase 2: author disjoint new files (parallel, standalone syntax-check, NO cmake) ----------
phase('Author')
const AUTHORS = [
  { key: 'texresolver', f: 'runtime/extract/TextureResolver.hpp', what: `the TextureResolver seam (url → decoded {width,height,RGBA bytes}) + a default stub resolver (returns a tiny known checker/solid so tests + PoC work IO-free). Consistent with runtime/extract/AssetResolver.hpp. Interface per spec-check 'seams': ${sfmt(C.seams)}` },
  { key: 'fontmetrics', f: 'runtime/extract/FontMetrics.hpp', what: `the FontMetrics seam ((family,style,codepoint) → advance width + optional glyph outline/atlas-uv) + a default MONOSPACED stub (fixed advance) so layout works without a real font. Per spec-check 'seams': ${sfmt(C.seams)}` },
  { key: 'textlayout', f: 'runtime/extract/TextLayout.hpp', what: `a PURE Text layout engine: input (strings, FontStyle params, length[], maxExtent, FontMetrics) → per-glyph positioned quads (local coords) + textBounds/lineBounds/origin. No scene/IO coupling. Implement the algorithm from spec-check 'text': ${sfmt(C.text)}. ALSO author its unit test runtime/extract/tests/text_layout_test.cpp (with the monospaced stub) BUT do NOT cmake-build it here — only \`g++ -fsyntax-only\` the header + test; the Integrate phase wires the CMake target.` },
  { key: 'texxform', f: 'runtime/extract/TextureTransform2D.hpp', what: `pure (s,t) TextureTransform math (center/rotation/scale/translation) applied to a texcoord, per spec-check 'tex': ${sfmt(C.tex)}. Header-only pure functions.` },
]
const authored = await parallel(AUTHORS.map(a => () =>
  agent(`${COMMON}\n\nAUTHOR (parallel, header-only/pure — do NOT run cmake; self-check ONLY with \`g++ -std=c++20 -fsyntax-only\` against a tiny TU that includes your header): create ${a.f} — ${a.what}\n\nKeep it a clean, minimal, independently-testable interface. Return the file path, the exact public interface (types + signatures) the integration units will call, whether it syntax-checks, and commit it ("extract: v1 ${a.key} (author)").`,
    { schema: AUTHORED_SCHEMA, label: `author:${a.key}`, phase: 'Author', model: 'sonnet' })))
const A = {}; AUTHORS.forEach((a, i) => { A[a.key] = authored[i] || { file: a.f, publicInterface: '(author failed — re-derive)', syntaxChecked: false } })
const afmt = (x) => x ? `${x.file} :: ${x.publicInterface}` : '(none)'
log('authored: ' + AUTHORS.map(a => `${a.key}${A[a.key].syntaxChecked ? '' : '(!)'}`).join(', '))

// ---------- Phase 3: sequential integration on the shared extractor ----------
const IFACES = `Authored interfaces to call:\n- ${afmt(A.texresolver)}\n- ${afmt(A.fontmetrics)}\n- ${afmt(A.textlayout)}\n- ${afmt(A.texxform)}`
const UNITS = [
  { id: 'U-renderitem', title: 'RenderItem fields for resolved texture + Text glyphs', model: 'sonnet', effort: undefined,
    prompt: `UNIT — Extend runtime/extract/RenderItem.hpp with the fields BOTH feature tracks need, in one place to avoid later churn: (a) a resolved-texture handle/slot on TextureRef (or a parallel field) so an extractor can attach the TextureResolver result; (b) any field a Text render-item needs to carry glyph quads / glyph refs (or confirm the existing MeshData quads + TextureRef suffice and document that). Keep it minimal + documented; do not break existing consumers. ${IFACES}\nBuild -j4 (this is the first cmake build of the run), run the existing extract tests to confirm no regression. Commit: "extract: v1 RenderItem fields for resolved textures + Text glyphs".` },
  { id: 'U-tex', title: 'Texture extraction + TextureTransform + resolver threading', model: undefined, effort: 'high',
    prompt: `UNIT T-TEX integration — in SceneExtractor/MeshBuilder: extract AUTHORED TextureCoordinate/TextureCoordinateGenerator onto RenderItem.texcoords (fall back to the existing default/§13 when absent); apply TextureTransform via the authored TextureTransform2D; surface repeatS/T + TextureProperties modes on TextureRef; and thread an (optional) TextureResolver so a consumer gets resolved pixels (default stub when none supplied). Texture spec: ${sfmt(C.tex)}. ${IFACES}\nTDD: runtime/extract/tests — authored-texcoord extraction, TextureTransform application, resolver round-trip with the stub. Build -j4, run. Commit: "extract: v1 textures end-to-end (authored texcoord + TextureTransform + resolver seam)".` },
  { id: 'U-text', title: 'Text extraction via the layout engine', model: undefined, effort: 'high',
    prompt: `UNIT T-TEXT integration — wire the authored TextLayout into the MeshBuilder/SceneExtractor Text branch: produce glyph render-items (positioned quads + glyph UV/outline refs via FontMetrics) and set the Text outputs textBounds/lineBounds/origin on the node. Use the default monospaced FontMetrics stub when the embedder supplies none. Text spec: ${sfmt(C.text)}. ${IFACES}\nTDD: register runtime/extract/tests/text_layout_test.cpp (target x3d_text_layout) AND add a Text-extraction test (a Text node → expected glyph count/positions + textBounds with the stub). Build -j4, run. Commit: "extract: v1 Text rendered via font-metrics seam + layout engine".` },
  { id: 'U-poc', title: 'PoC binds resolved textures + Text', model: undefined, effort: 'high',
    prompt: `UNIT — wire the PoC renderer (X3D_CPP_BUILD_POC) to (a) resolve + bind one ImageTexture/PixelTexture via the TextureResolver seam (replace the white fallback) and (b) render Text glyph quads from the extractor. PoC bind points: ${sfmt(C.poc)}. Build the PoC (\`mise run poc\` or the cmake POC flag) -j4; confirm it builds + renders a known textured + Text scene (headless screenshot or item-count assertion is fine). Commit: "poc: v1 bind resolved textures + render Text".` },
]
async function buildUnit(u) {
  const impl = await agent(`${COMMON}\n\n${u.prompt}`, { label: `impl:${u.id}`, phase: 'Integrate', model: u.model, effort: u.effort })
  let review = await agent(`${COMMON}\n\nADVERSARIAL review of ${u.id} (${u.title}). Don't trust the implementer; read the diff, build -j4, run the relevant ctest, and run \`mise run golden\` (goldenOk = byte-identical OR a clean scoped regeneration). Verify spec-compliance vs ${SPEC} + citations and no regression. Implementer report:\n${impl}\napproved only if spec-correct AND build AND tests AND goldenOk.`,
    { schema: REVIEW_SCHEMA, label: `review:${u.id}`, phase: 'Review', model: 'sonnet' })
  let t = 0
  while (!review.approved && t < 2) { t++
    log(`${u.id} issues (try ${t}): ${(review.issues||[]).join(' | ')}`)
    await agent(`${COMMON}\n\nFix ${u.id}: address ALL issues, keep TDD, rebuild -j4 + re-test, keep golden ok, commit:\n${(review.issues||[]).map((s,i)=>`${i+1}. ${s}`).join('\n')}`, { label: `fix:${u.id}#${t}`, phase: 'Fix', model: u.model, effort: u.effort })
    review = await agent(`${COMMON}\n\nRe-review ${u.id}: build -j4 + ctest + golden. approved only if all gates pass.`, { schema: REVIEW_SCHEMA, label: `re-review:${u.id}#${t}`, phase: 'Review', model: 'sonnet' })
  }
  log(`${u.id}: ${review.approved ? 'APPROVED' : 'NOT APPROVED'} (build=${review.buildPassed} tests=${review.testsPassed} golden=${review.goldenOk})`)
  return review.approved
}
phase('Integrate')
let ok = true
for (const u of UNITS) { if (!ok) { log(`stopping before ${u.id} — prior unit unapproved`); break } ok = await buildUnit(u) }

// ---------- Phase 4: SDK façade + docs + examples ----------
let sdkOk = false
if (ok) {
  phase('SDK')
  sdkOk = await buildUnit({ id: 'U-sdk', title: 'embedder API façade + docs + examples', model: undefined, effort: 'high',
    prompt: `UNIT T-SDK — create the curated embedder SDK surface (depends on the landed features + the Script/SAI track):
1. A public façade: an umbrella header (e.g. include/x3d/x3d.hpp or runtime/sdk/X3DSdk.hpp) + a x3d::sdk namespace re-exporting the SUPPORTED surface — load (parse + conformance findings), the runtime (tick + input setters + script registration), extraction (snapshot/render-items), and the seams (AssetResolver/TextureResolver/FontMetrics/ScriptEngine). Hide internals; mark stable vs experimental. API shape per spec-check 'api': ${sfmt(C.api)}.
2. docs/sdk/: a quick-start usage guide (the tick model, the seams, the v1 capability matrix + the post-v1 list from ${SPEC}) + doc-comments on the façade.
3. examples/: 2-3 runnable programs — (1) headless load→validate→convert; (2) extract→feed-a-renderer (reuse the PoC extraction path); (3) attach a Script/behavior + drive ticks. Wire them as build targets (guarded) and confirm they compile + run.
Build -j4, run the examples. Commit: "sdk: v1 embedder façade + docs + examples".` })
}

// ---------- Phase 5: v1 readiness gate ----------
phase('Gate')
const GATE_SCHEMA = { type: 'object', additionalProperties: false, properties: {
  goldenOk: { type: 'boolean' }, ctestPassed: { type: 'boolean' }, ctestSummary: { type: 'string' },
  corpusSmoke: { type: 'string' }, capabilityMatrixWritten: { type: 'boolean' }, backlogUpdated: { type: 'boolean' },
  v1Ready: { type: 'boolean' }, notes: { type: 'string' },
}, required: ['goldenOk', 'ctestPassed', 'ctestSummary', 'capabilityMatrixWritten', 'backlogUpdated', 'v1Ready', 'notes'] }
const gate = await agent(`${COMMON}\n\nV1 READINESS GATE (integration approved: ${ok}; SDK approved: ${sdkOk}).
1. \`mise run golden\` — must pass (byte-identical or clean scoped regen); report which + the sha if regenerated.
2. \`cmake --build build -j4 && ctest --test-dir build --output-on-failure\` — all pass incl. x3d_text_layout + the new texture/Text tests; report the count.
3. Corpus smoke: parse + extract a handful of real scenes (incl. ones with textures + Text) from <x3d-render-workspace>/testdata; report no-crash + that textures/Text now produce render items.
4. Write docs/sdk/v1-capabilities.md — the capability matrix (what works) + the explicit post-v1 list (collision, pick sensors, full SAI, NURBS, Sound, network resolution, advanced components) with reasons, derived from ${SPEC} + the BACKLOG.
5. BACKLOG sweep: mark v1 rows CLOSED (T-TEX, T-TEXT, T-SDK) with commit SHAs; ensure each post-v1 exclusion has a row + reason. Record the v1 state (golden sha, ctest count, corpus conformance) in the matrix doc.
Set v1Ready=true only if golden ok + ctest green + the matrix/BACKLOG are committed. Commit docs. Return the gate.`,
  { schema: GATE_SCHEMA, label: 'gate:v1-readiness', phase: 'Gate', model: 'sonnet' })

return { specChecks: C, authored: A, integrationApproved: ok, sdkApproved: sdkOk, gate }
