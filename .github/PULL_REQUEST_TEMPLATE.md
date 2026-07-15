## What and why

<!-- One paragraph. Link the card/issue. -->

## Conformance claim

<!-- What does this make true that wasn't? If it changes spec-visible behavior,
     link the docs/conformance/findings.yaml entry. If it makes no conformance
     claim, say so -- "none: packaging metadata only" is a complete answer. -->

## Checklist

- [ ] `mise run ci` is green locally
- [ ] A test fails without this change — **or** N/A with a reason:
      <!-- Docs-only, packaging-metadata and repo-policy changes have no runtime
           surface to test. Say which, e.g. "N/A: wording only, no behavior
           change" or "N/A: CI config; verified by the run on this PR". Do not
           tick this box for a change that COULD have had a test. -->
- [ ] Docs updated in this diff (`mise run docs-drift working` lists candidates)
- [ ] If a third-party dep/backend was added: root `NOTICE` updated
- [ ] If symbols moved: `mise run code-ingest` / `mise run docs-ingest` refreshed
- [ ] If public surface changed: `mise run validate-examples` (not in `mise run ci`)
