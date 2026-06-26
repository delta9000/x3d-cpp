# Prose-Grounded Behavioral Conformance Audit — Workflow Design

**Date:** 2026-06-18  **Status:** approved design (ready to author the workflow script).

**Goal:** Leverage the new `X3DInterfaceRegistry` to run a multi-agent workflow that, for every *behavioral* X3D interface, grounds the runtime's behavior against the ISO normative prose, adversarially verifies each divergence, and emits a **prioritized, verified conformance gap register**. The workflow's job ends at the register; fixes flow through the SDD pipeline as separate plans.

**Calibration decisions (locked):**
- Gap targeted: **prose-grounded semantic conformance** ("do we *behave* per spec", which corpus roundtrip does NOT test).
- End state: **audit → verified prioritized gap register** (fixes via SDD separately).
- Coverage: **behavioral interfaces** only (~14).
- Staging: **calibration run of 3 interfaces first**, tune, then fan out the remaining 11.

---

## 1. Preflight (inline, before launching the Workflow)

The audit's correctness depends entirely on both RAG DBs being current. Verified state as of 2026-06-18:

- **`spec_rag` — FRESH** (immutable ISO prose; verified: `time.md §8.4.1 TimeSensor` returns at 0.748). No action.
- **`code_rag` — RE-INGESTED + VERIFIED.** Ran `mise run code-ingest` after the `runtime-registry-pivot` merge (10,716 chunks). Confirmed: deleted `ColorInterpolatorSystem.hpp` is gone; `runtime/events/InterpolatorSystem.hpp` + `MultiInterpolatorSystem` present (runtime tier); `generated_cpp_bindings/X3DInterfaceRegistry` present (generated tier, 0.839). Re-run this whenever the runtime changes before re-launching.

**Re-verify gate before any launch:** `mise run code-rag query "ColorInterpolator HSV"` must NOT return `ColorInterpolatorSystem.hpp`; `--source generated` query for `nodeImplements` must return `X3DInterfaceRegistry`.

## 2. Work-list (scouted inline, passed as `args`)

The hybrid pattern: derive the audit units inline from `generated_cpp_bindings/X3DInterfaceRegistry.cpp` (the registry is the source of the exhaustive interface→node map), pair each with its ISO clause(s) and runtime System file(s), and pass the list to the Workflow as `args`. The workflow does not rediscover it.

The ~14 behavioral audit units:

| # | Interface (InterfaceId) | Concrete nodes (from registry) | ISO clause(s) | Runtime behavior (code_rag entrypoint) |
|---|---|---|---|---|
| 1 | X3DTimeDependentNode | TimeSensor, AudioClip, MovieTexture, sound sources | §8 Time, §16 Sound | X3DTimeDependentSystem, TimeSensorSystem |
| 2 | X3DInterpolatorNode | 8 core + Spline*/SquadOrientation* | §10 Interpolation | InterpolatorSystem / MultiInterpolatorSystem, Interpolation.hpp |
| 3 | X3DTouchSensorNode (pointing) | TouchSensor | §20 Pointing-device sensor | PointingSensorSystem |
| 4 | X3DDragSensorNode | PlaneSensor, CylinderSensor, SphereSensor | §20 | PointingSensorSystem |
| 5 | X3DEnvironmentalSensorNode | ProximitySensor, VisibilitySensor, TransformSensor, GeoProximitySensor | §22 Environmental sensor | ViewDependentSystem |
| 6 | X3DKeyDeviceSensorNode | KeySensor, StringSensor | §21 Key-device sensor | (KeyState seam) |
| 7 | X3DNetworkSensorNode | LoadSensor | §9 Networking | (asset/seam) |
| 8 | X3DBindableNode | Viewpoint(s), Background(s), NavigationInfo, Fog | §23/§24/§25 | BindingSystem, BindingStack |
| 9 | X3DFollowerNode (Chaser/Damper) | PositionChaser, ColorDamper, … | §40 Followers | (follower system) |
| 10 | X3DGroupingNode active-child | Switch, LOD, Collision | §10.3 / §30 | ViewDependentSystem (LOD), SceneExtractor (Switch) |
| 11 | X3DScriptNode | Script | §29 Scripting | ScriptSystem, ScriptEngine, SAI |
| 12 | X3DTriggerNode | BooleanTrigger, IntegerTrigger, TimeTrigger | §31 Event utilities | (event-utility handlers) |
| 13 | X3DSequencerNode | BooleanSequencer, IntegerSequencer | §31 | (event-utility handlers) |
| 14 | Event-utility filters | BooleanFilter, BooleanToggle | §31 | (event-utility handlers) |

Coverage is **logged, never silently capped** — if a unit is dropped for scale, the register names it. The exact node lists and clause anchors are filled at scout time from the registry + the prose mirror; the table above is the seed.

**Calibration-3** (run first): units **1 (X3DTimeDependentNode)**, **4 (X3DDragSensorNode)**, **11 (X3DScriptNode)** — chosen for behavioral richness across three different shapes (clock state machine, geometric drag projection, scripting/SAI).

## 3. Phases

### Phase 1 — AUDIT (pipeline stage 1; one agent per interface)
Each agent receives a work unit and:
1. Pulls normative prose per mandated behavior: `spec_rag query "<behavior>"` (component clause + abstract-type semantics). Distinguishes **normative** prose from informative/examples.
2. Pulls runtime behavior: `code_rag query` + reads the actual System file(s) at `file:line`.
3. For each spec-mandated behavior, emits a finding classified `implemented-ok | partial | missing | wrong`, with spec quote+citation, observed behavior + `code_ref`, and initial severity.

### Phase 2 — VERIFY (pipeline stage 2; adversarial, per finding)
As soon as an interface's audit returns, each non-`implemented-ok` finding is checked by **3 perspective-diverse skeptics** (run concurrently):
- **L1 handled-elsewhere:** is it truly absent, or implemented in another System / a base class / the event cascade?
- **L2 prose-fidelity:** does the cited clause *normatively* mandate this? Is the quote accurate and not an example/informative note?
- **L3 repro:** would it manifest in a real scene? Sketch the minimal X3D that exposes it.

Each returns a verdict; **default to refuted when uncertain.** A finding survives only if **≥2 of 3 confirm** it is a real gap. This is the precision gate that keeps the register trustworthy for downstream SDD.

*Pipeline, not barrier:* interface B audits while interface A's findings verify.

### Phase 3 — SYNTHESIZE (barrier — needs all confirmed findings)
1. **Dedupe** across interfaces: a base-interface gap surfacing under multiple nodes collapses into one entry listing all affected nodes.
2. **Prioritize:** `severity × corpus_prevalence × (1/fix_effort)`, where `corpus_prevalence` = per-node frequency from the existing 17.7k-file sweep.
3. **Emit** the register in two forms: `docs/superpowers/specs/2026-06-18-conformance-gap-register.md` (human) + a `.json` sidecar (SDD-consumable).

### Phase 4 — COMPLETENESS CRITIC (final agent)
Identifies what the audit itself missed: behavioral interfaces/clauses not covered; any concrete node implementing a behavioral interface with **no System at all** (a whole-node behavioral hole); any mandated behavior never checked. Appends a "coverage gaps / next wave" section.

## 4. Schemas

```
AUDIT_SCHEMA = {
  interface: string,
  nodes_covered: string[],
  findings: [{
    id: string,                  // "<iface>-<n>"
    title: string,
    spec_clause: string,         // e.g. "§8.4.1"
    spec_quote: string,          // verbatim normative text
    mandated_behavior: string,
    observed_behavior: string,
    code_ref: string,            // "file:line"
    divergence_class: "implemented-ok" | "partial" | "missing" | "wrong",
    severity: "critical" | "major" | "minor",
    affected_nodes: string[]
  }]
}

VERDICT_SCHEMA = {
  finding_id: string,
  lens: "handled-elsewhere" | "prose-fidelity" | "repro",
  is_real: boolean,             // default false when uncertain
  confidence: "high" | "medium" | "low",
  reason: string,               // refutation or confirmation, with evidence
  corrected_severity: "critical" | "major" | "minor" | null,
  corrected_class: "partial" | "missing" | "wrong" | null
}

REGISTER_ENTRY = {
  id, interface, affected_nodes[],
  spec_clause, spec_quote, mandated, observed,
  class, severity, corpus_prevalence, est_fix_effort: "S"|"M"|"L",
  suggested_test, dependencies[]
}

COVERAGE_SCHEMA = {
  interfaces_covered: string[],
  interfaces_skipped: string[],
  nodes_with_no_system: string[],
  unchecked_behaviors: string[]
}
```

## 5. Control flow (workflow script skeleton)

```js
// args = the ~14 (or calibration-3) work units
const confirmed = (await pipeline(args,
  unit  => agent(auditPrompt(unit), {schema: AUDIT_SCHEMA, phase: 'Audit',
                                     label: `audit:${unit.interface}`}),
  audit => parallel((audit?.findings || [])
    .filter(f => f.divergence_class !== 'implemented-ok')
    .map(f => () =>
      parallel(['handled-elsewhere','prose-fidelity','repro'].map(lens => () =>
        agent(verifyPrompt(f, lens), {schema: VERDICT_SCHEMA, phase: 'Verify',
                                      label: `verify:${f.id}:${lens}`})))
      .then(vs => ({ ...f,
        real: vs.filter(Boolean).filter(v => v.is_real).length >= 2,
        verdicts: vs.filter(Boolean) }))))
)).flat().filter(Boolean).filter(f => f.real);

// barrier: dedupe + prioritize across ALL confirmed findings
const deduped   = dedupeByClauseAndBehavior(confirmed);          // plain code
const prioritized = prioritize(deduped, corpusFreq);              // severity×prevalence×1/effort
const coverage  = await agent(completenessCriticPrompt(args, confirmed),
                              {schema: COVERAGE_SCHEMA, phase: 'Critic'});
return { register: prioritized, coverage };
```

`corpusFreq` is read from the existing corpus-sweep frequency data (or recomputed once inline and passed in `args`). `dedupeByClauseAndBehavior` is plain JS (no agent) keyed on `(spec_clause, normalized mandated_behavior)`.

## 6. Scale, cost, de-risking

- Full run ≈ **14 audit + (~5 findings/iface × 3 skeptics ≈ 210 verify) + 1 critic ≈ ~225 agents.** Within Workflow caps (1000 total, 16 concurrent). **Token-intensive — explicit opt-in required.**
- **Calibration-3 first** (≈ 3 audit + ~45 verify + 1 critic ≈ ~50 agents): validates finding quality, verifier precision, and register format on a small budget before the full fan-out. Tune `auditPrompt`/`verifyPrompt` from the calibration register, then run the remaining 11.

## 7. Outputs

- `docs/superpowers/specs/2026-06-18-conformance-gap-register.md` — human-readable, prioritized.
- `docs/superpowers/specs/2026-06-18-conformance-gap-register.json` — SDD-consumable sidecar.
- The register's "coverage gaps / next wave" section seeds the *next* audit wave (e.g. extraction-semantic, or the static/value interfaces).

## 8. Downstream (out of scope for this workflow)

Each high-priority register cluster becomes a `superpowers:writing-plans` → `subagent-driven-development` fix campaign, golden/ctest/corpus-gated — exactly the pipeline used for `runtime-registry-pivot`. The audit workflow ends at the verified register; it does not implement fixes.

## 9. Quality patterns employed

Adversarial verify (precision); perspective-diverse verifiers (catch failure modes redundancy can't); completeness critic (what the audit missed); corpus-prevalence prioritization (impact-first); no-silent-caps logging (dropped units named). These mirror the patterns proven on the pivot.
