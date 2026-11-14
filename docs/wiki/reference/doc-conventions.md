---
title: Documentation conventions
summary: Where a page goes, how each kind of page is written, and the frontmatter, link and nav rules the docs gate enforces.
tags: [reference, conventions, meta]
updated: 2026-11-14
related:
  - subsystems/_template.md
  - ../explanation/decisions/0000-template.md
---

# Documentation conventions

All prose documentation lives in one tree, `docs/wiki/`, organised by
[Diataxis](https://diataxis.fr/). Each page serves exactly one purpose.
`mkdocs build --strict` (`mise run docs-build`) enforces links and nav.

## Where a page goes

| Directory | A page there | Written as |
|---|---|---|
| `tutorials/` | Teaches by building something that works | Numbered steps with the expected output after each; no options, no digressions |
| `how-to/` | Gets one named task done | Steps for a reader who already knows the basics; links to reference for the rules |
| `reference/` | States exact behavior | Tables and short bullets; no instructions, no rationale |
| `reference/subsystems/` | Describes one subsystem's contract | The [subsystem template](subsystems/_template.md) |
| `reference/examples/` | Describes one example program | What it covers, how to build it, its options |
| `explanation/` | Says why | Prose; no steps |
| `explanation/decisions/` | Records one binding design decision | The [ADR template](../explanation/decisions/0000-template.md) |
| `sai/` | The SAI SDK, a product of its own | The same four directories inside `sai/` |

Rules:

- New material goes into the matching directory, the `mkdocs.yml` nav, and the
  landing page (`index.md`, or `sai/index.md` for SAI material).
- State a fact once, in reference, and link to it from the other three kinds.
- If a page needs a second purpose, split it.
- Prefer a table to a paragraph that lists cases. A paragraph longer than
  about six lines usually hides a list.
- Verify tutorial and how-to commands by running them. Paste real output.

## What stays outside the tree

| Location | Holds | Rule |
|---|---|---|
| `README.md`, `CONTRIBUTING.md` | The pitch, a quick start and links | Pointers into the tree; no reference material |
| READMEs under `examples/`, `sai/`, `adapters/sai_provider/`, `tests/cmake/sai_*/` | What the directory is and one command | Short pointers into the tree |
| READMEs under `examples/*/assets/` | Asset provenance, licensing and the recipe that regenerates the assets | Stay with the assets; how to run an example goes in `reference/examples/` |
| `README.generator.md` | The PyPI package description for `x3d-cpp-gen` | Packaged metadata; keep self-contained |
| `docs/conformance/` | The findings register and its generated view | Edit the YAML; never hand-edit generated files |
| `sai/docs/conformance/` | The SAI service register and its baseline | Same |
| `CHANGELOG.md` | Release notes | |

The old dated specs and plans (`docs/superpowers/`, `docs/plans/`,
`sai/docs/plans/`) were removed from the tree. Cite one as a GitHub link
pinned to commit `db23737`, as the ADRs do. Don't recreate them.

## Frontmatter

Every page starts with this block. All fields are required.

```yaml
---
title: "Human-readable page title"
summary: "One sentence on what this page covers."
tags: [tag1, tag2]          # lowercase, hyphenated
updated: YYYY-MM-DD         # date the content last changed
related:                    # paths relative to this file; may be []
  - ../explanation/architecture.md
---
```

| Field | Rule |
|---|---|
| `title` | Matches the `# H1` that follows the frontmatter |
| `tags` | Lowercase and hyphenated; include the page kind (`tutorial`, `how-to`, `reference`, `explanation`, `adr`, `subsystem`) |
| `updated` | The date the content changed, not the git date |
| `related` | Pages a reader should also visit. Every entry also appears as an in-page link |

## Links

| Target | Write it as |
|---|---|
| Another page in `docs/wiki/` | A relative Markdown link with the `.md` extension, resolvable from the file's own location |
| Anything else in the repository (source, registers, READMEs) | An inline-code path. `docs_dir` is `docs/wiki`, so an out-of-tree relative link fails the strict build |
| A removed spec or plan | A GitHub link pinned to commit `db23737` |

Do not use absolute or site-root-relative paths (`/architecture/`). Check
incoming anchors before renaming a heading.

## Nav

The `nav:` section of `mkdocs.yml` is the single source of truth for
navigation. A page that is not in nav warns under `--strict`.

To add a page:

1. Create the `.md` file in the matching directory.
2. Add it to the matching `nav:` section in `mkdocs.yml`.
3. Add it to the landing page list.
4. Run `mise run docs-build`.

## Commands in pages

`scripts/doc_ctest_gate.py` checks every `ctest ... -R <pattern>` written in
`docs/wiki/` against the CTest registrations in the CMake sources. Doctest
case names are not CTest targets; to select a case, run the test binary with
`--test-case`.

## ADRs

Number ADRs sequentially (`0001-`, `0002-`) with a short hyphenated slug.
Write one only when you make a binding design decision. Behavior goes in
subsystem pages, not ADRs.

## Subsystem pages

The slug is the subsystem's canonical name (for example `cli-suite`,
`event-cascade`, `extract`). Add the page to the
[source map](source-map.md#subsystem-sources) too.

## Accuracy

Every factual claim (file path, function name, API, behavior) must be
verifiable against the current code or a linked specification. No invented
APIs or paths.
