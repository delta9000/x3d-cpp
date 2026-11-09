---
title: Golden regeneration
summary: Regenerate committed bindings after an intentional generator change.
tags: [guide, golden]
updated: 2026-11-05
related: []
---

# Golden regeneration

`generated_cpp_bindings/` contains committed C++ output from the generator.
Change the templates, emitters, or input model rather than editing generated
files. [Gate system](gate-system.md#generated-bindings) describes the comparison;
[ADR-0005](../decisions/0005-golden-files-in-git.md) records the policy.

## Regenerate and review

After an intentional generator change, run:

```sh
mise install
mise run gen
git diff -- generated_cpp_bindings/ tools/x3d-cli/
mise run golden
```

`gen` updates both the bindings and CLI profile tables. Review emitted changes
alongside the generator change. Keep them together when committing.
The generation fixture used by pytest passes `--no-test`; a direct generator
run can also produce gitignored smoke artifacts. `test.cpp` is excluded from
the full-tree golden comparison.

## Diagnose drift

The comparator distinguishes missing, extra, and byte-different sources.
First check the input model and generator diff, then the formatter:

```sh
mise exec -- clang-format --version
```

The full formatter pin lives in `mise.toml`. `CLANG_FORMAT` overrides executable
selection; otherwise the generator and gate use `clang-format` from PATH.
Formatting-only drift can result from a mismatched version. Correct that
environment before accepting a new golden tree.

Generated-source equality is separate from runtime floating-point behavior and
scene round-trip fidelity. CLI/canonical corpus baselines have their own
[regression gate](gate-system.md#corpus-regressions).
