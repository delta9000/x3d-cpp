---
title: Gate System
summary: How the golden, conformance, CLI regression, and docs gates work and how to run them.
tags: [guide, gates, golden, conformance, regression, docs]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/cli-suite.md
---

# Gate System

The project enforces four gates, each protecting a different correctness axis. All four run
together in `mise run ci`. Understanding what each gate protects — and how to update a
baseline when a divergence is intentional — is the discipline backbone for working on the
codebase.

---

## Overview

| Gate | Mise task | Axis protected | Exit-1 condition |
|---|---|---|---|
| GOLDEN | `mise run golden` | Generator output byte-stability | Any `*.hpp`/`*.cpp` drift in `generated_cpp_bindings/` |
| CONFORMANCE | `mise run conformance-gate` | findings.yaml schema + generated view drift | Schema error or generated Markdown diverges from `docs/conformance/` |
| CLI REGRESSION | `mise run cli-gate-regression` | Validate/convert/canonicalize baseline regression | Any baseline-PASS item now fails |
| DOCS | `mise run docs-build` | Wiki internal-link integrity and nav completeness | Broken link, orphan page, or strict-build error |

---

## Gate 1: GOLDEN (`mise run golden`)

**What it protects.** The generator (`x3d-cpp-gen`) is the single source of truth for the C++
binding layer. Every `*.hpp` and `*.cpp` under `generated_cpp_bindings/` is a generator
artifact committed to git. The GOLDEN gate re-runs the generator into a throwaway temp
directory and `diff`s the output byte-for-byte against the committed tree. Any drift — content
change, added file, deleted file — fails CI.

This makes codegen changes **opt-in**: you must regenerate and commit the new headers before
the gate passes. The canonical header `generated_cpp_bindings/X3DNode.hpp` (the base class
for all instantiable nodes) is a representative example of generator-emitted output.

**Script.** The gate is implemented in `scripts/check_golden.sh`. It:
1. Runs `uv run x3d-cpp-gen --out <tmp> --no-test` (omits the smoke test binary, which is
   gitignored and not golden).
2. Walks the committed `generated_cpp_bindings/` tree checking every `*.hpp`/`*.cpp` against
   the regen output, and vice versa (catches additions and deletions).
3. Prints a diff snippet for each drifted file and exits non-zero.

**Running it.**

```bash
mise run golden
```

**What golden drift means.** If a template or emitter change causes drift, the gate output
names every drifted file with a short unified diff. Drift is always intentional after a
deliberate codegen change, and accidental otherwise (e.g. a whitespace-only template
formatting change that was not committed).

**Regenerating when a divergence is intentional.**

```bash
uv run x3d-cpp-gen --out generated_cpp_bindings   # overwrite the committed tree
git add generated_cpp_bindings/                    # stage the new headers + sources
# then commit — the gate will pass again
```

---

## Gate 2: CONFORMANCE (`mise run conformance-gate`)

**What it protects.** The conformance campaign tracks behavioral gaps against the ISO 19775-1
prose. The SOURCE OF TRUTH for all judgments is `docs/conformance/findings.yaml`. A Python
script (`scripts/conformance_view.py`) derives facts (node exists? extracts? wired? which
component/profile?) automatically from the bindings and registry, then joins them with the
authored judgments to produce:

- `docs/conformance/model.json` — machine-readable conformance model
- `docs/conformance/INDEX.md` — human zoom-out
- `docs/conformance/components/*.md` — per-component breakdowns

The gate (`conformance_view.py check`) validates the `findings.yaml` schema and regenerates
the view into a temp directory, then diffs byte-for-byte against the committed
`docs/conformance/` files. Any schema error or uncommitted regeneration output fails CI.

**Running it.**

```bash
mise run conformance-gate   # gate mode (used in CI)
mise run conformance        # regenerate in place (after editing findings.yaml)
```

**The one file you edit.** To raise, update, or close a behavioral gap:

1. Edit `docs/conformance/findings.yaml`. Each finding has a stable `id`, `severity`,
   `status` (`open | deferred | fixed | closed`), `clause`, and `summary`. Facts are never
   authored here — they are auto-derived and cannot rot.
2. Run `mise run conformance` to regenerate the view in place.
3. Commit `findings.yaml` together with the regenerated `docs/conformance/` files.

The gate will then pass because the committed view matches the regen.

---

## Gate 3: CLI REGRESSION (`mise run cli-gate-regression`)

**What it protects.** The [CLI Suite](../subsystems/cli-suite.md) is the first real SDK
consumer. Two differential harnesses compare the SDK's `validate`, `convert`, and
`canonicalize` outputs against committed baselines:

- `tools/x3d-cli/goldens/cli-gate-baseline.tsv` — validate agreement + convert round-trip
  results over the curated corpus subset
- `tools/x3d-cli/goldens/canon-gate-baseline.tsv` — X3DC14N idempotence (Tier 1) +
  tolerant-diff vs X3DJSAIL (Tier 2) results

The regression gate runs both harnesses in `--gate` mode. It fails if **any item that was
PASS in the baseline is now FAIL** — new failures are never silently absorbed. Items that
were already FAIL in the baseline are allowed (they represent known/justified divergences).

The canon Tier-1 (idempotence) check is a **hard 100% gate**: every file in the corpus must
canonicalize to the same output twice. This is Java-free and always runs.

**Running it.**

```bash
mise run cli-gate-regression               # uses default corpus path
mise run cli-gate-regression /custom/path  # override corpus directory
```

The task depends on `mise run build` (compiles `x3d_cli_gate` and `x3d_canon_gate` if needed).

**The absent-corpus footgun.** If the corpus directory does not exist, the harnesses exit 1
immediately rather than silently reporting zero checks (which would be a false-green). This is
an explicit design decision: a missing corpus is never allowed to make the gate vacuously pass.
Run `mise run corpus-fetch` first to populate one — it pulls the curated subset **plus its
transitive `Inline`/`EXTERNPROTO` dependencies** into `.x3d-corpus/` (the default the gate
tasks read), in a few seconds. A bare subset without the dependency closure would report false
regressions, because validating a scene resolves the inlines/protos it pulls in.

**Informative (non-CI) mode.**

```bash
mise run cli-gate        # informative: produces divergence-report.md without failing
mise run canon-gate      # informative: Tier-1 + Tier-2 + Tier-3 rates, no baseline
```

**Regenerating a baseline when divergences are intentionally accepted.**

After verifying that a new divergence is correct behavior (not a regression):

```bash
mise run cli-gate-baseline
# Review the updated TSVs, then:
git add tools/x3d-cli/goldens/cli-gate-baseline.tsv
git add tools/x3d-cli/goldens/canon-gate-baseline.tsv
git commit -m "chore(gate): accept new baseline divergences — <reason>"
```

`cli-gate-baseline` runs both harnesses with `--write-baseline`, overwriting the committed
TSVs. The commit locks the new allowed set as the regression floor.

**Generating fresh X3DJSAIL golden fixtures** (requires JDK 25 and the corpus archive):

```bash
mise run cli-golden-gen    # validate verdicts via X3DJSAIL -validate
mise run canon-golden-gen  # canonical reference fixtures via X3DJSAIL -canonical
```

These are one-time (or after expanding the corpus subset) — not part of routine CI.

---

## Gate 4: DOCS (`mise run docs-build`)

**What it protects.** The wiki (`docs/wiki/`) is built with MkDocs and `--strict`. Under
`--strict`, MkDocs fails on:

- Broken internal relative links (`.md` hrefs that resolve to non-existent files)
- Pages present on disk but absent from the `nav:` section of `mkdocs.yml`

This keeps the wiki navigable and prevents link rot as pages are added or renamed.

**Running it.**

```bash
mise run docs-build   # strict build — the gate
mise run docs         # local serve with live-reload at http://127.0.0.1:8000
```

**Adding a new wiki page.** Per the [conventions](../_conventions/CONVENTIONS.md):

1. Create the `.md` file under `docs/wiki/`.
2. Add it to the correct `nav:` section in `mkdocs.yml`.
3. Run `mise run docs-build` — the strict build must pass before committing.

Cross-linking: use relative `.md` paths resolvable from the file's own location. Do not link
to pages that do not yet exist in `nav:` — `--strict` will fail.

---

## Running all gates together

```bash
mise run ci
```

`ci` depends on `test`, `golden`, `conformance-gate`, `coverage-gate`, `doc-ctest-gate`, `build`, and `cli-gate-regression` (in
that order, with the C++ build providing the compiled gate binaries). The DOCS gate
(`docs-build`) is separate and should also be run before any wiki PR merge.

---

## Quick-reference: when to regenerate each baseline

| Situation | Action |
|---|---|
| Intentional codegen change (template/emitter edit) | `uv run x3d-cpp-gen --out generated_cpp_bindings` then commit |
| Conformance finding raised, updated, or closed | Edit `findings.yaml` + `mise run conformance` then commit both |
| CLI/canon behavior intentionally changed | `mise run cli-gate-baseline` then commit the updated TSV(s) |
| New wiki page added | Add to `mkdocs.yml` nav + `mise run docs-build` to verify |
