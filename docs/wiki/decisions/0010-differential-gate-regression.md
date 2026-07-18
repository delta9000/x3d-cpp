---
title: "ADR-0010: Differential CLI Gate as Baseline Regression"
summary: The differential CLI gate (cli-gate / canon-gate --gate) is wired as a permanent baseline regression gate in CI.
tags: [adr, differential-gate, regression, cli-gate, canon-gate]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/cli-suite.md
  - ../guides/gate-system.md
---

# ADR-0010: Differential CLI Gate as Baseline Regression

## Status

Accepted — 2026-06-20

## Context

The [CLI Suite](../subsystems/cli-suite.md) produces a rich differential signal against the Web3D reference implementation (X3DJSAIL) and via its own round-trip harnesses. After the first golden-generation run, that signal existed only as an informational report in `tools/x3d-cli/goldens/divergence-report.md`. Nothing stopped a future change from silently regressing a file that previously agreed with X3DJSAIL, or silently breaking a convert round-trip that previously passed.

Three forces drove the decision:

**The project's "golden files in git" ethos.** All other hard-won correctness results — the generated binding layer, the conformance view, the golden sim traces — are committed artifacts that CI diffs against. The differential outputs fit naturally into that pattern. Leaving them as informational-only would be inconsistent and would make them gradually stale.

**The first-run divergences are partially justified, partially bugs.** The triage produced two distinct categories: real spec gaps and bugs to fix (convert script-CDATA trimming, validate coordIndex checks) vs. known, justified divergences where X3DJSAIL applies stricter-than-spec conventions that we intentionally do not (X3DJSAIL requiring `<meta>`, X3DJSAIL rejecting `containerField='rootNode'` that was valid on 3.x files). Once that distinction is drawn and documented in a baseline, a regression gate can enforce it: PASS-to-FAIL is a regression; FAIL-to-PASS is a recorded improvement. Known FAIL rows stay allowed without widening the net.

**Java-free CI is non-negotiable.** X3DJSAIL requires a JDK (and for `-canonical`, specifically JDK 25 due to a class-version incompatibility with JDK 17). Installing a JDK in routine CI would add a large, fragile dependency to every build. The golden-gen + captured-baseline architecture already solves this: run Java once to produce fixtures, commit them, run everything else against the committed artifacts. The gate must not reintroduce a Java dependency.

The design also had a specific footgun to close: a gate that runs against a missing corpus directory produces zero checks. If zero checks were silently allowed to pass the gate, a CI environment without the corpus would always be green — a false safety. This "0-checks" failure mode had to be an explicit exit-1, not a vacuous pass.

See the full design rationale at `docs/superpowers/specs/2026-06-20-x3d-cli-convert-validate-design.md`.

## Decision

We decided to wire both differential harnesses as a permanent baseline-regression gate in `mise run ci`, using a `--gate` mode that compares current results against committed TSV baselines:

- `tools/x3d-cli/goldens/cli-gate-baseline.tsv` — 360 data rows covering validate-diff (200-file subset × validate check) and convert round-trip (160 pairs); header records the snapshot agreement rates (`validate: 40/200 agree convert: 160/160 pass`).
- `tools/x3d-cli/goldens/canon-gate-baseline.tsv` — 406 data rows covering X3DC14N Tier-1 idempotence (204 files) and Tier-2 tolerant-diff vs X3DJSAIL (202 XML files); header records rates (`T1-idempotence: 204/204 T2-tolerant: 22/202`).

The gate logic is: **PASS-in-baseline and FAIL-now = regression; exit non-zero.** FAIL-in-baseline rows remain allowed — they are the committed, reviewed set of justified divergences. Improvements (FAIL → PASS) are logged but do not fail CI.

Canon Tier-1 (idempotence) is a hard 100% gate: every file must canonicalize to the same output twice, with no baseline allowlist. This check is always enforced, regardless of whether Tier-2 or Tier-3 agrees with X3DJSAIL.

The zero-checks footgun is closed: `cli_gate.cpp` exits 1 immediately if the gate runs 0 checks (`currentState.empty()` after the corpus walk — `tools/x3d-cli/cli_gate.cpp` line 720), rather than reporting "no regressions" on an absent corpus.

Baselines are refreshed explicitly, not automatically: `mise run cli-gate-baseline` (runs both harnesses with `--write-baseline`), then the caller reviews and commits the updated TSVs. This makes baseline widening a deliberate, visible commit rather than a silent side effect of CI.

## Consequences

**Positive:**

- Every validate/convert/canonicalize result that was correct at the time the baseline was captured is now locked. A future codec change cannot silently regress a previously-passing file without failing CI.
- The justified-divergence set is explicit and reviewable. Any claim that "we differ from X3DJSAIL for a good reason" must be present in the committed baseline with a recorded reason (`divergence-report.md`); undocumented divergences are regressions.
- CI remains Java-free. The gate runs entirely against committed goldens and our own CLI binary — no JDK required. X3DJSAIL is invoked only in `mise run cli-golden-gen` / `mise run cli-golden-gen`, which is an on-demand, developer-local step.
- The canon Tier-1 100% hard gate means idempotence of the canonical form is always enforced, independent of X3DJSAIL availability or agreement rates. This is the strongest correctness property of the canonicalize command.
- The `--write-baseline` workflow makes accepting new divergences intentional: a developer must run the baseline task, inspect the diff, and commit. This prevents both accidental regression and accidental acceptance of new failures.

**Trade-offs / costs:**

- The baseline files are large committed artifacts (366 and 408 TSV lines). They drift whenever the corpus subset changes or a new divergence is accepted, producing noisy git history. The trade-off is accepted: the correctness guarantee outweighs the TSV churn.
- The validate agreement rate (40/200 = 20%) looks low as a headline number. The ADR locks in this rate as the baseline floor, which makes the metric potentially misleading to a reader unfamiliar with the triage: most of the FAIL rows are X3DJSAIL applying stricter-than-spec conventions (requiring `<meta>`, rejecting `containerField='rootNode'`), not gaps in our validator. The `tools/x3d-cli/goldens/validate-check-categories.md` worklist breaks the 160 disagreements down by category (`jsail-error` 114, `invalid-only-jsail` 43, `inline-warn-us` 2, `range-only-us` 1) so improvements are tracked per category, not by the headline percentage alone; the TSV itself does not carry this.
- The Tier-2 canon agreement rate (22/202) is locked as a FAIL baseline for those files that do not yet agree with X3DJSAIL. Tier-3 (byte-exact rate) is informative only and is NOT baseline-locked; `canon_gate.cpp` documents T3 as "reports the gap; NOT a gate" and no canon-t3 entries exist in the baseline TSV. The underlying cause of Tier-2 failures — the parser discards per-field was-set state and per-node child source order (needed to match X3DJSAIL's source-preserving X3DC14N) — is a substantial future effort (`reader source-provenance` tracking).
- `mise run cli-golden-gen` is not in CI (requires JDK 25 + the X3DJSAIL jar cached in `tools/x3d-cli/.javacache/`, not committed). The verdicts it produces (`tools/x3d-cli/goldens/validate-verdicts.tsv`) are committed, but they become stale if the corpus subset changes without a re-run. This is documented as a maintenance responsibility.

## Related

- [CLI Suite](../subsystems/cli-suite.md) — the validate, convert, and canonicalize commands that the gate exercises; key files `tools/x3d-cli/cli_gate.cpp`, `tools/x3d-cli/canon_gate.cpp`, `tools/x3d-cli/scene_equiv.hpp`
- [Gate System guide](../guides/gate-system.md) — operational detail: how to run `mise run cli-gate-regression`, regenerate baselines, and what the `--gate` vs informative modes do
- [Architecture](../architecture.md)
- Design spec: `docs/superpowers/specs/2026-06-20-x3d-cli-convert-validate-design.md` — the "Phase B — gates" section; the "captured-golden differential gate" rationale; the convert-differential pivot off X3DJSAIL's converter onto its `-validate` (XSLT quirk)
- Committed baseline artifacts: `tools/x3d-cli/goldens/cli-gate-baseline.tsv`, `tools/x3d-cli/goldens/canon-gate-baseline.tsv`, `tools/x3d-cli/goldens/validate-check-categories.md`, `tools/x3d-cli/goldens/divergence-report.md`
