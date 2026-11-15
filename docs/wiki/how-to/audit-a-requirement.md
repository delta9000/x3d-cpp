---
title: Audit a requirement
summary: Record normative requirements, code evidence, and unresolved behavior.
tags: [how-to, conformance]
updated: 2026-11-05
related: []
---

# Audit a requirement

Use this to check one ISO requirement against the code and record the result
in `docs/conformance/findings.yaml`. What the register and its generated view
can and cannot establish is in
[Conformance infrastructure](../reference/subsystems/conformance-infra.md); the
claim boundaries are in the [requirements audit](../reference/x3d4-requirements-audit.md).

## Check the requirement

1. Read the full normative clause, its profile exceptions and the edition that
   applies. Use the [RAG tools](search-with-the-rags.md) to find the prose and
   the implementation, then read the actual sources.
2. Name the SDK part or host that owns the behavior, and any backend or
   configuration it needs. Keep field storage, events, extraction and
   presentation apart.
3. Trace the code and the existing assertions. For a suspected gap, keep a
   focused reproduction with an observable expected result.

## Record a finding

1. Add an entry to `docs/conformance/findings.yaml` with a stable identifier,
   summary, severity, status and at least one node or interface. Include the
   clause, the source evidence and the acceptance conditions. Write down any
   ambiguous interpretation. The file header defines the schema.
2. Regenerate the view and check it:

    ```sh
    mise run conformance
    mise run conformance-gate
    ```

3. Review the register and the regenerated view together. Never edit
   `INDEX.md`, `model.json` or `components/*.md` by hand.

## Close or defer a finding

- Close a finding only after a regression with an independent expected result
  proves the required behavior. Record the evidence and the closing commit.
- If only part is fixed, keep the remaining requirement in that finding or a
  linked one. Never reuse an ID.
- To defer, set `deferred` with a concrete reason and acceptance condition.
  Deferred findings still count as open.
- Before rejecting a reported defect, re-read the whole clause. A contradictory
  sentence or another implementation's behavior does not settle the
  interpretation on its own.
