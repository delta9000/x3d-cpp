---
title: Gate system
summary: What the verification gates check and where their definitions live.
tags: [reference, gates]
updated: 2026-11-05
related: []
---

# Gate system

Run `mise tasks` for the task catalog. `mise.toml` owns task commands and
aggregate dependencies; `.github/workflows/ci.yml` owns the GitHub job selection.
Local aggregates and GitHub jobs do not select every optional configuration.

## Generated bindings

`tests/test_golden_tree.py` compares regenerated `*.hpp` and `*.cpp` files with
`generated_cpp_bindings/` in both directions. Missing, extra, or changed files
fail. `scripts/check_golden.sh` runs that comparison for `mise run golden`.
The Python suite shares one regenerated tree; `ci` does not run a second
standalone golden comparison.

The formatter must match the full version pinned in `mise.toml`. CI, `test`,
and `golden` fail on a missing or mismatched formatter. Ordinary local pytest
can skip the golden test in that case. See
[Golden regeneration](../how-to/regenerate-goldens.md) for intentional changes.

## Conformance records

- `docs/conformance/findings.yaml` owns conformance judgments.
- `scripts/conformance_view.py check` validates records and compares a
  temporary regeneration with the committed view.
- It rejects unknown node/interface references and generated-file drift.
- It does not execute normative behavior.
- See [Conformance audits](../how-to/audit-a-requirement.md) for the evidence
  workflow and [Conformance infrastructure](subsystems/conformance-infra.md)
  for the model.

## Corpus regressions

- `mise run cli-gate-regression` builds the CLI and its two differential
  harnesses.
- It rejects failures of items recorded as passing in the committed CLI and
  canonical baselines.
- Missing corpus directories fail; populate the corpus with `mise run
  corpus-fetch` or pass its directory to the regression task.
- The fetcher writes to `.x3d-corpus/`, including the selected scenes' Inline
  and EXTERNPROTO dependencies; `X3D_CORPUS_DIR` selects another root.

Canonical tier 1 checks idempotence on every input and needs no baseline rows.
Tiers 2 and 3 compare with local X3DJSAIL canonical fixtures. Without those
fixtures, the harness reports tier-1-only coverage. Generate them with
`mise run canon-golden-gen` when those comparisons are needed. A tier-1 result
alone does not establish canonical agreement with X3DJSAIL.

After reviewing an intentional divergence, `mise run cli-gate-baseline`
refreshes the two baseline TSVs under `tools/x3d-cli/goldens/`. Review those
diffs before accepting them. An incomplete tier-2 run cannot overwrite an
existing tier-2 baseline with empty results.

## Documentation

`mise run docs-build` invokes MkDocs with `--strict`. Warnings fail the build;
inspect informational messages too, since strict mode does not turn every
navigation or anchor diagnostic into a warning. Keep pages in `mkdocs.yml` and
check incoming anchors before changing headings.

`python3 scripts/doc_ctest_gate.py` checks that documented `ctest -R x3d_*`
patterns match a CTest registration in the CMake sources. It includes gated
registrations; it does not prove that an optional target is configured locally.

## Consumer and authoring checks

`tools/tests/x3d_cli_test.sh` validates the gallery scenes and checks that
extraction produces triangles. `scripts/validate-examples.sh` builds example
consumers and checks selected renders for non-background pixels, alongside
their behavior tests. These checks do not establish full renderer conformance.

`scripts/authoring-footprint.sh` rejects excluded subsystem symbols in the
slim authoring consumer. It has no absolute executable-size threshold.
