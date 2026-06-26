---
title: Card → Done Workflow
summary: How a GitHub Project card is handed to an agent and driven to documented completion — the Definition of Ready/Done gates, the state machine, and the traceability chain that ties card → issue → branch → PR → docs → Done.
tags: [guide, workflow, project, agent, definition-of-done]
updated: 2026-06-22
related:
  - workflow-subagent-discipline.md
  - gate-system.md
---

# Card → Done Workflow

The [GitHub Project](https://github.com/users/delta9000/projects/2) is the planning board;
[`findings.yaml`](https://github.com/delta9000/x3d-cpp/blob/main/docs/conformance/findings.yaml)
is the conformance source of truth (see the [BACKLOG deprecation](../index.md)). This guide is the **connective tissue**: how one card
becomes shipped, documented work.

It does **not** replace [Workflow / Subagent Discipline](workflow-subagent-discipline.md) —
that guide is the *how* of multi-agent execution (phases, isolation, COST). This guide is the
*pipeline around it*: the gates a card passes through and the audit trail it leaves.

Grounded in proven practice — spec-driven development (GitHub Spec Kit / AWS Kiro), the
explore→plan→code→commit loop (Anthropic Claude Code best practices), INVEST/DoR/DoD (agile),
the Rust-RFC "gate big, flow small" carve-out, and Conventional-Commits traceability.

---

## The pipeline at a glance

```
Backlog ──(Definition of Ready)──▶ Ready ──(pick: convert to issue)──▶ In progress
   │                                                                        │
   │                                                              plan → TDD → implement
   │                                                                        ▼
 Done ◀──(merge: Closes #N auto-moves)── In review ◀──(adversarial review + PR)── verify
```

WIP discipline: **≤ 1 card In progress per worker; ≤ 3 cards Ready.** Without a cap a
solo+agents setup starts many and finishes few; a small Ready queue stops agents being fed
unvalidated specs.

---

## Stage 0 — Refine (`Backlog → Ready`)

A card is **Ready** only when it passes the **Definition of Ready**. Under-specified items are
the #1 cause of agent failure (a measured 20–40 % drop in success rate), so this gate is not
optional.

**Definition of Ready** — the card body must contain:

- [ ] **Anchors** — the seam + exact `file:line` locations and interfaces the work touches.
      Verify each path exists (`ls`/`grep`) before marking Ready — stale anchors make agents
      invent file paths.
- [ ] **Acceptance criteria** — testable bullets. *Seam cards already carry these as their DoD
      checklist.*
- [ ] **A runnable verification command** — the single most important artifact. One of:
      `mise run ci`, a named new `ctest`, or an `x3d sim` golden trace. "Without a check the
      agent can run, 'looks done' is the only signal and you become the verification loop."
- [ ] **Docs-to-touch named** — which living doc(s) the change updates: the subsystem page,
      `findings.yaml`, `v1-capabilities.md`, the seam-status matrix, or a new ADR.
- [ ] **One-PR sized** (INVEST-small). If larger, decompose into sub-cards. Seam cards are
      *epics* → split into their DoD lines.

**The fork — gate big, flow small** (Rust's RFC carve-out; don't gate everything or the
process becomes bureaucracy):

| Card kind | Path |
|---|---|
| **Substantial** — seam-hardening milestone, new subsystem, binding decision | Write a dated **design spec** (`docs/superpowers/specs/`) **+ an ADR** first, review-gated by the human, then run the **five-phase fan-out workflow** ([discipline guide](workflow-subagent-discipline.md)). |
| **Small** — a Core card, a single finding, a localized fix | Skip the spec. One agent: `writing-plans` → `test-driven-development` → implement. |

The `Seam`/`Phase` fields on the board tell you which: a `Prove … generic` card is substantial;
its pulled-in finding-cards are usually small.

---

## Stage 1 — Dispatch (`Ready → In progress`)

1. **Convert the draft card → a GitHub Issue.** Draft cards cannot carry `Closes #N` or trigger
   auto-move; an issue makes the exit chain machine-checkable. Backlog stays clean drafts —
   issue noise only appears once work is **live**. Use the helper:
   ```
   scripts/pick-card.sh "Prove ScriptEngine generic: Duktape + QuickJS"
   ```
   It resolves the card, converts it to an issue (staying in the Project), and prints the
   handoff packet.
2. **Branch off `main`:** `feat/<seam>-<slug>` (e.g. `feat/scriptengine-quickjs`).
3. **Hand the agent the handoff packet** — the refreshed `COMMON` block from the
   [discipline guide](workflow-subagent-discipline.md#common-string-template): repo root, the
   *branch-off-main + PR* model, `BUILD RULE: cmake --build --preset dev`, the GOLDEN RULE
   (codegen-free?), the verified anchors, the acceptance criteria + verification command, and
   the docs-to-touch list.
4. **One card = one session** (context discipline; `/clear` between cards). Freeze the spec at
   dispatch — route mid-task changes to the PR, not the issue.

---

## Stage 2 — Verify (`In progress → In review`)

- **Fresh-context adversarial review by a *different* agent** (never the implementer) using the
  existing `REVIEW_SCHEMA` (`approved` / `buildPassed` / `testsPassed` / `goldenUntouched` /
  `issues`). Fix bound **≤ 2 rounds**, then escalate to the human.
- **Open the PR:** semantic title (Conventional-Commits grammar, already the repo's style),
  body contains `Closes #N`, the ticked DoD checklist, and the verification command's output.
- **Living-doc updates are in the same diff** — run `mise run docs-drift` as the advisory
  pre-check. Docs as a follow-up ticket = drift returns immediately.
- PR marked `ready for review` → Project automation moves the card to **In review**.

---

## Stage 3 — Done (`In review → Done`)

- Required status check (`mise run ci` via [CI](https://github.com/delta9000/x3d-cpp/blob/main/.github/workflows/ci.yml))
  green + one human approval (author ≠ final approver). Merge.
- The PR's `Closes #N` auto-closes the issue → built-in automation moves the card to **Done**.
- **Documented completion** = the durable record, not the closed card:
  - `findings.yaml` status flipped (then `mise run conformance`), and/or
  - the subsystem page / `v1-capabilities.md` updated, and/or
  - the **seam-status matrix** row turned green.
  - For a seam card, "Done" *means* the dual-backend swap-test is green in CI.
  - If the change adds a third-party backend/dependency (FetchContent / vcpkg /
    vendored), the root `NOTICE` lists it (component, source, license, build
    flag) **in the same diff**. `docs-drift` does not cover `NOTICE`, so it has
    silently drifted before — treat it as part of the diff like the living docs.

---

## The traceability chain (every link machine-checkable)

```
card ─▶ issue #N ─▶ branch feat/<seam>-<slug> ─▶ commits "feat(seam): … (#N)"
     ─▶ PR "Closes #N" + DoD ─▶ merge ─▶ issue closed ─▶ card Done
     (living docs updated in the same diff; seam-status matrix is the lasting record)
```

| Link | Mechanism |
|---|---|
| card → issue | `scripts/pick-card.sh` (`convertProjectV2DraftIssueItemToIssue`) |
| issue → branch | `feat/<seam>-<slug>` naming |
| branch → commits | Conventional Commits with `(#N)` |
| PR → issue | `Closes #N` in the PR **body** |
| PR ↔ card status | Project built-in automations (ready-for-review → In review; merge → Done) |
| code → docs | same-PR diff + `docs-build` dead-link gate + `docs-drift` advisory |

---

## Quick reference

| Situation | Rule |
|---|---|
| Card not Ready | Missing anchors / acceptance criteria / verify command / docs list — refine first |
| Substantial card (seam / subsystem / decision) | Spec + ADR + five-phase workflow before code |
| Small card | One agent: plan → TDD → implement |
| Picking a card | `scripts/pick-card.sh "<title>"` → issue + handoff packet |
| Branch name | `feat/<seam>-<slug>` off `main` |
| Reviewer | A different agent than the implementer; ≤ 2 fix rounds |
| Definition of Done | CI green + docs in the same diff + human approval + `Closes #N` |
| WIP | ≤ 1 In progress / worker; ≤ 3 Ready |
