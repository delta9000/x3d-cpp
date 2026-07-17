---
title: Workflow / Subagent Discipline
summary: How multi-agent fan-out workflows are structured here — phases, isolation rules, review gates, and the COST discipline that keeps runs from hitting the session limit.
tags: [guide, workflow, subagent, multi-agent, discipline]
updated: 2026-06-20
related:
  - gate-system.md
---

# Workflow / Subagent Discipline

Multi-agent workflows are the primary mechanism for large, structured work in this project.
This guide records how they are built and why — grounded in the workflows that actually
shipped (`v1-closure`, `script-sai`, `m2d-interaction`, the conformance audit waves, the wiki
waves).

---

## The two-layer model: spec before workflow

Every workflow has a sibling dated design spec. The spec is written and **review-gated by the
user before the workflow launches**. This separation is non-negotiable:

1. **Design spec** (`docs/superpowers/specs/YYYY-MM-DD-<name>-design.md`) — the "what and why":
   ISO clause anchors, data schemas, architectural decisions, known risk areas, phase
   structure. The human reads it and approves before any subagent runs.
2. **Workflow script** (`docs/superpowers/plans/YYYY-MM-DD-<name>-workflow.js`) — the
   executable plan: phases, prompts, schemas, constants. It reads the spec; it does not
   re-derive the design.

The pattern prevents subagents from designing as they go, which produces inconsistent
seams and mixes research with execution cost.

---

## Phase shape

All workflows in this project follow the same five-phase spine. Not every workflow uses
every phase; skip phases that genuinely don't apply.

| Phase | Runs | Purpose |
|---|---|---|
| **Spec-check** | parallel, read-only | Ground each work unit in ISO prose + code reality before touching anything |
| **Implement** | parallel or sequential per unit | Author / create the artifacts (new files, edits) |
| **Review** | parallel, adversarial | Per-unit adversarial check: build + ctest + golden-untouched |
| **Fix** | sequential per issue | Address review findings; re-review bound ≤ 2 rounds |
| **Verify / Gate** | single consolidating agent | Golden byte-identical + full ctest + conformance-gate |

The **review gate is mandatory** before the fix phase. Skipping it and fixing forward from
implementation output alone has historically let subtle errors through that ctest caught only
at the gate phase — wasting the whole fix round.

---

## Isolation rules for parallel implementation

Parallel subagents **must not share mutable state**. In practice that means:

- **Author-phase agents** work on disjoint files. The v1-closure workflow achieved this by
  assigning each agent a new header-only file (`TextureResolver.hpp`, `FontMetrics.hpp`,
  `TextLayout.hpp`) with no overlap. Self-check via
  `g++ -std=c++20 -fsyntax-only -I generated_cpp_bindings -I runtime <header>` before
  touching the shared `build/` directory.
- **Integration** (edits to shared files like `SceneExtractor.hpp`, `CMakeLists.txt`) runs
  **sequentially** — never in parallel. The m2d-interaction-layer workflow serialized its
  four implementation units specifically because they all wrote into
  `runtime/events/PointingSensorSystem.hpp`.
- **Build** is always `cmake --build --preset dev` (matching `mise run build`). After the C1
  decl/def split the Ninja job pool is set at configure time to the logical-core count; no
  `-j` cap is needed. Tests run via `ctest --preset dev`.

---

## The COST discipline: batch your verifiers

The single biggest cost mistake in this project was pairing one verify agent with one author
agent **per page** in wiki Wave 2. With 47 pages, that produced ~94 agents, ~3.76M subagent
tokens, and **hit the session limit mid-run** — the verify, fix, and gate stages all failed
to complete.

The recovery ran the same accuracy audit with **6 batched verifiers (≈ 8 pages each) + 1
consolidated fix agent = 7 total agents**. Coverage was identical; cost was roughly 7×
cheaper.

**The rule:** verification is a *reading* task, not a *mutation* task. One agent can audit 8
pages — or 8 findings, or 8 test files — in a single pass. Reserve one-agent-per-item for
phases that **mutate files in parallel** (authoring / editing needs isolation to avoid
conflicts; auditing does not).

| Phase | Agent count |
|---|---|
| Author (parallel, disjoint files) | one per item |
| Verify / review (read + judge) | batch: ≈ 8 items per agent |
| Fix (sequential, single consolidating) | one |
| Gate (golden + ctest) | one |

**Signal:** if your workflow has > 50 agents, check whether any verify/audit/review stage is
one-per-item. Batch it.

---

## The implement → adversarial-verify → fix loop

Every unit that mutates the codebase gets a dedicated adversarial reviewer. The reviewer
receives a schema that forces a verdict:

```js
const REVIEW_SCHEMA = {
  approved: boolean,
  buildPassed: boolean,
  testsPassed: boolean,
  goldenUntouched: boolean,
  issues: string[],
}
```

The reviewer is explicitly NOT the same agent that did the implementation. Its default stance
is sceptical. It runs the build (`cmake --build --preset dev`), ctest, and verifies the golden
hash did not drift — these are not optional. See the actual schemas in
`docs/superpowers/plans/2026-06-16-m2d-interaction-layer-workflow.js` and
`docs/superpowers/plans/2026-06-16-v1-closure-workflow.js`.

The fix bound is **≤ 2 re-review rounds**. If issues survive two fix rounds, the session
stops and the human decides — unlimited re-review loops chew cost without convergence.

---

## The conformance audit pattern: pipeline + adversarial verify per finding

The behavioral conformance audits (waves 1–4, 2026-06-18/19) ran a specialized shape:

```
audit-agent-per-interface
  → per finding: 3 perspective-diverse verify agents in parallel (pipeline, not barrier)
  → synthesize (barrier — all findings must be confirmed)
  → completeness critic (single agent)
```

Key decisions documented in
`docs/superpowers/specs/2026-06-18-conformance-audit-workflow-design.md`:

- **Pipeline, not barrier between audit and verify.** Interface B's findings start verifying
  while interface A's verifiers are still running.
- **Three lenses per finding:** (L1) "handled elsewhere?", (L2) "does the clause actually
  mandate this?", (L3) "would it reproduce in a real scene?" Each lens returns
  `is_real: boolean` with confidence. A finding survives if ≥ 2 of 3 confirm it.
  Default on uncertainty: refute. This precision gate is what kept the register trustworthy.
- **Completeness critic last.** After synthesis, one final agent asks "what did the audit
  miss?" It re-derives the interface list and flags any behavioral interface with no System at
  all.

Calibration: wave 1 (3 interfaces) cost ~1.9M subagent tokens / ~55 agents. The full
11-interface remainder was estimated at ~7M tokens / ~175 agents. That does NOT fit alongside
a fix cycle in one session — separate them.

---

## Pre-flight checklist before launching a workflow

1. **RAGs current.** If the runtime changed since the last ingest, run
   `mise run code-ingest` before launching. Verify with
   `mise run code-rag query "<something recently changed>"` — stale chunks return deleted
   files, which cause agents to propose edits to non-existent paths.

2. **Design spec approved.** The dated spec must be committed and reviewed by the user.
   The workflow script `COMMON` string should reference it: agents are told to read the spec
   first before doing anything.

3. **Build clean.** Run `mise run ci` (or at minimum `mise run build`) to confirm the
   baseline is green before adding agents on top of a broken state.

4. **One big push at a time.** The m2d-interaction and script-sai workflow scripts both
   carry an explicit `LAUNCH ONLY AFTER <prior workflow> has landed` header. Concurrent
   workflows on overlapping files race each other. Serialize them.

5. **Golden rule stated.** Every `COMMON` string must state whether the workflow is
   "codegen-free" (no generator/template touch, golden hash must stay byte-identical) or
   "codegen-authorized" (template changes allowed; regenerate via `mise run gen` then commit
   new headers). Ambiguity on this point caused the most expensive review failures.

---

## COMMON string template

Every workflow script in this project wraps agent instructions in a `COMMON` constant that
agents receive at the top of every prompt. The pattern from the shipped workflows:

```js
const COMMON = `Repo: <repo-root>
  Work on a feature branch off main: feat/<seam>-<slug>. Open a PR; do NOT commit to main.
  PR body must include "Closes #<issue>" and the ticked Definition of Done.
Read the design spec first: docs/superpowers/specs/YYYY-MM-DD-<name>-design.md.
BUILD RULE: cmake --build --preset dev  (matches mise run build; Ninja job pool set at configure time).
GOLDEN RULE: this work is codegen-free — do NOT touch generated_cpp_bindings/.
  Golden hash MUST stay byte-identical.
VERIFY: run <the card's verification command> AND mise run ci to green before opening the PR.
DOCS: update the living docs named on the card in THIS PR (mise run docs-drift as a pre-check).
<anchors: exact file:line locations of the seams this work uses>
Follow TDD (red→green). Commit each unit with a clear message (Conventional Commits, "(#<issue>)") when green.`
```

> **Note (2026-06-22):** the trunk model changed at public release. Work now happens on
> feature branches off `main` with a PR per card (not direct commits to a `modernize-x3d-spec`
> trunk). The dispatch/handoff sequence around this template is the
> [Card → Done Workflow](card-to-done-workflow.md).

The anchors section is critical. Before writing the workflow, verify that each cited path
exists (`ls` or `grep`) — stale anchors are a leading cause of agents inventing file paths
that do not exist.

---

## How the gate ties in

After the workflow's verify phase, the full gate suite runs as a single consolidating step:

```bash
mise run golden             # generator output byte-stable
mise run conformance-gate   # findings.yaml schema + generated view
mise run cli-gate-regression # validate/convert/canonicalize baseline
mise run docs-build         # wiki strict build (if wiki pages were touched)
mise run build              # cmake configure + compile + ctest
```

Or all together: `mise run ci`.

The gate is not optional even if `ctest` passed in the review phase — the per-unit review
checks the unit in isolation; the gate checks the whole-system integration. See
[Gate System](../wiki/guides/gate-system.md) for what each gate protects and when to regenerate a baseline.

---

## Quick reference

| Situation | Rule |
|---|---|
| > 50 agents in a workflow | Check verify stages — batch them |
| Verify / audit / review stage | ≈ 8 items per agent |
| Authoring new disjoint files | One agent per file (parallel OK) |
| Editing shared files | Sequential only |
| Build command | `cmake --build --preset dev` (Ninja pool auto-sized; matches `mise run build`) |
| Fix rounds | ≤ 2; escalate to human if not resolved |
| Pre-launch RAG check | `mise run code-ingest` if runtime changed since last ingest |
| Concurrent workflows | Never on overlapping files; serialize explicitly |
