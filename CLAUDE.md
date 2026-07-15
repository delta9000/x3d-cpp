# x3d-cpp — project instructions

Headless, renderer-agnostic X3D domain-runtime SDK. Task runner is **mise** (`mise tasks`);
the canonical knowledge home is the in-repo wiki (`docs/wiki/`, served by `mise run docs`).

**How work flows here.** Tasks come from the [GitHub Project](https://github.com/users/delta9000/projects/2)
(`scripts/pick-card.sh --list`). Take a card → documented completion via
`docs/contributor/card-to-done-workflow.md` (Definition of Ready/Done; the
card→issue→branch→PR→docs→Done chain). Multi-agent execution discipline:
`docs/contributor/workflow-subagent-discipline.md`.

## Docs are part of the diff (anti-drift discipline)

The docs have drifted from the code before (e.g. `physics.md` claimed "no events surfaced"
after contact-reporting shipped; the Followers runtime shipped with no doc page at all).
To stop that, treat the **living docs as part of any code change** — not a follow-up.

When you change code under `runtime/`, `tools/`, or `include/x3d/`, before you call the
work done:

1. **Run the drift suggester** on your change — ideally *before* you commit:
   ```
   mise run docs-drift            # no arg = your uncommitted work (HEAD if the tree is clean)
   mise run docs-drift working    # force the uncommitted-vs-HEAD check (incl. untracked files)
   mise run docs-drift <rev>      # a specific commit
   ```
   It RAGs the diff against the living docs and prints a review-list. **Review the `CITES`
   hits first** — a doc that names the code you touched is the most likely to be stale; a
   `[NEW FILE]` with no citations probably needs a brand-new page. It is advisory (always
   exits 0); the judgment call is yours.

2. **Update the docs it flags, in the same change.** The living docs that track code:
   - `docs/wiki/subsystems/<name>.md` — the subsystem page (create one if you shipped a
     new subsystem; add it to `mkdocs.yml` nav + a `docs/wiki/coverage.md` row).
   - `docs/wiki/coverage.md` — subsystem/ADR/guide coverage table + counts.
   - `docs/conformance/findings.yaml` — the conformance source of truth (then
     `mise run conformance` regenerates the view; never hand-edit the generated `.md`).
   - `docs/sdk/v1-capabilities.md` — capability claims.
   - `docs/wiki/decisions/NNNN-*.md` — a new ADR if you made a binding design decision.

   `docs/superpowers/` (dated specs/plans) is the **historical record** — cite it, don't
   edit it.

   **Deferral tracking (no longer a doc):** `docs/superpowers/BACKLOG.md` is **deprecated**
   (2026-06-22). Deferrals now live in two complementary trackers — pick by kind:
   - *Behavioral / spec-conformance* gaps → `docs/conformance/findings.yaml` (above).
   - *Engineering / planning* deferrals (build, codec, extraction, SDK, seams) → the
     [GitHub Project](https://github.com/users/delta9000/projects/2) (`gh project item-list 2 --owner delta9000`;
     create needs the `project` auth scope). Don't reopen BACKLOG.md as a tracker.

3. **Refresh the RAG stores when symbols move** so search/drift stays accurate:
   `mise run code-ingest` (C++) and `mise run docs-ingest` (docs).

## Verify before claiming "shipped / covered"

Don't relay doc language about what's done — read the code and enumerate what's actually
wired vs. ignored (this is how the `physics.md` and Followers gaps were caught). For
partial seams, list the ignored fields explicitly.

## Gates (what actually hard-fails)

`mise run ci` runs tests + golden + `conformance-gate` + build + cli-gate-regression.
The wiki's strict gate is `mise run docs-build` (dead links / nav orphans). Drift that a
gate *can't* catch (semantic staleness) is what `mise run docs-drift` is for.

## Commit conventions

Do **not** put `Claude-Session:` trailers or any `claude.ai/code/session_…` URLs in commit
messages or PR bodies — this is a shared repo and that history is public to collaborators.
Keep commit messages and PR bodies tool-agnostic.
