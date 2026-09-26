# x3d-cpp — project instructions

Headless, renderer-agnostic X3D domain-runtime SDK. One-person project: correctness matters,
process doesn't. Task runner is **mise** (`mise tasks`); the in-repo wiki (`docs/wiki/`,
`mise run docs`) is the knowledge home.

Work is: branch off `main`, make the change, `mise run ci` green, docs updated in the same
diff, PR. To-dos are plain GitHub issues; conformance gaps go in
`docs/conformance/findings.yaml`. No specs, plans, or cards required — write an ADR
(`docs/wiki/decisions/NNNN-*.md`) only when you make a binding design decision.

## Docs are part of the diff

The docs have drifted from the code before (`physics.md` claimed "no events surfaced" after
contact reporting shipped; Followers shipped with no page). When you change `runtime/`,
`tools/`, or `include/x3d/`:

1. Run `mise run docs-drift` (uncommitted work; or `working` / `<rev>`). Check the `CITES`
   hits first; a `[NEW FILE]` with no citations probably needs a page. Advisory only.
2. Update what it flags in the same change:
   - `docs/wiki/subsystems/<name>.md` (new subsystem → new page + `mkdocs.yml` nav +
     `docs/wiki/coverage.md` row)
   - `docs/conformance/findings.yaml`, then `mise run conformance` (never hand-edit the
     generated `.md`)
   - `docs/sdk/v1-capabilities.md`
   - root `NOTICE` if you added a third-party dependency or backend
3. If symbols moved: `mise run code-ingest` / `mise run docs-ingest`.

`docs/superpowers/` and `docs/plans/` are a frozen archive of old specs and plans. Cite them,
don't edit them, don't add to them.

## Verify before claiming "shipped / covered"

Don't relay doc language about what's done — read the code and list what's actually wired
vs. ignored. For partial seams, name the ignored fields.

## Gates

`mise run ci` is what hard-fails (`mise tasks info ci` lists its parts). `mise run ci-all`
adds sanitizers and the example renderers; run it before pushing risky C++ changes.
`mise run docs-build` is the wiki's strict gate (dead links, nav orphans).

## Commits

No `Claude-Session:` trailers or `claude.ai/code/session_…` URLs in commits or PR bodies.
Keep them tool-agnostic.
