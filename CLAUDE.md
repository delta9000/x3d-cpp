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
   - `docs/wiki/subsystems/<name>.md` (new subsystem → new page + `mkdocs.yml` nav)
   - `docs/conformance/findings.yaml`, then `mise run conformance` (never hand-edit the
     generated `.md`)
   - `docs/sdk/v1-capabilities.md`
   - root `NOTICE` if you added a third-party dependency or backend
3. If symbols moved: `mise run code-ingest` / `mise run docs-ingest`.

The old dated specs and plans (`docs/superpowers/`, `docs/plans/`, `sai/docs/plans/`) were removed
from the tree; they live in git history at `db23737`. Don't recreate them: decisions go in ADRs,
behaviour in subsystem pages.

## SAI SDK (`sai/`)

`sai/` is the SAI SDK (folded back from the old `x3d-sai` repo, ADR-0076). It stays its own
CMake project and uv package: the runtime build never adds it, and `adapters/sai_provider` is
the only bridge. Its gates are `mise run sai-ci` (part of `ci`) and `mise run sai-provider`
(part of `ci-all`). Its service register is `sai/docs/conformance/`; regenerate the baseline
with `mise run sai-baseline`. `docs/conformance/sai-services.yaml` is the runtime's separate
Script-SAI register.

Its docs are one Diataxis tree, `docs/wiki/sai/` (tutorials, how-to, reference, explanation;
rules in `docs/wiki/_conventions/CONVENTIONS.md`). Put new SAI material there. The READMEs
under `sai/`, `adapters/sai_provider/`, `examples/sai_host/` and `tests/cmake/sai_*/` are short
pointers into it, and `sai/docs/` holds only the register.

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
