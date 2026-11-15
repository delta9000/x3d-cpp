---
title: Documentation conventions
summary: Choose a page's purpose, write for its reader, and follow the documentation build and review rules.
tags: [reference, conventions, meta]
updated: 2026-11-15
related:
  - subsystems/_template.md
  - ../explanation/decisions/0000-template.md
---

# Documentation conventions

The main documentation lives in `docs/wiki/`, organised by
[Diataxis](https://diataxis.fr/). Each page serves one reader need: learning,
completing a task, looking up a contract, or understanding a design.

`mkdocs build --strict` (`mise run docs-build`) rejects broken internal page
and heading links and pages omitted from navigation. Frontmatter and prose
conventions below require review; the build does not enforce them.

## Where a page goes

| Directory | Reader need | What to include |
|---|---|---|
| `tutorials/` | Learn by building something that works | A clear starting point, one guided sequence, and observable checkpoints; choose a path for the learner |
| `how-to/` | Complete one named task | Prerequisites, actions, and a completion check for a reader who knows the basics; link to reference for detailed rules |
| `reference/` | Look up exact behavior | Contracts, parameters, defaults, limits, and errors; use short examples when they clarify a rule |
| `reference/subsystems/` | Look up one subsystem's contract | The [subsystem template](subsystems/_template.md) |
| `reference/examples/` | Find an example program's capabilities and options | What it covers, how to build it, and its options |
| `explanation/` | Understand how and why a design works | Concepts, relationships, tradeoffs, and diagrams; link to separate procedures for tasks |
| `explanation/decisions/` | Understand one binding design decision | The [ADR template](../explanation/decisions/0000-template.md) |
| `sai/` | Work with the SAI SDK | The same four directories inside `sai/` |

Choose the page kind by what the reader needs to do, then choose the format
that makes the material clear. Use tables to compare cases, numbered steps
for a sequence, and paragraphs to develop an explanation.

When adding or revising a page:

- New material goes into the matching directory, the `mkdocs.yml` nav, and the
  landing page (`index.md`, or `sai/index.md` for SAI material).
- Keep authoritative behavior and detailed contracts in reference, and link
  to them from tutorials, how-to guides, and explanation.
- If a page takes on a second purpose, split that material into a linked page.
  A short example that clarifies the current purpose can stay.
- Keep tutorials and how-to guides usable from their stated starting point.
  Name required files, tools, and working directories before using them.
- Verify commands by running them. Show the result readers should check, using
  actual output when it helps them recognise success.

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
| `summary` | One sentence describing what the reader can learn or do on the page |
| `tags` | Lowercase and hyphenated; include the page kind (`tutorial`, `how-to`, `reference`, `explanation`, `adr`, `subsystem`) |
| `updated` | The date the content changed, not the git date |
| `related` | Pages a reader should also visit. Every entry also appears as an in-page link |

The theme does not display `related` metadata. Add the links to the page body
so readers can follow them and the build can validate them.

## Links

| Target | Write it as |
|---|---|
| Another page in `docs/wiki/` | A relative Markdown link with the `.md` extension, resolvable from the file's own location |
| A repository file outside `docs/wiki/` that the reader needs to open (source, registers, READMEs) | A GitHub `blob/main/` link, or `tree/main/` for a directory; pin a commit when the text describes a historical revision |
| A repository path used in a command or mentioned without a need to open it | An inline-code path |
| A removed spec or plan | A GitHub link pinned to commit `db23737` |

Do not use absolute or site-root-relative paths (`/architecture/`) or relative
links outside `docs/wiki/`: they do not resolve in the built site. External
GitHub URLs are not checked by MkDocs; verify their targets when adding them.
Check incoming anchors before renaming a heading.

## Nav

The `nav:` section of `mkdocs.yml` is the single source of truth for
navigation. A page omitted from nav raises a warning, which fails a strict
build.

To add a page:

1. Create the `.md` file in the matching directory.
2. Add it to the matching `nav:` section in `mkdocs.yml`.
3. Add it to the landing page list.
4. Run `mise run docs-build`.

## Commands in pages

`scripts/doc_ctest_gate.py` checks documented `ctest ... -R <pattern>` commands
whose pattern contains `x3d_` against the CTest registrations in the CMake
sources. Doctest case names are not CTest targets; to select a case, run the
test binary with `--test-case`.

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
