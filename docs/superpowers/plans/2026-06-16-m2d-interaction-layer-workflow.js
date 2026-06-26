// M2D Interaction Layer — drag sensors + collision-free navigation, on the input seam.
// Design spec: docs/superpowers/specs/2026-06-16-m2d-interaction-layer-design.md
// LAUNCH ONLY AFTER the m25-input-seam-touchsensor workflow has landed — this extends the
// same PointingSensorSystem.hpp / X3DExecutionContext, so do not run concurrently.
// Run with: Workflow({scriptPath: "<this file>"}).

export const meta = {
  name: 'm2d-interaction-layer',
  description: 'Drag sensors (Plane/Sphere/Cylinder) + collision-free NavigationSystem (EXAMINE/FLY/LOOKAT) on the input seam, per the committed design spec, with adversarial review + golden/ctest gate',
  phases: [
    { title: 'Spec-check', detail: 'exact drag + nav-mode math from ISO §20.4/§20.2.2/§23 (parallel, read-only)' },
    { title: 'Implement', detail: 'sequential TDD units: drag math → drag dispatch → KeyState → NavigationSystem' },
    { title: 'Review', detail: 'adversarial per unit: build -j4 + ctest + golden-untouched' },
    { title: 'Fix', detail: 'address review issues, re-review (≤2)' },
    { title: 'Verify', detail: 'golden byte-identical + full ctest + BACKLOG' },
  ],
}

const REPO = '<repo-root>'
const SPEC = 'docs/superpowers/specs/2026-06-16-m2d-interaction-layer-design.md'
const SEAM_SPEC = 'docs/superpowers/specs/2026-06-16-m25-input-seam-touchsensor-design.md'

const COMMON = `Repo: ${REPO} (branch modernize-x3d-spec, the project trunk — commit directly to it).
Read the design spec first: ${SPEC} (authoritative; ISO §20/§23 citations). The input-seam it
builds on is described in ${SEAM_SPEC} (PointerState, PointingSensorSystem, grab model — already landed).
BUILD RULE: always \`cmake --build build -j4\` (NEVER unbounded -j — the all-headers TU OOMs); tests via ctest.
GOLDEN RULE: this work is codegen-free (drag-sensor + Viewpoint emit/accessor methods already exist on the
generated nodes) — do NOT touch generated_cpp_bindings/, templates/, or the generator. Golden hash MUST stay byte-identical.
Anchors (verified): Systems implement \`void update(double now, X3DExecutionContext& ctx)\` and tick() runs
them then cascade_.process(); ctx.pick(ray) returns PickResult (now carrying path/normal/texCoord);
PointingSensorSystem.hpp + PointerState.hpp + X3DExecutionContext.hpp live under runtime/events/;
the grab/resolution model (lowest enabled sensor on the hit path) is already implemented for TouchSensor —
drag sensors reuse it (resolve + grab is identical; only the per-motion math differs); PickSystem::worldOf(node)
gives a node's world matrix (sensor frame); bound Viewpoint via ctx.boundViewpoint(); bound NavigationInfo via
ctx.boundNavigationInfo(); the C++ test harness uses check(cond,msg), tests in runtime/events/tests/ wired in the ROOT CMakeLists.txt.
Follow TDD (red→green). Commit each unit with a clear message when green.`

const FORMULA_SCHEMA = {
  type: 'object', additionalProperties: false,
  properties: {
    behavior: { type: 'string', description: 'precise behavior incl. activation, per-motion mapping, deactivation' },
    math: { type: 'string', description: 'exact geometric formula in the local sensor (or view) frame, concrete enough to code without re-reading the spec' },
    fields: { type: 'string', description: 'the node fields read/written + their roles (clamps, offset, axisRotation, diskAngle, speed, centerOfRotation, etc.)' },
    edgeCases: { type: 'array', items: { type: 'string' } },
    citations: { type: 'array', items: { type: 'string' } },
  },
  required: ['behavior', 'math', 'fields', 'citations'],
}
const REVIEW_SCHEMA = {
  type: 'object', additionalProperties: false,
  properties: {
    approved: { type: 'boolean' },
    buildPassed: { type: 'boolean' },
    testsPassed: { type: 'boolean' },
    goldenUntouched: { type: 'boolean' },
    issues: { type: 'array', items: { type: 'string' } },
  },
  required: ['approved', 'buildPassed', 'testsPassed', 'goldenUntouched', 'issues'],
}

// ---------- Phase 1: spec-check (parallel, read-only) ----------
phase('Spec-check')
const CHECKS = [
  { key: 'plane', q: 'PlaneSensor (§20.4.2) + drag-sensor common (§20.2.2): tracking-plane mapping (parallel to Z=0 of the axisRotation-applied local frame, coincident with initial hit), translation_changed = in-plane relative delta + offset clamped by minPosition/maxPosition (min>max ⇒ unclamped, min==max ⇒ locked/line-sensor), trackPoint_changed = unclamped plane point, autoOffset/offset_changed on deactivation' },
  { key: 'sphere', q: 'SphereSensor (§20.4.3): virtual sphere at local origin, radius = |initial hit − origin| fixed at activation, rotation_changed (SFRotation) = relative rotation from initial point + offset, trackPoint_changed on the sphere surface, autoOffset' },
  { key: 'cylinder', q: 'CylinderSensor (§20.4.1): initial acute angle between bearing and local Y vs diskAngle selects cylinder (radius=shortest dist hit→Y axis, rotation about Y) vs disk (infinite disk in Y=0 plane), rotation_changed clamped by minAngle/maxAngle (min>max ⇒ unclamped), axisRotation frame, offset/autoOffset' },
  { key: 'examine', q: 'EXAMINE navigation (§23): orbit/turntable about the Viewpoint centerOfRotation, how drag maps to viewpoint position+orientation rotation, dolly; and the exact Viewpoint pose fields (position SFVec3f, orientation SFRotation, centerOfRotation SFVec3f, fieldOfView)' },
  { key: 'fly', q: 'FLY navigation (§23): free flight translate along view direction + yaw/pitch from drag, scaled by NavigationInfo.speed, with a preserved up vector, terrain/gravity disabled; how pointer-drag and keyboard map to viewpoint pose mutation' },
  { key: 'lookat', q: 'LOOKAT navigation (§23): animate the Viewpoint to frame a picked object world bounding box over transitionTime using transitionType (LINEAR/TELEPORT), and transitionComplete output; the framing geometry (fit bbox in fieldOfView)' },
]
const specResults = await parallel(CHECKS.map(c => () =>
  agent(`${COMMON}

TASK: Using the project spec RAG (\`cd ${REPO} && uv run python scripts/spec_rag.py query "<text>"\`, \`spec_rag.py node <Name>\`) and the verbatim prose mirror ($X3D_SPEC_PROSE/pointingDeviceSensor.md and navigation.md), extract the EXACT, codeable specification for:

${c.q}

Return concrete formulas/behavior in the relevant local (sensor or view) coordinate frame, the fields involved, edge cases, and section citations. An implementer must be able to code it from your answer without re-reading the spec.`,
    { schema: FORMULA_SCHEMA, label: `spec:${c.key}`, phase: 'Spec-check', model: 'sonnet' })
))
const F = {}
CHECKS.forEach((c, i) => { F[c.key] = specResults[i] || { behavior: '(spec-check failed — re-derive from the spec)', math: '', fields: '', citations: [] } })
log(`spec-check done: ${CHECKS.map(c => c.key).join(', ')}`)

const fmt = (x) => x ? `behavior: ${x.behavior}\nmath: ${x.math}\nfields: ${x.fields}\nedgeCases: ${(x.edgeCases||[]).join('; ')}\ncitations: ${(x.citations||[]).join(', ')}` : '(none)'

// ---------- Phase 2: sequential implement units, each impl → review → fix ----------
const UNITS = [
  {
    id: 'U1-drag-math',
    title: 'Pure per-sensor drag-math headers + unit test',
    model: undefined, effort: 'high',
    prompt:
`UNIT U1 — Create three PURE drag-math headers (no node/System dependencies; just geometry) under runtime/events/drag/:
- PlaneDrag.hpp: given the local sensor frame, the activation hit point, the current bearing ray (in the sensor frame), offset, minPosition, maxPosition → {trackPoint (unclamped in-plane), translation (clamped per the spec)}.
- SphereDrag.hpp: virtual-sphere rotation → {trackPoint on sphere, rotation (SFRotation) relative-to-initial + offset}.
- CylinderDrag.hpp: diskAngle decision (cylinder vs disk) → {trackPoint, rotation about the axis}, clamped by minAngle/maxAngle.
Implement EXACTLY these spec-verified specs (verified this session):

PLANE:
${fmt(F.plane)}

SPHERE:
${fmt(F.sphere)}

CYLINDER:
${fmt(F.cylinder)}

Reuse the project's math types (SFVec3f/SFVec2f/SFRotation/Mat4 + helpers in runtime/math/). Each function takes/returns plain values, no scene coupling — so it is unit-testable in isolation.
TDD: create runtime/events/tests/drag_math_test.cpp (check() harness) with hand-computed expected geometry for each: PlaneSensor translation + min/max clamp + line-sensor (min==max locks a component); SphereSensor rotation about origin from two ray positions; CylinderSensor disk-vs-cylinder selection AT the diskAngle boundary + min/max clamp. Register target x3d_drag_math in the ROOT CMakeLists.txt. Build -j4, run. Commit: "events: M2D pure drag-sensor math (Plane/Sphere/Cylinder)".`,
  },
  {
    id: 'U2-drag-dispatch',
    title: 'Drag dispatch in PointingSensorSystem',
    model: undefined, effort: 'high',
    prompt:
`UNIT U2 — Extend runtime/events/PointingSensorSystem.hpp so that, on the EXISTING grab, a resolved DRAG sensor runs its drag math (depends on U1). The resolution + grab + isOver/isActive path is already implemented for TouchSensor — REUSE it unchanged; only add per-type behavior for PlaneSensor/SphereSensor/CylinderSensor:
- On activation (isActive TRUE) over a drag sensor: record the activation hit + the sensor frame (PickSystem::worldOf(sensor)).
- On each motion while grabbed: call the matching PlaneDrag/SphereDrag/CylinderDrag (U1) with the current bearing transformed into the sensor frame; emit trackPoint_changed + the sensor's <value>_changed (translation_changed / rotation_changed) via the generated emit methods.
- On deactivation: if autoOffset TRUE, set offset to the last <value>_changed and emit offset_changed.
Dispatch by resolved sensor nodeTypeName(); the TouchSensor path is unchanged. Keep the drag math OUT of this file (it lives in the U1 headers); this file is the stateful glue only.
Drag-sensor common semantics (verified): ${fmt(F.plane)}
TDD: extend runtime/events/tests/pointing_sensor_test.cpp (or add to x3d_pointing_sensor): a PlaneSensor sibling of a Box → button-down over the Box, drag → assert translation_changed + trackPoint_changed; deactivate with autoOffset → assert offset updated; confirm grab exclusivity still holds. Build x3d_pointing_sensor -j4, run. Commit: "events: M2D drag-sensor dispatch (Plane/Sphere/Cylinder) on the grab model".`,
  },
  {
    id: 'U3-keystate',
    title: 'KeyState seam extension',
    model: 'sonnet', effort: undefined,
    prompt:
`UNIT U3 — Add keyboard input to the seam (PDS-4; needed by FLY/EXAMINE). In runtime/events/ (extend PointerState.hpp or a new KeyState.hpp): a small held-keys holder (e.g. a set/bitset of key codes) with a revision counter. In X3DExecutionContext.hpp add \`void setKey(int code, bool down)\` (bumps revision) + a const accessor the NavigationSystem reads. Keep it minimal and consistent with how PointerState is owned/exposed. TDD: a small test asserting set/clear + revision bump. Build the affected event test -j4. Commit: "events: M2D KeyState seam extension (keyboard input)".`,
  },
  {
    id: 'U4-navigation',
    title: 'NavigationSystem (EXAMINE/FLY/LOOKAT/NONE)',
    model: undefined, effort: 'high',
    prompt:
`UNIT U4 — Create runtime/events/NavigationSystem.hpp: a System (update(now,ctx)) that reads the input seam (pointer-drag deltas + KeyState from U3) and the bound NavigationInfo (ctx.boundNavigationInfo(): type/speed/avatarSize/transitionType/transitionTime) and mutates the bound Viewpoint (ctx.boundViewpoint(): position/orientation, centerOfRotation) per mode. Wire it so tick() drives it (member-System pattern, like pick_). Cross-tick state: last pointer pos, drag anchor, active LOOKAT transition. Implement EXACTLY these spec-verified modes:

EXAMINE:
${fmt(F.examine)}

FLY:
${fmt(F.fly)}

LOOKAT:
${fmt(F.lookat)}

NONE = inert; ANY = behave as EXAMINE. Do NOT implement WALK / terrain-following / gravity / collision (deferred NAV-COLLISION — see spec §6). LOOKAT framing may use ctx.worldBounds(target) for the bbox.
TDD: create runtime/events/tests/navigation_test.cpp + register x3d_navigation in the ROOT CMakeLists.txt: EXAMINE drag orbits the bound Viewpoint about centerOfRotation (assert pose change); FLY key/drag translates+rotates scaled by speed; LOOKAT animates toward a target bbox over transitionTime and emits transitionComplete; NONE is inert; switching NavigationInfo.type switches behavior. Build x3d_navigation -j4, run. Commit: "events: M2D NavigationSystem — EXAMINE/FLY/LOOKAT/NONE (collision-free)".`,
  },
]

async function buildUnit(u) {
  const impl = await agent(`${COMMON}\n\n${u.prompt}`,
    { label: `impl:${u.id}`, phase: 'Implement', model: u.model, effort: u.effort })
  let review = await agent(
    `${COMMON}\n\nADVERSARIAL review of unit ${u.id} (${u.title}). Do NOT trust the implementer; read the actual diff, build -j4, run the relevant ctest target yourself, and confirm via \`git show --stat HEAD\` that NO generated_cpp_bindings/ or template/generator file changed.\nImplementer report:\n${impl}\n\nVerify spec-compliance against ${SPEC} + the ISO citations, correctness of the geometry/mode math, and that the existing TouchSensor/seam behavior did not regress. approved=true only if spec-correct AND build passes AND tests pass AND golden untouched.`,
    { schema: REVIEW_SCHEMA, label: `review:${u.id}`, phase: 'Review', model: 'sonnet' })
  let tries = 0
  while (!review.approved && tries < 2) {
    tries++
    log(`${u.id} issues (attempt ${tries}): ${(review.issues||[]).join(' | ')}`)
    await agent(`${COMMON}\n\nFix unit ${u.id}. Address ALL reviewer issues, keep TDD, rebuild -j4 + re-run tests, keep golden byte-identical, commit:\n${(review.issues||[]).map((s,i)=>`${i+1}. ${s}`).join('\n')}`,
      { label: `fix:${u.id}#${tries}`, phase: 'Fix', model: u.model, effort: u.effort })
    review = await agent(`${COMMON}\n\nRe-review unit ${u.id} after fixes: build -j4 + ctest + golden-untouched (git show --stat HEAD). approved=true only if all gates pass and the reported issues are resolved.`,
      { schema: REVIEW_SCHEMA, label: `re-review:${u.id}#${tries}`, phase: 'Review', model: 'sonnet' })
  }
  log(`${u.id}: ${review.approved ? 'APPROVED' : 'NOT APPROVED'} (build=${review.buildPassed} tests=${review.testsPassed} golden=${review.goldenUntouched})`)
  return review.approved
}

phase('Implement')
let ok = true
for (const u of UNITS) {
  if (!ok) { log(`Stopping chain before ${u.id} — a prior unit was not approved.`); break }
  ok = await buildUnit(u)
}

// ---------- Phase 3: verify + BACKLOG ----------
phase('Verify')
const VERIFY_SCHEMA = {
  type: 'object', additionalProperties: false,
  properties: {
    goldenByteIdentical: { type: 'boolean' },
    ctestPassed: { type: 'boolean' },
    ctestSummary: { type: 'string' },
    backlogUpdated: { type: 'boolean' },
    notes: { type: 'string' },
  },
  required: ['goldenByteIdentical', 'ctestPassed', 'ctestSummary', 'backlogUpdated', 'notes'],
}
const verify = await agent(
  `${COMMON}

FINAL VERIFICATION GATE for the M2D interaction layer (prior units approved: ${ok}).
1. \`mise run golden\` — confirm BYTE-IDENTICAL (codegen-free). Any drift = FAIL; name the changed file.
2. \`cmake --build build -j4 && ctest --test-dir build --output-on-failure\` — all pass, incl. x3d_drag_math, x3d_pointing_sensor, x3d_navigation. Report the count.
3. BACKLOG (docs/superpowers/BACKLOG.md): mark M2D-1 CLOSED (TouchSensor + the 3 drag sensors) and M2D-3 collision-free modes CLOSED (EXAMINE/FLY/LOOKAT/NONE), with commit SHAs; log NAV-COLLISION (WALK + collision gate) and NAV-EXTRA (EXPLORE / ANIMATE) as deferred with the spec §6 reasons. Commit the BACKLOG update.
Return the gate results.`,
  { schema: VERIFY_SCHEMA, label: 'verify:golden+ctest+backlog', phase: 'Verify', model: 'sonnet' })

return { specChecks: F, allUnitsApproved: ok, verification: verify }
