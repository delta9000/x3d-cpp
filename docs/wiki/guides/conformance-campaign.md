---
title: Conformance audits
summary: Record normative requirements, code evidence, and unresolved behavior.
tags: [guide, conformance]
updated: 2026-11-05
related: []
---

# Conformance audits

Conformance audits compare observable behavior with the applicable ISO clause.
Structural validation, successful parsing, and detected System registration
are separate evidence. The [requirements audit](x3d4-requirements-audit.md)
sets the claim boundaries; [Conformance infrastructure](../subsystems/conformance-infra.md)
describes the generated model.

## Record ownership

`docs/conformance/findings.yaml` owns behavioral and extraction judgments.
Its file header defines the schema. The generated `INDEX.md`, `model.json`,
and `components/*.md` are views; don't edit them directly.
`docs/conformance/sai-services.yaml` separately records SAI service evidence.

The view generator reads the bindings and runtime wiring, then joins those
facts with findings. Its source inspection is a discovery aid, not a proof of
behavior. A behavioral node with wiring and an open finding is partial. An
explicit open finding can mark it inert. Otherwise it remains unverified;
closed findings alone do not establish complete conformance.

## Audit a requirement

Use this procedure for each requirement:

1. Read the full normative clause, its profile exceptions, and the applicable
   edition. Use [RAG tools](rags.md) to locate prose and implementation, then
   inspect the actual sources.
2. Identify the SDK or host responsible for the behavior. Name required
   backends and configuration. Distinguish field storage, events, extraction,
   and presentation.
3. Trace the code and existing assertions. For a suspected gap, retain a
   focused reproduction and an observable expected result.
4. Add a finding with a stable identifier, summary, severity, status, and at
   least one node or interface. Include the clause, source evidence, and
   acceptance conditions. Record ambiguous interpretations explicitly.
5. After changing the register, regenerate its view with `mise run conformance`
   and check it with `mise run conformance-gate`. Review the register and view
   together.

## Close or defer a finding

Before closing a finding, verify its required behavior through a regression
with an independent expected result. Record the evidence and closing commit.
If only part is fixed, keep the remaining requirement explicit in that finding
or a linked finding. Retain fixed and closed records rather than reusing IDs.

Use `deferred` with a concrete reason and acceptance condition. Deferred records
remain in open totals. Before rejecting a reported defect, re-read the full
clause; a contradictory sentence or a peer implementation alone does not settle
an interpretation.

Schema and generated-view checks establish register consistency. They do not
establish semantic completeness, full profile coverage, or host certification.
