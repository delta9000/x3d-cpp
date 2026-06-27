---
title: Conformance Campaign
summary: How behavioral conformance audits are run, how findings.yaml is managed, and how to close a finding.
tags: [guide, conformance, audit, findings]
updated: 2026-06-20
related:
  - ../subsystems/conformance-infra.md
  - gate-system.md
---

# Conformance Campaign

The conformance campaign is the process by which the SDK proves it **behaves** per
ISO 19775-1 prose, not just that it parses without crashing. This is distinct from the
structural (L2 validator) and corpus-sweep axes — see
[Conformance Infrastructure](../subsystems/conformance-infra.md) for the full picture.
This guide covers: the cadence, the audit workflow, and the generated conformance view
that is the live gap register.

---

## What the campaign covers

The campaign targets **behavioral interfaces**: nodes whose runtime behavior is governed
by normative ISO 19775-1 clauses (timing, event cascade, sensor actuation, binding
stacks, interpolation, sequencing, triggering, following). Static/data nodes are out of
scope — they are covered by corpus-sweep + extraction audits.

The tracked behavioral abstract interfaces (from `scripts/conformance_view.py`):

- `X3DTimeDependentNode` (TimeSensor, AudioClip, MovieTexture)
- `X3DInterpolatorNode` + `EaseInEaseOut` (fraction modifiers)
- `X3DSensorNode` / `X3DPointingDeviceSensorNode` / `X3DDragSensorNode`
- `X3DKeyDeviceSensorNode`
- `X3DNetworkSensorNode`
- `X3DEnvironmentalSensorNode`
- `X3DScriptNode`
- `X3DBindableNode` (Viewpoint, NavigationInfo, Background, Fog, …)
- `X3DFollowerNode` (Chaser/Damper families)
- `X3DSequencerNode`, `X3DTriggerNode`
- `BooleanFilter`, `BooleanToggle` (§30 event utilities — no abstract interface but behavioral)

The **extraction-semantic axis** (tessellation, Material PBR, texture-coordinate
transforms, Text layout) is also tracked in `docs/conformance/findings.yaml` in a
separate section.

---

## The generated conformance view

The generated view lives under `docs/conformance/` and is the authoritative live gap
register. Never edit it directly.

| File | Role | Edit? |
|---|---|---|
| `docs/conformance/findings.yaml` | **Behavioral judgments — the source of truth** | Yes, the only file you edit |
| `docs/conformance/INDEX.md` | Bug picture + component × dimension matrix (generated) | No |
| `docs/conformance/components/*.md` | Per-component drill-down: node rows + findings (generated) | No |
| `docs/conformance/model.json` | Machine/agent/RAG-queryable merged model (generated) | No |
| `docs/conformance/profiles.yaml` | ISO profile → component-level reference table | Review only |

### The facts/judgments split

The view generator (`scripts/conformance_view.py`) joins two categories:

- **Facts** (node exists, extracts, has a behavioral System wired, component/level/profile)
  are auto-derived from `generated_cpp_bindings/`, `X3DInterfaceRegistry.cpp`, and
  `runtime/` System headers. They cannot rot — regenerating re-reads the code.
- **Judgments** (does it behave per ISO prose? severity? status?) live exclusively in
  `docs/conformance/findings.yaml`. Human knowledge; requires human editing.

A behavioral node with no System wired reads `✗ inert`. Wired with an open finding reads
`◑ partial`. Wired and clean reads `✓`. A finding that references a node or interface the
code no longer has is flagged as stale and fails the gate — catches renaming accidents.

### The three columns in the component matrix

The `INDEX.md` matrix shows three conformance dimensions per component:

- **Extract** — `extractable/total geometry nodes` (e.g. `7/7`, `0/8`, `—` for non-geometry)
- **Behaves** — rollup of behavioral nodes: `3✓ 1◑ 2✗` (conformant/partial/inert)
- **Open gaps** — severity summary: `1 crit, 2 maj` for that component's open findings

---

## Running the view

```bash
# After editing findings.yaml: regenerate in place
mise run conformance

# CI gate: validate schema + fail on any drift
mise run conformance-gate
```

`mise run conformance` runs `uv run python scripts/conformance_view.py generate` and
overwrites `docs/conformance/model.json`, `INDEX.md`, and all `components/*.md`.

`mise run conformance-gate` runs `scripts/conformance_view.py check`. It:
1. Validates every finding against the schema (required fields, allowed severity/status).
2. Regenerates the view into a temp directory.
3. Diffs every generated file byte-for-byte against the committed `docs/conformance/`
   tree. Any divergence exits non-zero with `CONFORMANCE DRIFT DETECTED`.

`conformance-gate` is wired into `mise run ci` — it runs on every full local CI pass.
See [Gate System](gate-system.md) for the broader gate picture.

---

## The one file you edit: findings.yaml

Every gap the campaign tracks is a YAML record in `docs/conformance/findings.yaml`. The
file header documents the full schema; the key fields are:

```yaml
- id: TDN-3                         # stable ID; never renumber or reuse
  nodes: [TimeSensor]               # concrete node(s) — OR use interfaces:
  interfaces: [X3DTimeDependentNode] # tags all implementers instead of listing each node
  clause: "8.2.4.3"                 # ISO 19775-1 section number
  severity: major                   # critical | major | minor | low
  status: open                      # open | deferred | fixed | closed
  wave: 1                           # optional: campaign wave number
  commit: c7d2c21                   # optional: the closing commit (for fixed/closed)
  summary: "loop=FALSE finishes the current cycle instead of deactivating next tick."
  note: "Optional closure plan or deferral reason."
```

**Status semantics:**
- `open` — actionable now; no blocking dependency.
- `deferred` — blocked on a missing subsystem, seam, or design (e.g. Sound component,
  collision subsystem). Counted in open totals for the bug picture but not prioritized.
- `fixed` — committed fix; kept for history and closed-rate tracking.
- `closed` — same as fixed; use interchangeably.

### Raising a new finding

Append a finding to the relevant section of `findings.yaml`. Required fields: `id`,
`summary`, one of `nodes`/`interfaces`, `severity`, `status`. Run
`mise run conformance` to validate the schema and regenerate the view.

**ID convention:** use a prefix matching the behavioral interface or cluster plus a
sequential number — `TDN-8`, `INTERP-03`, `BIND-09`, `MAT-007`. Within a section,
IDs sort lexicographically (zero-pad numerics if the section may exceed 9 entries).

**Gotcha:** every finding must tag at least one node name (in `nodes:`) or at least one
interface name (in `interfaces:`). A finding with neither fails schema validation and
blocks the gate. Use `interfaces:` when the gap applies uniformly to all implementers
(e.g. all drag sensors); use `nodes:` when the gap is specific to one or two concrete
types.

### Closing a finding

Set `status: closed` (or `fixed`) and add the commit hash:

```yaml
  status: closed
  commit: abc1234
```

Then regenerate and commit together:

```bash
mise run conformance
git add docs/conformance/findings.yaml docs/conformance/
git commit -m "fix(…): close FINDING-ID — <one-line reason>"
```

The gate passes because the committed view now matches the regenerated one.

**Gotcha:** do not delete or renumber old fixed/closed findings. They are the campaign
history and the `closed_count` in `INDEX.md` would drift if you did. The gate would also
catch the deletion as drift.

---

## Cadence

The campaign runs in **session-scoped phases**: one audit fan-out (3–4 behavioral
interface clusters) plus one fix cycle per session. This keeps the findings register and
the closures in sync so the register never rots, and bounds token spend (a fan-out of
3 interfaces calibrated at ~1.9M subagent tokens / 55 agents).

The design rationale and full methodology are in
`docs/superpowers/specs/2026-06-18-conformance-gap-register.md`.

---

## The audit workflow

When starting a new fan-out wave:

1. **Partition the registry.** Pick 3–4 behavioral interface clusters not yet audited.
   The full set of candidates is the list in `scripts/conformance_view.py` under
   `BEHAVIORAL_INTERFACES`. Completed interfaces are those where all implementers read `✓`
   or `◑` in `INDEX.md` with no `inert` classification.

2. **Fan-out per interface.** For each cluster, a subagent reads the ISO 19775-1 prose
   (via spec_rag at `scripts/spec_rag.py` or the prose mirror at
   `$X3D_SPEC_PROSE_DIR`), reads the corresponding runtime System headers under
   `runtime/events/` and `runtime/scene/`, and produces candidate findings.

3. **Adversarial verify (3-skeptic protocol).** Each candidate is reviewed by three
   independent skeptics. A finding survives iff at least 2/3 confirm it. This filters
   misreadings of ambiguous prose. The audit workflow design is in
   `docs/superpowers/specs/2026-06-18-conformance-audit-workflow-design.md`.

4. **Prioritize.** Surviving findings are scored by `severity × corpus-prevalence ×
   1/effort`. Critical/major findings on nodes used in the Interchange or Interactive
   profiles come first.

5. **Completeness critic.** A final critic reviews the confirmed findings against the
   `unchecked_behaviors` list to catch behaviors the auditors skipped. If the critic
   raises a behavior the verifier inconsistently rejected, it goes back for one re-review
   (calibration lesson: the verifier rejected `pauseTime_changed` 1/3 while confirming
   the identical mandate `resumeTime_changed` 2/3 — both survived on re-review).

6. **Fold into findings.yaml.** Add confirmed findings using the schema above. Run
   `mise run conformance`. Commit the updated YAML and regenerated view together.

7. **Fix cycle.** In the same session or the next: pick the highest-priority open
   findings, write C++ fixes using TDD (write a test, make it pass, verify golden
   byte-identical), update `status` to `closed` + add `commit:`, regenerate, commit.

---

## Speccheck before closing

Before closing a finding as "not a bug" (reclassifying to `deferred` or removing):
always re-read the ISO prose directly. The campaign has had adversarially-confirmed
findings overturned by a direct spec-check — TXF-1 (texture scale/rotate order) was
confirmed by the audit then reclassified `deferred` because the normative sentence
contradicted the matrix the verifiers weighted. Do not trade one spec reading for
another without reading the clause.

---

## Reading the INDEX.md bug picture

The top of `docs/conformance/INDEX.md` shows a generated summary like (illustrative — exact counts move as `findings.yaml` evolves):

```
Open/deferred gaps: 16 critical, 15 major, 15 minor, 3 low (36 deferred) · closed: 59
Inert behavioral nodes (no System wired): Analyser, AudioClip, …
Worst-offender components: Followers (8), Networking (8), …
```

- **Inert** means the node has a behavioral interface but no System is registered for it
  in the runtime. It will never fire an event or output a value. These are the highest
  priority to fix (or explicitly defer with a deferral reason).
- **Deferred** findings count toward the "open/deferred" total but are separately
  parenthesized. They are not ignored — they are tracked with explicit deferral reasons
  so they can be closed when their blocking dependency ships.
- **Worst-offender components** are ranked by open finding count. Use this to decide
  which component to target next.

---

## Stale findings

If the gate reports `CONFORMANCE DRIFT DETECTED` with a stale-finding warning like:

```
⚠️ Stale findings (reference unknown nodes/interfaces): XYZ-01
```

A finding's `nodes:` or `interfaces:` list names a node or interface that no longer
exists in the generated bindings or registry. This happens when a node is renamed or
removed. Fix: update the `nodes:`/`interfaces:` list in `findings.yaml` to match the
current name, then `mise run conformance`.

---

## Gotchas summary

| Gotcha | How to avoid it |
|---|---|
| Editing `INDEX.md` or `components/*.md` directly | They are generated — edit `findings.yaml` only |
| A finding with neither `nodes:` nor `interfaces:` | Schema validation fails the gate; always tag at least one |
| Forgetting to run `mise run conformance` after editing | Gate detects drift; always regenerate before committing |
| Deleting closed findings | Breaks `closed_count` and causes drift; keep them as history |
| Renaming a node without updating findings.yaml | Stale-finding warning fails the gate; update the `nodes:` field |
| Closing a finding without adding `commit:` | Allowed (not schema-required), but the history is incomplete; add it |
| Treating `deferred` as "won't fix" | Deferred = blocked; re-evaluate when the blocking subsystem ships |
| Over-weighting one prose sentence over another | Always re-read the full clause before calling a behavior conformant |
