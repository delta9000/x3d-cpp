---
title: Wiki Conventions
summary: Frontmatter schema, link style, and nav conventions for the x3d-cpp wiki.
tags: [conventions, meta]
updated: 2026-06-20
related: []
---

# Wiki Conventions

This page defines the conventions every wiki page MUST follow. Wave-1 slice authors read this before authoring. These conventions are enforced by `mkdocs build --strict` (dead links, orphans) and spot-checked by the review gate.

## Frontmatter schema

Every wiki page starts with a YAML frontmatter block. All fields are required:

```yaml
---
title: "Human-readable page title"
summary: "One-sentence summary of what this page covers."
tags: [tag1, tag2]          # lowercase, hyphenated; e.g. [runtime, events, adr]
updated: YYYY-MM-DD         # date this page was last meaningfully updated
related:                    # relative paths from docs/wiki/ root; may be empty list
  - architecture.md
  - decisions/0001-ext-firewall.md
---
```

**Rules:**
- `title` must match the `# H1` heading that immediately follows the frontmatter.
- `tags` are lowercase and hyphenated; no spaces.
- `updated` is the date the *content* changed, not the git date (they may diverge).
- `related` lists pages that a reader should also visit; relative links from `docs/wiki/` root.

## Relative-link style

All internal links use relative markdown paths, resolvable from the file's own location.

```markdown
<!-- In docs/wiki/index.md, linking to architecture -->
[Architecture](architecture.md)

<!-- In docs/wiki/decisions/0001-ext-firewall.md, linking to architecture -->
[Architecture](../architecture.md)

<!-- Referencing an artifact OUTSIDE docs/wiki/ (a dated spec, source file, BACKLOG):
     cite it as an inline-code PATH, NOT a markdown link — mkdocs --strict cannot
     resolve links outside docs_dir and will fail the build. -->
See the design at `docs/superpowers/specs/2026-06-20-project-wiki-design.md`.
```

**Rules:**
- **Hyperlink ONLY within `docs/wiki/`.** Anything outside it (dated specs/plans in `docs/superpowers/`, source files, `BACKLOG.md`, the conformance view) is cited as an **inline-code path** (`` `docs/superpowers/specs/…md` ``) — greppable and IDE-clickable, but NOT a live href. `docs_dir` is `docs/wiki`, so out-of-tree links break `mkdocs build --strict` (the docs gate). Rendering the dated specs into the site so they can be linked is a possible future change; until then, cite by path.
- Use `.md` extensions in in-wiki link hrefs (MkDocs resolves these correctly).
- Do NOT use absolute paths or site-root-relative paths (`/architecture/`).
- Reference dated specs/plans as the historical record (cite by path); do NOT duplicate their content.
- Every page mentioned in `related:` frontmatter MUST also appear as an in-page link.

## How nav works

The `nav:` section of `mkdocs.yml` is the single source of truth for navigation. Every page that should appear in the sidebar MUST be listed there. Pages NOT in nav will trigger a warning under `--strict` build.

To add a new page:
1. Create the `.md` file under `docs/wiki/`.
2. Add it to the appropriate `nav:` section in `mkdocs.yml`.
3. Run `mise run docs-build` to verify the strict build still passes.

## ADR convention

Decisions live in `docs/wiki/decisions/`. Use the `adr-template.md` template. Number ADRs sequentially: `0001-`, `0002-`, etc. The slug after the number is a short hyphenated description of the decision topic.

## Subsystem page convention

Subsystem pages live in `docs/wiki/subsystems/`. Use the `subsystem-template.md` template. The slug is the subsystem's canonical name (e.g. `cli-suite`, `event-cascade`, `extract`).

## Accuracy rule

Every factual claim in a wiki page (file path, function name, API, behavior) MUST be verifiable against the current codebase or a linked spec. No invented APIs or paths. Wave-2 verify stage cross-checks each page via `code_rag`/file reads.
